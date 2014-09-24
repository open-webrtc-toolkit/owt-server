/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/decoder/vp9_onyxd_int.h"
#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_header.h"
#include "vp9/common/vp9_reconintra.h"
#include "vp9/common/vp9_reconintra4x4.h"
#include "vp9/common/vp9_reconinter.h"
#include "vp9/common/vp9_entropy.h"
#include "vp9/decoder/vp9_decodframe.h"
#include "vp9/decoder/vp9_detokenize.h"
#include "vp9/common/vp9_invtrans.h"
#include "vp9/common/vp9_alloccommon.h"
#include "vp9/common/vp9_entropymode.h"
#include "vp9/common/vp9_quant_common.h"
#include "vpx_scale/vpx_scale.h"
#include "vp9/common/vp9_setupintrarecon.h"

#include "vp9/decoder/vp9_decodemv.h"
#include "vp9/common/vp9_extend.h"
#include "vp9/common/vp9_modecont.h"
#include "vpx_mem/vpx_mem.h"
#include "vp9/decoder/vp9_dboolhuff.h"

#include "vp9/common/vp9_seg_common.h"
#include "vp9/common/vp9_entropy.h"
#include "vp9_rtcd.h"

#include <assert.h>
#include <stdio.h>

#define COEFCOUNT_TESTING

//#define DEC_DEBUG
#ifdef DEC_DEBUG
int dec_debug = 0;
#endif

static int merge_index(int v, int n, int modulus) {
  int max1 = (n - 1 - modulus / 2) / modulus + 1;
  if (v < max1) v = v * modulus + modulus / 2;
  else {
    int w;
    v -= max1;
    w = v;
    v += (v + modulus - modulus / 2) / modulus;
    while (v % modulus == modulus / 2 ||
           w != v - (v + modulus - modulus / 2) / modulus) v++;
  }
  return v;
}

static int inv_remap_prob(int v, int m) {
  const int n = 256;
  const int modulus = MODULUS_PARAM;
  int i;
  v = merge_index(v, n - 1, modulus);
  if ((m << 1) <= n) {
    i = vp9_inv_recenter_nonneg(v + 1, m);
  } else {
    i = n - 1 - vp9_inv_recenter_nonneg(v + 1, n - 1 - m);
  }
  return i;
}

static vp9_prob read_prob_diff_update(vp9_reader *const bc, int oldp) {
  int delp = vp9_decode_term_subexp(bc, SUBEXP_PARAM, 255);
  return (vp9_prob)inv_remap_prob(delp, oldp);
}

void vp9_init_de_quantizer(VP9D_COMP *pbi) {
  int i;
  int Q;
  VP9_COMMON *const pc = &pbi->common;

  for (Q = 0; Q < QINDEX_RANGE; Q++) {
    pc->Y1dequant[Q][0] = (int16_t)vp9_dc_quant(Q, pc->y1dc_delta_q);
    pc->Y2dequant[Q][0] = (int16_t)vp9_dc2quant(Q, pc->y2dc_delta_q);
    pc->UVdequant[Q][0] = (int16_t)vp9_dc_uv_quant(Q, pc->uvdc_delta_q);

    /* all the ac values =; */
    for (i = 1; i < 16; i++) {
      int rc = vp9_default_zig_zag1d_4x4[i];

      pc->Y1dequant[Q][rc] = (int16_t)vp9_ac_yquant(Q);
      pc->Y2dequant[Q][rc] = (int16_t)vp9_ac2quant(Q, pc->y2ac_delta_q);
      pc->UVdequant[Q][rc] = (int16_t)vp9_ac_uv_quant(Q, pc->uvac_delta_q);
    }
  }
}

static void mb_init_dequantizer(VP9D_COMP *pbi, MACROBLOCKD *xd) {
  int i;
  int QIndex;
  VP9_COMMON *const pc = &pbi->common;
  int segment_id = xd->mode_info_context->mbmi.segment_id;

  // Set the Q baseline allowing for any segment level adjustment
  if (vp9_segfeature_active(xd, segment_id, SEG_LVL_ALT_Q)) {
    /* Abs Value */
    if (xd->mb_segment_abs_delta == SEGMENT_ABSDATA)
      QIndex = vp9_get_segdata(xd, segment_id, SEG_LVL_ALT_Q);

    /* Delta Value */
    else {
      QIndex = pc->base_qindex +
               vp9_get_segdata(xd, segment_id, SEG_LVL_ALT_Q);
      QIndex = (QIndex >= 0) ? ((QIndex <= MAXQ) ? QIndex : MAXQ) : 0;    /* Clamp to valid range */
    }
  } else
    QIndex = pc->base_qindex;
  xd->q_index = QIndex;

  /* Set up the block level dequant pointers */
  for (i = 0; i < 16; i++) {
    xd->block[i].dequant = pc->Y1dequant[QIndex];
  }

#if CONFIG_LOSSLESS
  if (!QIndex) {
    pbi->mb.inv_xform4x4_1_x8     = vp9_short_inv_walsh4x4_1_x8;
    pbi->mb.inv_xform4x4_x8       = vp9_short_inv_walsh4x4_x8;
    pbi->mb.inv_walsh4x4_1        = vp9_short_inv_walsh4x4_1_lossless;
    pbi->mb.inv_walsh4x4_lossless = vp9_short_inv_walsh4x4_lossless;
    pbi->idct_add            = vp9_dequant_idct_add_lossless_c;
    pbi->dc_idct_add         = vp9_dequant_dc_idct_add_lossless_c;
    pbi->dc_idct_add_y_block = vp9_dequant_dc_idct_add_y_block_lossless_c;
    pbi->idct_add_y_block    = vp9_dequant_idct_add_y_block_lossless_c;
    pbi->idct_add_uv_block   = vp9_dequant_idct_add_uv_block_lossless_c;
  } else {
    pbi->mb.inv_xform4x4_1_x8     = vp9_short_idct4x4llm_1;
    pbi->mb.inv_xform4x4_x8       = vp9_short_idct4x4llm;
    pbi->mb.inv_walsh4x4_1        = vp9_short_inv_walsh4x4_1;
    pbi->mb.inv_walsh4x4_lossless = vp9_short_inv_walsh4x4;
    pbi->idct_add            = vp9_dequant_idct_add;
    pbi->dc_idct_add         = vp9_dequant_dc_idct_add;
    pbi->dc_idct_add_y_block = vp9_dequant_dc_idct_add_y_block;
    pbi->idct_add_y_block    = vp9_dequant_idct_add_y_block;
    pbi->idct_add_uv_block   = vp9_dequant_idct_add_uv_block;
  }
#else
  pbi->mb.inv_xform4x4_1_x8     = vp9_short_idct4x4llm_1;
  pbi->mb.inv_xform4x4_x8       = vp9_short_idct4x4llm;
  pbi->mb.inv_walsh4x4_1        = vp9_short_inv_walsh4x4_1;
  pbi->mb.inv_walsh4x4_lossless = vp9_short_inv_walsh4x4;
  pbi->idct_add            = vp9_dequant_idct_add;
  pbi->dc_idct_add         = vp9_dequant_dc_idct_add;
  pbi->dc_idct_add_y_block = vp9_dequant_dc_idct_add_y_block;
  pbi->idct_add_y_block    = vp9_dequant_idct_add_y_block;
  pbi->idct_add_uv_block   = vp9_dequant_idct_add_uv_block;
#endif

  for (i = 16; i < 24; i++) {
    xd->block[i].dequant = pc->UVdequant[QIndex];
  }

  xd->block[24].dequant = pc->Y2dequant[QIndex];

}

/* skip_recon_mb() is Modified: Instead of writing the result to predictor buffer and then copying it
 *  to dst buffer, we can write the result directly to dst buffer. This eliminates unnecessary copy.
 */
static void skip_recon_mb(VP9D_COMP *pbi, MACROBLOCKD *xd) {
  if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME) {
    if (xd->mode_info_context->mbmi.sb_type == BLOCK_SIZE_SB64X64) {
      vp9_build_intra_predictors_sb64uv_s(xd);
      vp9_build_intra_predictors_sb64y_s(xd);
    } else if (xd->mode_info_context->mbmi.sb_type == BLOCK_SIZE_SB32X32) {
      vp9_build_intra_predictors_sbuv_s(xd);
      vp9_build_intra_predictors_sby_s(xd);
    } else {
      vp9_build_intra_predictors_mbuv_s(xd);
      vp9_build_intra_predictors_mby_s(xd);
    }
  } else {
    if (xd->mode_info_context->mbmi.sb_type == BLOCK_SIZE_SB64X64) {
      vp9_build_inter64x64_predictors_sb(xd,
                                         xd->dst.y_buffer,
                                         xd->dst.u_buffer,
                                         xd->dst.v_buffer,
                                         xd->dst.y_stride,
                                         xd->dst.uv_stride);
    } else if (xd->mode_info_context->mbmi.sb_type == BLOCK_SIZE_SB32X32) {
      vp9_build_inter32x32_predictors_sb(xd,
                                         xd->dst.y_buffer,
                                         xd->dst.u_buffer,
                                         xd->dst.v_buffer,
                                         xd->dst.y_stride,
                                         xd->dst.uv_stride);
    } else {
      vp9_build_1st_inter16x16_predictors_mb(xd,
                                             xd->dst.y_buffer,
                                             xd->dst.u_buffer,
                                             xd->dst.v_buffer,
                                             xd->dst.y_stride,
                                             xd->dst.uv_stride);

      if (xd->mode_info_context->mbmi.second_ref_frame > 0) {
        vp9_build_2nd_inter16x16_predictors_mb(xd,
                                               xd->dst.y_buffer,
                                               xd->dst.u_buffer,
                                               xd->dst.v_buffer,
                                               xd->dst.y_stride,
                                               xd->dst.uv_stride);
      }
#if CONFIG_COMP_INTERINTRA_PRED
      else if (xd->mode_info_context->mbmi.second_ref_frame == INTRA_FRAME) {
        vp9_build_interintra_16x16_predictors_mb(xd,
                                                 xd->dst.y_buffer,
                                                 xd->dst.u_buffer,
                                                 xd->dst.v_buffer,
                                                 xd->dst.y_stride,
                                                 xd->dst.uv_stride);
      }
#endif
    }
  }
}

