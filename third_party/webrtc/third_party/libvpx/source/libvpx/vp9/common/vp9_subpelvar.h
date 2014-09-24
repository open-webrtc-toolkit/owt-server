/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_SUBPELVAR_H_
#define VP9_COMMON_VP9_SUBPELVAR_H_

#include "vp9/common/vp9_filter.h"

static void variance(const uint8_t *src_ptr,
                     int  source_stride,
                     const uint8_t *ref_ptr,
                     int  recon_stride,
                     int  w,
                     int  h,
                     unsigned int *sse,
                     int *sum) {
  int i, j;
  int diff;

  *sum = 0;
  *sse = 0;

  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      diff = src_ptr[j] - ref_ptr[j];
      *sum += diff;
      *sse += diff * diff;
    }

    src_ptr += source_stride;
    ref_ptr += recon_stride;
  }
}

/****************************************************************************
 *
 *  ROUTINE       : filter_block2d_bil_first_pass
 *
 *  INPUTS        : uint8_t  *src_ptr          : Pointer to source block.
 *                  uint32_t src_pixels_per_line : Stride of input block.
 *                  uint32_t pixel_step        : Offset between filter input samples (see notes).
 *                  uint32_t output_height     : Input block height.
 *                  uint32_t output_width      : Input block width.
 *                  int32_t  *vp9_filter          : Array of 2 bi-linear filter taps.
 *
 *  OUTPUTS       : int32_t *output_ptr        : Pointer to filtered block.
 *
 *  RETURNS       : void
 *
 *  FUNCTION      : Applies a 1-D 2-tap bi-linear filter to the source block in
 *                  either horizontal or vertical direction to produce the
 *                  filtered output block. Used to implement first-pass
 *                  of 2-D separable filter.
 *
 *  SPECIAL NOTES : Produces int32_t output to retain precision for next pass.
 *                  Two filter taps should sum to VP9_FILTER_WEIGHT.
 *                  pixel_step defines whether the filter is applied
 *                  horizontally (pixel_step=1) or vertically (pixel_step=stride).
 *                  It defines the offset required to move from one input
 *                  to the next.
 *
 ****************************************************************************/
static void var_filter_block2d_bil_first_pass(const uint8_t *src_ptr,
                                              uint16_t *output_ptr,
                                              unsigned int src_pixels_per_line,
                                              int pixel_step,
                                              unsigned int output_height,
                                              unsigned int output_width,
                                              const int16_t *vp9_filter) {
  unsigned int i, j;

  for (i = 0; i < output_height; i++) {
    for (j = 0; j < output_width; j++) {
      // Apply bilinear filter
      output_ptr[j] = (((int)src_ptr[0]          * vp9_filter[0]) +
                       ((int)src_ptr[pixel_step] * vp9_filter[1]) +
                       (VP9_FILTER_WEIGHT / 2)) >> VP9_FILTER_SHIFT;
      src_ptr++;
    }

    // Next row...
    src_ptr    += src_pixels_per_line - output_width;
    output_ptr += output_width;
  }
}

/****************************************************************************
 *
 *  ROUTINE       : filter_block2d_bil_second_pass
 *
 *  INPUTS        : int32_t  *src_ptr          : Pointer to source block.
 *                  uint32_t src_pixels_per_line : Stride of input block.
 *                  uint32_t pixel_step        : Offset between filter input samples (see notes).
 *                  uint32_t output_height     : Input block height.
 *                  uint32_t output_width      : Input block width.
 *                  int32_t  *vp9_filter          : Array of 2 bi-linear filter taps.
 *
 *  OUTPUTS       : uint16_t *output_ptr       : Pointer to filtered block.
 *
 *  RETURNS       : void
 *
 *  FUNCTION      : Applies a 1-D 2-tap bi-linear filter to the source block in
 *                  either horizontal or vertical direction to produce the
 *                  filtered output block. Used to implement second-pass
 *                  of 2-D separable filter.
 *
 *  SPECIAL NOTES : Requires 32-bit input as produced by filter_block2d_bil_first_pass.
 *                  Two filter taps should sum to VP9_FILTER_WEIGHT.
 *                  pixel_step defines whether the filter is applied
 *                  horizontally (pixel_step=1) or vertically (pixel_step=stride).
 *                  It defines the offset required to move from one input
 *                  to the next.
 *
 ****************************************************************************/
static void var_filter_block2d_bil_second_pass(const uint16_t *src_ptr,
                                               uint8_t *output_ptr,
                                               unsigned int src_pixels_per_line,
                                               unsigned int pixel_step,
                                               unsigned int output_height,
                                               unsigned int output_width,
                                               const int16_t *vp9_filter) {
  unsigned int  i, j;
  int  Temp;

  for (i = 0; i < output_height; i++) {
    for (j = 0; j < output_width; j++) {
      // Apply filter
      Temp = ((int)src_ptr[0]         * vp9_filter[0]) +
             ((int)src_ptr[pixel_step] * vp9_filter[1]) +
             (VP9_FILTER_WEIGHT / 2);
      output_ptr[j] = (unsigned int)(Temp >> VP9_FILTER_SHIFT);
      src_ptr++;
    }

    // Next row...
    src_ptr    += src_pixels_per_line - output_width;
    output_ptr += output_width;
  }
}

#endif  // VP9_COMMON_VP9_SUBPELVAR_H_
