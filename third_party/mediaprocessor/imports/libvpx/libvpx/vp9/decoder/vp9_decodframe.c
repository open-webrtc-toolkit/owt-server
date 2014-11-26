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
#include "vp9/common/vp9_tile_common.h"
#include "vp9_rtcd.h"

#include <assert.h>
#include <stdio.h>

#define COEFCOUNT_TESTING

// #define DEC_DEBUG
#ifdef DEC_DEBUG
int dec_debug = 0;
#endif

static int read_le16(const uint8_t *p) {
  return (p[1] << 8) | p[0];
}

static int read_le32(const uint8_t *p) {
  return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

// len == 0 is not allowed
static int read_is_valid(const unsigned char *start, size_t len,
                         const unsigned char *end) {
  return start + len > start && start + len <= end;
}

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

  v = merge_index(v, n - 1, modulus);
  if ((m << 1) <= n) {
    return vp9_inv_recenter_nonneg(v + 1, m);
  } else {
    return n - 1 - vp9_inv_recenter_nonneg(v + 1, n - 1 - m);
  }
}

static vp9_prob read_prob_diff_update(vp9_reader *const bc, int oldp) {
  int delp = vp9_decode_term_subexp(bc, SUBEXP_PARAM, 255);
  return (vp9_prob)inv_remap_prob(delp, oldp);
}

void vp9_init_de_quantizer(VP9D_COMP *pbi) {
  int i;
  int q;
  VP9_COMMON *const pc = &pbi->common;

  for (q = 0; q < QINDEX_RANGE; q++) {
    pc->Y1dequant[q][0] = (int16_t)vp9_dc_quant(q, pc->y1dc_delta_q);
    pc->UVdequant[q][0] = (int16_t)vp9_dc_uv_quant(q, pc->uvdc_delta_q);

    /* all the ac values =; */
    for (i = 1; i < 16; i++) {
      int rc = vp9_default_zig_zag1d_4x4[i];

      pc->Y1dequant[q][rc] = (int16_t)vp9_ac_yquant(q);
      pc->UVdequant[q][rc] = (int16_t)vp9_ac_uv_quant(q, pc->uvac_delta_q);
    }
  }
}

static int get_qindex(MACROBLOCKD *mb, int segment_id, int base_qindex) {
  // Set the Q baseline allowing for any segment level adjustment
  if (vp9_segfeature_active(mb, segment_id, SEG_LVL_ALT_Q)) {
    if (mb->mb_segment_abs_delta == SEGMENT_ABSDATA)
      return vp9_get_segdata(mb, segment_id, SEG_LVL_ALT_Q);  // Abs Value
    else
      return clamp(base_qindex + vp9_get_segdata(mb, segment_id, SEG_LVL_ALT_Q),
                   0, MAXQ);  // Delta Value
  } else {
    return base_qindex;
  }
}

static void mb_init_dequantizer(VP9D_COMP *pbi, MACROBLOCKD *mb) {
  int i;

  VP9_COMMON *const pc = &pbi->common;
  const int segment_id = mb->mode_info_context->mbmi.segment_id;
  const int qindex = get_qindex(mb, segment_id, pc->base_qindex);
  mb->q_index = qindex;

  for (i = 0; i < 16; i++)
    mb->block[i].dequant = pc->Y1dequant[qindex];

  for (i = 16; i < 24; i++)
    mb->block[i].dequant = pc->UVdequant[qindex];

  if (mb->lossless) {
    assert(qindex == 0);
    mb->inv_txm4x4_1      = vp9_short_iwalsh4x4_1;
    mb->inv_txm4x4        = vp9_short_iwalsh4x4;
    mb->itxm_add          = vp9_dequant_idct_add_lossless_c;
    mb->itxm_add_y_block  = vp9_dequant_idct_add_y_block_lossless_c;
    mb->itxm_add_uv_block = vp9_dequant_idct_add_uv_block_lossless_c;
  } else {
    mb->inv_txm4x4_1      = vp9_short_idct4x4_1;
    mb->inv_txm4x4        = vp9_short_idct4x4;
    mb->itxm_add          = vp9_dequant_idct_add;
    mb->itxm_add_y_block  = vp9_dequant_idct_add_y_block;
    mb->itxm_add_uv_block = vp9_dequant_idct_add_uv_block;
  }
}

/* skip_recon_mb() is Modified: Instead of writing the result to predictor buffer and then copying it
 *  to dst buffer, we can write the result directly to dst buffer. This eliminates unnecessary copy.
 */
static void skip_recon_mb(VP9D_COMP *pbi, MACROBLOCKD *xd,
                          int mb_row, int mb_col) {
  BLOCK_SIZE_TYPE sb_type = xd->mode_info_context->mbmi.sb_type;

  if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME) {
    if (sb_type == BLOCK_SIZE_SB64X64) {
      vp9_build_intra_predictors_sb64uv_s(xd);
      vp9_build_intra_predictors_sb64y_s(xd);
    } else if (sb_type == BLOCK_SIZE_SB32X32) {
      vp9_build_intra_predictors_sbuv_s(xd);
      vp9_build_intra_predictors_sby_s(xd);
    } else {
      vp9_build_intra_predictors_mbuv_s(xd);
      vp9_build_intra_predictors_mby_s(xd);
    }
  } else {
    if (sb_type == BLOCK_SIZE_SB64X64) {
      vp9_build_inter64x64_predictors_sb(xd,
                                         xd->dst.y_buffer,
                                         xd->dst.u_buffer,
                                         xd->dst.v_buffer,
                                         xd->dst.y_stride,
                                         xd->dst.uv_stride,
                                         mb_row, mb_col);
    } else if (sb_type == BLOCK_SIZE_SB32X32) {
      vp9_build_inter32x32_predictors_sb(xd,
                                         xd->dst.y_buffer,
                                         xd->dst.u_buffer,
                                         xd->dst.v_buffer,
                                         xd->dst.y_stride,
                                         xd->dst.uv_stride,
                                         mb_row, mb_col);
    } else {
      vp9_build_inter16x16_predictors_mb(xd,
                                         xd->dst.y_buffer,
                                         xd->dst.u_buffer,
                                         xd->dst.v_buffer,
                                         xd->dst.y_stride,
                                         xd->dst.uv_stride,
                                         mb_row, mb_col);
    }
  }
}

static void decode_16x16(VP9D_COMP *pbi, MACROBLOCKD *xd,
                         BOOL_DECODER* const bc) {
  TX_TYPE tx_type = get_tx_type_16x16(xd, 0);
#if 0  // def DEC_DEBUG
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
      xd->dst.uv_stride, xd);
}

static void decode_8x8(VP9D_COMP *pbi, MACROBLOCKD *xd,
                       BOOL_DECODER* const bc) {
  // First do Y
  // if the first one is DCT_DCT assume all the rest are as well
  TX_TYPE tx_type = get_tx_type_8x8(xd, 0);
#if 0  // def DEC_DEBUG
  if (dec_debug) {
    int i;
    printf("\n");
    printf("qcoeff 8x8\n");
    for (i = 0; i < 384; i++) {
      printf("%3d ", xd->qcoeff[i]);
      if (i % 16 == 15) printf("\n");
    }
  }
#endif
  if (tx_type != DCT_DCT || xd->mode_info_context->mbmi.mode == I8X8_PRED) {
    int i;
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
        vp9_intra8x8_predict(xd, b, i8x8mode, b->predictor);
      }
      tx_type = get_tx_type_8x8(xd, ib);
      if (tx_type != DCT_DCT) {
        vp9_ht_dequant_idct_add_8x8_c(tx_type, q, dq, pre, dst, 16, stride,
                                      xd->eobs[idx]);
      } else {
        vp9_dequant_idct_add_8x8_c(q, dq, pre, dst, 16, stride,
                                   xd->eobs[idx]);
      }
    }
  } else {
    vp9_dequant_idct_add_y_block_8x8(xd->qcoeff,
                                     xd->block[0].dequant,
                                     xd->predictor,
                                     xd->dst.y_buffer,
                                     xd->dst.y_stride,
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
      vp9_intra_uv4x4_predict(xd, b, i8x8mode, b->predictor);
      xd->itxm_add(b->qcoeff, b->dequant, b->predictor,
                   *(b->base_dst) + b->dst, 8, b->dst_stride, xd->eobs[16 + i]);

      b = &xd->block[20 + i];
      vp9_intra_uv4x4_predict(xd, b, i8x8mode, b->predictor);
      xd->itxm_add(b->qcoeff, b->dequant, b->predictor,
                   *(b->base_dst) + b->dst, 8, b->dst_stride, xd->eobs[20 + i]);
    }
  } else if (xd->mode_info_context->mbmi.mode == SPLITMV) {
    xd->itxm_add_uv_block(xd->qcoeff + 16 * 16, xd->block[16].dequant,
         xd->predictor + 16 * 16, xd->dst.u_buffer, xd->dst.v_buffer,
         xd->dst.uv_stride, xd);
  } else {
    vp9_dequant_idct_add_uv_block_8x8
        (xd->qcoeff + 16 * 16, xd->block[16].dequant,
         xd->predictor + 16 * 16, xd->dst.u_buffer, xd->dst.v_buffer,
         xd->dst.uv_stride, xd);
  }