static void decode_16x16(VP9D_COMP *pbi, MACROBLOCKD *xd,
                         BOOL_DECODER* const bc) {
  BLOCKD *bd = &xd->block[0];
  TX_TYPE tx_type = get_tx_type_16x16(xd, bd);
  assert(get_2nd_order_usage(xd) == 0);
#ifdef DEC_DEBUG
  if (dec_debug) {
    int i;
    printf("\n");
    printf("qcoeff 16x16\n");
    for (i = 0; i < 400; i++) {
      printf("%3d ", xd->qcoeff[i]);
      if (i % 16 == 15) printf("\n");
    }
    printf("\n");
    printf("predictor\n");
    for (i = 0; i < 400; i++) {
      printf("%3d ", xd->predictor[i]);
      if (i % 16 == 15) printf("\n");
    }
  }
#endif
  if (tx_type != DCT_DCT) {
    vp9_ht_dequant_idct_add_16x16_c(tx_type, xd->qcoeff,
                                    xd->block[0].dequant, xd->predictor,
                                    xd->dst.y_buffer, 16, xd->dst.y_stride,
                                    xd->eobs[0]);
  } else {
    vp9_dequant_idct_add_16x16(xd->qcoeff, xd->block[0].dequant,
                               xd->predictor, xd->dst.y_buffer,
                               16, xd->dst.y_stride, xd->eobs[0]);
  }
  vp9_dequant_idct_add_uv_block_8x8(
      xd->qcoeff + 16 * 16, xd->block[16].dequant,
      xd->predictor + 16 * 16, xd->dst.u_buffer, xd->dst.v_buffer,
      xd->dst.uv_stride, xd->eobs + 16, xd);
}

static void decode_8x8(VP9D_COMP *pbi, MACROBLOCKD *xd,
                       BOOL_DECODER* const bc) {
  // First do Y
  // if the first one is DCT_DCT assume all the rest are as well
  TX_TYPE tx_type = get_tx_type_8x8(xd, &xd->block[0]);
#ifdef DEC_DEBUG
  if (dec_debug) {
    int i;
    printf("\n");
    printf("qcoeff 8x8\n");
    for (i = 0; i < 400; i++) {
      printf("%3d ", xd->qcoeff[i]);
      if (i % 16 == 15) printf("\n");
    }
  }
#endif
  if (tx_type != DCT_DCT || xd->mode_info_context->mbmi.mode == I8X8_PRED) {
    int i;
    assert(get_2nd_order_usage(xd) == 0);
    for (i = 0; i < 4; i++) {
      int ib = vp9_i8x8_block[i];
      int idx = (ib & 0x02) ? (ib + 2) : ib;
      int16_t *q  = xd->block[idx].qcoeff;
      int16_t *dq = xd->block[0].dequant;
      uint8_t *pre = xd->block[ib].predictor;
      uint8_t *dst = *(xd->block[ib].base_dst) + xd->block[ib].dst;
      int stride = xd->dst.y_stride;
      BLOCKD *b = &xd->block[ib];
      if (xd->mode_info_context->mbmi.mode == I8X8_PRED) {
        int i8x8mode = b->bmi.as_mode.first;
        vp9_intra8x8_predict(b, i8x8mode, b->predictor);
      }
      tx_type = get_tx_type_8x8(xd, &xd->block[ib]);
      if (tx_type != DCT_DCT) {
        vp9_ht_dequant_idct_add_8x8_c(tx_type, q, dq, pre, dst, 16, stride,
                                      xd->eobs[idx]);
      } else {
        vp9_dequant_idct_add_8x8_c(q, dq, pre, dst, 16, stride,
                                   0, xd->eobs[idx]);
      }
    }
  } else if (xd->mode_info_context->mbmi.mode == SPLITMV) {
    assert(get_2nd_order_usage(xd) == 0);
    vp9_dequant_idct_add_y_block_8x8(xd->qcoeff,
                                     xd->block[0].dequant,
                                     xd->predictor,
                                     xd->dst.y_buffer,
                                     xd->dst.y_stride,
                                     xd->eobs, xd);
  } else {
    BLOCKD *b = &xd->block[24];
    assert(get_2nd_order_usage(xd) == 1);
    vp9_dequantize_b_2x2(b);
    vp9_short_ihaar2x2(&b->dqcoeff[0], b->diff, 8);
    ((int *)b->qcoeff)[0] = 0;  // 2nd order block are set to 0 after idct
    ((int *)b->qcoeff)[1] = 0;
    ((int *)b->qcoeff)[2] = 0;
    ((int *)b->qcoeff)[3] = 0;
    ((int *)b->qcoeff)[4] = 0;
    ((int *)b->qcoeff)[5] = 0;
    ((int *)b->qcoeff)[6] = 0;
    ((int *)b->qcoeff)[7] = 0;
    vp9_dequant_dc_idct_add_y_block_8x8(xd->qcoeff,
                                        xd->block[0].dequant,
                                        xd->predictor,
                                        xd->dst.y_buffer,
                                        xd->dst.y_stride,
                                        xd->eobs,
                                        xd->block[24].diff,
                                        xd);
  }

  // Now do UV
  if (xd->mode_info_context->mbmi.mode == I8X8_PRED) {
    int i;
    for (i = 0; i < 4; i++) {
      int ib = vp9_i8x8_block[i];
      BLOCKD *b = &xd->block[ib];
      int i8x8mode = b->bmi.as_mode.first;
      b = &xd->block[16 + i];
      vp9_intra_uv4x4_predict(&xd->block[16 + i], i8x8mode, b->predictor);
      pbi->idct_add(b->qcoeff, b->dequant, b->predictor,
                    *(b->base_dst) + b->dst, 8, b->dst_stride);
      b = &xd->block[20 + i];
      vp9_intra_uv4x4_predict(&xd->block[20 + i], i8x8mode, b->predictor);
      pbi->idct_add(b->qcoeff, b->dequant, b->predictor,
                    *(b->base_dst) + b->dst, 8, b->dst_stride);
    }
  } else if (xd->mode_info_context->mbmi.mode == SPLITMV) {
    pbi->idct_add_uv_block(xd->qcoeff + 16 * 16, xd->block[16].dequant,
         xd->predictor + 16 * 16, xd->dst.u_buffer, xd->dst.v_buffer,
         xd->dst.uv_stride, xd->eobs + 16);
  } else {
    vp9_dequant_idct_add_uv_block_8x8
        (xd->qcoeff + 16 * 16, xd->block[16].dequant,
         xd->predictor + 16 * 16, xd->dst.u_buffer, xd->dst.v_buffer,
         xd->dst.uv_stride, xd->eobs + 16, xd);
  }
#ifdef DEC_DEBUG
  if (dec_debug) {
    int i;
    printf("\n");
    printf("predictor\n");
    for (i = 0; i < 384; i++) {
      printf("%3d ", xd->predictor[i]);
      if (i % 16 == 15) printf("\n");
    }
  }
#endif
}

