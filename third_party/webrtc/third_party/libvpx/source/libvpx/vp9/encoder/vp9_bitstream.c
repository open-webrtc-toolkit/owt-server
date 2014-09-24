/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/common/vp9_header.h"
#include "vp9/encoder/vp9_encodemv.h"
#include "vp9/common/vp9_entropymode.h"
#include "vp9/common/vp9_entropymv.h"
#include "vp9/common/vp9_findnearmv.h"
#include "vp9/encoder/vp9_mcomp.h"
#include "vp9/common/vp9_systemdependent.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include "vp9/common/vp9_pragmas.h"
#include "vpx/vpx_encoder.h"
#include "vpx_mem/vpx_mem.h"
#include "vp9/encoder/vp9_bitstream.h"
#include "vp9/encoder/vp9_segmentation.h"

#include "vp9/common/vp9_seg_common.h"
#include "vp9/common/vp9_pred_common.h"
#include "vp9/common/vp9_entropy.h"
#include "vp9/encoder/vp9_encodemv.h"
#include "vp9/common/vp9_entropymv.h"
#include "vp9/common/vp9_mvref_common.h"
#include "vp9/common/vp9_treecoder.h"

#if defined(SECTIONBITS_OUTPUT)
unsigned __int64 Sectionbits[500];
#endif

#ifdef ENTROPY_STATS
int intra_mode_stats[VP9_KF_BINTRAMODES]
                    [VP9_KF_BINTRAMODES]
                    [VP9_KF_BINTRAMODES];
vp9_coeff_stats tree_update_hist_4x4[BLOCK_TYPES_4X4];
vp9_coeff_stats hybrid_tree_update_hist_4x4[BLOCK_TYPES_4X4];
vp9_coeff_stats tree_update_hist_8x8[BLOCK_TYPES_8X8];
vp9_coeff_stats hybrid_tree_update_hist_8x8[BLOCK_TYPES_8X8];
vp9_coeff_stats tree_update_hist_16x16[BLOCK_TYPES_16X16];
vp9_coeff_stats hybrid_tree_update_hist_16x16[BLOCK_TYPES_16X16];
vp9_coeff_stats tree_update_hist_32x32[BLOCK_TYPES_32X32];

extern unsigned int active_section;
#endif

#ifdef MODE_STATS
int count_mb_seg[4] = { 0, 0, 0, 0 };
#endif

#define vp9_cost_upd  ((int)(vp9_cost_one(upd) - vp9_cost_zero(upd)) >> 8)
#define vp9_cost_upd256  ((int)(vp9_cost_one(upd) - vp9_cost_zero(upd)))

#define SEARCH_NEWP
static int update_bits[255];

static void compute_update_table() {
  int i;
  for (i = 0; i < 255; i++)
    update_bits[i] = vp9_count_term_subexp(i, SUBEXP_PARAM, 255);
}

static int split_index(int i, int n, int modulus) {
  int max1 = (n - 1 - modulus / 2) / modulus + 1;
  if (i % modulus == modulus / 2) i = i / modulus;
  else i = max1 + i - (i + modulus - modulus / 2) / modulus;
  return i;
}

static int remap_prob(int v, int m) {
  const int n = 256;
  const int modulus = MODULUS_PARAM;
  int i;
  if ((m << 1) <= n)
    i = vp9_recenter_nonneg(v, m) - 1;
  else
    i = vp9_recenter_nonneg(n - 1 - v, n - 1 - m) - 1;

  i = split_index(i, n - 1, modulus);
  return i;
}

static void write_prob_diff_update(vp9_writer *const bc,
                                   vp9_prob newp, vp9_prob oldp) {
  int delp = remap_prob(newp, oldp);
  vp9_encode_term_subexp(bc, delp, SUBEXP_PARAM, 255);
}

static int prob_diff_update_cost(vp9_prob newp, vp9_prob oldp) {
  int delp = remap_prob(newp, oldp);
  return update_bits[delp] * 256;
}

static void update_mode(
  vp9_writer *const bc,
  int n,
  vp9_token tok               [/* n */],
  vp9_tree tree,
  vp9_prob Pnew               [/* n-1 */],
  vp9_prob Pcur               [/* n-1 */],
  unsigned int bct            [/* n-1 */] [2],
  const unsigned int num_events[/* n */]
) {
  unsigned int new_b = 0, old_b = 0;
  int i = 0;

  vp9_tree_probs_from_distribution(n--, tok, tree,
                                   Pnew, bct, num_events);

  do {
    new_b += cost_branch(bct[i], Pnew[i]);
    old_b += cost_branch(bct[i], Pcur[i]);
  } while (++i < n);

  if (new_b + (n << 8) < old_b) {
    int i = 0;

    vp9_write_bit(bc, 1);

    do {
      const vp9_prob p = Pnew[i];

      vp9_write_literal(bc, Pcur[i] = p ? p : 1, 8);
    } while (++i < n);
  } else
    vp9_write_bit(bc, 0);
}

static void update_mbintra_mode_probs(VP9_COMP* const cpi,
                                      vp9_writer* const bc) {
  VP9_COMMON *const cm = &cpi->common;

  {
    vp9_prob Pnew   [VP9_YMODES - 1];
    unsigned int bct [VP9_YMODES - 1] [2];

    update_mode(
      bc, VP9_YMODES, vp9_ymode_encodings, vp9_ymode_tree,
      Pnew, cm->fc.ymode_prob, bct, (unsigned int *)cpi->ymode_count
    );
    update_mode(bc, VP9_I32X32_MODES, vp9_sb_ymode_encodings,
                vp9_sb_ymode_tree, Pnew, cm->fc.sb_ymode_prob, bct,
                (unsigned int *)cpi->sb_ymode_count);
  }
}

void vp9_update_skip_probs(VP9_COMP *cpi) {
  VP9_COMMON *const pc = &cpi->common;
  int k;

  for (k = 0; k < MBSKIP_CONTEXTS; ++k) {
    pc->mbskip_pred_probs[k] = get_binary_prob(cpi->skip_false_count[k],
                                               cpi->skip_true_count[k]);
  }
}

static void update_switchable_interp_probs(VP9_COMP *cpi,
                                           vp9_writer* const bc) {
  VP9_COMMON *const pc = &cpi->common;
  unsigned int branch_ct[32][2];
  int i, j;
  for (j = 0; j <= VP9_SWITCHABLE_FILTERS; ++j) {
    vp9_tree_probs_from_distribution(
        VP9_SWITCHABLE_FILTERS,
        vp9_switchable_interp_encodings, vp9_switchable_interp_tree,
        pc->fc.switchable_interp_prob[j], branch_ct,
        cpi->switchable_interp_count[j]);
    for (i = 0; i < VP9_SWITCHABLE_FILTERS - 1; ++i) {
      if (pc->fc.switchable_interp_prob[j][i] < 1)
        pc->fc.switchable_interp_prob[j][i] = 1;
      vp9_write_literal(bc, pc->fc.switchable_interp_prob[j][i], 8);
    }
  }
}

// This function updates the reference frame prediction stats
static void update_refpred_stats(VP9_COMP *cpi) {
  VP9_COMMON *const cm = &cpi->common;
  int i;
  vp9_prob new_pred_probs[PREDICTION_PROBS];
  int old_cost, new_cost;

  // Set the prediction probability structures to defaults
  if (cm->frame_type == KEY_FRAME) {
    // Set the prediction probabilities to defaults
    cm->ref_pred_probs[0] = 120;
    cm->ref_pred_probs[1] = 80;
    cm->ref_pred_probs[2] = 40;

    vpx_memset(cpi->ref_pred_probs_update, 0,
               sizeof(cpi->ref_pred_probs_update));
  } else {
    // From the prediction counts set the probabilities for each context
    for (i = 0; i < PREDICTION_PROBS; i++) {
      new_pred_probs[i] = get_binary_prob(cpi->ref_pred_count[i][0],
                                          cpi->ref_pred_count[i][1]);

      // Decide whether or not to update the reference frame probs.
      // Returned costs are in 1/256 bit units.
      old_cost =
        (cpi->ref_pred_count[i][0] * vp9_cost_zero(cm->ref_pred_probs[i])) +
        (cpi->ref_pred_count[i][1] * vp9_cost_one(cm->ref_pred_probs[i]));

      new_cost =
        (cpi->ref_pred_count[i][0] * vp9_cost_zero(new_pred_probs[i])) +
        (cpi->ref_pred_count[i][1] * vp9_cost_one(new_pred_probs[i]));

      // Cost saving must be >= 8 bits (2048 in these units)
      if ((old_cost - new_cost) >= 2048) {
        cpi->ref_pred_probs_update[i] = 1;
        cm->ref_pred_probs[i] = new_pred_probs[i];
      } else
        cpi->ref_pred_probs_update[i] = 0;

    }
  }
}

// This function is called to update the mode probability context used to encode
// inter modes. It assumes the branch counts table has already been populated
// prior to the actual packing of the bitstream (in rd stage or dummy pack)
//
// The branch counts table is re-populated during the actual pack stage and in
// the decoder to facilitate backwards update of the context.
static void update_mode_probs(VP9_COMMON *cm,
                              int mode_context[INTER_MODE_CONTEXTS][4]) {
  int i, j;
  unsigned int (*mv_ref_ct)[4][2];

  vpx_memcpy(mode_context, cm->fc.vp9_mode_contexts,
             sizeof(cm->fc.vp9_mode_contexts));

  mv_ref_ct = cm->fc.mv_ref_ct;

  for (i = 0; i < INTER_MODE_CONTEXTS; i++) {
    for (j = 0; j < 4; j++) {
      int new_prob, old_cost, new_cost;

      // Work out cost of coding branches with the old and optimal probability
      old_cost = cost_branch256(mv_ref_ct[i][j], mode_context[i][j]);
      new_prob = get_binary_prob(mv_ref_ct[i][j][0], mv_ref_ct[i][j][1]);
      new_cost = cost_branch256(mv_ref_ct[i][j], new_prob);

      // If cost saving is >= 14 bits then update the mode probability.
      // This is the approximate net cost of updating one probability given
      // that the no update case ismuch more common than the update case.
      if (new_cost <= (old_cost - (14 << 8))) {
        mode_context[i][j] = new_prob;
      }
    }
  }
}

