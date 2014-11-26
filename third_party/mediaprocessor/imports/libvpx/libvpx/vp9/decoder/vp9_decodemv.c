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
#include "vp9/common/vp9_reconinter.h"
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

static B_PREDICTION_MODE read_bmode(vp9_reader *bc, const vp9_prob *p) {
  B_PREDICTION_MODE m = treed_read(bc, vp9_bmode_tree, p);
#if CONFIG_NEWBINTRAMODES
  if (m == B_CONTEXT_PRED - CONTEXT_PRED_REPLACEMENTS)
    m = B_CONTEXT_PRED;
  assert(m < B_CONTEXT_PRED - CONTEXT_PRED_REPLACEMENTS || m == B_CONTEXT_PRED);
#endif
  return m;
}

static B_PREDICTION_MODE read_kf_bmode(vp9_reader *bc, const vp9_prob *p) {
  return (B_PREDICTION_MODE)treed_read(bc, vp9_kf_bmode_tree, p);
}

static MB_PREDICTION_MODE read_ymode(vp9_reader *bc, const vp9_prob *p) {
  return (MB_PREDICTION_MODE)treed_read(bc, vp9_ymode_tree, p);
}

static MB_PREDICTION_MODE read_sb_ymode(vp9_reader *bc, const vp9_prob *p) {
  return (MB_PREDICTION_MODE)treed_read(bc, vp9_sb_ymode_tree, p);
}

static MB_PREDICTION_MODE read_kf_sb_ymode(vp9_reader *bc, const vp9_prob *p) {
  return (MB_PREDICTION_MODE)treed_read(bc, vp9_uv_mode_tree, p);
}

static MB_PREDICTION_MODE read_kf_mb_ymode(vp9_reader *bc, const vp9_prob *p) {
  return (MB_PREDICTION_MODE)treed_read(bc, vp9_kf_ymode_tree, p);
}

static int read_i8x8_mode(vp9_reader *bc, const vp9_prob *p) {
  return treed_read(bc, vp9_i8x8_mode_tree, p);
}

static MB_PREDICTION_MODE read_uv_mode(vp9_reader *bc, const vp9_prob *p) {
  return (MB_PREDICTION_MODE)treed_read(bc, vp9_uv_mode_tree, p);
}

// This function reads the current macro block's segnent id from the bitstream
// It should only be called if a segment map update is indicated.
static void read_mb_segid(vp9_reader *r, MB_MODE_INFO *mi, MACROBLOCKD *xd) {
  if (xd->segmentation_enabled && xd->update_mb_segmentation_map) {
    const vp9_prob *const p = xd->mb_segment_tree_probs;
    mi->segment_id = vp9_read(r, p[0]) ? 2 + vp9_read(r, p[2])
                                       : vp9_read(r, p[1]);
  }
}

// This function reads the current macro block's segnent id from the bitstream
// It should only be called if a segment map update is indicated.
static void read_mb_segid_except(VP9_COMMON *cm,
                                 vp9_reader *r, MB_MODE_INFO *mi,
                                 MACROBLOCKD *xd, int mb_row, int mb_col) {
  const int mb_index = mb_row * cm->mb_cols + mb_col;
  const int pred_seg_id = vp9_get_pred_mb_segid(cm, xd, mb_index);
  const vp9_prob *const p = xd->mb_segment_tree_probs;
  const vp9_prob prob = xd->mb_segment_mispred_tree_probs[pred_seg_id];

  if (xd->segmentation_enabled && xd->update_mb_segmentation_map) {
    mi->segment_id = vp9_read(r, prob)
        ? 2 + (pred_seg_id  < 2 ? vp9_read(r, p[2]) : (pred_seg_id == 2))
        :     (pred_seg_id >= 2 ? vp9_read(r, p[1]) : (pred_seg_id == 0));
  }
}

