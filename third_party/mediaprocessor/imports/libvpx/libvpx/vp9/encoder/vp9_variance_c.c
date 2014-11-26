/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/encoder/vp9_variance.h"
#include "vp9/common/vp9_filter.h"
#include "vp9/common/vp9_subpelvar.h"
#include "vpx/vpx_integer.h"

unsigned int vp9_get_mb_ss_c(const int16_t *src_ptr) {
  unsigned int i, sum = 0;

  for (i = 0; i < 256; i++) {
    sum += (src_ptr[i] * src_ptr[i]);
  }

  return sum;
}

unsigned int vp9_variance64x64_c(const uint8_t *src_ptr,
                                 int  source_stride,
                                 const uint8_t *ref_ptr,
                                 int  recon_stride,
                                 unsigned int *sse) {
  unsigned int var;
  int avg;

  variance(src_ptr, source_stride, ref_ptr, recon_stride, 64, 64, &var, &avg);
  *sse = var;
  return (var - (((int64_t)avg * avg) >> 12));
}

unsigned int vp9_variance32x32_c(const uint8_t *src_ptr,
                                 int  source_stride,
                                 const uint8_t *ref_ptr,
                                 int  recon_stride,
                                 unsigned int *sse) {
  unsigned int var;
  int avg;

  variance(src_ptr, source_stride, ref_ptr, recon_stride, 32, 32, &var, &avg);
  *sse = var;
  return (var - (((int64_t)avg * avg) >> 10));
}

unsigned int vp9_variance16x16_c(const uint8_t *src_ptr,
                                 int  source_stride,
                                 const uint8_t *ref_ptr,
                                 int  recon_stride,
                                 unsigned int *sse) {
  unsigned int var;
  int avg;

  variance(src_ptr, source_stride, ref_ptr, recon_stride, 16, 16, &var, &avg);
  *sse = var;
  return (var - (((unsigned int)avg * avg) >> 8));
}

unsigned int vp9_variance8x16_c(const uint8_t *src_ptr,
                                int  source_stride,
                                const uint8_t *ref_ptr,
                                int  recon_stride,
                                unsigned int *sse) {
  unsigned int var;
  int avg;

  variance(src_ptr, source_stride, ref_ptr, recon_stride, 8, 16, &var, &avg);
  *sse = var;
  return (var - (((unsigned int)avg * avg) >> 7));
}

unsigned int vp9_variance16x8_c(const uint8_t *src_ptr,
                                int  source_stride,
                                const uint8_t *ref_ptr,
                                int  recon_stride,
                                unsigned int *sse) {
  unsigned int var;
  int avg;

  variance(src_ptr, source_stride, ref_ptr, recon_stride, 16, 8, &var, &avg);
  *sse = var;
  return (var - (((unsigned int)avg * avg) >> 7));
}


unsigned int vp9_variance8x8_c(const uint8_t *src_ptr,
                               int  source_stride,
                               const uint8_t *ref_ptr,
                               int  recon_stride,
                               unsigned int *sse) {
  unsigned int var;
  int avg;

  variance(src_ptr, source_stride, ref_ptr, recon_stride, 8, 8, &var, &avg);
  *sse = var;
  return (var - (((unsigned int)avg * avg) >> 6));
}

unsigned int vp9_variance4x4_c(const uint8_t *src_ptr,
                               int  source_stride,
                               const uint8_t *ref_ptr,
                               int  recon_stride,
                               unsigned int *sse) {
  unsigned int var;
  int avg;

  variance(src_ptr, source_stride, ref_ptr, recon_stride, 4, 4, &var, &avg);
  *sse = var;
  return (var - (((unsigned int)avg * avg) >> 4));
}


unsigned int vp9_mse16x16_c(const uint8_t *src_ptr,
                            int  source_stride,
                            const uint8_t *ref_ptr,
                            int  recon_stride,
                            unsigned int *sse) {
  unsigned int var;
  int avg;

  variance(src_ptr, source_stride, ref_ptr, recon_stride, 16, 16, &var, &avg);
  *sse = var;
  return var;
}


