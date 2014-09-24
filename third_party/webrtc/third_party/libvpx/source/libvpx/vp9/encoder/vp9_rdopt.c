/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <assert.h>
#include "vp9/common/vp9_pragmas.h"

#include "vp9/encoder/vp9_tokenize.h"
#include "vp9/encoder/vp9_treewriter.h"
#include "vp9/encoder/vp9_onyx_int.h"
#include "vp9/encoder/vp9_modecosts.h"
#include "vp9/encoder/vp9_encodeintra.h"
#include "vp9/common/vp9_entropymode.h"
#include "vp9/common/vp9_reconinter.h"
#include "vp9/common/vp9_reconintra.h"
#include "vp9/common/vp9_reconintra4x4.h"
#include "vp9/common/vp9_findnearmv.h"
#include "vp9/common/vp9_quant_common.h"
#include "vp9/encoder/vp9_encodemb.h"
#include "vp9/encoder/vp9_quantize.h"
#include "vp9/encoder/vp9_variance.h"
#include "vp9/encoder/vp9_mcomp.h"
#include "vp9/encoder/vp9_rdopt.h"
#include "vp9/encoder/vp9_ratectrl.h"
#include "vpx_mem/vpx_mem.h"
#include "vp9/common/vp9_systemdependent.h"
#include "vp9/encoder/vp9_encodemv.h"

#include "vp9/common/vp9_seg_common.h"
#include "vp9/common/vp9_pred_common.h"
#include "vp9/common/vp9_entropy.h"
#include "vp9_rtcd.h"
#include "vp9/common/vp9_mvref_common.h"
#include "vp9/common/vp9_common.h"

#define MAXF(a,b)            (((a) > (b)) ? (a) : (b))

#define INVALID_MV 0x80008000

/* Factor to weigh the rate for switchable interp filters */
#define SWITCHABLE_INTERP_RATE_FACTOR 1

static const int auto_speed_thresh[17] = {
  1000,
  200,
  150,
  130,
  150,
  125,
  120,
  115,
  115,
  115,
  115,
  115,
  115,
  115,
  115,
  115,
  105
};

const MODE_DEFINITION vp9_mode_order[MAX_MODES] = {
  {ZEROMV,    LAST_FRAME,   NONE},
  {DC_PRED,   INTRA_FRAME,  NONE},

  {NEARESTMV, LAST_FRAME,   NONE},
  {NEARMV,    LAST_FRAME,   NONE},

  {ZEROMV,    GOLDEN_FRAME, NONE},
  {NEARESTMV, GOLDEN_FRAME, NONE},

  {ZEROMV,    ALTREF_FRAME, NONE},
  {NEARESTMV, ALTREF_FRAME, NONE},

  {NEARMV,    GOLDEN_FRAME, NONE},
  {NEARMV,    ALTREF_FRAME, NONE},

  {V_PRED,    INTRA_FRAME,  NONE},
  {H_PRED,    INTRA_FRAME,  NONE},
  {D45_PRED,  INTRA_FRAME,  NONE},
  {D135_PRED, INTRA_FRAME,  NONE},
  {D117_PRED, INTRA_FRAME,  NONE},
  {D153_PRED, INTRA_FRAME,  NONE},
  {D27_PRED,  INTRA_FRAME,  NONE},
  {D63_PRED,  INTRA_FRAME,  NONE},

  {TM_PRED,   INTRA_FRAME,  NONE},

  {NEWMV,     LAST_FRAME,   NONE},
  {NEWMV,     GOLDEN_FRAME, NONE},
  {NEWMV,     ALTREF_FRAME, NONE},

  {SPLITMV,   LAST_FRAME,   NONE},
  {SPLITMV,   GOLDEN_FRAME, NONE},
  {SPLITMV,   ALTREF_FRAME, NONE},

  {B_PRED,    INTRA_FRAME,  NONE},
  {I8X8_PRED, INTRA_FRAME,  NONE},

  /* compound prediction modes */
  {ZEROMV,    LAST_FRAME,   GOLDEN_FRAME},
  {NEARESTMV, LAST_FRAME,   GOLDEN_FRAME},
  {NEARMV,    LAST_FRAME,   GOLDEN_FRAME},

  {ZEROMV,    ALTREF_FRAME, LAST_FRAME},
  {NEARESTMV, ALTREF_FRAME, LAST_FRAME},
  {NEARMV,    ALTREF_FRAME, LAST_FRAME},

  {ZEROMV,    GOLDEN_FRAME, ALTREF_FRAME},
  {NEARESTMV, GOLDEN_FRAME, ALTREF_FRAME},
  {NEARMV,    GOLDEN_FRAME, ALTREF_FRAME},

  {NEWMV,     LAST_FRAME,   GOLDEN_FRAME},
  {NEWMV,     ALTREF_FRAME, LAST_FRAME  },
  {NEWMV,     GOLDEN_FRAME, ALTREF_FRAME},

  {SPLITMV,   LAST_FRAME,   GOLDEN_FRAME},
  {SPLITMV,   ALTREF_FRAME, LAST_FRAME  },
  {SPLITMV,   GOLDEN_FRAME, ALTREF_FRAME},

#if CONFIG_COMP_INTERINTRA_PRED
  /* compound inter-intra prediction */
  {ZEROMV,    LAST_FRAME,   INTRA_FRAME},
  {NEARESTMV, LAST_FRAME,   INTRA_FRAME},
  {NEARMV,    LAST_FRAME,   INTRA_FRAME},
  {NEWMV,     LAST_FRAME,   INTRA_FRAME},

  {ZEROMV,    GOLDEN_FRAME,   INTRA_FRAME},
  {NEARESTMV, GOLDEN_FRAME,   INTRA_FRAME},
  {NEARMV,    GOLDEN_FRAME,   INTRA_FRAME},
  {NEWMV,     GOLDEN_FRAME,   INTRA_FRAME},

  {ZEROMV,    ALTREF_FRAME,   INTRA_FRAME},
  {NEARESTMV, ALTREF_FRAME,   INTRA_FRAME},
  {NEARMV,    ALTREF_FRAME,   INTRA_FRAME},
  {NEWMV,     ALTREF_FRAME,   INTRA_FRAME},
#endif
};

static void fill_token_costs(vp9_coeff_count *c,
                             vp9_coeff_probs *p,
                             int block_type_counts) {
  int i, j, k;

  for (i = 0; i < block_type_counts; i++)
    for (j = 0; j < COEF_BANDS; j++)
      for (k = 0; k < PREV_COEF_CONTEXTS; k++) {
        if (k == 0 && ((j > 0 && i > 0) || (j > 1 && i == 0)))
          vp9_cost_tokens_skip((int *)(c[i][j][k]),
                               p[i][j][k],
                               vp9_coef_tree);
        else
          vp9_cost_tokens((int *)(c[i][j][k]),
                          p[i][j][k],
                          vp9_coef_tree);
      }
}


static int rd_iifactor[32] =  { 4, 4, 3, 2, 1, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, };

// 3* dc_qlookup[Q]*dc_qlookup[Q];

/* values are now correlated to quantizer */
static int sad_per_bit16lut[QINDEX_RANGE];
static int sad_per_bit4lut[QINDEX_RANGE];

void vp9_init_me_luts() {
  int i;

  // Initialize the sad lut tables using a formulaic calculation for now
  // This is to make it easier to resolve the impact of experimental changes
  // to the quantizer tables.
  for (i = 0; i < QINDEX_RANGE; i++) {
    sad_per_bit16lut[i] =
      (int)((0.0418 * vp9_convert_qindex_to_q(i)) + 2.4107);
    sad_per_bit4lut[i] = (int)((0.063 * vp9_convert_qindex_to_q(i)) + 2.742);
  }
}

static int compute_rd_mult(int qindex) {
  int q;

  q = vp9_dc_quant(qindex, 0);
  return (11 * q * q) >> 6;
}

void vp9_initialize_me_consts(VP9_COMP *cpi, int QIndex) {
  cpi->mb.sadperbit16 =  sad_per_bit16lut[QIndex];
  cpi->mb.sadperbit4  =  sad_per_bit4lut[QIndex];
}


void vp9_initialize_rd_consts(VP9_COMP *cpi, int QIndex) {
  int q, i;

  vp9_clear_system_state();  // __asm emms;

  // Further tests required to see if optimum is different
  // for key frames, golden frames and arf frames.
  // if (cpi->common.refresh_golden_frame ||
  //     cpi->common.refresh_alt_ref_frame)
  QIndex = (QIndex < 0) ? 0 : ((QIndex > MAXQ) ? MAXQ : QIndex);

  cpi->RDMULT = compute_rd_mult(QIndex);

  // Extend rate multiplier along side quantizer zbin increases
  if (cpi->zbin_over_quant  > 0) {
    double oq_factor;

    // Experimental code using the same basic equation as used for Q above
    // The units of cpi->zbin_over_quant are 1/128 of Q bin size
    oq_factor = 1.0 + ((double)0.0015625 * cpi->zbin_over_quant);
    cpi->RDMULT = (int)((double)cpi->RDMULT * oq_factor * oq_factor);
  }

  if (cpi->pass == 2 && (cpi->common.frame_type != KEY_FRAME)) {
    if (cpi->twopass.next_iiratio > 31)
      cpi->RDMULT += (cpi->RDMULT * rd_iifactor[31]) >> 4;
    else
      cpi->RDMULT +=
        (cpi->RDMULT * rd_iifactor[cpi->twopass.next_iiratio]) >> 4;
  }

  if (cpi->RDMULT < 7)
    cpi->RDMULT = 7;

  cpi->mb.errorperbit = (cpi->RDMULT / 110);
  cpi->mb.errorperbit += (cpi->mb.errorperbit == 0);

  vp9_set_speed_features(cpi);

  q = (int)pow(vp9_dc_quant(QIndex, 0) >> 2, 1.25);
  q = q << 2;
  cpi->RDMULT = cpi->RDMULT << 4;

  if (q < 8)
    q = 8;

  if (cpi->RDMULT > 1000) {
    cpi->RDDIV = 1;
    cpi->RDMULT /= 100;

    for (i = 0; i < MAX_MODES; i++) {
      if (cpi->sf.thresh_mult[i] < INT_MAX) {
        cpi->rd_threshes[i] = cpi->sf.thresh_mult[i] * q / 100;
      } else {
        cpi->rd_threshes[i] = INT_MAX;
      }

      cpi->rd_baseline_thresh[i] = cpi->rd_threshes[i];
    }
  } else {
    cpi->RDDIV = 100;

    for (i = 0; i < MAX_MODES; i++) {
      if (cpi->sf.thresh_mult[i] < (INT_MAX / q)) {
        cpi->rd_threshes[i] = cpi->sf.thresh_mult[i] * q;
      } else {
        cpi->rd_threshes[i] = INT_MAX;
      }

      cpi->rd_baseline_thresh[i] = cpi->rd_threshes[i];
    }
  }

  fill_token_costs(cpi->mb.token_costs[TX_4X4],
                   cpi->common.fc.coef_probs_4x4, BLOCK_TYPES_4X4);
  fill_token_costs(cpi->mb.hybrid_token_costs[TX_4X4],
                   cpi->common.fc.hybrid_coef_probs_4x4, BLOCK_TYPES_4X4);

  fill_token_costs(cpi->mb.token_costs[TX_8X8],
                   cpi->common.fc.coef_probs_8x8, BLOCK_TYPES_8X8);
  fill_token_costs(cpi->mb.hybrid_token_costs[TX_8X8],
                   cpi->common.fc.hybrid_coef_probs_8x8, BLOCK_TYPES_8X8);

  fill_token_costs(cpi->mb.token_costs[TX_16X16],
                   cpi->common.fc.coef_probs_16x16, BLOCK_TYPES_16X16);
  fill_token_costs(cpi->mb.hybrid_token_costs[TX_16X16],
                   cpi->common.fc.hybrid_coef_probs_16x16, BLOCK_TYPES_16X16);

  fill_token_costs(cpi->mb.token_costs[TX_32X32],
                   cpi->common.fc.coef_probs_32x32, BLOCK_TYPES_32X32);

  /*rough estimate for costing*/
  cpi->common.kf_ymode_probs_index = cpi->common.base_qindex >> 4;
  vp9_init_mode_costs(cpi);

  if (cpi->common.frame_type != KEY_FRAME) {
    vp9_build_nmv_cost_table(
        cpi->mb.nmvjointcost,
        cpi->mb.e_mbd.allow_high_precision_mv ?
        cpi->mb.nmvcost_hp : cpi->mb.nmvcost,
        &cpi->common.fc.nmvc,
        cpi->mb.e_mbd.allow_high_precision_mv, 1, 1);
  }
}

int vp9_block_error_c(int16_t *coeff, int16_t *dqcoeff, int block_size) {
  int i, error = 0;

  for (i = 0; i < block_size; i++) {
    int this_diff = coeff[i] - dqcoeff[i];
    error += this_diff * this_diff;
  }

  return error;
}

int vp9_mbblock_error_8x8_c(MACROBLOCK *mb, int dc) {
  BLOCK  *be;
  BLOCKD *bd;
  int i, j;
  int berror, error = 0;

  for (i = 0; i < 16; i+=4) {
    be = &mb->block[i];
    bd = &mb->e_mbd.block[i];
    berror = 0;
    for (j = dc; j < 64; j++) {
      int this_diff = be->coeff[j] - bd->dqcoeff[j];
      berror += this_diff * this_diff;
    }
    error += berror;
  }
  return error;
}

int vp9_mbblock_error_c(MACROBLOCK *mb, int dc) {
  BLOCK  *be;
  BLOCKD *bd;
  int i, j;
  int berror, error = 0;

  for (i = 0; i < 16; i++) {
    be = &mb->block[i];
    bd = &mb->e_mbd.block[i];
    berror = 0;
    for (j = dc; j < 16; j++) {
      int this_diff = be->coeff[j] - bd->dqcoeff[j];
      berror += this_diff * this_diff;
    }
    error += berror;
  }
  return error;
}

int vp9_mbuverror_c(MACROBLOCK *mb) {
  BLOCK  *be;
  BLOCKD *bd;

  int i, error = 0;

  for (i = 16; i < 24; i++) {
    be = &mb->block[i];
    bd = &mb->e_mbd.block[i];

    error += vp9_block_error_c(be->coeff, bd->dqcoeff, 16);
  }

  return error;
}

int vp9_uvsse(MACROBLOCK *x) {
  uint8_t *uptr, *vptr;
  uint8_t *upred_ptr = (*(x->block[16].base_src) + x->block[16].src);
  uint8_t *vpred_ptr = (*(x->block[20].base_src) + x->block[20].src);
  int uv_stride = x->block[16].src_stride;

  unsigned int sse1 = 0;
  unsigned int sse2 = 0;
  int mv_row = x->e_mbd.mode_info_context->mbmi.mv[0].as_mv.row;
  int mv_col = x->e_mbd.mode_info_context->mbmi.mv[0].as_mv.col;
  int offset;
  int pre_stride = x->e_mbd.block[16].pre_stride;

  if (mv_row < 0)
    mv_row -= 1;
  else
    mv_row += 1;

  if (mv_col < 0)
    mv_col -= 1;
  else
    mv_col += 1;

  mv_row /= 2;
  mv_col /= 2;

  offset = (mv_row >> 3) * pre_stride + (mv_col >> 3);
  uptr = x->e_mbd.pre.u_buffer + offset;
  vptr = x->e_mbd.pre.v_buffer + offset;

  if ((mv_row | mv_col) & 7) {
    vp9_sub_pixel_variance8x8(uptr, pre_stride, (mv_col & 7) << 1,
                              (mv_row & 7) << 1, upred_ptr, uv_stride, &sse2);
    vp9_sub_pixel_variance8x8(vptr, pre_stride, (mv_col & 7) << 1,
                              (mv_row & 7) << 1, vpred_ptr, uv_stride, &sse1);
    sse2 += sse1;
  } else {
    vp9_variance8x8(uptr, pre_stride, upred_ptr, uv_stride, &sse2);
    vp9_variance8x8(vptr, pre_stride, vpred_ptr, uv_stride, &sse1);
    sse2 += sse1;
  }
  return sse2;

}

#if CONFIG_NEWCOEFCONTEXT
#define PT pn
#else
#define PT pt
#endif
static int cost_coeffs(MACROBLOCK *mb,
                       BLOCKD *b, PLANE_TYPE type,
                       ENTROPY_CONTEXT *a,
                       ENTROPY_CONTEXT *l,
                       TX_SIZE tx_size) {
  int pt;
  const int eob = b->eob;
  MACROBLOCKD *xd = &mb->e_mbd;
  const int ib = (int)(b - xd->block);
  int c = (type == PLANE_TYPE_Y_NO_DC) ? 1 : 0;
  int cost = 0, seg_eob;
  const int segment_id = xd->mode_info_context->mbmi.segment_id;
  const int *scan, *band;
  int16_t *qcoeff_ptr = b->qcoeff;
  const TX_TYPE tx_type = (type == PLANE_TYPE_Y_WITH_DC) ?
                          get_tx_type(xd, b) : DCT_DCT;
#if CONFIG_NEWCOEFCONTEXT
  const int *neighbors;
  int pn;
#endif

  ENTROPY_CONTEXT a_ec = *a, l_ec = *l;

  switch (tx_size) {
    case TX_4X4:
      scan = vp9_default_zig_zag1d_4x4;
      band = vp9_coef_bands_4x4;
      seg_eob = 16;
      if (type == PLANE_TYPE_Y_WITH_DC) {
        if (tx_type == ADST_DCT) {
          scan = vp9_row_scan_4x4;
        } else if (tx_type == DCT_ADST) {
          scan = vp9_col_scan_4x4;
        }
      }
      break;
    case TX_8X8:
      if (type == PLANE_TYPE_Y2) {
        scan = vp9_default_zig_zag1d_4x4;
        band = vp9_coef_bands_4x4;
        seg_eob = 4;
      } else {
        scan = vp9_default_zig_zag1d_8x8;
        band = vp9_coef_bands_8x8;
        seg_eob = 64;
      }
      break;
    case TX_16X16:
      scan = vp9_default_zig_zag1d_16x16;
      band = vp9_coef_bands_16x16;
      seg_eob = 256;
      if (type == PLANE_TYPE_UV) {
        const int uv_idx = ib - 16;
        qcoeff_ptr = xd->sb_coeff_data.qcoeff + 1024 + 64 * uv_idx;
      }
      break;
    case TX_32X32:
      scan = vp9_default_zig_zag1d_32x32;
      band = vp9_coef_bands_32x32;
      seg_eob = 1024;
      qcoeff_ptr = xd->sb_coeff_data.qcoeff;
      break;
    default:
      abort();
      break;
  }

  VP9_COMBINEENTROPYCONTEXTS(pt, a_ec, l_ec);
#if CONFIG_NEWCOEFCONTEXT
  neighbors = vp9_get_coef_neighbors_handle(scan);
  pn = pt;
#endif

  if (vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB))
    seg_eob = vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);

  if (tx_type != DCT_DCT) {
    for (; c < eob; c++) {
      int v = qcoeff_ptr[scan[c]];
      int t = vp9_dct_value_tokens_ptr[v].Token;
      cost += mb->hybrid_token_costs[tx_size][type][band[c]][PT][t];
      cost += vp9_dct_value_cost_ptr[v];
      pt = vp9_prev_token_class[t];
#if CONFIG_NEWCOEFCONTEXT
      if (c < seg_eob - 1 && NEWCOEFCONTEXT_BAND_COND(band[c + 1]))
        pn = vp9_get_coef_neighbor_context(
            qcoeff_ptr, (type == PLANE_TYPE_Y_NO_DC), neighbors, scan[c + 1]);
      else
        pn = pt;
#endif
    }
    if (c < seg_eob)
      cost += mb->hybrid_token_costs[tx_size][type][band[c]]
          [PT][DCT_EOB_TOKEN];
  } else {
    for (; c < eob; c++) {
      int v = qcoeff_ptr[scan[c]];
      int t = vp9_dct_value_tokens_ptr[v].Token;
      cost += mb->token_costs[tx_size][type][band[c]][pt][t];
      cost += vp9_dct_value_cost_ptr[v];
      pt = vp9_prev_token_class[t];
#if CONFIG_NEWCOEFCONTEXT
      if (c < seg_eob - 1 && NEWCOEFCONTEXT_BAND_COND(band[c + 1]))
        pn = vp9_get_coef_neighbor_context(
            qcoeff_ptr, (type == PLANE_TYPE_Y_NO_DC), neighbors, scan[c + 1]);
      else
        pn = pt;
#endif
    }
    if (c < seg_eob)
      cost += mb->token_costs[tx_size][type][band[c]]
          [PT][DCT_EOB_TOKEN];
  }

  // is eob first coefficient;
  pt = (c > !type);
  *a = *l = pt;
  return cost;
}

static int rdcost_mby_4x4(MACROBLOCK *mb, int has_2nd_order, int backup) {
  int cost = 0;
  int b;
  MACROBLOCKD *xd = &mb->e_mbd;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta;
  ENTROPY_CONTEXT *tl;

  if (backup) {
    vpx_memcpy(&t_above, xd->above_context, sizeof(ENTROPY_CONTEXT_PLANES));
    vpx_memcpy(&t_left, xd->left_context, sizeof(ENTROPY_CONTEXT_PLANES));

    ta = (ENTROPY_CONTEXT *)&t_above;
    tl = (ENTROPY_CONTEXT *)&t_left;
  } else {
    ta = (ENTROPY_CONTEXT *)xd->above_context;
    tl = (ENTROPY_CONTEXT *)xd->left_context;
  }

  for (b = 0; b < 16; b++)
    cost += cost_coeffs(mb, xd->block + b,
                        (has_2nd_order ?
                         PLANE_TYPE_Y_NO_DC : PLANE_TYPE_Y_WITH_DC),
                        ta + vp9_block2above[TX_4X4][b],
                        tl + vp9_block2left[TX_4X4][b],
                        TX_4X4);

  if (has_2nd_order)
    cost += cost_coeffs(mb, xd->block + 24, PLANE_TYPE_Y2,
                        ta + vp9_block2above[TX_4X4][24],
                        tl + vp9_block2left[TX_4X4][24],
                        TX_4X4);

  return cost;
}

static void macro_block_yrd_4x4(MACROBLOCK *mb,
                                int *Rate,
                                int *Distortion,
                                int *skippable, int backup) {
  MACROBLOCKD *const xd = &mb->e_mbd;
  BLOCK   *const mb_y2 = mb->block + 24;
  BLOCKD *const x_y2  = xd->block + 24;
  int d, has_2nd_order;

  xd->mode_info_context->mbmi.txfm_size = TX_4X4;
  has_2nd_order = get_2nd_order_usage(xd);
  // Fdct and building the 2nd order block
  vp9_transform_mby_4x4(mb);
  vp9_quantize_mby_4x4(mb);
  d = vp9_mbblock_error(mb, has_2nd_order);
  if (has_2nd_order)
    d += vp9_block_error(mb_y2->coeff, x_y2->dqcoeff, 16);

  *Distortion = (d >> 2);
  // rate
  *Rate = rdcost_mby_4x4(mb, has_2nd_order, backup);
  *skippable = vp9_mby_is_skippable_4x4(&mb->e_mbd, has_2nd_order);
}

static int rdcost_mby_8x8(MACROBLOCK *mb, int has_2nd_order, int backup) {
  int cost = 0;
  int b;
  MACROBLOCKD *xd = &mb->e_mbd;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta;
  ENTROPY_CONTEXT *tl;

  if (backup) {
    vpx_memcpy(&t_above,xd->above_context, sizeof(ENTROPY_CONTEXT_PLANES));
    vpx_memcpy(&t_left, xd->left_context, sizeof(ENTROPY_CONTEXT_PLANES));

    ta = (ENTROPY_CONTEXT *)&t_above;
    tl = (ENTROPY_CONTEXT *)&t_left;
  } else {
    ta = (ENTROPY_CONTEXT *)mb->e_mbd.above_context;
    tl = (ENTROPY_CONTEXT *)mb->e_mbd.left_context;
  }

  for (b = 0; b < 16; b += 4)
    cost += cost_coeffs(mb, xd->block + b,
                        (has_2nd_order ?
                         PLANE_TYPE_Y_NO_DC : PLANE_TYPE_Y_WITH_DC),
                        ta + vp9_block2above[TX_8X8][b],
                        tl + vp9_block2left[TX_8X8][b],
                        TX_8X8);

  if (has_2nd_order)
    cost += cost_coeffs(mb, xd->block + 24, PLANE_TYPE_Y2,
                            ta + vp9_block2above[TX_8X8][24],
                            tl + vp9_block2left[TX_8X8][24],
                            TX_8X8);
  return cost;
}

static void macro_block_yrd_8x8(MACROBLOCK *mb,
                                int *Rate,
                                int *Distortion,
                                int *skippable, int backup) {
  MACROBLOCKD *const xd = &mb->e_mbd;
  BLOCK   *const mb_y2 = mb->block + 24;
  BLOCKD *const x_y2  = xd->block + 24;
  int d, has_2nd_order;

  xd->mode_info_context->mbmi.txfm_size = TX_8X8;

  vp9_transform_mby_8x8(mb);
  vp9_quantize_mby_8x8(mb);
  has_2nd_order = get_2nd_order_usage(xd);
  d = vp9_mbblock_error_8x8_c(mb, has_2nd_order);
  if (has_2nd_order)
    d += vp9_block_error(mb_y2->coeff, x_y2->dqcoeff, 16);

  *Distortion = (d >> 2);
  // rate
  *Rate = rdcost_mby_8x8(mb, has_2nd_order, backup);
  *skippable = vp9_mby_is_skippable_8x8(&mb->e_mbd, has_2nd_order);
}

static int rdcost_mby_16x16(MACROBLOCK *mb, int backup) {
  int cost;
  MACROBLOCKD *xd = &mb->e_mbd;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta, *tl;

  if (backup) {
    vpx_memcpy(&t_above, xd->above_context, sizeof(ENTROPY_CONTEXT_PLANES));
    vpx_memcpy(&t_left, xd->left_context, sizeof(ENTROPY_CONTEXT_PLANES));

    ta = (ENTROPY_CONTEXT *)&t_above;
    tl = (ENTROPY_CONTEXT *)&t_left;
  } else {
    ta = (ENTROPY_CONTEXT *)xd->above_context;
    tl = (ENTROPY_CONTEXT *)xd->left_context;
  }

  cost = cost_coeffs(mb, xd->block, PLANE_TYPE_Y_WITH_DC, ta, tl, TX_16X16);
  return cost;
}

static void macro_block_yrd_16x16(MACROBLOCK *mb, int *Rate, int *Distortion,
                                  int *skippable, int backup) {
  int d;
  MACROBLOCKD *xd = &mb->e_mbd;

  xd->mode_info_context->mbmi.txfm_size = TX_16X16;
  vp9_transform_mby_16x16(mb);
  vp9_quantize_mby_16x16(mb);
  // TODO(jingning) is it possible to quickly determine whether to force
  //                trailing coefficients to be zero, instead of running trellis
  //                optimization in the rate-distortion optimization loop?
  if (mb->e_mbd.mode_info_context->mbmi.mode < I8X8_PRED)
    vp9_optimize_mby_16x16(mb);

  d = vp9_mbblock_error(mb, 0);

  *Distortion = (d >> 2);
  // rate
  *Rate = rdcost_mby_16x16(mb, backup);
  *skippable = vp9_mby_is_skippable_16x16(&mb->e_mbd);
}