#if CONFIG_NEW_MVREF
int vp9_read_mv_ref_id(vp9_reader *r, vp9_prob *ref_id_probs) {
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
  MACROBLOCKD *const xd  = &pbi->mb;
  const int mis = pbi->common.mode_info_stride;
  int map_index = mb_row * pbi->common.mb_cols + mb_col;
  MB_PREDICTION_MODE y_mode;

  m->mbmi.ref_frame = INTRA_FRAME;

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
      (!vp9_segfeature_active(&pbi->mb, m->mbmi.segment_id, SEG_LVL_SKIP))) {
    m->mbmi.mb_skip_coeff = vp9_read(bc, vp9_get_pred_prob(cm, &pbi->mb,
                                                           PRED_MBSKIP));
  } else {
    m->mbmi.mb_skip_coeff = vp9_segfeature_active(&pbi->mb, m->mbmi.segment_id,
                                                  SEG_LVL_SKIP);
  }

  y_mode = m->mbmi.sb_type ?
      read_kf_sb_ymode(bc,
          pbi->common.sb_kf_ymode_prob[pbi->common.kf_ymode_probs_index]):
      read_kf_mb_ymode(bc,
          pbi->common.kf_ymode_prob[pbi->common.kf_ymode_probs_index]);

  m->mbmi.ref_frame = INTRA_FRAME;

  if ((m->mbmi.mode = y_mode) == B_PRED) {
    int i = 0;
    do {
      const B_PREDICTION_MODE a = above_block_mode(m, i, mis);
      const B_PREDICTION_MODE l = (xd->left_available || (i & 3)) ?
                                  left_block_mode(m, i) : B_DC_PRED;

      m->bmi[i].as_mode.first = read_kf_bmode(bc,
                                              pbi->common.kf_bmode_prob[a][l]);
    } while (++i < 16);
  }

  if ((m->mbmi.mode = y_mode) == I8X8_PRED) {
    int i;
    for (i = 0; i < 4; i++) {
      const int ib = vp9_i8x8_block[i];
      const int mode8x8 = read_i8x8_mode(bc, pbi->common.fc.i8x8_mode_prob);

      m->bmi[ib + 0].as_mode.first = mode8x8;
      m->bmi[ib + 1].as_mode.first = mode8x8;
      m->bmi[ib + 4].as_mode.first = mode8x8;
      m->bmi[ib + 5].as_mode.first = mode8x8;
    }
  } else {
    m->mbmi.uv_mode = read_uv_mode(bc,
                                   pbi->common.kf_uv_mode_prob[m->mbmi.mode]);
  }

  if (cm->txfm_mode == TX_MODE_SELECT &&
      m->mbmi.mb_skip_coeff == 0 &&
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
  int mag, d;
  const int sign = vp9_read(r, mvcomp->sign);
  const int mv_class = treed_read(r, vp9_mv_class_tree, mvcomp->classes);

  if (mv_class == MV_CLASS_0) {
    d = treed_read(r, vp9_mv_class0_tree, mvcomp->class0);
  } else {
    int i;
    int n = mv_class + CLASS0_BITS - 1;  // number of bits

    d = 0;
    for (i = 0; i < n; ++i)
      d |= vp9_read(r, mvcomp->bits[i]) << i;
  }

  mag = vp9_get_mv_mag(mv_class, d << 3);
  return sign ? -(mag + 8) : (mag + 8);
}

static int read_nmv_component_fp(vp9_reader *r,
                                 int v,
                                 int rv,
                                 const nmv_component *mvcomp,
                                 int usehp) {
  const int sign = v < 0;
  int mag = ((sign ? -v : v) - 1) & ~7;  // magnitude - 1
  int offset;
  const int mv_class = vp9_get_mv_class(mag, &offset);
  const int f = mv_class == MV_CLASS_0 ?
      treed_read(r, vp9_mv_fp_tree, mvcomp->class0_fp[offset >> 3]):
      treed_read(r, vp9_mv_fp_tree, mvcomp->fp);

  offset += f << 1;

  if (usehp) {
    const vp9_prob p = mv_class == MV_CLASS_0 ? mvcomp->class0_hp : mvcomp->hp;
    offset += vp9_read(r, p);
  } else {
    offset += 1;  // If hp is not used, the default value of the hp bit is 1
  }
  mag = vp9_get_mv_mag(mv_class, offset);
  return sign ? -(mag + 1) : (mag + 1);
}

static void read_nmv(vp9_reader *r, MV *mv, const MV *ref,
                     const nmv_context *mvctx) {
  const MV_JOINT_TYPE j = treed_read(r, vp9_mv_joint_tree, mvctx->joints);
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
  const MV_JOINT_TYPE j = vp9_get_mv_joint(*mv);
  usehp = usehp && vp9_use_nmv_hp(ref);
  if (j == MV_JOINT_HZVNZ || j == MV_JOINT_HNZVNZ) {
    mv->row = read_nmv_component_fp(r, mv->row, ref->row, &mvctx->comps[0],
                                    usehp);
  }
  if (j == MV_JOINT_HNZVZ || j == MV_JOINT_HNZVNZ) {
    mv->col = read_nmv_component_fp(r, mv->col, ref->col, &mvctx->comps[1],
                                    usehp);
  }
  /*
  printf("MV: %d %d REF: %d %d\n", mv->row + ref->row, mv->col + ref->col,
	 ref->row, ref->col);
	 */
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
  if (!vp9_read_bit(bc))
    return;
#endif
  for (j = 0; j < MV_JOINTS - 1; ++j)
    update_nmv(bc, &mvctx->joints[j], VP9_NMV_UPDATE_PROB);

  for (i = 0; i < 2; ++i) {
    update_nmv(bc, &mvctx->comps[i].sign, VP9_NMV_UPDATE_PROB);
    for (j = 0; j < MV_CLASSES - 1; ++j)
      update_nmv(bc, &mvctx->comps[i].classes[j], VP9_NMV_UPDATE_PROB);

    for (j = 0; j < CLASS0_SIZE - 1; ++j)
      update_nmv(bc, &mvctx->comps[i].class0[j], VP9_NMV_UPDATE_PROB);

    for (j = 0; j < MV_OFFSET_BITS; ++j)
      update_nmv(bc, &mvctx->comps[i].bits[j], VP9_NMV_UPDATE_PROB);
  }

  for (i = 0; i < 2; ++i) {
    for (j = 0; j < CLASS0_SIZE; ++j) {
      for (k = 0; k < 3; ++k)
        update_nmv(bc, &mvctx->comps[i].class0_fp[j][k], VP9_NMV_UPDATE_PROB);
    }

    for (j = 0; j < 3; ++j)
      update_nmv(bc, &mvctx->comps[i].fp[j], VP9_NMV_UPDATE_PROB);
  }

  if (usehp) {
    for (i = 0; i < 2; ++i) {
      update_nmv(bc, &mvctx->comps[i].class0_hp, VP9_NMV_UPDATE_PROB);
      update_nmv(bc, &mvctx->comps[i].hp, VP9_NMV_UPDATE_PROB);
    }
  }
}