#if CONFIG_NEW_MVREF
static void update_mv_ref_probs(VP9_COMP *cpi,
                                int mvref_probs[MAX_REF_FRAMES]
                                               [MAX_MV_REF_CANDIDATES-1]) {
  MACROBLOCKD *xd = &cpi->mb.e_mbd;
  int rf;     // Reference frame
  int ref_c;  // Motion reference candidate
  int node;   // Probability node index

  for (rf = 0; rf < MAX_REF_FRAMES; ++rf) {
    int count = 0;

    // Skip the dummy entry for intra ref frame.
    if (rf == INTRA_FRAME) {
      continue;
    }

    // Sum the counts for all candidates
    for (ref_c = 0; ref_c < MAX_MV_REF_CANDIDATES; ++ref_c) {
      count += cpi->mb_mv_ref_count[rf][ref_c];
    }

    // Calculate the tree node probabilities
    for (node = 0; node < MAX_MV_REF_CANDIDATES-1; ++node) {
      int new_prob, old_cost, new_cost;
      unsigned int branch_cnts[2];

      // How many hits on each branch at this node
      branch_cnts[0] = cpi->mb_mv_ref_count[rf][node];
      branch_cnts[1] = count - cpi->mb_mv_ref_count[rf][node];

      // Work out cost of coding branches with the old and optimal probability
      old_cost = cost_branch256(branch_cnts, xd->mb_mv_ref_probs[rf][node]);
      new_prob = get_prob(branch_cnts[0], count);
      new_cost = cost_branch256(branch_cnts, new_prob);

      // Take current 0 branch cases out of residual count
      count -= cpi->mb_mv_ref_count[rf][node];

      if ((new_cost + VP9_MV_REF_UPDATE_COST) <= old_cost) {
        mvref_probs[rf][node] = new_prob;
      } else {
        mvref_probs[rf][node] = xd->mb_mv_ref_probs[rf][node];
      }
    }
  }
}
#endif

static void write_ymode(vp9_writer *bc, int m, const vp9_prob *p) {
  write_token(bc, vp9_ymode_tree, p, vp9_ymode_encodings + m);
}

static void kfwrite_ymode(vp9_writer *bc, int m, const vp9_prob *p) {
  write_token(bc, vp9_kf_ymode_tree, p, vp9_kf_ymode_encodings + m);
}

static void write_sb_ymode(vp9_writer *bc, int m, const vp9_prob *p) {
  write_token(bc, vp9_sb_ymode_tree, p, vp9_sb_ymode_encodings + m);
}

static void sb_kfwrite_ymode(vp9_writer *bc, int m, const vp9_prob *p) {
  write_token(bc, vp9_uv_mode_tree, p, vp9_sb_kf_ymode_encodings + m);
}

static void write_i8x8_mode(vp9_writer *bc, int m, const vp9_prob *p) {
  write_token(bc, vp9_i8x8_mode_tree, p, vp9_i8x8_mode_encodings + m);
}

static void write_uv_mode(vp9_writer *bc, int m, const vp9_prob *p) {
  write_token(bc, vp9_uv_mode_tree, p, vp9_uv_mode_encodings + m);
}


static void write_bmode(vp9_writer *bc, int m, const vp9_prob *p) {
#if CONFIG_NEWBINTRAMODES
  assert(m < B_CONTEXT_PRED - CONTEXT_PRED_REPLACEMENTS || m == B_CONTEXT_PRED);
  if (m == B_CONTEXT_PRED) m -= CONTEXT_PRED_REPLACEMENTS;
#endif
  write_token(bc, vp9_bmode_tree, p, vp9_bmode_encodings + m);
}

static void write_kf_bmode(vp9_writer *bc, int m, const vp9_prob *p) {
  write_token(bc, vp9_kf_bmode_tree, p, vp9_kf_bmode_encodings + m);
}

static void write_split(vp9_writer *bc, int x, const vp9_prob *p) {
  write_token(
    bc, vp9_mbsplit_tree, p, vp9_mbsplit_encodings + x);
}

static int prob_update_savings(const unsigned int *ct,
                               const vp9_prob oldp, const vp9_prob newp,
                               const vp9_prob upd) {
  const int old_b = cost_branch256(ct, oldp);
  const int new_b = cost_branch256(ct, newp);
  const int update_b = 2048 + vp9_cost_upd256;
  return (old_b - new_b - update_b);
}

static int prob_diff_update_savings(const unsigned int *ct,
                                    const vp9_prob oldp, const vp9_prob newp,
                                    const vp9_prob upd) {
  const int old_b = cost_branch256(ct, oldp);
  const int new_b = cost_branch256(ct, newp);
  const int update_b = (newp == oldp ? 0 :
                        prob_diff_update_cost(newp, oldp) + vp9_cost_upd256);
  return (old_b - new_b - update_b);
}

static int prob_diff_update_savings_search(const unsigned int *ct,
                                           const vp9_prob oldp, vp9_prob *bestp,
                                           const vp9_prob upd) {
  const int old_b = cost_branch256(ct, oldp);
  int new_b, update_b, savings, bestsavings, step;
  vp9_prob newp, bestnewp;

  bestsavings = 0;
  bestnewp = oldp;

  step = (*bestp > oldp ? -1 : 1);
  for (newp = *bestp; newp != oldp; newp += step) {
    new_b = cost_branch256(ct, newp);
    update_b = prob_diff_update_cost(newp, oldp) + vp9_cost_upd256;
    savings = old_b - new_b - update_b;
    if (savings > bestsavings) {
      bestsavings = savings;
      bestnewp = newp;
    }
  }
  *bestp = bestnewp;
  return bestsavings;
}

static void vp9_cond_prob_update(vp9_writer *bc, vp9_prob *oldp, vp9_prob upd,
                                 unsigned int *ct) {
  vp9_prob newp;
  int savings;
  newp = get_binary_prob(ct[0], ct[1]);
  savings = prob_update_savings(ct, *oldp, newp, upd);
  if (savings > 0) {
    vp9_write(bc, 1, upd);
    vp9_write_literal(bc, newp, 8);
    *oldp = newp;
  } else {
    vp9_write(bc, 0, upd);
  }
}

static void pack_mb_tokens(vp9_writer* const bc,
                           TOKENEXTRA **tp,
                           const TOKENEXTRA *const stop) {
  TOKENEXTRA *p = *tp;

  while (p < stop) {
    const int t = p->Token;
    vp9_token *const a = vp9_coef_encodings + t;
    const vp9_extra_bit_struct *const b = vp9_extra_bits + t;
    int i = 0;
    const unsigned char *pp = p->context_tree;
    int v = a->value;
    int n = a->Len;

    if (t == EOSB_TOKEN)
    {
      ++p;
      break;
    }

    /* skip one or two nodes */
    if (p->skip_eob_node) {
      n -= p->skip_eob_node;
      i = 2 * p->skip_eob_node;
    }

    do {
      const int bb = (v >> --n) & 1;
      encode_bool(bc, bb, pp[i >> 1]);
      i = vp9_coef_tree[i + bb];
    } while (n);


    if (b->base_val) {
      const int e = p->Extra, L = b->Len;

      if (L) {
        const unsigned char *pp = b->prob;
        int v = e >> 1;
        int n = L;              /* number of bits in v, assumed nonzero */
        int i = 0;

        do {
          const int bb = (v >> --n) & 1;
          encode_bool(bc, bb, pp[i >> 1]);
          i = b->tree[i + bb];
        } while (n);
      }

      encode_bool(bc, e & 1, 128);
    }
    ++p;
  }

  *tp = p;
}

static void write_partition_size(unsigned char *cx_data, int size) {
  signed char csize;

  csize = size & 0xff;
  *cx_data = csize;
  csize = (size >> 8) & 0xff;
  *(cx_data + 1) = csize;
  csize = (size >> 16) & 0xff;
  *(cx_data + 2) = csize;

}

static void write_mv_ref
(
  vp9_writer *bc, MB_PREDICTION_MODE m, const vp9_prob *p
) {
#if CONFIG_DEBUG
  assert(NEARESTMV <= m  &&  m <= SPLITMV);
#endif
  write_token(bc, vp9_mv_ref_tree, p,
              vp9_mv_ref_encoding_array - NEARESTMV + m);
}

static void write_sb_mv_ref(vp9_writer *bc, MB_PREDICTION_MODE m,
                            const vp9_prob *p) {
#if CONFIG_DEBUG
  assert(NEARESTMV <= m  &&  m < SPLITMV);
#endif
  write_token(bc, vp9_sb_mv_ref_tree, p,
              vp9_sb_mv_ref_encoding_array - NEARESTMV + m);
}

static void write_sub_mv_ref
(
  vp9_writer *bc, B_PREDICTION_MODE m, const vp9_prob *p
) {
#if CONFIG_DEBUG
  assert(LEFT4X4 <= m  &&  m <= NEW4X4);
#endif
  write_token(bc, vp9_sub_mv_ref_tree, p,
              vp9_sub_mv_ref_encoding_array - LEFT4X4 + m);
}

static void write_nmv(vp9_writer *bc, const MV *mv, const int_mv *ref,
                      const nmv_context *nmvc, int usehp) {
  MV e;
  e.row = mv->row - ref->as_mv.row;
  e.col = mv->col - ref->as_mv.col;

  vp9_encode_nmv(bc, &e, &ref->as_mv, nmvc);
  vp9_encode_nmv_fp(bc, &e, &ref->as_mv, nmvc, usehp);
}

#if CONFIG_NEW_MVREF
static void vp9_write_mv_ref_id(vp9_writer *w,
                                vp9_prob * ref_id_probs,
                                int mv_ref_id) {
  // Encode the index for the MV reference.
  switch (mv_ref_id) {
    case 0:
      vp9_write(w, 0, ref_id_probs[0]);
      break;
    case 1:
      vp9_write(w, 1, ref_id_probs[0]);
      vp9_write(w, 0, ref_id_probs[1]);
      break;
    case 2:
      vp9_write(w, 1, ref_id_probs[0]);
      vp9_write(w, 1, ref_id_probs[1]);
      vp9_write(w, 0, ref_id_probs[2]);
      break;
    case 3:
      vp9_write(w, 1, ref_id_probs[0]);
      vp9_write(w, 1, ref_id_probs[1]);
      vp9_write(w, 1, ref_id_probs[2]);
      break;

      // TRAP.. This should not happen
    default:
      assert(0);
      break;
  }
}
#endif