static void choose_txfm_size_from_rd(VP9_COMP *cpi, MACROBLOCK *x,
                                     int (*r)[2], int *rate,
                                     int *d, int *distortion,
                                     int *s, int *skip,
                                     int64_t txfm_cache[NB_TXFM_MODES],
                                     TX_SIZE max_txfm_size) {
  VP9_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = &xd->mode_info_context->mbmi;
  vp9_prob skip_prob = cm->mb_no_coeff_skip ?
                       vp9_get_pred_prob(cm, xd, PRED_MBSKIP) : 128;
  int64_t rd[TX_SIZE_MAX_SB][2];
  int n, m;

  for (n = TX_4X4; n <= max_txfm_size; n++) {
    r[n][1] = r[n][0];
    for (m = 0; m <= n - (n == max_txfm_size); m++) {
      if (m == n)
        r[n][1] += vp9_cost_zero(cm->prob_tx[m]);
      else
        r[n][1] += vp9_cost_one(cm->prob_tx[m]);
    }
  }

  if (cm->mb_no_coeff_skip) {
    int s0, s1;

    assert(skip_prob > 0);
    s0 = vp9_cost_bit(skip_prob, 0);
    s1 = vp9_cost_bit(skip_prob, 1);

    for (n = TX_4X4; n <= max_txfm_size; n++) {
      if (s[n]) {
        rd[n][0] = rd[n][1] = RDCOST(x->rdmult, x->rddiv, s1, d[n]);
      } else {
        rd[n][0] = RDCOST(x->rdmult, x->rddiv, r[n][0] + s0, d[n]);
        rd[n][1] = RDCOST(x->rdmult, x->rddiv, r[n][1] + s0, d[n]);
      }
    }
  } else {
    for (n = TX_4X4; n <= max_txfm_size; n++) {
      rd[n][0] = RDCOST(x->rdmult, x->rddiv, r[n][0], d[n]);
      rd[n][1] = RDCOST(x->rdmult, x->rddiv, r[n][1], d[n]);
    }
  }

  if (max_txfm_size == TX_32X32 &&
      (cm->txfm_mode == ALLOW_32X32 ||
       (cm->txfm_mode == TX_MODE_SELECT &&
        rd[TX_32X32][1] < rd[TX_16X16][1] && rd[TX_32X32][1] < rd[TX_8X8][1] &&
        rd[TX_32X32][1] < rd[TX_4X4][1]))) {
    mbmi->txfm_size = TX_32X32;
  } else if ( cm->txfm_mode == ALLOW_16X16 ||
             (max_txfm_size == TX_16X16 && cm->txfm_mode == ALLOW_32X32) ||
             (cm->txfm_mode == TX_MODE_SELECT &&
              rd[TX_16X16][1] < rd[TX_8X8][1] &&
              rd[TX_16X16][1] < rd[TX_4X4][1])) {
    mbmi->txfm_size = TX_16X16;
  } else if (cm->txfm_mode == ALLOW_8X8 ||
           (cm->txfm_mode == TX_MODE_SELECT && rd[TX_8X8][1] < rd[TX_4X4][1])) {
    mbmi->txfm_size = TX_8X8;
  } else {
    assert(cm->txfm_mode == ONLY_4X4 || cm->txfm_mode == TX_MODE_SELECT);
    mbmi->txfm_size = TX_4X4;
  }

  *distortion = d[mbmi->txfm_size];
  *rate       = r[mbmi->txfm_size][cm->txfm_mode == TX_MODE_SELECT];
  *skip       = s[mbmi->txfm_size];

  txfm_cache[ONLY_4X4] = rd[TX_4X4][0];
  txfm_cache[ALLOW_8X8] = rd[TX_8X8][0];
  txfm_cache[ALLOW_16X16] = rd[TX_16X16][0];
  txfm_cache[ALLOW_32X32] = rd[max_txfm_size][0];
  if (max_txfm_size == TX_32X32 &&
      rd[TX_32X32][1] < rd[TX_16X16][1] && rd[TX_32X32][1] < rd[TX_8X8][1] &&
      rd[TX_32X32][1] < rd[TX_4X4][1])
    txfm_cache[TX_MODE_SELECT] = rd[TX_32X32][1];
  else if (rd[TX_16X16][1] < rd[TX_8X8][1] && rd[TX_16X16][1] < rd[TX_4X4][1])
    txfm_cache[TX_MODE_SELECT] = rd[TX_16X16][1];
  else
    txfm_cache[TX_MODE_SELECT] = rd[TX_4X4][1] < rd[TX_8X8][1] ?
                                 rd[TX_4X4][1] : rd[TX_8X8][1];
}

static void macro_block_yrd(VP9_COMP *cpi, MACROBLOCK *x, int *rate,
                            int *distortion, int *skippable,
                            int64_t txfm_cache[NB_TXFM_MODES]) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int r[TX_SIZE_MAX_MB][2], d[TX_SIZE_MAX_MB], s[TX_SIZE_MAX_MB];

  vp9_subtract_mby(x->src_diff, *(x->block[0].base_src), xd->predictor,
                   x->block[0].src_stride);

  macro_block_yrd_16x16(x, &r[TX_16X16][0], &d[TX_16X16], &s[TX_16X16], 1);
  macro_block_yrd_8x8(x, &r[TX_8X8][0], &d[TX_8X8], &s[TX_8X8], 1);
  macro_block_yrd_4x4(x, &r[TX_4X4][0], &d[TX_4X4], &s[TX_4X4], 1);

  choose_txfm_size_from_rd(cpi, x, r, rate, d, distortion, s, skippable,
                           txfm_cache, TX_16X16);
}

static void copy_predictor(uint8_t *dst, const uint8_t *predictor) {
  const unsigned int *p = (const unsigned int *)predictor;
  unsigned int *d = (unsigned int *)dst;
  d[0] = p[0];
  d[4] = p[4];
  d[8] = p[8];
  d[12] = p[12];
}

static int rdcost_sby_32x32(MACROBLOCK *x, int backup) {
  MACROBLOCKD * const xd = &x->e_mbd;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta, *tl;

  if (backup) {
    ta = (ENTROPY_CONTEXT *) &t_above,
    tl = (ENTROPY_CONTEXT *) &t_left;

    vpx_memcpy(&t_above, xd->above_context, sizeof(ENTROPY_CONTEXT_PLANES));
    vpx_memcpy(&t_left,  xd->left_context,  sizeof(ENTROPY_CONTEXT_PLANES));
  } else {
    ta = (ENTROPY_CONTEXT *) xd->above_context;
    tl = (ENTROPY_CONTEXT *) xd->left_context;
  }

  return cost_coeffs(x, xd->block, PLANE_TYPE_Y_WITH_DC, ta, tl, TX_32X32);
}

static int vp9_sb_block_error_c(int16_t *coeff, int16_t *dqcoeff,
                                int block_size) {
  int i;
  int64_t error = 0;

  for (i = 0; i < block_size; i++) {
    unsigned int this_diff = coeff[i] - dqcoeff[i];
    error += this_diff * this_diff;
  }

  return error > INT_MAX ? INT_MAX : (int)error;
}

#define DEBUG_ERROR 0
static void super_block_yrd_32x32(MACROBLOCK *x,
                                  int *rate, int *distortion, int *skippable,
                                  int backup) {
  SUPERBLOCK  * const x_sb = &x->sb_coeff_data;
  MACROBLOCKD * const xd = &x->e_mbd;
  SUPERBLOCKD * const xd_sb = &xd->sb_coeff_data;
#if DEBUG_ERROR || CONFIG_DWTDCTHYBRID
  int16_t out[1024];
#endif

  vp9_transform_sby_32x32(x);
  vp9_quantize_sby_32x32(x);
#if DEBUG_ERROR || CONFIG_DWTDCTHYBRID
  vp9_short_idct32x32(xd_sb->dqcoeff, out, 64);
#endif

#if !CONFIG_DWTDCTHYBRID
  *distortion = vp9_sb_block_error_c(x_sb->coeff, xd_sb->dqcoeff, 1024);
#else
  *distortion = vp9_block_error_c(x_sb->src_diff, out, 1024) << 4;
#endif
#if DEBUG_ERROR
  printf("IDCT/FDCT error 32x32: %d (d: %d)\n",
         vp9_block_error_c(x_sb->src_diff, out, 1024), *distortion);
#endif
  *rate       = rdcost_sby_32x32(x, backup);
  *skippable  = vp9_sby_is_skippable_32x32(&x->e_mbd);
}

static void super_block_yrd(VP9_COMP *cpi,
                            MACROBLOCK *x, int *rate, int *distortion,
                            int *skip,
                            int64_t txfm_cache[NB_TXFM_MODES]) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int r[TX_SIZE_MAX_SB][2], d[TX_SIZE_MAX_SB], s[TX_SIZE_MAX_SB], n;
  const uint8_t *src = x->src.y_buffer, *dst = xd->dst.y_buffer;
  int src_y_stride = x->src.y_stride, dst_y_stride = xd->dst.y_stride;
  ENTROPY_CONTEXT_PLANES t_above[TX_SIZE_MAX_MB][2],
                        *orig_above = xd->above_context;
  ENTROPY_CONTEXT_PLANES t_left[TX_SIZE_MAX_MB][2],
                        *orig_left = xd->left_context;

  for (n = TX_4X4; n < TX_SIZE_MAX_MB; n++) {
    vpx_memcpy(t_above[n], xd->above_context, sizeof(t_above[n]));
    vpx_memcpy(t_left[n], xd->left_context, sizeof(t_left[n]));
    r[n][0] = 0;
    d[n] = 0;
    s[n] = 1;
  }

  vp9_subtract_sby_s_c(x->sb_coeff_data.src_diff, src, src_y_stride,
                       dst, dst_y_stride);
  super_block_yrd_32x32(x, &r[TX_32X32][0], &d[TX_32X32], &s[TX_32X32], 1);

#if DEBUG_ERROR
  int err[3] = { 0, 0, 0 };
#endif
  for (n = 0; n < 4; n++) {
    int x_idx = n & 1, y_idx = n >> 1;
    int r_tmp, d_tmp, s_tmp;

    vp9_subtract_mby_s_c(x->src_diff,
                         src + x_idx * 16 + y_idx * 16 * src_y_stride,
                         src_y_stride,
                         dst + x_idx * 16 + y_idx * 16 * dst_y_stride,
                         dst_y_stride);

    xd->above_context = &t_above[TX_16X16][x_idx];
    xd->left_context = &t_left[TX_16X16][y_idx];
    macro_block_yrd_16x16(x, &r_tmp, &d_tmp, &s_tmp, 0);
    d[TX_16X16] += d_tmp;
    r[TX_16X16][0] += r_tmp;
    s[TX_16X16] = s[TX_16X16] && s_tmp;
#if DEBUG_ERROR
    vp9_inverse_transform_mby_16x16(xd);
    err[2] += vp9_block_error_c(xd->diff, x->src_diff, 256);
#endif

    xd->above_context = &t_above[TX_4X4][x_idx];
    xd->left_context = &t_left[TX_4X4][y_idx];
    macro_block_yrd_4x4(x, &r_tmp, &d_tmp, &s_tmp, 0);
    d[TX_4X4] += d_tmp;
    r[TX_4X4][0] += r_tmp;
    s[TX_4X4] = s[TX_4X4] && s_tmp;
#if DEBUG_ERROR
    vp9_inverse_transform_mby_4x4(xd);
    err[0] += vp9_block_error_c(xd->diff, x->src_diff, 256);
#endif

    xd->above_context = &t_above[TX_8X8][x_idx];
    xd->left_context = &t_left[TX_8X8][y_idx];
    macro_block_yrd_8x8(x, &r_tmp, &d_tmp, &s_tmp, 0);
    d[TX_8X8] += d_tmp;
    r[TX_8X8][0] += r_tmp;
    s[TX_8X8] = s[TX_8X8] && s_tmp;
#if DEBUG_ERROR
    vp9_inverse_transform_mby_8x8(xd);
    err[1] += vp9_block_error_c(xd->diff, x->src_diff, 256);
#endif
  }
#if DEBUG_ERROR
  printf("IDCT/FDCT error 16x16: %d (d: %d)\n", err[2], d[2]);
  printf("IDCT/FDCT error 8x8: %d (d: %d)\n", err[1], d[1]);
  printf("IDCT/FDCT error 4x4: %d (d: %d)\n", err[0], d[0]);
#endif
  choose_txfm_size_from_rd(cpi, x, r, rate, d, distortion, s, skip, txfm_cache,
                           TX_SIZE_MAX_SB - 1);

  xd->above_context = orig_above;
  xd->left_context = orig_left;
}

static void super_block_64_yrd(VP9_COMP *cpi,
                               MACROBLOCK *x, int *rate, int *distortion,
                               int *skip,
                               int64_t txfm_cache[NB_TXFM_MODES]) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int r[TX_SIZE_MAX_SB][2], d[TX_SIZE_MAX_SB], s[TX_SIZE_MAX_SB], n;
  const uint8_t *src = x->src.y_buffer, *dst = xd->dst.y_buffer;
  int src_y_stride = x->src.y_stride, dst_y_stride = xd->dst.y_stride;
  ENTROPY_CONTEXT_PLANES t_above[TX_SIZE_MAX_SB][4],
                        *orig_above = xd->above_context;
  ENTROPY_CONTEXT_PLANES t_left[TX_SIZE_MAX_SB][4],
                        *orig_left = xd->left_context;

  for (n = TX_4X4; n < TX_SIZE_MAX_SB; n++) {
    vpx_memcpy(t_above[n], xd->above_context, sizeof(t_above[n]));
    vpx_memcpy(t_left[n], xd->left_context, sizeof(t_left[n]));
    r[n][0] = 0;
    d[n] = 0;
    s[n] = 1;
  }

  for (n = 0; n < 4; n++) {
    int x_idx = n & 1, y_idx = n >> 1;
    int r_tmp, d_tmp, s_tmp;

    xd->above_context = &t_above[TX_32X32][x_idx << 1];
    xd->left_context = &t_left[TX_32X32][y_idx << 1];
    vp9_subtract_sby_s_c(x->sb_coeff_data.src_diff,
                         src + 32 * x_idx + 32 * y_idx * src_y_stride,
                         src_y_stride,
                         dst + 32 * x_idx + 32 * y_idx * dst_y_stride,
                         dst_y_stride);
    super_block_yrd_32x32(x, &r_tmp, &d_tmp, &s_tmp, 0);
    r[TX_32X32][0] += r_tmp;
    d[TX_32X32] += d_tmp;
    s[TX_32X32] = s[TX_32X32] && s_tmp;
  }

#if DEBUG_ERROR
  int err[3] = { 0, 0, 0 };
#endif
  for (n = 0; n < 16; n++) {
    int x_idx = n & 3, y_idx = n >> 2;
    int r_tmp, d_tmp, s_tmp;

    vp9_subtract_mby_s_c(x->src_diff,
                         src + x_idx * 16 + y_idx * 16 * src_y_stride,
                         src_y_stride,
                         dst + x_idx * 16 + y_idx * 16 * dst_y_stride,
                         dst_y_stride);

    xd->above_context = &t_above[TX_16X16][x_idx];
    xd->left_context = &t_left[TX_16X16][y_idx];
    macro_block_yrd_16x16(x, &r_tmp, &d_tmp, &s_tmp, 0);
    d[TX_16X16] += d_tmp;
    r[TX_16X16][0] += r_tmp;
    s[TX_16X16] = s[TX_16X16] && s_tmp;
#if DEBUG_ERROR
    vp9_inverse_transform_mby_16x16(xd);
    err[2] += vp9_block_error_c(xd->diff, x->src_diff, 256);
#endif

    xd->above_context = &t_above[TX_4X4][x_idx];
    xd->left_context = &t_left[TX_4X4][y_idx];
    macro_block_yrd_4x4(x, &r_tmp, &d_tmp, &s_tmp, 0);
    d[TX_4X4] += d_tmp;
    r[TX_4X4][0] += r_tmp;
    s[TX_4X4] = s[TX_4X4] && s_tmp;
#if DEBUG_ERROR
    vp9_inverse_transform_mby_4x4(xd);
    err[0] += vp9_block_error_c(xd->diff, x->src_diff, 256);
#endif

    xd->above_context = &t_above[TX_8X8][x_idx];
    xd->left_context = &t_left[TX_8X8][y_idx];
    macro_block_yrd_8x8(x, &r_tmp, &d_tmp, &s_tmp, 0);
    d[TX_8X8] += d_tmp;
    r[TX_8X8][0] += r_tmp;
    s[TX_8X8] = s[TX_8X8] && s_tmp;
#if DEBUG_ERROR
    vp9_inverse_transform_mby_8x8(xd);
    err[1] += vp9_block_error_c(xd->diff, x->src_diff, 256);
#endif
  }
#if DEBUG_ERROR
  printf("IDCT/FDCT error 16x16: %d (d: %d)\n", err[2], d[2]);
  printf("IDCT/FDCT error 8x8: %d (d: %d)\n", err[1], d[1]);
  printf("IDCT/FDCT error 4x4: %d (d: %d)\n", err[0], d[0]);
#endif
  choose_txfm_size_from_rd(cpi, x, r, rate, d, distortion, s, skip, txfm_cache,
                           TX_SIZE_MAX_SB - 1);

  xd->above_context = orig_above;
  xd->left_context = orig_left;
}

static void copy_predictor_8x8(uint8_t *dst, const uint8_t *predictor) {
  const unsigned int *p = (const unsigned int *)predictor;
  unsigned int *d = (unsigned int *)dst;
  d[0] = p[0];
  d[1] = p[1];
  d[4] = p[4];
  d[5] = p[5];
  d[8] = p[8];
  d[9] = p[9];
  d[12] = p[12];
  d[13] = p[13];
  d[16] = p[16];
  d[17] = p[17];
  d[20] = p[20];
  d[21] = p[21];
  d[24] = p[24];
  d[25] = p[25];
  d[28] = p[28];
  d[29] = p[29];
}

static int64_t rd_pick_intra4x4block(VP9_COMP *cpi, MACROBLOCK *x, BLOCK *be,
                                     BLOCKD *b, B_PREDICTION_MODE *best_mode,
                                     int *bmode_costs,
                                     ENTROPY_CONTEXT *a, ENTROPY_CONTEXT *l,
                                     int *bestrate, int *bestratey,
                                     int *bestdistortion) {
  B_PREDICTION_MODE mode;
  MACROBLOCKD *xd = &x->e_mbd;
  int64_t best_rd = INT64_MAX;
  int rate = 0;
  int distortion;

  ENTROPY_CONTEXT ta = *a, tempa = *a;
  ENTROPY_CONTEXT tl = *l, templ = *l;
  TX_TYPE tx_type = DCT_DCT;
  TX_TYPE best_tx_type = DCT_DCT;
  /*
   * The predictor buffer is a 2d buffer with a stride of 16.  Create
   * a temp buffer that meets the stride requirements, but we are only
   * interested in the left 4x4 block
   * */
  DECLARE_ALIGNED_ARRAY(16, uint8_t, best_predictor, 16 * 4);
  DECLARE_ALIGNED_ARRAY(16, int16_t, best_dqcoeff, 16);

#if CONFIG_NEWBINTRAMODES
  b->bmi.as_mode.context = vp9_find_bpred_context(b);
#endif
  for (mode = B_DC_PRED; mode < LEFT4X4; mode++) {
    int64_t this_rd;
    int ratey;

#if CONFIG_NEWBINTRAMODES
    if (xd->frame_type == KEY_FRAME) {
      if (mode == B_CONTEXT_PRED) continue;
    } else {
      if (mode >= B_CONTEXT_PRED - CONTEXT_PRED_REPLACEMENTS &&
          mode < B_CONTEXT_PRED)
        continue;
    }
#endif

    b->bmi.as_mode.first = mode;
#if CONFIG_NEWBINTRAMODES
    rate = bmode_costs[
        mode == B_CONTEXT_PRED ? mode - CONTEXT_PRED_REPLACEMENTS : mode];
#else
    rate = bmode_costs[mode];
#endif

    vp9_intra4x4_predict(b, mode, b->predictor);
    vp9_subtract_b(be, b, 16);

    b->bmi.as_mode.first = mode;
    tx_type = get_tx_type_4x4(xd, b);
    if (tx_type != DCT_DCT) {
      vp9_fht(be->src_diff, 32, be->coeff, tx_type, 4);
      vp9_ht_quantize_b_4x4(be, b, tx_type);
    } else {
      x->vp9_short_fdct4x4(be->src_diff, be->coeff, 32);
      x->quantize_b_4x4(be, b);
    }

    tempa = ta;
    templ = tl;

    ratey = cost_coeffs(x, b, PLANE_TYPE_Y_WITH_DC, &tempa, &templ, TX_4X4);
    rate += ratey;
    distortion = vp9_block_error(be->coeff, b->dqcoeff, 16) >> 2;

    this_rd = RDCOST(x->rdmult, x->rddiv, rate, distortion);

    if (this_rd < best_rd) {
      *bestrate = rate;
      *bestratey = ratey;
      *bestdistortion = distortion;
      best_rd = this_rd;
      *best_mode = mode;
      best_tx_type = tx_type;
      *a = tempa;
      *l = templ;
      copy_predictor(best_predictor, b->predictor);
      vpx_memcpy(best_dqcoeff, b->dqcoeff, 32);
    }
  }
  b->bmi.as_mode.first = (B_PREDICTION_MODE)(*best_mode);

  // inverse transform
  if (best_tx_type != DCT_DCT)
    vp9_ihtllm(best_dqcoeff, b->diff, 32, best_tx_type, 4, b->eob);
  else
    xd->inv_xform4x4_x8(best_dqcoeff, b->diff, 32);

  vp9_recon_b(best_predictor, b->diff, *(b->base_dst) + b->dst, b->dst_stride);

  return best_rd;
}

static int64_t rd_pick_intra4x4mby_modes(VP9_COMP *cpi, MACROBLOCK *mb,
                                         int *Rate, int *rate_y,
                                         int *Distortion, int64_t best_rd,
                                         int update_contexts) {
  int i;
  MACROBLOCKD *const xd = &mb->e_mbd;
  int cost = mb->mbmode_cost [xd->frame_type] [B_PRED];
  int distortion = 0;
  int tot_rate_y = 0;
  int64_t total_rd = 0;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta, *tl;
  int *bmode_costs;

  if (update_contexts) {
    ta = (ENTROPY_CONTEXT *)xd->above_context;
    tl = (ENTROPY_CONTEXT *)xd->left_context;
  } else {
    vpx_memcpy(&t_above, xd->above_context,
               sizeof(ENTROPY_CONTEXT_PLANES));
    vpx_memcpy(&t_left, xd->left_context,
               sizeof(ENTROPY_CONTEXT_PLANES));

    ta = (ENTROPY_CONTEXT *)&t_above;
    tl = (ENTROPY_CONTEXT *)&t_left;
  }

  xd->mode_info_context->mbmi.mode = B_PRED;
  bmode_costs = mb->inter_bmode_costs;

  for (i = 0; i < 16; i++) {
    MODE_INFO *const mic = xd->mode_info_context;
    const int mis = xd->mode_info_stride;
    B_PREDICTION_MODE UNINITIALIZED_IS_SAFE(best_mode);
    int UNINITIALIZED_IS_SAFE(r), UNINITIALIZED_IS_SAFE(ry), UNINITIALIZED_IS_SAFE(d);

    if (xd->frame_type == KEY_FRAME) {
      const B_PREDICTION_MODE A = above_block_mode(mic, i, mis);
      const B_PREDICTION_MODE L = left_block_mode(mic, i);

      bmode_costs  = mb->bmode_costs[A][L];
    }
#if CONFIG_NEWBINTRAMODES
    mic->bmi[i].as_mode.context = vp9_find_bpred_context(xd->block + i);
#endif

    total_rd += rd_pick_intra4x4block(
                  cpi, mb, mb->block + i, xd->block + i, &best_mode,
                  bmode_costs, ta + vp9_block2above[TX_4X4][i],
                  tl + vp9_block2left[TX_4X4][i], &r, &ry, &d);

    cost += r;
    distortion += d;
    tot_rate_y += ry;

    mic->bmi[i].as_mode.first = best_mode;

#if 0  // CONFIG_NEWBINTRAMODES
    printf("%d %d\n", mic->bmi[i].as_mode.first, mic->bmi[i].as_mode.context);
#endif

    if (total_rd >= best_rd)
      break;
  }

  if (total_rd >= best_rd)
    return INT64_MAX;

  *Rate = cost;
  *rate_y = tot_rate_y;
  *Distortion = distortion;

  return RDCOST(mb->rdmult, mb->rddiv, cost, distortion);
}

static int64_t rd_pick_intra_sby_mode(VP9_COMP *cpi,
                                      MACROBLOCK *x,
                                      int *rate,
                                      int *rate_tokenonly,
                                      int *distortion,
                                      int *skippable,
                                      int64_t txfm_cache[NB_TXFM_MODES]) {
  MB_PREDICTION_MODE mode;
  MB_PREDICTION_MODE UNINITIALIZED_IS_SAFE(mode_selected);
  int this_rate, this_rate_tokenonly;
  int this_distortion, s;
  int64_t best_rd = INT64_MAX, this_rd;

  /* Y Search for 32x32 intra prediction mode */
  for (mode = DC_PRED; mode <= TM_PRED; mode++) {
    x->e_mbd.mode_info_context->mbmi.mode = mode;
    vp9_build_intra_predictors_sby_s(&x->e_mbd);

    super_block_yrd(cpi, x, &this_rate_tokenonly,
                    &this_distortion, &s, txfm_cache);
    this_rate = this_rate_tokenonly +
                x->mbmode_cost[x->e_mbd.frame_type]
                              [x->e_mbd.mode_info_context->mbmi.mode];
    this_rd = RDCOST(x->rdmult, x->rddiv, this_rate, this_distortion);

    if (this_rd < best_rd) {
      mode_selected   = mode;
      best_rd         = this_rd;
      *rate           = this_rate;
      *rate_tokenonly = this_rate_tokenonly;
      *distortion     = this_distortion;
      *skippable      = s;
    }
  }

  x->e_mbd.mode_info_context->mbmi.mode = mode_selected;

  return best_rd;
}

static int64_t rd_pick_intra_sb64y_mode(VP9_COMP *cpi,
                                        MACROBLOCK *x,
                                        int *rate,
                                        int *rate_tokenonly,
                                        int *distortion,
                                        int *skippable,
                                        int64_t txfm_cache[NB_TXFM_MODES]) {
  MB_PREDICTION_MODE mode;
  MB_PREDICTION_MODE UNINITIALIZED_IS_SAFE(mode_selected);
  int this_rate, this_rate_tokenonly;
  int this_distortion, s;
  int64_t best_rd = INT64_MAX, this_rd;

  /* Y Search for 32x32 intra prediction mode */
  for (mode = DC_PRED; mode <= TM_PRED; mode++) {
    x->e_mbd.mode_info_context->mbmi.mode = mode;
    vp9_build_intra_predictors_sb64y_s(&x->e_mbd);

    super_block_64_yrd(cpi, x, &this_rate_tokenonly,
                       &this_distortion, &s, txfm_cache);
    this_rate = this_rate_tokenonly +
                x->mbmode_cost[x->e_mbd.frame_type]
                              [x->e_mbd.mode_info_context->mbmi.mode];
    this_rd = RDCOST(x->rdmult, x->rddiv, this_rate, this_distortion);

    if (this_rd < best_rd) {
      mode_selected   = mode;
      best_rd         = this_rd;
      *rate           = this_rate;
      *rate_tokenonly = this_rate_tokenonly;
      *distortion     = this_distortion;
      *skippable      = s;
    }
  }

  x->e_mbd.mode_info_context->mbmi.mode = mode_selected;

  return best_rd;
}

