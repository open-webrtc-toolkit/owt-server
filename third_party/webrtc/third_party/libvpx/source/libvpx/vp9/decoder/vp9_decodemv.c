/*
  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/decoder/vp9_treereader.h"
#include "vp9/common/vp9_entropymv.h"
#include "vp9/common/vp9_entropymode.h"
#include "vp9/decoder/vp9_onyxd_int.h"
#include "vp9/common/vp9_findnearmv.h"
#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_seg_common.h"
#include "vp9/common/vp9_pred_common.h"
#include "vp9/common/vp9_entropy.h"
#include "vp9/decoder/vp9_decodemv.h"
#include "vp9/common/vp9_mvref_common.h"
#if CONFIG_DEBUG
#include <assert.h>
#endif

// #define DEBUG_DEC_MV
#ifdef DEBUG_DEC_MV
int dec_mvcount = 0;
#endif
// #define DEC_DEBUG
#ifdef DEC_DEBUG
extern int dec_debug;
#endif

static int read_bmode(vp9_reader *bc, const vp9_prob *p) {
  B_PREDICTION_MODE m = treed_read(bc, vp9_bmode_tree, p);
#if CONFIG_NEWBINTRAMODES
  if (m == B_CONTEXT_PRED - CONTEXT_PRED_REPLACEMENTS)
    m = B_CONTEXT_PRED;
  assert(m < B_CONTEXT_PRED - CONTEXT_PRED_REPLACEMENTS || m == B_CONTEXT_PRED);
#endif
  return m;
}

static int read_kf_bmode(vp9_reader *bc, const vp9_prob *p) {
  return treed_read(bc, vp9_kf_bmode_tree, p);
}

static int read_ymode(vp9_reader *bc, const vp9_prob *p) {
  return treed_read(bc, vp9_ymode_tree, p);
}

static int read_sb_ymode(vp9_reader *bc, const vp9_prob *p) {
  return treed_read(bc, vp9_sb_ymode_tree, p);
}

static int read_kf_sb_ymode(vp9_reader *bc, const vp9_prob *p) {
  return treed_read(bc, vp9_uv_mode_tree, p);
}

static int read_kf_mb_ymode(vp9_reader *bc, const vp9_prob *p) {
  return treed_read(bc, vp9_kf_ymode_tree, p);
}

static int read_i8x8_mode(vp9_reader *bc, const vp9_prob *p) {
  return treed_read(bc, vp9_i8x8_mode_tree, p);
}

static int read_uv_mode(vp9_reader *bc, const vp9_prob *p) {
  return treed_read(bc, vp9_uv_mode_tree, p);
}

// This function reads the current macro block's segnent id from the bitstream
// It should only be called if a segment map update is indicated.
static void read_mb_segid(vp9_reader *r, MB_MODE_INFO *mi,
                          MACROBLOCKD *xd) {
  /* Is segmentation enabled */
  if (xd->segmentation_enabled && xd->update_mb_segmentation_map) {
    /* If so then read the segment id. */
    if (vp9_read(r, xd->mb_segment_tree_probs[0]))
      mi->segment_id =
        (unsigned char)(2 + vp9_read(r, xd->mb_segment_tree_probs[2]));
    else
      mi->segment_id =
        (unsigned char)(vp9_read(r, xd->mb_segment_tree_probs[1]));
  }
}

#if CONFIG_NEW_MVREF
int vp9_read_mv_ref_id(vp9_reader *r,
                       vp9_prob * ref_id_probs) {
  int ref_index = 0;

  if (vp9_read(r, ref_id_probs[0])) {
    ref_index++;
    if (vp9_read(r, ref_id_probs[1])) {
      ref_index++;
      if (vp9_read(r, ref_id_probs[2]))
        ref_index++;
    }
  }
  return ref_index;
}
#endif

extern const int vp9_i8x8_block[4];
static void kfread_modes(VP9D_COMP *pbi,
                         MODE_INFO *m,
                         int mb_row,
                         int mb_col,
                         BOOL_DECODER* const bc) {
  VP9_COMMON *const cm = &pbi->common;
  const int mis = pbi->common.mode_info_stride;
  int map_index = mb_row * pbi->common.mb_cols + mb_col;
  MB_PREDICTION_MODE y_mode;

  // Read the Macroblock segmentation map if it is being updated explicitly
  // this frame (reset to 0 by default).
  m->mbmi.segment_id = 0;
  if (pbi->mb.update_mb_segmentation_map) {
    read_mb_segid(bc, &m->mbmi, &pbi->mb);
    if (m->mbmi.sb_type) {
      const int nmbs = 1 << m->mbmi.sb_type;
      const int ymbs = MIN(cm->mb_rows - mb_row, nmbs);
      const int xmbs = MIN(cm->mb_cols - mb_col, nmbs);
      int x, y;

      for (y = 0; y < ymbs; y++) {
        for (x = 0; x < xmbs; x++) {
          cm->last_frame_seg_map[map_index + x + y * cm->mb_cols] =
              m->mbmi.segment_id;
        }
      }
    } else {
      cm->last_frame_seg_map[map_index] = m->mbmi.segment_id;
    }
  }

  m->mbmi.mb_skip_coeff = 0;
  if (pbi->common.mb_no_coeff_skip &&
      (!vp9_segfeature_active(&pbi->mb,
                              m->mbmi.segment_id, SEG_LVL_EOB) ||
       (vp9_get_segdata(&pbi->mb,
                        m->mbmi.segment_id, SEG_LVL_EOB) != 0))) {
    MACROBLOCKD *const xd  = &pbi->mb;
    m->mbmi.mb_skip_coeff =
      vp9_read(bc, vp9_get_pred_prob(cm, xd, PRED_MBSKIP));
  } else {
    if (vp9_segfeature_active(&pbi->mb,
                              m->mbmi.segment_id, SEG_LVL_EOB) &&
        (vp9_get_segdata(&pbi->mb,
                         m->mbmi.segment_id, SEG_LVL_EOB) == 0)) {
      m->mbmi.mb_skip_coeff = 1;
    } else
      m->mbmi.mb_skip_coeff = 0;
  }

  if (m->mbmi.sb_type) {
    y_mode = (MB_PREDICTION_MODE) read_kf_sb_ymode(bc,
      pbi->common.sb_kf_ymode_prob[pbi->common.kf_ymode_probs_index]);
  } else {
    y_mode = (MB_PREDICTION_MODE) read_kf_mb_ymode(bc,
      pbi->common.kf_ymode_prob[pbi->common.kf_ymode_probs_index]);
  }

  m->mbmi.ref_frame = INTRA_FRAME;

  if ((m->mbmi.mode = y_mode) == B_PRED) {
    int i = 0;
    do {
      const B_PREDICTION_MODE A = above_block_mode(m, i, mis);
      const B_PREDICTION_MODE L = left_block_mode(m, i);

      m->bmi[i].as_mode.first =
        (B_PREDICTION_MODE) read_kf_bmode(
          bc, pbi->common.kf_bmode_prob [A] [L]);
    } while (++i < 16);
  }
  if ((m->mbmi.mode = y_mode) == I8X8_PRED) {
    int i;
    int mode8x8;
    for (i = 0; i < 4; i++) {
      int ib = vp9_i8x8_block[i];
      mode8x8 = read_i8x8_mode(bc, pbi->common.fc.i8x8_mode_prob);
      m->bmi[ib + 0].as_mode.first = mode8x8;
      m->bmi[ib + 1].as_mode.first = mode8x8;
      m->bmi[ib + 4].as_mode.first = mode8x8;
      m->bmi[ib + 5].as_mode.first = mode8x8;
    }
  } else
    m->mbmi.uv_mode = (MB_PREDICTION_MODE)read_uv_mode(bc,
                                                       pbi->common.kf_uv_mode_prob[m->mbmi.mode]);

  if (cm->txfm_mode == TX_MODE_SELECT && m->mbmi.mb_skip_coeff == 0 &&
      m->mbmi.mode <= I8X8_PRED) {
    // FIXME(rbultje) code ternary symbol once all experiments are merged
    m->mbmi.txfm_size = vp9_read(bc, cm->prob_tx[0]);
    if (m->mbmi.txfm_size != TX_4X4 && m->mbmi.mode != I8X8_PRED) {
      m->mbmi.txfm_size += vp9_read(bc, cm->prob_tx[1]);
      if (m->mbmi.txfm_size != TX_8X8 && m->mbmi.sb_type)
        m->mbmi.txfm_size += vp9_read(bc, cm->prob_tx[2]);
    }
  } else if (cm->txfm_mode >= ALLOW_32X32 && m->mbmi.sb_type) {
    m->mbmi.txfm_size = TX_32X32;
  } else if (cm->txfm_mode >= ALLOW_16X16 && m->mbmi.mode <= TM_PRED) {
    m->mbmi.txfm_size = TX_16X16;
  } else if (cm->txfm_mode >= ALLOW_8X8 && m->mbmi.mode != B_PRED) {
    m->mbmi.txfm_size = TX_8X8;
  } else {
    m->mbmi.txfm_size = TX_4X4;
  }
}

