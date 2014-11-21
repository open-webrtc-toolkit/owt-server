/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_RECONINTER_H_
#define VP9_COMMON_VP9_RECONINTER_H_

#include "vpx/vpx_integer.h"
#include "vp9/common/vp9_onyxc_int.h"

struct subpix_fn_table;

void vp9_build_inter16x16_predictors_mby(MACROBLOCKD *xd,
                                         uint8_t *dst_y,
                                         int dst_ystride,
                                         int mb_row,
                                         int mb_col);

void vp9_build_inter16x16_predictors_mbuv(MACROBLOCKD *xd,
                                          uint8_t *dst_u,
                                          uint8_t *dst_v,
                                          int dst_uvstride,
                                          int mb_row,
                                          int mb_col);

void vp9_build_inter16x16_predictors_mb(MACROBLOCKD *xd,
                                        uint8_t *dst_y,
                                        uint8_t *dst_u,
                                        uint8_t *dst_v,
                                        int dst_ystride,
                                        int dst_uvstride,
                                        int mb_row,
                                        int mb_col);

void vp9_build_inter32x32_predictors_sb(MACROBLOCKD *x,
                                        uint8_t *dst_y,
                                        uint8_t *dst_u,
                                        uint8_t *dst_v,
                                        int dst_ystride,
                                        int dst_uvstride,
                                        int mb_row,
                                        int mb_col);

void vp9_build_inter64x64_predictors_sb(MACROBLOCKD *x,
                                        uint8_t *dst_y,
                                        uint8_t *dst_u,
                                        uint8_t *dst_v,
                                        int dst_ystride,
                                        int dst_uvstride,
                                        int mb_row,
                                        int mb_col);

void vp9_build_inter_predictors_mb(MACROBLOCKD *xd,
                                   int mb_row,
                                   int mb_col);

void vp9_build_inter4x4_predictors_mbuv(MACROBLOCKD *xd,
                                        int mb_row,
                                        int mb_col);

void vp9_setup_interp_filters(MACROBLOCKD *xd,
                              INTERPOLATIONFILTERTYPE filter,
                              VP9_COMMON *cm);

void vp9_setup_scale_factors_for_frame(struct scale_factors *scale,
                                       YV12_BUFFER_CONFIG *other,
                                       int this_w, int this_h);

void vp9_build_inter_predictor(const uint8_t *src, int src_stride,
                               uint8_t *dst, int dst_stride,
                               const int_mv *mv_q3,
                               const struct scale_factors *scale,
                               int w, int h, int do_avg,
                               const struct subpix_fn_table *subpix);

void vp9_build_inter_predictor_q4(const uint8_t *src, int src_stride,
                                  uint8_t *dst, int dst_stride,
                                  const int_mv *fullpel_mv_q3,
                                  const int_mv *frac_mv_q4,
                                  const struct scale_factors *scale,
                                  int w, int h, int do_avg,
                                  const struct subpix_fn_table *subpix);

static int scale_value_x(int val, const struct scale_factors *scale) {
  return val * scale->x_num / scale->x_den;
}

static int scale_value_y(int val, const struct scale_factors *scale) {
  return val * scale->y_num / scale->y_den;
}

static int scaled_buffer_offset(int x_offset,
                                int y_offset,
                                int stride,
                                const struct scale_factors *scale) {
  return scale_value_y(y_offset, scale) * stride +
      scale_value_x(x_offset, scale);
}

static void setup_pred_block(YV12_BUFFER_CONFIG *dst,
                             const YV12_BUFFER_CONFIG *src,
                             int mb_row, int mb_col,
                             const struct scale_factors *scale,
                             const struct scale_factors *scale_uv) {
  const int recon_y_stride = src->y_stride;
  const int recon_uv_stride = src->uv_stride;
  int recon_yoffset;
  int recon_uvoffset;

  if (scale) {
    recon_yoffset = scaled_buffer_offset(16 * mb_col, 16 * mb_row,
                                         recon_y_stride, scale);
    recon_uvoffset = scaled_buffer_offset(8 * mb_col, 8 * mb_row,
                                          recon_uv_stride, scale_uv);
  } else {
    recon_yoffset = 16 * mb_row * recon_y_stride + 16 * mb_col;
    recon_uvoffset = 8 * mb_row * recon_uv_stride + 8 * mb_col;
  }
  *dst = *src;
  dst->y_buffer += recon_yoffset;
  dst->u_buffer += recon_uvoffset;
  dst->v_buffer += recon_uvoffset;
}

static void set_scale_factors(MACROBLOCKD *xd,
    int ref0, int ref1,
    struct scale_factors scale_factor[MAX_REF_FRAMES]) {

  xd->scale_factor[0] = scale_factor[ref0 >= 0 ? ref0 : 0];
  xd->scale_factor[1] = scale_factor[ref1 >= 0 ? ref1 : 0];
  xd->scale_factor_uv[0] = xd->scale_factor[0];
  xd->scale_factor_uv[1] = xd->scale_factor[1];
}

#endif  // VP9_COMMON_VP9_RECONINTER_H_
