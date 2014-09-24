/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "./vpx_config.h"
#include "vp9_rtcd.h"
#include "vp9/encoder/vp9_quantize.h"
#include "vp9/common/vp9_reconintra.h"
#include "vp9/common/vp9_reconintra4x4.h"
#include "vp9/encoder/vp9_encodemb.h"
#include "vp9/common/vp9_invtrans.h"
#include "vp9/encoder/vp9_encodeintra.h"

int vp9_encode_intra(VP9_COMP *cpi, MACROBLOCK *x, int use_16x16_pred) {
  int i;
  int intra_pred_var = 0;
  MB_MODE_INFO * mbmi = &x->e_mbd.mode_info_context->mbmi;
  (void) cpi;

  if (use_16x16_pred) {
    mbmi->mode = DC_PRED;
    mbmi->uv_mode = DC_PRED;
    mbmi->ref_frame = INTRA_FRAME;

    vp9_encode_intra16x16mby(x);
  } else {
    for (i = 0; i < 16; i++) {
      x->e_mbd.block[i].bmi.as_mode.first = B_DC_PRED;
      vp9_encode_intra4x4block(x, i);
    }
  }

  intra_pred_var = vp9_get_mb_ss(x->src_diff);

  return intra_pred_var;
}

void vp9_encode_intra4x4block(MACROBLOCK *x, int ib) {
  BLOCKD *b = &x->e_mbd.block[ib];
  BLOCK *be = &x->block[ib];
  TX_TYPE tx_type;

#if CONFIG_NEWBINTRAMODES
  b->bmi.as_mode.context = vp9_find_bpred_context(b);
#endif

  vp9_intra4x4_predict(b, b->bmi.as_mode.first, b->predictor);
  vp9_subtract_b(be, b, 16);

  tx_type = get_tx_type_4x4(&x->e_mbd, b);
  if (tx_type != DCT_DCT) {
    vp9_fht(be->src_diff, 32, be->coeff, tx_type, 4);
    vp9_ht_quantize_b_4x4(be, b, tx_type);
    vp9_ihtllm(b->dqcoeff, b->diff, 32, tx_type, 4, b->eob);
  } else {
    x->vp9_short_fdct4x4(be->src_diff, be->coeff, 32);
    x->quantize_b_4x4(be, b) ;
    vp9_inverse_transform_b_4x4(&x->e_mbd, ib, 32);
  }

  vp9_recon_b(b->predictor, b->diff, *(b->base_dst) + b->dst, b->dst_stride);
}

void vp9_encode_intra4x4mby(MACROBLOCK *mb) {
  int i;

  for (i = 0; i < 16; i++)
    vp9_encode_intra4x4block(mb, i);
  return;
}

void vp9_encode_intra16x16mby(MACROBLOCK *x) {
  MACROBLOCKD *xd = &x->e_mbd;
  BLOCK *b = &x->block[0];
  TX_SIZE tx_size = xd->mode_info_context->mbmi.txfm_size;

  vp9_build_intra_predictors_mby(xd);

  vp9_subtract_mby(x->src_diff, *(b->base_src), xd->predictor, b->src_stride);

  if (tx_size == TX_16X16) {
    vp9_transform_mby_16x16(x);
    vp9_quantize_mby_16x16(x);
    if (x->optimize)
      vp9_optimize_mby_16x16(x);
    vp9_inverse_transform_mby_16x16(xd);
  } else if (tx_size == TX_8X8) {
    vp9_transform_mby_8x8(x);
    vp9_quantize_mby_8x8(x);
    if (x->optimize)
      vp9_optimize_mby_8x8(x);
    vp9_inverse_transform_mby_8x8(xd);
  } else {
    vp9_transform_mby_4x4(x);
    vp9_quantize_mby_4x4(x);
    if (x->optimize)
      vp9_optimize_mby_4x4(x);
    vp9_inverse_transform_mby_4x4(xd);
  }

  vp9_recon_mby(xd);
}