static void decode_4x4(VP9D_COMP *pbi, MACROBLOCKD *xd,
                       BOOL_DECODER* const bc) {
  TX_TYPE tx_type;
  int i, eobtotal = 0;
  MB_PREDICTION_MODE mode = xd->mode_info_context->mbmi.mode;
  if (mode == I8X8_PRED) {
    assert(get_2nd_order_usage(xd) == 0);
    for (i = 0; i < 4; i++) {
      int ib = vp9_i8x8_block[i];
      const int iblock[4] = {0, 1, 4, 5};
      int j;
      int i8x8mode;
      BLOCKD *b;
      b = &xd->block[ib];
      i8x8mode = b->bmi.as_mode.first;
      vp9_intra8x8_predict(b, i8x8mode, b->predictor);
      for (j = 0; j < 4; j++) {
        b = &xd->block[ib + iblock[j]];
        tx_type = get_tx_type_4x4(xd, b);
        if (tx_type != DCT_DCT) {
          vp9_ht_dequant_idct_add_c(tx_type, b->qcoeff,
                                    b->dequant, b->predictor,
                                    *(b->base_dst) + b->dst, 16,
                                    b->dst_stride, b->eob);
        } else {
          vp9_dequant_idct_add(b->qcoeff, b->dequant, b->predictor,
                               *(b->base_dst) + b->dst, 16, b->dst_stride);
        }
      }
      b = &xd->block[16 + i];
      vp9_intra_uv4x4_predict(b, i8x8mode, b->predictor);
      pbi->idct_add(b->qcoeff, b->dequant, b->predictor,
                    *(b->base_dst) + b->dst, 8, b->dst_stride);
      b = &xd->block[20 + i];
      vp9_intra_uv4x4_predict(b, i8x8mode, b->predictor);
      pbi->idct_add(b->qcoeff, b->dequant, b->predictor,
                    *(b->base_dst) + b->dst, 8, b->dst_stride);
    }
  } else if (mode == B_PRED) {
    assert(get_2nd_order_usage(xd) == 0);
    for (i = 0; i < 16; i++) {
      int b_mode;
      BLOCKD *b = &xd->block[i];
      b_mode = xd->mode_info_context->bmi[i].as_mode.first;
#if CONFIG_NEWBINTRAMODES
      xd->mode_info_context->bmi[i].as_mode.context = b->bmi.as_mode.context =
          vp9_find_bpred_context(b);
#endif
      if (!xd->mode_info_context->mbmi.mb_skip_coeff)
        eobtotal += vp9_decode_coefs_4x4(pbi, xd, bc, PLANE_TYPE_Y_WITH_DC, i);

      vp9_intra4x4_predict(b, b_mode, b->predictor);
      tx_type = get_tx_type_4x4(xd, b);
      if (tx_type != DCT_DCT) {
        vp9_ht_dequant_idct_add_c(tx_type, b->qcoeff,
                                  b->dequant, b->predictor,
                                  *(b->base_dst) + b->dst, 16, b->dst_stride,
                                  b->eob);
      } else {
        vp9_dequant_idct_add(b->qcoeff, b->dequant, b->predictor,
                             *(b->base_dst) + b->dst, 16, b->dst_stride);
      }
    }
    if (!xd->mode_info_context->mbmi.mb_skip_coeff) {
      vp9_decode_mb_tokens_4x4_uv(pbi, xd, bc);
    }
    xd->above_context->y2 = 0;
    xd->left_context->y2 = 0;
    vp9_build_intra_predictors_mbuv(xd);
    pbi->idct_add_uv_block(xd->qcoeff + 16 * 16,
                           xd->block[16].dequant,
                           xd->predictor + 16 * 16,
                           xd->dst.u_buffer,
                           xd->dst.v_buffer,
                           xd->dst.uv_stride,
                           xd->eobs + 16);
  } else if (mode == SPLITMV) {
    assert(get_2nd_order_usage(xd) == 0);
    pbi->idct_add_y_block(xd->qcoeff,
                          xd->block[0].dequant,
                          xd->predictor,
                          xd->dst.y_buffer,
                          xd->dst.y_stride,
                          xd->eobs);
    pbi->idct_add_uv_block(xd->qcoeff + 16 * 16,
                           xd->block[16].dequant,
                           xd->predictor + 16 * 16,
                           xd->dst.u_buffer,
                           xd->dst.v_buffer,
                           xd->dst.uv_stride,
                           xd->eobs + 16);
  } else {
#ifdef DEC_DEBUG
    if (dec_debug) {
      int i;
      printf("\n");
      printf("qcoeff 4x4\n");
      for (i = 0; i < 400; i++) {
        printf("%3d ", xd->qcoeff[i]);
        if (i % 16 == 15) printf("\n");
      }
      printf("\n");
      printf("predictor\n");
      for (i = 0; i < 400; i++) {
        printf("%3d ", xd->predictor[i]);
        if (i % 16 == 15) printf("\n");
      }
    }
#endif
    tx_type = get_tx_type_4x4(xd, &xd->block[0]);
    if (tx_type != DCT_DCT) {
      assert(get_2nd_order_usage(xd) == 0);
      for (i = 0; i < 16; i++) {
        BLOCKD *b = &xd->block[i];
        tx_type = get_tx_type_4x4(xd, b);
        if (tx_type != DCT_DCT) {
          vp9_ht_dequant_idct_add_c(tx_type, b->qcoeff,
                                    b->dequant, b->predictor,
                                    *(b->base_dst) + b->dst, 16,
                                    b->dst_stride, b->eob);
        } else {
          vp9_dequant_idct_add(b->qcoeff, b->dequant, b->predictor,
                               *(b->base_dst) + b->dst, 16, b->dst_stride);
        }
      }
    } else {
      BLOCKD *b = &xd->block[24];
      assert(get_2nd_order_usage(xd) == 1);
      vp9_dequantize_b(b);
      if (xd->eobs[24] > 1) {
        vp9_short_inv_walsh4x4(&b->dqcoeff[0], b->diff);
        ((int *)b->qcoeff)[0] = 0;
        ((int *)b->qcoeff)[1] = 0;
        ((int *)b->qcoeff)[2] = 0;
        ((int *)b->qcoeff)[3] = 0;
        ((int *)b->qcoeff)[4] = 0;
        ((int *)b->qcoeff)[5] = 0;
        ((int *)b->qcoeff)[6] = 0;
        ((int *)b->qcoeff)[7] = 0;
      } else {
        xd->inv_walsh4x4_1(&b->dqcoeff[0], b->diff);
        ((int *)b->qcoeff)[0] = 0;
      }
      vp9_dequantize_b(b);
      pbi->dc_idct_add_y_block(xd->qcoeff,
                               xd->block[0].dequant,
                               xd->predictor,
                               xd->dst.y_buffer,
                               xd->dst.y_stride,
                               xd->eobs,
                               xd->block[24].diff);
    }
    pbi->idct_add_uv_block(xd->qcoeff + 16 * 16,
                           xd->block[16].dequant,
                           xd->predictor + 16 * 16,
                           xd->dst.u_buffer,
                           xd->dst.v_buffer,
                           xd->dst.uv_stride,
                           xd->eobs + 16);
  }
}

static void decode_16x16_sb(VP9D_COMP *pbi, MACROBLOCKD *xd,
                            BOOL_DECODER* const bc, int n,
                            int maska, int shiftb) {
  int x_idx = n & maska, y_idx = n >> shiftb;
  TX_TYPE tx_type = get_tx_type_16x16(xd, &xd->block[0]);
  if (tx_type != DCT_DCT) {
    vp9_ht_dequant_idct_add_16x16_c(
        tx_type, xd->qcoeff, xd->block[0].dequant,
        xd->dst.y_buffer + y_idx * 16 * xd->dst.y_stride + x_idx * 16,
        xd->dst.y_buffer + y_idx * 16 * xd->dst.y_stride + x_idx * 16,
        xd->dst.y_stride, xd->dst.y_stride, xd->block[0].eob);
  } else {
    vp9_dequant_idct_add_16x16(
        xd->qcoeff, xd->block[0].dequant,
        xd->dst.y_buffer + y_idx * 16 * xd->dst.y_stride + x_idx * 16,
        xd->dst.y_buffer + y_idx * 16 * xd->dst.y_stride + x_idx * 16,
        xd->dst.y_stride, xd->dst.y_stride, xd->eobs[0]);
  }
  vp9_dequant_idct_add_uv_block_8x8_inplace_c(
      xd->qcoeff + 16 * 16,
      xd->block[16].dequant,
      xd->dst.u_buffer + y_idx * 8 * xd->dst.uv_stride + x_idx * 8,
      xd->dst.v_buffer + y_idx * 8 * xd->dst.uv_stride + x_idx * 8,
      xd->dst.uv_stride, xd->eobs + 16, xd);
};

static void decode_8x8_sb(VP9D_COMP *pbi, MACROBLOCKD *xd,
                          BOOL_DECODER* const bc, int n,
                          int maska, int shiftb) {
  int x_idx = n & maska, y_idx = n >> shiftb;
  BLOCKD *b = &xd->block[24];
  TX_TYPE tx_type = get_tx_type_8x8(xd, &xd->block[0]);
  if (tx_type != DCT_DCT) {
    int i;
    for (i = 0; i < 4; i++) {
      int ib = vp9_i8x8_block[i];
      int idx = (ib & 0x02) ? (ib + 2) : ib;
      int16_t *q  = xd->block[idx].qcoeff;
      int16_t *dq = xd->block[0].dequant;
      int stride = xd->dst.y_stride;
      BLOCKD *b = &xd->block[ib];
      tx_type = get_tx_type_8x8(xd, &xd->block[ib]);
      if (tx_type != DCT_DCT) {
        vp9_ht_dequant_idct_add_8x8_c(
            tx_type, q, dq,
            xd->dst.y_buffer + (y_idx * 16 + (i / 2) * 8) * xd->dst.y_stride
            + x_idx * 16 + (i & 1) * 8,
            xd->dst.y_buffer + (y_idx * 16 + (i / 2) * 8) * xd->dst.y_stride
            + x_idx * 16 + (i & 1) * 8,
            stride, stride, b->eob);
      } else {
        vp9_dequant_idct_add_8x8_c(
            q, dq,
            xd->dst.y_buffer + (y_idx * 16 + (i / 2) * 8) * xd->dst.y_stride
            + x_idx * 16 + (i & 1) * 8,
            xd->dst.y_buffer + (y_idx * 16 + (i / 2) * 8) * xd->dst.y_stride
            + x_idx * 16 + (i & 1) * 8,
            stride, stride, 0, b->eob);
      }
      vp9_dequant_idct_add_uv_block_8x8_inplace_c(
          xd->qcoeff + 16 * 16, xd->block[16].dequant,
          xd->dst.u_buffer + y_idx * 8 * xd->dst.uv_stride + x_idx * 8,
          xd->dst.v_buffer + y_idx * 8 * xd->dst.uv_stride + x_idx * 8,
          xd->dst.uv_stride, xd->eobs + 16, xd);
    }
  } else {
    vp9_dequantize_b_2x2(b);
    vp9_short_ihaar2x2(&b->dqcoeff[0], b->diff, 8);
    ((int *)b->qcoeff)[0] = 0;  // 2nd order block are set to 0 after idct
    ((int *)b->qcoeff)[1] = 0;
    ((int *)b->qcoeff)[2] = 0;
    ((int *)b->qcoeff)[3] = 0;
    ((int *)b->qcoeff)[4] = 0;
    ((int *)b->qcoeff)[5] = 0;
    ((int *)b->qcoeff)[6] = 0;
    ((int *)b->qcoeff)[7] = 0;
    vp9_dequant_dc_idct_add_y_block_8x8_inplace_c(
        xd->qcoeff, xd->block[0].dequant,
        xd->dst.y_buffer + y_idx * 16 * xd->dst.y_stride + x_idx * 16,
        xd->dst.y_stride, xd->eobs, xd->block[24].diff, xd);
    vp9_dequant_idct_add_uv_block_8x8_inplace_c(
        xd->qcoeff + 16 * 16, xd->block[16].dequant,
        xd->dst.u_buffer + y_idx * 8 * xd->dst.uv_stride + x_idx * 8,
        xd->dst.v_buffer + y_idx * 8 * xd->dst.uv_stride + x_idx * 8,
        xd->dst.uv_stride, xd->eobs + 16, xd);
  }
};