unsigned int vp9_sub_pixel_variance4x4_c(const uint8_t *src_ptr,
                                         int  src_pixels_per_line,
                                         int  xoffset,
                                         int  yoffset,
                                         const uint8_t *dst_ptr,
                                         int dst_pixels_per_line,
                                         unsigned int *sse) {
  uint8_t temp2[20 * 16];
  const int16_t *HFilter, *VFilter;
  uint16_t FData3[5 * 4];  // Temp data bufffer used in filtering

  HFilter = VP9_BILINEAR_FILTERS_2TAP(xoffset);
  VFilter = VP9_BILINEAR_FILTERS_2TAP(yoffset);

  // First filter 1d Horizontal
  var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line, 1, 5, 4, HFilter);

  // Now filter Verticaly
  var_filter_block2d_bil_second_pass(FData3, temp2, 4,  4,  4,  4, VFilter);

  return vp9_variance4x4_c(temp2, 4, dst_ptr, dst_pixels_per_line, sse);
}


unsigned int vp9_sub_pixel_variance8x8_c(const uint8_t *src_ptr,
                                         int  src_pixels_per_line,
                                         int  xoffset,
                                         int  yoffset,
                                         const uint8_t *dst_ptr,
                                         int dst_pixels_per_line,
                                         unsigned int *sse) {
  uint16_t FData3[9 * 8];  // Temp data bufffer used in filtering
  uint8_t temp2[20 * 16];
  const int16_t *HFilter, *VFilter;

  HFilter = VP9_BILINEAR_FILTERS_2TAP(xoffset);
  VFilter = VP9_BILINEAR_FILTERS_2TAP(yoffset);

  var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line, 1, 9, 8, HFilter);
  var_filter_block2d_bil_second_pass(FData3, temp2, 8, 8, 8, 8, VFilter);

  return vp9_variance8x8_c(temp2, 8, dst_ptr, dst_pixels_per_line, sse);
}

unsigned int vp9_sub_pixel_variance16x16_c(const uint8_t *src_ptr,
                                           int  src_pixels_per_line,
                                           int  xoffset,
                                           int  yoffset,
                                           const uint8_t *dst_ptr,
                                           int dst_pixels_per_line,
                                           unsigned int *sse) {
  uint16_t FData3[17 * 16];  // Temp data bufffer used in filtering
  uint8_t temp2[20 * 16];
  const int16_t *HFilter, *VFilter;

  HFilter = VP9_BILINEAR_FILTERS_2TAP(xoffset);
  VFilter = VP9_BILINEAR_FILTERS_2TAP(yoffset);

  var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line, 1, 17, 16, HFilter);
  var_filter_block2d_bil_second_pass(FData3, temp2, 16, 16, 16, 16, VFilter);

  return vp9_variance16x16_c(temp2, 16, dst_ptr, dst_pixels_per_line, sse);
}

unsigned int vp9_sub_pixel_variance64x64_c(const uint8_t *src_ptr,
                                           int  src_pixels_per_line,
                                           int  xoffset,
                                           int  yoffset,
                                           const uint8_t *dst_ptr,
                                           int dst_pixels_per_line,
                                           unsigned int *sse) {
  uint16_t FData3[65 * 64];  // Temp data bufffer used in filtering
  uint8_t temp2[68 * 64];
  const int16_t *HFilter, *VFilter;

  HFilter = VP9_BILINEAR_FILTERS_2TAP(xoffset);
  VFilter = VP9_BILINEAR_FILTERS_2TAP(yoffset);

  var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line,
                                    1, 65, 64, HFilter);
  var_filter_block2d_bil_second_pass(FData3, temp2, 64, 64, 64, 64, VFilter);

  return vp9_variance64x64_c(temp2, 64, dst_ptr, dst_pixels_per_line, sse);
}