// This function writes the current macro block's segnment id to the bitstream
// It should only be called if a segment map update is indicated.
static void write_mb_segid(vp9_writer *bc,
                           const MB_MODE_INFO *mi, const MACROBLOCKD *xd) {
  // Encode the MB segment id.
  int seg_id = mi->segment_id;

  if (xd->segmentation_enabled && xd->update_mb_segmentation_map) {
    switch (seg_id) {
      case 0:
        vp9_write(bc, 0, xd->mb_segment_tree_probs[0]);
        vp9_write(bc, 0, xd->mb_segment_tree_probs[1]);
        break;
      case 1:
        vp9_write(bc, 0, xd->mb_segment_tree_probs[0]);
        vp9_write(bc, 1, xd->mb_segment_tree_probs[1]);
        break;
      case 2:
        vp9_write(bc, 1, xd->mb_segment_tree_probs[0]);
        vp9_write(bc, 0, xd->mb_segment_tree_probs[2]);
        break;
      case 3:
        vp9_write(bc, 1, xd->mb_segment_tree_probs[0]);
        vp9_write(bc, 1, xd->mb_segment_tree_probs[2]);
        break;

        // TRAP.. This should not happen
      default:
        vp9_write(bc, 0, xd->mb_segment_tree_probs[0]);
        vp9_write(bc, 0, xd->mb_segment_tree_probs[1]);
        break;
    }
  }
}

// This function encodes the reference frame
static void encode_ref_frame(vp9_writer *const bc,
                             VP9_COMMON *const cm,
                             MACROBLOCKD *xd,
                             int segment_id,
                             MV_REFERENCE_FRAME rf) {
  int seg_ref_active;
  int seg_ref_count = 0;
  seg_ref_active = vp9_segfeature_active(xd,
                                         segment_id,
                                         SEG_LVL_REF_FRAME);

  if (seg_ref_active) {
    seg_ref_count = vp9_check_segref(xd, segment_id, INTRA_FRAME) +
                    vp9_check_segref(xd, segment_id, LAST_FRAME) +
                    vp9_check_segref(xd, segment_id, GOLDEN_FRAME) +
                    vp9_check_segref(xd, segment_id, ALTREF_FRAME);
  }

  // If segment level coding of this signal is disabled...
  // or the segment allows multiple reference frame options
  if (!seg_ref_active || (seg_ref_count > 1)) {
    // Values used in prediction model coding
    unsigned char prediction_flag;
    vp9_prob pred_prob;
    MV_REFERENCE_FRAME pred_rf;

    // Get the context probability the prediction flag
    pred_prob = vp9_get_pred_prob(cm, xd, PRED_REF);

    // Get the predicted value.
    pred_rf = vp9_get_pred_ref(cm, xd);

    // Did the chosen reference frame match its predicted value.
    prediction_flag =
      (xd->mode_info_context->mbmi.ref_frame == pred_rf);

    vp9_set_pred_flag(xd, PRED_REF, prediction_flag);
    vp9_write(bc, prediction_flag, pred_prob);

    // If not predicted correctly then code value explicitly
    if (!prediction_flag) {
      vp9_prob mod_refprobs[PREDICTION_PROBS];

      vpx_memcpy(mod_refprobs,
                 cm->mod_refprobs[pred_rf], sizeof(mod_refprobs));

      // If segment coding enabled blank out options that cant occur by
      // setting the branch probability to 0.
      if (seg_ref_active) {
        mod_refprobs[INTRA_FRAME] *=
          vp9_check_segref(xd, segment_id, INTRA_FRAME);
        mod_refprobs[LAST_FRAME] *=
          vp9_check_segref(xd, segment_id, LAST_FRAME);
        mod_refprobs[GOLDEN_FRAME] *=
          (vp9_check_segref(xd, segment_id, GOLDEN_FRAME) *
           vp9_check_segref(xd, segment_id, ALTREF_FRAME));
      }

      if (mod_refprobs[0]) {
        vp9_write(bc, (rf != INTRA_FRAME), mod_refprobs[0]);
      }

      // Inter coded
      if (rf != INTRA_FRAME) {
        if (mod_refprobs[1]) {
          vp9_write(bc, (rf != LAST_FRAME), mod_refprobs[1]);
        }

        if (rf != LAST_FRAME) {
          if (mod_refprobs[2]) {
            vp9_write(bc, (rf != GOLDEN_FRAME), mod_refprobs[2]);
          }
        }
      }
    }
  }

  // if using the prediction mdoel we have nothing further to do because
  // the reference frame is fully coded by the segment
}

// Update the probabilities used to encode reference frame data
static void update_ref_probs(VP9_COMP *const cpi) {
  VP9_COMMON *const cm = &cpi->common;

  const int *const rfct = cpi->count_mb_ref_frame_usage;
  const int rf_intra = rfct[INTRA_FRAME];
  const int rf_inter = rfct[LAST_FRAME] +
                       rfct[GOLDEN_FRAME] + rfct[ALTREF_FRAME];

  cm->prob_intra_coded = get_binary_prob(rf_intra, rf_inter);
  cm->prob_last_coded = get_prob(rfct[LAST_FRAME], rf_inter);
  cm->prob_gf_coded = get_binary_prob(rfct[GOLDEN_FRAME], rfct[ALTREF_FRAME]);

  // Compute a modified set of probabilities to use when prediction of the
  // reference frame fails
  vp9_compute_mod_refprobs(cm);
}