static void decode_4x4_sb(VP9D_COMP *pbi, MACROBLOCKD *xd,
                          BOOL_DECODER* const bc, int n,
                          int maska, int shiftb) {
  int x_idx = n & maska, y_idx = n >> shiftb;
  BLOCKD *b = &xd->block[24];
  TX_TYPE tx_type = get_tx_type_4x4(xd, &xd->block[0]);
  if (tx_type != DCT_DCT) {
    int i;
    for (i = 0; i < 16; i++) {
      BLOCKD *b = &xd->block[i];
      tx_type = get_tx_type_4x4(xd, b);
      if (tx_type != DCT_DCT) {
        vp9_ht_dequant_idct_add_c(
            tx_type, b->qcoeff, b->dequant,
            xd->dst.y_buffer + (y_idx * 16 + (i / 4) * 4) * xd->dst.y_stride
            + x_idx * 16 + (i & 3) * 4,
            xd->dst.y_buffer + (y_idx * 16 + (i / 4) * 4) * xd->dst.y_stride
            + x_idx * 16 + (i & 3) * 4,
            xd->dst.y_stride, xd->dst.y_stride, b->eob);
      } else {
        vp9_dequant_idct_add_c(
            b->qcoeff, b->dequant,
            xd->dst.y_buffer + (y_idx * 16 + (i / 4) * 4) * xd->dst.y_stride
            + x_idx * 16 + (i & 3) * 4,
            xd->dst.y_buffer + (y_idx * 16 + (i / 4) * 4) * xd->dst.y_stride
            + x_idx * 16 + (i & 3) * 4,
            xd->dst.y_stride, xd->dst.y_stride);
      }
    }
  } else {
    vp9_dequantize_b(b);
    if (xd->eobs[24] > 1) {
      vp9_short_inv_walsh4x4(&b->dqcoeff[0], b->diff);
      ((int *)b->qcoeff)[0] = 0;
      ((int *)b->qcoeff)[1] = 0;
      ((int *)b->qcoeff)[2] = 0;
      ((int *)b->qcoeff)[3] = 0;
      ((int *)b->qcoeff)[4] = 0;
      ((int *)b->qcoeff)[5] = 0;
      ((int *)b->qcoeff)[6] = 0;
      ((int *)b->qcoeff)[7] = 0;
    } else {
      xd->inv_walsh4x4_1(&b->dqcoeff[0], b->diff);
      ((int *)b->qcoeff)[0] = 0;
    }
    vp9_dequant_dc_idct_add_y_block_4x4_inplace_c(
        xd->qcoeff, xd->block[0].dequant,
        xd->dst.y_buffer + y_idx * 16 * xd->dst.y_stride + x_idx * 16,
        xd->dst.y_stride, xd->eobs, xd->block[24].diff, xd);
  }
  vp9_dequant_idct_add_uv_block_4x4_inplace_c(
      xd->qcoeff + 16 * 16, xd->block[16].dequant,
      xd->dst.u_buffer + y_idx * 8 * xd->dst.uv_stride + x_idx * 8,
      xd->dst.v_buffer + y_idx * 8 * xd->dst.uv_stride + x_idx * 8,
      xd->dst.uv_stride, xd->eobs + 16, xd);
};

static void decode_superblock64(VP9D_COMP *pbi, MACROBLOCKD *xd,
                                int mb_row, int mb_col,
                                BOOL_DECODER* const bc) {
  int i, n, eobtotal;
  TX_SIZE tx_size = xd->mode_info_context->mbmi.txfm_size;
  VP9_COMMON *const pc = &pbi->common;
  MODE_INFO *orig_mi = xd->mode_info_context;
  const int mis = pc->mode_info_stride;

  assert(xd->mode_info_context->mbmi.sb_type == BLOCK_SIZE_SB64X64);

  if (pbi->common.frame_type != KEY_FRAME)
    vp9_setup_interp_filters(xd, xd->mode_info_context->mbmi.interp_filter, pc);

  // re-initialize macroblock dequantizer before detokenization
  if (xd->segmentation_enabled)
    mb_init_dequantizer(pbi, xd);

  if (xd->mode_info_context->mbmi.mb_skip_coeff) {
    int n;

    vp9_reset_mb_tokens_context(xd);
    for (n = 1; n <= 3; n++) {
      if (mb_col < pc->mb_cols - n)
        xd->above_context += n;
      if (mb_row < pc->mb_rows - n)
        xd->left_context += n;
      vp9_reset_mb_tokens_context(xd);
      if (mb_col < pc->mb_cols - n)
        xd->above_context -= n;
      if (mb_row < pc->mb_rows - n)
        xd->left_context -= n;
    }

    /* Special case:  Force the loopfilter to skip when eobtotal and
     * mb_skip_coeff are zero.
     */
    skip_recon_mb(pbi, xd);
    return;
  }

  /* do prediction */
  if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME) {
    vp9_build_intra_predictors_sb64y_s(xd);
    vp9_build_intra_predictors_sb64uv_s(xd);
  } else {
    vp9_build_inter64x64_predictors_sb(xd, xd->dst.y_buffer,
                                       xd->dst.u_buffer, xd->dst.v_buffer,
                                       xd->dst.y_stride, xd->dst.uv_stride);
  }

  /* dequantization and idct */
  if (xd->mode_info_context->mbmi.txfm_size == TX_32X32) {
    for (n = 0; n < 4; n++) {
      const int x_idx = n & 1, y_idx = n >> 1;

      if (mb_col + x_idx * 2 >= pc->mb_cols ||
          mb_row + y_idx * 2 >= pc->mb_rows)
        continue;

      xd->left_context = pc->left_context + (y_idx << 1);
      xd->above_context = pc->above_context + mb_col + (x_idx << 1);
      xd->mode_info_context = orig_mi + x_idx * 2 + y_idx * 2 * mis;
      eobtotal = vp9_decode_sb_tokens(pbi, xd, bc);
      if (eobtotal == 0) {  // skip loopfilter
        xd->mode_info_context->mbmi.mb_skip_coeff = 1;
        if (mb_col + 1 < pc->mb_cols)
          xd->mode_info_context[1].mbmi.mb_skip_coeff = 1;
        if (mb_row + 1 < pc->mb_rows) {
          xd->mode_info_context[mis].mbmi.mb_skip_coeff = 1;
          if (mb_col + 1 < pc->mb_cols)
            xd->mode_info_context[mis + 1].mbmi.mb_skip_coeff = 1;
        }
      } else {
        vp9_dequant_idct_add_32x32(xd->sb_coeff_data.qcoeff, xd->block[0].dequant,
                                   xd->dst.y_buffer + x_idx * 32 +
                                       xd->dst.y_stride * y_idx * 32,
                                   xd->dst.y_buffer + x_idx * 32 +
                                       xd->dst.y_stride * y_idx * 32,
                                   xd->dst.y_stride, xd->dst.y_stride,
                                   xd->eobs[0]);
        vp9_dequant_idct_add_uv_block_16x16_c(xd->sb_coeff_data.qcoeff + 1024,
                                              xd->block[16].dequant,
                                              xd->dst.u_buffer + x_idx * 16 +
                                                xd->dst.uv_stride * y_idx * 16,
                                              xd->dst.v_buffer + x_idx * 16 +
                                                xd->dst.uv_stride * y_idx * 16,
                                              xd->dst.uv_stride, xd->eobs + 16);
      }
    }
  } else {
    for (n = 0; n < 16; n++) {
      int x_idx = n & 3, y_idx = n >> 2;

      if (mb_col + x_idx >= pc->mb_cols || mb_row + y_idx >= pc->mb_rows)
        continue;

      xd->above_context = pc->above_context + mb_col + x_idx;
      xd->left_context = pc->left_context + y_idx;
      xd->mode_info_context = orig_mi + x_idx + y_idx * mis;
      for (i = 0; i < 25; i++) {
        xd->block[i].eob = 0;
        xd->eobs[i] = 0;
      }

      eobtotal = vp9_decode_mb_tokens(pbi, xd, bc);
      if (eobtotal == 0) {  // skip loopfilter
        xd->mode_info_context->mbmi.mb_skip_coeff = 1;
        continue;
      }

      if (tx_size == TX_16X16) {
        decode_16x16_sb(pbi, xd, bc, n, 3, 2);
      } else if (tx_size == TX_8X8) {
        decode_8x8_sb(pbi, xd, bc, n, 3, 2);
      } else {
        decode_4x4_sb(pbi, xd, bc, n, 3, 2);
      }
    }
  }

  xd->above_context = pc->above_context + mb_col;
  xd->left_context = pc->left_context;
  xd->mode_info_context = orig_mi;
}

static void decode_superblock32(VP9D_COMP *pbi, MACROBLOCKD *xd,
                                int mb_row, int mb_col,
                                BOOL_DECODER* const bc) {
  int i, n, eobtotal;
  TX_SIZE tx_size = xd->mode_info_context->mbmi.txfm_size;
  VP9_COMMON *const pc = &pbi->common;
  MODE_INFO *orig_mi = xd->mode_info_context;
  const int mis = pc->mode_info_stride;

  assert(xd->mode_info_context->mbmi.sb_type == BLOCK_SIZE_SB32X32);

  if (pbi->common.frame_type != KEY_FRAME)
    vp9_setup_interp_filters(xd, xd->mode_info_context->mbmi.interp_filter, pc);

  // re-initialize macroblock dequantizer before detokenization
  if (xd->segmentation_enabled)
    mb_init_dequantizer(pbi, xd);

  if (xd->mode_info_context->mbmi.mb_skip_coeff) {
    vp9_reset_mb_tokens_context(xd);
    if (mb_col < pc->mb_cols - 1)
      xd->above_context++;
    if (mb_row < pc->mb_rows - 1)
      xd->left_context++;
    vp9_reset_mb_tokens_context(xd);
    if (mb_col < pc->mb_cols - 1)
      xd->above_context--;
    if (mb_row < pc->mb_rows - 1)
      xd->left_context--;

    /* Special case:  Force the loopfilter to skip when eobtotal and
     * mb_skip_coeff are zero.
     */
    skip_recon_mb(pbi, xd);
    return;
  }

  /* do prediction */
  if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME) {
    vp9_build_intra_predictors_sby_s(xd);
    vp9_build_intra_predictors_sbuv_s(xd);
  } else {
    vp9_build_inter32x32_predictors_sb(xd, xd->dst.y_buffer,
                                       xd->dst.u_buffer, xd->dst.v_buffer,
                                       xd->dst.y_stride, xd->dst.uv_stride);
  }

  /* dequantization and idct */
  if (xd->mode_info_context->mbmi.txfm_size == TX_32X32) {
    eobtotal = vp9_decode_sb_tokens(pbi, xd, bc);
    if (eobtotal == 0) {  // skip loopfilter
      xd->mode_info_context->mbmi.mb_skip_coeff = 1;
      if (mb_col + 1 < pc->mb_cols)
        xd->mode_info_context[1].mbmi.mb_skip_coeff = 1;
      if (mb_row + 1 < pc->mb_rows) {
        xd->mode_info_context[mis].mbmi.mb_skip_coeff = 1;
        if (mb_col + 1 < pc->mb_cols)
          xd->mode_info_context[mis + 1].mbmi.mb_skip_coeff = 1;
      }
    } else {
      vp9_dequant_idct_add_32x32(xd->sb_coeff_data.qcoeff, xd->block[0].dequant,
                                 xd->dst.y_buffer, xd->dst.y_buffer,
                                 xd->dst.y_stride, xd->dst.y_stride,
                                 xd->eobs[0]);
      vp9_dequant_idct_add_uv_block_16x16_c(xd->sb_coeff_data.qcoeff + 1024,
                                            xd->block[16].dequant,
                                            xd->dst.u_buffer, xd->dst.v_buffer,
                                            xd->dst.uv_stride, xd->eobs + 16);
    }
  } else {
    for (n = 0; n < 4; n++) {
      int x_idx = n & 1, y_idx = n >> 1;

      if (mb_col + x_idx >= pc->mb_cols || mb_row + y_idx >= pc->mb_rows)
        continue;

      xd->above_context = pc->above_context + mb_col + x_idx;
      xd->left_context = pc->left_context + y_idx + (mb_row & 2);
      xd->mode_info_context = orig_mi + x_idx + y_idx * mis;
      for (i = 0; i < 25; i++) {
        xd->block[i].eob = 0;
        xd->eobs[i] = 0;
      }

      eobtotal = vp9_decode_mb_tokens(pbi, xd, bc);
      if (eobtotal == 0) {  // skip loopfilter
        xd->mode_info_context->mbmi.mb_skip_coeff = 1;
        continue;
      }

      if (tx_size == TX_16X16) {
        decode_16x16_sb(pbi, xd, bc, n, 1, 1);
      } else if (tx_size == TX_8X8) {
        decode_8x8_sb(pbi, xd, bc, n, 1, 1);
      } else {
        decode_4x4_sb(pbi, xd, bc, n, 1, 1);
      }
    }

    xd->above_context = pc->above_context + mb_col;
    xd->left_context = pc->left_context + (mb_row & 2);
    xd->mode_info_context = orig_mi;
  }
}