#if 0  // def DEC_DEBUG
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
#if 0  // def DEC_DEBUG
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
  if (mode == I8X8_PRED) {
    for (i = 0; i < 4; i++) {
      int ib = vp9_i8x8_block[i];
      const int iblock[4] = {0, 1, 4, 5};
      int j;
      BLOCKD *b = &xd->block[ib];
      int i8x8mode = b->bmi.as_mode.first;
      vp9_intra8x8_predict(xd, b, i8x8mode, b->predictor);
      for (j = 0; j < 4; j++) {
        b = &xd->block[ib + iblock[j]];
        tx_type = get_tx_type_4x4(xd, ib + iblock[j]);
        if (tx_type != DCT_DCT) {
          vp9_ht_dequant_idct_add_c(tx_type, b->qcoeff,
                                    b->dequant, b->predictor,
                                    *(b->base_dst) + b->dst, 16,
                                    b->dst_stride, xd->eobs[ib + iblock[j]]);
        } else {
          xd->itxm_add(b->qcoeff, b->dequant, b->predictor,
                       *(b->base_dst) + b->dst, 16, b->dst_stride,
                       xd->eobs[ib + iblock[j]]);
        }
      }
      b = &xd->block[16 + i];
      vp9_intra_uv4x4_predict(xd, b, i8x8mode, b->predictor);
      xd->itxm_add(b->qcoeff, b->dequant, b->predictor,
                   *(b->base_dst) + b->dst, 8, b->dst_stride, xd->eobs[16 + i]);
      b = &xd->block[20 + i];
      vp9_intra_uv4x4_predict(xd, b, i8x8mode, b->predictor);
      xd->itxm_add(b->qcoeff, b->dequant, b->predictor,
                   *(b->base_dst) + b->dst, 8, b->dst_stride, xd->eobs[20 + i]);
    }
  } else if (mode == B_PRED) {
    for (i = 0; i < 16; i++) {
      BLOCKD *b = &xd->block[i];
      int b_mode = xd->mode_info_context->bmi[i].as_mode.first;
#if CONFIG_NEWBINTRAMODES
      xd->mode_info_context->bmi[i].as_mode.context = b->bmi.as_mode.context =
          vp9_find_bpred_context(xd, b);
#endif
      if (!xd->mode_info_context->mbmi.mb_skip_coeff)
        eobtotal += vp9_decode_coefs_4x4(pbi, xd, bc, PLANE_TYPE_Y_WITH_DC, i);

      vp9_intra4x4_predict(xd, b, b_mode, b->predictor);
      tx_type = get_tx_type_4x4(xd, i);
      if (tx_type != DCT_DCT) {
        vp9_ht_dequant_idct_add_c(tx_type, b->qcoeff,
                                  b->dequant, b->predictor,
                                  *(b->base_dst) + b->dst, 16, b->dst_stride,
                                  xd->eobs[i]);
      } else {
        xd->itxm_add(b->qcoeff, b->dequant, b->predictor,
                      *(b->base_dst) + b->dst, 16, b->dst_stride, xd->eobs[i]);
      }
    }
    if (!xd->mode_info_context->mbmi.mb_skip_coeff) {
      vp9_decode_mb_tokens_4x4_uv(pbi, xd, bc);
    }
    vp9_build_intra_predictors_mbuv(xd);
    xd->itxm_add_uv_block(xd->qcoeff + 16 * 16,
                           xd->block[16].dequant,
                           xd->predictor + 16 * 16,
                           xd->dst.u_buffer,
                           xd->dst.v_buffer,
                           xd->dst.uv_stride,
                           xd);
  } else if (mode == SPLITMV || get_tx_type_4x4(xd, 0) == DCT_DCT) {
    xd->itxm_add_y_block(xd->qcoeff,
                          xd->block[0].dequant,
                          xd->predictor,
                          xd->dst.y_buffer,
                          xd->dst.y_stride,
                          xd);
    xd->itxm_add_uv_block(xd->qcoeff + 16 * 16,
                           xd->block[16].dequant,
                           xd->predictor + 16 * 16,
                           xd->dst.u_buffer,
                           xd->dst.v_buffer,
                           xd->dst.uv_stride,
                           xd);
  } else {
#if 0  // def DEC_DEBUG
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
    for (i = 0; i < 16; i++) {
      BLOCKD *b = &xd->block[i];
      tx_type = get_tx_type_4x4(xd, i);
      if (tx_type != DCT_DCT) {
        vp9_ht_dequant_idct_add_c(tx_type, b->qcoeff,
                                  b->dequant, b->predictor,
                                  *(b->base_dst) + b->dst, 16,
                                  b->dst_stride, xd->eobs[i]);
      } else {
        xd->itxm_add(b->qcoeff, b->dequant, b->predictor,
                      *(b->base_dst) + b->dst, 16, b->dst_stride, xd->eobs[i]);
      }
    }
    xd->itxm_add_uv_block(xd->qcoeff + 16 * 16,
                           xd->block[16].dequant,
                           xd->predictor + 16 * 16,
                           xd->dst.u_buffer,
                           xd->dst.v_buffer,
                           xd->dst.uv_stride,
                           xd);
  }
}