static void pack_inter_mode_mvs(VP9_COMP *cpi, MODE_INFO *m,
                                vp9_writer *bc,
                                int mb_rows_left, int mb_cols_left) {
  VP9_COMMON *const pc = &cpi->common;
  const nmv_context *nmvc = &pc->fc.nmvc;
  MACROBLOCK *const x = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  const int mis = pc->mode_info_stride;
  MB_MODE_INFO *const mi = &m->mbmi;
  const MV_REFERENCE_FRAME rf = mi->ref_frame;
  const MB_PREDICTION_MODE mode = mi->mode;
  const int segment_id = mi->segment_id;
  const int mb_size = 1 << mi->sb_type;
  int skip_coeff;

  int mb_row = pc->mb_rows - mb_rows_left;
  int mb_col = pc->mb_cols - mb_cols_left;
  xd->prev_mode_info_context = pc->prev_mi + (m - pc->mi);
  x->partition_info = x->pi + (m - pc->mi);

  // Distance of Mb to the various image edges.
  // These specified to 8th pel as they are always compared to MV
  // values that are in 1/8th pel units
  xd->mb_to_left_edge = -((mb_col * 16) << 3);
  xd->mb_to_top_edge = -((mb_row * 16)) << 3;
  xd->mb_to_right_edge = ((pc->mb_cols - mb_size - mb_col) * 16) << 3;
  xd->mb_to_bottom_edge = ((pc->mb_rows - mb_size - mb_row) * 16) << 3;

#ifdef ENTROPY_STATS
  active_section = 9;
#endif

  if (cpi->mb.e_mbd.update_mb_segmentation_map) {
    // Is temporal coding of the segment map enabled
    if (pc->temporal_update) {
      unsigned char prediction_flag = vp9_get_pred_flag(xd, PRED_SEG_ID);
      vp9_prob pred_prob = vp9_get_pred_prob(pc, xd, PRED_SEG_ID);

      // Code the segment id prediction flag for this mb
      vp9_write(bc, prediction_flag, pred_prob);

      // If the mb segment id wasn't predicted code explicitly
      if (!prediction_flag)
        write_mb_segid(bc, mi, &cpi->mb.e_mbd);
    } else {
      // Normal unpredicted coding
      write_mb_segid(bc, mi, &cpi->mb.e_mbd);
    }
  }

  if (!pc->mb_no_coeff_skip) {
    skip_coeff = 0;
  } else if (vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB) &&
             vp9_get_segdata(xd, segment_id, SEG_LVL_EOB) == 0) {
    skip_coeff = 1;
  } else {
    const int nmbs = mb_size;
    const int xmbs = MIN(nmbs, mb_cols_left);
    const int ymbs = MIN(nmbs, mb_rows_left);
    int x, y;

    skip_coeff = 1;
    for (y = 0; y < ymbs; y++) {
      for (x = 0; x < xmbs; x++) {
        skip_coeff = skip_coeff && m[y * mis + x].mbmi.mb_skip_coeff;
      }
    }

    vp9_write(bc, skip_coeff,
              vp9_get_pred_prob(pc, xd, PRED_MBSKIP));
  }

  // Encode the reference frame.
  if (!vp9_segfeature_active(xd, segment_id, SEG_LVL_MODE)
      || vp9_get_segdata(xd, segment_id, SEG_LVL_MODE) >= NEARESTMV) {
    encode_ref_frame(bc, pc, xd, segment_id, rf);
  } else {
    assert(rf == INTRA_FRAME);
  }

  if (rf == INTRA_FRAME) {
#ifdef ENTROPY_STATS
    active_section = 6;
#endif

    if (!vp9_segfeature_active(xd, segment_id, SEG_LVL_MODE)) {
      if (m->mbmi.sb_type)
        write_sb_ymode(bc, mode, pc->fc.sb_ymode_prob);
      else
        write_ymode(bc, mode, pc->fc.ymode_prob);
    }
    if (mode == B_PRED) {
      int j = 0;
      do {
        write_bmode(bc, m->bmi[j].as_mode.first,
                    pc->fc.bmode_prob);
      } while (++j < 16);
    }
    if (mode == I8X8_PRED) {
      write_i8x8_mode(bc, m->bmi[0].as_mode.first,
                      pc->fc.i8x8_mode_prob);
      write_i8x8_mode(bc, m->bmi[2].as_mode.first,
                      pc->fc.i8x8_mode_prob);
      write_i8x8_mode(bc, m->bmi[8].as_mode.first,
                      pc->fc.i8x8_mode_prob);
      write_i8x8_mode(bc, m->bmi[10].as_mode.first,
                      pc->fc.i8x8_mode_prob);
    } else {
      write_uv_mode(bc, mi->uv_mode,
                    pc->fc.uv_mode_prob[mode]);
    }
  } else {
    vp9_prob mv_ref_p[VP9_MVREFS - 1];

    vp9_mv_ref_probs(&cpi->common, mv_ref_p, mi->mb_mode_context[rf]);

    // #ifdef ENTROPY_STATS
#ifdef ENTROPY_STATS
    accum_mv_refs(mode, ct);
    active_section = 3;
#endif

    // Is the segment coding of mode enabled
    if (!vp9_segfeature_active(xd, segment_id, SEG_LVL_MODE)) {
      if (mi->sb_type) {
        write_sb_mv_ref(bc, mode, mv_ref_p);
      } else {
        write_mv_ref(bc, mode, mv_ref_p);
      }
      vp9_accum_mv_refs(&cpi->common, mode, mi->mb_mode_context[rf]);
    }

    if (mode >= NEARESTMV && mode <= SPLITMV) {
      if (cpi->common.mcomp_filter_type == SWITCHABLE) {
        write_token(bc, vp9_switchable_interp_tree,
                    vp9_get_pred_probs(&cpi->common, xd,
                                       PRED_SWITCHABLE_INTERP),
                    vp9_switchable_interp_encodings +
                    vp9_switchable_interp_map[mi->interp_filter]);
      } else {
        assert(mi->interp_filter == cpi->common.mcomp_filter_type);
      }
    }

    // does the feature use compound prediction or not
    // (if not specified at the frame/segment level)
    if (cpi->common.comp_pred_mode == HYBRID_PREDICTION) {
      vp9_write(bc, mi->second_ref_frame > INTRA_FRAME,
                vp9_get_pred_prob(pc, xd, PRED_COMP));
    }
#if CONFIG_COMP_INTERINTRA_PRED
    if (cpi->common.use_interintra &&
        mode >= NEARESTMV && mode < SPLITMV &&
        mi->second_ref_frame <= INTRA_FRAME) {
      vp9_write(bc, mi->second_ref_frame == INTRA_FRAME,
                pc->fc.interintra_prob);
      // if (!cpi->dummy_packing)
      //   printf("-- %d (%d)\n", mi->second_ref_frame == INTRA_FRAME,
      //          pc->fc.interintra_prob);
      if (mi->second_ref_frame == INTRA_FRAME) {
        // if (!cpi->dummy_packing)
        //   printf("** %d %d\n", mi->interintra_mode,
        // mi->interintra_uv_mode);
        write_ymode(bc, mi->interintra_mode, pc->fc.ymode_prob);
#if SEPARATE_INTERINTRA_UV
        write_uv_mode(bc, mi->interintra_uv_mode,
                      pc->fc.uv_mode_prob[mi->interintra_mode]);
#endif
      }
    }
#endif

#if CONFIG_NEW_MVREF
    // if ((mode == NEWMV) || (mode == SPLITMV)) {
    if (mode == NEWMV) {
      // Encode the index of the choice.
      vp9_write_mv_ref_id(bc,
                          xd->mb_mv_ref_probs[rf], mi->best_index);

      if (mi->second_ref_frame > 0) {
        // Encode the index of the choice.
        vp9_write_mv_ref_id(
                            bc, xd->mb_mv_ref_probs[mi->second_ref_frame],
                            mi->best_second_index);
      }
    }
#endif

    switch (mode) { /* new, split require MVs */
      case NEWMV:
#ifdef ENTROPY_STATS
        active_section = 5;
#endif
        write_nmv(bc, &mi->mv[0].as_mv, &mi->best_mv,
                  (const nmv_context*) nmvc,
                  xd->allow_high_precision_mv);

        if (mi->second_ref_frame > 0) {
          write_nmv(bc, &mi->mv[1].as_mv, &mi->best_second_mv,
                    (const nmv_context*) nmvc,
                    xd->allow_high_precision_mv);
        }
        break;
      case SPLITMV: {
        int j = 0;

#ifdef MODE_STATS
        ++count_mb_seg[mi->partitioning];
#endif

        write_split(bc, mi->partitioning, cpi->common.fc.mbsplit_prob);
        cpi->mbsplit_count[mi->partitioning]++;

        do {
          B_PREDICTION_MODE blockmode;
          int_mv blockmv;
          const int *const  L = vp9_mbsplits[mi->partitioning];
          int k = -1;  /* first block in subset j */
          int mv_contz;
          int_mv leftmv, abovemv;

          blockmode = cpi->mb.partition_info->bmi[j].mode;
          blockmv = cpi->mb.partition_info->bmi[j].mv;
#if CONFIG_DEBUG
          while (j != L[++k])
            if (k >= 16)
              assert(0);
#else
          while (j != L[++k]);
#endif
          leftmv.as_int = left_block_mv(m, k);
          abovemv.as_int = above_block_mv(m, k, mis);
          mv_contz = vp9_mv_cont(&leftmv, &abovemv);

          write_sub_mv_ref(bc, blockmode,
                           cpi->common.fc.sub_mv_ref_prob[mv_contz]);
          cpi->sub_mv_ref_count[mv_contz][blockmode - LEFT4X4]++;
          if (blockmode == NEW4X4) {
#ifdef ENTROPY_STATS
            active_section = 11;
#endif
            write_nmv(bc, &blockmv.as_mv, &mi->best_mv,
                      (const nmv_context*) nmvc,
                      xd->allow_high_precision_mv);

            if (mi->second_ref_frame > 0) {
              write_nmv(bc,
                        &cpi->mb.partition_info->bmi[j].second_mv.as_mv,
                        &mi->best_second_mv,
                        (const nmv_context*) nmvc,
                        xd->allow_high_precision_mv);
            }
          }
        } while (++j < cpi->mb.partition_info->count);
        break;
      }
      default:
        break;
    }
  }

  if (((rf == INTRA_FRAME && mode <= I8X8_PRED) ||
       (rf != INTRA_FRAME && !(mode == SPLITMV &&
                               mi->partitioning == PARTITIONING_4X4))) &&
      pc->txfm_mode == TX_MODE_SELECT &&
      !((pc->mb_no_coeff_skip && skip_coeff) ||
        (vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB) &&
         vp9_get_segdata(xd, segment_id, SEG_LVL_EOB) == 0))) {
    TX_SIZE sz = mi->txfm_size;
    // FIXME(rbultje) code ternary symbol once all experiments are merged
    vp9_write(bc, sz != TX_4X4, pc->prob_tx[0]);
    if (sz != TX_4X4 && mode != I8X8_PRED && mode != SPLITMV) {
      vp9_write(bc, sz != TX_8X8, pc->prob_tx[1]);
      if (mi->sb_type && sz != TX_8X8)
        vp9_write(bc, sz != TX_16X16, pc->prob_tx[2]);
    }
  }
}

static void write_mb_modes_kf(const VP9_COMP *cpi,
                              const MODE_INFO *m,
                              vp9_writer *bc,
                              int mb_rows_left, int mb_cols_left) {
  const VP9_COMMON *const c = &cpi->common;
  const MACROBLOCKD *const xd = &cpi->mb.e_mbd;
  const int mis = c->mode_info_stride;
  const int ym = m->mbmi.mode;
  const int segment_id = m->mbmi.segment_id;
  int skip_coeff;

  if (xd->update_mb_segmentation_map) {
    write_mb_segid(bc, &m->mbmi, xd);
  }

  if (!c->mb_no_coeff_skip) {
    skip_coeff = 0;
  } else if (vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB) &&
             vp9_get_segdata(xd, segment_id, SEG_LVL_EOB) == 0) {
    skip_coeff = 1;
  } else {
    const int nmbs = 1 << m->mbmi.sb_type;
    const int xmbs = MIN(nmbs, mb_cols_left);
    const int ymbs = MIN(nmbs, mb_rows_left);
    int x, y;

    skip_coeff = 1;
    for (y = 0; y < ymbs; y++) {
      for (x = 0; x < xmbs; x++) {
        skip_coeff = skip_coeff && m[y * mis + x].mbmi.mb_skip_coeff;
      }
    }

    vp9_write(bc, skip_coeff,
              vp9_get_pred_prob(c, xd, PRED_MBSKIP));
  }

  if (m->mbmi.sb_type) {
    sb_kfwrite_ymode(bc, ym,
                     c->sb_kf_ymode_prob[c->kf_ymode_probs_index]);
  } else {
    kfwrite_ymode(bc, ym,
                  c->kf_ymode_prob[c->kf_ymode_probs_index]);
  }

  if (ym == B_PRED) {
    int i = 0;
    do {
      const B_PREDICTION_MODE A = above_block_mode(m, i, mis);
      const B_PREDICTION_MODE L = left_block_mode(m, i);
      const int bm = m->bmi[i].as_mode.first;

#ifdef ENTROPY_STATS
      ++intra_mode_stats [A] [L] [bm];
#endif

      write_kf_bmode(bc, bm, c->kf_bmode_prob[A][L]);
    } while (++i < 16);
  }
  if (ym == I8X8_PRED) {
    write_i8x8_mode(bc, m->bmi[0].as_mode.first,
                    c->fc.i8x8_mode_prob);
    // printf("    mode: %d\n", m->bmi[0].as_mode.first); fflush(stdout);
    write_i8x8_mode(bc, m->bmi[2].as_mode.first,
                    c->fc.i8x8_mode_prob);
    // printf("    mode: %d\n", m->bmi[2].as_mode.first); fflush(stdout);
    write_i8x8_mode(bc, m->bmi[8].as_mode.first,
                    c->fc.i8x8_mode_prob);
    // printf("    mode: %d\n", m->bmi[8].as_mode.first); fflush(stdout);
    write_i8x8_mode(bc, m->bmi[10].as_mode.first,
                    c->fc.i8x8_mode_prob);
    // printf("    mode: %d\n", m->bmi[10].as_mode.first); fflush(stdout);
  } else
    write_uv_mode(bc, m->mbmi.uv_mode, c->kf_uv_mode_prob[ym]);

  if (ym <= I8X8_PRED && c->txfm_mode == TX_MODE_SELECT &&
      !((c->mb_no_coeff_skip && skip_coeff) ||
        (vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB) &&
         vp9_get_segdata(xd, segment_id, SEG_LVL_EOB) == 0))) {
    TX_SIZE sz = m->mbmi.txfm_size;
    // FIXME(rbultje) code ternary symbol once all experiments are merged
    vp9_write(bc, sz != TX_4X4, c->prob_tx[0]);
    if (sz != TX_4X4 && ym <= TM_PRED) {
      vp9_write(bc, sz != TX_8X8, c->prob_tx[1]);
      if (m->mbmi.sb_type && sz != TX_8X8)
        vp9_write(bc, sz != TX_16X16, c->prob_tx[2]);
    }
  }
}

