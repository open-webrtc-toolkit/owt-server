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
#include "vpx_mem/vpx_mem.h"
#include "vp9/common/vp9_blockd.h"

#define build_intra_predictors_mbuv_prototype(sym) \
  void sym(unsigned char *dst, int dst_stride, \
           const unsigned char *src, int src_stride)
typedef build_intra_predictors_mbuv_prototype((*build_intra_pred_mbuv_fn_t));

extern build_intra_predictors_mbuv_prototype(vp9_intra_pred_uv_dc_mmx2);
extern build_intra_predictors_mbuv_prototype(vp9_intra_pred_uv_dctop_mmx2);
extern build_intra_predictors_mbuv_prototype(vp9_intra_pred_uv_dcleft_mmx2);
extern build_intra_predictors_mbuv_prototype(vp9_intra_pred_uv_dc128_mmx);
extern build_intra_predictors_mbuv_prototype(vp9_intra_pred_uv_ho_mmx2);
extern build_intra_predictors_mbuv_prototype(vp9_intra_pred_uv_ho_ssse3);
extern build_intra_predictors_mbuv_prototype(vp9_intra_pred_uv_ve_mmx);
extern build_intra_predictors_mbuv_prototype(vp9_intra_pred_uv_tm_sse2);
extern build_intra_predictors_mbuv_prototype(vp9_intra_pred_uv_tm_ssse3);

static void build_intra_predictors_mbuv_x86(MACROBLOCKD *xd,
                                            unsigned char *dst_u,
                                            unsigned char *dst_v,
                                            int dst_stride,
                                            build_intra_pred_mbuv_fn_t tm_fn,
                                            build_intra_pred_mbuv_fn_t ho_fn) {
  int mode = xd->mode_info_context->mbmi.uv_mode;
  build_intra_pred_mbuv_fn_t fn;
  int src_stride = xd->dst.uv_stride;

  switch (mode) {
    case  V_PRED:
      fn = vp9_intra_pred_uv_ve_mmx;
      break;
    case  H_PRED:
      fn = ho_fn;
      break;
    case TM_PRED:
      fn = tm_fn;
      break;
    case DC_PRED:
      if (xd->up_available) {
        if (xd->left_available) {
          fn = vp9_intra_pred_uv_dc_mmx2;
          break;
        } else {
          fn = vp9_intra_pred_uv_dctop_mmx2;
          break;
        }
      } else if (xd->left_available) {
        fn = vp9_intra_pred_uv_dcleft_mmx2;
        break;
      } else {
        fn = vp9_intra_pred_uv_dc128_mmx;
        break;
      }
      break;
    default:
      return;
  }

  fn(dst_u, dst_stride, xd->dst.u_buffer, src_stride);
  fn(dst_v, dst_stride, xd->dst.v_buffer, src_stride);
}

void vp9_build_intra_predictors_mbuv_sse2(MACROBLOCKD *xd) {
  build_intra_predictors_mbuv_x86(xd, &xd->predictor[256],
                                  &xd->predictor[320], 8,
                                  vp9_intra_pred_uv_tm_sse2,
                                  vp9_intra_pred_uv_ho_mmx2);
}

void vp9_build_intra_predictors_mbuv_ssse3(MACROBLOCKD *xd) {
  build_intra_predictors_mbuv_x86(xd, &xd->predictor[256],
                                  &xd->predictor[320], 8,
                                  vp9_intra_pred_uv_tm_ssse3,
                                  vp9_intra_pred_uv_ho_ssse3);
}

void vp9_build_intra_predictors_mbuv_s_sse2(MACROBLOCKD *xd) {
  build_intra_predictors_mbuv_x86(xd, xd->dst.u_buffer,
                                  xd->dst.v_buffer, xd->dst.uv_stride,
                                  vp9_intra_pred_uv_tm_sse2,
                                  vp9_intra_pred_uv_ho_mmx2);
}

void vp9_build_intra_predictors_mbuv_s_ssse3(MACROBLOCKD *xd) {
  build_intra_predictors_mbuv_x86(xd, xd->dst.u_buffer,
                                  xd->dst.v_buffer, xd->dst.uv_stride,
                                  vp9_intra_pred_uv_tm_ssse3,
                                  vp9_intra_pred_uv_ho_ssse3);
}