static void decode_macroblock(VP9D_COMP *pbi, MACROBLOCKD *xd,
                              int mb_row, unsigned int mb_col,
                              BOOL_DECODER* const bc) {
  int eobtotal = 0;
  MB_PREDICTION_MODE mode;
  int i;
  int tx_size;

  assert(!xd->mode_info_context->mbmi.sb_type);

  // re-initialize macroblock dequantizer before detokenization
  if (xd->segmentation_enabled)
    mb_init_dequantizer(pbi, xd);

  tx_size = xd->mode_info_context->mbmi.txfm_size;
  mode = xd->mode_info_context->mbmi.mode;

  if (xd->mode_info_context->mbmi.mb_skip_coeff) {
    vp9_reset_mb_tokens_context(xd);
  } else if (!bool_error(bc)) {
    for (i = 0; i < 25; i++) {
      xd->block[i].eob = 0;
      xd->eobs[i] = 0;
    }
    if (mode != B_PRED) {
      eobtotal = vp9_decode_mb_tokens(pbi, xd, bc);
    }
  }

  //mode = xd->mode_info_context->mbmi.mode;
  if (pbi->common.frame_type != KEY_FRAME)
    vp9_setup_interp_filters(xd, xd->mode_info_context->mbmi.interp_filter,
                             &pbi->common);

  if (eobtotal == 0 && mode != B_PRED && mode != SPLITMV
      && mode != I8X8_PRED
      && !bool_error(bc)) {
    /* Special case:  Force the loopfilter to skip when eobtotal and
     * mb_skip_coeff are zero.
     * */
    xd->mode_info_context->mbmi.mb_skip_coeff = 1;
    skip_recon_mb(pbi, xd);
    return;
  }
#ifdef DEC_DEBUG
  if (dec_debug)
    printf("Decoding mb:  %d %d\n", xd->mode_info_context->mbmi.mode, tx_size);
#endif

  // moved to be performed before detokenization
//  if (xd->segmentation_enabled)
//    mb_init_dequantizer(pbi, xd);

  /* do prediction */
  if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME) {
    if (mode != I8X8_PRED) {
      vp9_build_intra_predictors_mbuv(xd);
      if (mode != B_PRED) {
        vp9_build_intra_predictors_mby(xd);
      }
    }
  } else {
#ifdef DEC_DEBUG
  if (dec_debug)
    printf("Decoding mb:  %d %d interp %d\n",
           xd->mode_info_context->mbmi.mode, tx_size,
           xd->mode_info_context->mbmi.interp_filter);
#endif
    vp9_build_inter_predictors_mb(xd);
  }

  if (tx_size == TX_16X16) {
    decode_16x16(pbi, xd, bc);
  } else if (tx_size == TX_8X8) {
    decode_8x8(pbi, xd, bc);
  } else {
    decode_4x4(pbi, xd, bc);
  }
#ifdef DEC_DEBUG
  if (dec_debug) {
    int i, j;
    printf("\n");
    printf("final y\n");
    for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++)
        printf("%3d ", xd->dst.y_buffer[i * xd->dst.y_stride + j]);
      printf("\n");
    }
    printf("\n");
    printf("final u\n");
    for (i = 0; i < 8; i++) {
      for (j = 0; j < 8; j++)
        printf("%3d ", xd->dst.u_buffer[i * xd->dst.uv_stride + j]);
      printf("\n");
    }
    printf("\n");
    printf("final v\n");
    for (i = 0; i < 8; i++) {
      for (j = 0; j < 8; j++)
        printf("%3d ", xd->dst.v_buffer[i * xd->dst.uv_stride + j]);
      printf("\n");
    }
    fflush(stdout);
  }
#endif
}


static int get_delta_q(vp9_reader *bc, int prev, int *q_update) {
  int ret_val = 0;

  if (vp9_read_bit(bc)) {
    ret_val = vp9_read_literal(bc, 4);

    if (vp9_read_bit(bc))
      ret_val = -ret_val;
  }

  /* Trigger a quantizer update if the delta-q value has changed */
  if (ret_val != prev)
    *q_update = 1;

  return ret_val;
}

#ifdef PACKET_TESTING
#include <stdio.h>
FILE *vpxlog = 0;
#endif

static void set_offsets(VP9D_COMP *pbi, int block_size,
                        int mb_row, int mb_col) {
  VP9_COMMON *const cm = &pbi->common;
  MACROBLOCKD *const xd = &pbi->mb;
  const int mis = cm->mode_info_stride;
  const int idx = mis * mb_row + mb_col;
  const int dst_fb_idx = cm->new_fb_idx;
  const int recon_y_stride = cm->yv12_fb[dst_fb_idx].y_stride;
  const int recon_uv_stride = cm->yv12_fb[dst_fb_idx].uv_stride;
  const int recon_yoffset = mb_row * 16 * recon_y_stride + 16 * mb_col;
  const int recon_uvoffset = mb_row * 8 * recon_uv_stride + 8 * mb_col;

  xd->mode_info_context = cm->mi + idx;
  xd->mode_info_context->mbmi.sb_type = block_size >> 5;
  xd->prev_mode_info_context = cm->prev_mi + idx;
  xd->above_context = cm->above_context + mb_col;
  xd->left_context = cm->left_context + (mb_row & 3);

  /* Distance of Mb to the various image edges.
   * These are specified to 8th pel as they are always compared to
   * values that are in 1/8th pel units
   */
  block_size >>= 4;  // in mb units
  xd->mb_to_top_edge = -((mb_row * 16)) << 3;
  xd->mb_to_left_edge = -((mb_col * 16) << 3);
  xd->mb_to_bottom_edge = ((cm->mb_rows - block_size - mb_row) * 16) << 3;
  xd->mb_to_right_edge = ((cm->mb_cols - block_size - mb_col) * 16) << 3;

  xd->up_available = (mb_row != 0);
  xd->left_available = (mb_col != 0);

  xd->dst.y_buffer = cm->yv12_fb[dst_fb_idx].y_buffer + recon_yoffset;
  xd->dst.u_buffer = cm->yv12_fb[dst_fb_idx].u_buffer + recon_uvoffset;
  xd->dst.v_buffer = cm->yv12_fb[dst_fb_idx].v_buffer + recon_uvoffset;
}

static void set_refs(VP9D_COMP *pbi, int block_size,
                     int mb_row, int mb_col) {
  VP9_COMMON *const cm = &pbi->common;
  MACROBLOCKD *const xd = &pbi->mb;
  MODE_INFO *mi = xd->mode_info_context;
  MB_MODE_INFO *const mbmi = &mi->mbmi;

  if (mbmi->ref_frame > INTRA_FRAME) {
    int ref_fb_idx, ref_yoffset, ref_uvoffset, ref_y_stride, ref_uv_stride;

    /* Select the appropriate reference frame for this MB */
    if (mbmi->ref_frame == LAST_FRAME)
      ref_fb_idx = cm->lst_fb_idx;
    else if (mbmi->ref_frame == GOLDEN_FRAME)
      ref_fb_idx = cm->gld_fb_idx;
    else
      ref_fb_idx = cm->alt_fb_idx;

    ref_y_stride = cm->yv12_fb[ref_fb_idx].y_stride;
    ref_yoffset = mb_row * 16 * ref_y_stride + 16 * mb_col;
    xd->pre.y_buffer = cm->yv12_fb[ref_fb_idx].y_buffer + ref_yoffset;
    ref_uv_stride = cm->yv12_fb[ref_fb_idx].uv_stride;
    ref_uvoffset = mb_row * 8 * ref_uv_stride + 8 * mb_col;
    xd->pre.u_buffer = cm->yv12_fb[ref_fb_idx].u_buffer + ref_uvoffset;
    xd->pre.v_buffer = cm->yv12_fb[ref_fb_idx].v_buffer + ref_uvoffset;

    /* propagate errors from reference frames */
    xd->corrupted |= cm->yv12_fb[ref_fb_idx].corrupted;

    if (mbmi->second_ref_frame > INTRA_FRAME) {
      int second_ref_fb_idx;

      /* Select the appropriate reference frame for this MB */
      if (mbmi->second_ref_frame == LAST_FRAME)
        second_ref_fb_idx = cm->lst_fb_idx;
      else if (mbmi->second_ref_frame == GOLDEN_FRAME)
        second_ref_fb_idx = cm->gld_fb_idx;
      else
        second_ref_fb_idx = cm->alt_fb_idx;

      xd->second_pre.y_buffer =
          cm->yv12_fb[second_ref_fb_idx].y_buffer + ref_yoffset;
      xd->second_pre.u_buffer =
          cm->yv12_fb[second_ref_fb_idx].u_buffer + ref_uvoffset;
      xd->second_pre.v_buffer =
          cm->yv12_fb[second_ref_fb_idx].v_buffer + ref_uvoffset;

      /* propagate errors from reference frames */
      xd->corrupted |= cm->yv12_fb[second_ref_fb_idx].corrupted;
    }
  }

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
  }
}