static int64_t rd_pick_intra16x16mby_mode(VP9_COMP *cpi,
                                          MACROBLOCK *x,
                                          int *Rate,
                                          int *rate_y,
                                          int *Distortion,
                                          int *skippable,
                                          int64_t txfm_cache[NB_TXFM_MODES]) {
  MB_PREDICTION_MODE mode;
  TX_SIZE txfm_size = 0;
  MB_PREDICTION_MODE UNINITIALIZED_IS_SAFE(mode_selected);
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = &xd->mode_info_context->mbmi;
  int rate, ratey;
  int distortion, skip;
  int64_t best_rd = INT64_MAX;
  int64_t this_rd;

  int i;
  for (i = 0; i < NB_TXFM_MODES; i++)
    txfm_cache[i] = INT64_MAX;

  // Y Search for 16x16 intra prediction mode
  for (mode = DC_PRED; mode <= TM_PRED; mode++) {
    int64_t local_txfm_cache[NB_TXFM_MODES];

    mbmi->mode = mode;

    vp9_build_intra_predictors_mby(xd);

    macro_block_yrd(cpi, x, &ratey, &distortion, &skip, local_txfm_cache);

    // FIXME add compoundmode cost
    // FIXME add rate for mode2
    rate = ratey + x->mbmode_cost[xd->frame_type][mbmi->mode];

    this_rd = RDCOST(x->rdmult, x->rddiv, rate, distortion);

    if (this_rd < best_rd) {
      mode_selected = mode;
      txfm_size = mbmi->txfm_size;
      best_rd = this_rd;
      *Rate = rate;
      *rate_y = ratey;
      *Distortion = distortion;
      *skippable = skip;
    }

    for (i = 0; i < NB_TXFM_MODES; i++) {
      int64_t adj_rd = this_rd + local_txfm_cache[i] -
                        local_txfm_cache[cpi->common.txfm_mode];
      if (adj_rd < txfm_cache[i]) {
        txfm_cache[i] = adj_rd;
      }
    }
  }

  mbmi->txfm_size = txfm_size;
  mbmi->mode = mode_selected;

  return best_rd;
}


static int64_t rd_pick_intra8x8block(VP9_COMP *cpi, MACROBLOCK *x, int ib,
                                     B_PREDICTION_MODE *best_mode,
                                     int *mode_costs,
                                     ENTROPY_CONTEXT *a, ENTROPY_CONTEXT *l,
                                     int *bestrate, int *bestratey,
                                     int *bestdistortion) {
  MB_PREDICTION_MODE mode;
  MACROBLOCKD *xd = &x->e_mbd;
  int64_t best_rd = INT64_MAX;
  int distortion = 0, rate = 0;
  BLOCK  *be = x->block + ib;
  BLOCKD *b = xd->block + ib;
  ENTROPY_CONTEXT ta0, ta1, besta0 = 0, besta1 = 0;
  ENTROPY_CONTEXT tl0, tl1, bestl0 = 0, bestl1 = 0;

  /*
   * The predictor buffer is a 2d buffer with a stride of 16.  Create
   * a temp buffer that meets the stride requirements, but we are only
   * interested in the left 8x8 block
   * */
  DECLARE_ALIGNED_ARRAY(16, uint8_t, best_predictor, 16 * 8);
  DECLARE_ALIGNED_ARRAY(16, int16_t, best_dqcoeff, 16 * 4);

  // perform transformation of dimension 8x8
  // note the input and output index mapping
  int idx = (ib & 0x02) ? (ib + 2) : ib;

  for (mode = DC_PRED; mode <= TM_PRED; mode++) {
    int64_t this_rd;
    int rate_t = 0;

    // FIXME rate for compound mode and second intrapred mode
    rate = mode_costs[mode];
    b->bmi.as_mode.first = mode;

    vp9_intra8x8_predict(b, mode, b->predictor);

    vp9_subtract_4b_c(be, b, 16);

    assert(get_2nd_order_usage(xd) == 0);
    if (xd->mode_info_context->mbmi.txfm_size == TX_8X8) {
      TX_TYPE tx_type = get_tx_type_8x8(xd, b);
      if (tx_type != DCT_DCT)
        vp9_fht(be->src_diff, 32, (x->block + idx)->coeff, tx_type, 8);
      else
        x->vp9_short_fdct8x8(be->src_diff, (x->block + idx)->coeff, 32);
      x->quantize_b_8x8(x->block + idx, xd->block + idx);

      // compute quantization mse of 8x8 block
      distortion = vp9_block_error_c((x->block + idx)->coeff,
                                     (xd->block + idx)->dqcoeff, 64);
      ta0 = a[vp9_block2above[TX_8X8][idx]];
      tl0 = l[vp9_block2left[TX_8X8][idx]];

      rate_t = cost_coeffs(x, xd->block + idx, PLANE_TYPE_Y_WITH_DC,
                           &ta0, &tl0, TX_8X8);

      rate += rate_t;
      ta1 = ta0;
      tl1 = tl0;
    } else {
      static const int iblock[4] = {0, 1, 4, 5};
      TX_TYPE tx_type;
      int i;
      ta0 = a[vp9_block2above[TX_4X4][ib]];
      ta1 = a[vp9_block2above[TX_4X4][ib + 1]];
      tl0 = l[vp9_block2left[TX_4X4][ib]];
      tl1 = l[vp9_block2left[TX_4X4][ib + 4]];
      distortion = 0;
      rate_t = 0;
      for (i = 0; i < 4; ++i) {
        b = &xd->block[ib + iblock[i]];
        be = &x->block[ib + iblock[i]];
        tx_type = get_tx_type_4x4(xd, b);
        if (tx_type != DCT_DCT) {
          vp9_fht_c(be->src_diff, 32, be->coeff, tx_type, 4);
          vp9_ht_quantize_b_4x4(be, b, tx_type);
        } else {
          x->vp9_short_fdct4x4(be->src_diff, be->coeff, 32);
          x->quantize_b_4x4(be, b);
        }
        distortion += vp9_block_error_c(be->coeff, b->dqcoeff, 16);
        rate_t += cost_coeffs(x, b, PLANE_TYPE_Y_WITH_DC,
                              // i&1 ? &ta1 : &ta0, i&2 ? &tl1 : &tl0,
                              &ta0, &tl0,
                              TX_4X4);
      }
      rate += rate_t;
    }

    distortion >>= 2;
    this_rd = RDCOST(x->rdmult, x->rddiv, rate, distortion);
    if (this_rd < best_rd) {
      *bestrate = rate;
      *bestratey = rate_t;
      *bestdistortion = distortion;
      besta0 = ta0;
      besta1 = ta1;
      bestl0 = tl0;
      bestl1 = tl1;
      best_rd = this_rd;
      *best_mode = mode;
      copy_predictor_8x8(best_predictor, b->predictor);
      vpx_memcpy(best_dqcoeff, b->dqcoeff, 64);
      vpx_memcpy(best_dqcoeff + 32, b->dqcoeff + 64, 64);
    }
  }
  b->bmi.as_mode.first = (*best_mode);
  vp9_encode_intra8x8(x, ib);

  if (xd->mode_info_context->mbmi.txfm_size == TX_8X8) {
    a[vp9_block2above[TX_8X8][idx]]     = besta0;
    a[vp9_block2above[TX_8X8][idx] + 1] = besta1;
    l[vp9_block2left[TX_8X8][idx]]      = bestl0;
    l[vp9_block2left[TX_8X8][idx] + 1]  = bestl1;
  } else {
    a[vp9_block2above[TX_4X4][ib]]     = besta0;
    a[vp9_block2above[TX_4X4][ib + 1]] = besta1;
    l[vp9_block2left[TX_4X4][ib]]      = bestl0;
    l[vp9_block2left[TX_4X4][ib + 4]]  = bestl1;
  }

  return best_rd;
}

static int64_t rd_pick_intra8x8mby_modes(VP9_COMP *cpi, MACROBLOCK *mb,
                                         int *Rate, int *rate_y,
                                         int *Distortion, int64_t best_rd) {
  MACROBLOCKD *const xd = &mb->e_mbd;
  int i, ib;
  int cost = mb->mbmode_cost [xd->frame_type] [I8X8_PRED];
  int distortion = 0;
  int tot_rate_y = 0;
  int64_t total_rd = 0;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta, *tl;
  int *i8x8mode_costs;

  vpx_memcpy(&t_above, xd->above_context, sizeof(ENTROPY_CONTEXT_PLANES));
  vpx_memcpy(&t_left, xd->left_context, sizeof(ENTROPY_CONTEXT_PLANES));

  ta = (ENTROPY_CONTEXT *)&t_above;
  tl = (ENTROPY_CONTEXT *)&t_left;

  xd->mode_info_context->mbmi.mode = I8X8_PRED;
  i8x8mode_costs  = mb->i8x8_mode_costs;

  for (i = 0; i < 4; i++) {
    MODE_INFO *const mic = xd->mode_info_context;
    B_PREDICTION_MODE UNINITIALIZED_IS_SAFE(best_mode);
    int UNINITIALIZED_IS_SAFE(r), UNINITIALIZED_IS_SAFE(ry), UNINITIALIZED_IS_SAFE(d);

    ib = vp9_i8x8_block[i];
    total_rd += rd_pick_intra8x8block(
                  cpi, mb, ib, &best_mode,
                  i8x8mode_costs, ta, tl, &r, &ry, &d);
    cost += r;
    distortion += d;
    tot_rate_y += ry;
    mic->bmi[ib].as_mode.first = best_mode;
  }

  *Rate = cost;
  *rate_y = tot_rate_y;
  *Distortion = distortion;
  return RDCOST(mb->rdmult, mb->rddiv, cost, distortion);
}

static int rd_cost_mbuv_4x4(MACROBLOCK *mb, int backup) {
  int b;
  int cost = 0;
  MACROBLOCKD *xd = &mb->e_mbd;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta, *tl;

  if (backup) {
    vpx_memcpy(&t_above, xd->above_context, sizeof(ENTROPY_CONTEXT_PLANES));
    vpx_memcpy(&t_left, xd->left_context, sizeof(ENTROPY_CONTEXT_PLANES));

    ta = (ENTROPY_CONTEXT *)&t_above;
    tl = (ENTROPY_CONTEXT *)&t_left;
  } else {
    ta = (ENTROPY_CONTEXT *)xd->above_context;
    tl = (ENTROPY_CONTEXT *)xd->left_context;
  }

  for (b = 16; b < 24; b++)
    cost += cost_coeffs(mb, xd->block + b, PLANE_TYPE_UV,
                        ta + vp9_block2above[TX_4X4][b],
                        tl + vp9_block2left[TX_4X4][b],
                        TX_4X4);

  return cost;
}


static int64_t rd_inter16x16_uv_4x4(VP9_COMP *cpi, MACROBLOCK *x, int *rate,
                                    int *distortion, int fullpixel, int *skip,
                                    int do_ctx_backup) {
  vp9_transform_mbuv_4x4(x);
  vp9_quantize_mbuv_4x4(x);

  *rate       = rd_cost_mbuv_4x4(x, do_ctx_backup);
  *distortion = vp9_mbuverror(x) / 4;
  *skip       = vp9_mbuv_is_skippable_4x4(&x->e_mbd);

  return RDCOST(x->rdmult, x->rddiv, *rate, *distortion);
}

static int rd_cost_mbuv_8x8(MACROBLOCK *mb, int backup) {
  int b;
  int cost = 0;
  MACROBLOCKD *xd = &mb->e_mbd;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta, *tl;

  if (backup) {
    vpx_memcpy(&t_above, xd->above_context, sizeof(ENTROPY_CONTEXT_PLANES));
    vpx_memcpy(&t_left, xd->left_context, sizeof(ENTROPY_CONTEXT_PLANES));

    ta = (ENTROPY_CONTEXT *)&t_above;
    tl = (ENTROPY_CONTEXT *)&t_left;
  } else {
    ta = (ENTROPY_CONTEXT *)mb->e_mbd.above_context;
    tl = (ENTROPY_CONTEXT *)mb->e_mbd.left_context;
  }

  for (b = 16; b < 24; b += 4)
    cost += cost_coeffs(mb, xd->block + b, PLANE_TYPE_UV,
                        ta + vp9_block2above[TX_8X8][b],
                        tl + vp9_block2left[TX_8X8][b], TX_8X8);

  return cost;
}

static int64_t rd_inter16x16_uv_8x8(VP9_COMP *cpi, MACROBLOCK *x, int *rate,
                                    int *distortion, int fullpixel, int *skip,
                                    int do_ctx_backup) {
  vp9_transform_mbuv_8x8(x);
  vp9_quantize_mbuv_8x8(x);

  *rate       = rd_cost_mbuv_8x8(x, do_ctx_backup);
  *distortion = vp9_mbuverror(x) / 4;
  *skip       = vp9_mbuv_is_skippable_8x8(&x->e_mbd);

  return RDCOST(x->rdmult, x->rddiv, *rate, *distortion);
}

static int rd_cost_sbuv_16x16(MACROBLOCK *x, int backup) {
  int b;
  int cost = 0;
  MACROBLOCKD *const xd = &x->e_mbd;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta, *tl;

  if (backup) {
    vpx_memcpy(&t_above, xd->above_context, sizeof(ENTROPY_CONTEXT_PLANES));
    vpx_memcpy(&t_left, xd->left_context, sizeof(ENTROPY_CONTEXT_PLANES));

    ta = (ENTROPY_CONTEXT *) &t_above;
    tl = (ENTROPY_CONTEXT *) &t_left;
  } else {
    ta = (ENTROPY_CONTEXT *)xd->above_context;
    tl = (ENTROPY_CONTEXT *)xd->left_context;
  }

  for (b = 16; b < 24; b += 4)
    cost += cost_coeffs(x, xd->block + b, PLANE_TYPE_UV,
                        ta + vp9_block2above[TX_8X8][b],
                        tl + vp9_block2left[TX_8X8][b], TX_16X16);

  return cost;
}

static void rd_inter32x32_uv_16x16(MACROBLOCK *x, int *rate,
                                   int *distortion, int *skip,
                                   int backup) {
  MACROBLOCKD *const xd = &x->e_mbd;

  vp9_transform_sbuv_16x16(x);
  vp9_quantize_sbuv_16x16(x);

  *rate       = rd_cost_sbuv_16x16(x, backup);
  *distortion = vp9_block_error_c(x->sb_coeff_data.coeff + 1024,
                                   xd->sb_coeff_data.dqcoeff + 1024, 512) >> 2;
  *skip       = vp9_sbuv_is_skippable_16x16(xd);
}

static int64_t rd_inter32x32_uv(VP9_COMP *cpi, MACROBLOCK *x, int *rate,
                                int *distortion, int fullpixel, int *skip) {
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = &xd->mode_info_context->mbmi;
  const uint8_t *usrc = x->src.u_buffer, *udst = xd->dst.u_buffer;
  const uint8_t *vsrc = x->src.v_buffer, *vdst = xd->dst.v_buffer;
  int src_uv_stride = x->src.uv_stride, dst_uv_stride = xd->dst.uv_stride;

  if (mbmi->txfm_size == TX_32X32) {
    vp9_subtract_sbuv_s_c(x->sb_coeff_data.src_diff,
                          usrc, vsrc, src_uv_stride,
                          udst, vdst, dst_uv_stride);
    rd_inter32x32_uv_16x16(x, rate, distortion, skip, 1);
  } else {
    int n, r = 0, d = 0;
    int skippable = 1;
    ENTROPY_CONTEXT_PLANES t_above[2], t_left[2];
    ENTROPY_CONTEXT_PLANES *ta = xd->above_context;
    ENTROPY_CONTEXT_PLANES *tl = xd->left_context;

    memcpy(t_above, xd->above_context, sizeof(t_above));
    memcpy(t_left, xd->left_context, sizeof(t_left));

    for (n = 0; n < 4; n++) {
      int x_idx = n & 1, y_idx = n >> 1;
      int d_tmp, s_tmp, r_tmp;

      xd->above_context = ta + x_idx;
      xd->left_context = tl + y_idx;
      vp9_subtract_mbuv_s_c(x->src_diff,
                            usrc + x_idx * 8 + y_idx * 8 * src_uv_stride,
                            vsrc + x_idx * 8 + y_idx * 8 * src_uv_stride,
                            src_uv_stride,
                            udst + x_idx * 8 + y_idx * 8 * dst_uv_stride,
                            vdst + x_idx * 8 + y_idx * 8 * dst_uv_stride,
                            dst_uv_stride);

      if (mbmi->txfm_size == TX_4X4) {
        rd_inter16x16_uv_4x4(cpi, x, &r_tmp, &d_tmp, fullpixel, &s_tmp, 0);
      } else {
        rd_inter16x16_uv_8x8(cpi, x, &r_tmp, &d_tmp, fullpixel, &s_tmp, 0);
      }

      r += r_tmp;
      d += d_tmp;
      skippable = skippable && s_tmp;
    }

    *rate = r;
    *distortion = d;
    *skip = skippable;
    xd->left_context = tl;
    xd->above_context = ta;
    memcpy(xd->above_context, t_above, sizeof(t_above));
    memcpy(xd->left_context, t_left, sizeof(t_left));
  }

  return RDCOST(x->rdmult, x->rddiv, *rate, *distortion);
}

static void super_block_64_uvrd(MACROBLOCK *x, int *rate,
                                int *distortion, int *skip);
static int64_t rd_inter64x64_uv(VP9_COMP *cpi, MACROBLOCK *x, int *rate,
                                int *distortion, int fullpixel, int *skip) {
  super_block_64_uvrd(x, rate, distortion, skip);
  return RDCOST(x->rdmult, x->rddiv, *rate, *distortion);
}

static int64_t rd_inter4x4_uv(VP9_COMP *cpi, MACROBLOCK *x, int *rate,
                              int *distortion, int *skip, int fullpixel) {
  vp9_build_inter4x4_predictors_mbuv(&x->e_mbd);
  vp9_subtract_mbuv(x->src_diff, x->src.u_buffer, x->src.v_buffer,
                    x->e_mbd.predictor, x->src.uv_stride);
  return rd_inter16x16_uv_4x4(cpi, x, rate, distortion, fullpixel, skip, 1);
}

static void rd_pick_intra_mbuv_mode(VP9_COMP *cpi,
                                    MACROBLOCK *x,
                                    int *rate,
                                    int *rate_tokenonly,
                                    int *distortion,
                                    int *skippable) {
  MB_PREDICTION_MODE mode;
  MB_PREDICTION_MODE UNINITIALIZED_IS_SAFE(mode_selected);
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO * mbmi = &x->e_mbd.mode_info_context->mbmi;
  int64_t best_rd = INT64_MAX;
  int UNINITIALIZED_IS_SAFE(d), UNINITIALIZED_IS_SAFE(r);
  int rate_to, UNINITIALIZED_IS_SAFE(skip);

  for (mode = DC_PRED; mode <= TM_PRED; mode++) {
    int rate;
    int distortion;
    int64_t this_rd;

    mbmi->uv_mode = mode;
    vp9_build_intra_predictors_mbuv(&x->e_mbd);

    vp9_subtract_mbuv(x->src_diff, x->src.u_buffer, x->src.v_buffer,
                      x->e_mbd.predictor, x->src.uv_stride);
    vp9_transform_mbuv_4x4(x);
    vp9_quantize_mbuv_4x4(x);

    rate_to = rd_cost_mbuv_4x4(x, 1);
    rate = rate_to
           + x->intra_uv_mode_cost[x->e_mbd.frame_type][mbmi->uv_mode];

    distortion = vp9_mbuverror(x) / 4;

    this_rd = RDCOST(x->rdmult, x->rddiv, rate, distortion);

    if (this_rd < best_rd) {
      skip = vp9_mbuv_is_skippable_4x4(xd);
      best_rd = this_rd;
      d = distortion;
      r = rate;
      *rate_tokenonly = rate_to;
      mode_selected = mode;
    }
  }

  *rate = r;
  *distortion = d;
  *skippable = skip;

  mbmi->uv_mode = mode_selected;
}

static void rd_pick_intra_mbuv_mode_8x8(VP9_COMP *cpi,
                                        MACROBLOCK *x,
                                        int *rate,
                                        int *rate_tokenonly,
                                        int *distortion,
                                        int *skippable) {
  MACROBLOCKD *xd = &x->e_mbd;
  MB_PREDICTION_MODE mode;
  MB_PREDICTION_MODE UNINITIALIZED_IS_SAFE(mode_selected);
  MB_MODE_INFO * mbmi = &x->e_mbd.mode_info_context->mbmi;
  int64_t best_rd = INT64_MAX;
  int UNINITIALIZED_IS_SAFE(d), UNINITIALIZED_IS_SAFE(r);
  int rate_to, UNINITIALIZED_IS_SAFE(skip);

  for (mode = DC_PRED; mode <= TM_PRED; mode++) {
    int rate;
    int distortion;
    int64_t this_rd;

    mbmi->uv_mode = mode;
    vp9_build_intra_predictors_mbuv(&x->e_mbd);
    vp9_subtract_mbuv(x->src_diff, x->src.u_buffer, x->src.v_buffer,
                      x->e_mbd.predictor, x->src.uv_stride);
    vp9_transform_mbuv_8x8(x);

    vp9_quantize_mbuv_8x8(x);

    rate_to = rd_cost_mbuv_8x8(x, 1);
    rate = rate_to + x->intra_uv_mode_cost[x->e_mbd.frame_type][mbmi->uv_mode];

    distortion = vp9_mbuverror(x) / 4;
    this_rd = RDCOST(x->rdmult, x->rddiv, rate, distortion);

    if (this_rd < best_rd) {
      skip = vp9_mbuv_is_skippable_8x8(xd);
      best_rd = this_rd;
      d = distortion;
      r = rate;
      *rate_tokenonly = rate_to;
      mode_selected = mode;
    }
  }
  *rate = r;
  *distortion = d;
  *skippable = skip;
  mbmi->uv_mode = mode_selected;
}

// TODO(rbultje) very similar to rd_inter32x32_uv(), merge?
static void super_block_uvrd(MACROBLOCK *x,
                             int *rate,
                             int *distortion,
                             int *skippable) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = &xd->mode_info_context->mbmi;
  const uint8_t *usrc = x->src.u_buffer, *udst = xd->dst.u_buffer;
  const uint8_t *vsrc = x->src.v_buffer, *vdst = xd->dst.v_buffer;
  int src_uv_stride = x->src.uv_stride, dst_uv_stride = xd->dst.uv_stride;

  if (mbmi->txfm_size == TX_32X32) {
    vp9_subtract_sbuv_s_c(x->sb_coeff_data.src_diff,
                          usrc, vsrc, src_uv_stride,
                          udst, vdst, dst_uv_stride);
    rd_inter32x32_uv_16x16(x, rate, distortion, skippable, 1);
  } else {
    int d = 0, r = 0, n, s = 1;
    ENTROPY_CONTEXT_PLANES t_above[2], t_left[2];
    ENTROPY_CONTEXT_PLANES *ta_orig = xd->above_context;
    ENTROPY_CONTEXT_PLANES *tl_orig = xd->left_context;

    memcpy(t_above, xd->above_context, sizeof(t_above));
    memcpy(t_left,  xd->left_context,  sizeof(t_left));

    for (n = 0; n < 4; n++) {
      int x_idx = n & 1, y_idx = n >> 1;

      vp9_subtract_mbuv_s_c(x->src_diff,
                            usrc + x_idx * 8 + y_idx * 8 * src_uv_stride,
                            vsrc + x_idx * 8 + y_idx * 8 * src_uv_stride,
                            src_uv_stride,
                            udst + x_idx * 8 + y_idx * 8 * dst_uv_stride,
                            vdst + x_idx * 8 + y_idx * 8 * dst_uv_stride,
                            dst_uv_stride);
      if (mbmi->txfm_size == TX_4X4) {
        vp9_transform_mbuv_4x4(x);
        vp9_quantize_mbuv_4x4(x);
        s &= vp9_mbuv_is_skippable_4x4(xd);
      } else {
        vp9_transform_mbuv_8x8(x);
        vp9_quantize_mbuv_8x8(x);
        s &= vp9_mbuv_is_skippable_8x8(xd);
      }

      d += vp9_mbuverror(x) >> 2;
      xd->above_context = t_above + x_idx;
      xd->left_context = t_left + y_idx;
      if (mbmi->txfm_size == TX_4X4) {
        r += rd_cost_mbuv_4x4(x, 0);
      } else {
        r += rd_cost_mbuv_8x8(x, 0);
      }
    }

    xd->above_context = ta_orig;
    xd->left_context = tl_orig;

    *distortion = d;
    *rate       = r;
    *skippable  = s;
  }
}

static void super_block_64_uvrd(MACROBLOCK *x,
                                int *rate,
                                int *distortion,
                                int *skippable) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = &xd->mode_info_context->mbmi;
  const uint8_t *usrc = x->src.u_buffer, *udst = xd->dst.u_buffer;
  const uint8_t *vsrc = x->src.v_buffer, *vdst = xd->dst.v_buffer;
  int src_uv_stride = x->src.uv_stride, dst_uv_stride = xd->dst.uv_stride;
  ENTROPY_CONTEXT_PLANES t_above[4], t_left[4];
  ENTROPY_CONTEXT_PLANES *ta_orig = xd->above_context;
  ENTROPY_CONTEXT_PLANES *tl_orig = xd->left_context;
  int d = 0, r = 0, n, s = 1;

  memcpy(t_above, xd->above_context, sizeof(t_above));
  memcpy(t_left,  xd->left_context,  sizeof(t_left));

  if (mbmi->txfm_size == TX_32X32) {
    int n;

    *rate = 0;
    for (n = 0; n < 4; n++) {
      int x_idx = n & 1, y_idx = n >> 1;
      int r_tmp, d_tmp, s_tmp;

      vp9_subtract_sbuv_s_c(x->sb_coeff_data.src_diff,
                            usrc + x_idx * 16 + y_idx * 16 * src_uv_stride,
                            vsrc + x_idx * 16 + y_idx * 16 * src_uv_stride,
                            src_uv_stride,
                            udst + x_idx * 16 + y_idx * 16 * dst_uv_stride,
                            vdst + x_idx * 16 + y_idx * 16 * dst_uv_stride,
                            dst_uv_stride);
      xd->above_context = t_above + x_idx * 2;
      xd->left_context = t_left + y_idx * 2;
      rd_inter32x32_uv_16x16(x, &r_tmp, &d_tmp, &s_tmp, 0);
      r += r_tmp;
      d += d_tmp;
      s = s && s_tmp;
    }
  } else {
    for (n = 0; n < 16; n++) {
      int x_idx = n & 3, y_idx = n >> 2;

      vp9_subtract_mbuv_s_c(x->src_diff,
                            usrc + x_idx * 8 + y_idx * 8 * src_uv_stride,
                            vsrc + x_idx * 8 + y_idx * 8 * src_uv_stride,
                            src_uv_stride,
                            udst + x_idx * 8 + y_idx * 8 * dst_uv_stride,
                            vdst + x_idx * 8 + y_idx * 8 * dst_uv_stride,
                            dst_uv_stride);
      if (mbmi->txfm_size == TX_4X4) {
        vp9_transform_mbuv_4x4(x);
        vp9_quantize_mbuv_4x4(x);
        s &= vp9_mbuv_is_skippable_4x4(xd);
      } else {
        vp9_transform_mbuv_8x8(x);
        vp9_quantize_mbuv_8x8(x);
        s &= vp9_mbuv_is_skippable_8x8(xd);
      }

      xd->above_context = t_above + x_idx;
      xd->left_context = t_left + y_idx;
      d += vp9_mbuverror(x) >> 2;
      if (mbmi->txfm_size == TX_4X4) {
        r += rd_cost_mbuv_4x4(x, 0);
      } else {
        r += rd_cost_mbuv_8x8(x, 0);
      }
    }
  }

  *distortion = d;
  *rate       = r;
  *skippable  = s;

  xd->left_context = tl_orig;
  xd->above_context = ta_orig;
}