static void decode_superblock64(VP9D_COMP *pbi, MACROBLOCKD *xd,
                                int mb_row, int mb_col,
                                BOOL_DECODER* const bc) {
  int n, eobtotal;
  VP9_COMMON *const pc = &pbi->common;
  MODE_INFO *mi = xd->mode_info_context;
  const int mis = pc->mode_info_stride;

  assert(xd->mode_info_context->mbmi.sb_type == BLOCK_SIZE_SB64X64);

  if (pbi->common.frame_type != KEY_FRAME)
    vp9_setup_interp_filters(xd, xd->mode_info_context->mbmi.interp_filter, pc);

  // re-initialize macroblock dequantizer before detokenization
  if (xd->segmentation_enabled)
    mb_init_dequantizer(pbi, xd);

  if (xd->mode_info_context->mbmi.mb_skip_coeff) {
    vp9_reset_sb64_tokens_context(xd);

    /* Special case:  Force the loopfilter to skip when eobtotal and
     * mb_skip_coeff are zero.
     */
    skip_recon_mb(pbi, xd, mb_row, mb_col);
    return;
  }

  /* do prediction */
  if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME) {
    vp9_build_intra_predictors_sb64y_s(xd);
    vp9_build_intra_predictors_sb64uv_s(xd);
  } else {
    vp9_build_inter64x64_predictors_sb(xd, xd->dst.y_buffer,
                                       xd->dst.u_buffer, xd->dst.v_buffer,
                                       xd->dst.y_stride, xd->dst.uv_stride,
                                       mb_row, mb_col);
  }

  /* dequantization and idct */
  eobtotal = vp9_decode_sb64_tokens(pbi, xd, bc);
  if (eobtotal == 0) {  // skip loopfilter
    for (n = 0; n < 16; n++) {
      const int x_idx = n & 3, y_idx = n >> 2;

      if (mb_col + x_idx < pc->mb_cols && mb_row + y_idx < pc->mb_rows)
        mi[y_idx * mis + x_idx].mbmi.mb_skip_coeff = mi->mbmi.mb_skip_coeff;
    }
  } else {
    switch (xd->mode_info_context->mbmi.txfm_size) {
      case TX_32X32:
        for (n = 0; n < 4; n++) {
          const int x_idx = n & 1, y_idx = n >> 1;
          const int y_offset = x_idx * 32 + y_idx * xd->dst.y_stride * 32;
          vp9_dequant_idct_add_32x32(xd->qcoeff + n * 1024,
              xd->block[0].dequant,
              xd->dst.y_buffer + y_offset,
              xd->dst.y_buffer + y_offset,
              xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n * 64]);
        }
        vp9_dequant_idct_add_32x32(xd->qcoeff + 4096,
            xd->block[16].dequant, xd->dst.u_buffer, xd->dst.u_buffer,
            xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[256]);
        vp9_dequant_idct_add_32x32(xd->qcoeff + 4096 + 1024,
            xd->block[20].dequant, xd->dst.v_buffer, xd->dst.v_buffer,
            xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[320]);
        break;
      case TX_16X16:
        for (n = 0; n < 16; n++) {
          const int x_idx = n & 3, y_idx = n >> 2;
          const int y_offset = y_idx * 16 * xd->dst.y_stride + x_idx * 16;
          const TX_TYPE tx_type = get_tx_type_16x16(xd,
                                                    (y_idx * 16 + x_idx) * 4);

          if (tx_type == DCT_DCT) {
            vp9_dequant_idct_add_16x16(xd->qcoeff + n * 256,
                xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n * 16]);
          } else {
            vp9_ht_dequant_idct_add_16x16_c(tx_type, xd->qcoeff + n * 256,
                xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n * 16]);
          }
        }
        for (n = 0; n < 4; n++) {
          const int x_idx = n & 1, y_idx = n >> 1;
          const int uv_offset = y_idx * 16 * xd->dst.uv_stride + x_idx * 16;
          vp9_dequant_idct_add_16x16(xd->qcoeff + 4096 + n * 256,
              xd->block[16].dequant,
              xd->dst.u_buffer + uv_offset,
              xd->dst.u_buffer + uv_offset,
              xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[256 + n * 16]);
          vp9_dequant_idct_add_16x16(xd->qcoeff + 4096 + 1024 + n * 256,
              xd->block[20].dequant,
              xd->dst.v_buffer + uv_offset,
              xd->dst.v_buffer + uv_offset,
              xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[320 + n * 16]);
        }
        break;
      case TX_8X8:
        for (n = 0; n < 64; n++) {
          const int x_idx = n & 7, y_idx = n >> 3;
          const int y_offset = y_idx * 8 * xd->dst.y_stride + x_idx * 8;
          const TX_TYPE tx_type = get_tx_type_8x8(xd, (y_idx * 16 + x_idx) * 2);
          if (tx_type == DCT_DCT) {
            vp9_dequant_idct_add_8x8_c(xd->qcoeff + n * 64,
                xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n * 4]);
          } else {
            vp9_ht_dequant_idct_add_8x8_c(tx_type, xd->qcoeff + n * 64,
                xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n * 4]);
          }
        }
        for (n = 0; n < 16; n++) {
          const int x_idx = n & 3, y_idx = n >> 2;
          const int uv_offset = y_idx * 8 * xd->dst.uv_stride + x_idx * 8;
          vp9_dequant_idct_add_8x8_c(xd->qcoeff + n * 64 + 4096,
              xd->block[16].dequant,
              xd->dst.u_buffer + uv_offset,
              xd->dst.u_buffer + uv_offset,
              xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[256 + n * 4]);
          vp9_dequant_idct_add_8x8_c(xd->qcoeff + n * 64 + 4096 + 1024,
              xd->block[20].dequant,
              xd->dst.v_buffer + uv_offset,
              xd->dst.v_buffer + uv_offset,
              xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[320 + n * 4]);
        }
        break;
      case TX_4X4:
        for (n = 0; n < 256; n++) {
          const int x_idx = n & 15, y_idx = n >> 4;
          const int y_offset = y_idx * 4 * xd->dst.y_stride + x_idx * 4;
          const TX_TYPE tx_type = get_tx_type_4x4(xd, y_idx * 16 + x_idx);
          if (tx_type == DCT_DCT) {
            xd->itxm_add(xd->qcoeff + n * 16, xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n]);
          } else {
            vp9_ht_dequant_idct_add_c(tx_type, xd->qcoeff + n * 16,
                xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n]);
          }
        }
        for (n = 0; n < 64; n++) {
          const int x_idx = n & 7, y_idx = n >> 3;
          const int uv_offset = y_idx * 4 * xd->dst.uv_stride + x_idx * 4;
          xd->itxm_add(xd->qcoeff + 4096 + n * 16,
              xd->block[16].dequant,
              xd->dst.u_buffer + uv_offset,
              xd->dst.u_buffer + uv_offset,
              xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[256 + n]);
          xd->itxm_add(xd->qcoeff + 4096 + 1024 + n * 16,
              xd->block[20].dequant,
              xd->dst.v_buffer + uv_offset,
              xd->dst.v_buffer + uv_offset,
              xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[320 + n]);
        }
        break;
      default: assert(0);
    }
  }
}