/* Decode a row of Superblocks (2x2 region of MBs) */
static void decode_sb_row(VP9D_COMP *pbi, VP9_COMMON *pc,
                          int mb_row, MACROBLOCKD *xd,
                          BOOL_DECODER* const bc) {
  int mb_col;

  // For a SB there are 2 left contexts, each pertaining to a MB row within
  vpx_memset(pc->left_context, 0, sizeof(pc->left_context));

  for (mb_col = 0; mb_col < pc->mb_cols; mb_col += 4) {
    if (vp9_read(bc, pc->sb64_coded)) {
      set_offsets(pbi, 64, mb_row, mb_col);
      vp9_decode_mb_mode_mv(pbi, xd, mb_row, mb_col, bc);
      set_refs(pbi, 64, mb_row, mb_col);
      decode_superblock64(pbi, xd, mb_row, mb_col, bc);
      xd->corrupted |= bool_error(bc);
    } else {
      int j;

      for (j = 0; j < 4; j++) {
        const int x_idx_sb = (j & 1) << 1, y_idx_sb = j & 2;

        if (mb_row + y_idx_sb >= pc->mb_rows ||
            mb_col + x_idx_sb >= pc->mb_cols) {
          // MB lies outside frame, skip on to next
          continue;
        }

        xd->sb_index = j;

        if (vp9_read(bc, pc->sb32_coded)) {
          set_offsets(pbi, 32, mb_row + y_idx_sb, mb_col + x_idx_sb);
          vp9_decode_mb_mode_mv(pbi,
                                xd, mb_row + y_idx_sb, mb_col + x_idx_sb, bc);
          set_refs(pbi, 32, mb_row + y_idx_sb, mb_col + x_idx_sb);
          decode_superblock32(pbi,
                              xd, mb_row + y_idx_sb, mb_col + x_idx_sb, bc);
          xd->corrupted |= bool_error(bc);
        } else {
          int i;

          // Process the 4 MBs within the SB in the order:
          // top-left, top-right, bottom-left, bottom-right
          for (i = 0; i < 4; i++) {
            const int x_idx = x_idx_sb + (i & 1), y_idx = y_idx_sb + (i >> 1);

            if (mb_row + y_idx >= pc->mb_rows ||
                mb_col + x_idx >= pc->mb_cols) {
              // MB lies outside frame, skip on to next
              continue;
            }

            set_offsets(pbi, 16, mb_row + y_idx, mb_col + x_idx);
            xd->mb_index = i;
            vp9_decode_mb_mode_mv(pbi, xd, mb_row + y_idx, mb_col + x_idx, bc);
            update_blockd_bmi(xd);
            set_refs(pbi, 16, mb_row + y_idx, mb_col + x_idx);
            vp9_intra_prediction_down_copy(xd);
            decode_macroblock(pbi, xd, mb_row, mb_col, bc);

            /* check if the boolean decoder has suffered an error */
            xd->corrupted |= bool_error(bc);
          }
        }
      }
    }
  }
}

static unsigned int read_partition_size(const unsigned char *cx_size) {
  const unsigned int size =
    cx_size[0] + (cx_size[1] << 8) + (cx_size[2] << 16);
  return size;
}

static int read_is_valid(const unsigned char *start,
                         size_t               len,
                         const unsigned char *end) {
  return (start + len > start && start + len <= end);
}


static void setup_token_decoder(VP9D_COMP *pbi,
                                const unsigned char *cx_data,
                                BOOL_DECODER* const bool_decoder) {
  VP9_COMMON          *pc = &pbi->common;
  const unsigned char *user_data_end = pbi->Source + pbi->source_sz;
  const unsigned char *partition;

  ptrdiff_t            partition_size;
  ptrdiff_t            bytes_left;

  // Set up pointers to token partition
  partition = cx_data;
  bytes_left = user_data_end - partition;
  partition_size = bytes_left;

  /* Validate the calculated partition length. If the buffer
   * described by the partition can't be fully read, then restrict
   * it to the portion that can be (for EC mode) or throw an error.
   */
  if (!read_is_valid(partition, partition_size, user_data_end)) {
    vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                       "Truncated packet or corrupt partition "
                       "%d length", 1);
  }

  if (vp9_start_decode(bool_decoder,
                       partition, (unsigned int)partition_size))
    vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                       "Failed to allocate bool decoder %d", 1);
}

static void init_frame(VP9D_COMP *pbi) {
  VP9_COMMON *const pc = &pbi->common;
  MACROBLOCKD *const xd  = &pbi->mb;

  if (pc->frame_type == KEY_FRAME) {

    if (pc->last_frame_seg_map)
      vpx_memset(pc->last_frame_seg_map, 0, (pc->mb_rows * pc->mb_cols));

    vp9_init_mv_probs(pc);

    vp9_init_mbmode_probs(pc);
    vp9_default_bmode_probs(pc->fc.bmode_prob);

    vp9_default_coef_probs(pc);
    vp9_kf_default_bmode_probs(pc->kf_bmode_prob);

    // Reset the segment feature data to the default stats:
    // Features disabled, 0, with delta coding (Default state).
    vp9_clearall_segfeatures(xd);

    xd->mb_segment_abs_delta = SEGMENT_DELTADATA;

    /* reset the mode ref deltasa for loop filter */
    vpx_memset(xd->ref_lf_deltas, 0, sizeof(xd->ref_lf_deltas));
    vpx_memset(xd->mode_lf_deltas, 0, sizeof(xd->mode_lf_deltas));

    /* All buffers are implicitly updated on key frames. */
    pc->refresh_golden_frame = 1;
    pc->refresh_alt_ref_frame = 1;
    pc->copy_buffer_to_gf = 0;
    pc->copy_buffer_to_arf = 0;

    /* Note that Golden and Altref modes cannot be used on a key frame so
     * ref_frame_sign_bias[] is undefined and meaningless
     */
    pc->ref_frame_sign_bias[GOLDEN_FRAME] = 0;
    pc->ref_frame_sign_bias[ALTREF_FRAME] = 0;

    vp9_init_mode_contexts(&pbi->common);
    vpx_memcpy(&pc->lfc, &pc->fc, sizeof(pc->fc));
    vpx_memcpy(&pc->lfc_a, &pc->fc, sizeof(pc->fc));

    vpx_memset(pc->prev_mip, 0,
               (pc->mb_cols + 1) * (pc->mb_rows + 1)* sizeof(MODE_INFO));
    vpx_memset(pc->mip, 0,
               (pc->mb_cols + 1) * (pc->mb_rows + 1)* sizeof(MODE_INFO));

    vp9_update_mode_info_border(pc, pc->mip);
    vp9_update_mode_info_in_image(pc, pc->mi);


  } else {

    if (!pc->use_bilinear_mc_filter)
      pc->mcomp_filter_type = EIGHTTAP;
    else
      pc->mcomp_filter_type = BILINEAR;

    /* To enable choice of different interpolation filters */
    vp9_setup_interp_filters(xd, pc->mcomp_filter_type, pc);
  }

  xd->mode_info_context = pc->mi;
  xd->prev_mode_info_context = pc->prev_mi;
  xd->frame_type = pc->frame_type;
  xd->mode_info_context->mbmi.mode = DC_PRED;
  xd->mode_info_stride = pc->mode_info_stride;
  xd->corrupted = 0; /* init without corruption */

  xd->fullpixel_mask = 0xffffffff;
  if (pc->full_pixel)
    xd->fullpixel_mask = 0xfffffff8;

}

static void read_coef_probs_common(BOOL_DECODER* const bc,
                                   vp9_coeff_probs *coef_probs,
                                   int block_types) {
  int i, j, k, l;

  if (vp9_read_bit(bc)) {
    for (i = 0; i < block_types; i++) {
      for (j = !i; j < COEF_BANDS; j++) {
        /* NB: This j loop starts from 1 on block type i == 0 */
        for (k = 0; k < PREV_COEF_CONTEXTS; k++) {
          if (k >= 3 && ((i == 0 && j == 1) ||
                         (i > 0 && j == 0)))
            continue;
          for (l = 0; l < ENTROPY_NODES; l++) {
            vp9_prob *const p = coef_probs[i][j][k] + l;

            if (vp9_read(bc, COEF_UPDATE_PROB)) {
              *p = read_prob_diff_update(bc, *p);
            }
          }
        }
      }
    }
  }
}

static void read_coef_probs(VP9D_COMP *pbi, BOOL_DECODER* const bc) {
  VP9_COMMON *const pc = &pbi->common;

  read_coef_probs_common(bc, pc->fc.coef_probs_4x4, BLOCK_TYPES_4X4);
  read_coef_probs_common(bc, pc->fc.hybrid_coef_probs_4x4, BLOCK_TYPES_4X4);

  if (pbi->common.txfm_mode != ONLY_4X4) {
    read_coef_probs_common(bc, pc->fc.coef_probs_8x8, BLOCK_TYPES_8X8);
    read_coef_probs_common(bc, pc->fc.hybrid_coef_probs_8x8, BLOCK_TYPES_8X8);
  }
  if (pbi->common.txfm_mode > ALLOW_8X8) {
    read_coef_probs_common(bc, pc->fc.coef_probs_16x16, BLOCK_TYPES_16X16);
    read_coef_probs_common(bc, pc->fc.hybrid_coef_probs_16x16,
                           BLOCK_TYPES_16X16);
  }
  if (pbi->common.txfm_mode > ALLOW_16X16) {
    read_coef_probs_common(bc, pc->fc.coef_probs_32x32, BLOCK_TYPES_32X32);
  }
}

