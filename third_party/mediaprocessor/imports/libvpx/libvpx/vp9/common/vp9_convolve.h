/*
 *  Copyright (c) 2013 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef VP9_COMMON_CONVOLVE_H_
#define VP9_COMMON_CONVOLVE_H_

#include "./vpx_config.h"
#include "vpx/vpx_integer.h"

typedef void (*convolve_fn_t)(const uint8_t *src, int src_stride,
                              uint8_t *dst, int dst_stride,
                              const int16_t *filter_x, int x_step_q4,
                              const int16_t *filter_y, int y_step_q4,
                              int w, int h);

// Not a convolution, a block copy conforming to the convolution prototype
void vp9_convolve_copy(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int x_step_q4,
                       const int16_t *filter_y, int y_step_q4,
                       int w, int h);

// Not a convolution, a block average conforming to the convolution prototype
void vp9_convolve_avg(const uint8_t *src, int src_stride,
                      uint8_t *dst, int dst_stride,
                      const int16_t *filter_x, int x_step_q4,
                      const int16_t *filter_y, int y_step_q4,
                      int w, int h);

#if CONFIG_IMPLICIT_COMPOUNDINTER_WEIGHT
// Not a convolution, a block wtd (1/8, 7/8) average for (dst, src)
void vp9_convolve_1by8(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int x_step_q4,
                       const int16_t *filter_y, int y_step_q4,
                       int w, int h);

// Not a convolution, a block wtd (1/4, 3/4) average for (dst, src)
void vp9_convolve_qtr(const uint8_t *src, int src_stride,
                      uint8_t *dst, int dst_stride,
                      const int16_t *filter_x, int x_step_q4,
                      const int16_t *filter_y, int y_step_q4,
                      int w, int h);

// Not a convolution, a block wtd (3/8, 5/8) average for (dst, src)
void vp9_convolve_3by8(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int x_step_q4,
                       const int16_t *filter_y, int y_step_q4,
                       int w, int h);

// Not a convolution, a block wtd (5/8, 3/8) average for (dst, src)
void vp9_convolve_5by8(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int x_step_q4,
                       const int16_t *filter_y, int y_step_q4,
                       int w, int h);

// Not a convolution, a block wtd (3/4, 1/4) average for (dst, src)
void vp9_convolve_3qtr(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int x_step_q4,
                       const int16_t *filter_y, int y_step_q4,
                       int w, int h);

// Not a convolution, a block wtd (7/8, 1/8) average for (dst, src)
void vp9_convolve_7by8(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int x_step_q4,
                       const int16_t *filter_y, int y_step_q4,
                       int w, int h);
#endif

struct subpix_fn_table {
  const int16_t (*filter_x)[8];
  const int16_t (*filter_y)[8];
};

#endif  // VP9_COMMON_CONVOLVE_H_