static void write_modes_b(VP9_COMP *cpi, MODE_INFO *m, vp9_writer *bc,
                          TOKENEXTRA **tok, TOKENEXTRA *tok_end,
                          int mb_row, int mb_col) {
  VP9_COMMON *const c = &cpi->common;
  MACROBLOCKD *const xd = &cpi->mb.e_mbd;

  xd->mode_info_context = m;
  if (c->frame_type == KEY_FRAME) {
    write_mb_modes_kf(cpi, m, bc,
                      c->mb_rows - mb_row, c->mb_cols - mb_col);
#ifdef ENTROPY_STATS
    active_section = 8;
#endif
  } else {
    pack_inter_mode_mvs(cpi, m, bc,
                        c->mb_rows - mb_row, c->mb_cols - mb_col);
#ifdef ENTROPY_STATS
    active_section = 1;
#endif
  }

  assert(*tok < tok_end);
  pack_mb_tokens(bc, tok, tok_end);
}

static void write_modes(VP9_COMP *cpi, vp9_writer* const bc) {
  VP9_COMMON *const c = &cpi->common;
  const int mis = c->mode_info_stride;
  MODE_INFO *m, *m_ptr = c->mi;
  int i, mb_row, mb_col;
  TOKENEXTRA *tok = cpi->tok;
  TOKENEXTRA *tok_end = tok + cpi->tok_count;

  for (mb_row = 0; mb_row < c->mb_rows; mb_row += 4, m_ptr += 4 * mis) {
    m = m_ptr;
    for (mb_col = 0; mb_col < c->mb_cols; mb_col += 4, m += 4) {
      vp9_write(bc, m->mbmi.sb_type == BLOCK_SIZE_SB64X64, c->sb64_coded);
      if (m->mbmi.sb_type == BLOCK_SIZE_SB64X64) {
        write_modes_b(cpi, m, bc, &tok, tok_end, mb_row, mb_col);
      } else {
        int j;

        for (j = 0; j < 4; j++) {
          const int x_idx_sb = (j & 1) << 1, y_idx_sb = j & 2;
          MODE_INFO *sb_m = m + y_idx_sb * mis + x_idx_sb;

          if (mb_col + x_idx_sb >= c->mb_cols ||
              mb_row + y_idx_sb >= c->mb_rows)
            continue;

          vp9_write(bc, sb_m->mbmi.sb_type, c->sb32_coded);
          if (sb_m->mbmi.sb_type) {
            assert(sb_m->mbmi.sb_type == BLOCK_SIZE_SB32X32);
            write_modes_b(cpi, sb_m, bc, &tok, tok_end,
                          mb_row + y_idx_sb, mb_col + x_idx_sb);
          } else {
            // Process the 4 MBs in the order:
            // top-left, top-right, bottom-left, bottom-right
            for (i = 0; i < 4; i++) {
              const int x_idx = x_idx_sb + (i & 1), y_idx = y_idx_sb + (i >> 1);
              MODE_INFO *mb_m = m + x_idx + y_idx * mis;

              if (mb_row + y_idx >= c->mb_rows ||
                  mb_col + x_idx >= c->mb_cols) {
                // MB lies outside frame, move on
                continue;
              }

              assert(mb_m->mbmi.sb_type == BLOCK_SIZE_MB16X16);
              write_modes_b(cpi, mb_m, bc, &tok, tok_end,
                            mb_row + y_idx, mb_col + x_idx);
            }
          }
        }
      }
    }
  }
}


/* This function is used for debugging probability trees. */
static void print_prob_tree(vp9_coeff_probs *coef_probs) {
  /* print coef probability tree */
  int i, j, k, l;
  FILE *f = fopen("enc_tree_probs.txt", "a");
  fprintf(f, "{\n");
  for (i = 0; i < BLOCK_TYPES_4X4; i++) {
    fprintf(f, "  {\n");
    for (j = 0; j < COEF_BANDS; j++) {
      fprintf(f, "    {\n");
      for (k = 0; k < PREV_COEF_CONTEXTS; k++) {
        fprintf(f, "      {");
        for (l = 0; l < ENTROPY_NODES; l++) {
          fprintf(f, "%3u, ",
                  (unsigned int)(coef_probs [i][j][k][l]));
        }
        fprintf(f, " }\n");
      }
      fprintf(f, "    }\n");
    }
    fprintf(f, "  }\n");
  }
  fprintf(f, "}\n");
  fclose(f);
}

static void build_tree_distribution(vp9_coeff_probs *coef_probs,
                                    vp9_coeff_count *coef_counts,
#ifdef ENTROPY_STATS
                                    VP9_COMP *cpi,
                                    vp9_coeff_accum *context_counters,
#endif
                                    vp9_coeff_stats *coef_branch_ct,
                                    int block_types) {
  int i = 0, j, k;
#ifdef ENTROPY_STATS
  int t = 0;
#endif

  for (i = 0; i < block_types; ++i) {
    for (j = 0; j < COEF_BANDS; ++j) {
      for (k = 0; k < PREV_COEF_CONTEXTS; ++k) {
        if (k >= 3 && ((i == 0 && j == 1) || (i > 0 && j == 0)))
          continue;
        vp9_tree_probs_from_distribution(MAX_ENTROPY_TOKENS,
                                         vp9_coef_encodings, vp9_coef_tree,
                                         coef_probs[i][j][k],
                                         coef_branch_ct[i][j][k],
                                         coef_counts[i][j][k]);
#ifdef ENTROPY_STATS
        if (!cpi->dummy_packing)
          for (t = 0; t < MAX_ENTROPY_TOKENS; ++t)
            context_counters[i][j][k][t] += coef_counts[i][j][k][t];
#endif
      }
    }
  }
}

static void build_coeff_contexts(VP9_COMP *cpi) {
  build_tree_distribution(cpi->frame_coef_probs_4x4,
                          cpi->coef_counts_4x4,
#ifdef ENTROPY_STATS
                          cpi, context_counters_4x4,
#endif
                          cpi->frame_branch_ct_4x4, BLOCK_TYPES_4X4);
  build_tree_distribution(cpi->frame_hybrid_coef_probs_4x4,
                          cpi->hybrid_coef_counts_4x4,
#ifdef ENTROPY_STATS
                          cpi, hybrid_context_counters_4x4,
#endif
                          cpi->frame_hybrid_branch_ct_4x4, BLOCK_TYPES_4X4);
  build_tree_distribution(cpi->frame_coef_probs_8x8,
                          cpi->coef_counts_8x8,
#ifdef ENTROPY_STATS
                          cpi, context_counters_8x8,
#endif
                          cpi->frame_branch_ct_8x8, BLOCK_TYPES_8X8);
  build_tree_distribution(cpi->frame_hybrid_coef_probs_8x8,
                          cpi->hybrid_coef_counts_8x8,
#ifdef ENTROPY_STATS
                          cpi, hybrid_context_counters_8x8,
#endif
                          cpi->frame_hybrid_branch_ct_8x8, BLOCK_TYPES_8X8);
  build_tree_distribution(cpi->frame_coef_probs_16x16,
                          cpi->coef_counts_16x16,
#ifdef ENTROPY_STATS
                          cpi, context_counters_16x16,
#endif
                          cpi->frame_branch_ct_16x16, BLOCK_TYPES_16X16);
  build_tree_distribution(cpi->frame_hybrid_coef_probs_16x16,
                          cpi->hybrid_coef_counts_16x16,
#ifdef ENTROPY_STATS
                          cpi, hybrid_context_counters_16x16,
#endif
                          cpi->frame_hybrid_branch_ct_16x16, BLOCK_TYPES_16X16);
  build_tree_distribution(cpi->frame_coef_probs_32x32,
                          cpi->coef_counts_32x32,
#ifdef ENTROPY_STATS
                          cpi, context_counters_32x32,
#endif
                          cpi->frame_branch_ct_32x32, BLOCK_TYPES_32X32);
}

static void update_coef_probs_common(vp9_writer* const bc,
#ifdef ENTROPY_STATS
                                     VP9_COMP *cpi,
                                     vp9_coeff_stats *tree_update_hist,
#endif
                                     vp9_coeff_probs *new_frame_coef_probs,
                                     vp9_coeff_probs *old_frame_coef_probs,
                                     vp9_coeff_stats *frame_branch_ct,
                                     int block_types) {
  int i, j, k, t;
  int update[2] = {0, 0};
  int savings;
  // vp9_prob bestupd = find_coef_update_prob(cpi);

  /* dry run to see if there is any udpate at all needed */
  savings = 0;
  for (i = 0; i < block_types; ++i) {
    for (j = !i; j < COEF_BANDS; ++j) {
      int prev_coef_savings[ENTROPY_NODES] = {0};
      for (k = 0; k < PREV_COEF_CONTEXTS; ++k) {
        for (t = 0; t < ENTROPY_NODES; ++t) {
          vp9_prob newp = new_frame_coef_probs[i][j][k][t];
          const vp9_prob oldp = old_frame_coef_probs[i][j][k][t];
          const vp9_prob upd = COEF_UPDATE_PROB;
          int s = prev_coef_savings[t];
          int u = 0;
          if (k >= 3 && ((i == 0 && j == 1) || (i > 0 && j == 0)))
            continue;
#if defined(SEARCH_NEWP)
          s = prob_diff_update_savings_search(
                frame_branch_ct[i][j][k][t],
                oldp, &newp, upd);
          if (s > 0 && newp != oldp)
            u = 1;
          if (u)
            savings += s - (int)(vp9_cost_zero(upd));
          else
            savings -= (int)(vp9_cost_zero(upd));
#else
          s = prob_update_savings(
                frame_branch_ct[i][j][k][t],
                oldp, newp, upd);
          if (s > 0)
            u = 1;
          if (u)
            savings += s;
#endif

          update[u]++;
        }
      }
    }
  }

  // printf("Update %d %d, savings %d\n", update[0], update[1], savings);
  /* Is coef updated at all */
  if (update[1] == 0 || savings < 0) {
    vp9_write_bit(bc, 0);
  } else {
    vp9_write_bit(bc, 1);
    for (i = 0; i < block_types; ++i) {
      for (j = !i; j < COEF_BANDS; ++j) {
        int prev_coef_savings[ENTROPY_NODES] = {0};
        for (k = 0; k < PREV_COEF_CONTEXTS; ++k) {
          // calc probs and branch cts for this frame only
          for (t = 0; t < ENTROPY_NODES; ++t) {
            vp9_prob newp = new_frame_coef_probs[i][j][k][t];
            vp9_prob *oldp = old_frame_coef_probs[i][j][k] + t;
            const vp9_prob upd = COEF_UPDATE_PROB;
            int s = prev_coef_savings[t];
            int u = 0;
            if (k >= 3 && ((i == 0 && j == 1) || (i > 0 && j == 0)))
              continue;

#if defined(SEARCH_NEWP)
            s = prob_diff_update_savings_search(
                  frame_branch_ct[i][j][k][t],
                  *oldp, &newp, upd);
            if (s > 0 && newp != *oldp)
              u = 1;
#else
            s = prob_update_savings(
                  frame_branch_ct[i][j][k][t],
                  *oldp, newp, upd);
            if (s > 0)
              u = 1;
#endif
            vp9_write(bc, u, upd);
#ifdef ENTROPY_STATS
            if (!cpi->dummy_packing)
              ++tree_update_hist[i][j][k][t][u];
#endif
            if (u) {
              /* send/use new probability */
              write_prob_diff_update(bc, newp, *oldp);
              *oldp = newp;
            }
          }
        }
      }
    }
  }
}

