/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vpx_config.h"
#include "vp9/encoder/vp9_variance.h"
#include "vp9/common/vp9_pragmas.h"
#include "vpx_ports/mem.h"

#define HALFNDX 8

extern unsigned int vp9_get16x16var_sse2
(
  const unsigned char *src_ptr,
  int source_stride,
  const unsigned char *ref_ptr,
  int recon_stride,
  unsigned int *SSE,
  int *Sum
);
extern void vp9_half_horiz_vert_variance16x_h_sse2
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  unsigned int Height,
  int *sum,
  unsigned int *sumsquared
);
extern void vp9_half_horiz_variance16x_h_sse2
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  unsigned int Height,
  int *sum,
  unsigned int *sumsquared
);
extern void vp9_half_vert_variance16x_h_sse2
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  unsigned int Height,
  int *sum,
  unsigned int *sumsquared
);
extern void vp9_filter_block2d_bil_var_ssse3
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  unsigned int Height,
  int  xoffset,
  int  yoffset,
  int *sum,
  unsigned int *sumsquared
);

unsigned int vp9_sub_pixel_variance16x16_ssse3
(
  const unsigned char  *src_ptr,
  int  src_pixels_per_line,
  int  xoffset,
  int  yoffset,
  const unsigned char *dst_ptr,
  int dst_pixels_per_line,
  unsigned int *sse
) {
  int xsum0;
  unsigned int xxsum0;

  // note we could avoid these if statements if the calling function
  // just called the appropriate functions inside.
  if (xoffset == HALFNDX && yoffset == 0) {
    vp9_half_horiz_variance16x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 16,
      &xsum0, &xxsum0);
  } else if (xoffset == 0 && yoffset == HALFNDX) {
    vp9_half_vert_variance16x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 16,
      &xsum0, &xxsum0);
  } else if (xoffset == HALFNDX && yoffset == HALFNDX) {
    vp9_half_horiz_vert_variance16x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 16,
      &xsum0, &xxsum0);
  } else {
    vp9_filter_block2d_bil_var_ssse3(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 16,
      xoffset, yoffset,
      &xsum0, &xxsum0);
  }

  *sse = xxsum0;
  return (xxsum0 - (((unsigned int)xsum0 * xsum0) >> 8));
}

unsigned int vp9_sub_pixel_variance16x8_ssse3
(
  const unsigned char  *src_ptr,
  int  src_pixels_per_line,
  int  xoffset,
  int  yoffset,
  const unsigned char *dst_ptr,
  int dst_pixels_per_line,
  unsigned int *sse

) {
  int xsum0;
  unsigned int xxsum0;

  if (xoffset == HALFNDX && yoffset == 0) {
    vp9_half_horiz_variance16x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 8,
      &xsum0, &xxsum0);
  } else if (xoffset == 0 && yoffset == HALFNDX) {
    vp9_half_vert_variance16x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 8,
      &xsum0, &xxsum0);
  } else if (xoffset == HALFNDX && yoffset == HALFNDX) {
    vp9_half_horiz_vert_variance16x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 8,
      &xsum0, &xxsum0);
  } else {
    vp9_filter_block2d_bil_var_ssse3(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 8,
      xoffset, yoffset,
      &xsum0, &xxsum0);
  }

  *sse = xxsum0;
  return (xxsum0 - (((unsigned int)xsum0 * xsum0) >> 7));
}