static void decode_superblock32(VP9D_COMP *pbi, MACROBLOCKD *xd,
                                int mb_row, int mb_col,
                                BOOL_DECODER* const bc) {
  int n, eobtotal;
  VP9_COMMON *const pc = &pbi->common;
  const int mis = pc->mode_info_stride;

  assert(xd->mode_info_context->mbmi.sb_type == BLOCK_SIZE_SB32X32);

  if (pbi->common.frame_type != KEY_FRAME)
    vp9_setup_interp_filters(xd, xd->mode_info_context->mbmi.interp_filter, pc);

  // re-initialize macroblock dequantizer before detokenization
  if (xd->segmentation_enabled)
    mb_init_dequantizer(pbi, xd);

  if (xd->mode_info_context->mbmi.mb_skip_coeff) {
    vp9_reset_sb_tokens_context(xd);

    /* Special case:  Force the loopfilter to skip when eobtotal and
     * mb_skip_coeff are zero.
     */
    skip_recon_mb(pbi, xd, mb_row, mb_col);
    return;
  }

  /* do prediction */
  if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME) {
    vp9_build_intra_predictors_sby_s(xd);
    vp9_build_intra_predictors_sbuv_s(xd);
  } else {
    vp9_build_inter32x32_predictors_sb(xd, xd->dst.y_buffer,
                                       xd->dst.u_buffer, xd->dst.v_buffer,
                                       xd->dst.y_stride, xd->dst.uv_stride,
                                       mb_row, mb_col);
  }

  /* dequantization and idct */
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
    switch (xd->mode_info_context->mbmi.txfm_size) {
      case TX_32X32:
        vp9_dequant_idct_add_32x32(xd->qcoeff, xd->block[0].dequant,
                                   xd->dst.y_buffer, xd->dst.y_buffer,
                                   xd->dst.y_stride, xd->dst.y_stride,
                                   xd->eobs[0]);
        vp9_dequant_idct_add_uv_block_16x16_c(xd->qcoeff + 1024,
                                              xd->block[16].dequant,
                                              xd->dst.u_buffer,
                                              xd->dst.v_buffer,
                                              xd->dst.uv_stride, xd);
        break;
      case TX_16X16:
        for (n = 0; n < 4; n++) {
          const int x_idx = n & 1, y_idx = n >> 1;
          const int y_offset = y_idx * 16 * xd->dst.y_stride + x_idx * 16;
          const TX_TYPE tx_type = get_tx_type_16x16(xd,
                                                    (y_idx * 8 + x_idx) * 4);
          if (tx_type == DCT_DCT) {
            vp9_dequant_idct_add_16x16(
                xd->qcoeff + n * 256, xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n * 16]);
          } else {
            vp9_ht_dequant_idct_add_16x16_c(tx_type, xd->qcoeff + n * 256,
                xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n * 16]);
          }
        }
        vp9_dequant_idct_add_uv_block_16x16_c(xd->qcoeff + 1024,
                                              xd->block[16].dequant,
                                              xd->dst.u_buffer,
                                              xd->dst.v_buffer,
                                              xd->dst.uv_stride, xd);
        break;
      case TX_8X8:
        for (n = 0; n < 16; n++) {
          const int x_idx = n & 3, y_idx = n >> 2;
          const int y_offset = y_idx * 8 * xd->dst.y_stride + x_idx * 8;
          const TX_TYPE tx_type = get_tx_type_8x8(xd, (y_idx * 8 + x_idx) * 2);
          if (tx_type == DCT_DCT) {
            vp9_dequant_idct_add_8x8_c(xd->qcoeff + n * 64,
                xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n * 4]);
          } else {
            vp9_ht_dequant_idct_add_8x8_c(tx_type, xd->qcoeff + n * 64,
                xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n * 4]);
          }
        }
        for (n = 0; n < 4; n++) {
          const int x_idx = n & 1, y_idx = n >> 1;
          const int uv_offset = y_idx * 8 * xd->dst.uv_stride + x_idx * 8;
          vp9_dequant_idct_add_8x8_c(xd->qcoeff + n * 64 + 1024,
              xd->block[16].dequant,
              xd->dst.u_buffer + uv_offset,
              xd->dst.u_buffer + uv_offset,
              xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[64 + n * 4]);
          vp9_dequant_idct_add_8x8_c(xd->qcoeff + n * 64 + 1280,
              xd->block[20].dequant,
              xd->dst.v_buffer + uv_offset,
              xd->dst.v_buffer + uv_offset,
              xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[80 + n * 4]);
        }
        break;
      case TX_4X4:
        for (n = 0; n < 64; n++) {
          const int x_idx = n & 7, y_idx = n >> 3;
          const int y_offset = y_idx * 4 * xd->dst.y_stride + x_idx * 4;

          const TX_TYPE tx_type = get_tx_type_4x4(xd, y_idx * 8 + x_idx);
          if (tx_type == DCT_DCT) {
            xd->itxm_add(xd->qcoeff + n * 16, xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n]);
          } else {
            vp9_ht_dequant_idct_add_c(tx_type, xd->qcoeff + n * 16,
                xd->block[0].dequant,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_buffer + y_offset,
                xd->dst.y_stride, xd->dst.y_stride, xd->eobs[n]);
          }
        }

        for (n = 0; n < 16; n++) {
          const int x_idx = n & 3, y_idx = n >> 2;
          const int uv_offset = y_idx * 4 * xd->dst.uv_stride + x_idx * 4;
          xd->itxm_add(xd->qcoeff + 1024 + n * 16,
              xd->block[16].dequant,
              xd->dst.u_buffer + uv_offset,
              xd->dst.u_buffer + uv_offset,
              xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[64 + n]);
          xd->itxm_add(xd->qcoeff + 1280 + n * 16,
              xd->block[20].dequant,
              xd->dst.v_buffer + uv_offset,
              xd->dst.v_buffer + uv_offset,
              xd->dst.uv_stride, xd->dst.uv_stride, xd->eobs[80 + n]);
        }
        break;
      default: assert(0);
    }
  }
}

static void decode_macroblock(VP9D_COMP *pbi, MACROBLOCKD *xd,
                              int mb_row, unsigned int mb_col,
                              BOOL_DECODER* const bc) {
  int eobtotal = 0;
  MB_PREDICTION_MODE mode;
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
    if (mode != B_PRED)
      eobtotal = vp9_decode_mb_tokens(pbi, xd, bc);
  }

  //mode = xd->mode_info_context->mbmi.mode;
  if (pbi->common.frame_type != KEY_FRAME)
    vp9_setup_interp_filters(xd, xd->mode_info_context->mbmi.interp_filter,
                             &pbi->common);

  if (eobtotal == 0 &&
      mode != B_PRED &&
      mode != SPLITMV &&
      mode != I8X8_PRED &&
      !bool_error(bc)) {
    /* Special case:  Force the loopfilter to skip when eobtotal and
       mb_skip_coeff are zero. */
    xd->mode_info_context->mbmi.mb_skip_coeff = 1;
    skip_recon_mb(pbi, xd, mb_row, mb_col);
    return;
  }
#if 0  // def DEC_DEBUG
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
#if 0  // def DEC_DEBUG
  if (dec_debug)
    printf("Decoding mb:  %d %d interp %d\n",
           xd->mode_info_context->mbmi.mode, tx_size,
           xd->mode_info_context->mbmi.interp_filter);
#endif
    vp9_build_inter_predictors_mb(xd, mb_row, mb_col);
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
    printf("predictor y\n");
    for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++)
        printf("%3d ", xd->predictor[i * 16 + j]);
      printf("\n");
    }
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

  // Distance of Mb to the various image edges.
  // These are specified to 8th pel as they are always compared to
  // values that are in 1/8th pel units
  block_size >>= 4;  // in mb units

  set_mb_row(cm, xd, mb_row, block_size);
  set_mb_col(cm, xd, mb_col, block_size);

  xd->dst.y_buffer = cm->yv12_fb[dst_fb_idx].y_buffer + recon_yoffset;
  xd->dst.u_buffer = cm->yv12_fb[dst_fb_idx].u_buffer + recon_uvoffset;
  xd->dst.v_buffer = cm->yv12_fb[dst_fb_idx].v_buffer + recon_uvoffset;
}

static void set_refs(VP9D_COMP *pbi, int block_size, int mb_row, int mb_col) {
  VP9_COMMON *const cm = &pbi->common;
  MACROBLOCKD *const xd = &pbi->mb;
  MB_MODE_INFO *const mbmi = &xd->mode_info_context->mbmi;

  if (mbmi->ref_frame > INTRA_FRAME) {
    // Select the appropriate reference frame for this MB
    int ref_fb_idx = cm->active_ref_idx[mbmi->ref_frame - 1];
    xd->scale_factor[0] = cm->active_ref_scale[mbmi->ref_frame - 1];
    xd->scale_factor_uv[0] = cm->active_ref_scale[mbmi->ref_frame - 1];
    setup_pred_block(&xd->pre, &cm->yv12_fb[ref_fb_idx], mb_row, mb_col,
                     &xd->scale_factor[0], &xd->scale_factor_uv[0]);

    // propagate errors from reference frames
    xd->corrupted |= cm->yv12_fb[ref_fb_idx].corrupted;

    if (mbmi->second_ref_frame > INTRA_FRAME) {
      // Select the appropriate reference frame for this MB
      int second_ref_fb_idx = cm->active_ref_idx[mbmi->second_ref_frame - 1];

      setup_pred_block(&xd->second_pre, &cm->yv12_fb[second_ref_fb_idx],
                       mb_row, mb_col,
                       &xd->scale_factor[1], &xd->scale_factor_uv[1]);

      // propagate errors from reference frames
      xd->corrupted |= cm->yv12_fb[second_ref_fb_idx].corrupted;
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

  for (mb_col = pc->cur_tile_mb_col_start;
       mb_col < pc->cur_tile_mb_col_end; mb_col += 4) {
    if (vp9_read(bc, pc->sb64_coded)) {
#ifdef DEC_DEBUG
      dec_debug = (pc->current_video_frame == 11 && pc->show_frame &&
                   mb_row == 8 && mb_col == 0);
      if (dec_debug)
        printf("Debug Decode SB64\n");
#endif
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
#ifdef DEC_DEBUG
          dec_debug = (pc->current_video_frame == 11 && pc->show_frame &&
                       mb_row + y_idx_sb == 8 && mb_col + x_idx_sb == 0);
          if (dec_debug)
            printf("Debug Decode SB32\n");
#endif
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
#ifdef DEC_DEBUG
            dec_debug = (pc->current_video_frame == 11 && pc->show_frame &&
                         mb_row + y_idx == 8 && mb_col + x_idx == 0);
            if (dec_debug)
              printf("Debug Decode MB\n");
#endif

            set_offsets(pbi, 16, mb_row + y_idx, mb_col + x_idx);
            xd->mb_index = i;
            vp9_decode_mb_mode_mv(pbi, xd, mb_row + y_idx, mb_col + x_idx, bc);
            set_refs(pbi, 16, mb_row + y_idx, mb_col + x_idx);
            decode_macroblock(pbi, xd, mb_row + y_idx, mb_col + x_idx, bc);

            /* check if the boolean decoder has suffered an error */
            xd->corrupted |= bool_error(bc);
          }
        }
      }
    }
  }
}