void vp9_encode_intra16x16mbuv(MACROBLOCK *x) {
  MACROBLOCKD *xd = &x->e_mbd;
  TX_SIZE tx_size = xd->mode_info_context->mbmi.txfm_size;

  vp9_build_intra_predictors_mbuv(xd);

  vp9_subtract_mbuv(x->src_diff, x->src.u_buffer, x->src.v_buffer,
                    xd->predictor, x->src.uv_stride);

  if (tx_size == TX_4X4) {
    vp9_transform_mbuv_4x4(x);
    vp9_quantize_mbuv_4x4(x);
    if (x->optimize)
      vp9_optimize_mbuv_4x4(x);
    vp9_inverse_transform_mbuv_4x4(xd);
  } else /* 16x16 or 8x8 */ {
    vp9_transform_mbuv_8x8(x);
    vp9_quantize_mbuv_8x8(x);
    if (x->optimize)
      vp9_optimize_mbuv_8x8(x);
    vp9_inverse_transform_mbuv_8x8(xd);
  }

  vp9_recon_intra_mbuv(xd);
}

void vp9_encode_intra8x8(MACROBLOCK *x, int ib) {
  MACROBLOCKD *xd = &x->e_mbd;
  BLOCKD *b = &xd->block[ib];
  BLOCK *be = &x->block[ib];
  const int iblock[4] = {0, 1, 4, 5};
  int i;
  TX_TYPE tx_type;

  vp9_intra8x8_predict(b, b->bmi.as_mode.first, b->predictor);
  // generate residual blocks
  vp9_subtract_4b_c(be, b, 16);

  if (xd->mode_info_context->mbmi.txfm_size == TX_8X8) {
    int idx = (ib & 0x02) ? (ib + 2) : ib;

    tx_type = get_tx_type_8x8(xd, &xd->block[ib]);
    if (tx_type != DCT_DCT) {
      vp9_fht(be->src_diff, 32, (x->block + idx)->coeff,
                tx_type, 8);
      x->quantize_b_8x8(x->block + idx, xd->block + idx);
      vp9_ihtllm(xd->block[idx].dqcoeff, xd->block[ib].diff, 32,
                   tx_type, 8, xd->block[idx].eob);
    } else {
      x->vp9_short_fdct8x8(be->src_diff, (x->block + idx)->coeff, 32);
      x->quantize_b_8x8(x->block + idx, xd->block + idx);
      vp9_short_idct8x8(xd->block[idx].dqcoeff, xd->block[ib].diff, 32);
    }
  } else {
    for (i = 0; i < 4; i++) {
      b = &xd->block[ib + iblock[i]];
      be = &x->block[ib + iblock[i]];
      tx_type = get_tx_type_4x4(xd, b);
      if (tx_type != DCT_DCT) {
        vp9_fht_c(be->src_diff, 32, be->coeff, tx_type, 4);
        vp9_ht_quantize_b_4x4(be, b, tx_type);
        vp9_ihtllm(b->dqcoeff, b->diff, 32, tx_type, 4, b->eob);
      } else {
        x->vp9_short_fdct4x4(be->src_diff, be->coeff, 32);
        x->quantize_b_4x4(be, b);
        vp9_inverse_transform_b_4x4(xd, ib + iblock[i], 32);
      }
    }
  }

  // reconstruct submacroblock
  for (i = 0; i < 4; i++) {
    b = &xd->block[ib + iblock[i]];
    vp9_recon_b_c(b->predictor, b->diff, *(b->base_dst) + b->dst,
                  b->dst_stride);
  }
}

void vp9_encode_intra8x8mby(MACROBLOCK *x) {
  int i, ib;

  for (i = 0; i < 4; i++) {
    ib = vp9_i8x8_block[i];
    vp9_encode_intra8x8(x, ib);
  }
}

static void encode_intra_uv4x4(MACROBLOCK *x, int ib,
                               int mode) {
  BLOCKD *b = &x->e_mbd.block[ib];
  BLOCK *be = &x->block[ib];

  vp9_intra_uv4x4_predict(b, mode, b->predictor);

  vp9_subtract_b(be, b, 8);

  x->vp9_short_fdct4x4(be->src_diff, be->coeff, 16);
  x->quantize_b_4x4(be, b);
  vp9_inverse_transform_b_4x4(&x->e_mbd, ib, 16);

  vp9_recon_uv_b_c(b->predictor, b->diff, *(b->base_dst) + b->dst,
                   b->dst_stride);
}

void vp9_encode_intra8x8mbuv(MACROBLOCK *x) {
  int i, ib, mode;
  BLOCKD *b;

  for (i = 0; i < 4; i++) {
    ib = vp9_i8x8_block[i];
    b = &x->e_mbd.block[ib];
    mode = b->bmi.as_mode.first;

    /*u */
    encode_intra_uv4x4(x, i + 16, mode);
    /*v */
    encode_intra_uv4x4(x, i + 20, mode);
  }
}