// Read the referncence frame
static MV_REFERENCE_FRAME read_ref_frame(VP9D_COMP *pbi,
                                         vp9_reader *const bc,
                                         unsigned char segment_id) {
  MV_REFERENCE_FRAME ref_frame;
  VP9_COMMON *const cm = &pbi->common;
  MACROBLOCKD *const xd = &pbi->mb;

  int seg_ref_count = 0;
  int seg_ref_active = vp9_segfeature_active(xd, segment_id, SEG_LVL_REF_FRAME);

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
    MV_REFERENCE_FRAME pred_ref;

    // Get the context probability the prediction flag
    vp9_prob pred_prob = vp9_get_pred_prob(cm, xd, PRED_REF);

    // Read the prediction status flag
    unsigned char prediction_flag = vp9_read(bc, pred_prob);

    // Store the prediction flag.
    vp9_set_pred_flag(xd, PRED_REF, prediction_flag);

    // Get the predicted reference frame.
    pred_ref = vp9_get_pred_ref(cm, xd);

    // If correctly predicted then use the predicted value
    if (prediction_flag) {
      ref_frame = pred_ref;
    } else {
      // decode the explicitly coded value
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
  } else {
    // Segment reference frame features are enabled
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

static const unsigned char mbsplit_fill_count[4] = { 8, 8, 4, 1 };
static const unsigned char mbsplit_fill_offset[4][16] = {
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15 },
  { 0,  1,  4,  5,  8,  9, 12, 13,  2,  3,   6,  7, 10, 11, 14, 15 },
  { 0,  1,  4,  5,  2,  3,  6,  7,  8,  9,  12, 13, 10, 11, 14, 15 },
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15 }
};

static void read_switchable_interp_probs(VP9D_COMP* const pbi,
                                         BOOL_DECODER* const bc) {
  VP9_COMMON *const cm = &pbi->common;
  int i, j;
  for (j = 0; j <= VP9_SWITCHABLE_FILTERS; ++j) {
    for (i = 0; i < VP9_SWITCHABLE_FILTERS - 1; ++i) {
      cm->fc.switchable_interp_prob[j][i] = vp9_read_prob(bc);
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
        cm->fc.interintra_prob = vp9_read_prob(bc);
    }
#endif
    // Decode the baseline probabilities for decoding reference frame
    cm->prob_intra_coded = vp9_read_prob(bc);
    cm->prob_last_coded  = vp9_read_prob(bc);
    cm->prob_gf_coded    = vp9_read_prob(bc);

    // Computes a modified set of probabilities for use when reference
    // frame prediction fails.
    vp9_compute_mod_refprobs(cm);

    pbi->common.comp_pred_mode = vp9_read(bc, 128);
    if (cm->comp_pred_mode)
      cm->comp_pred_mode += vp9_read(bc, 128);
    if (cm->comp_pred_mode == HYBRID_PREDICTION) {
      int i;
      for (i = 0; i < COMP_PRED_CONTEXTS; i++)
        cm->prob_comppred[i] = vp9_read_prob(bc);
    }

    if (vp9_read_bit(bc)) {
      int i = 0;

      do {
        cm->fc.ymode_prob[i] = vp9_read_prob(bc);
      } while (++i < VP9_YMODES - 1);
    }

    if (vp9_read_bit(bc)) {
      int i = 0;

      do {
        cm->fc.sb_ymode_prob[i] = vp9_read_prob(bc);
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
  MACROBLOCKD *const xd = &pbi->mb;
  MODE_INFO *mi = xd->mode_info_context;
  MB_MODE_INFO *mbmi = &mi->mbmi;
  int mb_index = mb_row * pbi->common.mb_cols + mb_col;

  if (xd->segmentation_enabled) {
    if (xd->update_mb_segmentation_map) {
      // Is temporal coding of the segment id for this mb enabled.
      if (cm->temporal_update) {
        // Get the context based probability for reading the
        // prediction status flag
        vp9_prob pred_prob = vp9_get_pred_prob(cm, xd, PRED_SEG_ID);

        // Read the prediction status flag
        unsigned char seg_pred_flag = vp9_read(bc, pred_prob);

        // Store the prediction flag.
        vp9_set_pred_flag(xd, PRED_SEG_ID, seg_pred_flag);

        // If the value is flagged as correctly predicted
        // then use the predicted value
        if (seg_pred_flag) {
          mbmi->segment_id = vp9_get_pred_mb_segid(cm, xd, mb_index);
        } else {
          // Decode it explicitly
          read_mb_segid_except(cm, bc, mbmi, xd, mb_row, mb_col);
        }
      } else {
        // Normal unpredicted coding mode
        read_mb_segid(bc, mbmi, xd);
      }

      if (mbmi->sb_type) {
        const int nmbs = 1 << mbmi->sb_type;
        const int ymbs = MIN(cm->mb_rows - mb_row, nmbs);
        const int xmbs = MIN(cm->mb_cols - mb_col, nmbs);
        int x, y;

        for (y = 0; y < ymbs; y++) {
          for (x = 0; x < xmbs; x++) {
            cm->last_frame_seg_map[mb_index + x + y * cm->mb_cols] =
                mbmi->segment_id;
          }
        }
      } else {
        cm->last_frame_seg_map[mb_index] = mbmi->segment_id;
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
                cm->last_frame_seg_map[mb_index + x + y * cm->mb_cols]);
          }
        }
        mbmi->segment_id = segment_id;
      } else {
        mbmi->segment_id = cm->last_frame_seg_map[mb_index];
      }
    }
  } else {
    // The encoder explicitly sets the segment_id to 0
    // when segmentation is disabled
    mbmi->segment_id = 0;
  }
}