static int64_t rd_pick_intra_sbuv_mode(VP9_COMP *cpi,
                                       MACROBLOCK *x,
                                       int *rate,
                                       int *rate_tokenonly,
                                       int *distortion,
                                       int *skippable) {
  MB_PREDICTION_MODE mode;
  MB_PREDICTION_MODE UNINITIALIZED_IS_SAFE(mode_selected);
  int64_t best_rd = INT64_MAX, this_rd;
  int this_rate_tokenonly, this_rate;
  int this_distortion, s;

  for (mode = DC_PRED; mode <= TM_PRED; mode++) {
    x->e_mbd.mode_info_context->mbmi.uv_mode = mode;
    vp9_build_intra_predictors_sbuv_s(&x->e_mbd);

    super_block_uvrd(x, &this_rate_tokenonly,
                     &this_distortion, &s);
    this_rate = this_rate_tokenonly +
                x->intra_uv_mode_cost[x->e_mbd.frame_type][mode];
    this_rd = RDCOST(x->rdmult, x->rddiv, this_rate, this_distortion);

    if (this_rd < best_rd) {
      mode_selected   = mode;
      best_rd         = this_rd;
      *rate           = this_rate;
      *rate_tokenonly = this_rate_tokenonly;
      *distortion     = this_distortion;
      *skippable      = s;
    }
  }

  x->e_mbd.mode_info_context->mbmi.uv_mode = mode_selected;

  return best_rd;
}

static int64_t rd_pick_intra_sb64uv_mode(VP9_COMP *cpi,
                                         MACROBLOCK *x,
                                         int *rate,
                                         int *rate_tokenonly,
                                         int *distortion,
                                         int *skippable) {
  MB_PREDICTION_MODE mode;
  MB_PREDICTION_MODE UNINITIALIZED_IS_SAFE(mode_selected);
  int64_t best_rd = INT64_MAX, this_rd;
  int this_rate_tokenonly, this_rate;
  int this_distortion, s;

  for (mode = DC_PRED; mode <= TM_PRED; mode++) {
    x->e_mbd.mode_info_context->mbmi.uv_mode = mode;
    vp9_build_intra_predictors_sb64uv_s(&x->e_mbd);

    super_block_64_uvrd(x, &this_rate_tokenonly,
                        &this_distortion, &s);
    this_rate = this_rate_tokenonly +
    x->intra_uv_mode_cost[x->e_mbd.frame_type][mode];
    this_rd = RDCOST(x->rdmult, x->rddiv, this_rate, this_distortion);

    if (this_rd < best_rd) {
      mode_selected   = mode;
      best_rd         = this_rd;
      *rate           = this_rate;
      *rate_tokenonly = this_rate_tokenonly;
      *distortion     = this_distortion;
      *skippable      = s;
    }
  }

  x->e_mbd.mode_info_context->mbmi.uv_mode = mode_selected;

  return best_rd;
}

int vp9_cost_mv_ref(VP9_COMP *cpi,
                    MB_PREDICTION_MODE m,
                    const int mode_context) {
  MACROBLOCKD *xd = &cpi->mb.e_mbd;
  int segment_id = xd->mode_info_context->mbmi.segment_id;

  // If the mode coding is done entirely at the segment level
  // we should not account for it at the per mb level in rd code.
  // Note that if the segment level coding is expanded from single mode
  // to multiple mode masks as per reference frame coding we will need
  // to do something different here.
  if (!vp9_segfeature_active(xd, segment_id, SEG_LVL_MODE)) {
    VP9_COMMON *pc = &cpi->common;

    vp9_prob p [VP9_MVREFS - 1];
    assert(NEARESTMV <= m  &&  m <= SPLITMV);
    vp9_mv_ref_probs(pc, p, mode_context);
    return cost_token(vp9_mv_ref_tree, p,
                      vp9_mv_ref_encoding_array - NEARESTMV + m);
  } else
    return 0;
}

void vp9_set_mbmode_and_mvs(MACROBLOCK *x, MB_PREDICTION_MODE mb, int_mv *mv) {
  x->e_mbd.mode_info_context->mbmi.mode = mb;
  x->e_mbd.mode_info_context->mbmi.mv[0].as_int = mv->as_int;
}

static int labels2mode(
  MACROBLOCK *x,
  int const *labelings, int which_label,
  B_PREDICTION_MODE this_mode,
  int_mv *this_mv, int_mv *this_second_mv,
  int_mv seg_mvs[MAX_REF_FRAMES - 1],
  int_mv *best_ref_mv,
  int_mv *second_best_ref_mv,
  int *mvjcost, int *mvcost[2]) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MODE_INFO *const mic = xd->mode_info_context;
  MB_MODE_INFO * mbmi = &mic->mbmi;
  const int mis = xd->mode_info_stride;

  int i, cost = 0, thismvcost = 0;

  /* We have to be careful retrieving previously-encoded motion vectors.
     Ones from this macroblock have to be pulled from the BLOCKD array
     as they have not yet made it to the bmi array in our MB_MODE_INFO. */
  for (i = 0; i < 16; ++i) {
    BLOCKD *const d = xd->block + i;
    const int row = i >> 2,  col = i & 3;

    B_PREDICTION_MODE m;

    if (labelings[i] != which_label)
      continue;

    if (col  &&  labelings[i] == labelings[i - 1])
      m = LEFT4X4;
    else if (row  &&  labelings[i] == labelings[i - 4])
      m = ABOVE4X4;
    else {
      // the only time we should do costing for new motion vector or mode
      // is when we are on a new label  (jbb May 08, 2007)
      switch (m = this_mode) {
        case NEW4X4 :
          if (mbmi->second_ref_frame > 0) {
            this_mv->as_int = seg_mvs[mbmi->ref_frame - 1].as_int;
            this_second_mv->as_int =
              seg_mvs[mbmi->second_ref_frame - 1].as_int;
          }

          thismvcost  = vp9_mv_bit_cost(this_mv, best_ref_mv, mvjcost, mvcost,
                                        102, xd->allow_high_precision_mv);
          if (mbmi->second_ref_frame > 0) {
            thismvcost += vp9_mv_bit_cost(this_second_mv, second_best_ref_mv,
                                          mvjcost, mvcost, 102,
                                          xd->allow_high_precision_mv);
          }
          break;
        case LEFT4X4:
          this_mv->as_int = col ? d[-1].bmi.as_mv.first.as_int : left_block_mv(mic, i);
          if (mbmi->second_ref_frame > 0)
            this_second_mv->as_int = col ? d[-1].bmi.as_mv.second.as_int : left_block_second_mv(mic, i);
          break;
        case ABOVE4X4:
          this_mv->as_int = row ? d[-4].bmi.as_mv.first.as_int : above_block_mv(mic, i, mis);
          if (mbmi->second_ref_frame > 0)
            this_second_mv->as_int = row ? d[-4].bmi.as_mv.second.as_int : above_block_second_mv(mic, i, mis);
          break;
        case ZERO4X4:
          this_mv->as_int = 0;
          if (mbmi->second_ref_frame > 0)
            this_second_mv->as_int = 0;
          break;
        default:
          break;
      }

      if (m == ABOVE4X4) { // replace above with left if same
        int_mv left_mv, left_second_mv;

        left_second_mv.as_int = 0;
        left_mv.as_int = col ? d[-1].bmi.as_mv.first.as_int :
                         left_block_mv(mic, i);
        if (mbmi->second_ref_frame > 0)
          left_second_mv.as_int = col ? d[-1].bmi.as_mv.second.as_int :
                                  left_block_second_mv(mic, i);

        if (left_mv.as_int == this_mv->as_int &&
            (mbmi->second_ref_frame <= 0 ||
             left_second_mv.as_int == this_second_mv->as_int))
          m = LEFT4X4;
      }

#if CONFIG_NEWBINTRAMODES
      cost = x->inter_bmode_costs[
          m == B_CONTEXT_PRED ? m - CONTEXT_PRED_REPLACEMENTS : m];
#else
      cost = x->inter_bmode_costs[m];
#endif
    }

    d->bmi.as_mv.first.as_int = this_mv->as_int;
    if (mbmi->second_ref_frame > 0)
      d->bmi.as_mv.second.as_int = this_second_mv->as_int;

    x->partition_info->bmi[i].mode = m;
    x->partition_info->bmi[i].mv.as_int = this_mv->as_int;
    if (mbmi->second_ref_frame > 0)
      x->partition_info->bmi[i].second_mv.as_int = this_second_mv->as_int;
  }

  cost += thismvcost;
  return cost;
}

static int64_t encode_inter_mb_segment(MACROBLOCK *x,
                                       int const *labels,
                                       int which_label,
                                       int *labelyrate,
                                       int *distortion,
                                       ENTROPY_CONTEXT *ta,
                                       ENTROPY_CONTEXT *tl) {
  int i;
  MACROBLOCKD *xd = &x->e_mbd;

  *labelyrate = 0;
  *distortion = 0;
  for (i = 0; i < 16; i++) {
    if (labels[i] == which_label) {
      BLOCKD *bd = &x->e_mbd.block[i];
      BLOCK *be = &x->block[i];
      int thisdistortion;

      vp9_build_inter_predictors_b(bd, 16, xd->subpixel_predict4x4);
      if (xd->mode_info_context->mbmi.second_ref_frame > 0)
        vp9_build_2nd_inter_predictors_b(bd, 16, xd->subpixel_predict_avg4x4);
      vp9_subtract_b(be, bd, 16);
      x->vp9_short_fdct4x4(be->src_diff, be->coeff, 32);
      x->quantize_b_4x4(be, bd);
      thisdistortion = vp9_block_error(be->coeff, bd->dqcoeff, 16);
      *distortion += thisdistortion;
      *labelyrate += cost_coeffs(x, bd, PLANE_TYPE_Y_WITH_DC,
                                 ta + vp9_block2above[TX_4X4][i],
                                 tl + vp9_block2left[TX_4X4][i], TX_4X4);
    }
  }
  *distortion >>= 2;
  return RDCOST(x->rdmult, x->rddiv, *labelyrate, *distortion);
}

static int64_t encode_inter_mb_segment_8x8(MACROBLOCK *x,
                                           int const *labels,
                                           int which_label,
                                           int *labelyrate,
                                           int *distortion,
                                           int64_t *otherrd,
                                           ENTROPY_CONTEXT *ta,
                                           ENTROPY_CONTEXT *tl) {
  int i, j;
  MACROBLOCKD *xd = &x->e_mbd;
  const int iblock[4] = { 0, 1, 4, 5 };
  int othercost = 0, otherdist = 0;
  ENTROPY_CONTEXT_PLANES tac, tlc;
  ENTROPY_CONTEXT *tacp = (ENTROPY_CONTEXT *) &tac,
                  *tlcp = (ENTROPY_CONTEXT *) &tlc;

  if (otherrd) {
    memcpy(&tac, ta, sizeof(ENTROPY_CONTEXT_PLANES));
    memcpy(&tlc, tl, sizeof(ENTROPY_CONTEXT_PLANES));
  }

  *distortion = 0;
  *labelyrate = 0;
  for (i = 0; i < 4; i++) {
    int ib = vp9_i8x8_block[i];

    if (labels[ib] == which_label) {
      int idx = (ib & 8) + ((ib & 2) << 1);
      BLOCKD *bd = &xd->block[ib], *bd2 = &xd->block[idx];
      BLOCK *be = &x->block[ib], *be2 = &x->block[idx];
      int thisdistortion;

      vp9_build_inter_predictors4b(xd, bd, 16);
      if (xd->mode_info_context->mbmi.second_ref_frame > 0)
        vp9_build_2nd_inter_predictors4b(xd, bd, 16);
      vp9_subtract_4b_c(be, bd, 16);

      if (xd->mode_info_context->mbmi.txfm_size == TX_4X4) {
        if (otherrd) {
          x->vp9_short_fdct8x8(be->src_diff, be2->coeff, 32);
          x->quantize_b_8x8(be2, bd2);
          thisdistortion = vp9_block_error_c(be2->coeff, bd2->dqcoeff, 64);
          otherdist += thisdistortion;
          othercost += cost_coeffs(x, bd2, PLANE_TYPE_Y_WITH_DC,
                                     tacp + vp9_block2above[TX_8X8][idx],
                                     tlcp + vp9_block2left[TX_8X8][idx],
                                     TX_8X8);
        }
        for (j = 0; j < 4; j += 2) {
          bd = &xd->block[ib + iblock[j]];
          be = &x->block[ib + iblock[j]];
          x->vp9_short_fdct8x4(be->src_diff, be->coeff, 32);
          x->quantize_b_4x4_pair(be, be + 1, bd, bd + 1);
          thisdistortion = vp9_block_error_c(be->coeff, bd->dqcoeff, 32);
          *distortion += thisdistortion;
          *labelyrate += cost_coeffs(x, bd, PLANE_TYPE_Y_WITH_DC,
                           ta + vp9_block2above[TX_4X4][ib + iblock[j]],
                           tl + vp9_block2left[TX_4X4][ib + iblock[j]],
                           TX_4X4);
          *labelyrate += cost_coeffs(x, bd + 1, PLANE_TYPE_Y_WITH_DC,
                           ta + vp9_block2above[TX_4X4][ib + iblock[j] + 1],
                           tl + vp9_block2left[TX_4X4][ib + iblock[j]],
                           TX_4X4);
        }
      } else /* 8x8 */ {
        if (otherrd) {
          for (j = 0; j < 4; j += 2) {
            BLOCKD *bd = &xd->block[ib + iblock[j]];
            BLOCK *be = &x->block[ib + iblock[j]];
            x->vp9_short_fdct8x4(be->src_diff, be->coeff, 32);
            x->quantize_b_4x4_pair(be, be + 1, bd, bd + 1);
            thisdistortion = vp9_block_error_c(be->coeff, bd->dqcoeff, 32);
            otherdist += thisdistortion;
            othercost += cost_coeffs(x, bd, PLANE_TYPE_Y_WITH_DC,
                           tacp + vp9_block2above[TX_4X4][ib + iblock[j]],
                           tlcp + vp9_block2left[TX_4X4][ib + iblock[j]],
                           TX_4X4);
            othercost += cost_coeffs(x, bd + 1, PLANE_TYPE_Y_WITH_DC,
                           tacp + vp9_block2above[TX_4X4][ib + iblock[j] + 1],
                           tlcp + vp9_block2left[TX_4X4][ib + iblock[j]],
                           TX_4X4);
          }
        }
        x->vp9_short_fdct8x8(be->src_diff, be2->coeff, 32);
        x->quantize_b_8x8(be2, bd2);
        thisdistortion = vp9_block_error_c(be2->coeff, bd2->dqcoeff, 64);
        *distortion += thisdistortion;
        *labelyrate += cost_coeffs(x, bd2, PLANE_TYPE_Y_WITH_DC,
                                   ta + vp9_block2above[TX_8X8][idx],
                                   tl + vp9_block2left[TX_8X8][idx], TX_8X8);
      }
    }
  }
  *distortion >>= 2;
  if (otherrd) {
    otherdist >>= 2;
    *otherrd = RDCOST(x->rdmult, x->rddiv, othercost, otherdist);
  }
  return RDCOST(x->rdmult, x->rddiv, *labelyrate, *distortion);
}

static const unsigned int segmentation_to_sseshift[4] = {3, 3, 2, 0};


typedef struct {
  int_mv *ref_mv, *second_ref_mv;
  int_mv mvp;

  int64_t segment_rd;
  SPLITMV_PARTITIONING_TYPE segment_num;
  TX_SIZE txfm_size;
  int r;
  int d;
  int segment_yrate;
  B_PREDICTION_MODE modes[16];
  int_mv mvs[16], second_mvs[16];
  int eobs[16];

  int mvthresh;
  int *mdcounts;

  int_mv sv_mvp[4];     // save 4 mvp from 8x8
  int sv_istep[2];  // save 2 initial step_param for 16x8/8x16

} BEST_SEG_INFO;

static __inline
int mv_check_bounds(MACROBLOCK *x, int_mv *mv) {
  int r = 0;
  r |= (mv->as_mv.row >> 3) < x->mv_row_min;
  r |= (mv->as_mv.row >> 3) > x->mv_row_max;
  r |= (mv->as_mv.col >> 3) < x->mv_col_min;
  r |= (mv->as_mv.col >> 3) > x->mv_col_max;
  return r;
}

static void rd_check_segment_txsize(VP9_COMP *cpi, MACROBLOCK *x,
                                    BEST_SEG_INFO *bsi,
                                    SPLITMV_PARTITIONING_TYPE segmentation,
                                    TX_SIZE tx_size, int64_t *otherrds,
                                    int64_t *rds, int *completed,
                                    /* 16 = n_blocks */
                                    int_mv seg_mvs[16 /* n_blocks */]
                                                  [MAX_REF_FRAMES - 1]) {
  int i, j;
  int const *labels;
  int br = 0, bd = 0;
  B_PREDICTION_MODE this_mode;
  MB_MODE_INFO * mbmi = &x->e_mbd.mode_info_context->mbmi;

  int label_count;
  int64_t this_segment_rd = 0, other_segment_rd;
  int label_mv_thresh;
  int rate = 0;
  int sbr = 0, sbd = 0;
  int segmentyrate = 0;
  int best_eobs[16] = { 0 };

  vp9_variance_fn_ptr_t *v_fn_ptr;

  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta, *tl;
  ENTROPY_CONTEXT_PLANES t_above_b, t_left_b;
  ENTROPY_CONTEXT *ta_b, *tl_b;

  vpx_memcpy(&t_above, x->e_mbd.above_context, sizeof(ENTROPY_CONTEXT_PLANES));
  vpx_memcpy(&t_left, x->e_mbd.left_context, sizeof(ENTROPY_CONTEXT_PLANES));

  ta = (ENTROPY_CONTEXT *)&t_above;
  tl = (ENTROPY_CONTEXT *)&t_left;
  ta_b = (ENTROPY_CONTEXT *)&t_above_b;
  tl_b = (ENTROPY_CONTEXT *)&t_left_b;

  v_fn_ptr = &cpi->fn_ptr[segmentation];
  labels = vp9_mbsplits[segmentation];
  label_count = vp9_mbsplit_count[segmentation];

  // 64 makes this threshold really big effectively
  // making it so that we very rarely check mvs on
  // segments.   setting this to 1 would make mv thresh
  // roughly equal to what it is for macroblocks
  label_mv_thresh = 1 * bsi->mvthresh / label_count;

  // Segmentation method overheads
  rate = cost_token(vp9_mbsplit_tree, vp9_mbsplit_probs,
                    vp9_mbsplit_encodings + segmentation);
  rate += vp9_cost_mv_ref(cpi, SPLITMV,
                          mbmi->mb_mode_context[mbmi->ref_frame]);
  this_segment_rd += RDCOST(x->rdmult, x->rddiv, rate, 0);
  br += rate;
  other_segment_rd = this_segment_rd;

  mbmi->txfm_size = tx_size;
  for (i = 0; i < label_count && this_segment_rd < bsi->segment_rd; i++) {
    int_mv mode_mv[B_MODE_COUNT], second_mode_mv[B_MODE_COUNT];
    int64_t best_label_rd = INT64_MAX, best_other_rd = INT64_MAX;
    B_PREDICTION_MODE mode_selected = ZERO4X4;
    int bestlabelyrate = 0;

    // search for the best motion vector on this segment
    for (this_mode = LEFT4X4; this_mode <= NEW4X4; this_mode ++) {
      int64_t this_rd, other_rd;
      int distortion;
      int labelyrate;
      ENTROPY_CONTEXT_PLANES t_above_s, t_left_s;
      ENTROPY_CONTEXT *ta_s;
      ENTROPY_CONTEXT *tl_s;

      vpx_memcpy(&t_above_s, &t_above, sizeof(ENTROPY_CONTEXT_PLANES));
      vpx_memcpy(&t_left_s, &t_left, sizeof(ENTROPY_CONTEXT_PLANES));

      ta_s = (ENTROPY_CONTEXT *)&t_above_s;
      tl_s = (ENTROPY_CONTEXT *)&t_left_s;

      // motion search for newmv (single predictor case only)
      if (mbmi->second_ref_frame <= 0 && this_mode == NEW4X4) {
        int sseshift, n;
        int step_param = 0;
        int further_steps;
        int thissme, bestsme = INT_MAX;
        BLOCK *c;
        BLOCKD *e;

        /* Is the best so far sufficiently good that we cant justify doing
         * and new motion search. */
        if (best_label_rd < label_mv_thresh)
          break;

        if (cpi->compressor_speed) {
          if (segmentation == PARTITIONING_8X16 ||
              segmentation == PARTITIONING_16X8) {
            bsi->mvp.as_int = bsi->sv_mvp[i].as_int;
            if (i == 1 && segmentation == PARTITIONING_16X8)
              bsi->mvp.as_int = bsi->sv_mvp[2].as_int;

            step_param = bsi->sv_istep[i];
          }

          // use previous block's result as next block's MV predictor.
          if (segmentation == PARTITIONING_4X4 && i > 0) {
            bsi->mvp.as_int = x->e_mbd.block[i - 1].bmi.as_mv.first.as_int;
            if (i == 4 || i == 8 || i == 12)
              bsi->mvp.as_int = x->e_mbd.block[i - 4].bmi.as_mv.first.as_int;
            step_param = 2;
          }
        }

        further_steps = (MAX_MVSEARCH_STEPS - 1) - step_param;

        {
          int sadpb = x->sadperbit4;
          int_mv mvp_full;

          mvp_full.as_mv.row = bsi->mvp.as_mv.row >> 3;
          mvp_full.as_mv.col = bsi->mvp.as_mv.col >> 3;

          // find first label
          n = vp9_mbsplit_offset[segmentation][i];

          c = &x->block[n];
          e = &x->e_mbd.block[n];

          bestsme = vp9_full_pixel_diamond(cpi, x, c, e, &mvp_full, step_param,
                                           sadpb, further_steps, 0, v_fn_ptr,
                                           bsi->ref_mv, &mode_mv[NEW4X4]);

          sseshift = segmentation_to_sseshift[segmentation];

          // Should we do a full search (best quality only)
          if ((cpi->compressor_speed == 0) && (bestsme >> sseshift) > 4000) {
            /* Check if mvp_full is within the range. */
            clamp_mv(&mvp_full, x->mv_col_min, x->mv_col_max,
                     x->mv_row_min, x->mv_row_max);

            thissme = cpi->full_search_sad(x, c, e, &mvp_full,
                                           sadpb, 16, v_fn_ptr,
                                           x->nmvjointcost, x->mvcost,
                                           bsi->ref_mv);

            if (thissme < bestsme) {
              bestsme = thissme;
              mode_mv[NEW4X4].as_int = e->bmi.as_mv.first.as_int;
            } else {
              /* The full search result is actually worse so re-instate the
               * previous best vector */
              e->bmi.as_mv.first.as_int = mode_mv[NEW4X4].as_int;
            }
          }
        }

        if (bestsme < INT_MAX) {
          int distortion;
          unsigned int sse;
          cpi->find_fractional_mv_step(x, c, e, &mode_mv[NEW4X4],
                                       bsi->ref_mv, x->errorperbit, v_fn_ptr,
                                       x->nmvjointcost, x->mvcost,
                                       &distortion, &sse);

          // safe motion search result for use in compound prediction
          seg_mvs[i][mbmi->ref_frame - 1].as_int = mode_mv[NEW4X4].as_int;
        }
      } else if (mbmi->second_ref_frame > 0 && this_mode == NEW4X4) {
        /* NEW4X4 */
        /* motion search not completed? Then skip newmv for this block with
         * comppred */
        if (seg_mvs[i][mbmi->second_ref_frame - 1].as_int == INVALID_MV ||
            seg_mvs[i][mbmi->ref_frame        - 1].as_int == INVALID_MV) {
          continue;
        }
      }

      rate = labels2mode(x, labels, i, this_mode, &mode_mv[this_mode],
                         &second_mode_mv[this_mode], seg_mvs[i],
                         bsi->ref_mv, bsi->second_ref_mv, x->nmvjointcost,
                         x->mvcost);

      // Trap vectors that reach beyond the UMV borders
      if (((mode_mv[this_mode].as_mv.row >> 3) < x->mv_row_min) ||
          ((mode_mv[this_mode].as_mv.row >> 3) > x->mv_row_max) ||
          ((mode_mv[this_mode].as_mv.col >> 3) < x->mv_col_min) ||
          ((mode_mv[this_mode].as_mv.col >> 3) > x->mv_col_max)) {
        continue;
      }
      if (mbmi->second_ref_frame > 0 &&
          mv_check_bounds(x, &second_mode_mv[this_mode]))
        continue;

      if (segmentation == PARTITIONING_4X4) {
        this_rd = encode_inter_mb_segment(x, labels, i, &labelyrate,
                                          &distortion, ta_s, tl_s);
        other_rd = this_rd;
      } else {
        this_rd = encode_inter_mb_segment_8x8(x, labels, i, &labelyrate,
                                              &distortion, &other_rd,
                                              ta_s, tl_s);
      }
      this_rd += RDCOST(x->rdmult, x->rddiv, rate, 0);
      rate += labelyrate;

      if (this_rd < best_label_rd) {
        sbr = rate;
        sbd = distortion;
        bestlabelyrate = labelyrate;
        mode_selected = this_mode;
        best_label_rd = this_rd;
        if (x->e_mbd.mode_info_context->mbmi.txfm_size == TX_4X4) {
          for (j = 0; j < 16; j++)
            if (labels[j] == i)
              best_eobs[j] = x->e_mbd.block[j].eob;
        } else {
          for (j = 0; j < 4; j++) {
            int ib = vp9_i8x8_block[j], idx = j * 4;

            if (labels[ib] == i)
              best_eobs[idx] = x->e_mbd.block[idx].eob;
          }
        }
        if (other_rd < best_other_rd)
          best_other_rd = other_rd;

        vpx_memcpy(ta_b, ta_s, sizeof(ENTROPY_CONTEXT_PLANES));
        vpx_memcpy(tl_b, tl_s, sizeof(ENTROPY_CONTEXT_PLANES));

      }
    } /*for each 4x4 mode*/

    vpx_memcpy(ta, ta_b, sizeof(ENTROPY_CONTEXT_PLANES));
    vpx_memcpy(tl, tl_b, sizeof(ENTROPY_CONTEXT_PLANES));

    labels2mode(x, labels, i, mode_selected, &mode_mv[mode_selected],
                &second_mode_mv[mode_selected], seg_mvs[i],
                bsi->ref_mv, bsi->second_ref_mv, x->nmvjointcost, x->mvcost);

    br += sbr;
    bd += sbd;
    segmentyrate += bestlabelyrate;
    this_segment_rd += best_label_rd;
    other_segment_rd += best_other_rd;
    if (rds)
      rds[i] = this_segment_rd;
    if (otherrds)
      otherrds[i] = other_segment_rd;
  } /* for each label */

  if (this_segment_rd < bsi->segment_rd) {
    bsi->r = br;
    bsi->d = bd;
    bsi->segment_yrate = segmentyrate;
    bsi->segment_rd = this_segment_rd;
    bsi->segment_num = segmentation;
    bsi->txfm_size = mbmi->txfm_size;

    // store everything needed to come back to this!!
    for (i = 0; i < 16; i++) {
      bsi->mvs[i].as_mv = x->partition_info->bmi[i].mv.as_mv;
      if (mbmi->second_ref_frame > 0)
        bsi->second_mvs[i].as_mv = x->partition_info->bmi[i].second_mv.as_mv;
      bsi->modes[i] = x->partition_info->bmi[i].mode;
      bsi->eobs[i] = best_eobs[i];
    }
  }

  if (completed) {
    *completed = i;
  }
}