int vp9_decode_frame(VP9D_COMP *pbi, const unsigned char **p_data_end) {
  BOOL_DECODER header_bc, residual_bc;
  VP9_COMMON *const pc = &pbi->common;
  MACROBLOCKD *const xd  = &pbi->mb;
  const unsigned char *data = (const unsigned char *)pbi->Source;
  const unsigned char *data_end = data + pbi->source_sz;
  ptrdiff_t first_partition_length_in_bytes = 0;

  int mb_row;
  int i, j;
  int corrupt_tokens = 0;

  /* start with no corruption of current frame */
  xd->corrupted = 0;
  pc->yv12_fb[pc->new_fb_idx].corrupted = 0;

  if (data_end - data < 3) {
    vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                       "Truncated packet");
  } else {
    pc->last_frame_type = pc->frame_type;
    pc->frame_type = (FRAME_TYPE)(data[0] & 1);
    pc->version = (data[0] >> 1) & 7;
    pc->show_frame = (data[0] >> 4) & 1;
    first_partition_length_in_bytes =
      (data[0] | (data[1] << 8) | (data[2] << 16)) >> 5;

    if ((data + first_partition_length_in_bytes > data_end
         || data + first_partition_length_in_bytes < data))
      vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                         "Truncated packet or corrupt partition 0 length");

    data += 3;

    vp9_setup_version(pc);

    if (pc->frame_type == KEY_FRAME) {
      const int Width = pc->Width;
      const int Height = pc->Height;

      /* vet via sync code */
      /* When error concealment is enabled we should only check the sync
       * code if we have enough bits available
       */
      if (data + 3 < data_end) {
        if (data[0] != 0x9d || data[1] != 0x01 || data[2] != 0x2a)
          vpx_internal_error(&pc->error, VPX_CODEC_UNSUP_BITSTREAM,
                             "Invalid frame sync code");
      }

      /* If error concealment is enabled we should only parse the new size
       * if we have enough data. Otherwise we will end up with the wrong
       * size.
       */
      if (data + 6 < data_end) {
        pc->Width = (data[3] | (data[4] << 8)) & 0x3fff;
        pc->horiz_scale = data[4] >> 6;
        pc->Height = (data[5] | (data[6] << 8)) & 0x3fff;
        pc->vert_scale = data[6] >> 6;
      }
      data += 7;

      if (Width != pc->Width  ||  Height != pc->Height) {
        if (pc->Width <= 0) {
          pc->Width = Width;
          vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                             "Invalid frame width");
        }

        if (pc->Height <= 0) {
          pc->Height = Height;
          vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                             "Invalid frame height");
        }

        if (vp9_alloc_frame_buffers(pc, pc->Width, pc->Height))
          vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                             "Failed to allocate frame buffers");
      }
    }
  }
#ifdef DEC_DEBUG
  printf("Decode frame %d\n", pc->current_video_frame);
#endif

  if ((!pbi->decoded_key_frame && pc->frame_type != KEY_FRAME) ||
      pc->Width == 0 || pc->Height == 0) {
    return -1;
  }

  init_frame(pbi);

  if (vp9_start_decode(&header_bc, data,
                       (unsigned int)first_partition_length_in_bytes))
    vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                       "Failed to allocate bool decoder 0");
  if (pc->frame_type == KEY_FRAME) {
    pc->clr_type    = (YUV_TYPE)vp9_read_bit(&header_bc);
    pc->clamp_type  = (CLAMP_TYPE)vp9_read_bit(&header_bc);
  }

  /* Is segmentation enabled */
  xd->segmentation_enabled = (unsigned char)vp9_read_bit(&header_bc);

  if (xd->segmentation_enabled) {
    // Read whether or not the segmentation map is being explicitly
    // updated this frame.
    xd->update_mb_segmentation_map = (unsigned char)vp9_read_bit(&header_bc);

    // If so what method will be used.
    if (xd->update_mb_segmentation_map) {
      // Which macro block level features are enabled

      // Read the probs used to decode the segment id for each macro
      // block.
      for (i = 0; i < MB_FEATURE_TREE_PROBS; i++) {
          xd->mb_segment_tree_probs[i] = vp9_read_bit(&header_bc) ?
              (vp9_prob)vp9_read_literal(&header_bc, 8) : 255;
      }

      // Read the prediction probs needed to decode the segment id
      pc->temporal_update = (unsigned char)vp9_read_bit(&header_bc);
      for (i = 0; i < PREDICTION_PROBS; i++) {
        if (pc->temporal_update) {
          pc->segment_pred_probs[i] = vp9_read_bit(&header_bc) ?
              (vp9_prob)vp9_read_literal(&header_bc, 8) : 255;
        } else {
          pc->segment_pred_probs[i] = 255;
        }
      }
    }
    // Is the segment data being updated
    xd->update_mb_segmentation_data = (unsigned char)vp9_read_bit(&header_bc);

    if (xd->update_mb_segmentation_data) {
      int data;

      xd->mb_segment_abs_delta = (unsigned char)vp9_read_bit(&header_bc);

      vp9_clearall_segfeatures(xd);

      // For each segmentation...
      for (i = 0; i < MAX_MB_SEGMENTS; i++) {
        // For each of the segments features...
        for (j = 0; j < SEG_LVL_MAX; j++) {
          // Is the feature enabled
          if (vp9_read_bit(&header_bc)) {
            // Update the feature data and mask
            vp9_enable_segfeature(xd, i, j);

            data = vp9_decode_unsigned_max(&header_bc,
                                           vp9_seg_feature_data_max(j));

            // Is the segment data signed..
            if (vp9_is_segfeature_signed(j)) {
              if (vp9_read_bit(&header_bc))
                data = -data;
            }
          } else
            data = 0;

          vp9_set_segdata(xd, i, j, data);
        }
      }
    }
  }

  // Read common prediction model status flag probability updates for the
  // reference frame
  if (pc->frame_type == KEY_FRAME) {
    // Set the prediction probabilities to defaults
    pc->ref_pred_probs[0] = 120;
    pc->ref_pred_probs[1] = 80;
    pc->ref_pred_probs[2] = 40;

  } else {
    for (i = 0; i < PREDICTION_PROBS; i++) {
      if (vp9_read_bit(&header_bc))
        pc->ref_pred_probs[i] = (vp9_prob)vp9_read_literal(&header_bc, 8);
    }
  }

  pc->sb64_coded = vp9_read_literal(&header_bc, 8);
  pc->sb32_coded = vp9_read_literal(&header_bc, 8);

  /* Read the loop filter level and type */
  pc->txfm_mode = vp9_read_literal(&header_bc, 2);
  if (pc->txfm_mode == 3)
    pc->txfm_mode += vp9_read_bit(&header_bc);
  if (pc->txfm_mode == TX_MODE_SELECT) {
    pc->prob_tx[0] = vp9_read_literal(&header_bc, 8);
    pc->prob_tx[1] = vp9_read_literal(&header_bc, 8);
    pc->prob_tx[2] = vp9_read_literal(&header_bc, 8);
  }

  pc->filter_type = (LOOPFILTERTYPE) vp9_read_bit(&header_bc);
  pc->filter_level = vp9_read_literal(&header_bc, 6);
  pc->sharpness_level = vp9_read_literal(&header_bc, 3);

  /* Read in loop filter deltas applied at the MB level based on mode or ref frame. */
  xd->mode_ref_lf_delta_update = 0;
  xd->mode_ref_lf_delta_enabled = (unsigned char)vp9_read_bit(&header_bc);

  if (xd->mode_ref_lf_delta_enabled) {
    /* Do the deltas need to be updated */
    xd->mode_ref_lf_delta_update = (unsigned char)vp9_read_bit(&header_bc);

    if (xd->mode_ref_lf_delta_update) {
      /* Send update */
      for (i = 0; i < MAX_REF_LF_DELTAS; i++) {
        if (vp9_read_bit(&header_bc)) {
          /*sign = vp9_read_bit( &header_bc );*/
          xd->ref_lf_deltas[i] = (signed char)vp9_read_literal(&header_bc, 6);

          if (vp9_read_bit(&header_bc))        /* Apply sign */
            xd->ref_lf_deltas[i] = xd->ref_lf_deltas[i] * -1;
        }
      }

      /* Send update */
      for (i = 0; i < MAX_MODE_LF_DELTAS; i++) {
        if (vp9_read_bit(&header_bc)) {
          /*sign = vp9_read_bit( &header_bc );*/
          xd->mode_lf_deltas[i] = (signed char)vp9_read_literal(&header_bc, 6);

          if (vp9_read_bit(&header_bc))        /* Apply sign */
            xd->mode_lf_deltas[i] = xd->mode_lf_deltas[i] * -1;
        }
      }
    }
  }

  // Dummy read for now
  vp9_read_literal(&header_bc, 2);

  setup_token_decoder(pbi, data + first_partition_length_in_bytes,
                      &residual_bc);

  /* Read the default quantizers. */
  {
    int Q, q_update;

    Q = vp9_read_literal(&header_bc, QINDEX_BITS);
    pc->base_qindex = Q;
    q_update = 0;
    /* AC 1st order Q = default */
    pc->y1dc_delta_q = get_delta_q(&header_bc, pc->y1dc_delta_q, &q_update);
    pc->y2dc_delta_q = get_delta_q(&header_bc, pc->y2dc_delta_q, &q_update);
    pc->y2ac_delta_q = get_delta_q(&header_bc, pc->y2ac_delta_q, &q_update);
    pc->uvdc_delta_q = get_delta_q(&header_bc, pc->uvdc_delta_q, &q_update);
    pc->uvac_delta_q = get_delta_q(&header_bc, pc->uvac_delta_q, &q_update);

    if (q_update)
      vp9_init_de_quantizer(pbi);

    /* MB level dequantizer setup */
    mb_init_dequantizer(pbi, &pbi->mb);
  }

  /* Determine if the golden frame or ARF buffer should be updated and how.
   * For all non key frames the GF and ARF refresh flags and sign bias
   * flags must be set explicitly.
   */
  if (pc->frame_type != KEY_FRAME) {
    /* Should the GF or ARF be updated from the current frame */
    pc->refresh_golden_frame = vp9_read_bit(&header_bc);
    pc->refresh_alt_ref_frame = vp9_read_bit(&header_bc);

    if (pc->refresh_alt_ref_frame) {
      vpx_memcpy(&pc->fc, &pc->lfc_a, sizeof(pc->fc));
    } else {
      vpx_memcpy(&pc->fc, &pc->lfc, sizeof(pc->fc));
    }

    /* Buffer to buffer copy flags. */
    pc->copy_buffer_to_gf = 0;

    if (!pc->refresh_golden_frame)
      pc->copy_buffer_to_gf = vp9_read_literal(&header_bc, 2);

    pc->copy_buffer_to_arf = 0;

    if (!pc->refresh_alt_ref_frame)
      pc->copy_buffer_to_arf = vp9_read_literal(&header_bc, 2);

    pc->ref_frame_sign_bias[GOLDEN_FRAME] = vp9_read_bit(&header_bc);
    pc->ref_frame_sign_bias[ALTREF_FRAME] = vp9_read_bit(&header_bc);

    /* Is high precision mv allowed */
    xd->allow_high_precision_mv = (unsigned char)vp9_read_bit(&header_bc);
    // Read the type of subpel filter to use
    if (vp9_read_bit(&header_bc)) {
      pc->mcomp_filter_type = SWITCHABLE;
    } else {
      pc->mcomp_filter_type = vp9_read_literal(&header_bc, 2);
    }
#if CONFIG_COMP_INTERINTRA_PRED
    pc->use_interintra = vp9_read_bit(&header_bc);
#endif
    /* To enable choice of different interploation filters */
    vp9_setup_interp_filters(xd, pc->mcomp_filter_type, pc);
  }

  pc->refresh_entropy_probs = vp9_read_bit(&header_bc);
  if (pc->refresh_entropy_probs == 0) {
    vpx_memcpy(&pc->lfc, &pc->fc, sizeof(pc->fc));
  }

  pc->refresh_last_frame = (pc->frame_type == KEY_FRAME)
                           || vp9_read_bit(&header_bc);

  // Read inter mode probability context updates
  if (pc->frame_type != KEY_FRAME) {
    int i, j;
    for (i = 0; i < INTER_MODE_CONTEXTS; i++) {
      for (j = 0; j < 4; j++) {
        if (vp9_read(&header_bc, 252)) {
          pc->fc.vp9_mode_contexts[i][j] =
            (vp9_prob)vp9_read_literal(&header_bc, 8);
        }
      }
    }
  }