static int read_nmv_component(vp9_reader *r,
                              int rv,
                              const nmv_component *mvcomp) {
  int v, s, z, c, o, d;
  s = vp9_read(r, mvcomp->sign);
  c = treed_read(r, vp9_mv_class_tree, mvcomp->classes);
  if (c == MV_CLASS_0) {
    d = treed_read(r, vp9_mv_class0_tree, mvcomp->class0);
  } else {
    int i, b;
    d = 0;
    b = c + CLASS0_BITS - 1;  /* number of bits */
    for (i = 0; i < b; ++i)
      d |= (vp9_read(r, mvcomp->bits[i]) << i);
  }
  o = d << 3;

  z = vp9_get_mv_mag(c, o);
  v = (s ? -(z + 8) : (z + 8));
  return v;
}

static int read_nmv_component_fp(vp9_reader *r,
                                 int v,
                                 int rv,
                                 const nmv_component *mvcomp,
                                 int usehp) {
  int s, z, c, o, d, e, f;
  s = v < 0;
  z = (s ? -v : v) - 1;       /* magnitude - 1 */
  z &= ~7;

  c = vp9_get_mv_class(z, &o);
  d = o >> 3;

  if (c == MV_CLASS_0) {
    f = treed_read(r, vp9_mv_fp_tree, mvcomp->class0_fp[d]);
  } else {
    f = treed_read(r, vp9_mv_fp_tree, mvcomp->fp);
  }
  o += (f << 1);

  if (usehp) {
    if (c == MV_CLASS_0) {
      e = vp9_read(r, mvcomp->class0_hp);
    } else {
      e = vp9_read(r, mvcomp->hp);
    }
    o += e;
  } else {
    ++o;  /* Note if hp is not used, the default value of the hp bit is 1 */
  }
  z = vp9_get_mv_mag(c, o);
  v = (s ? -(z + 1) : (z + 1));
  return v;
}

static void read_nmv(vp9_reader *r, MV *mv, const MV *ref,
                     const nmv_context *mvctx) {
  MV_JOINT_TYPE j = treed_read(r, vp9_mv_joint_tree, mvctx->joints);
  mv->row = mv-> col = 0;
  if (j == MV_JOINT_HZVNZ || j == MV_JOINT_HNZVNZ) {
    mv->row = read_nmv_component(r, ref->row, &mvctx->comps[0]);
  }
  if (j == MV_JOINT_HNZVZ || j == MV_JOINT_HNZVNZ) {
    mv->col = read_nmv_component(r, ref->col, &mvctx->comps[1]);
  }
}

static void read_nmv_fp(vp9_reader *r, MV *mv, const MV *ref,
                        const nmv_context *mvctx, int usehp) {
  MV_JOINT_TYPE j = vp9_get_mv_joint(*mv);
  usehp = usehp && vp9_use_nmv_hp(ref);
  if (j == MV_JOINT_HZVNZ || j == MV_JOINT_HNZVNZ) {
    mv->row = read_nmv_component_fp(r, mv->row, ref->row, &mvctx->comps[0],
                                    usehp);
  }
  if (j == MV_JOINT_HNZVZ || j == MV_JOINT_HNZVNZ) {
    mv->col = read_nmv_component_fp(r, mv->col, ref->col, &mvctx->comps[1],
                                    usehp);
  }
  //printf("  %d: %d %d ref: %d %d\n", usehp, mv->row, mv-> col, ref->row, ref->col);
}

static void update_nmv(vp9_reader *bc, vp9_prob *const p,
                       const vp9_prob upd_p) {
  if (vp9_read(bc, upd_p)) {
#ifdef LOW_PRECISION_MV_UPDATE
    *p = (vp9_read_literal(bc, 7) << 1) | 1;
#else
    *p = (vp9_read_literal(bc, 8));
#endif
  }
}