static void setup_token_decoder(VP9D_COMP *pbi,
                                const unsigned char *cx_data,
                                BOOL_DECODER* const bool_decoder) {
  VP9_COMMON *pc = &pbi->common;
  const unsigned char *user_data_end = pbi->Source + pbi->source_sz;
  const unsigned char *partition = cx_data;
  ptrdiff_t bytes_left = user_data_end - partition;
  ptrdiff_t partition_size = bytes_left;

  // Validate the calculated partition length. If the buffer
  // described by the partition can't be fully read, then restrict
  // it to the portion that can be (for EC mode) or throw an error.
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
  MACROBLOCKD *const xd = &pbi->mb;

  if (pc->frame_type == KEY_FRAME) {
    vp9_setup_past_independence(pc, xd);
    // All buffers are implicitly updated on key frames.
    pbi->refresh_frame_flags = (1 << NUM_REF_FRAMES) - 1;
  } else if (pc->error_resilient_mode) {
    vp9_setup_past_independence(pc, xd);
  }

  if (pc->frame_type != KEY_FRAME) {
    pc->mcomp_filter_type = pc->use_bilinear_mc_filter ? BILINEAR : EIGHTTAP;

    // To enable choice of different interpolation filters
    vp9_setup_interp_filters(xd, pc->mcomp_filter_type, pc);
  }

  xd->mode_info_context = pc->mi;
  xd->prev_mode_info_context = pc->prev_mi;
  xd->frame_type = pc->frame_type;
  xd->mode_info_context->mbmi.mode = DC_PRED;
  xd->mode_info_stride = pc->mode_info_stride;
  xd->corrupted = 0;
  xd->fullpixel_mask = pc->full_pixel ? 0xfffffff8 : 0xffffffff;
}

#if CONFIG_CODE_NONZEROCOUNT
static void read_nzc_probs_common(VP9_COMMON *cm,
                                  BOOL_DECODER* const bc,
                                  int block_size) {
  int c, r, b, t;
  int tokens, nodes;
  vp9_prob *nzc_probs;
  vp9_prob upd;

  if (!vp9_read_bit(bc)) return;

  if (block_size == 32) {
    tokens = NZC32X32_TOKENS;
    nzc_probs = cm->fc.nzc_probs_32x32[0][0][0];
    upd = NZC_UPDATE_PROB_32X32;
  } else if (block_size == 16) {
    tokens = NZC16X16_TOKENS;
    nzc_probs = cm->fc.nzc_probs_16x16[0][0][0];
    upd = NZC_UPDATE_PROB_16X16;
  } else if (block_size == 8) {
    tokens = NZC8X8_TOKENS;
    nzc_probs = cm->fc.nzc_probs_8x8[0][0][0];
    upd = NZC_UPDATE_PROB_8X8;
  } else {
    tokens = NZC4X4_TOKENS;
    nzc_probs = cm->fc.nzc_probs_4x4[0][0][0];
    upd = NZC_UPDATE_PROB_4X4;
  }
  nodes = tokens - 1;
  for (c = 0; c < MAX_NZC_CONTEXTS; ++c) {
    for (r = 0; r < REF_TYPES; ++r) {
      for (b = 0; b < BLOCK_TYPES; ++b) {
        int offset = c * REF_TYPES * BLOCK_TYPES + r * BLOCK_TYPES + b;
        int offset_nodes = offset * nodes;
        for (t = 0; t < nodes; ++t) {
          vp9_prob *p = &nzc_probs[offset_nodes + t];
          if (vp9_read(bc, upd)) {
            *p = read_prob_diff_update(bc, *p);
          }
        }
      }
    }
  }
}

static void read_nzc_pcat_probs(VP9_COMMON *cm, BOOL_DECODER* const bc) {
  int c, t, b;
  vp9_prob upd = NZC_UPDATE_PROB_PCAT;
  if (!vp9_read_bit(bc)) {
    return;
  }
  for (c = 0; c < MAX_NZC_CONTEXTS; ++c) {
    for (t = 0; t < NZC_TOKENS_EXTRA; ++t) {
      int bits = vp9_extranzcbits[t + NZC_TOKENS_NOEXTRA];
      for (b = 0; b < bits; ++b) {
        vp9_prob *p = &cm->fc.nzc_pcat_probs[c][t][b];
        if (vp9_read(bc, upd)) {
          *p = read_prob_diff_update(bc, *p);
        }
      }
    }
  }
}

static void read_nzc_probs(VP9_COMMON *cm,
                           BOOL_DECODER* const bc) {
  read_nzc_probs_common(cm, bc, 4);
  if (cm->txfm_mode != ONLY_4X4)
    read_nzc_probs_common(cm, bc, 8);
  if (cm->txfm_mode > ALLOW_8X8)
    read_nzc_probs_common(cm, bc, 16);
  if (cm->txfm_mode > ALLOW_16X16)
    read_nzc_probs_common(cm, bc, 32);
#ifdef NZC_PCAT_UPDATE
  read_nzc_pcat_probs(cm, bc);
#endif
}
#endif  // CONFIG_CODE_NONZEROCOUNT

static void read_coef_probs_common(BOOL_DECODER* const bc,
                                   vp9_coeff_probs *coef_probs,
                                   int block_types) {
#if CONFIG_MODELCOEFPROB && MODEL_BASED_UPDATE
  const int entropy_nodes_update = UNCONSTRAINED_UPDATE_NODES;
#else
  const int entropy_nodes_update = ENTROPY_NODES;
#endif

  int i, j, k, l, m;

  if (vp9_read_bit(bc)) {
    for (i = 0; i < block_types; i++) {
      for (j = 0; j < REF_TYPES; j++) {
        for (k = 0; k < COEF_BANDS; k++) {
          for (l = 0; l < PREV_COEF_CONTEXTS; l++) {
            if (l >= 3 && k == 0)
              continue;
            for (m = CONFIG_CODE_NONZEROCOUNT; m < entropy_nodes_update; m++) {
              vp9_prob *const p = coef_probs[i][j][k][l] + m;

              if (vp9_read(bc, vp9_coef_update_prob[m])) {
                *p = read_prob_diff_update(bc, *p);
#if CONFIG_MODELCOEFPROB && MODEL_BASED_UPDATE
                if (m == UNCONSTRAINED_NODES - 1)
                  vp9_get_model_distribution(*p, coef_probs[i][j][k][l], i, j);
#endif
              }
            }
          }
        }
      }
    }
  }
}

static void read_coef_probs(VP9D_COMP *pbi, BOOL_DECODER* const bc) {
  VP9_COMMON *const pc = &pbi->common;

  read_coef_probs_common(bc, pc->fc.coef_probs_4x4, BLOCK_TYPES);

  if (pbi->common.txfm_mode != ONLY_4X4)
    read_coef_probs_common(bc, pc->fc.coef_probs_8x8, BLOCK_TYPES);

  if (pbi->common.txfm_mode > ALLOW_8X8)
    read_coef_probs_common(bc, pc->fc.coef_probs_16x16, BLOCK_TYPES);

  if (pbi->common.txfm_mode > ALLOW_16X16)
    read_coef_probs_common(bc, pc->fc.coef_probs_32x32, BLOCK_TYPES);
}