#if CONFIG_NEW_MVREF
  // If Key frame reset mv ref id probabilities to defaults
  if (pc->frame_type == KEY_FRAME) {
    // Defaults probabilities for encoding the MV ref id signal
    vpx_memset(xd->mb_mv_ref_probs, VP9_DEFAULT_MV_REF_PROB,
               sizeof(xd->mb_mv_ref_probs));
  } else {
    // Read any mv_ref index probability updates
    int i, j;

    for (i = 0; i < MAX_REF_FRAMES; ++i) {
      // Skip the dummy entry for intra ref frame.
      if (i == INTRA_FRAME) {
        continue;
      }

      // Read any updates to probabilities
      for (j = 0; j < MAX_MV_REF_CANDIDATES - 1; ++j) {
        if (vp9_read(&header_bc, VP9_MVREF_UPDATE_PROB)) {
          xd->mb_mv_ref_probs[i][j] =
            (vp9_prob)vp9_read_literal(&header_bc, 8);
        }
      }
    }
  }
#endif

  if (0) {
    FILE *z = fopen("decodestats.stt", "a");
    fprintf(z, "%6d F:%d,G:%d,A:%d,L:%d,Q:%d\n",
            pc->current_video_frame,
            pc->frame_type,
            pc->refresh_golden_frame,
            pc->refresh_alt_ref_frame,
            pc->refresh_last_frame,
            pc->base_qindex);
    fclose(z);
  }

  vp9_copy(pbi->common.fc.pre_coef_probs_4x4,
           pbi->common.fc.coef_probs_4x4);
  vp9_copy(pbi->common.fc.pre_hybrid_coef_probs_4x4,
           pbi->common.fc.hybrid_coef_probs_4x4);
  vp9_copy(pbi->common.fc.pre_coef_probs_8x8,
           pbi->common.fc.coef_probs_8x8);
  vp9_copy(pbi->common.fc.pre_hybrid_coef_probs_8x8,
           pbi->common.fc.hybrid_coef_probs_8x8);
  vp9_copy(pbi->common.fc.pre_coef_probs_16x16,
           pbi->common.fc.coef_probs_16x16);
  vp9_copy(pbi->common.fc.pre_hybrid_coef_probs_16x16,
           pbi->common.fc.hybrid_coef_probs_16x16);
  vp9_copy(pbi->common.fc.pre_coef_probs_32x32,
           pbi->common.fc.coef_probs_32x32);
  vp9_copy(pbi->common.fc.pre_ymode_prob, pbi->common.fc.ymode_prob);
  vp9_copy(pbi->common.fc.pre_sb_ymode_prob, pbi->common.fc.sb_ymode_prob);
  vp9_copy(pbi->common.fc.pre_uv_mode_prob, pbi->common.fc.uv_mode_prob);
  vp9_copy(pbi->common.fc.pre_bmode_prob, pbi->common.fc.bmode_prob);
  vp9_copy(pbi->common.fc.pre_i8x8_mode_prob, pbi->common.fc.i8x8_mode_prob);
  vp9_copy(pbi->common.fc.pre_sub_mv_ref_prob, pbi->common.fc.sub_mv_ref_prob);
  vp9_copy(pbi->common.fc.pre_mbsplit_prob, pbi->common.fc.mbsplit_prob);
#if CONFIG_COMP_INTERINTRA_PRED
  pbi->common.fc.pre_interintra_prob = pbi->common.fc.interintra_prob;
#endif
  pbi->common.fc.pre_nmvc = pbi->common.fc.nmvc;
  vp9_zero(pbi->common.fc.coef_counts_4x4);
  vp9_zero(pbi->common.fc.hybrid_coef_counts_4x4);
  vp9_zero(pbi->common.fc.coef_counts_8x8);
  vp9_zero(pbi->common.fc.hybrid_coef_counts_8x8);
  vp9_zero(pbi->common.fc.coef_counts_16x16);
  vp9_zero(pbi->common.fc.hybrid_coef_counts_16x16);
  vp9_zero(pbi->common.fc.coef_counts_32x32);
  vp9_zero(pbi->common.fc.ymode_counts);
  vp9_zero(pbi->common.fc.sb_ymode_counts);
  vp9_zero(pbi->common.fc.uv_mode_counts);
  vp9_zero(pbi->common.fc.bmode_counts);
  vp9_zero(pbi->common.fc.i8x8_mode_counts);
  vp9_zero(pbi->common.fc.sub_mv_ref_counts);
  vp9_zero(pbi->common.fc.mbsplit_counts);
  vp9_zero(pbi->common.fc.NMVcount);
  vp9_zero(pbi->common.fc.mv_ref_ct);
#if CONFIG_COMP_INTERINTRA_PRED
  vp9_zero(pbi->common.fc.interintra_counts);
#endif

  read_coef_probs(pbi, &header_bc);

  vpx_memcpy(&xd->pre, &pc->yv12_fb[pc->lst_fb_idx], sizeof(YV12_BUFFER_CONFIG));
  vpx_memcpy(&xd->dst, &pc->yv12_fb[pc->new_fb_idx], sizeof(YV12_BUFFER_CONFIG));

  // Create the segmentation map structure and set to 0
  if (!pc->last_frame_seg_map)
    CHECK_MEM_ERROR(pc->last_frame_seg_map,
                    vpx_calloc((pc->mb_rows * pc->mb_cols), 1));

  /* set up frame new frame for intra coded blocks */
  vp9_setup_intra_recon(&pc->yv12_fb[pc->new_fb_idx]);

  vp9_setup_block_dptrs(xd);

  vp9_build_block_doffsets(xd);

  /* clear out the coeff buffer */
  vpx_memset(xd->qcoeff, 0, sizeof(xd->qcoeff));

  /* Read the mb_no_coeff_skip flag */
  pc->mb_no_coeff_skip = (int)vp9_read_bit(&header_bc);

  vp9_decode_mode_mvs_init(pbi, &header_bc);

  vpx_memset(pc->above_context, 0, sizeof(ENTROPY_CONTEXT_PLANES) * pc->mb_cols);

  /* Decode a row of superblocks */
  for (mb_row = 0; mb_row < pc->mb_rows; mb_row += 4) {
    decode_sb_row(pbi, pc, mb_row, xd, &residual_bc);
  }
  corrupt_tokens |= xd->corrupted;

  /* Collect information about decoder corruption. */
  /* 1. Check first boolean decoder for errors. */
  pc->yv12_fb[pc->new_fb_idx].corrupted = bool_error(&header_bc);
  /* 2. Check the macroblock information */
  pc->yv12_fb[pc->new_fb_idx].corrupted |= corrupt_tokens;

  if (!pbi->decoded_key_frame) {
    if (pc->frame_type == KEY_FRAME &&
        !pc->yv12_fb[pc->new_fb_idx].corrupted)
      pbi->decoded_key_frame = 1;
    else
      vpx_internal_error(&pbi->common.error, VPX_CODEC_CORRUPT_FRAME,
                         "A stream must start with a complete key frame");
  }

  vp9_adapt_coef_probs(pc);
  if (pc->frame_type != KEY_FRAME) {
    vp9_adapt_mode_probs(pc);
    vp9_adapt_nmv_probs(pc, xd->allow_high_precision_mv);
    vp9_update_mode_context(&pbi->common);
  }

  /* If this was a kf or Gf note the Q used */
  if ((pc->frame_type == KEY_FRAME) ||
      pc->refresh_golden_frame || pc->refresh_alt_ref_frame) {
    pc->last_kf_gf_q = pc->base_qindex;
  }
  if (pc->refresh_entropy_probs) {
    if (pc->refresh_alt_ref_frame)
      vpx_memcpy(&pc->lfc_a, &pc->fc, sizeof(pc->fc));
    else
      vpx_memcpy(&pc->lfc, &pc->fc, sizeof(pc->fc));
  }

#ifdef PACKET_TESTING
  {
    FILE *f = fopen("decompressor.VP8", "ab");
    unsigned int size = residual_bc.pos + header_bc.pos + 8;
    fwrite((void *) &size, 4, 1, f);
    fwrite((void *) pbi->Source, size, 1, f);
    fclose(f);
  }
#endif
  // printf("Frame %d Done\n", frame_count++);

  /* Find the end of the coded buffer */
  while (residual_bc.count > CHAR_BIT
         && residual_bc.count < VP9_BD_VALUE_SIZE) {
    residual_bc.count -= CHAR_BIT;
    residual_bc.user_buffer--;
  }
  *p_data_end = residual_bc.user_buffer;
  return 0;
}