unsigned int vp9_sub_pixel_variance32x32_c(const uint8_t *src_ptr,
                                           int  src_pixels_per_line,
                                           int  xoffset,
                                           int  yoffset,
                                           const uint8_t *dst_ptr,
                                           int dst_pixels_per_line,
                                           unsigned int *sse) {
  uint16_t FData3[33 * 32];  // Temp data bufffer used in filtering
  uint8_t temp2[36 * 32];
  const int16_t *HFilter, *VFilter;

  HFilter = VP9_BILINEAR_FILTERS_2TAP(xoffset);
  VFilter = VP9_BILINEAR_FILTERS_2TAP(yoffset);

  var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line, 1, 33, 32, HFilter);
  var_filter_block2d_bil_second_pass(FData3, temp2, 32, 32, 32, 32, VFilter);

  return vp9_variance32x32_c(temp2, 32, dst_ptr, dst_pixels_per_line, sse);
}

unsigned int vp9_variance_halfpixvar16x16_h_c(const uint8_t *src_ptr,
                                              int  source_stride,
                                              const uint8_t *ref_ptr,
                                              int  recon_stride,
                                              unsigned int *sse) {
  return vp9_sub_pixel_variance16x16_c(src_ptr, source_stride, 8, 0,
                                       ref_ptr, recon_stride, sse);
}

unsigned int vp9_variance_halfpixvar32x32_h_c(const uint8_t *src_ptr,
                                              int  source_stride,
                                              const uint8_t *ref_ptr,
                                              int  recon_stride,
                                              unsigned int *sse) {
  return vp9_sub_pixel_variance32x32_c(src_ptr, source_stride, 8, 0,
                                       ref_ptr, recon_stride, sse);
}

unsigned int vp9_variance_halfpixvar64x64_h_c(const uint8_t *src_ptr,
                                              int  source_stride,
                                              const uint8_t *ref_ptr,
                                              int  recon_stride,
                                              unsigned int *sse) {
  return vp9_sub_pixel_variance64x64_c(src_ptr, source_stride, 8, 0,
                                       ref_ptr, recon_stride, sse);
}

unsigned int vp9_variance_halfpixvar16x16_v_c(const uint8_t *src_ptr,
                                              int  source_stride,
                                              const uint8_t *ref_ptr,
                                              int  recon_stride,
                                              unsigned int *sse) {
  return vp9_sub_pixel_variance16x16_c(src_ptr, source_stride, 0, 8,
                                       ref_ptr, recon_stride, sse);
}

unsigned int vp9_variance_halfpixvar32x32_v_c(const uint8_t *src_ptr,
                                              int  source_stride,
                                              const uint8_t *ref_ptr,
                                              int  recon_stride,
                                              unsigned int *sse) {
  return vp9_sub_pixel_variance32x32_c(src_ptr, source_stride, 0, 8,
                                       ref_ptr, recon_stride, sse);
}

unsigned int vp9_variance_halfpixvar64x64_v_c(const uint8_t *src_ptr,
                                              int  source_stride,
                                              const uint8_t *ref_ptr,
                                              int  recon_stride,
                                              unsigned int *sse) {
  return vp9_sub_pixel_variance64x64_c(src_ptr, source_stride, 0, 8,
                                       ref_ptr, recon_stride, sse);
}

unsigned int vp9_variance_halfpixvar16x16_hv_c(const uint8_t *src_ptr,
                                               int  source_stride,
                                               const uint8_t *ref_ptr,
                                               int  recon_stride,
                                               unsigned int *sse) {
  return vp9_sub_pixel_variance16x16_c(src_ptr, source_stride, 8, 8,
                                       ref_ptr, recon_stride, sse);
}

unsigned int vp9_variance_halfpixvar32x32_hv_c(const uint8_t *src_ptr,
                                               int  source_stride,
                                               const uint8_t *ref_ptr,
                                               int  recon_stride,
                                               unsigned int *sse) {
  return vp9_sub_pixel_variance32x32_c(src_ptr, source_stride, 8, 8,
                                       ref_ptr, recon_stride, sse);
}