static void update_coef_probs(VP9_COMP* const cpi, vp9_writer* const bc) {
  vp9_clear_system_state();

  // Build the cofficient contexts based on counts collected in encode loop
  build_coeff_contexts(cpi);

  update_coef_probs_common(bc,
#ifdef ENTROPY_STATS
                           cpi,
                           tree_update_hist_4x4,
#endif
                           cpi->frame_coef_probs_4x4,
                           cpi->common.fc.coef_probs_4x4,
                           cpi->frame_branch_ct_4x4,
                           BLOCK_TYPES_4X4);

  update_coef_probs_common(bc,
#ifdef ENTROPY_STATS
                           cpi,
                           hybrid_tree_update_hist_4x4,
#endif
                           cpi->frame_hybrid_coef_probs_4x4,
                           cpi->common.fc.hybrid_coef_probs_4x4,
                           cpi->frame_hybrid_branch_ct_4x4,
                           BLOCK_TYPES_4X4);

  /* do not do this if not even allowed */
  if (cpi->common.txfm_mode != ONLY_4X4) {
    update_coef_probs_common(bc,
#ifdef ENTROPY_STATS
                             cpi,
                             tree_update_hist_8x8,
#endif
                             cpi->frame_coef_probs_8x8,
                             cpi->common.fc.coef_probs_8x8,
                             cpi->frame_branch_ct_8x8,
                             BLOCK_TYPES_8X8);

    update_coef_probs_common(bc,
#ifdef ENTROPY_STATS
                             cpi,
                             hybrid_tree_update_hist_8x8,
#endif
                             cpi->frame_hybrid_coef_probs_8x8,
                             cpi->common.fc.hybrid_coef_probs_8x8,
                             cpi->frame_hybrid_branch_ct_8x8,
                             BLOCK_TYPES_8X8);
  }

  if (cpi->common.txfm_mode > ALLOW_8X8) {
    update_coef_probs_common(bc,
#ifdef ENTROPY_STATS
                             cpi,
                             tree_update_hist_16x16,
#endif
                             cpi->frame_coef_probs_16x16,
                             cpi->common.fc.coef_probs_16x16,
                             cpi->frame_branch_ct_16x16,
                             BLOCK_TYPES_16X16);
    update_coef_probs_common(bc,
#ifdef ENTROPY_STATS
                             cpi,
                             hybrid_tree_update_hist_16x16,
#endif
                             cpi->frame_hybrid_coef_probs_16x16,
                             cpi->common.fc.hybrid_coef_probs_16x16,
                             cpi->frame_hybrid_branch_ct_16x16,
                             BLOCK_TYPES_16X16);
  }

  if (cpi->common.txfm_mode > ALLOW_16X16) {
    update_coef_probs_common(bc,
#ifdef ENTROPY_STATS
                             cpi,
                             tree_update_hist_32x32,
#endif
                             cpi->frame_coef_probs_32x32,
                             cpi->common.fc.coef_probs_32x32,
                             cpi->frame_branch_ct_32x32,
                             BLOCK_TYPES_32X32);
  }
}

#ifdef PACKET_TESTING
FILE *vpxlogc = 0;
#endif

static void put_delta_q(vp9_writer *bc, int delta_q) {
  if (delta_q != 0) {
    vp9_write_bit(bc, 1);
    vp9_write_literal(bc, abs(delta_q), 4);

    if (delta_q < 0)
      vp9_write_bit(bc, 1);
    else
      vp9_write_bit(bc, 0);
  } else
    vp9_write_bit(bc, 0);
}

static void decide_kf_ymode_entropy(VP9_COMP *cpi) {

  int mode_cost[MB_MODE_COUNT];
  int cost;
  int bestcost = INT_MAX;
  int bestindex = 0;
  int i, j;

  for (i = 0; i < 8; i++) {
    vp9_cost_tokens(mode_cost, cpi->common.kf_ymode_prob[i], vp9_kf_ymode_tree);
    cost = 0;
    for (j = 0; j < VP9_YMODES; j++) {
      cost += mode_cost[j] * cpi->ymode_count[j];
    }
    vp9_cost_tokens(mode_cost, cpi->common.sb_kf_ymode_prob[i],
                    vp9_sb_ymode_tree);
    for (j = 0; j < VP9_I32X32_MODES; j++) {
      cost += mode_cost[j] * cpi->sb_ymode_count[j];
    }
    if (cost < bestcost) {
      bestindex = i;
      bestcost = cost;
    }
  }
  cpi->common.kf_ymode_probs_index = bestindex;

}
static void segment_reference_frames(VP9_COMP *cpi) {
  VP9_COMMON *oci = &cpi->common;
  MODE_INFO *mi = oci->mi;
  int ref[MAX_MB_SEGMENTS] = {0};
  int i, j;
  int mb_index = 0;
  MACROBLOCKD *const xd = &cpi->mb.e_mbd;

  for (i = 0; i < oci->mb_rows; i++) {
    for (j = 0; j < oci->mb_cols; j++, mb_index++) {
      ref[mi[mb_index].mbmi.segment_id] |= (1 << mi[mb_index].mbmi.ref_frame);
    }
    mb_index++;
  }
  for (i = 0; i < MAX_MB_SEGMENTS; i++) {
    vp9_enable_segfeature(xd, i, SEG_LVL_REF_FRAME);
    vp9_set_segdata(xd, i, SEG_LVL_REF_FRAME, ref[i]);
  }
}