static void update_frame_size(VP9D_COMP *pbi) {
  VP9_COMMON *cm = &pbi->common;

  /* our internal buffers are always multiples of 16 */
  const int width = (cm->width + 15) & ~15;
  const int height = (cm->height + 15) & ~15;

  cm->mb_rows = height >> 4;
  cm->mb_cols = width >> 4;
  cm->MBs = cm->mb_rows * cm->mb_cols;
  cm->mode_info_stride = cm->mb_cols + 1;
  memset(cm->mip, 0,
        (cm->mb_cols + 1) * (cm->mb_rows + 1) * sizeof(MODE_INFO));
  vp9_update_mode_info_border(cm, cm->mip);

  cm->mi = cm->mip + cm->mode_info_stride + 1;
  cm->prev_mi = cm->prev_mip + cm->mode_info_stride + 1;
  vp9_update_mode_info_in_image(cm, cm->mi);
}

static void setup_segmentation(VP9_COMMON *pc, MACROBLOCKD *xd, vp9_reader *r) {
  int i, j;

  xd->segmentation_enabled = vp9_read_bit(r);
  if (xd->segmentation_enabled) {
    // Read whether or not the segmentation map is being explicitly updated
    // this frame.
    xd->update_mb_segmentation_map = vp9_read_bit(r);

    // If so what method will be used.
    if (xd->update_mb_segmentation_map) {
      // Which macro block level features are enabled. Read the probs used to
      // decode the segment id for each macro block.
      for (i = 0; i < MB_FEATURE_TREE_PROBS; i++) {
        xd->mb_segment_tree_probs[i] = vp9_read_bit(r) ? vp9_read_prob(r) : 255;
      }

      // Read the prediction probs needed to decode the segment id
      pc->temporal_update = vp9_read_bit(r);
      for (i = 0; i < PREDICTION_PROBS; i++) {
        pc->segment_pred_probs[i] = pc->temporal_update
            ? (vp9_read_bit(r) ? vp9_read_prob(r) : 255)
            : 255;
      }

      if (pc->temporal_update) {
        const vp9_prob *p = xd->mb_segment_tree_probs;
        vp9_prob *p_mod = xd->mb_segment_mispred_tree_probs;
        const int c0 =        p[0]  *        p[1];
        const int c1 =        p[0]  * (256 - p[1]);
        const int c2 = (256 - p[0]) *        p[2];
        const int c3 = (256 - p[0]) * (256 - p[2]);

        p_mod[0] = get_binary_prob(c1, c2 + c3);
        p_mod[1] = get_binary_prob(c0, c2 + c3);
        p_mod[2] = get_binary_prob(c0 + c1, c3);
        p_mod[3] = get_binary_prob(c0 + c1, c2);
      }
    }

    xd->update_mb_segmentation_data = vp9_read_bit(r);
    if (xd->update_mb_segmentation_data) {
      int data;

      xd->mb_segment_abs_delta = vp9_read_bit(r);

      vp9_clearall_segfeatures(xd);

      // For each segmentation...
      for (i = 0; i < MAX_MB_SEGMENTS; i++) {
        // For each of the segments features...
        for (j = 0; j < SEG_LVL_MAX; j++) {
          // Is the feature enabled
          if (vp9_read_bit(r)) {
            // Update the feature data and mask
            vp9_enable_segfeature(xd, i, j);

            data = vp9_decode_unsigned_max(r, vp9_seg_feature_data_max(j));

            // Is the segment data signed..
            if (vp9_is_segfeature_signed(j)) {
              if (vp9_read_bit(r))
                data = -data;
            }
          } else {
            data = 0;
          }

          vp9_set_segdata(xd, i, j, data);
        }
      }
    }
  }
}

static void setup_loopfilter(VP9_COMMON *pc, MACROBLOCKD *xd, vp9_reader *r) {
  int i;

  pc->filter_type = (LOOPFILTERTYPE) vp9_read_bit(r);
  pc->filter_level = vp9_read_literal(r, 6);
  pc->sharpness_level = vp9_read_literal(r, 3);

#if CONFIG_LOOP_DERING
  if (vp9_read_bit(r))
    pc->dering_enabled = 1 + vp9_read_literal(r, 4);
  else
    pc->dering_enabled = 0;
#endif

  // Read in loop filter deltas applied at the MB level based on mode or ref
  // frame.
  xd->mode_ref_lf_delta_update = 0;
  xd->mode_ref_lf_delta_enabled = vp9_read_bit(r);

  if (xd->mode_ref_lf_delta_enabled) {
    // Do the deltas need to be updated
    xd->mode_ref_lf_delta_update = vp9_read_bit(r);

    if (xd->mode_ref_lf_delta_update) {
      // Send update
      for (i = 0; i < MAX_REF_LF_DELTAS; i++) {
        if (vp9_read_bit(r)) {
          // sign = vp9_read_bit(r);
          xd->ref_lf_deltas[i] = vp9_read_literal(r, 6);

          if (vp9_read_bit(r))
            xd->ref_lf_deltas[i] = -xd->ref_lf_deltas[i];  // Apply sign
        }
      }

      // Send update
      for (i = 0; i < MAX_MODE_LF_DELTAS; i++) {
        if (vp9_read_bit(r)) {
          // sign = vp9_read_bit(r);
          xd->mode_lf_deltas[i] = vp9_read_literal(r, 6);

          if (vp9_read_bit(r))
            xd->mode_lf_deltas[i] = -xd->mode_lf_deltas[i];  // Apply sign
        }
      }
    }
  }
}

static const uint8_t *setup_frame_size(VP9D_COMP *pbi, int scaling_active,
                                      const uint8_t *data,
                                      const uint8_t *data_end) {
  VP9_COMMON *const pc = &pbi->common;
  const int width = pc->width;
  const int height = pc->height;

  // If error concealment is enabled we should only parse the new size
  // if we have enough data. Otherwise we will end up with the wrong size.
  if (scaling_active && data + 4 < data_end) {
    pc->display_width = read_le16(data + 0);
    pc->display_height = read_le16(data + 2);
    data += 4;
  }

  if (data + 4 < data_end) {
    pc->width = read_le16(data + 0);
    pc->height = read_le16(data + 2);
    data += 4;
  }

  if (!scaling_active) {
    pc->display_width = pc->width;
    pc->display_height = pc->height;
  }

  if (width != pc->width || height != pc->height) {
    if (pc->width <= 0) {
      pc->width = width;
      vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                         "Invalid frame width");
    }

    if (pc->height <= 0) {
      pc->height = height;
      vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                         "Invalid frame height");
    }

    if (!pbi->initial_width || !pbi->initial_height) {
      if (vp9_alloc_frame_buffers(pc, pc->width, pc->height))
        vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                           "Failed to allocate frame buffers");
      pbi->initial_width = pc->width;
      pbi->initial_height = pc->height;
    }

    if (pc->width > pbi->initial_width) {
      vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                         "Frame width too large");
    }

    if (pc->height > pbi->initial_height) {
      vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                         "Frame height too large");
    }

    update_frame_size(pbi);
  }

  return data;
}