unsigned int vp9_variance_halfpixvar64x64_hv_c(const uint8_t *src_ptr,
                                               int  source_stride,
                                               const uint8_t *ref_ptr,
                                               int  recon_stride,
                                               unsigned int *sse) {
  return vp9_sub_pixel_variance64x64_c(src_ptr, source_stride, 8, 8,
                                       ref_ptr, recon_stride, sse);
}

unsigned int vp9_sub_pixel_mse16x16_c(const uint8_t *src_ptr,
                                      int  src_pixels_per_line,
                                      int  xoffset,
                                      int  yoffset,
                                      const uint8_t *dst_ptr,
                                      int dst_pixels_per_line,
                                      unsigned int *sse) {
  vp9_sub_pixel_variance16x16_c(src_ptr, src_pixels_per_line,
                                xoffset, yoffset, dst_ptr,
                                dst_pixels_per_line, sse);
  return *sse;
}

unsigned int vp9_sub_pixel_mse32x32_c(const uint8_t *src_ptr,
                                      int  src_pixels_per_line,
                                      int  xoffset,
                                      int  yoffset,
                                      const uint8_t *dst_ptr,
                                      int dst_pixels_per_line,
                                      unsigned int *sse) {
  vp9_sub_pixel_variance32x32_c(src_ptr, src_pixels_per_line,
                                xoffset, yoffset, dst_ptr,
                                dst_pixels_per_line, sse);
  return *sse;
}

unsigned int vp9_sub_pixel_mse64x64_c(const uint8_t *src_ptr,
                                      int  src_pixels_per_line,
                                      int  xoffset,
                                      int  yoffset,
                                      const uint8_t *dst_ptr,
                                      int dst_pixels_per_line,
                                      unsigned int *sse) {
  vp9_sub_pixel_variance64x64_c(src_ptr, src_pixels_per_line,
                                xoffset, yoffset, dst_ptr,
                                dst_pixels_per_line, sse);
  return *sse;
}

unsigned int vp9_sub_pixel_variance16x8_c(const uint8_t *src_ptr,
                                          int  src_pixels_per_line,
                                          int  xoffset,
                                          int  yoffset,
                                          const uint8_t *dst_ptr,
                                          int dst_pixels_per_line,
                                          unsigned int *sse) {
  uint16_t FData3[16 * 9];  // Temp data bufffer used in filtering
  uint8_t temp2[20 * 16];
  const int16_t *HFilter, *VFilter;

  HFilter = VP9_BILINEAR_FILTERS_2TAP(xoffset);
  VFilter = VP9_BILINEAR_FILTERS_2TAP(yoffset);

  var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line, 1, 9, 16, HFilter);
  var_filter_block2d_bil_second_pass(FData3, temp2, 16, 16, 8, 16, VFilter);

  return vp9_variance16x8_c(temp2, 16, dst_ptr, dst_pixels_per_line, sse);
}

unsigned int vp9_sub_pixel_variance8x16_c(const uint8_t *src_ptr,
                                          int  src_pixels_per_line,
                                          int  xoffset,
                                          int  yoffset,
                                          const uint8_t *dst_ptr,
                                          int dst_pixels_per_line,
                                          unsigned int *sse) {
  uint16_t FData3[9 * 16];  // Temp data bufffer used in filtering
  uint8_t temp2[20 * 16];
  const int16_t *HFilter, *VFilter;

  HFilter = VP9_BILINEAR_FILTERS_2TAP(xoffset);
  VFilter = VP9_BILINEAR_FILTERS_2TAP(yoffset);

  var_filter_block2d_bil_first_pass(src_ptr, FData3, src_pixels_per_line,
                                    1, 17, 8, HFilter);
  var_filter_block2d_bil_second_pass(FData3, temp2, 8, 8, 16, 8, VFilter);

  return vp9_variance8x16_c(temp2, 8, dst_ptr, dst_pixels_per_line, sse);
}