static void read_nmvprobs(vp9_reader *bc, nmv_context *mvctx,
                          int usehp) {
  int i, j, k;
#ifdef MV_GROUP_UPDATE
  if (!vp9_read_bit(bc)) return;
#endif
  for (j = 0; j < MV_JOINTS - 1; ++j) {
    update_nmv(bc, &mvctx->joints[j],
               VP9_NMV_UPDATE_PROB);
  }
  for (i = 0; i < 2; ++i) {
    update_nmv(bc, &mvctx->comps[i].sign,
               VP9_NMV_UPDATE_PROB);
    for (j = 0; j < MV_CLASSES - 1; ++j) {
      update_nmv(bc, &mvctx->comps[i].classes[j],
                 VP9_NMV_UPDATE_PROB);
    }
    for (j = 0; j < CLASS0_SIZE - 1; ++j) {
      update_nmv(bc, &mvctx->comps[i].class0[j],
                 VP9_NMV_UPDATE_PROB);
    }
    for (j = 0; j < MV_OFFSET_BITS; ++j) {
      update_nmv(bc, &mvctx->comps[i].bits[j],
                 VP9_NMV_UPDATE_PROB);
    }
  }

  for (i = 0; i < 2; ++i) {
    for (j = 0; j < CLASS0_SIZE; ++j) {
      for (k = 0; k < 3; ++k)
        update_nmv(bc, &mvctx->comps[i].class0_fp[j][k],
                   VP9_NMV_UPDATE_PROB);
    }
    for (j = 0; j < 3; ++j) {
      update_nmv(bc, &mvctx->comps[i].fp[j],
                 VP9_NMV_UPDATE_PROB);
    }
  }

  if (usehp) {
    for (i = 0; i < 2; ++i) {
      update_nmv(bc, &mvctx->comps[i].class0_hp,
                 VP9_NMV_UPDATE_PROB);
      update_nmv(bc, &mvctx->comps[i].hp,
                 VP9_NMV_UPDATE_PROB);
    }
  }
}

// Read the referncence frame
static MV_REFERENCE_FRAME read_ref_frame(VP9D_COMP *pbi,
                                         vp9_reader *const bc,
                                         unsigned char segment_id) {
  MV_REFERENCE_FRAME ref_frame;
  int seg_ref_active;
  int seg_ref_count = 0;

  VP9_COMMON *const cm = &pbi->common;
  MACROBLOCKD *const xd = &pbi->mb;

  seg_ref_active = vp9_segfeature_active(xd,
                                         segment_id,
                                         SEG_LVL_REF_FRAME);

  // If segment coding enabled does the segment allow for more than one
  // possible reference frame
  if (seg_ref_active) {
    seg_ref_count = vp9_check_segref(xd, segment_id, INTRA_FRAME) +
                    vp9_check_segref(xd, segment_id, LAST_FRAME) +
                    vp9_check_segref(xd, segment_id, GOLDEN_FRAME) +
                    vp9_check_segref(xd, segment_id, ALTREF_FRAME);
  }

  // Segment reference frame features not available or allows for
  // multiple reference frame options
  if (!seg_ref_active || (seg_ref_count > 1)) {
    // Values used in prediction model coding
    unsigned char prediction_flag;
    vp9_prob pred_prob;
    MV_REFERENCE_FRAME pred_ref;

    // Get the context probability the prediction flag
    pred_prob = vp9_get_pred_prob(cm, xd, PRED_REF);

    // Read the prediction status flag
    prediction_flag = (unsigned char)vp9_read(bc, pred_prob);

    // Store the prediction flag.
    vp9_set_pred_flag(xd, PRED_REF, prediction_flag);

    // Get the predicted reference frame.
    pred_ref = vp9_get_pred_ref(cm, xd);

    // If correctly predicted then use the predicted value
    if (prediction_flag) {
      ref_frame = pred_ref;
    }
    // else decode the explicitly coded value
    else {
      vp9_prob mod_refprobs[PREDICTION_PROBS];
      vpx_memcpy(mod_refprobs,
                 cm->mod_refprobs[pred_ref], sizeof(mod_refprobs));

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

      // Default to INTRA_FRAME (value 0)
      ref_frame = INTRA_FRAME;

      // Do we need to decode the Intra/Inter branch
      if (mod_refprobs[0])
        ref_frame = (MV_REFERENCE_FRAME) vp9_read(bc, mod_refprobs[0]);
      else
        ref_frame++;

      if (ref_frame) {
        // Do we need to decode the Last/Gf_Arf branch
        if (mod_refprobs[1])
          ref_frame += vp9_read(bc, mod_refprobs[1]);
        else
          ref_frame++;

        if (ref_frame > 1) {
          // Do we need to decode the GF/Arf branch
          if (mod_refprobs[2])
            ref_frame += vp9_read(bc, mod_refprobs[2]);
          else {
            if (seg_ref_active) {
              if ((pred_ref == GOLDEN_FRAME) ||
                  !vp9_check_segref(xd, segment_id, GOLDEN_FRAME)) {
                ref_frame = ALTREF_FRAME;
              } else
                ref_frame = GOLDEN_FRAME;
            } else
              ref_frame = (pred_ref == GOLDEN_FRAME)
                          ? ALTREF_FRAME : GOLDEN_FRAME;
          }
        }
      }
    }
  }

  // Segment reference frame features are enabled
  else {
    // The reference frame for the mb is considered as correclty predicted
    // if it is signaled at the segment level for the purposes of the
    // common prediction model
    vp9_set_pred_flag(xd, PRED_REF, 1);
    ref_frame = vp9_get_pred_ref(cm, xd);
  }

  return (MV_REFERENCE_FRAME)ref_frame;
}

static MB_PREDICTION_MODE read_sb_mv_ref(vp9_reader *bc, const vp9_prob *p) {
  return (MB_PREDICTION_MODE) treed_read(bc, vp9_sb_mv_ref_tree, p);
}

static MB_PREDICTION_MODE read_mv_ref(vp9_reader *bc, const vp9_prob *p) {
  return (MB_PREDICTION_MODE) treed_read(bc, vp9_mv_ref_tree, p);
}

static B_PREDICTION_MODE sub_mv_ref(vp9_reader *bc, const vp9_prob *p) {
  return (B_PREDICTION_MODE) treed_read(bc, vp9_sub_mv_ref_tree, p);
}

#ifdef VPX_MODE_COUNT
unsigned int vp9_mv_cont_count[5][4] = {
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 }
};
#endif