static void update_frame_context(VP9D_COMP *pbi, vp9_reader *r) {
  FRAME_CONTEXT *const fc = &pbi->common.fc;

  vp9_copy(fc->pre_coef_probs_4x4, fc->coef_probs_4x4);
  vp9_copy(fc->pre_coef_probs_8x8, fc->coef_probs_8x8);
  vp9_copy(fc->pre_coef_probs_16x16, fc->coef_probs_16x16);
  vp9_copy(fc->pre_coef_probs_32x32, fc->coef_probs_32x32);
  vp9_copy(fc->pre_ymode_prob, fc->ymode_prob);
  vp9_copy(fc->pre_sb_ymode_prob, fc->sb_ymode_prob);
  vp9_copy(fc->pre_uv_mode_prob, fc->uv_mode_prob);
  vp9_copy(fc->pre_bmode_prob, fc->bmode_prob);
  vp9_copy(fc->pre_i8x8_mode_prob, fc->i8x8_mode_prob);
  vp9_copy(fc->pre_sub_mv_ref_prob, fc->sub_mv_ref_prob);
  vp9_copy(fc->pre_mbsplit_prob, fc->mbsplit_prob);
  fc->pre_nmvc = fc->nmvc;

  vp9_zero(fc->coef_counts_4x4);
  vp9_zero(fc->coef_counts_8x8);
  vp9_zero(fc->coef_counts_16x16);
  vp9_zero(fc->coef_counts_32x32);
  vp9_zero(fc->eob_branch_counts);
  vp9_zero(fc->ymode_counts);
  vp9_zero(fc->sb_ymode_counts);
  vp9_zero(fc->uv_mode_counts);
  vp9_zero(fc->bmode_counts);
  vp9_zero(fc->i8x8_mode_counts);
  vp9_zero(fc->sub_mv_ref_counts);
  vp9_zero(fc->mbsplit_counts);
  vp9_zero(fc->NMVcount);
  vp9_zero(fc->mv_ref_ct);

#if CONFIG_COMP_INTERINTRA_PRED
  fc->pre_interintra_prob = fc->interintra_prob;
  vp9_zero(fc->interintra_counts);
#endif

#if CONFIG_CODE_NONZEROCOUNT
  vp9_copy(fc->pre_nzc_probs_4x4, fc->nzc_probs_4x4);
  vp9_copy(fc->pre_nzc_probs_8x8, fc->nzc_probs_8x8);
  vp9_copy(fc->pre_nzc_probs_16x16, fc->nzc_probs_16x16);
  vp9_copy(fc->pre_nzc_probs_32x32, fc->nzc_probs_32x32);
  vp9_copy(fc->pre_nzc_pcat_probs, fc->nzc_pcat_probs);

  vp9_zero(fc->nzc_counts_4x4);
  vp9_zero(fc->nzc_counts_8x8);
  vp9_zero(fc->nzc_counts_16x16);
  vp9_zero(fc->nzc_counts_32x32);
  vp9_zero(fc->nzc_pcat_counts);
#endif

  read_coef_probs(pbi, r);
#if CONFIG_CODE_NONZEROCOUNT
  read_nzc_probs(&pbi->common, r);
#endif
}

static void decode_tiles(VP9D_COMP *pbi,
                         const uint8_t *data, int first_partition_size,
                         BOOL_DECODER *header_bc, BOOL_DECODER *residual_bc) {
  VP9_COMMON *const pc = &pbi->common;
  MACROBLOCKD *const xd  = &pbi->mb;

  const uint8_t *data_ptr = data + first_partition_size;
  int tile_row, tile_col, delta_log2_tiles;
  int mb_row;

  vp9_get_tile_n_bits(pc, &pc->log2_tile_columns, &delta_log2_tiles);
  while (delta_log2_tiles--) {
    if (vp9_read_bit(header_bc)) {
      pc->log2_tile_columns++;
    } else {
      break;
    }
  }
  pc->log2_tile_rows = vp9_read_bit(header_bc);
  if (pc->log2_tile_rows)
    pc->log2_tile_rows += vp9_read_bit(header_bc);
  pc->tile_columns = 1 << pc->log2_tile_columns;
  pc->tile_rows    = 1 << pc->log2_tile_rows;

  vpx_memset(pc->above_context, 0,
             sizeof(ENTROPY_CONTEXT_PLANES) * pc->mb_cols);

  if (pbi->oxcf.inv_tile_order) {
    const int n_cols = pc->tile_columns;
    const uint8_t *data_ptr2[4][1 << 6];
    BOOL_DECODER bc_bak = {0};

    // pre-initialize the offsets, we're going to read in inverse order
    data_ptr2[0][0] = data_ptr;
    for (tile_row = 0; tile_row < pc->tile_rows; tile_row++) {
      if (tile_row) {
        const int size = read_le32(data_ptr2[tile_row - 1][n_cols - 1]);
        data_ptr2[tile_row - 1][n_cols - 1] += 4;
        data_ptr2[tile_row][0] = data_ptr2[tile_row - 1][n_cols - 1] + size;
      }

      for (tile_col = 1; tile_col < n_cols; tile_col++) {
        const int size = read_le32(data_ptr2[tile_row][tile_col - 1]);
        data_ptr2[tile_row][tile_col - 1] += 4;
        data_ptr2[tile_row][tile_col] =
            data_ptr2[tile_row][tile_col - 1] + size;
      }
    }

    for (tile_row = 0; tile_row < pc->tile_rows; tile_row++) {
      vp9_get_tile_row_offsets(pc, tile_row);
      for (tile_col = n_cols - 1; tile_col >= 0; tile_col--) {
        vp9_get_tile_col_offsets(pc, tile_col);
        setup_token_decoder(pbi, data_ptr2[tile_row][tile_col], residual_bc);

        // Decode a row of superblocks
        for (mb_row = pc->cur_tile_mb_row_start;
             mb_row < pc->cur_tile_mb_row_end; mb_row += 4) {
          decode_sb_row(pbi, pc, mb_row, xd, residual_bc);
        }

        if (tile_row == pc->tile_rows - 1 && tile_col == n_cols - 1)
          bc_bak = *residual_bc;
      }
    }
    *residual_bc = bc_bak;
  } else {
    int has_more;

    for (tile_row = 0; tile_row < pc->tile_rows; tile_row++) {
      vp9_get_tile_row_offsets(pc, tile_row);
      for (tile_col = 0; tile_col < pc->tile_columns; tile_col++) {
        vp9_get_tile_col_offsets(pc, tile_col);

        has_more = tile_col < pc->tile_columns - 1 ||
                   tile_row < pc->tile_rows - 1;

        // Setup decoder
        setup_token_decoder(pbi, data_ptr + (has_more ? 4 : 0), residual_bc);

        // Decode a row of superblocks
        for (mb_row = pc->cur_tile_mb_row_start;
             mb_row < pc->cur_tile_mb_row_end; mb_row += 4) {
          decode_sb_row(pbi, pc, mb_row, xd, residual_bc);
        }

        if (has_more) {
          const int size = read_le32(data_ptr);
          data_ptr += 4 + size;
        }
      }
    }
  }
}