static void rd_check_segment(VP9_COMP *cpi, MACROBLOCK *x,
                             BEST_SEG_INFO *bsi,
                             unsigned int segmentation,
                             /* 16 = n_blocks */
                             int_mv seg_mvs[16][MAX_REF_FRAMES - 1],
                             int64_t txfm_cache[NB_TXFM_MODES]) {
  int i, n, c = vp9_mbsplit_count[segmentation];

  if (segmentation == PARTITIONING_4X4) {
    int64_t rd[16];

    rd_check_segment_txsize(cpi, x, bsi, segmentation, TX_4X4, NULL,
                            rd, &n, seg_mvs);
    if (n == c) {
      for (i = 0; i < NB_TXFM_MODES; i++) {
        if (rd[c - 1] < txfm_cache[i])
          txfm_cache[i] = rd[c - 1];
      }
    }
  } else {
    int64_t diff, base_rd;
    int cost4x4 = vp9_cost_bit(cpi->common.prob_tx[0], 0);
    int cost8x8 = vp9_cost_bit(cpi->common.prob_tx[0], 1);

    if (cpi->common.txfm_mode == TX_MODE_SELECT) {
      int64_t rd4x4[4], rd8x8[4];
      int n4x4, n8x8, nmin;
      BEST_SEG_INFO bsi4x4, bsi8x8;

      /* factor in cost of cost4x4/8x8 in decision */
      vpx_memcpy(&bsi4x4, bsi, sizeof(*bsi));
      vpx_memcpy(&bsi8x8, bsi, sizeof(*bsi));
      rd_check_segment_txsize(cpi, x, &bsi4x4, segmentation,
                              TX_4X4, NULL, rd4x4, &n4x4, seg_mvs);
      rd_check_segment_txsize(cpi, x, &bsi8x8, segmentation,
                              TX_8X8, NULL, rd8x8, &n8x8, seg_mvs);
      if (bsi4x4.segment_num == segmentation) {
        bsi4x4.segment_rd += RDCOST(x->rdmult, x->rddiv, cost4x4, 0);
        if (bsi4x4.segment_rd < bsi->segment_rd)
          vpx_memcpy(bsi, &bsi4x4, sizeof(*bsi));
      }
      if (bsi8x8.segment_num == segmentation) {
        bsi8x8.segment_rd += RDCOST(x->rdmult, x->rddiv, cost8x8, 0);
        if (bsi8x8.segment_rd < bsi->segment_rd)
          vpx_memcpy(bsi, &bsi8x8, sizeof(*bsi));
      }
      n = n4x4 > n8x8 ? n4x4 : n8x8;
      if (n == c) {
        nmin = n4x4 < n8x8 ? n4x4 : n8x8;
        diff = rd8x8[nmin - 1] - rd4x4[nmin - 1];
        if (n == n4x4) {
          base_rd = rd4x4[c - 1];
        } else {
          base_rd = rd8x8[c - 1] - diff;
        }
      }
    } else {
      int64_t rd[4], otherrd[4];

      if (cpi->common.txfm_mode == ONLY_4X4) {
        rd_check_segment_txsize(cpi, x, bsi, segmentation, TX_4X4, otherrd,
                                rd, &n, seg_mvs);
        if (n == c) {
          base_rd = rd[c - 1];
          diff = otherrd[c - 1] - rd[c - 1];
        }
      } else /* use 8x8 transform */ {
        rd_check_segment_txsize(cpi, x, bsi, segmentation, TX_8X8, otherrd,
                                rd, &n, seg_mvs);
        if (n == c) {
          diff = rd[c - 1] - otherrd[c - 1];
          base_rd = otherrd[c - 1];
        }
      }
    }

    if (n == c) {
      if (base_rd < txfm_cache[ONLY_4X4]) {
        txfm_cache[ONLY_4X4] = base_rd;
      }
      if (base_rd + diff < txfm_cache[1]) {
        txfm_cache[ALLOW_8X8] = txfm_cache[ALLOW_16X16] = base_rd + diff;
      }
      if (diff < 0) {
        base_rd += diff + RDCOST(x->rdmult, x->rddiv, cost8x8, 0);
      } else {
        base_rd += RDCOST(x->rdmult, x->rddiv, cost4x4, 0);
      }
      if (base_rd < txfm_cache[TX_MODE_SELECT]) {
        txfm_cache[TX_MODE_SELECT] = base_rd;
      }
    }
  }
}

static __inline void cal_step_param(int sr, int *sp) {
  int step = 0;

  if (sr > MAX_FIRST_STEP) sr = MAX_FIRST_STEP;
  else if (sr < 1) sr = 1;

  while (sr >>= 1)
    step++;

  *sp = MAX_MVSEARCH_STEPS - 1 - step;
}

static int rd_pick_best_mbsegmentation(VP9_COMP *cpi, MACROBLOCK *x,
                                       int_mv *best_ref_mv,
                                       int_mv *second_best_ref_mv,
                                       int64_t best_rd,
                                       int *mdcounts,
                                       int *returntotrate,
                                       int *returnyrate,
                                       int *returndistortion,
                                       int *skippable, int mvthresh,
                                       int_mv seg_mvs[NB_PARTITIONINGS]
                                                     [16 /* n_blocks */]
                                                     [MAX_REF_FRAMES - 1],
                                       int64_t txfm_cache[NB_TXFM_MODES]) {
  int i;
  BEST_SEG_INFO bsi;
  MB_MODE_INFO * mbmi = &x->e_mbd.mode_info_context->mbmi;

  vpx_memset(&bsi, 0, sizeof(bsi));
  for (i = 0; i < NB_TXFM_MODES; i++)
    txfm_cache[i] = INT64_MAX;

  bsi.segment_rd = best_rd;
  bsi.ref_mv = best_ref_mv;
  bsi.second_ref_mv = second_best_ref_mv;
  bsi.mvp.as_int = best_ref_mv->as_int;
  bsi.mvthresh = mvthresh;
  bsi.mdcounts = mdcounts;
  bsi.txfm_size = TX_4X4;

  for (i = 0; i < 16; i++)
    bsi.modes[i] = ZERO4X4;

  if (cpi->compressor_speed == 0) {
    /* for now, we will keep the original segmentation order
       when in best quality mode */
    rd_check_segment(cpi, x, &bsi, PARTITIONING_16X8,
                     seg_mvs[PARTITIONING_16X8], txfm_cache);
    rd_check_segment(cpi, x, &bsi, PARTITIONING_8X16,
                     seg_mvs[PARTITIONING_8X16], txfm_cache);
    rd_check_segment(cpi, x, &bsi, PARTITIONING_8X8,
                     seg_mvs[PARTITIONING_8X8], txfm_cache);
    rd_check_segment(cpi, x, &bsi, PARTITIONING_4X4,
                     seg_mvs[PARTITIONING_4X4], txfm_cache);
  } else {
    int sr;

    rd_check_segment(cpi, x, &bsi, PARTITIONING_8X8,
                     seg_mvs[PARTITIONING_8X8], txfm_cache);

    if (bsi.segment_rd < best_rd) {
      int tmp_col_min = x->mv_col_min;
      int tmp_col_max = x->mv_col_max;
      int tmp_row_min = x->mv_row_min;
      int tmp_row_max = x->mv_row_max;

      vp9_clamp_mv_min_max(x, best_ref_mv);

      /* Get 8x8 result */
      bsi.sv_mvp[0].as_int = bsi.mvs[0].as_int;
      bsi.sv_mvp[1].as_int = bsi.mvs[2].as_int;
      bsi.sv_mvp[2].as_int = bsi.mvs[8].as_int;
      bsi.sv_mvp[3].as_int = bsi.mvs[10].as_int;

      /* Use 8x8 result as 16x8/8x16's predictor MV. Adjust search range
       * according to the closeness of 2 MV. */
      /* block 8X16 */
      sr = MAXF((abs(bsi.sv_mvp[0].as_mv.row - bsi.sv_mvp[2].as_mv.row)) >> 3,
                (abs(bsi.sv_mvp[0].as_mv.col - bsi.sv_mvp[2].as_mv.col)) >> 3);
      cal_step_param(sr, &bsi.sv_istep[0]);

      sr = MAXF((abs(bsi.sv_mvp[1].as_mv.row - bsi.sv_mvp[3].as_mv.row)) >> 3,
                (abs(bsi.sv_mvp[1].as_mv.col - bsi.sv_mvp[3].as_mv.col)) >> 3);
      cal_step_param(sr, &bsi.sv_istep[1]);

      rd_check_segment(cpi, x, &bsi, PARTITIONING_8X16,
                       seg_mvs[PARTITIONING_8X16], txfm_cache);

      /* block 16X8 */
      sr = MAXF((abs(bsi.sv_mvp[0].as_mv.row - bsi.sv_mvp[1].as_mv.row)) >> 3,
                (abs(bsi.sv_mvp[0].as_mv.col - bsi.sv_mvp[1].as_mv.col)) >> 3);
      cal_step_param(sr, &bsi.sv_istep[0]);

      sr = MAXF((abs(bsi.sv_mvp[2].as_mv.row - bsi.sv_mvp[3].as_mv.row)) >> 3,
                (abs(bsi.sv_mvp[2].as_mv.col - bsi.sv_mvp[3].as_mv.col)) >> 3);
      cal_step_param(sr, &bsi.sv_istep[1]);

      rd_check_segment(cpi, x, &bsi, PARTITIONING_16X8,
                       seg_mvs[PARTITIONING_16X8], txfm_cache);

      /* If 8x8 is better than 16x8/8x16, then do 4x4 search */
      /* Not skip 4x4 if speed=0 (good quality) */
      if (cpi->sf.no_skip_block4x4_search ||
          bsi.segment_num == PARTITIONING_8X8) {
        /* || (sv_segment_rd8x8-bsi.segment_rd) < sv_segment_rd8x8>>5) */
        bsi.mvp.as_int = bsi.sv_mvp[0].as_int;
        rd_check_segment(cpi, x, &bsi, PARTITIONING_4X4,
                         seg_mvs[PARTITIONING_4X4], txfm_cache);
      }

      /* restore UMV window */
      x->mv_col_min = tmp_col_min;
      x->mv_col_max = tmp_col_max;
      x->mv_row_min = tmp_row_min;
      x->mv_row_max = tmp_row_max;
    }
  }

  /* set it to the best */
  for (i = 0; i < 16; i++) {
    BLOCKD *bd = &x->e_mbd.block[i];

    bd->bmi.as_mv.first.as_int = bsi.mvs[i].as_int;
    if (mbmi->second_ref_frame > 0)
      bd->bmi.as_mv.second.as_int = bsi.second_mvs[i].as_int;
    bd->eob = bsi.eobs[i];
  }

  *returntotrate = bsi.r;
  *returndistortion = bsi.d;
  *returnyrate = bsi.segment_yrate;
  *skippable = bsi.txfm_size == TX_4X4 ?
                    vp9_mby_is_skippable_4x4(&x->e_mbd, 0) :
                    vp9_mby_is_skippable_8x8(&x->e_mbd, 0);

  /* save partitions */
  mbmi->txfm_size = bsi.txfm_size;
  mbmi->partitioning = bsi.segment_num;
  x->partition_info->count = vp9_mbsplit_count[bsi.segment_num];

  for (i = 0; i < x->partition_info->count; i++) {
    int j;

    j = vp9_mbsplit_offset[bsi.segment_num][i];

    x->partition_info->bmi[i].mode = bsi.modes[j];
    x->partition_info->bmi[i].mv.as_mv = bsi.mvs[j].as_mv;
    if (mbmi->second_ref_frame > 0)
      x->partition_info->bmi[i].second_mv.as_mv = bsi.second_mvs[j].as_mv;
  }
  /*
   * used to set mbmi->mv.as_int
   */
  x->partition_info->bmi[15].mv.as_int = bsi.mvs[15].as_int;
  if (mbmi->second_ref_frame > 0)
    x->partition_info->bmi[15].second_mv.as_int = bsi.second_mvs[15].as_int;

  return (int)(bsi.segment_rd);
}

static void mv_pred(VP9_COMP *cpi, MACROBLOCK *x,
                    uint8_t *ref_y_buffer, int ref_y_stride,
                    int ref_frame, enum BlockSize block_size ) {
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = &xd->mode_info_context->mbmi;
  int_mv this_mv;
  int i;
  int zero_seen = FALSE;
  int best_index = 0;
  int best_sad = INT_MAX;
  int this_sad = INT_MAX;

  BLOCK *b = &x->block[0];
  uint8_t *src_y_ptr = *(b->base_src);
  uint8_t *ref_y_ptr;
  int row_offset, col_offset;

  // Get the sad for each candidate reference mv
  for (i = 0; i < 4; i++) {
    this_mv.as_int = mbmi->ref_mvs[ref_frame][i].as_int;

    // The list is at an end if we see 0 for a second time.
    if (!this_mv.as_int && zero_seen)
      break;
    zero_seen = zero_seen || !this_mv.as_int;

    row_offset = this_mv.as_mv.row >> 3;
    col_offset = this_mv.as_mv.col >> 3;
    ref_y_ptr = ref_y_buffer + (ref_y_stride * row_offset) + col_offset;

    // Find sad for current vector.
    this_sad = cpi->fn_ptr[block_size].sdf(src_y_ptr, b->src_stride,
                                           ref_y_ptr, ref_y_stride,
                                           0x7fffffff);

    // Note if it is the best so far.
    if (this_sad < best_sad) {
      best_sad = this_sad;
      best_index = i;
    }
  }

  // Note the index of the mv that worked best in the reference list.
  x->mv_best_ref_index[ref_frame] = best_index;
}

static void set_i8x8_block_modes(MACROBLOCK *x, int modes[4]) {
  int i;
  MACROBLOCKD *xd = &x->e_mbd;
  for (i = 0; i < 4; i++) {
    int ib = vp9_i8x8_block[i];
    xd->mode_info_context->bmi[ib + 0].as_mode.first = modes[i];
    xd->mode_info_context->bmi[ib + 1].as_mode.first = modes[i];
    xd->mode_info_context->bmi[ib + 4].as_mode.first = modes[i];
    xd->mode_info_context->bmi[ib + 5].as_mode.first = modes[i];
    // printf("%d,%d,%d,%d\n",
    //       modes[0], modes[1], modes[2], modes[3]);
  }

  for (i = 0; i < 16; i++) {
    xd->block[i].bmi = xd->mode_info_context->bmi[i];
  }
}

extern void vp9_calc_ref_probs(int *count, vp9_prob *probs);
static void estimate_curframe_refprobs(VP9_COMP *cpi, vp9_prob mod_refprobs[3], int pred_ref) {
  int norm_cnt[MAX_REF_FRAMES];
  const int *const rfct = cpi->count_mb_ref_frame_usage;
  int intra_count = rfct[INTRA_FRAME];
  int last_count  = rfct[LAST_FRAME];
  int gf_count    = rfct[GOLDEN_FRAME];
  int arf_count   = rfct[ALTREF_FRAME];

  // Work out modified reference frame probabilities to use where prediction
  // of the reference frame fails
  if (pred_ref == INTRA_FRAME) {
    norm_cnt[0] = 0;
    norm_cnt[1] = last_count;
    norm_cnt[2] = gf_count;
    norm_cnt[3] = arf_count;
    vp9_calc_ref_probs(norm_cnt, mod_refprobs);
    mod_refprobs[0] = 0;    // This branch implicit
  } else if (pred_ref == LAST_FRAME) {
    norm_cnt[0] = intra_count;
    norm_cnt[1] = 0;
    norm_cnt[2] = gf_count;
    norm_cnt[3] = arf_count;
    vp9_calc_ref_probs(norm_cnt, mod_refprobs);
    mod_refprobs[1] = 0;    // This branch implicit
  } else if (pred_ref == GOLDEN_FRAME) {
    norm_cnt[0] = intra_count;
    norm_cnt[1] = last_count;
    norm_cnt[2] = 0;
    norm_cnt[3] = arf_count;
    vp9_calc_ref_probs(norm_cnt, mod_refprobs);
    mod_refprobs[2] = 0;  // This branch implicit
  } else {
    norm_cnt[0] = intra_count;
    norm_cnt[1] = last_count;
    norm_cnt[2] = gf_count;
    norm_cnt[3] = 0;
    vp9_calc_ref_probs(norm_cnt, mod_refprobs);
    mod_refprobs[2] = 0;  // This branch implicit
  }
}

static __inline unsigned weighted_cost(vp9_prob *tab0, vp9_prob *tab1, int idx, int val, int weight) {
  unsigned cost0 = tab0[idx] ? vp9_cost_bit(tab0[idx], val) : 0;
  unsigned cost1 = tab1[idx] ? vp9_cost_bit(tab1[idx], val) : 0;
  // weight is 16-bit fixed point, so this basically calculates:
  // 0.5 + weight * cost1 + (1.0 - weight) * cost0
  return (0x8000 + weight * cost1 + (0x10000 - weight) * cost0) >> 16;
}

static void estimate_ref_frame_costs(VP9_COMP *cpi, int segment_id, unsigned int *ref_costs) {
  VP9_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &cpi->mb.e_mbd;
  vp9_prob *mod_refprobs;

  unsigned int cost;
  int pred_ref;
  int pred_flag;
  int pred_ctx;
  int i;

  vp9_prob pred_prob, new_pred_prob;
  int seg_ref_active;
  int seg_ref_count = 0;
  seg_ref_active = vp9_segfeature_active(xd,
                                         segment_id,
                                         SEG_LVL_REF_FRAME);

  if (seg_ref_active) {
    seg_ref_count = vp9_check_segref(xd, segment_id, INTRA_FRAME)  +
                    vp9_check_segref(xd, segment_id, LAST_FRAME)   +
                    vp9_check_segref(xd, segment_id, GOLDEN_FRAME) +
                    vp9_check_segref(xd, segment_id, ALTREF_FRAME);
  }

  // Get the predicted reference for this mb
  pred_ref = vp9_get_pred_ref(cm, xd);

  // Get the context probability for the prediction flag (based on last frame)
  pred_prob = vp9_get_pred_prob(cm, xd, PRED_REF);

  // Predict probability for current frame based on stats so far
  pred_ctx = vp9_get_pred_context(cm, xd, PRED_REF);
  new_pred_prob = get_binary_prob(cpi->ref_pred_count[pred_ctx][0],
                                  cpi->ref_pred_count[pred_ctx][1]);

  // Get the set of probabilities to use if prediction fails
  mod_refprobs = cm->mod_refprobs[pred_ref];

  // For each possible selected reference frame work out a cost.
  for (i = 0; i < MAX_REF_FRAMES; i++) {
    if (seg_ref_active && seg_ref_count == 1) {
      cost = 0;
    } else {
      pred_flag = (i == pred_ref);

      // Get the prediction for the current mb
      cost = weighted_cost(&pred_prob, &new_pred_prob, 0,
                           pred_flag, cpi->seg0_progress);
      if (cost > 1024) cost = 768; // i.e. account for 4 bits max.

      // for incorrectly predicted cases
      if (! pred_flag) {
        vp9_prob curframe_mod_refprobs[3];

        if (cpi->seg0_progress) {
          estimate_curframe_refprobs(cpi, curframe_mod_refprobs, pred_ref);
        } else {
          vpx_memset(curframe_mod_refprobs, 0, sizeof(curframe_mod_refprobs));
        }

        cost += weighted_cost(mod_refprobs, curframe_mod_refprobs, 0,
                              (i != INTRA_FRAME), cpi->seg0_progress);
        if (i != INTRA_FRAME) {
          cost += weighted_cost(mod_refprobs, curframe_mod_refprobs, 1,
                                (i != LAST_FRAME), cpi->seg0_progress);
          if (i != LAST_FRAME) {
            cost += weighted_cost(mod_refprobs, curframe_mod_refprobs, 2,
                                  (i != GOLDEN_FRAME), cpi->seg0_progress);
          }
        }
      }
    }

    ref_costs[i] = cost;
  }
}

static void store_coding_context(MACROBLOCK *x, PICK_MODE_CONTEXT *ctx,
                                 int mode_index,
                                 PARTITION_INFO *partition,
                                 int_mv *ref_mv,
                                 int_mv *second_ref_mv,
                                 int64_t comp_pred_diff[NB_PREDICTION_TYPES],
                                 int64_t txfm_size_diff[NB_TXFM_MODES]) {
  MACROBLOCKD *const xd = &x->e_mbd;

  // Take a snapshot of the coding context so it can be
  // restored if we decide to encode this way
  ctx->skip = x->skip;
  ctx->best_mode_index = mode_index;
  vpx_memcpy(&ctx->mic, xd->mode_info_context,
             sizeof(MODE_INFO));
  if (partition)
    vpx_memcpy(&ctx->partition_info, partition,
               sizeof(PARTITION_INFO));
  ctx->best_ref_mv.as_int = ref_mv->as_int;
  ctx->second_best_ref_mv.as_int = second_ref_mv->as_int;

  ctx->single_pred_diff = (int)comp_pred_diff[SINGLE_PREDICTION_ONLY];
  ctx->comp_pred_diff   = (int)comp_pred_diff[COMP_PREDICTION_ONLY];
  ctx->hybrid_pred_diff = (int)comp_pred_diff[HYBRID_PREDICTION];

  memcpy(ctx->txfm_rd_diff, txfm_size_diff, sizeof(ctx->txfm_rd_diff));
}

static void inter_mode_cost(VP9_COMP *cpi, MACROBLOCK *x,
                            int *rate2, int *distortion2, int *rate_y,
                            int *distortion, int* rate_uv, int *distortion_uv,
                            int *skippable, int64_t txfm_cache[NB_TXFM_MODES]) {
  int y_skippable, uv_skippable;

  // Y cost and distortion
  macro_block_yrd(cpi, x, rate_y, distortion, &y_skippable, txfm_cache);

  *rate2 += *rate_y;
  *distortion2 += *distortion;

  // UV cost and distortion
  vp9_subtract_mbuv(x->src_diff, x->src.u_buffer, x->src.v_buffer,
                    x->e_mbd.predictor, x->src.uv_stride);
  if (x->e_mbd.mode_info_context->mbmi.txfm_size != TX_4X4)
    rd_inter16x16_uv_8x8(cpi, x, rate_uv, distortion_uv,
                         cpi->common.full_pixel, &uv_skippable, 1);
  else
    rd_inter16x16_uv_4x4(cpi, x, rate_uv, distortion_uv,
                         cpi->common.full_pixel, &uv_skippable, 1);

  *rate2 += *rate_uv;
  *distortion2 += *distortion_uv;
  *skippable = y_skippable && uv_skippable;
}

static void setup_buffer_inter(VP9_COMP *cpi, MACROBLOCK *x,
                               int idx, MV_REFERENCE_FRAME frame_type,
                               int block_size,
                               int recon_yoffset, int recon_uvoffset,
                               int_mv frame_nearest_mv[MAX_REF_FRAMES],
                               int_mv frame_near_mv[MAX_REF_FRAMES],
                               int frame_mdcounts[4][4],
                               uint8_t *y_buffer[4],
                               uint8_t *u_buffer[4],
                               uint8_t *v_buffer[4]) {
  YV12_BUFFER_CONFIG *yv12 = &cpi->common.yv12_fb[idx];
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO *const mbmi = &xd->mode_info_context->mbmi;

  y_buffer[frame_type] = yv12->y_buffer + recon_yoffset;
  u_buffer[frame_type] = yv12->u_buffer + recon_uvoffset;
  v_buffer[frame_type] = yv12->v_buffer + recon_uvoffset;

  // Gets an initial list of candidate vectors from neighbours and orders them
  vp9_find_mv_refs(xd, xd->mode_info_context,
                   xd->prev_mode_info_context,
                   frame_type,
                   mbmi->ref_mvs[frame_type],
                   cpi->common.ref_frame_sign_bias);

  // Candidate refinement carried out at encoder and decoder
  vp9_find_best_ref_mvs(xd, y_buffer[frame_type],
                        yv12->y_stride,
                        mbmi->ref_mvs[frame_type],
                        &frame_nearest_mv[frame_type],
                        &frame_near_mv[frame_type]);


  // Further refinement that is encode side only to test the top few candidates
  // in full and choose the best as the centre point for subsequent searches.
  mv_pred(cpi, x, y_buffer[frame_type], yv12->y_stride,
          frame_type, block_size);

}