void vp9_pack_bitstream(VP9_COMP *cpi, unsigned char *dest,
                        unsigned long *size) {
  int i, j;
  VP9_HEADER oh;
  VP9_COMMON *const pc = &cpi->common;
  vp9_writer header_bc, residual_bc;
  MACROBLOCKD *const xd = &cpi->mb.e_mbd;
  int extra_bytes_packed = 0;

  unsigned char *cx_data = dest;

  oh.show_frame = (int) pc->show_frame;
  oh.type = (int)pc->frame_type;
  oh.version = pc->version;
  oh.first_partition_length_in_bytes = 0;

  cx_data += 3;

#if defined(SECTIONBITS_OUTPUT)
  Sectionbits[active_section = 1] += sizeof(VP9_HEADER) * 8 * 256;
#endif

  compute_update_table();

  /* vp9_kf_default_bmode_probs() is called in vp9_setup_key_frame() once
   * for each K frame before encode frame. pc->kf_bmode_prob doesn't get
   * changed anywhere else. No need to call it again here. --yw
   * vp9_kf_default_bmode_probs( pc->kf_bmode_prob);
   */

  /* every keyframe send startcode, width, height, scale factor, clamp
   * and color type.
   */
  if (oh.type == KEY_FRAME) {
    int v;

    // Start / synch code
    cx_data[0] = 0x9D;
    cx_data[1] = 0x01;
    cx_data[2] = 0x2a;

    v = (pc->horiz_scale << 14) | pc->Width;
    cx_data[3] = v;
    cx_data[4] = v >> 8;

    v = (pc->vert_scale << 14) | pc->Height;
    cx_data[5] = v;
    cx_data[6] = v >> 8;

    extra_bytes_packed = 7;
    cx_data += extra_bytes_packed;

    vp9_start_encode(&header_bc, cx_data);

    // signal clr type
    vp9_write_bit(&header_bc, pc->clr_type);
    vp9_write_bit(&header_bc, pc->clamp_type);

  } else {
    vp9_start_encode(&header_bc, cx_data);
  }

  // Signal whether or not Segmentation is enabled
  vp9_write_bit(&header_bc, (xd->segmentation_enabled) ? 1 : 0);

  // Indicate which features are enabled
  if (xd->segmentation_enabled) {
    // Indicate whether or not the segmentation map is being updated.
    vp9_write_bit(&header_bc, (xd->update_mb_segmentation_map) ? 1 : 0);

    // If it is, then indicate the method that will be used.
    if (xd->update_mb_segmentation_map) {
      // Select the coding strategy (temporal or spatial)
      vp9_choose_segmap_coding_method(cpi);
      // Send the tree probabilities used to decode unpredicted
      // macro-block segments
      for (i = 0; i < MB_FEATURE_TREE_PROBS; i++) {
        int data = xd->mb_segment_tree_probs[i];

        if (data != 255) {
          vp9_write_bit(&header_bc, 1);
          vp9_write_literal(&header_bc, data, 8);
        } else {
          vp9_write_bit(&header_bc, 0);
        }
      }

      // Write out the chosen coding method.
      vp9_write_bit(&header_bc, (pc->temporal_update) ? 1 : 0);
      if (pc->temporal_update) {
        for (i = 0; i < PREDICTION_PROBS; i++) {
          int data = pc->segment_pred_probs[i];

          if (data != 255) {
            vp9_write_bit(&header_bc, 1);
            vp9_write_literal(&header_bc, data, 8);
          } else {
            vp9_write_bit(&header_bc, 0);
          }
        }
      }
    }

    vp9_write_bit(&header_bc, (xd->update_mb_segmentation_data) ? 1 : 0);

    // segment_reference_frames(cpi);

    if (xd->update_mb_segmentation_data) {
      signed char Data;

      vp9_write_bit(&header_bc, (xd->mb_segment_abs_delta) ? 1 : 0);

      // For each segments id...
      for (i = 0; i < MAX_MB_SEGMENTS; i++) {
        // For each segmentation codable feature...
        for (j = 0; j < SEG_LVL_MAX; j++) {
          Data = vp9_get_segdata(xd, i, j);

          // If the feature is enabled...
          if (vp9_segfeature_active(xd, i, j)) {
            vp9_write_bit(&header_bc, 1);

            // Is the segment data signed..
            if (vp9_is_segfeature_signed(j)) {
              // Encode the relevant feature data
              if (Data < 0) {
                Data = - Data;
                vp9_encode_unsigned_max(&header_bc, Data,
                                        vp9_seg_feature_data_max(j));
                vp9_write_bit(&header_bc, 1);
              } else {
                vp9_encode_unsigned_max(&header_bc, Data,
                                        vp9_seg_feature_data_max(j));
                vp9_write_bit(&header_bc, 0);
              }
            }
            // Unsigned data element so no sign bit needed
            else
              vp9_encode_unsigned_max(&header_bc, Data,
                                      vp9_seg_feature_data_max(j));
          } else
            vp9_write_bit(&header_bc, 0);
        }
      }
    }
  }

  // Encode the common prediction model status flag probability updates for
  // the reference frame
  update_refpred_stats(cpi);
  if (pc->frame_type != KEY_FRAME) {
    for (i = 0; i < PREDICTION_PROBS; i++) {
      if (cpi->ref_pred_probs_update[i]) {
        vp9_write_bit(&header_bc, 1);
        vp9_write_literal(&header_bc, pc->ref_pred_probs[i], 8);
      } else {
        vp9_write_bit(&header_bc, 0);
      }
    }
  }

  pc->sb64_coded = get_binary_prob(cpi->sb64_count[0], cpi->sb64_count[1]);
  vp9_write_literal(&header_bc, pc->sb64_coded, 8);
  pc->sb32_coded = get_binary_prob(cpi->sb32_count[0], cpi->sb32_count[1]);
  vp9_write_literal(&header_bc, pc->sb32_coded, 8);

  {
    if (pc->txfm_mode == TX_MODE_SELECT) {
      pc->prob_tx[0] = get_prob(cpi->txfm_count_32x32p[TX_4X4] +
                                cpi->txfm_count_16x16p[TX_4X4] +
                                cpi->txfm_count_8x8p[TX_4X4],
                                cpi->txfm_count_32x32p[TX_4X4] +
                                cpi->txfm_count_32x32p[TX_8X8] +
                                cpi->txfm_count_32x32p[TX_16X16] +
                                cpi->txfm_count_32x32p[TX_32X32] +
                                cpi->txfm_count_16x16p[TX_4X4] +
                                cpi->txfm_count_16x16p[TX_8X8] +
                                cpi->txfm_count_16x16p[TX_16X16] +
                                cpi->txfm_count_8x8p[TX_4X4] +
                                cpi->txfm_count_8x8p[TX_8X8]);
      pc->prob_tx[1] = get_prob(cpi->txfm_count_32x32p[TX_8X8] +
                                cpi->txfm_count_16x16p[TX_8X8],
                                cpi->txfm_count_32x32p[TX_8X8] +
                                cpi->txfm_count_32x32p[TX_16X16] +
                                cpi->txfm_count_32x32p[TX_32X32] +
                                cpi->txfm_count_16x16p[TX_8X8] +
                                cpi->txfm_count_16x16p[TX_16X16]);
      pc->prob_tx[2] = get_prob(cpi->txfm_count_32x32p[TX_16X16],
                                cpi->txfm_count_32x32p[TX_16X16] +
                                cpi->txfm_count_32x32p[TX_32X32]);
    } else {
      pc->prob_tx[0] = 128;
      pc->prob_tx[1] = 128;
      pc->prob_tx[2] = 128;
    }
    vp9_write_literal(&header_bc, pc->txfm_mode <= 3 ? pc->txfm_mode : 3, 2);
    if (pc->txfm_mode > ALLOW_16X16) {
      vp9_write_bit(&header_bc, pc->txfm_mode == TX_MODE_SELECT);
    }
    if (pc->txfm_mode == TX_MODE_SELECT) {
      vp9_write_literal(&header_bc, pc->prob_tx[0], 8);
      vp9_write_literal(&header_bc, pc->prob_tx[1], 8);
      vp9_write_literal(&header_bc, pc->prob_tx[2], 8);
    }
  }

  // Encode the loop filter level and type
  vp9_write_bit(&header_bc, pc->filter_type);
  vp9_write_literal(&header_bc, pc->filter_level, 6);
  vp9_write_literal(&header_bc, pc->sharpness_level, 3);

  // Write out loop filter deltas applied at the MB level based on mode or ref frame (if they are enabled).
  vp9_write_bit(&header_bc, (xd->mode_ref_lf_delta_enabled) ? 1 : 0);

  if (xd->mode_ref_lf_delta_enabled) {
    // Do the deltas need to be updated
    int send_update = xd->mode_ref_lf_delta_update;

    vp9_write_bit(&header_bc, send_update);
    if (send_update) {
      int Data;

      // Send update
      for (i = 0; i < MAX_REF_LF_DELTAS; i++) {
        Data = xd->ref_lf_deltas[i];

        // Frame level data
        if (xd->ref_lf_deltas[i] != xd->last_ref_lf_deltas[i]) {
          xd->last_ref_lf_deltas[i] = xd->ref_lf_deltas[i];
          vp9_write_bit(&header_bc, 1);

          if (Data > 0) {
            vp9_write_literal(&header_bc, (Data & 0x3F), 6);
            vp9_write_bit(&header_bc, 0);    // sign
          } else {
            Data = -Data;
            vp9_write_literal(&header_bc, (Data & 0x3F), 6);
            vp9_write_bit(&header_bc, 1);    // sign
          }
        } else {
          vp9_write_bit(&header_bc, 0);
        }
      }

      // Send update
      for (i = 0; i < MAX_MODE_LF_DELTAS; i++) {
        Data = xd->mode_lf_deltas[i];

        if (xd->mode_lf_deltas[i] != xd->last_mode_lf_deltas[i]) {
          xd->last_mode_lf_deltas[i] = xd->mode_lf_deltas[i];
          vp9_write_bit(&header_bc, 1);

          if (Data > 0) {
            vp9_write_literal(&header_bc, (Data & 0x3F), 6);
            vp9_write_bit(&header_bc, 0);    // sign
          } else {
            Data = -Data;
            vp9_write_literal(&header_bc, (Data & 0x3F), 6);
            vp9_write_bit(&header_bc, 1);    // sign
          }
        } else {
          vp9_write_bit(&header_bc, 0);
        }
      }
    }
  }

  // signal here is multi token partition is enabled
  // vp9_write_literal(&header_bc, pc->multi_token_partition, 2);
  vp9_write_literal(&header_bc, 0, 2);

  // Frame Q baseline quantizer index
  vp9_write_literal(&header_bc, pc->base_qindex, QINDEX_BITS);

  // Transmit Dc, Second order and Uv quantizer delta information
  put_delta_q(&header_bc, pc->y1dc_delta_q);
  put_delta_q(&header_bc, pc->y2dc_delta_q);
  put_delta_q(&header_bc, pc->y2ac_delta_q);
  put_delta_q(&header_bc, pc->uvdc_delta_q);
  put_delta_q(&header_bc, pc->uvac_delta_q);

  // When there is a key frame all reference buffers are updated using the new key frame
  if (pc->frame_type != KEY_FRAME) {
    // Should the GF or ARF be updated using the transmitted frame or buffer
    vp9_write_bit(&header_bc, pc->refresh_golden_frame);
    vp9_write_bit(&header_bc, pc->refresh_alt_ref_frame);

    // For inter frames the current default behavior is that when
    // cm->refresh_golden_frame is set we copy the old GF over to
    // the ARF buffer. This is purely an encoder decision at present.
    if (pc->refresh_golden_frame)
      pc->copy_buffer_to_arf  = 2;

    // If not being updated from current frame should either GF or ARF be updated from another buffer
    if (!pc->refresh_golden_frame)
      vp9_write_literal(&header_bc, pc->copy_buffer_to_gf, 2);

    if (!pc->refresh_alt_ref_frame)
      vp9_write_literal(&header_bc, pc->copy_buffer_to_arf, 2);

    // Indicate reference frame sign bias for Golden and ARF frames (always 0 for last frame buffer)
    vp9_write_bit(&header_bc, pc->ref_frame_sign_bias[GOLDEN_FRAME]);
    vp9_write_bit(&header_bc, pc->ref_frame_sign_bias[ALTREF_FRAME]);

    // Signal whether to allow high MV precision
    vp9_write_bit(&header_bc, (xd->allow_high_precision_mv) ? 1 : 0);
    if (pc->mcomp_filter_type == SWITCHABLE) {
      /* Check to see if only one of the filters is actually used */
      int count[VP9_SWITCHABLE_FILTERS];
      int i, j, c = 0;
      for (i = 0; i < VP9_SWITCHABLE_FILTERS; ++i) {
        count[i] = 0;
        for (j = 0; j <= VP9_SWITCHABLE_FILTERS; ++j) {
          count[i] += cpi->switchable_interp_count[j][i];
        }
        c += (count[i] > 0);
      }
      if (c == 1) {
        /* Only one filter is used. So set the filter at frame level */
        for (i = 0; i < VP9_SWITCHABLE_FILTERS; ++i) {
          if (count[i]) {
            pc->mcomp_filter_type = vp9_switchable_interp[i];
            break;
          }
        }
      }
    }
    // Signal the type of subpel filter to use
    vp9_write_bit(&header_bc, (pc->mcomp_filter_type == SWITCHABLE));
    if (pc->mcomp_filter_type != SWITCHABLE)
      vp9_write_literal(&header_bc, (pc->mcomp_filter_type), 2);
#if CONFIG_COMP_INTERINTRA_PRED
    //  printf("Counts: %d %d\n", cpi->interintra_count[0],
    //         cpi->interintra_count[1]);
    if (!cpi->dummy_packing && pc->use_interintra)
      pc->use_interintra = (cpi->interintra_count[1] > 0);
    vp9_write_bit(&header_bc, pc->use_interintra);
    if (!pc->use_interintra)
      vp9_zero(cpi->interintra_count);
#endif
  }

  vp9_write_bit(&header_bc, pc->refresh_entropy_probs);

  if (pc->frame_type != KEY_FRAME)
    vp9_write_bit(&header_bc, pc->refresh_last_frame);

#ifdef ENTROPY_STATS
  if (pc->frame_type == INTER_FRAME)
    active_section = 0;
  else
    active_section = 7;
#endif

  // If appropriate update the inter mode probability context and code the
  // changes in the bitstream.
  if (pc->frame_type != KEY_FRAME) {
    int i, j;
    int new_context[INTER_MODE_CONTEXTS][4];
    update_mode_probs(pc, new_context);

    for (i = 0; i < INTER_MODE_CONTEXTS; i++) {
      for (j = 0; j < 4; j++) {
        if (new_context[i][j] != pc->fc.vp9_mode_contexts[i][j]) {
          vp9_write(&header_bc, 1, 252);
          vp9_write_literal(&header_bc, new_context[i][j], 8);

          // Only update the persistent copy if this is the "real pack"
          if (!cpi->dummy_packing) {
            pc->fc.vp9_mode_contexts[i][j] = new_context[i][j];
          }
        } else {
          vp9_write(&header_bc, 0, 252);
        }
      }
    }
  }

#if CONFIG_NEW_MVREF
  if ((pc->frame_type != KEY_FRAME)) {
    int new_mvref_probs[MAX_REF_FRAMES][MAX_MV_REF_CANDIDATES-1];
    int i, j;

    update_mv_ref_probs(cpi, new_mvref_probs);

    for (i = 0; i < MAX_REF_FRAMES; ++i) {
      // Skip the dummy entry for intra ref frame.
      if (i == INTRA_FRAME) {
        continue;
      }

      // Encode any mandated updates to probabilities
      for (j = 0; j < MAX_MV_REF_CANDIDATES - 1; ++j) {
        if (new_mvref_probs[i][j] != xd->mb_mv_ref_probs[i][j]) {
          vp9_write(&header_bc, 1, VP9_MVREF_UPDATE_PROB);
          vp9_write_literal(&header_bc, new_mvref_probs[i][j], 8);

          // Only update the persistent copy if this is the "real pack"
          if (!cpi->dummy_packing) {
            xd->mb_mv_ref_probs[i][j] = new_mvref_probs[i][j];
          }
        } else {
          vp9_write(&header_bc, 0, VP9_MVREF_UPDATE_PROB);
        }
      }
    }
  }
#endif

  vp9_clear_system_state();  // __asm emms;

  vp9_copy(cpi->common.fc.pre_coef_probs_4x4,
           cpi->common.fc.coef_probs_4x4);
  vp9_copy(cpi->common.fc.pre_hybrid_coef_probs_4x4,
           cpi->common.fc.hybrid_coef_probs_4x4);
  vp9_copy(cpi->common.fc.pre_coef_probs_8x8,
           cpi->common.fc.coef_probs_8x8);
  vp9_copy(cpi->common.fc.pre_hybrid_coef_probs_8x8,
           cpi->common.fc.hybrid_coef_probs_8x8);
  vp9_copy(cpi->common.fc.pre_coef_probs_16x16,
           cpi->common.fc.coef_probs_16x16);
  vp9_copy(cpi->common.fc.pre_hybrid_coef_probs_16x16,
           cpi->common.fc.hybrid_coef_probs_16x16);
  vp9_copy(cpi->common.fc.pre_coef_probs_32x32,
           cpi->common.fc.coef_probs_32x32);
  vp9_copy(cpi->common.fc.pre_sb_ymode_prob, cpi->common.fc.sb_ymode_prob);
  vp9_copy(cpi->common.fc.pre_ymode_prob, cpi->common.fc.ymode_prob);
  vp9_copy(cpi->common.fc.pre_uv_mode_prob, cpi->common.fc.uv_mode_prob);
  vp9_copy(cpi->common.fc.pre_bmode_prob, cpi->common.fc.bmode_prob);
  vp9_copy(cpi->common.fc.pre_sub_mv_ref_prob, cpi->common.fc.sub_mv_ref_prob);
  vp9_copy(cpi->common.fc.pre_mbsplit_prob, cpi->common.fc.mbsplit_prob);
  vp9_copy(cpi->common.fc.pre_i8x8_mode_prob, cpi->common.fc.i8x8_mode_prob);
  cpi->common.fc.pre_nmvc = cpi->common.fc.nmvc;
#if CONFIG_COMP_INTERINTRA_PRED
  cpi->common.fc.pre_interintra_prob = cpi->common.fc.interintra_prob;
#endif
  vp9_zero(cpi->sub_mv_ref_count);
  vp9_zero(cpi->mbsplit_count);
  vp9_zero(cpi->common.fc.mv_ref_ct)

  update_coef_probs(cpi, &header_bc);

#ifdef ENTROPY_STATS
  active_section = 2;
#endif

  // Write out the mb_no_coeff_skip flag
  vp9_write_bit(&header_bc, pc->mb_no_coeff_skip);
  if (pc->mb_no_coeff_skip) {
    int k;

    vp9_update_skip_probs(cpi);
    for (k = 0; k < MBSKIP_CONTEXTS; ++k)
      vp9_write_literal(&header_bc, pc->mbskip_pred_probs[k], 8);
  }

  if (pc->frame_type == KEY_FRAME) {
    if (!pc->kf_ymode_probs_update) {
      vp9_write_literal(&header_bc, pc->kf_ymode_probs_index, 3);
    }
  } else {
    // Update the probabilities used to encode reference frame data
    update_ref_probs(cpi);

#ifdef ENTROPY_STATS
    active_section = 1;
#endif

    if (pc->mcomp_filter_type == SWITCHABLE)
      update_switchable_interp_probs(cpi, &header_bc);

    #if CONFIG_COMP_INTERINTRA_PRED
    if (pc->use_interintra) {
      vp9_cond_prob_update(&header_bc,
                           &pc->fc.interintra_prob,
                           VP9_UPD_INTERINTRA_PROB,
                           cpi->interintra_count);
    }
#endif

    vp9_write_literal(&header_bc, pc->prob_intra_coded, 8);
    vp9_write_literal(&header_bc, pc->prob_last_coded, 8);
    vp9_write_literal(&header_bc, pc->prob_gf_coded, 8);

    {
      const int comp_pred_mode = cpi->common.comp_pred_mode;
      const int use_compound_pred = (comp_pred_mode != SINGLE_PREDICTION_ONLY);
      const int use_hybrid_pred = (comp_pred_mode == HYBRID_PREDICTION);

      vp9_write(&header_bc, use_compound_pred, 128);
      if (use_compound_pred) {
        vp9_write(&header_bc, use_hybrid_pred, 128);
        if (use_hybrid_pred) {
          for (i = 0; i < COMP_PRED_CONTEXTS; i++) {
            pc->prob_comppred[i] = get_binary_prob(cpi->single_pred_count[i],
                                                   cpi->comp_pred_count[i]);
            vp9_write_literal(&header_bc, pc->prob_comppred[i], 8);
          }
        }
      }
    }
    update_mbintra_mode_probs(cpi, &header_bc);

    vp9_write_nmv_probs(cpi, xd->allow_high_precision_mv, &header_bc);
  }

  vp9_stop_encode(&header_bc);

  oh.first_partition_length_in_bytes = header_bc.pos;

  /* update frame tag */
  {
    int v = (oh.first_partition_length_in_bytes << 5) |
            (oh.show_frame << 4) |
            (oh.version << 1) |
            oh.type;

    dest[0] = v;
    dest[1] = v >> 8;
    dest[2] = v >> 16;
  }

  *size = VP9_HEADER_SIZE + extra_bytes_packed + header_bc.pos;
  vp9_start_encode(&residual_bc, cx_data + header_bc.pos);

  if (pc->frame_type == KEY_FRAME) {
    decide_kf_ymode_entropy(cpi);
    write_modes(cpi, &residual_bc);
  } else {
    /* This is not required if the counts in cpi are consistent with the
     * final packing pass */
    // if (!cpi->dummy_packing) vp9_zero(cpi->NMVcount);
    write_modes(cpi, &residual_bc);

    vp9_update_mode_context(&cpi->common);
  }

  vp9_stop_encode(&residual_bc);

  *size += residual_bc.pos;
}