static const unsigned char mbsplit_fill_count[4] = {8, 8, 4, 1};
static const unsigned char mbsplit_fill_offset[4][16] = {
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15},
  { 0,  1,  4,  5,  8,  9, 12, 13,  2,  3,   6,  7, 10, 11, 14, 15},
  { 0,  1,  4,  5,  2,  3,  6,  7,  8,  9,  12, 13, 10, 11, 14, 15},
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15}
};

static void read_switchable_interp_probs(VP9D_COMP* const pbi,
                                         BOOL_DECODER* const bc) {
  VP9_COMMON *const cm = &pbi->common;
  int i, j;
  for (j = 0; j <= VP9_SWITCHABLE_FILTERS; ++j) {
    for (i = 0; i < VP9_SWITCHABLE_FILTERS - 1; ++i) {
      cm->fc.switchable_interp_prob[j][i] = vp9_read_literal(bc, 8);
    }
  }
  //printf("DECODER: %d %d\n", cm->fc.switchable_interp_prob[0],
  //cm->fc.switchable_interp_prob[1]);
}

static void mb_mode_mv_init(VP9D_COMP *pbi, vp9_reader *bc) {
  VP9_COMMON *const cm = &pbi->common;
  nmv_context *const nmvc = &pbi->common.fc.nmvc;
  MACROBLOCKD *const xd  = &pbi->mb;

  if (cm->frame_type == KEY_FRAME) {
    if (!cm->kf_ymode_probs_update)
      cm->kf_ymode_probs_index = vp9_read_literal(bc, 3);
  } else {
    if (cm->mcomp_filter_type == SWITCHABLE)
      read_switchable_interp_probs(pbi, bc);
#if CONFIG_COMP_INTERINTRA_PRED
    if (cm->use_interintra) {
      if (vp9_read(bc, VP9_UPD_INTERINTRA_PROB))
        cm->fc.interintra_prob  = (vp9_prob)vp9_read_literal(bc, 8);
    }
#endif
    // Decode the baseline probabilities for decoding reference frame
    cm->prob_intra_coded = (vp9_prob)vp9_read_literal(bc, 8);
    cm->prob_last_coded  = (vp9_prob)vp9_read_literal(bc, 8);
    cm->prob_gf_coded    = (vp9_prob)vp9_read_literal(bc, 8);

    // Computes a modified set of probabilities for use when reference
    // frame prediction fails.
    vp9_compute_mod_refprobs(cm);

    pbi->common.comp_pred_mode = vp9_read(bc, 128);
    if (cm->comp_pred_mode)
      cm->comp_pred_mode += vp9_read(bc, 128);
    if (cm->comp_pred_mode == HYBRID_PREDICTION) {
      int i;
      for (i = 0; i < COMP_PRED_CONTEXTS; i++)
        cm->prob_comppred[i] = (vp9_prob)vp9_read_literal(bc, 8);
    }

    if (vp9_read_bit(bc)) {
      int i = 0;

      do {
        cm->fc.ymode_prob[i] = (vp9_prob) vp9_read_literal(bc, 8);
      } while (++i < VP9_YMODES - 1);
    }

    if (vp9_read_bit(bc)) {
      int i = 0;

      do {
        cm->fc.sb_ymode_prob[i] = (vp9_prob) vp9_read_literal(bc, 8);
      } while (++i < VP9_I32X32_MODES - 1);
    }

    read_nmvprobs(bc, nmvc, xd->allow_high_precision_mv);
  }
}

// This function either reads the segment id for the current macroblock from
// the bitstream or if the value is temporally predicted asserts the predicted
// value
static void read_mb_segment_id(VP9D_COMP *pbi,
                               int mb_row, int mb_col,
                               BOOL_DECODER* const bc) {
  VP9_COMMON *const cm = &pbi->common;
  MACROBLOCKD *const xd  = &pbi->mb;
  MODE_INFO *mi = xd->mode_info_context;
  MB_MODE_INFO *mbmi = &mi->mbmi;
  int index = mb_row * pbi->common.mb_cols + mb_col;

  if (xd->segmentation_enabled) {
    if (xd->update_mb_segmentation_map) {
      // Is temporal coding of the segment id for this mb enabled.
      if (cm->temporal_update) {
        // Get the context based probability for reading the
        // prediction status flag
        vp9_prob pred_prob =
          vp9_get_pred_prob(cm, xd, PRED_SEG_ID);

        // Read the prediction status flag
        unsigned char seg_pred_flag =
          (unsigned char)vp9_read(bc, pred_prob);

        // Store the prediction flag.
        vp9_set_pred_flag(xd, PRED_SEG_ID, seg_pred_flag);

        // If the value is flagged as correctly predicted
        // then use the predicted value
        if (seg_pred_flag) {
          mbmi->segment_id = vp9_get_pred_mb_segid(cm, xd, index);
        }
        // Else .... decode it explicitly
        else {
          read_mb_segid(bc, mbmi, xd);
        }
      }
      // Normal unpredicted coding mode
      else {
        read_mb_segid(bc, mbmi, xd);
      }
      if (mbmi->sb_type) {
        const int nmbs = 1 << mbmi->sb_type;
        const int ymbs = MIN(cm->mb_rows - mb_row, nmbs);
        const int xmbs = MIN(cm->mb_cols - mb_col, nmbs);
        int x, y;

        for (y = 0; y < ymbs; y++) {
          for (x = 0; x < xmbs; x++) {
            cm->last_frame_seg_map[index + x + y * cm->mb_cols] =
                mbmi->segment_id;
          }
        }
      } else {
        cm->last_frame_seg_map[index] = mbmi->segment_id;
      }
    } else {
      if (mbmi->sb_type) {
        const int nmbs = 1 << mbmi->sb_type;
        const int ymbs = MIN(cm->mb_rows - mb_row, nmbs);
        const int xmbs = MIN(cm->mb_cols - mb_col, nmbs);
        unsigned segment_id = -1;
        int x, y;

        for (y = 0; y < ymbs; y++) {
          for (x = 0; x < xmbs; x++) {
            segment_id = MIN(segment_id,
                             cm->last_frame_seg_map[index + x +
                                                    y * cm->mb_cols]);
          }
        }
        mbmi->segment_id = segment_id;
      } else {
        mbmi->segment_id = cm->last_frame_seg_map[index];
      }
    }
  } else {
    // The encoder explicitly sets the segment_id to 0
    // when segmentation is disabled
    mbmi->segment_id = 0;
  }
}