static int64_t handle_inter_mode(VP9_COMP *cpi, MACROBLOCK *x,
                                 enum BlockSize block_size,
                                 int *saddone, int near_sadidx[],
                                 int mdcounts[4], int64_t txfm_cache[],
                                 int *rate2, int *distortion, int *skippable,
                                 int *compmode_cost,
#if CONFIG_COMP_INTERINTRA_PRED
                                 int *compmode_interintra_cost,
#endif
                                 int *rate_y, int *distortion_y,
                                 int *rate_uv, int *distortion_uv,
                                 int *mode_excluded, int *disable_skip,
                                 int recon_yoffset, int mode_index,
                                 int_mv frame_mv[MB_MODE_COUNT]
                                                [MAX_REF_FRAMES]) {
  VP9_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = &xd->mode_info_context->mbmi;
  BLOCK *b = &x->block[0];
  BLOCKD *d = &xd->block[0];
  const int is_comp_pred = (mbmi->second_ref_frame > 0);
#if CONFIG_COMP_INTERINTRA_PRED
  const int is_comp_interintra_pred = (mbmi->second_ref_frame == INTRA_FRAME);
#endif
  const int num_refs = is_comp_pred ? 2 : 1;
  const int this_mode = mbmi->mode;
  int i;
  int refs[2] = { mbmi->ref_frame,
                  (mbmi->second_ref_frame < 0 ? 0 : mbmi->second_ref_frame) };
  int_mv cur_mv[2];
  int_mv ref_mv[2];
  int64_t this_rd = 0;

  switch (this_mode) {
    case NEWMV:
      ref_mv[0] = mbmi->ref_mvs[refs[0]][0];
      ref_mv[1] = mbmi->ref_mvs[refs[1]][0];

      if (is_comp_pred) {
        if (frame_mv[NEWMV][refs[0]].as_int == INVALID_MV ||
            frame_mv[NEWMV][refs[1]].as_int == INVALID_MV)
          return INT64_MAX;
        *rate2 += vp9_mv_bit_cost(&frame_mv[NEWMV][refs[0]],
                                  &ref_mv[0],
                                  x->nmvjointcost, x->mvcost, 96,
                                  x->e_mbd.allow_high_precision_mv);
        *rate2 += vp9_mv_bit_cost(&frame_mv[NEWMV][refs[1]],
                                  &ref_mv[1],
                                  x->nmvjointcost, x->mvcost, 96,
                                  x->e_mbd.allow_high_precision_mv);
      } else {
        int bestsme = INT_MAX;
        int further_steps, step_param = cpi->sf.first_step;
        int sadpb = x->sadperbit16;
        int_mv mvp_full, tmp_mv;
        int sr = 0;

        int tmp_col_min = x->mv_col_min;
        int tmp_col_max = x->mv_col_max;
        int tmp_row_min = x->mv_row_min;
        int tmp_row_max = x->mv_row_max;

        vp9_clamp_mv_min_max(x, &ref_mv[0]);

        // mvp_full.as_int = ref_mv[0].as_int;
        mvp_full.as_int =
         mbmi->ref_mvs[refs[0]][x->mv_best_ref_index[refs[0]]].as_int;

        mvp_full.as_mv.col >>= 3;
        mvp_full.as_mv.row >>= 3;
        if (mvp_full.as_int != mvp_full.as_int) {
          mvp_full.as_int = mvp_full.as_int;
        }

        // adjust search range according to sr from mv prediction
        step_param = MAX(step_param, sr);

        // Further step/diamond searches as necessary
        further_steps = (cpi->sf.max_step_search_steps - 1) - step_param;

        bestsme = vp9_full_pixel_diamond(cpi, x, b, d, &mvp_full, step_param,
                                         sadpb, further_steps, 1,
                                         &cpi->fn_ptr[block_size],
                                         &ref_mv[0], &tmp_mv);

        x->mv_col_min = tmp_col_min;
        x->mv_col_max = tmp_col_max;
        x->mv_row_min = tmp_row_min;
        x->mv_row_max = tmp_row_max;

        if (bestsme < INT_MAX) {
          int dis; /* TODO: use dis in distortion calculation later. */
          unsigned int sse;
          cpi->find_fractional_mv_step(x, b, d, &tmp_mv,
                                       &ref_mv[0],
                                       x->errorperbit,
                                       &cpi->fn_ptr[block_size],
                                       x->nmvjointcost, x->mvcost,
                                       &dis, &sse);
        }
        d->bmi.as_mv.first.as_int = tmp_mv.as_int;
        frame_mv[NEWMV][refs[0]].as_int = d->bmi.as_mv.first.as_int;

        // Add the new motion vector cost to our rolling cost variable
        *rate2 += vp9_mv_bit_cost(&tmp_mv, &ref_mv[0],
                                  x->nmvjointcost, x->mvcost,
                                  96, xd->allow_high_precision_mv);
      }
      break;
    case NEARESTMV:
    case NEARMV:
      // Do not bother proceeding if the vector (from newmv, nearest or
      // near) is 0,0 as this should then be coded using the zeromv mode.
      for (i = 0; i < num_refs; ++i)
        if (frame_mv[this_mode][refs[i]].as_int == 0)
          return INT64_MAX;
    case ZEROMV:
    default:
      break;
  }
  for (i = 0; i < num_refs; ++i) {
    cur_mv[i] = frame_mv[this_mode][refs[i]];
    // Clip "next_nearest" so that it does not extend to far out of image
    clamp_mv2(&cur_mv[i], xd);
    if (mv_check_bounds(x, &cur_mv[i]))
      return INT64_MAX;
    mbmi->mv[i].as_int = cur_mv[i].as_int;
  }

  if (cpi->common.mcomp_filter_type == SWITCHABLE) {
    const int c = vp9_get_pred_context(cm, xd, PRED_SWITCHABLE_INTERP);
    const int m = vp9_switchable_interp_map[mbmi->interp_filter];
    *rate2 += SWITCHABLE_INTERP_RATE_FACTOR * x->switchable_interp_costs[c][m];
  }

  /* We don't include the cost of the second reference here, because there
   * are only three options: Last/Golden, ARF/Last or Golden/ARF, or in other
   * words if you present them in that order, the second one is always known
   * if the first is known */
  *compmode_cost = vp9_cost_bit(vp9_get_pred_prob(cm, xd, PRED_COMP),
                                is_comp_pred);
  *rate2 += vp9_cost_mv_ref(cpi, this_mode,
                            mbmi->mb_mode_context[mbmi->ref_frame]);
#if CONFIG_COMP_INTERINTRA_PRED
  if (!is_comp_pred) {
    *compmode_interintra_cost = vp9_cost_bit(cm->fc.interintra_prob,
                                             is_comp_interintra_pred);
    if (is_comp_interintra_pred) {
      *compmode_interintra_cost +=
          x->mbmode_cost[xd->frame_type][mbmi->interintra_mode];
#if SEPARATE_INTERINTRA_UV
      *compmode_interintra_cost +=
          x->intra_uv_mode_cost[xd->frame_type][mbmi->interintra_uv_mode];
#endif
    }
  }
#endif

  if (block_size == BLOCK_64X64) {
    vp9_build_inter64x64_predictors_sb(xd,
                                       xd->dst.y_buffer,
                                       xd->dst.u_buffer,
                                       xd->dst.v_buffer,
                                       xd->dst.y_stride,
                                       xd->dst.uv_stride);
  } else if (block_size == BLOCK_32X32) {
    vp9_build_inter32x32_predictors_sb(xd,
                                       xd->dst.y_buffer,
                                       xd->dst.u_buffer,
                                       xd->dst.v_buffer,
                                       xd->dst.y_stride,
                                       xd->dst.uv_stride);
  } else {
    assert(block_size == BLOCK_16X16);
    vp9_build_1st_inter16x16_predictors_mby(xd, xd->predictor, 16, 0);
    if (is_comp_pred)
      vp9_build_2nd_inter16x16_predictors_mby(xd, xd->predictor, 16);
#if CONFIG_COMP_INTERINTRA_PRED
    if (is_comp_interintra_pred) {
      vp9_build_interintra_16x16_predictors_mby(xd, xd->predictor, 16);
    }
#endif
  }

  if (cpi->active_map_enabled && x->active_ptr[0] == 0)
    x->skip = 1;
  else if (x->encode_breakout) {
    unsigned int sse, var;
    int threshold = (xd->block[0].dequant[1]
                     * xd->block[0].dequant[1] >> 4);

    if (threshold < x->encode_breakout)
      threshold = x->encode_breakout;

    if (block_size == BLOCK_64X64) {
      var = vp9_variance64x64(*(b->base_src), b->src_stride,
                              xd->dst.y_buffer, xd->dst.y_stride, &sse);
    } else if (block_size == BLOCK_32X32) {
      var = vp9_variance32x32(*(b->base_src), b->src_stride,
                              xd->dst.y_buffer, xd->dst.y_stride, &sse);
    } else {
      assert(block_size == BLOCK_16X16);
      var = vp9_variance16x16(*(b->base_src), b->src_stride,
                              xd->predictor, 16, &sse);
    }

    if ((int)sse < threshold) {
      unsigned int q2dc = xd->block[24].dequant[0];
      /* If there is no codeable 2nd order dc
       or a very small uniform pixel change change */
      if ((sse - var < q2dc * q2dc >> 4) ||
          (sse / 2 > var && sse - var < 64)) {
        // Check u and v to make sure skip is ok
        int sse2;

        if (block_size == BLOCK_64X64) {
          unsigned int sse2u, sse2v;
          var = vp9_variance32x32(x->src.u_buffer, x->src.uv_stride,
                                  xd->dst.u_buffer, xd->dst.uv_stride, &sse2u);
          var = vp9_variance32x32(x->src.v_buffer, x->src.uv_stride,
                                  xd->dst.v_buffer, xd->dst.uv_stride, &sse2v);
          sse2 = sse2u + sse2v;
        } else if (block_size == BLOCK_32X32) {
          unsigned int sse2u, sse2v;
          var = vp9_variance16x16(x->src.u_buffer, x->src.uv_stride,
                                  xd->dst.u_buffer, xd->dst.uv_stride, &sse2u);
          var = vp9_variance16x16(x->src.v_buffer, x->src.uv_stride,
                                  xd->dst.v_buffer, xd->dst.uv_stride, &sse2v);
          sse2 = sse2u + sse2v;
        } else {
          assert(block_size == BLOCK_16X16);
          sse2 = vp9_uvsse(x);
        }

        if (sse2 * 2 < threshold) {
          x->skip = 1;
          *distortion = sse + sse2;
          *rate2 = 500;

          /* for best_yrd calculation */
          *rate_uv = 0;
          *distortion_uv = sse2;

          *disable_skip = 1;
          this_rd = RDCOST(x->rdmult, x->rddiv, *rate2, *distortion);
        }
      }
    }
  }

  if (!(*mode_excluded)) {
    if (is_comp_pred) {
      *mode_excluded = (cpi->common.comp_pred_mode == SINGLE_PREDICTION_ONLY);
    } else {
      *mode_excluded = (cpi->common.comp_pred_mode == COMP_PREDICTION_ONLY);
    }
#if CONFIG_COMP_INTERINTRA_PRED
    if (is_comp_interintra_pred && !cm->use_interintra) *mode_excluded = 1;
#endif
  }

  if (!x->skip) {
    if (block_size == BLOCK_64X64) {
      int skippable_y, skippable_uv;

      // Y cost and distortion
      super_block_64_yrd(cpi, x, rate_y, distortion_y,
                         &skippable_y, txfm_cache);
      *rate2 += *rate_y;
      *distortion += *distortion_y;

      rd_inter64x64_uv(cpi, x, rate_uv, distortion_uv,
                       cm->full_pixel, &skippable_uv);

      *rate2 += *rate_uv;
      *distortion += *distortion_uv;
      *skippable = skippable_y && skippable_uv;
    } else if (block_size == BLOCK_32X32) {
      int skippable_y, skippable_uv;

      // Y cost and distortion
      super_block_yrd(cpi, x, rate_y, distortion_y,
                      &skippable_y, txfm_cache);
      *rate2 += *rate_y;
      *distortion += *distortion_y;

      rd_inter32x32_uv(cpi, x, rate_uv, distortion_uv,
                       cm->full_pixel, &skippable_uv);

      *rate2 += *rate_uv;
      *distortion += *distortion_uv;
      *skippable = skippable_y && skippable_uv;
    } else {
      assert(block_size == BLOCK_16X16);

      vp9_build_1st_inter16x16_predictors_mbuv(xd, &xd->predictor[256],
                                               &xd->predictor[320], 8);
      if (is_comp_pred)
        vp9_build_2nd_inter16x16_predictors_mbuv(xd, &xd->predictor[256],
                                                 &xd->predictor[320], 8);
#if CONFIG_COMP_INTERINTRA_PRED
      if (is_comp_interintra_pred) {
        vp9_build_interintra_16x16_predictors_mbuv(xd, &xd->predictor[256],
                                                   &xd->predictor[320], 8);
      }
#endif
      inter_mode_cost(cpi, x, rate2, distortion,
                      rate_y, distortion_y, rate_uv, distortion_uv,
                      skippable, txfm_cache);
    }
  }
  return this_rd;  // if 0, this will be re-calculated by caller
}

static void rd_pick_inter_mode(VP9_COMP *cpi, MACROBLOCK *x,
                               int recon_yoffset, int recon_uvoffset,
                               int *returnrate, int *returndistortion,
                               int64_t *returnintra) {
  VP9_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  union b_mode_info best_bmodes[16];
  MB_MODE_INFO best_mbmode;
  PARTITION_INFO best_partition;
  int_mv best_ref_mv, second_best_ref_mv;
  MB_PREDICTION_MODE this_mode;
  MB_PREDICTION_MODE best_mode = DC_PRED;
  MB_MODE_INFO * mbmi = &xd->mode_info_context->mbmi;
  int i, best_mode_index = 0;
  int mode8x8[4];
  unsigned char segment_id = mbmi->segment_id;

  int mode_index;
  int mdcounts[4];
  int rate, distortion;
  int rate2, distortion2;
  int64_t best_txfm_rd[NB_TXFM_MODES];
  int64_t best_txfm_diff[NB_TXFM_MODES];
  int64_t best_pred_diff[NB_PREDICTION_TYPES];
  int64_t best_pred_rd[NB_PREDICTION_TYPES];
  int64_t best_rd = INT64_MAX, best_intra_rd = INT64_MAX;
#if CONFIG_COMP_INTERINTRA_PRED
  int is_best_interintra = 0;
  int64_t best_intra16_rd = INT64_MAX;
  int best_intra16_mode = DC_PRED, best_intra16_uv_mode = DC_PRED;
#endif
  int64_t best_overall_rd = INT64_MAX;
  INTERPOLATIONFILTERTYPE best_filter = SWITCHABLE;
  int uv_intra_rate, uv_intra_distortion, uv_intra_rate_tokenonly;
  int uv_intra_skippable = 0;
  int uv_intra_rate_8x8 = 0, uv_intra_distortion_8x8 = 0, uv_intra_rate_tokenonly_8x8 = 0;
  int uv_intra_skippable_8x8 = 0;
  int rate_y, UNINITIALIZED_IS_SAFE(rate_uv);
  int distortion_uv = INT_MAX;
  int64_t best_yrd = INT64_MAX;
  int switchable_filter_index = 0;

  MB_PREDICTION_MODE uv_intra_mode;
  MB_PREDICTION_MODE uv_intra_mode_8x8 = 0;

  int near_sadidx[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  int saddone = 0;

  int_mv frame_mv[MB_MODE_COUNT][MAX_REF_FRAMES];
  int frame_mdcounts[4][4];
  uint8_t *y_buffer[4], *u_buffer[4], *v_buffer[4];

  unsigned int ref_costs[MAX_REF_FRAMES];
  int_mv seg_mvs[NB_PARTITIONINGS][16 /* n_blocks */][MAX_REF_FRAMES - 1];

  int intra_cost_penalty = 20 * vp9_dc_quant(cpi->common.base_qindex,
                                             cpi->common.y1dc_delta_q);

  vpx_memset(mode8x8, 0, sizeof(mode8x8));
  vpx_memset(&frame_mv, 0, sizeof(frame_mv));
  vpx_memset(&best_mbmode, 0, sizeof(best_mbmode));
  vpx_memset(&best_bmodes, 0, sizeof(best_bmodes));
  vpx_memset(&x->mb_context[xd->sb_index][xd->mb_index], 0,
             sizeof(PICK_MODE_CONTEXT));

  for (i = 0; i < MAX_REF_FRAMES; i++)
    frame_mv[NEWMV][i].as_int = INVALID_MV;
  for (i = 0; i < NB_PREDICTION_TYPES; ++i)
    best_pred_rd[i] = INT64_MAX;
  for (i = 0; i < NB_TXFM_MODES; i++)
    best_txfm_rd[i] = INT64_MAX;

  for (i = 0; i < NB_PARTITIONINGS; i++) {
    int j, k;

    for (j = 0; j < 16; j++)
      for (k = 0; k < MAX_REF_FRAMES - 1; k++)
        seg_mvs[i][j][k].as_int = INVALID_MV;
  }

  if (cpi->ref_frame_flags & VP9_LAST_FLAG) {
    setup_buffer_inter(cpi, x, cpi->common.lst_fb_idx, LAST_FRAME,
                       BLOCK_16X16, recon_yoffset, recon_uvoffset,
                       frame_mv[NEARESTMV], frame_mv[NEARMV],
                       frame_mdcounts, y_buffer, u_buffer, v_buffer);
  }

  if (cpi->ref_frame_flags & VP9_GOLD_FLAG) {
    setup_buffer_inter(cpi, x, cpi->common.gld_fb_idx, GOLDEN_FRAME,
                       BLOCK_16X16, recon_yoffset, recon_uvoffset,
                       frame_mv[NEARESTMV], frame_mv[NEARMV],
                       frame_mdcounts, y_buffer, u_buffer, v_buffer);
  }

  if (cpi->ref_frame_flags & VP9_ALT_FLAG) {
    setup_buffer_inter(cpi, x, cpi->common.alt_fb_idx, ALTREF_FRAME,
                       BLOCK_16X16, recon_yoffset, recon_uvoffset,
                       frame_mv[NEARESTMV], frame_mv[NEARMV],
                       frame_mdcounts, y_buffer, u_buffer, v_buffer);
  }

  *returnintra = INT64_MAX;

  mbmi->ref_frame = INTRA_FRAME;

  /* Initialize zbin mode boost for uv costing */
  cpi->zbin_mode_boost = 0;
  vp9_update_zbin_extra(cpi, x);

  rd_pick_intra_mbuv_mode(cpi, x, &uv_intra_rate,
                          &uv_intra_rate_tokenonly, &uv_intra_distortion,
                          &uv_intra_skippable);
  uv_intra_mode = mbmi->uv_mode;

  /* rough estimate for now */
  if (cpi->common.txfm_mode != ONLY_4X4) {
    rd_pick_intra_mbuv_mode_8x8(cpi, x, &uv_intra_rate_8x8,
                                &uv_intra_rate_tokenonly_8x8,
                                &uv_intra_distortion_8x8,
                                &uv_intra_skippable_8x8);
    uv_intra_mode_8x8 = mbmi->uv_mode;
  }

  // Get estimates of reference frame costs for each reference frame
  // that depend on the current prediction etc.
  estimate_ref_frame_costs(cpi, segment_id, ref_costs);

  for (mode_index = 0; mode_index < MAX_MODES;
       mode_index += (!switchable_filter_index)) {
    int64_t this_rd = INT64_MAX;
    int disable_skip = 0, skippable = 0;
    int other_cost = 0;
    int compmode_cost = 0;
#if CONFIG_COMP_INTERINTRA_PRED
    int compmode_interintra_cost = 0;
#endif
    int mode_excluded = 0;
    int64_t txfm_cache[NB_TXFM_MODES] = { 0 };

    // These variables hold are rolling total cost and distortion for this mode
    rate2 = 0;
    distortion2 = 0;
    rate_y = 0;
    rate_uv = 0;

    x->skip = 0;

    this_mode = vp9_mode_order[mode_index].mode;
    mbmi->mode = this_mode;
    mbmi->uv_mode = DC_PRED;
    mbmi->ref_frame = vp9_mode_order[mode_index].ref_frame;
    mbmi->second_ref_frame = vp9_mode_order[mode_index].second_ref_frame;

    // Evaluate all sub-pel filters irrespective of whether we can use
    // them for this frame.
    if (this_mode >= NEARESTMV && this_mode <= SPLITMV) {
      mbmi->interp_filter =
          vp9_switchable_interp[switchable_filter_index++];
      if (switchable_filter_index == VP9_SWITCHABLE_FILTERS)
        switchable_filter_index = 0;
      if ((cm->mcomp_filter_type != SWITCHABLE) &&
          (cm->mcomp_filter_type != mbmi->interp_filter)) {
        mode_excluded = 1;
      }
      vp9_setup_interp_filters(xd, mbmi->interp_filter, &cpi->common);
    }

    // Test best rd so far against threshold for trying this mode.
    if (best_rd <= cpi->rd_threshes[mode_index])
      continue;

    // current coding mode under rate-distortion optimization test loop
#if CONFIG_COMP_INTERINTRA_PRED
    mbmi->interintra_mode = (MB_PREDICTION_MODE)(DC_PRED - 1);
    mbmi->interintra_uv_mode = (MB_PREDICTION_MODE)(DC_PRED - 1);
#endif

    // If the segment reference frame feature is enabled....
    // then do nothing if the current ref frame is not allowed..
    if (vp9_segfeature_active(xd, segment_id, SEG_LVL_REF_FRAME) &&
        !vp9_check_segref(xd, segment_id, mbmi->ref_frame)) {
      continue;
    // If the segment mode feature is enabled....
    // then do nothing if the current mode is not allowed..
    } else if (vp9_segfeature_active(xd, segment_id, SEG_LVL_MODE) &&
               (this_mode !=
                vp9_get_segdata(xd, segment_id, SEG_LVL_MODE))) {
      continue;
    // Disable this drop out case if either the mode or ref frame
    // segment level feature is enabled for this segment. This is to
    // prevent the possibility that the we end up unable to pick any mode.
    } else if (!vp9_segfeature_active(xd, segment_id, SEG_LVL_REF_FRAME) &&
               !vp9_segfeature_active(xd, segment_id, SEG_LVL_MODE)) {
      // Only consider ZEROMV/ALTREF_FRAME for alt ref frame,
      // unless ARNR filtering is enabled in which case we want
      // an unfiltered alternative
      if (cpi->is_src_frame_alt_ref && (cpi->oxcf.arnr_max_frames == 0)) {
        if (this_mode != ZEROMV ||
            mbmi->ref_frame != ALTREF_FRAME) {
          continue;
        }
      }
    }

    /* everything but intra */
    if (mbmi->ref_frame) {
      int ref = mbmi->ref_frame;

      xd->pre.y_buffer = y_buffer[ref];
      xd->pre.u_buffer = u_buffer[ref];
      xd->pre.v_buffer = v_buffer[ref];
      best_ref_mv = mbmi->ref_mvs[ref][0];
      vpx_memcpy(mdcounts, frame_mdcounts[ref], sizeof(mdcounts));
    }

    if (mbmi->second_ref_frame > 0) {
      int ref = mbmi->second_ref_frame;

      xd->second_pre.y_buffer = y_buffer[ref];
      xd->second_pre.u_buffer = u_buffer[ref];
      xd->second_pre.v_buffer = v_buffer[ref];
      second_best_ref_mv = mbmi->ref_mvs[ref][0];
    }

    // Experimental code. Special case for gf and arf zeromv modes.
    // Increase zbin size to suppress noise
    if (cpi->zbin_mode_boost_enabled) {
      if (vp9_mode_order[mode_index].ref_frame == INTRA_FRAME)
        cpi->zbin_mode_boost = 0;
      else {
        if (vp9_mode_order[mode_index].mode == ZEROMV) {
          if (vp9_mode_order[mode_index].ref_frame != LAST_FRAME)
            cpi->zbin_mode_boost = GF_ZEROMV_ZBIN_BOOST;
          else
            cpi->zbin_mode_boost = LF_ZEROMV_ZBIN_BOOST;
        } else if (vp9_mode_order[mode_index].mode == SPLITMV)
          cpi->zbin_mode_boost = 0;
        else
          cpi->zbin_mode_boost = MV_ZBIN_BOOST;
      }

      vp9_update_zbin_extra(cpi, x);
    }

    // Intra
    if (!mbmi->ref_frame) {
      switch (this_mode) {
        default:
        case V_PRED:
        case H_PRED:
        case D45_PRED:
        case D135_PRED:
        case D117_PRED:
        case D153_PRED:
        case D27_PRED:
        case D63_PRED:
          rate2 += intra_cost_penalty;
        case DC_PRED:
        case TM_PRED:
          mbmi->ref_frame = INTRA_FRAME;
          // FIXME compound intra prediction
          vp9_build_intra_predictors_mby(&x->e_mbd);
          macro_block_yrd(cpi, x, &rate_y, &distortion, &skippable, txfm_cache);
          rate2 += rate_y;
          distortion2 += distortion;
          rate2 += x->mbmode_cost[xd->frame_type][mbmi->mode];
          if (mbmi->txfm_size != TX_4X4) {
            rate2 += uv_intra_rate_8x8;
            rate_uv = uv_intra_rate_tokenonly_8x8;
            distortion2 += uv_intra_distortion_8x8;
            distortion_uv = uv_intra_distortion_8x8;
            skippable = skippable && uv_intra_skippable_8x8;
          } else {
            rate2 += uv_intra_rate;
            rate_uv = uv_intra_rate_tokenonly;
            distortion2 += uv_intra_distortion;
            distortion_uv = uv_intra_distortion;
            skippable = skippable && uv_intra_skippable;
          }
          break;
        case B_PRED: {
          int64_t tmp_rd;

          // Note the rate value returned here includes the cost of coding
          // the BPRED mode : x->mbmode_cost[xd->frame_type][BPRED];
          mbmi->txfm_size = TX_4X4;
          tmp_rd = rd_pick_intra4x4mby_modes(cpi, x, &rate, &rate_y,
                                             &distortion, best_yrd,
                                             cpi->update_context);
          rate2 += rate;
          rate2 += intra_cost_penalty;
          distortion2 += distortion;

          if (tmp_rd < best_yrd) {
            rate2 += uv_intra_rate;
            rate_uv = uv_intra_rate_tokenonly;
            distortion2 += uv_intra_distortion;
            distortion_uv = uv_intra_distortion;
          } else {
            this_rd = INT64_MAX;
            disable_skip = 1;
          }
        }
        break;
        case I8X8_PRED: {
          int cost0 = vp9_cost_bit(cm->prob_tx[0], 0);
          int cost1 = vp9_cost_bit(cm->prob_tx[0], 1);
          int64_t tmp_rd_4x4s, tmp_rd_8x8s;
          int64_t tmp_rd_4x4, tmp_rd_8x8, tmp_rd;
          int r4x4, tok4x4, d4x4, r8x8, tok8x8, d8x8;
          mbmi->txfm_size = TX_4X4;
          tmp_rd_4x4 = rd_pick_intra8x8mby_modes(cpi, x, &r4x4, &tok4x4,
                                                 &d4x4, best_yrd);
          mode8x8[0] = xd->mode_info_context->bmi[0].as_mode.first;
          mode8x8[1] = xd->mode_info_context->bmi[2].as_mode.first;
          mode8x8[2] = xd->mode_info_context->bmi[8].as_mode.first;
          mode8x8[3] = xd->mode_info_context->bmi[10].as_mode.first;
          mbmi->txfm_size = TX_8X8;
          tmp_rd_8x8 = rd_pick_intra8x8mby_modes(cpi, x, &r8x8, &tok8x8,
                                                 &d8x8, best_yrd);
          txfm_cache[ONLY_4X4]  = tmp_rd_4x4;
          txfm_cache[ALLOW_8X8] = tmp_rd_8x8;
          txfm_cache[ALLOW_16X16] = tmp_rd_8x8;
          tmp_rd_4x4s = tmp_rd_4x4 + RDCOST(x->rdmult, x->rddiv, cost0, 0);
          tmp_rd_8x8s = tmp_rd_8x8 + RDCOST(x->rdmult, x->rddiv, cost1, 0);
          txfm_cache[TX_MODE_SELECT] = tmp_rd_4x4s < tmp_rd_8x8s ? tmp_rd_4x4s : tmp_rd_8x8s;
          if (cm->txfm_mode == TX_MODE_SELECT) {
            if (tmp_rd_4x4s < tmp_rd_8x8s) {
              rate = r4x4 + cost0;
              rate_y = tok4x4 + cost0;
              distortion = d4x4;
              mbmi->txfm_size = TX_4X4;
              tmp_rd = tmp_rd_4x4s;
            } else {
              rate = r8x8 + cost1;
              rate_y = tok8x8 + cost1;
              distortion = d8x8;
              mbmi->txfm_size = TX_8X8;
              tmp_rd = tmp_rd_8x8s;

              mode8x8[0] = xd->mode_info_context->bmi[0].as_mode.first;
              mode8x8[1] = xd->mode_info_context->bmi[2].as_mode.first;
              mode8x8[2] = xd->mode_info_context->bmi[8].as_mode.first;
              mode8x8[3] = xd->mode_info_context->bmi[10].as_mode.first;
            }
          } else if (cm->txfm_mode == ONLY_4X4) {
            rate = r4x4;
            rate_y = tok4x4;
            distortion = d4x4;
            mbmi->txfm_size = TX_4X4;
            tmp_rd = tmp_rd_4x4;
          } else {
            rate = r8x8;
            rate_y = tok8x8;
            distortion = d8x8;
            mbmi->txfm_size = TX_8X8;
            tmp_rd = tmp_rd_8x8;

            mode8x8[0] = xd->mode_info_context->bmi[0].as_mode.first;
            mode8x8[1] = xd->mode_info_context->bmi[2].as_mode.first;
            mode8x8[2] = xd->mode_info_context->bmi[8].as_mode.first;
            mode8x8[3] = xd->mode_info_context->bmi[10].as_mode.first;
          }

          rate2 += rate;
          rate2 += intra_cost_penalty;
          distortion2 += distortion;

          /* TODO: uv rate maybe over-estimated here since there is UV intra
                   mode coded in I8X8_PRED prediction */
          if (tmp_rd < best_yrd) {
            rate2 += uv_intra_rate;
            rate_uv = uv_intra_rate_tokenonly;
            distortion2 += uv_intra_distortion;
            distortion_uv = uv_intra_distortion;
          } else {
            this_rd = INT64_MAX;
            disable_skip = 1;
          }
        }
        break;
      }
    }
    // Split MV. The code is very different from the other inter modes so
    // special case it.
    else if (this_mode == SPLITMV) {
      const int is_comp_pred = mbmi->second_ref_frame > 0;
      int64_t tmp_rd, this_rd_thresh;
      int_mv *second_ref = is_comp_pred ? &second_best_ref_mv : NULL;

      this_rd_thresh =
              (mbmi->ref_frame == LAST_FRAME) ?
          cpi->rd_threshes[THR_NEWMV] : cpi->rd_threshes[THR_NEWA];
      this_rd_thresh =
              (mbmi->ref_frame == GOLDEN_FRAME) ?
          cpi->rd_threshes[THR_NEWG] : this_rd_thresh;

      tmp_rd = rd_pick_best_mbsegmentation(cpi, x, &best_ref_mv,
                                           second_ref, best_yrd, mdcounts,
                                           &rate, &rate_y, &distortion,
                                           &skippable,
                                           (int)this_rd_thresh, seg_mvs,
                                           txfm_cache);
      rate2 += rate;
      distortion2 += distortion;

      if (cpi->common.mcomp_filter_type == SWITCHABLE)
        rate2 += SWITCHABLE_INTERP_RATE_FACTOR * x->switchable_interp_costs
            [vp9_get_pred_context(&cpi->common, xd, PRED_SWITCHABLE_INTERP)]
                [vp9_switchable_interp_map[mbmi->interp_filter]];

      // If even the 'Y' rd value of split is higher than best so far
      // then dont bother looking at UV
      if (tmp_rd < best_yrd) {
        int uv_skippable;

        rd_inter4x4_uv(cpi, x, &rate_uv, &distortion_uv, &uv_skippable,
                       cpi->common.full_pixel);
        rate2 += rate_uv;
        distortion2 += distortion_uv;
        skippable = skippable && uv_skippable;
      } else {
        this_rd = INT64_MAX;
        disable_skip = 1;
      }

      if (!mode_excluded) {
        if (is_comp_pred)
          mode_excluded = cpi->common.comp_pred_mode == SINGLE_PREDICTION_ONLY;
        else
          mode_excluded = cpi->common.comp_pred_mode == COMP_PREDICTION_ONLY;
      }

      compmode_cost =
        vp9_cost_bit(vp9_get_pred_prob(cm, xd, PRED_COMP), is_comp_pred);
      mbmi->mode = this_mode;
    }
    else {
#if CONFIG_COMP_INTERINTRA_PRED
      if (mbmi->second_ref_frame == INTRA_FRAME) {
        if (best_intra16_mode == DC_PRED - 1) continue;
        mbmi->interintra_mode = best_intra16_mode;
#if SEPARATE_INTERINTRA_UV
        mbmi->interintra_uv_mode = best_intra16_uv_mode;
#else
        mbmi->interintra_uv_mode = best_intra16_mode;
#endif
      }
#endif
      this_rd = handle_inter_mode(cpi, x, BLOCK_16X16,
                                  &saddone, near_sadidx, mdcounts, txfm_cache,
                                  &rate2, &distortion2, &skippable,
                                  &compmode_cost,
#if CONFIG_COMP_INTERINTRA_PRED
                                  &compmode_interintra_cost,
#endif
                                  &rate_y, &distortion,
                                  &rate_uv, &distortion_uv,
                                  &mode_excluded, &disable_skip, recon_yoffset,
                                  mode_index, frame_mv);
      if (this_rd == INT64_MAX)
        continue;
    }

#if CONFIG_COMP_INTERINTRA_PRED
    if (cpi->common.use_interintra)
      rate2 += compmode_interintra_cost;
#endif

    if (cpi->common.comp_pred_mode == HYBRID_PREDICTION)
      rate2 += compmode_cost;

    // Estimate the reference frame signaling cost and add it
    // to the rolling cost variable.
    rate2 += ref_costs[mbmi->ref_frame];

    if (!disable_skip) {
      // Test for the condition where skip block will be activated
      // because there are no non zero coefficients and make any
      // necessary adjustment for rate. Ignore if skip is coded at
      // segment level as the cost wont have been added in.
      if (cpi->common.mb_no_coeff_skip) {
        int mb_skip_allowed;

        // Is Mb level skip allowed for this mb.
        mb_skip_allowed =
          !vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB) ||
          vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);

        if (skippable) {
          mbmi->mb_skip_coeff = 1;

          // Back out the coefficient coding costs
          rate2 -= (rate_y + rate_uv);
          // for best_yrd calculation
          rate_uv = 0;

          if (mb_skip_allowed) {
            int prob_skip_cost;

            // Cost the skip mb case
            vp9_prob skip_prob =
              vp9_get_pred_prob(cm, &x->e_mbd, PRED_MBSKIP);

            if (skip_prob) {
              prob_skip_cost = vp9_cost_bit(skip_prob, 1);
              rate2 += prob_skip_cost;
              other_cost += prob_skip_cost;
            }
          }
        }
        // Add in the cost of the no skip flag.
        else {
          mbmi->mb_skip_coeff = 0;
          if (mb_skip_allowed) {
            int prob_skip_cost = vp9_cost_bit(
                   vp9_get_pred_prob(cm, &x->e_mbd, PRED_MBSKIP), 0);
            rate2 += prob_skip_cost;
            other_cost += prob_skip_cost;
          }
        }
      }

      // Calculate the final RD estimate for this mode.
      this_rd = RDCOST(x->rdmult, x->rddiv, rate2, distortion2);
    }

    // Keep record of best intra distortion
    if ((mbmi->ref_frame == INTRA_FRAME) &&
        (this_rd < best_intra_rd)) {
      best_intra_rd = this_rd;
      *returnintra = distortion2;
    }
#if CONFIG_COMP_INTERINTRA_PRED
    if ((mbmi->ref_frame == INTRA_FRAME) &&
        (this_mode <= TM_PRED) &&
        (this_rd < best_intra16_rd)) {
      best_intra16_rd = this_rd;
      best_intra16_mode = this_mode;
      best_intra16_uv_mode = (mbmi->txfm_size != TX_4X4 ?
                              uv_intra_mode_8x8 : uv_intra_mode);
    }
#endif

    if (!disable_skip && mbmi->ref_frame == INTRA_FRAME)
      for (i = 0; i < NB_PREDICTION_TYPES; ++i)
        best_pred_rd[i] = MIN(best_pred_rd[i], this_rd);

    if (this_rd < best_overall_rd) {
      best_overall_rd = this_rd;
      best_filter = mbmi->interp_filter;
      best_mode = this_mode;
#if CONFIG_COMP_INTERINTRA_PRED
      is_best_interintra = (mbmi->second_ref_frame == INTRA_FRAME);
#endif
    }

    // Did this mode help.. i.e. is it the new best mode
    if (this_rd < best_rd || x->skip) {
      if (!mode_excluded) {
        /*
        if (mbmi->second_ref_frame == INTRA_FRAME) {
          printf("rd %d best %d bestintra16 %d\n", this_rd, best_rd, best_intra16_rd);
        }
        */
        // Note index of best mode so far
        best_mode_index = mode_index;

        if (this_mode <= B_PRED) {
          if (mbmi->txfm_size != TX_4X4
              && this_mode != B_PRED
              && this_mode != I8X8_PRED)
            mbmi->uv_mode = uv_intra_mode_8x8;
          else
            mbmi->uv_mode = uv_intra_mode;
          /* required for left and above block mv */
          mbmi->mv[0].as_int = 0;
        }

        other_cost += ref_costs[mbmi->ref_frame];

        /* Calculate the final y RD estimate for this mode */
        best_yrd = RDCOST(x->rdmult, x->rddiv, (rate2 - rate_uv - other_cost),
                          (distortion2 - distortion_uv));

        *returnrate = rate2;
        *returndistortion = distortion2;
        best_rd = this_rd;
        vpx_memcpy(&best_mbmode, mbmi, sizeof(MB_MODE_INFO));
        vpx_memcpy(&best_partition, x->partition_info, sizeof(PARTITION_INFO));

        if ((this_mode == B_PRED)
            || (this_mode == I8X8_PRED)
            || (this_mode == SPLITMV))
          for (i = 0; i < 16; i++) {
            best_bmodes[i] = xd->block[i].bmi;
          }
      }

      // Testing this mode gave rise to an improvement in best error score.
      // Lower threshold a bit for next time
      cpi->rd_thresh_mult[mode_index] =
          (cpi->rd_thresh_mult[mode_index] >= (MIN_THRESHMULT + 2)) ?
          cpi->rd_thresh_mult[mode_index] - 2 : MIN_THRESHMULT;
      cpi->rd_threshes[mode_index] =
          (cpi->rd_baseline_thresh[mode_index] >> 7) *
          cpi->rd_thresh_mult[mode_index];
    } else {
      // If the mode did not help improve the best error case then raise the
      // threshold for testing that mode next time around.
      cpi->rd_thresh_mult[mode_index] += 4;

      if (cpi->rd_thresh_mult[mode_index] > MAX_THRESHMULT)
        cpi->rd_thresh_mult[mode_index] = MAX_THRESHMULT;

      cpi->rd_threshes[mode_index] = (cpi->rd_baseline_thresh[mode_index] >> 7)
          * cpi->rd_thresh_mult[mode_index];
    }

    /* keep record of best compound/single-only prediction */
    if (!disable_skip && mbmi->ref_frame != INTRA_FRAME) {
      int64_t single_rd, hybrid_rd;
      int single_rate, hybrid_rate;

      if (cpi->common.comp_pred_mode == HYBRID_PREDICTION) {
        single_rate = rate2 - compmode_cost;
        hybrid_rate = rate2;
      } else {
        single_rate = rate2;
        hybrid_rate = rate2 + compmode_cost;
      }

      single_rd = RDCOST(x->rdmult, x->rddiv, single_rate, distortion2);
      hybrid_rd = RDCOST(x->rdmult, x->rddiv, hybrid_rate, distortion2);

      if (mbmi->second_ref_frame <= INTRA_FRAME &&
          single_rd < best_pred_rd[SINGLE_PREDICTION_ONLY]) {
        best_pred_rd[SINGLE_PREDICTION_ONLY] = single_rd;
      } else if (mbmi->second_ref_frame > INTRA_FRAME &&
                 single_rd < best_pred_rd[COMP_PREDICTION_ONLY]) {
        best_pred_rd[COMP_PREDICTION_ONLY] = single_rd;
      }
      if (hybrid_rd < best_pred_rd[HYBRID_PREDICTION])
        best_pred_rd[HYBRID_PREDICTION] = hybrid_rd;
    }

    /* keep record of best txfm size */
    if (!mode_excluded && this_rd != INT64_MAX) {
      for (i = 0; i < NB_TXFM_MODES; i++) {
        int64_t adj_rd;
        if (this_mode != B_PRED) {
          const int64_t txfm_mode_diff =
              txfm_cache[i] - txfm_cache[cm->txfm_mode];
          adj_rd = this_rd + txfm_mode_diff;
        } else {
          adj_rd = this_rd;
        }
        if (adj_rd < best_txfm_rd[i])
          best_txfm_rd[i] = adj_rd;
      }
    }

    if (x->skip && !mode_excluded)
      break;
    }

  assert((cm->mcomp_filter_type == SWITCHABLE) ||
         (cm->mcomp_filter_type == best_mbmode.interp_filter) ||
         (best_mbmode.mode <= B_PRED));