int vp9_decode_frame(VP9D_COMP *pbi, const unsigned char **p_data_end) {
  BOOL_DECODER header_bc, residual_bc;
  VP9_COMMON *const pc = &pbi->common;
  MACROBLOCKD *const xd  = &pbi->mb;
  const uint8_t *data = (const uint8_t *)pbi->Source;
  const uint8_t *data_end = data + pbi->source_sz;
  ptrdiff_t first_partition_length_in_bytes = 0;
  int i, corrupt_tokens = 0;

  // printf("Decoding frame %d\n", pc->current_video_frame);

  xd->corrupted = 0;  // start with no corruption of current frame
  pc->yv12_fb[pc->new_fb_idx].corrupted = 0;

  if (data_end - data < 3) {
    vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME, "Truncated packet");
  } else {
    int scaling_active;
    pc->last_frame_type = pc->frame_type;
    pc->frame_type = (FRAME_TYPE)(data[0] & 1);
    pc->version = (data[0] >> 1) & 7;
    pc->show_frame = (data[0] >> 4) & 1;
    scaling_active = (data[0] >> 5) & 1;
    first_partition_length_in_bytes = read_le16(data + 1);

    if (!read_is_valid(data, first_partition_length_in_bytes, data_end))
      vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                         "Truncated packet or corrupt partition 0 length");

    data += 3;

    vp9_setup_version(pc);

    if (pc->frame_type == KEY_FRAME) {
      // When error concealment is enabled we should only check the sync
      // code if we have enough bits available
      if (data + 3 < data_end) {
        if (data[0] != 0x9d || data[1] != 0x01 || data[2] != 0x2a)
          vpx_internal_error(&pc->error, VPX_CODEC_UNSUP_BITSTREAM,
                             "Invalid frame sync code");
      }
      data += 3;
    }

    data = setup_frame_size(pbi, scaling_active, data, data_end);
  }

  if ((!pbi->decoded_key_frame && pc->frame_type != KEY_FRAME) ||
      pc->width == 0 || pc->height == 0) {
    return -1;
  }

  init_frame(pbi);

  // Reset the frame pointers to the current frame size
  vp8_yv12_realloc_frame_buffer(&pc->yv12_fb[pc->new_fb_idx],
                                pc->width, pc->height,
                                VP9BORDERINPIXELS);

  if (vp9_start_decode(&header_bc, data,
                       (unsigned int)first_partition_length_in_bytes))
    vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                       "Failed to allocate bool decoder 0");

  pc->clr_type = (YUV_TYPE)vp9_read_bit(&header_bc);
  pc->clamp_type = (CLAMP_TYPE)vp9_read_bit(&header_bc);
  pc->error_resilient_mode = vp9_read_bit(&header_bc);

  setup_segmentation(pc, xd, &header_bc);

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
        pc->ref_pred_probs[i] = vp9_read_prob(&header_bc);
    }
  }

  pc->sb64_coded = vp9_read_prob(&header_bc);
  pc->sb32_coded = vp9_read_prob(&header_bc);
  xd->lossless = vp9_read_bit(&header_bc);
  if (xd->lossless) {
    pc->txfm_mode = ONLY_4X4;
  } else {
    // Read the loop filter level and type
    pc->txfm_mode = vp9_read_literal(&header_bc, 2);
    if (pc->txfm_mode == ALLOW_32X32)
      pc->txfm_mode += vp9_read_bit(&header_bc);

    if (pc->txfm_mode == TX_MODE_SELECT) {
      pc->prob_tx[0] = vp9_read_prob(&header_bc);
      pc->prob_tx[1] = vp9_read_prob(&header_bc);
      pc->prob_tx[2] = vp9_read_prob(&header_bc);
    }
  }

  setup_loopfilter(pc, xd, &header_bc);

  // Dummy read for now
  vp9_read_literal(&header_bc, 2);

  /* Read the default quantizers. */
  {
    int q_update = 0;
    pc->base_qindex = vp9_read_literal(&header_bc, QINDEX_BITS);

    /* AC 1st order Q = default */
    pc->y1dc_delta_q = get_delta_q(&header_bc, pc->y1dc_delta_q, &q_update);
    pc->uvdc_delta_q = get_delta_q(&header_bc, pc->uvdc_delta_q, &q_update);
    pc->uvac_delta_q = get_delta_q(&header_bc, pc->uvac_delta_q, &q_update);

    if (q_update)
      vp9_init_de_quantizer(pbi);

    /* MB level dequantizer setup */
    mb_init_dequantizer(pbi, &pbi->mb);
  }

  // Determine if the golden frame or ARF buffer should be updated and how.
  // For all non key frames the GF and ARF refresh flags and sign bias
  // flags must be set explicitly.
  if (pc->frame_type == KEY_FRAME) {
    pc->active_ref_idx[0] = pc->new_fb_idx;
    pc->active_ref_idx[1] = pc->new_fb_idx;
    pc->active_ref_idx[2] = pc->new_fb_idx;
  } else {
    // Should the GF or ARF be updated from the current frame
    pbi->refresh_frame_flags = vp9_read_literal(&header_bc, NUM_REF_FRAMES);

    // Select active reference frames
    for (i = 0; i < 3; i++) {
      int ref_frame_num = vp9_read_literal(&header_bc, NUM_REF_FRAMES_LG2);
      pc->active_ref_idx[i] = pc->ref_frame_map[ref_frame_num];
    }

    pc->ref_frame_sign_bias[GOLDEN_FRAME] = vp9_read_bit(&header_bc);
    pc->ref_frame_sign_bias[ALTREF_FRAME] = vp9_read_bit(&header_bc);

    // Is high precision mv allowed
    xd->allow_high_precision_mv = vp9_read_bit(&header_bc);

    // Read the type of subpel filter to use
    pc->mcomp_filter_type = vp9_read_bit(&header_bc)
                                ? SWITCHABLE
                                : vp9_read_literal(&header_bc, 2);

#if CONFIG_COMP_INTERINTRA_PRED
    pc->use_interintra = vp9_read_bit(&header_bc);
#endif
    // To enable choice of different interploation filters
    vp9_setup_interp_filters(xd, pc->mcomp_filter_type, pc);
  }

  if (!pc->error_resilient_mode) {
    pc->refresh_entropy_probs = vp9_read_bit(&header_bc);
    pc->frame_parallel_decoding_mode = vp9_read_bit(&header_bc);
  } else {
    pc->refresh_entropy_probs = 0;
    pc->frame_parallel_decoding_mode = 1;
  }
  pc->frame_context_idx = vp9_read_literal(&header_bc, NUM_FRAME_CONTEXTS_LG2);
  vpx_memcpy(&pc->fc, &pc->frame_contexts[pc->frame_context_idx],
             sizeof(pc->fc));

  // Read inter mode probability context updates
  if (pc->frame_type != KEY_FRAME) {
    int i, j;
    for (i = 0; i < INTER_MODE_CONTEXTS; i++) {
      for (j = 0; j < 4; j++) {
        if (vp9_read(&header_bc, 252)) {
          pc->fc.vp9_mode_contexts[i][j] = vp9_read_prob(&header_bc);
        }
      }
    }
  }
#if CONFIG_MODELCOEFPROB && ADJUST_KF_COEF_PROBS
  if (pc->frame_type == KEY_FRAME)
    vp9_adjust_default_coef_probs(pc);
#endif

#if CONFIG_NEW_MVREF
  // If Key frame reset mv ref id probabilities to defaults
  if (pc->frame_type != KEY_FRAME) {
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
          xd->mb_mv_ref_probs[i][j] = vp9_read_prob(&header_bc);
        }
      }
    }
  }
#endif

  if (0) {
    FILE *z = fopen("decodestats.stt", "a");
    fprintf(z, "%6d F:%d,R:%d,Q:%d\n",
            pc->current_video_frame,
            pc->frame_type,
            pbi->refresh_frame_flags,
            pc->base_qindex);
    fclose(z);
  }

  update_frame_context(pbi, &header_bc);

  // Initialize xd pointers. Any reference should do for xd->pre, so use 0.
  vpx_memcpy(&xd->pre, &pc->yv12_fb[pc->active_ref_idx[0]],
             sizeof(YV12_BUFFER_CONFIG));
  vpx_memcpy(&xd->dst, &pc->yv12_fb[pc->new_fb_idx],
             sizeof(YV12_BUFFER_CONFIG));

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

  decode_tiles(pbi, data, first_partition_length_in_bytes,
               &header_bc, &residual_bc);
  corrupt_tokens |= xd->corrupted;

  // keep track of the last coded dimensions
  pc->last_width = pc->width;
  pc->last_height = pc->height;

  // Collect information about decoder corruption.
  // 1. Check first boolean decoder for errors.
  // 2. Check the macroblock information
  pc->yv12_fb[pc->new_fb_idx].corrupted = bool_error(&header_bc) |
                                          corrupt_tokens;

  if (!pbi->decoded_key_frame) {
    if (pc->frame_type == KEY_FRAME && !pc->yv12_fb[pc->new_fb_idx].corrupted)
      pbi->decoded_key_frame = 1;
    else
      vpx_internal_error(&pbi->common.error, VPX_CODEC_CORRUPT_FRAME,
                         "A stream must start with a complete key frame");
  }

  if (!pc->error_resilient_mode && !pc->frame_parallel_decoding_mode) {
    vp9_adapt_coef_probs(pc);
#if CONFIG_CODE_NONZEROCOUNT
    vp9_adapt_nzc_probs(pc);
#endif
  }

  if (pc->frame_type != KEY_FRAME) {
    if (!pc->error_resilient_mode && !pc->frame_parallel_decoding_mode) {
      vp9_adapt_mode_probs(pc);
      vp9_adapt_nmv_probs(pc, xd->allow_high_precision_mv);
      vp9_adapt_mode_context(&pbi->common);
    }
  }

  if (pc->refresh_entropy_probs) {
    vpx_memcpy(&pc->frame_contexts[pc->frame_context_idx], &pc->fc,
               sizeof(pc->fc));
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

  /* Find the end of the coded buffer */
  while (residual_bc.count > CHAR_BIT &&
         residual_bc.count < VP9_BD_VALUE_SIZE) {
    residual_bc.count -= CHAR_BIT;
    residual_bc.user_buffer--;
  }
  *p_data_end = residual_bc.user_buffer;
  return 0;
}