static void read_mb_modes_mv(VP9D_COMP *pbi, MODE_INFO *mi, MB_MODE_INFO *mbmi,
                             MODE_INFO *prev_mi,
                             int mb_row, int mb_col,
                             BOOL_DECODER* const bc) {
  VP9_COMMON *const cm = &pbi->common;
  nmv_context *const nmvc = &pbi->common.fc.nmvc;
  const int mis = pbi->common.mode_info_stride;
  MACROBLOCKD *const xd  = &pbi->mb;

  int_mv *const mv = &mbmi->mv[0];
  int mb_to_left_edge;
  int mb_to_right_edge;
  int mb_to_top_edge;
  int mb_to_bottom_edge;
  const int mb_size = 1 << mi->mbmi.sb_type;

  mb_to_top_edge = xd->mb_to_top_edge;
  mb_to_bottom_edge = xd->mb_to_bottom_edge;
  mb_to_top_edge -= LEFT_TOP_MARGIN;
  mb_to_bottom_edge += RIGHT_BOTTOM_MARGIN;
  mbmi->need_to_clamp_mvs = 0;
  mbmi->need_to_clamp_secondmv = 0;
  mbmi->second_ref_frame = NONE;
  /* Distance of Mb to the various image edges.
   * These specified to 8th pel as they are always compared to MV values that are in 1/8th pel units
   */
  xd->mb_to_left_edge =
    mb_to_left_edge = -((mb_col * 16) << 3);
  mb_to_left_edge -= LEFT_TOP_MARGIN;
  xd->mb_to_right_edge =
      mb_to_right_edge = ((pbi->common.mb_cols - mb_size - mb_col) * 16) << 3;
  mb_to_right_edge += RIGHT_BOTTOM_MARGIN;

  // Make sure the MACROBLOCKD mode info pointer is pointed at the
  // correct entry for the current macroblock.
  xd->mode_info_context = mi;
  xd->prev_mode_info_context = prev_mi;

  // Read the macroblock segment id.
  read_mb_segment_id(pbi, mb_row, mb_col, bc);

  if (pbi->common.mb_no_coeff_skip &&
      (!vp9_segfeature_active(xd,
                              mbmi->segment_id, SEG_LVL_EOB) ||
       (vp9_get_segdata(xd, mbmi->segment_id, SEG_LVL_EOB) != 0))) {
    // Read the macroblock coeff skip flag if this feature is in use,
    // else default to 0
    mbmi->mb_skip_coeff = vp9_read(bc, vp9_get_pred_prob(cm, xd, PRED_MBSKIP));
  } else {
    if (vp9_segfeature_active(xd,
                              mbmi->segment_id, SEG_LVL_EOB) &&
        (vp9_get_segdata(xd, mbmi->segment_id, SEG_LVL_EOB) == 0)) {
      mbmi->mb_skip_coeff = 1;
    } else
      mbmi->mb_skip_coeff = 0;
  }

  // Read the reference frame
  if (vp9_segfeature_active(xd, mbmi->segment_id, SEG_LVL_MODE)
      && vp9_get_segdata(xd, mbmi->segment_id, SEG_LVL_MODE) < NEARESTMV)
    mbmi->ref_frame = INTRA_FRAME;
  else
    mbmi->ref_frame = read_ref_frame(pbi, bc, mbmi->segment_id);

  // If reference frame is an Inter frame
  if (mbmi->ref_frame) {
    int_mv nearest, nearby, best_mv;
    int_mv nearest_second, nearby_second, best_mv_second;
    vp9_prob mv_ref_p [VP9_MVREFS - 1];

    int recon_y_stride, recon_yoffset;
    int recon_uv_stride, recon_uvoffset;
    MV_REFERENCE_FRAME ref_frame = mbmi->ref_frame;

    {
      int ref_fb_idx;

      /* Select the appropriate reference frame for this MB */
      if (ref_frame == LAST_FRAME)
        ref_fb_idx = cm->lst_fb_idx;
      else if (ref_frame == GOLDEN_FRAME)
        ref_fb_idx = cm->gld_fb_idx;
      else
        ref_fb_idx = cm->alt_fb_idx;

      recon_y_stride = cm->yv12_fb[ref_fb_idx].y_stride  ;
      recon_uv_stride = cm->yv12_fb[ref_fb_idx].uv_stride;

      recon_yoffset = (mb_row * recon_y_stride * 16) + (mb_col * 16);
      recon_uvoffset = (mb_row * recon_uv_stride * 8) + (mb_col * 8);

      xd->pre.y_buffer = cm->yv12_fb[ref_fb_idx].y_buffer + recon_yoffset;
      xd->pre.u_buffer = cm->yv12_fb[ref_fb_idx].u_buffer + recon_uvoffset;
      xd->pre.v_buffer = cm->yv12_fb[ref_fb_idx].v_buffer + recon_uvoffset;

#ifdef DEC_DEBUG
      if (dec_debug)
        printf("%d %d\n", xd->mode_info_context->mbmi.mv[0].as_mv.row,
               xd->mode_info_context->mbmi.mv[0].as_mv.col);
#endif
      vp9_find_mv_refs(xd, mi, prev_mi,
                       ref_frame, mbmi->ref_mvs[ref_frame],
                       cm->ref_frame_sign_bias);

      vp9_mv_ref_probs(&pbi->common, mv_ref_p,
                       mbmi->mb_mode_context[ref_frame]);

      // Is the segment level mode feature enabled for this segment
      if (vp9_segfeature_active(xd, mbmi->segment_id, SEG_LVL_MODE)) {
        mbmi->mode =
          vp9_get_segdata(xd, mbmi->segment_id, SEG_LVL_MODE);
      } else {
        if (mbmi->sb_type)
          mbmi->mode = read_sb_mv_ref(bc, mv_ref_p);
        else
          mbmi->mode = read_mv_ref(bc, mv_ref_p);

        vp9_accum_mv_refs(&pbi->common, mbmi->mode,
                          mbmi->mb_mode_context[ref_frame]);
      }

      if (mbmi->mode != ZEROMV) {
        vp9_find_best_ref_mvs(xd,
                              xd->pre.y_buffer,
                              recon_y_stride,
                              mbmi->ref_mvs[ref_frame],
                              &nearest, &nearby);

        best_mv.as_int = (mbmi->ref_mvs[ref_frame][0]).as_int;
      }

#ifdef DEC_DEBUG
      if (dec_debug)
        printf("[D %d %d] %d %d %d %d\n", ref_frame,
               mbmi->mb_mode_context[ref_frame],
               mv_ref_p[0], mv_ref_p[1], mv_ref_p[2], mv_ref_p[3]);
#endif
    }

    if (mbmi->mode >= NEARESTMV && mbmi->mode <= SPLITMV)
    {
      if (cm->mcomp_filter_type == SWITCHABLE) {
        mbmi->interp_filter = vp9_switchable_interp[
            treed_read(bc, vp9_switchable_interp_tree,
                       vp9_get_pred_probs(cm, xd, PRED_SWITCHABLE_INTERP))];
      } else {
        mbmi->interp_filter = cm->mcomp_filter_type;
      }
    }

    if (cm->comp_pred_mode == COMP_PREDICTION_ONLY ||
        (cm->comp_pred_mode == HYBRID_PREDICTION &&
         vp9_read(bc, vp9_get_pred_prob(cm, xd, PRED_COMP)))) {
      /* Since we have 3 reference frames, we can only have 3 unique
       * combinations of combinations of 2 different reference frames
       * (A-G, G-L or A-L). In the bitstream, we use this to simply
       * derive the second reference frame from the first reference
       * frame, by saying it's the next one in the enumerator, and
       * if that's > n_refs, then the second reference frame is the
       * first one in the enumerator. */
      mbmi->second_ref_frame = mbmi->ref_frame + 1;
      if (mbmi->second_ref_frame == 4)
        mbmi->second_ref_frame = 1;
      if (mbmi->second_ref_frame > 0) {
        int second_ref_fb_idx;
        /* Select the appropriate reference frame for this MB */
        if (mbmi->second_ref_frame == LAST_FRAME)
          second_ref_fb_idx = cm->lst_fb_idx;
        else if (mbmi->second_ref_frame ==
          GOLDEN_FRAME)
          second_ref_fb_idx = cm->gld_fb_idx;
        else
          second_ref_fb_idx = cm->alt_fb_idx;

        xd->second_pre.y_buffer =
          cm->yv12_fb[second_ref_fb_idx].y_buffer + recon_yoffset;
        xd->second_pre.u_buffer =
          cm->yv12_fb[second_ref_fb_idx].u_buffer + recon_uvoffset;
        xd->second_pre.v_buffer =
          cm->yv12_fb[second_ref_fb_idx].v_buffer + recon_uvoffset;

        vp9_find_mv_refs(xd, mi, prev_mi,
                         mbmi->second_ref_frame,
                         mbmi->ref_mvs[mbmi->second_ref_frame],
                         cm->ref_frame_sign_bias);

        if (mbmi->mode != ZEROMV) {
          vp9_find_best_ref_mvs(xd,
                                xd->second_pre.y_buffer,
                                recon_y_stride,
                                mbmi->ref_mvs[mbmi->second_ref_frame],
                                &nearest_second,
                                &nearby_second);
          best_mv_second = mbmi->ref_mvs[mbmi->second_ref_frame][0];
        }
      }

    } else {
#if CONFIG_COMP_INTERINTRA_PRED
      if (pbi->common.use_interintra &&
          mbmi->mode >= NEARESTMV && mbmi->mode < SPLITMV &&
          mbmi->second_ref_frame == NONE) {
        mbmi->second_ref_frame = (vp9_read(bc, pbi->common.fc.interintra_prob) ?
                                  INTRA_FRAME : NONE);
        // printf("-- %d (%d)\n", mbmi->second_ref_frame == INTRA_FRAME,
        //        pbi->common.fc.interintra_prob);
        pbi->common.fc.interintra_counts[
            mbmi->second_ref_frame == INTRA_FRAME]++;
        if (mbmi->second_ref_frame == INTRA_FRAME) {
          mbmi->interintra_mode = (MB_PREDICTION_MODE)read_ymode(
              bc, pbi->common.fc.ymode_prob);
          pbi->common.fc.ymode_counts[mbmi->interintra_mode]++;
#if SEPARATE_INTERINTRA_UV
          mbmi->interintra_uv_mode = (MB_PREDICTION_MODE)read_uv_mode(
              bc, pbi->common.fc.uv_mode_prob[mbmi->interintra_mode]);
          pbi->common.fc.uv_mode_counts[mbmi->interintra_mode]
                                       [mbmi->interintra_uv_mode]++;
#else
          mbmi->interintra_uv_mode = mbmi->interintra_mode;
#endif
          // printf("** %d %d\n",
          //        mbmi->interintra_mode, mbmi->interintra_uv_mode);
        }
      }
#endif
    }

#if CONFIG_NEW_MVREF
    // if ((mbmi->mode == NEWMV) || (mbmi->mode == SPLITMV))
    if (mbmi->mode == NEWMV) {
      int best_index;
      MV_REFERENCE_FRAME ref_frame = mbmi->ref_frame;

      // Encode the index of the choice.
      best_index =
        vp9_read_mv_ref_id(bc, xd->mb_mv_ref_probs[ref_frame]);

      best_mv.as_int = mbmi->ref_mvs[ref_frame][best_index].as_int;

      if (mbmi->second_ref_frame > 0) {
        ref_frame = mbmi->second_ref_frame;

        // Encode the index of the choice.
        best_index =
          vp9_read_mv_ref_id(bc, xd->mb_mv_ref_probs[ref_frame]);
        best_mv_second.as_int = mbmi->ref_mvs[ref_frame][best_index].as_int;
      }
    }
#endif

    mbmi->uv_mode = DC_PRED;
    switch (mbmi->mode) {
      case SPLITMV: {
        const int s = mbmi->partitioning =
                        treed_read(bc, vp9_mbsplit_tree, cm->fc.mbsplit_prob);
        const int num_p = vp9_mbsplit_count [s];
        int j = 0;
        cm->fc.mbsplit_counts[s]++;

        mbmi->need_to_clamp_mvs = 0;
        do { /* for each subset j */
          int_mv leftmv, abovemv, second_leftmv, second_abovemv;
          int_mv blockmv, secondmv;
          int k;  /* first block in subset j */
          int mv_contz;
          int blockmode;

          k = vp9_mbsplit_offset[s][j];

          leftmv.as_int = left_block_mv(mi, k);
          abovemv.as_int = above_block_mv(mi, k, mis);
          second_leftmv.as_int = 0;
          second_abovemv.as_int = 0;
          if (mbmi->second_ref_frame > 0) {
            second_leftmv.as_int = left_block_second_mv(mi, k);
            second_abovemv.as_int = above_block_second_mv(mi, k, mis);
          }
          mv_contz = vp9_mv_cont(&leftmv, &abovemv);
          blockmode = sub_mv_ref(bc, cm->fc.sub_mv_ref_prob [mv_contz]);
          cm->fc.sub_mv_ref_counts[mv_contz][blockmode - LEFT4X4]++;

          switch (blockmode) {
            case NEW4X4:
              read_nmv(bc, &blockmv.as_mv, &best_mv.as_mv, nmvc);
              read_nmv_fp(bc, &blockmv.as_mv, &best_mv.as_mv, nmvc,
                          xd->allow_high_precision_mv);
              vp9_increment_nmv(&blockmv.as_mv, &best_mv.as_mv,
                                &cm->fc.NMVcount, xd->allow_high_precision_mv);
              blockmv.as_mv.row += best_mv.as_mv.row;
              blockmv.as_mv.col += best_mv.as_mv.col;

              if (mbmi->second_ref_frame > 0) {
                read_nmv(bc, &secondmv.as_mv, &best_mv_second.as_mv, nmvc);
                read_nmv_fp(bc, &secondmv.as_mv, &best_mv_second.as_mv, nmvc,
                            xd->allow_high_precision_mv);
                vp9_increment_nmv(&secondmv.as_mv, &best_mv_second.as_mv,
                                  &cm->fc.NMVcount, xd->allow_high_precision_mv);
                secondmv.as_mv.row += best_mv_second.as_mv.row;
                secondmv.as_mv.col += best_mv_second.as_mv.col;
              }
#ifdef VPX_MODE_COUNT
              vp9_mv_cont_count[mv_contz][3]++;
#endif
              break;
            case LEFT4X4:
              blockmv.as_int = leftmv.as_int;
              if (mbmi->second_ref_frame > 0)
                secondmv.as_int = second_leftmv.as_int;
#ifdef VPX_MODE_COUNT
              vp9_mv_cont_count[mv_contz][0]++;
#endif
              break;
            case ABOVE4X4:
              blockmv.as_int = abovemv.as_int;
              if (mbmi->second_ref_frame > 0)
                secondmv.as_int = second_abovemv.as_int;
#ifdef VPX_MODE_COUNT
              vp9_mv_cont_count[mv_contz][1]++;
#endif
              break;
            case ZERO4X4:
              blockmv.as_int = 0;
              if (mbmi->second_ref_frame > 0)
                secondmv.as_int = 0;
#ifdef VPX_MODE_COUNT
              vp9_mv_cont_count[mv_contz][2]++;
#endif
              break;
            default:
              break;
          }

          /*  Commenting this section out, not sure why this was needed, and
           *  there are mismatches with this section in rare cases since it is
           *  not done in the encoder at all.
          mbmi->need_to_clamp_mvs |= check_mv_bounds(&blockmv,
                                                     mb_to_left_edge,
                                                     mb_to_right_edge,
                                                     mb_to_top_edge,
                                                     mb_to_bottom_edge);
          if (mbmi->second_ref_frame > 0) {
            mbmi->need_to_clamp_mvs |= check_mv_bounds(&secondmv,
                                                       mb_to_left_edge,
                                                       mb_to_right_edge,
                                                       mb_to_top_edge,
                                                       mb_to_bottom_edge);
          }
          */

          {
            /* Fill (uniform) modes, mvs of jth subset.
             Must do it here because ensuing subsets can
             refer back to us via "left" or "above". */
            const unsigned char *fill_offset;
            unsigned int fill_count = mbsplit_fill_count[s];

            fill_offset = &mbsplit_fill_offset[s][(unsigned char)j * mbsplit_fill_count[s]];

            do {
              mi->bmi[ *fill_offset].as_mv.first.as_int = blockmv.as_int;
              if (mbmi->second_ref_frame > 0)
                mi->bmi[ *fill_offset].as_mv.second.as_int = secondmv.as_int;
              fill_offset++;
            } while (--fill_count);
          }

        } while (++j < num_p);
      }

      mv->as_int = mi->bmi[15].as_mv.first.as_int;
      mbmi->mv[1].as_int = mi->bmi[15].as_mv.second.as_int;

      break;  /* done with SPLITMV */

      case NEARMV:
        mv->as_int = nearby.as_int;
        /* Clip "next_nearest" so that it does not extend to far out of image */
        clamp_mv(mv, mb_to_left_edge, mb_to_right_edge,
                 mb_to_top_edge, mb_to_bottom_edge);
        if (mbmi->second_ref_frame > 0) {
          mbmi->mv[1].as_int = nearby_second.as_int;
          clamp_mv(&mbmi->mv[1], mb_to_left_edge, mb_to_right_edge,
                   mb_to_top_edge, mb_to_bottom_edge);
        }
        break;

      case NEARESTMV:
        mv->as_int = nearest.as_int;
        /* Clip "next_nearest" so that it does not extend to far out of image */
        clamp_mv(mv, mb_to_left_edge, mb_to_right_edge,
                 mb_to_top_edge, mb_to_bottom_edge);
        if (mbmi->second_ref_frame > 0) {
          mbmi->mv[1].as_int = nearest_second.as_int;
          clamp_mv(&mbmi->mv[1], mb_to_left_edge, mb_to_right_edge,
                   mb_to_top_edge, mb_to_bottom_edge);
        }
        break;

      case ZEROMV:
        mv->as_int = 0;
        if (mbmi->second_ref_frame > 0)
          mbmi->mv[1].as_int = 0;
        break;

      case NEWMV:

        read_nmv(bc, &mv->as_mv, &best_mv.as_mv, nmvc);
        read_nmv_fp(bc, &mv->as_mv, &best_mv.as_mv, nmvc,
                    xd->allow_high_precision_mv);
        vp9_increment_nmv(&mv->as_mv, &best_mv.as_mv, &cm->fc.NMVcount,
                          xd->allow_high_precision_mv);

        mv->as_mv.row += best_mv.as_mv.row;
        mv->as_mv.col += best_mv.as_mv.col;

        /* Don't need to check this on NEARMV and NEARESTMV modes
         * since those modes clamp the MV. The NEWMV mode does not,
         * so signal to the prediction stage whether special
         * handling may be required.
         */
        mbmi->need_to_clamp_mvs = check_mv_bounds(mv,
                                                  mb_to_left_edge,
                                                  mb_to_right_edge,
                                                  mb_to_top_edge,
                                                  mb_to_bottom_edge);

        if (mbmi->second_ref_frame > 0) {
          read_nmv(bc, &mbmi->mv[1].as_mv, &best_mv_second.as_mv, nmvc);
          read_nmv_fp(bc, &mbmi->mv[1].as_mv, &best_mv_second.as_mv, nmvc,
                      xd->allow_high_precision_mv);
          vp9_increment_nmv(&mbmi->mv[1].as_mv, &best_mv_second.as_mv,
                            &cm->fc.NMVcount, xd->allow_high_precision_mv);
          mbmi->mv[1].as_mv.row += best_mv_second.as_mv.row;
          mbmi->mv[1].as_mv.col += best_mv_second.as_mv.col;
          mbmi->need_to_clamp_secondmv |=
            check_mv_bounds(&mbmi->mv[1],
                            mb_to_left_edge, mb_to_right_edge,
                            mb_to_top_edge, mb_to_bottom_edge);
        }
        break;
      default:
;
#if CONFIG_DEBUG
        assert(0);
#endif
    }
  } else {
    /* required for left and above block mv */
    mbmi->mv[0].as_int = 0;

    if (vp9_segfeature_active(xd, mbmi->segment_id, SEG_LVL_MODE)) {
      mbmi->mode = (MB_PREDICTION_MODE)
                   vp9_get_segdata(xd, mbmi->segment_id, SEG_LVL_MODE);
    } else if (mbmi->sb_type) {
      mbmi->mode = (MB_PREDICTION_MODE)
                   read_sb_ymode(bc, pbi->common.fc.sb_ymode_prob);
      pbi->common.fc.sb_ymode_counts[mbmi->mode]++;
    } else {
      mbmi->mode = (MB_PREDICTION_MODE)
                   read_ymode(bc, pbi->common.fc.ymode_prob);
      pbi->common.fc.ymode_counts[mbmi->mode]++;
    }

    // If MB mode is BPRED read the block modes
    if (mbmi->mode == B_PRED) {
      int j = 0;
      do {
        int m;
        m = mi->bmi[j].as_mode.first = (B_PREDICTION_MODE)
            read_bmode(bc, pbi->common.fc.bmode_prob);
#if CONFIG_NEWBINTRAMODES
        if (m == B_CONTEXT_PRED) m -= CONTEXT_PRED_REPLACEMENTS;
#endif
        pbi->common.fc.bmode_counts[m]++;
      } while (++j < 16);
    }

    if (mbmi->mode == I8X8_PRED) {
      int i;
      int mode8x8;
      for (i = 0; i < 4; i++) {
        int ib = vp9_i8x8_block[i];
        mode8x8 = read_i8x8_mode(bc, pbi->common.fc.i8x8_mode_prob);
        mi->bmi[ib + 0].as_mode.first = mode8x8;
        mi->bmi[ib + 1].as_mode.first = mode8x8;
        mi->bmi[ib + 4].as_mode.first = mode8x8;
        mi->bmi[ib + 5].as_mode.first = mode8x8;
        pbi->common.fc.i8x8_mode_counts[mode8x8]++;
      }
    } else {
      mbmi->uv_mode = (MB_PREDICTION_MODE)read_uv_mode(
        bc, pbi->common.fc.uv_mode_prob[mbmi->mode]);
      pbi->common.fc.uv_mode_counts[mbmi->mode][mbmi->uv_mode]++;
    }
  }

  if (cm->txfm_mode == TX_MODE_SELECT && mbmi->mb_skip_coeff == 0 &&
      ((mbmi->ref_frame == INTRA_FRAME && mbmi->mode <= I8X8_PRED) ||
       (mbmi->ref_frame != INTRA_FRAME && !(mbmi->mode == SPLITMV &&
                           mbmi->partitioning == PARTITIONING_4X4)))) {
    // FIXME(rbultje) code ternary symbol once all experiments are merged
    mbmi->txfm_size = vp9_read(bc, cm->prob_tx[0]);
    if (mbmi->txfm_size != TX_4X4 && mbmi->mode != I8X8_PRED &&
        mbmi->mode != SPLITMV) {
      mbmi->txfm_size += vp9_read(bc, cm->prob_tx[1]);
      if (mbmi->sb_type && mbmi->txfm_size != TX_8X8)
        mbmi->txfm_size += vp9_read(bc, cm->prob_tx[2]);
    }
  } else if (mbmi->sb_type && cm->txfm_mode >= ALLOW_32X32) {
    mbmi->txfm_size = TX_32X32;
  } else if (cm->txfm_mode >= ALLOW_16X16 &&
      ((mbmi->ref_frame == INTRA_FRAME && mbmi->mode <= TM_PRED) ||
       (mbmi->ref_frame != INTRA_FRAME && mbmi->mode != SPLITMV))) {
    mbmi->txfm_size = TX_16X16;
  } else if (cm->txfm_mode >= ALLOW_8X8 &&
      (!(mbmi->ref_frame == INTRA_FRAME && mbmi->mode == B_PRED) &&
       !(mbmi->ref_frame != INTRA_FRAME && mbmi->mode == SPLITMV &&
         mbmi->partitioning == PARTITIONING_4X4))) {
    mbmi->txfm_size = TX_8X8;
  } else {
    mbmi->txfm_size = TX_4X4;
  }
}

void vp9_decode_mode_mvs_init(VP9D_COMP* const pbi, BOOL_DECODER* const bc) {
  VP9_COMMON *cm = &pbi->common;

  vpx_memset(cm->mbskip_pred_probs, 0, sizeof(cm->mbskip_pred_probs));
  if (pbi->common.mb_no_coeff_skip) {
    int k;
    for (k = 0; k < MBSKIP_CONTEXTS; ++k)
      cm->mbskip_pred_probs[k] = (vp9_prob)vp9_read_literal(bc, 8);
  }

  mb_mode_mv_init(pbi, bc);
}
void vp9_decode_mb_mode_mv(VP9D_COMP* const pbi,
                           MACROBLOCKD* const xd,
                           int mb_row,
                           int mb_col,
                           BOOL_DECODER* const bc) {
  MODE_INFO *mi = xd->mode_info_context;
  MODE_INFO *prev_mi = xd->prev_mode_info_context;

  if (pbi->common.frame_type == KEY_FRAME)
    kfread_modes(pbi, mi, mb_row, mb_col, bc);
  else
    read_mb_modes_mv(pbi, mi, &mi->mbmi, prev_mi, mb_row, mb_col, bc);
}