#if CONFIG_COMP_INTERINTRA_PRED
  ++cpi->interintra_select_count[is_best_interintra];
#endif

  // Accumulate filter usage stats
  // TODO(agrange): Use RD criteria to select interpolation filter mode.
  if ((best_mode >= NEARESTMV) && (best_mode <= SPLITMV))
    ++cpi->best_switchable_interp_count[vp9_switchable_interp_map[best_filter]];

  // Reduce the activation RD thresholds for the best choice mode
  if ((cpi->rd_baseline_thresh[best_mode_index] > 0) &&
      (cpi->rd_baseline_thresh[best_mode_index] < (INT_MAX >> 2))) {
    int best_adjustment = (cpi->rd_thresh_mult[best_mode_index] >> 2);

    cpi->rd_thresh_mult[best_mode_index] =
        (cpi->rd_thresh_mult[best_mode_index] >=
         (MIN_THRESHMULT + best_adjustment)) ?
        cpi->rd_thresh_mult[best_mode_index] - best_adjustment : MIN_THRESHMULT;
    cpi->rd_threshes[best_mode_index] =
        (cpi->rd_baseline_thresh[best_mode_index] >> 7) *
        cpi->rd_thresh_mult[best_mode_index];
  }

  // This code force Altref,0,0 and skip for the frame that overlays a
  // an alrtef unless Altref is filtered. However, this is unsafe if
  // segment level coding of ref frame or mode is enabled for this
  // segment.
  if (!vp9_segfeature_active(xd, segment_id, SEG_LVL_REF_FRAME) &&
      !vp9_segfeature_active(xd, segment_id, SEG_LVL_MODE) &&
      cpi->is_src_frame_alt_ref &&
      (cpi->oxcf.arnr_max_frames == 0) &&
      (best_mbmode.mode != ZEROMV || best_mbmode.ref_frame != ALTREF_FRAME)) {
    mbmi->mode = ZEROMV;
    if (cm->txfm_mode <= ALLOW_8X8)
      mbmi->txfm_size = cm->txfm_mode;
    else
      mbmi->txfm_size = TX_16X16;
    mbmi->ref_frame = ALTREF_FRAME;
    mbmi->mv[0].as_int = 0;
    mbmi->uv_mode = DC_PRED;
    mbmi->mb_skip_coeff =
      (cpi->common.mb_no_coeff_skip) ? 1 : 0;
    mbmi->partitioning = 0;

    vpx_memset(best_pred_diff, 0, sizeof(best_pred_diff));
    vpx_memset(best_txfm_diff, 0, sizeof(best_txfm_diff));
    goto end;
  }

  // macroblock modes
  vpx_memcpy(mbmi, &best_mbmode, sizeof(MB_MODE_INFO));
  if (best_mbmode.mode == B_PRED) {
    for (i = 0; i < 16; i++) {
      xd->mode_info_context->bmi[i].as_mode = best_bmodes[i].as_mode;
      xd->block[i].bmi.as_mode = xd->mode_info_context->bmi[i].as_mode;
    }
  }

  if (best_mbmode.mode == I8X8_PRED)
    set_i8x8_block_modes(x, mode8x8);

  if (best_mbmode.mode == SPLITMV) {
    for (i = 0; i < 16; i++)
      xd->mode_info_context->bmi[i].as_mv.first.as_int = best_bmodes[i].as_mv.first.as_int;
    if (mbmi->second_ref_frame > 0)
      for (i = 0; i < 16; i++)
        xd->mode_info_context->bmi[i].as_mv.second.as_int = best_bmodes[i].as_mv.second.as_int;

    vpx_memcpy(x->partition_info, &best_partition, sizeof(PARTITION_INFO));

    mbmi->mv[0].as_int = x->partition_info->bmi[15].mv.as_int;
    mbmi->mv[1].as_int = x->partition_info->bmi[15].second_mv.as_int;
  }

  for (i = 0; i < NB_PREDICTION_TYPES; ++i) {
    if (best_pred_rd[i] == INT64_MAX)
      best_pred_diff[i] = INT_MIN;
    else
      best_pred_diff[i] = best_rd - best_pred_rd[i];
  }

  if (!x->skip) {
    for (i = 0; i < NB_TXFM_MODES; i++) {
      if (best_txfm_rd[i] == INT64_MAX)
        best_txfm_diff[i] = INT_MIN;
      else
        best_txfm_diff[i] = best_rd - best_txfm_rd[i];
    }
  } else {
    vpx_memset(best_txfm_diff, 0, sizeof(best_txfm_diff));
  }

end:
  store_coding_context(x, &x->mb_context[xd->sb_index][xd->mb_index],
                       best_mode_index, &best_partition,
                       &mbmi->ref_mvs[mbmi->ref_frame][0],
                       &mbmi->ref_mvs[mbmi->second_ref_frame < 0 ? 0 :
                                      mbmi->second_ref_frame][0],
                       best_pred_diff, best_txfm_diff);
}

void vp9_rd_pick_intra_mode_sb32(VP9_COMP *cpi, MACROBLOCK *x,
                                 int *returnrate,
                                 int *returndist) {
  VP9_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  int rate_y = 0, rate_uv;
  int rate_y_tokenonly = 0, rate_uv_tokenonly;
  int dist_y = 0, dist_uv;
  int y_skip = 0, uv_skip;
  int64_t txfm_cache[NB_TXFM_MODES];

  rd_pick_intra_sby_mode(cpi, x, &rate_y, &rate_y_tokenonly,
                                   &dist_y, &y_skip, txfm_cache);
  rd_pick_intra_sbuv_mode(cpi, x, &rate_uv, &rate_uv_tokenonly,
                                     &dist_uv, &uv_skip);

  if (cpi->common.mb_no_coeff_skip && y_skip && uv_skip) {
    *returnrate = rate_y + rate_uv - rate_y_tokenonly - rate_uv_tokenonly +
                  vp9_cost_bit(vp9_get_pred_prob(cm, xd, PRED_MBSKIP), 1);
    *returndist = dist_y + (dist_uv >> 2);
  } else {
    *returnrate = rate_y + rate_uv;
    if (cpi->common.mb_no_coeff_skip)
      *returnrate += vp9_cost_bit(vp9_get_pred_prob(cm, xd, PRED_MBSKIP), 0);
    *returndist = dist_y + (dist_uv >> 2);
  }
}

void vp9_rd_pick_intra_mode_sb64(VP9_COMP *cpi, MACROBLOCK *x,
                                 int *returnrate,
                                 int *returndist) {
  VP9_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  int rate_y = 0, rate_uv;
  int rate_y_tokenonly = 0, rate_uv_tokenonly;
  int dist_y = 0, dist_uv;
  int y_skip = 0, uv_skip;
  int64_t txfm_cache[NB_TXFM_MODES];

  rd_pick_intra_sb64y_mode(cpi, x, &rate_y, &rate_y_tokenonly,
                                     &dist_y, &y_skip, txfm_cache);
  rd_pick_intra_sb64uv_mode(cpi, x, &rate_uv, &rate_uv_tokenonly,
                                       &dist_uv, &uv_skip);

  if (cpi->common.mb_no_coeff_skip && y_skip && uv_skip) {
    *returnrate = rate_y + rate_uv - rate_y_tokenonly - rate_uv_tokenonly +
    vp9_cost_bit(vp9_get_pred_prob(cm, xd, PRED_MBSKIP), 1);
    *returndist = dist_y + (dist_uv >> 2);
  } else {
    *returnrate = rate_y + rate_uv;
    if (cm->mb_no_coeff_skip)
      *returnrate += vp9_cost_bit(vp9_get_pred_prob(cm, xd, PRED_MBSKIP), 0);
    *returndist = dist_y + (dist_uv >> 2);
  }
}

void vp9_rd_pick_intra_mode(VP9_COMP *cpi, MACROBLOCK *x,
                            int *returnrate, int *returndist) {
  VP9_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO * mbmi = &x->e_mbd.mode_info_context->mbmi;
  int64_t error4x4, error16x16;
  int rate4x4, rate16x16 = 0, rateuv, rateuv8x8;
  int dist4x4 = 0, dist16x16 = 0, distuv = 0, distuv8x8 = 0;
  int rate;
  int rate4x4_tokenonly = 0;
  int rate16x16_tokenonly = 0;
  int rateuv_tokenonly = 0, rateuv8x8_tokenonly = 0;
  int64_t error8x8;
  int rate8x8_tokenonly=0;
  int rate8x8, dist8x8;
  int mode16x16;
  int mode8x8[4];
  int dist;
  int modeuv, uv_intra_skippable, uv_intra_skippable_8x8;
  int y_intra16x16_skippable = 0;
  int64_t txfm_cache[NB_TXFM_MODES];
  TX_SIZE txfm_size_16x16;
  int i;

  mbmi->ref_frame = INTRA_FRAME;
  rd_pick_intra_mbuv_mode(cpi, x, &rateuv, &rateuv_tokenonly, &distuv,
                          &uv_intra_skippable);
  modeuv = mbmi->uv_mode;
  if (cpi->common.txfm_mode != ONLY_4X4) {
    rd_pick_intra_mbuv_mode_8x8(cpi, x, &rateuv8x8, &rateuv8x8_tokenonly,
                                &distuv8x8, &uv_intra_skippable_8x8);
  } else {
    uv_intra_skippable_8x8 = uv_intra_skippable;
    rateuv8x8 = rateuv;
    distuv8x8 = distuv;
    rateuv8x8_tokenonly = rateuv_tokenonly;
  }

  // current macroblock under rate-distortion optimization test loop
  error16x16 = rd_pick_intra16x16mby_mode(cpi, x, &rate16x16,
                                          &rate16x16_tokenonly, &dist16x16,
                                          &y_intra16x16_skippable, txfm_cache);
  mode16x16 = mbmi->mode;
  txfm_size_16x16 = mbmi->txfm_size;

  // FIXME(rbultje) support transform-size selection
  mbmi->txfm_size = (cm->txfm_mode == ONLY_4X4) ? TX_4X4 : TX_8X8;
  error8x8 = rd_pick_intra8x8mby_modes(cpi, x, &rate8x8, &rate8x8_tokenonly,
                                       &dist8x8, error16x16);
  mode8x8[0]= xd->mode_info_context->bmi[0].as_mode.first;
  mode8x8[1]= xd->mode_info_context->bmi[2].as_mode.first;
  mode8x8[2]= xd->mode_info_context->bmi[8].as_mode.first;
  mode8x8[3]= xd->mode_info_context->bmi[10].as_mode.first;

  error4x4 = rd_pick_intra4x4mby_modes(cpi, x,
                                       &rate4x4, &rate4x4_tokenonly,
                                       &dist4x4, error16x16,
                                       cpi->update_context);

  mbmi->mb_skip_coeff = 0;
  if (cpi->common.mb_no_coeff_skip &&
      y_intra16x16_skippable && uv_intra_skippable_8x8) {
    mbmi->mb_skip_coeff = 1;
    mbmi->mode = mode16x16;
    mbmi->uv_mode = modeuv;
    rate = rateuv8x8 + rate16x16 - rateuv8x8_tokenonly - rate16x16_tokenonly +
           vp9_cost_bit(vp9_get_pred_prob(cm, xd, PRED_MBSKIP), 1);
    dist = dist16x16 + (distuv8x8 >> 2);

    mbmi->txfm_size = txfm_size_16x16;
    memset(x->mb_context[xd->sb_index][xd->mb_index].txfm_rd_diff, 0,
           sizeof(x->mb_context[xd->sb_index][xd->mb_index].txfm_rd_diff));
  } else if (error8x8 > error16x16) {
    if (error4x4 < error16x16) {
      rate = rateuv + rate4x4;
      mbmi->mode = B_PRED;
      mbmi->txfm_size = TX_4X4;
      dist = dist4x4 + (distuv >> 2);
      memset(x->mb_context[xd->sb_index][xd->mb_index].txfm_rd_diff, 0,
             sizeof(x->mb_context[xd->sb_index][xd->mb_index].txfm_rd_diff));
    } else {
      mbmi->txfm_size = txfm_size_16x16;
      mbmi->mode = mode16x16;
      rate = rate16x16 + rateuv8x8;
      dist = dist16x16 + (distuv8x8 >> 2);
      for (i = 0; i < NB_TXFM_MODES; i++) {
        x->mb_context[xd->sb_index][xd->mb_index].txfm_rd_diff[i] =
            error16x16 - txfm_cache[i];
      }
    }
    if (cpi->common.mb_no_coeff_skip)
      rate += vp9_cost_bit(vp9_get_pred_prob(cm, xd, PRED_MBSKIP), 0);
  } else {
    if (error4x4 < error8x8) {
      rate = rateuv + rate4x4;
      mbmi->mode = B_PRED;
      mbmi->txfm_size = TX_4X4;
      dist = dist4x4 + (distuv >> 2);
      memset(x->mb_context[xd->sb_index][xd->mb_index].txfm_rd_diff, 0,
             sizeof(x->mb_context[xd->sb_index][xd->mb_index].txfm_rd_diff));
    } else {
      // FIXME(rbultje) support transform-size selection
      mbmi->mode = I8X8_PRED;
      mbmi->txfm_size = (cm->txfm_mode == ONLY_4X4) ? TX_4X4 : TX_8X8;
      set_i8x8_block_modes(x, mode8x8);
      rate = rate8x8 + rateuv;
      dist = dist8x8 + (distuv >> 2);
      memset(x->mb_context[xd->sb_index][xd->mb_index].txfm_rd_diff, 0,
             sizeof(x->mb_context[xd->sb_index][xd->mb_index].txfm_rd_diff));
    }
    if (cpi->common.mb_no_coeff_skip)
      rate += vp9_cost_bit(vp9_get_pred_prob(cm, xd, PRED_MBSKIP), 0);
  }

  *returnrate = rate;
  *returndist = dist;
}