#ifdef ENTROPY_STATS
static void print_tree_update_for_type(FILE *f,
                                       vp9_coeff_stats *tree_update_hist,
                                       int block_types, const char *header) {
  int i, j, k, l;

  fprintf(f, "const vp9_coeff_prob %s = {\n", header);
  for (i = 0; i < block_types; i++) {
    fprintf(f, "  { \n");
    for (j = 0; j < COEF_BANDS; j++) {
      fprintf(f, "    {\n");
      for (k = 0; k < PREV_COEF_CONTEXTS; k++) {
        fprintf(f, "      {");
        for (l = 0; l < ENTROPY_NODES; l++) {
          fprintf(f, "%3d, ",
                  get_binary_prob(tree_update_hist[i][j][k][l][0],
                                  tree_update_hist[i][j][k][l][1]));
        }
        fprintf(f, "},\n");
      }
      fprintf(f, "    },\n");
    }
    fprintf(f, "  },\n");
  }
  fprintf(f, "};\n");
}

void print_tree_update_probs() {
  FILE *f = fopen("coefupdprob.h", "w");
  fprintf(f, "\n/* Update probabilities for token entropy tree. */\n\n");

  print_tree_update_for_type(f, tree_update_hist_4x4, BLOCK_TYPES_4X4,
                             "vp9_coef_update_probs_4x4[BLOCK_TYPES_4X4]");
  print_tree_update_for_type(f, hybrid_tree_update_hist_4x4, BLOCK_TYPES_4X4,
                             "vp9_coef_update_probs_4x4[BLOCK_TYPES_4X4]");
  print_tree_update_for_type(f, tree_update_hist_8x8, BLOCK_TYPES_8X8,
                             "vp9_coef_update_probs_8x8[BLOCK_TYPES_8X8]");
  print_tree_update_for_type(f, hybrid_tree_update_hist_8x8, BLOCK_TYPES_8X8,
                             "vp9_coef_update_probs_8x8[BLOCK_TYPES_8X8]");
  print_tree_update_for_type(f, tree_update_hist_16x16, BLOCK_TYPES_16X16,
                             "vp9_coef_update_probs_16x16[BLOCK_TYPES_16X16]");
  print_tree_update_for_type(f, hybrid_tree_update_hist_16x16,
                             BLOCK_TYPES_16X16,
                             "vp9_coef_update_probs_16x16[BLOCK_TYPES_16X16]");
  print_tree_update_for_type(f, tree_update_hist_32x32, BLOCK_TYPES_32X32,
                             "vp9_coef_update_probs_32x32[BLOCK_TYPES_32X32]");

  fclose(f);
  f = fopen("treeupdate.bin", "wb");
  fwrite(tree_update_hist_4x4, sizeof(tree_update_hist_4x4), 1, f);
  fwrite(tree_update_hist_8x8, sizeof(tree_update_hist_8x8), 1, f);
  fwrite(tree_update_hist_16x16, sizeof(tree_update_hist_16x16), 1, f);
  fclose(f);
}
#endif