static INLINE void assign_and_clamp_mv(int_mv *dst, const int_mv *src,
                                       int mb_to_left_edge,
                                       int mb_to_right_edge,
                                       int mb_to_top_edge,
                                       int mb_to_bottom_edge) {
  dst->as_int = src->as_int;
  clamp_mv(dst, mb_to_left_edge, mb_to_right_edge, mb_to_top_edge,
           mb_to_bottom_edge);
}

static INLINE void process_mv(BOOL_DECODER* bc, MV *mv, MV *ref,
                              nmv_context *nmvc, nmv_context_counts *mvctx,
                              int usehp) {
  read_nmv(bc, mv, ref, nmvc);
  read_nmv_fp(bc, mv, ref, nmvc, usehp);
  vp9_increment_nmv(mv, ref, mvctx, usehp);
  mv->row += ref->row;
  mv->col += ref->col;
}

static void read_mb_modes_mv(VP9D_COMP *pbi, MODE_INFO *mi, MB_MODE_INFO *mbmi,
                             MODE_INFO *prev_mi,
                             int mb_row, int mb_col,
                             BOOL_DECODER* const bc) {
  VP9_COMMON *const cm = &pbi->common;
  nmv_context *const nmvc = &pbi->common.fc.nmvc;
  const int mis = pbi->common.mode_info_stride;
  MACROBLOCKD *const xd = &pbi->mb;

  int_mv *const mv = &mbmi->mv[0];
  const int mb_size = 1 << mi->mbmi.sb_type;

  const int use_prev_in_find_mv_refs = cm->width == cm->last_width &&
                                       cm->height == cm->last_height &&
                                       !cm->error_resilient_mode;

  int mb_to_left_edge, mb_to_right_edge, mb_to_top_edge, mb_to_bottom_edge;

  mbmi->need_to_clamp_mvs = 0;
  mbmi->need_to_clamp_secondmv = 0;
  mbmi->second_ref_frame = NONE;

  // Make sure the MACROBLOCKD mode info pointer is pointed at the
  // correct entry for the current macroblock.
  xd->mode_info_context = mi;
  xd->prev_mode_info_context = prev_mi;

  // Distance of Mb to the various image edges.
  // These specified to 8th pel as they are always compared to MV values
  // that are in 1/8th pel units
  set_mb_row(cm, xd, mb_row, mb_size);
  set_mb_col(cm, xd, mb_col, mb_size);

  mb_to_top_edge = xd->mb_to_top_edge - LEFT_TOP_MARGIN;
  mb_to_bottom_edge = xd->mb_to_bottom_edge + RIGHT_BOTTOM_MARGIN;
  mb_to_left_edge = xd->mb_to_left_edge - LEFT_TOP_MARGIN;
  mb_to_right_edge = xd->mb_to_right_edge + RIGHT_BOTTOM_MARGIN;

  // Read the macroblock segment id.
  read_mb_segment_id(pbi, mb_row, mb_col, bc);

  if (pbi->common.mb_no_coeff_skip &&
      (!vp9_segfeature_active(xd, mbmi->segment_id, SEG_LVL_SKIP))) {
    // Read the macroblock coeff skip flag if this feature is in use,
    // else default to 0
    mbmi->mb_skip_coeff = vp9_read(bc, vp9_get_pred_prob(cm, xd, PRED_MBSKIP));
  } else {
    mbmi->mb_skip_coeff = vp9_segfeature_active(xd, mbmi->segment_id,
                                                SEG_LVL_SKIP);
  }

  // Read the reference frame
  mbmi->ref_frame = read_ref_frame(pbi, bc, mbmi->segment_id);

  /*
  if (pbi->common.current_video_frame == 1)
    printf("ref frame: %d [%d %d]\n", mbmi->ref_frame, mb_row, mb_col);
    */

  // If reference frame is an Inter frame
  if (mbmi->ref_frame) {
    int_mv nearest, nearby, best_mv;
    int_mv nearest_second, nearby_second, best_mv_second;
    vp9_prob mv_ref_p[VP9_MVREFS - 1];

    MV_REFERENCE_FRAME ref_frame = mbmi->ref_frame;
    xd->scale_factor[0] = cm->active_ref_scale[mbmi->ref_frame - 1];

    {
      const int use_prev_in_find_best_ref =
          xd->scale_factor[0].x_num == xd->scale_factor[0].x_den &&
          xd->scale_factor[0].y_num == xd->scale_factor[0].y_den &&
          !cm->error_resilient_mode &&
          !cm->frame_parallel_decoding_mode;

      /* Select the appropriate reference frame for this MB */
      const int ref_fb_idx = cm->active_ref_idx[ref_frame - 1];

      setup_pred_block(&xd->pre, &cm->yv12_fb[ref_fb_idx],
          mb_row, mb_col, &xd->scale_factor[0], &xd->scale_factor_uv[0]);

#ifdef DEC_DEBUG
      if (dec_debug)
        printf("%d %d\n", xd->mode_info_context->mbmi.mv[0].as_mv.row,
               xd->mode_info_context->mbmi.mv[0].as_mv.col);
#endif
      // if (cm->current_video_frame == 1 && mb_row == 4 && mb_col == 5)
      //  printf("Dello\n");
      vp9_find_mv_refs(cm, xd, mi, use_prev_in_find_mv_refs ? prev_mi : NULL,
                       ref_frame, mbmi->ref_mvs[ref_frame],
                       cm->ref_frame_sign_bias);

      vp9_mv_ref_probs(&pbi->common, mv_ref_p,
                       mbmi->mb_mode_context[ref_frame]);

      // If the segment level skip mode enabled
      if (vp9_segfeature_active(xd, mbmi->segment_id, SEG_LVL_SKIP)) {
        mbmi->mode = ZEROMV;
      } else {
        mbmi->mode = mbmi->sb_type ? read_sb_mv_ref(bc, mv_ref_p)
                                   : read_mv_ref(bc, mv_ref_p);
        vp9_accum_mv_refs(&pbi->common, mbmi->mode,
                          mbmi->mb_mode_context[ref_frame]);
      }

      if (mbmi->mode != ZEROMV) {
        vp9_find_best_ref_mvs(xd,
                              use_prev_in_find_best_ref ?
                                  xd->pre.y_buffer : NULL,
                              xd->pre.y_stride,
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

    if (mbmi->mode >= NEARESTMV && mbmi->mode <= SPLITMV) {
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
        int use_prev_in_find_best_ref;

        xd->scale_factor[1] = cm->active_ref_scale[mbmi->second_ref_frame - 1];
        use_prev_in_find_best_ref =
            xd->scale_factor[1].x_num == xd->scale_factor[1].x_den &&
            xd->scale_factor[1].y_num == xd->scale_factor[1].y_den &&
            !cm->error_resilient_mode &&
            !cm->frame_parallel_decoding_mode;

        /* Select the appropriate reference frame for this MB */
        second_ref_fb_idx = cm->active_ref_idx[mbmi->second_ref_frame - 1];

        setup_pred_block(&xd->second_pre, &cm->yv12_fb[second_ref_fb_idx],
             mb_row, mb_col, &xd->scale_factor[1], &xd->scale_factor_uv[1]);

        vp9_find_mv_refs(cm, xd, mi, use_prev_in_find_mv_refs ? prev_mi : NULL,
                         mbmi->second_ref_frame,
                         mbmi->ref_mvs[mbmi->second_ref_frame],
                         cm->ref_frame_sign_bias);

        if (mbmi->mode != ZEROMV) {
          vp9_find_best_ref_mvs(xd,
                                use_prev_in_find_best_ref ?
                                    xd->second_pre.y_buffer : NULL,
                                xd->second_pre.y_stride,
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
          mbmi->interintra_mode = read_ymode(bc, pbi->common.fc.ymode_prob);
          pbi->common.fc.ymode_counts[mbmi->interintra_mode]++;
#if SEPARATE_INTERINTRA_UV
          mbmi->interintra_uv_mode = read_uv_mode(bc,
              pbi->common.fc.uv_mode_prob[mbmi->interintra_mode]);
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
        const int s = treed_read(bc, vp9_mbsplit_tree, cm->fc.mbsplit_prob);
        const int num_p = vp9_mbsplit_count[s];
        int j = 0;

        cm->fc.mbsplit_counts[s]++;
        mbmi->need_to_clamp_mvs = 0;
        mbmi->partitioning = s;
        do {  // for each subset j
          int_mv leftmv, abovemv, second_leftmv, second_abovemv;
          int_mv blockmv, secondmv;
          int mv_contz;
          int blockmode;
          int k = vp9_mbsplit_offset[s][j];  // first block in subset j

          leftmv.as_int = left_block_mv(xd, mi, k);
          abovemv.as_int = above_block_mv(mi, k, mis);
          second_leftmv.as_int = 0;
          second_abovemv.as_int = 0;
          if (mbmi->second_ref_frame > 0) {
            second_leftmv.as_int = left_block_second_mv(xd, mi, k);
            second_abovemv.as_int = above_block_second_mv(mi, k, mis);
          }
          mv_contz = vp9_mv_cont(&leftmv, &abovemv);
          blockmode = sub_mv_ref(bc, cm->fc.sub_mv_ref_prob [mv_contz]);
          cm->fc.sub_mv_ref_counts[mv_contz][blockmode - LEFT4X4]++;

          switch (blockmode) {
            case NEW4X4:
              process_mv(bc, &blockmv.as_mv, &best_mv.as_mv, nmvc,
                         &cm->fc.NMVcount, xd->allow_high_precision_mv);

              if (mbmi->second_ref_frame > 0)
                process_mv(bc, &secondmv.as_mv, &best_mv_second.as_mv, nmvc,
                           &cm->fc.NMVcount, xd->allow_high_precision_mv);

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
            unsigned int fill_count = mbsplit_fill_count[s];
            const unsigned char *fill_offset =
                &mbsplit_fill_offset[s][j * fill_count];

            do {
              mi->bmi[*fill_offset].as_mv[0].as_int = blockmv.as_int;
              if (mbmi->second_ref_frame > 0)
                mi->bmi[*fill_offset].as_mv[1].as_int = secondmv.as_int;
              fill_offset++;
            } while (--fill_count);
          }

        } while (++j < num_p);
      }

      mv->as_int = mi->bmi[15].as_mv[0].as_int;
      mbmi->mv[1].as_int = mi->bmi[15].as_mv[1].as_int;

      break;  /* done with SPLITMV */

      case NEARMV:
        // Clip "next_nearest" so that it does not extend to far out of image
        assign_and_clamp_mv(mv, &nearby, mb_to_left_edge,
                                         mb_to_right_edge,
                                         mb_to_top_edge,
                                         mb_to_bottom_edge);
        if (mbmi->second_ref_frame > 0)
          assign_and_clamp_mv(&mbmi->mv[1], &nearby_second, mb_to_left_edge,
                                                            mb_to_right_edge,
                                                            mb_to_top_edge,
                                                            mb_to_bottom_edge);
        break;

      case NEARESTMV:
        // Clip "next_nearest" so that it does not extend to far out of image
        assign_and_clamp_mv(mv, &nearest, mb_to_left_edge,
                                          mb_to_right_edge,
                                          mb_to_top_edge,
                                          mb_to_bottom_edge);
        if (mbmi->second_ref_frame > 0)
          assign_and_clamp_mv(&mbmi->mv[1], &nearest_second, mb_to_left_edge,
                                                             mb_to_right_edge,
                                                             mb_to_top_edge,
                                                             mb_to_bottom_edge);
        break;

      case ZEROMV:
        mv->as_int = 0;
        if (mbmi->second_ref_frame > 0)
          mbmi->mv[1].as_int = 0;
        break;

      case NEWMV:
        process_mv(bc, &mv->as_mv, &best_mv.as_mv, nmvc, &cm->fc.NMVcount,
                   xd->allow_high_precision_mv);

        // Don't need to check this on NEARMV and NEARESTMV modes
        // since those modes clamp the MV. The NEWMV mode does not,
        // so signal to the prediction stage whether special
        // handling may be required.
        mbmi->need_to_clamp_mvs = check_mv_bounds(mv,
                                                  mb_to_left_edge,
                                                  mb_to_right_edge,
                                                  mb_to_top_edge,
                                                  mb_to_bottom_edge);

        if (mbmi->second_ref_frame > 0) {
          process_mv(bc, &mbmi->mv[1].as_mv, &best_mv_second.as_mv, nmvc,
                     &cm->fc.NMVcount, xd->allow_high_precision_mv);
          mbmi->need_to_clamp_secondmv |= check_mv_bounds(&mbmi->mv[1],
                                                          mb_to_left_edge,
                                                          mb_to_right_edge,
                                                          mb_to_top_edge,
                                                          mb_to_bottom_edge);
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

    if (mbmi->sb_type) {
      mbmi->mode = read_sb_ymode(bc, pbi->common.fc.sb_ymode_prob);
      pbi->common.fc.sb_ymode_counts[mbmi->mode]++;
    } else {
      mbmi->mode = read_ymode(bc, pbi->common.fc.ymode_prob);
      pbi->common.fc.ymode_counts[mbmi->mode]++;
    }

    // If MB mode is BPRED read the block modes
    if (mbmi->mode == B_PRED) {
      int j = 0;
      do {
        int m = read_bmode(bc, pbi->common.fc.bmode_prob);
        mi->bmi[j].as_mode.first = m;
#if CONFIG_NEWBINTRAMODES
        if (m == B_CONTEXT_PRED) m -= CONTEXT_PRED_REPLACEMENTS;
#endif
        pbi->common.fc.bmode_counts[m]++;
      } while (++j < 16);
    }

    if (mbmi->mode == I8X8_PRED) {
      int i;
      for (i = 0; i < 4; i++) {
        const int ib = vp9_i8x8_block[i];
        const int mode8x8 = read_i8x8_mode(bc, pbi->common.fc.i8x8_mode_prob);

        mi->bmi[ib + 0].as_mode.first = mode8x8;
        mi->bmi[ib + 1].as_mode.first = mode8x8;
        mi->bmi[ib + 4].as_mode.first = mode8x8;
        mi->bmi[ib + 5].as_mode.first = mode8x8;
        pbi->common.fc.i8x8_mode_counts[mode8x8]++;
      }
    } else {
      mbmi->uv_mode = read_uv_mode(bc, pbi->common.fc.uv_mode_prob[mbmi->mode]);
      pbi->common.fc.uv_mode_counts[mbmi->mode][mbmi->uv_mode]++;
    }
  }
  /*
  if (pbi->common.current_video_frame == 1)
    printf("mode: %d skip: %d\n", mbmi->mode, mbmi->mb_skip_coeff);
    */

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
    for (k = 0; k < MBSKIP_CONTEXTS; ++k) {
      cm->mbskip_pred_probs[k] = vp9_read_prob(bc);
    }
  }

  mb_mode_mv_init(pbi, bc);
}

#if CONFIG_CODE_NONZEROCOUNT
static uint16_t read_nzc(VP9_COMMON *const cm,
                         int nzc_context,
                         TX_SIZE tx_size,
                         int ref,
                         int type,
                         BOOL_DECODER* const bc) {
  int c, e;
  uint16_t nzc;
  if (tx_size == TX_32X32) {
    c = treed_read(bc, vp9_nzc32x32_tree,
                   cm->fc.nzc_probs_32x32[nzc_context][ref][type]);
    cm->fc.nzc_counts_32x32[nzc_context][ref][type][c]++;
  } else if (tx_size == TX_16X16) {
    c = treed_read(bc, vp9_nzc16x16_tree,
                   cm->fc.nzc_probs_16x16[nzc_context][ref][type]);
    cm->fc.nzc_counts_16x16[nzc_context][ref][type][c]++;
  } else if (tx_size == TX_8X8) {
    c = treed_read(bc, vp9_nzc8x8_tree,
                   cm->fc.nzc_probs_8x8[nzc_context][ref][type]);
    cm->fc.nzc_counts_8x8[nzc_context][ref][type][c]++;
  } else if (tx_size == TX_4X4) {
    c = treed_read(bc, vp9_nzc4x4_tree,
                   cm->fc.nzc_probs_4x4[nzc_context][ref][type]);
    cm->fc.nzc_counts_4x4[nzc_context][ref][type][c]++;
  } else {
    assert(0);
  }
  nzc = vp9_basenzcvalue[c];
  if ((e = vp9_extranzcbits[c])) {
    int x = 0;
    while (e--) {
      int b = vp9_read(
          bc, cm->fc.nzc_pcat_probs[nzc_context][c - NZC_TOKENS_NOEXTRA][e]);
      x |= (b << e);
      cm->fc.nzc_pcat_counts[nzc_context][c - NZC_TOKENS_NOEXTRA][e][b]++;
    }
    nzc += x;
  }
  if (tx_size == TX_32X32)
    assert(nzc <= 1024);
  else if (tx_size == TX_16X16)
    assert(nzc <= 256);
  else if (tx_size == TX_8X8)
    assert(nzc <= 64);
  else if (tx_size == TX_4X4)
    assert(nzc <= 16);
  return nzc;
}

static void read_nzcs_sb64(VP9_COMMON *const cm,
                           MACROBLOCKD* xd,
                           int mb_row,
                           int mb_col,
                           BOOL_DECODER* const bc) {
  MODE_INFO *m = xd->mode_info_context;
  MB_MODE_INFO *const mi = &m->mbmi;
  int j, nzc_context;
  const int ref = m->mbmi.ref_frame != INTRA_FRAME;

  assert(mb_col == get_mb_col(xd));
  assert(mb_row == get_mb_row(xd));

  vpx_memset(m->mbmi.nzcs, 0, 384 * sizeof(m->mbmi.nzcs[0]));

  if (mi->mb_skip_coeff)
    return;

  switch (mi->txfm_size) {
    case TX_32X32:
      for (j = 0; j < 256; j += 64) {
        nzc_context = vp9_get_nzc_context_y_sb64(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_32X32, ref, 0, bc);
      }
      for (j = 256; j < 384; j += 64) {
        nzc_context = vp9_get_nzc_context_uv_sb64(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_32X32, ref, 1, bc);
      }
      break;

    case TX_16X16:
      for (j = 0; j < 256; j += 16) {
        nzc_context = vp9_get_nzc_context_y_sb64(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_16X16, ref, 0, bc);
      }
      for (j = 256; j < 384; j += 16) {
        nzc_context = vp9_get_nzc_context_uv_sb64(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_16X16, ref, 1, bc);
      }
      break;

    case TX_8X8:
      for (j = 0; j < 256; j += 4) {
        nzc_context = vp9_get_nzc_context_y_sb64(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_8X8, ref, 0, bc);
      }
      for (j = 256; j < 384; j += 4) {
        nzc_context = vp9_get_nzc_context_uv_sb64(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_8X8, ref, 1, bc);
      }
      break;

    case TX_4X4:
      for (j = 0; j < 256; ++j) {
        nzc_context = vp9_get_nzc_context_y_sb64(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_4X4, ref, 0, bc);
      }
      for (j = 256; j < 384; ++j) {
        nzc_context = vp9_get_nzc_context_uv_sb64(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_4X4, ref, 1, bc);
      }
      break;

    default:
      break;
  }
}

static void read_nzcs_sb32(VP9_COMMON *const cm,
                           MACROBLOCKD* xd,
                           int mb_row,
                           int mb_col,
                           BOOL_DECODER* const bc) {
  MODE_INFO *m = xd->mode_info_context;
  MB_MODE_INFO *const mi = &m->mbmi;
  int j, nzc_context;
  const int ref = m->mbmi.ref_frame != INTRA_FRAME;

  assert(mb_col == get_mb_col(xd));
  assert(mb_row == get_mb_row(xd));

  vpx_memset(m->mbmi.nzcs, 0, 384 * sizeof(m->mbmi.nzcs[0]));

  if (mi->mb_skip_coeff)
    return;

  switch (mi->txfm_size) {
    case TX_32X32:
      for (j = 0; j < 64; j += 64) {
        nzc_context = vp9_get_nzc_context_y_sb32(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_32X32, ref, 0, bc);
      }
      for (j = 64; j < 96; j += 16) {
        nzc_context = vp9_get_nzc_context_uv_sb32(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_16X16, ref, 1, bc);
      }
      break;

    case TX_16X16:
      for (j = 0; j < 64; j += 16) {
        nzc_context = vp9_get_nzc_context_y_sb32(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_16X16, ref, 0, bc);
      }
      for (j = 64; j < 96; j += 16) {
        nzc_context = vp9_get_nzc_context_uv_sb32(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_16X16, ref, 1, bc);
      }
      break;

    case TX_8X8:
      for (j = 0; j < 64; j += 4) {
        nzc_context = vp9_get_nzc_context_y_sb32(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_8X8, ref, 0, bc);
      }
      for (j = 64; j < 96; j += 4) {
        nzc_context = vp9_get_nzc_context_uv_sb32(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_8X8, ref, 1, bc);
      }
      break;

    case TX_4X4:
      for (j = 0; j < 64; ++j) {
        nzc_context = vp9_get_nzc_context_y_sb32(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_4X4, ref, 0, bc);
      }
      for (j = 64; j < 96; ++j) {
        nzc_context = vp9_get_nzc_context_uv_sb32(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_4X4, ref, 1, bc);
      }
      break;

    default:
      break;
  }
}

static void read_nzcs_mb16(VP9_COMMON *const cm,
                           MACROBLOCKD* xd,
                           int mb_row,
                           int mb_col,
                           BOOL_DECODER* const bc) {
  MODE_INFO *m = xd->mode_info_context;
  MB_MODE_INFO *const mi = &m->mbmi;
  int j, nzc_context;
  const int ref = m->mbmi.ref_frame != INTRA_FRAME;

  assert(mb_col == get_mb_col(xd));
  assert(mb_row == get_mb_row(xd));

  vpx_memset(m->mbmi.nzcs, 0, 384 * sizeof(m->mbmi.nzcs[0]));

  if (mi->mb_skip_coeff)
    return;

  switch (mi->txfm_size) {
    case TX_16X16:
      for (j = 0; j < 16; j += 16) {
        nzc_context = vp9_get_nzc_context_y_mb16(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_16X16, ref, 0, bc);
      }
      for (j = 16; j < 24; j += 4) {
        nzc_context = vp9_get_nzc_context_uv_mb16(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_8X8, ref, 1, bc);
      }
      break;

    case TX_8X8:
      for (j = 0; j < 16; j += 4) {
        nzc_context = vp9_get_nzc_context_y_mb16(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_8X8, ref, 0, bc);
      }
      if (mi->mode == I8X8_PRED || mi->mode == SPLITMV) {
        for (j = 16; j < 24; ++j) {
          nzc_context = vp9_get_nzc_context_uv_mb16(cm, m, mb_row, mb_col, j);
          m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_4X4, ref, 1, bc);
        }
      } else {
        for (j = 16; j < 24; j += 4) {
          nzc_context = vp9_get_nzc_context_uv_mb16(cm, m, mb_row, mb_col, j);
          m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_8X8, ref, 1, bc);
        }
      }
      break;

    case TX_4X4:
      for (j = 0; j < 16; ++j) {
        nzc_context = vp9_get_nzc_context_y_mb16(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_4X4, ref, 0, bc);
      }
      for (j = 16; j < 24; ++j) {
        nzc_context = vp9_get_nzc_context_uv_mb16(cm, m, mb_row, mb_col, j);
        m->mbmi.nzcs[j] = read_nzc(cm, nzc_context, TX_4X4, ref, 1, bc);
      }
      break;

    default:
      break;
  }
}
#endif  // CONFIG_CODE_NONZEROCOUNT

void vp9_decode_mb_mode_mv(VP9D_COMP* const pbi,
                           MACROBLOCKD* const xd,
                           int mb_row,
                           int mb_col,
                           BOOL_DECODER* const bc) {
  VP9_COMMON *const cm = &pbi->common;
  MODE_INFO *mi = xd->mode_info_context;
  MODE_INFO *prev_mi = xd->prev_mode_info_context;
  MB_MODE_INFO *const mbmi = &mi->mbmi;

  if (pbi->common.frame_type == KEY_FRAME) {
    kfread_modes(pbi, mi, mb_row, mb_col, bc);
  } else {
    read_mb_modes_mv(pbi, mi, &mi->mbmi, prev_mi, mb_row, mb_col, bc);
    set_scale_factors(xd,
                      mi->mbmi.ref_frame - 1, mi->mbmi.second_ref_frame - 1,
                      pbi->common.active_ref_scale);
  }
#if CONFIG_CODE_NONZEROCOUNT
  if (mbmi->sb_type == BLOCK_SIZE_SB64X64)
    read_nzcs_sb64(cm, xd, mb_row, mb_col, bc);
  else if (mbmi->sb_type == BLOCK_SIZE_SB32X32)
    read_nzcs_sb32(cm, xd, mb_row, mb_col, bc);
  else
    read_nzcs_mb16(cm, xd, mb_row, mb_col, bc);
#endif  // CONFIG_CODE_NONZEROCOUNT

  if (mbmi->sb_type) {
    const int n_mbs = 1 << mbmi->sb_type;
    const int y_mbs = MIN(n_mbs, cm->mb_rows - mb_row);
    const int x_mbs = MIN(n_mbs, cm->mb_cols - mb_col);
    const int mis = cm->mode_info_stride;
    int x, y;

    for (y = 0; y < y_mbs; y++) {
      for (x = !y; x < x_mbs; x++) {
        mi[y * mis + x] = *mi;
      }
    }
  } else {
    update_blockd_bmi(xd);
  }
}