static int64_t vp9_rd_pick_inter_mode_sb(VP9_COMP *cpi, MACROBLOCK *x,
                                         int recon_yoffset, int recon_uvoffset,
                                         int *returnrate,
                                         int *returndistortion,
                                         int block_size) {
  VP9_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = &xd->mode_info_context->mbmi;
  MB_PREDICTION_MODE this_mode;
  MB_PREDICTION_MODE best_mode = DC_PRED;
  MV_REFERENCE_FRAME ref_frame;
  unsigned char segment_id = xd->mode_info_context->mbmi.segment_id;
  int comp_pred, i;
  int_mv frame_mv[MB_MODE_COUNT][MAX_REF_FRAMES];
  int frame_mdcounts[4][4];
  uint8_t *y_buffer[4];
  uint8_t *u_buffer[4];
  uint8_t *v_buffer[4];
  static const int flag_list[4] = { 0, VP9_LAST_FLAG, VP9_GOLD_FLAG,
                                    VP9_ALT_FLAG };
  int idx_list[4] = { 0, cpi->common.lst_fb_idx, cpi->common.gld_fb_idx,
                      cpi->common.alt_fb_idx };
  int mdcounts[4];
  int near_sadidx[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  int saddone = 0;
  int64_t best_rd = INT64_MAX;
  int64_t best_txfm_rd[NB_TXFM_MODES];
  int64_t best_txfm_diff[NB_TXFM_MODES];
  int64_t best_pred_diff[NB_PREDICTION_TYPES];
  int64_t best_pred_rd[NB_PREDICTION_TYPES];
  MB_MODE_INFO best_mbmode;
  int mode_index, best_mode_index = 0;
  unsigned int ref_costs[MAX_REF_FRAMES];
#if CONFIG_COMP_INTERINTRA_PRED
  int is_best_interintra = 0;
  int64_t best_intra16_rd = INT64_MAX;
  int best_intra16_mode = DC_PRED, best_intra16_uv_mode = DC_PRED;
#endif
  int64_t best_overall_rd = INT64_MAX;
  INTERPOLATIONFILTERTYPE best_filter = SWITCHABLE;
  int rate_uv_4x4 = 0, rate_uv_8x8 = 0, rate_uv_tokenonly_4x4 = 0,
      rate_uv_tokenonly_8x8 = 0;
  int dist_uv_4x4 = 0, dist_uv_8x8 = 0, uv_skip_4x4 = 0, uv_skip_8x8 = 0;
  MB_PREDICTION_MODE mode_uv_4x4 = NEARESTMV, mode_uv_8x8 = NEARESTMV;
  int switchable_filter_index = 0;
  int rate_uv_16x16 = 0, rate_uv_tokenonly_16x16 = 0;
  int dist_uv_16x16 = 0, uv_skip_16x16 = 0;
  MB_PREDICTION_MODE mode_uv_16x16 = NEARESTMV;

  x->skip = 0;
  xd->mode_info_context->mbmi.segment_id = segment_id;
  estimate_ref_frame_costs(cpi, segment_id, ref_costs);
  vpx_memset(&best_mbmode, 0, sizeof(best_mbmode));

  for (i = 0; i < NB_PREDICTION_TYPES; ++i)
    best_pred_rd[i] = INT64_MAX;
  for (i = 0; i < NB_TXFM_MODES; i++)
    best_txfm_rd[i] = INT64_MAX;

  for (ref_frame = LAST_FRAME; ref_frame <= ALTREF_FRAME; ref_frame++) {
    if (cpi->ref_frame_flags & flag_list[ref_frame]) {
      setup_buffer_inter(cpi, x, idx_list[ref_frame], ref_frame, block_size,
                         recon_yoffset, recon_uvoffset, frame_mv[NEARESTMV],
                         frame_mv[NEARMV], frame_mdcounts,
                         y_buffer, u_buffer, v_buffer);
    }
    frame_mv[NEWMV][ref_frame].as_int = INVALID_MV;
    frame_mv[ZEROMV][ref_frame].as_int = 0;
  }

  if (block_size == BLOCK_64X64) {
    mbmi->mode = DC_PRED;
    if (cm->txfm_mode == ONLY_4X4 || cm->txfm_mode == TX_MODE_SELECT) {
      mbmi->txfm_size = TX_4X4;
      rd_pick_intra_sb64uv_mode(cpi, x, &rate_uv_4x4, &rate_uv_tokenonly_4x4,
                                &dist_uv_4x4, &uv_skip_4x4);
      mode_uv_4x4 = mbmi->uv_mode;
    }
    if (cm->txfm_mode != ONLY_4X4) {
      mbmi->txfm_size = TX_8X8;
      rd_pick_intra_sb64uv_mode(cpi, x, &rate_uv_8x8, &rate_uv_tokenonly_8x8,
                                &dist_uv_8x8, &uv_skip_8x8);
      mode_uv_8x8 = mbmi->uv_mode;
    }
    if (cm->txfm_mode >= ALLOW_32X32) {
      mbmi->txfm_size = TX_32X32;
      rd_pick_intra_sb64uv_mode(cpi, x, &rate_uv_16x16,
                                &rate_uv_tokenonly_16x16,
                                &dist_uv_16x16, &uv_skip_16x16);
      mode_uv_16x16 = mbmi->uv_mode;
    }
  } else {
    assert(block_size == BLOCK_32X32);
    mbmi->mode = DC_PRED;
    if (cm->txfm_mode == ONLY_4X4 || cm->txfm_mode == TX_MODE_SELECT) {
      mbmi->txfm_size = TX_4X4;
      rd_pick_intra_sbuv_mode(cpi, x, &rate_uv_4x4, &rate_uv_tokenonly_4x4,
                              &dist_uv_4x4, &uv_skip_4x4);
      mode_uv_4x4 = mbmi->uv_mode;
    }
    if (cm->txfm_mode != ONLY_4X4) {
      mbmi->txfm_size = TX_8X8;
      rd_pick_intra_sbuv_mode(cpi, x, &rate_uv_8x8, &rate_uv_tokenonly_8x8,
                              &dist_uv_8x8, &uv_skip_8x8);
      mode_uv_8x8 = mbmi->uv_mode;
    }
    if (cm->txfm_mode >= ALLOW_32X32) {
      mbmi->txfm_size = TX_32X32;
      rd_pick_intra_sbuv_mode(cpi, x, &rate_uv_16x16, &rate_uv_tokenonly_16x16,
                              &dist_uv_16x16, &uv_skip_16x16);
      mode_uv_16x16 = mbmi->uv_mode;
    }
  }

  for (mode_index = 0; mode_index < MAX_MODES;
       mode_index += (!switchable_filter_index)) {
    int mode_excluded = 0;
    int64_t this_rd = INT64_MAX;
    int disable_skip = 0;
    int other_cost = 0;
    int compmode_cost = 0;
    int rate2 = 0, rate_y = 0, rate_uv = 0;
    int distortion2 = 0, distortion_y = 0, distortion_uv = 0;
    int skippable;
    int64_t txfm_cache[NB_TXFM_MODES];
#if CONFIG_COMP_INTERINTRA_PRED
    int compmode_interintra_cost = 0;
#endif

    // Test best rd so far against threshold for trying this mode.
    if (best_rd <= cpi->rd_threshes[mode_index] ||
        cpi->rd_threshes[mode_index] == INT_MAX) {
      switchable_filter_index = 0;
      continue;
    }

    this_mode = vp9_mode_order[mode_index].mode;
    ref_frame = vp9_mode_order[mode_index].ref_frame;
    if (!(ref_frame == INTRA_FRAME ||
          (cpi->ref_frame_flags & flag_list[ref_frame]))) {
      continue;
    }
    mbmi->ref_frame = ref_frame;
    mbmi->second_ref_frame = vp9_mode_order[mode_index].second_ref_frame;
    comp_pred = mbmi->second_ref_frame > INTRA_FRAME;
    mbmi->mode = this_mode;
    mbmi->uv_mode = DC_PRED;
#if CONFIG_COMP_INTERINTRA_PRED
    mbmi->interintra_mode = (MB_PREDICTION_MODE)(DC_PRED - 1);
    mbmi->interintra_uv_mode = (MB_PREDICTION_MODE)(DC_PRED - 1);
#endif
    // Evaluate all sub-pel filters irrespective of whether we can use
    // them for this frame.
    if (this_mode >= NEARESTMV && this_mode <= SPLITMV) {
      mbmi->interp_filter =
          vp9_switchable_interp[switchable_filter_index++];
      if (switchable_filter_index == VP9_SWITCHABLE_FILTERS)
        switchable_filter_index = 0;
      if ((cm->mcomp_filter_type != SWITCHABLE) &&
          (cm->mcomp_filter_type != mbmi->interp_filter)) {
        mode_excluded = 1;
      }
      vp9_setup_interp_filters(xd, mbmi->interp_filter, &cpi->common);
    }

    // if (!(cpi->ref_frame_flags & flag_list[ref_frame]))
    //  continue;

    if (this_mode == I8X8_PRED || this_mode == B_PRED || this_mode == SPLITMV)
      continue;
    //  if (vp9_mode_order[mode_index].second_ref_frame == INTRA_FRAME)
    //  continue;

    if (comp_pred) {
      int second_ref;

      if (ref_frame == ALTREF_FRAME) {
        second_ref = LAST_FRAME;
      } else {
        second_ref = ref_frame + 1;
      }
      if (!(cpi->ref_frame_flags & flag_list[second_ref]))
        continue;
      mbmi->second_ref_frame = second_ref;

      xd->second_pre.y_buffer = y_buffer[second_ref];
      xd->second_pre.u_buffer = u_buffer[second_ref];
      xd->second_pre.v_buffer = v_buffer[second_ref];
      mode_excluded =
          mode_excluded ?
              mode_excluded : cm->comp_pred_mode == SINGLE_PREDICTION_ONLY;
    } else {
      // mbmi->second_ref_frame = vp9_mode_order[mode_index].second_ref_frame;
      if (ref_frame != INTRA_FRAME) {
        if (mbmi->second_ref_frame != INTRA_FRAME)
          mode_excluded =
              mode_excluded ?
                  mode_excluded : cm->comp_pred_mode == COMP_PREDICTION_ONLY;
#if CONFIG_COMP_INTERINTRA_PRED
        else
          mode_excluded = mode_excluded ? mode_excluded : !cm->use_interintra;
#endif
      }
    }

    xd->pre.y_buffer = y_buffer[ref_frame];
    xd->pre.u_buffer = u_buffer[ref_frame];
    xd->pre.v_buffer = v_buffer[ref_frame];
    vpx_memcpy(mdcounts, frame_mdcounts[ref_frame], sizeof(mdcounts));

    // If the segment reference frame feature is enabled....
    // then do nothing if the current ref frame is not allowed..
    if (vp9_segfeature_active(xd, segment_id, SEG_LVL_REF_FRAME) &&
        !vp9_check_segref(xd, segment_id, ref_frame)) {
      continue;
    // If the segment mode feature is enabled....
    // then do nothing if the current mode is not allowed..
    } else if (vp9_segfeature_active(xd, segment_id, SEG_LVL_MODE) &&
               (this_mode != vp9_get_segdata(xd, segment_id, SEG_LVL_MODE))) {
      continue;
    // Disable this drop out case if either the mode or ref frame
    // segment level feature is enabled for this segment. This is to
    // prevent the possibility that we end up unable to pick any mode.
    } else if (!vp9_segfeature_active(xd, segment_id, SEG_LVL_REF_FRAME) &&
               !vp9_segfeature_active(xd, segment_id, SEG_LVL_MODE)) {
      // Only consider ZEROMV/ALTREF_FRAME for alt ref frame,
      // unless ARNR filtering is enabled in which case we want
      // an unfiltered alternative
      if (cpi->is_src_frame_alt_ref && (cpi->oxcf.arnr_max_frames == 0)) {
        if (this_mode != ZEROMV || ref_frame != ALTREF_FRAME) {
          continue;
        }
      }
    }

    if (ref_frame == INTRA_FRAME) {
      if (block_size == BLOCK_64X64) {
        vp9_build_intra_predictors_sb64y_s(xd);
        super_block_64_yrd(cpi, x, &rate_y, &distortion_y,
                           &skippable, txfm_cache);
      } else {
        assert(block_size == BLOCK_32X32);
        vp9_build_intra_predictors_sby_s(xd);
        super_block_yrd(cpi, x, &rate_y, &distortion_y,
                        &skippable, txfm_cache);
      }
      if (mbmi->txfm_size == TX_4X4) {
        rate_uv = rate_uv_4x4;
        distortion_uv = dist_uv_4x4;
        skippable = skippable && uv_skip_4x4;
        mbmi->uv_mode = mode_uv_4x4;
      } else if (mbmi->txfm_size == TX_32X32) {
        rate_uv = rate_uv_16x16;
        distortion_uv = dist_uv_16x16;
        skippable = skippable && uv_skip_16x16;
        mbmi->uv_mode = mode_uv_16x16;
      } else {
        rate_uv = rate_uv_8x8;
        distortion_uv = dist_uv_8x8;
        skippable = skippable && uv_skip_8x8;
        mbmi->uv_mode = mode_uv_8x8;
      }

      rate2 = rate_y + x->mbmode_cost[cm->frame_type][mbmi->mode] + rate_uv;
      distortion2 = distortion_y + distortion_uv;
    } else {
#if CONFIG_COMP_INTERINTRA_PRED
      if (mbmi->second_ref_frame == INTRA_FRAME) {
        if (best_intra16_mode == DC_PRED - 1) continue;
        mbmi->interintra_mode = best_intra16_mode;
#if SEPARATE_INTERINTRA_UV
        mbmi->interintra_uv_mode = best_intra16_uv_mode;
#else
        mbmi->interintra_uv_mode = best_intra16_mode;
#endif
      }
#endif
      this_rd = handle_inter_mode(cpi, x, block_size,
                                  &saddone, near_sadidx, mdcounts, txfm_cache,
                                  &rate2, &distortion2, &skippable,
                                  &compmode_cost,
#if CONFIG_COMP_INTERINTRA_PRED
                                  &compmode_interintra_cost,
#endif
                                  &rate_y, &distortion_y,
                                  &rate_uv, &distortion_uv,
                                  &mode_excluded, &disable_skip, recon_yoffset,
                                  mode_index, frame_mv);
      if (this_rd == INT64_MAX)
        continue;
    }

#if CONFIG_COMP_INTERINTRA_PRED
    if (cpi->common.use_interintra) {
      rate2 += compmode_interintra_cost;
    }
#endif
    if (cpi->common.comp_pred_mode == HYBRID_PREDICTION) {
      rate2 += compmode_cost;
    }

    // Estimate the reference frame signaling cost and add it
    // to the rolling cost variable.
    rate2 += ref_costs[xd->mode_info_context->mbmi.ref_frame];

    if (!disable_skip) {
      // Test for the condition where skip block will be activated
      // because there are no non zero coefficients and make any
      // necessary adjustment for rate. Ignore if skip is coded at
      // segment level as the cost wont have been added in.
      if (cpi->common.mb_no_coeff_skip) {
        int mb_skip_allowed;

        // Is Mb level skip allowed for this mb.
        mb_skip_allowed =
          !vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB) ||
          vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);

        if (skippable) {
          // Back out the coefficient coding costs
          rate2 -= (rate_y + rate_uv);
          // for best_yrd calculation
          rate_uv = 0;

          if (mb_skip_allowed) {
            int prob_skip_cost;

            // Cost the skip mb case
            vp9_prob skip_prob =
              vp9_get_pred_prob(cm, xd, PRED_MBSKIP);

            if (skip_prob) {
              prob_skip_cost = vp9_cost_bit(skip_prob, 1);
              rate2 += prob_skip_cost;
              other_cost += prob_skip_cost;
            }
          }
        }
        // Add in the cost of the no skip flag.
        else if (mb_skip_allowed) {
          int prob_skip_cost = vp9_cost_bit(vp9_get_pred_prob(cm, xd,
                                                          PRED_MBSKIP), 0);
          rate2 += prob_skip_cost;
          other_cost += prob_skip_cost;
        }
      }

      // Calculate the final RD estimate for this mode.
      this_rd = RDCOST(x->rdmult, x->rddiv, rate2, distortion2);
    }

#if 0
    // Keep record of best intra distortion
    if ((xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME) &&
        (this_rd < best_intra_rd)) {
      best_intra_rd = this_rd;
      *returnintra = distortion2;
    }
#endif
#if CONFIG_COMP_INTERINTRA_PRED
    if ((mbmi->ref_frame == INTRA_FRAME) &&
        (this_mode <= TM_PRED) &&
        (this_rd < best_intra16_rd)) {
      best_intra16_rd = this_rd;
      best_intra16_mode = this_mode;
      best_intra16_uv_mode = (mbmi->txfm_size != TX_4X4 ?
                              mode_uv_8x8 : mode_uv_4x4);
    }
#endif

    if (!disable_skip && mbmi->ref_frame == INTRA_FRAME)
      for (i = 0; i < NB_PREDICTION_TYPES; ++i)
        best_pred_rd[i] = MIN(best_pred_rd[i], this_rd);

    if (this_rd < best_overall_rd) {
      best_overall_rd = this_rd;
      best_filter = mbmi->interp_filter;
      best_mode = this_mode;
#if CONFIG_COMP_INTERINTRA_PRED
      is_best_interintra = (mbmi->second_ref_frame == INTRA_FRAME);
#endif
    }

    // Did this mode help.. i.e. is it the new best mode
    if (this_rd < best_rd || x->skip) {
      if (!mode_excluded) {
        // Note index of best mode so far
        best_mode_index = mode_index;

        if (this_mode <= B_PRED) {
          /* required for left and above block mv */
          mbmi->mv[0].as_int = 0;
        }

        other_cost += ref_costs[xd->mode_info_context->mbmi.ref_frame];
        *returnrate = rate2;
        *returndistortion = distortion2;
        best_rd = this_rd;
        vpx_memcpy(&best_mbmode, mbmi, sizeof(MB_MODE_INFO));
      }
#if 0
      // Testing this mode gave rise to an improvement in best error score.
      // Lower threshold a bit for next time
      cpi->rd_thresh_mult[mode_index] =
          (cpi->rd_thresh_mult[mode_index] >= (MIN_THRESHMULT + 2)) ?
              cpi->rd_thresh_mult[mode_index] - 2 : MIN_THRESHMULT;
      cpi->rd_threshes[mode_index] =
          (cpi->rd_baseline_thresh[mode_index] >> 7)
              * cpi->rd_thresh_mult[mode_index];
#endif
    } else {
      // If the mode did not help improve the best error case then
      // raise the threshold for testing that mode next time around.
#if 0
      cpi->rd_thresh_mult[mode_index] += 4;

      if (cpi->rd_thresh_mult[mode_index] > MAX_THRESHMULT)
        cpi->rd_thresh_mult[mode_index] = MAX_THRESHMULT;

      cpi->rd_threshes[mode_index] =
          (cpi->rd_baseline_thresh[mode_index] >> 7)
              * cpi->rd_thresh_mult[mode_index];
#endif
    }

    /* keep record of best compound/single-only prediction */
    if (!disable_skip && mbmi->ref_frame != INTRA_FRAME) {
      int single_rd, hybrid_rd, single_rate, hybrid_rate;

      if (cpi->common.comp_pred_mode == HYBRID_PREDICTION) {
        single_rate = rate2 - compmode_cost;
        hybrid_rate = rate2;
      } else {
        single_rate = rate2;
        hybrid_rate = rate2 + compmode_cost;
      }

      single_rd = RDCOST(x->rdmult, x->rddiv, single_rate, distortion2);
      hybrid_rd = RDCOST(x->rdmult, x->rddiv, hybrid_rate, distortion2);

      if (mbmi->second_ref_frame <= INTRA_FRAME &&
          single_rd < best_pred_rd[SINGLE_PREDICTION_ONLY]) {
        best_pred_rd[SINGLE_PREDICTION_ONLY] = single_rd;
      } else if (mbmi->second_ref_frame > INTRA_FRAME &&
                 single_rd < best_pred_rd[COMP_PREDICTION_ONLY]) {
        best_pred_rd[COMP_PREDICTION_ONLY] = single_rd;
      }
      if (hybrid_rd < best_pred_rd[HYBRID_PREDICTION])
        best_pred_rd[HYBRID_PREDICTION] = hybrid_rd;
    }

    /* keep record of best txfm size */
    if (!mode_excluded && this_rd != INT64_MAX) {
      for (i = 0; i < NB_TXFM_MODES; i++) {
        int64_t adj_rd;
        if (this_mode != B_PRED) {
          adj_rd = this_rd + txfm_cache[i] - txfm_cache[cm->txfm_mode];
        } else {
          adj_rd = this_rd;
        }
        if (adj_rd < best_txfm_rd[i])
          best_txfm_rd[i] = adj_rd;
      }
    }

    if (x->skip && !mode_excluded)
      break;
  }

  assert((cm->mcomp_filter_type == SWITCHABLE) ||
         (cm->mcomp_filter_type == best_mbmode.interp_filter) ||
         (best_mbmode.mode <= B_PRED));

#if CONFIG_COMP_INTERINTRA_PRED
  ++cpi->interintra_select_count[is_best_interintra];
  // if (is_best_interintra)  printf("best_interintra\n");
#endif

  // Accumulate filter usage stats
  // TODO(agrange): Use RD criteria to select interpolation filter mode.
  if ((best_mode >= NEARESTMV) && (best_mode <= SPLITMV))
    ++cpi->best_switchable_interp_count[vp9_switchable_interp_map[best_filter]];

  // TODO(rbultje) integrate with RD thresholding
#if 0
  // Reduce the activation RD thresholds for the best choice mode
  if ((cpi->rd_baseline_thresh[best_mode_index] > 0) &&
      (cpi->rd_baseline_thresh[best_mode_index] < (INT_MAX >> 2))) {
    int best_adjustment = (cpi->rd_thresh_mult[best_mode_index] >> 2);

    cpi->rd_thresh_mult[best_mode_index] =
      (cpi->rd_thresh_mult[best_mode_index] >= (MIN_THRESHMULT + best_adjustment)) ?
      cpi->rd_thresh_mult[best_mode_index] - best_adjustment : MIN_THRESHMULT;
    cpi->rd_threshes[best_mode_index] =
      (cpi->rd_baseline_thresh[best_mode_index] >> 7) * cpi->rd_thresh_mult[best_mode_index];
  }
#endif

  // This code forces Altref,0,0 and skip for the frame that overlays a
  // an alrtef unless Altref is filtered. However, this is unsafe if
  // segment level coding of ref frame or mode is enabled for this
  // segment.
  if (!vp9_segfeature_active(xd, segment_id, SEG_LVL_REF_FRAME) &&
      !vp9_segfeature_active(xd, segment_id, SEG_LVL_MODE) &&
      cpi->is_src_frame_alt_ref &&
      (cpi->oxcf.arnr_max_frames == 0) &&
      (best_mbmode.mode != ZEROMV || best_mbmode.ref_frame != ALTREF_FRAME)) {
    mbmi->mode = ZEROMV;
    mbmi->ref_frame = ALTREF_FRAME;
    mbmi->second_ref_frame = INTRA_FRAME;
    mbmi->mv[0].as_int = 0;
    mbmi->uv_mode = DC_PRED;
    mbmi->mb_skip_coeff = (cpi->common.mb_no_coeff_skip) ? 1 : 0;
    mbmi->partitioning = 0;
    mbmi->txfm_size = cm->txfm_mode == TX_MODE_SELECT ?
                      TX_16X16 : cm->txfm_mode;

    vpx_memset(best_txfm_diff, 0, sizeof(best_txfm_diff));
    vpx_memset(best_pred_diff, 0, sizeof(best_pred_diff));
    goto end;
  }

  // macroblock modes
  vpx_memcpy(mbmi, &best_mbmode, sizeof(MB_MODE_INFO));

  for (i = 0; i < NB_PREDICTION_TYPES; ++i) {
    if (best_pred_rd[i] == INT64_MAX)
      best_pred_diff[i] = INT_MIN;
    else
      best_pred_diff[i] = best_rd - best_pred_rd[i];
  }

  if (!x->skip) {
    for (i = 0; i < NB_TXFM_MODES; i++) {
      if (best_txfm_rd[i] == INT64_MAX)
        best_txfm_diff[i] = INT_MIN;
      else
        best_txfm_diff[i] = best_rd - best_txfm_rd[i];
    }
  } else {
    vpx_memset(best_txfm_diff, 0, sizeof(best_txfm_diff));
  }

 end:
  {
    PICK_MODE_CONTEXT *p = (block_size == BLOCK_32X32) ?
                            &x->sb32_context[xd->sb_index] :
                            &x->sb64_context;
    store_coding_context(x, p, best_mode_index, NULL,
                         &mbmi->ref_mvs[mbmi->ref_frame][0],
                         &mbmi->ref_mvs[mbmi->second_ref_frame < 0 ? 0 :
                             mbmi->second_ref_frame][0],
                         best_pred_diff, best_txfm_diff);
  }

  return best_rd;
}

int64_t vp9_rd_pick_inter_mode_sb32(VP9_COMP *cpi, MACROBLOCK *x,
                                    int recon_yoffset, int recon_uvoffset,
                                    int *returnrate,
                                    int *returndistortion) {
  return vp9_rd_pick_inter_mode_sb(cpi, x, recon_yoffset, recon_uvoffset,
                                   returnrate, returndistortion, BLOCK_32X32);
}

int64_t vp9_rd_pick_inter_mode_sb64(VP9_COMP *cpi, MACROBLOCK *x,
                                    int recon_yoffset, int recon_uvoffset,
                                    int *returnrate,
                                    int *returndistortion) {
  return vp9_rd_pick_inter_mode_sb(cpi, x, recon_yoffset, recon_uvoffset,
                                   returnrate, returndistortion, BLOCK_64X64);
}

void vp9_pick_mode_inter_macroblock(VP9_COMP *cpi, MACROBLOCK *x,
                                    int recon_yoffset,
                                    int recon_uvoffset,
                                    int *totalrate, int *totaldist) {
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_MODE_INFO * mbmi = &x->e_mbd.mode_info_context->mbmi;
  int rate, distortion;
  int64_t intra_error = 0;
  unsigned char *segment_id = &mbmi->segment_id;

  if (xd->segmentation_enabled)
    x->encode_breakout = cpi->segment_encode_breakout[*segment_id];
  else
    x->encode_breakout = cpi->oxcf.encode_breakout;

  // if (cpi->sf.RD)
  // For now this codebase is limited to a single rd encode path
  {
    int zbin_mode_boost_enabled = cpi->zbin_mode_boost_enabled;

    rd_pick_inter_mode(cpi, x, recon_yoffset, recon_uvoffset, &rate,
                       &distortion, &intra_error);

    /* restore cpi->zbin_mode_boost_enabled */
    cpi->zbin_mode_boost_enabled = zbin_mode_boost_enabled;
  }
  // else
  // The non rd encode path has been deleted from this code base
  // to simplify development
  //    vp9_pick_inter_mode

  // Store metrics so they can be added in to totals if this mode is picked
  x->mb_context[xd->sb_index][xd->mb_index].distortion  = distortion;
  x->mb_context[xd->sb_index][xd->mb_index].intra_error = intra_error;

  *totalrate = rate;
  *totaldist = distortion;
}
