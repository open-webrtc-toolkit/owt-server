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

extern void filter_block1d_h6_mmx(const unsigned char *src_ptr, unsigned short *output_ptr, unsigned int src_pixels_per_line, unsigned int pixel_step, unsigned int output_height, unsigned int output_width, short *vp7_filter);
extern void filter_block1d_v6_mmx(const short *src_ptr, unsigned char *output_ptr, unsigned int pixels_per_line, unsigned int pixel_step, unsigned int output_height, unsigned int output_width, short *vp7_filter);
extern void filter_block1d8_h6_sse2(const unsigned char *src_ptr, unsigned short *output_ptr, unsigned int src_pixels_per_line, unsigned int pixel_step, unsigned int output_height, unsigned int output_width, short *vp7_filter);
extern void filter_block1d8_v6_sse2(const short *src_ptr, unsigned char *output_ptr, unsigned int pixels_per_line, unsigned int pixel_step, unsigned int output_height, unsigned int output_width, short *vp7_filter);

extern void vp9_filter_block2d_bil4x4_var_mmx
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  const short *HFilter,
  const short *VFilter,
  int *sum,
  unsigned int *sumsquared
);

extern unsigned int vp9_get4x4var_mmx
(
  const unsigned char *src_ptr,
  int  source_stride,
  const unsigned char *ref_ptr,
  int  recon_stride,
  unsigned int *SSE,
  int *Sum
);

unsigned int vp9_get_mb_ss_sse2
(
  const short *src_ptr
);
unsigned int vp9_get16x16var_sse2
(
  const unsigned char *src_ptr,
  int source_stride,
  const unsigned char *ref_ptr,
  int recon_stride,
  unsigned int *SSE,
  int *Sum
);
unsigned int vp9_get8x8var_sse2
(
  const unsigned char *src_ptr,
  int source_stride,
  const unsigned char *ref_ptr,
  int recon_stride,
  unsigned int *SSE,
  int *Sum
);
void vp9_filter_block2d_bil_var_sse2
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
void vp9_half_horiz_vert_variance8x_h_sse2
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  unsigned int Height,
  int *sum,
  unsigned int *sumsquared
);
void vp9_half_horiz_vert_variance16x_h_sse2
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  unsigned int Height,
  int *sum,
  unsigned int *sumsquared
);
void vp9_half_horiz_variance8x_h_sse2
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  unsigned int Height,
  int *sum,
  unsigned int *sumsquared
);
void vp9_half_horiz_variance16x_h_sse2
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  unsigned int Height,
  int *sum,
  unsigned int *sumsquared
);
void vp9_half_vert_variance8x_h_sse2
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  unsigned int Height,
  int *sum,
  unsigned int *sumsquared
);
void vp9_half_vert_variance16x_h_sse2
(
  const unsigned char *ref_ptr,
  int ref_pixels_per_line,
  const unsigned char *src_ptr,
  int src_pixels_per_line,
  unsigned int Height,
  int *sum,
  unsigned int *sumsquared
);

DECLARE_ALIGNED(16, extern const short, vp9_bilinear_filters_mmx[16][8]);

unsigned int vp9_variance4x4_wmt(
  const unsigned char *src_ptr,
  int  source_stride,
  const unsigned char *ref_ptr,
  int  recon_stride,
  unsigned int *sse) {
  unsigned int var;
  int avg;

  vp9_get4x4var_mmx(src_ptr, source_stride, ref_ptr, recon_stride, &var, &avg);
  *sse = var;
  return (var - (((unsigned int)avg * avg) >> 4));

}

unsigned int vp9_variance8x8_wmt
(
  const unsigned char *src_ptr,
  int  source_stride,
  const unsigned char *ref_ptr,
  int  recon_stride,
  unsigned int *sse) {
  unsigned int var;
  int avg;

  vp9_get8x8var_sse2(src_ptr, source_stride, ref_ptr, recon_stride, &var, &avg);
  *sse = var;
  return (var - (((unsigned int)avg * avg) >> 6));

}


unsigned int vp9_variance16x16_wmt
(
  const unsigned char *src_ptr,
  int  source_stride,
  const unsigned char *ref_ptr,
  int  recon_stride,
  unsigned int *sse) {
  unsigned int sse0;
  int sum0;


  vp9_get16x16var_sse2(src_ptr, source_stride, ref_ptr, recon_stride, &sse0, &sum0);
  *sse = sse0;
  return (sse0 - (((unsigned int)sum0 * sum0) >> 8));
}
unsigned int vp9_mse16x16_wmt(
  const unsigned char *src_ptr,
  int  source_stride,
  const unsigned char *ref_ptr,
  int  recon_stride,
  unsigned int *sse) {

  unsigned int sse0;
  int sum0;
  vp9_get16x16var_sse2(src_ptr, source_stride, ref_ptr, recon_stride, &sse0, &sum0);
  *sse = sse0;
  return sse0;

}


unsigned int vp9_variance16x8_wmt
(
  const unsigned char *src_ptr,
  int  source_stride,
  const unsigned char *ref_ptr,
  int  recon_stride,
  unsigned int *sse) {
  unsigned int sse0, sse1, var;
  int sum0, sum1, avg;

  vp9_get8x8var_sse2(src_ptr, source_stride, ref_ptr, recon_stride, &sse0, &sum0);
  vp9_get8x8var_sse2(src_ptr + 8, source_stride, ref_ptr + 8, recon_stride, &sse1, &sum1);

  var = sse0 + sse1;
  avg = sum0 + sum1;
  *sse = var;
  return (var - (((unsigned int)avg * avg) >> 7));

}

unsigned int vp9_variance8x16_wmt
(
  const unsigned char *src_ptr,
  int  source_stride,
  const unsigned char *ref_ptr,
  int  recon_stride,
  unsigned int *sse) {
  unsigned int sse0, sse1, var;
  int sum0, sum1, avg;

  vp9_get8x8var_sse2(src_ptr, source_stride, ref_ptr, recon_stride, &sse0, &sum0);
  vp9_get8x8var_sse2(src_ptr + 8 * source_stride, source_stride, ref_ptr + 8 * recon_stride, recon_stride, &sse1, &sum1);

  var = sse0 + sse1;
  avg = sum0 + sum1;
  *sse = var;
  return (var - (((unsigned int)avg * avg) >> 7));

}

unsigned int vp9_sub_pixel_variance4x4_wmt
(
  const unsigned char  *src_ptr,
  int  src_pixels_per_line,
  int  xoffset,
  int  yoffset,
  const unsigned char *dst_ptr,
  int dst_pixels_per_line,
  unsigned int *sse
) {
  int xsum;
  unsigned int xxsum;
  vp9_filter_block2d_bil4x4_var_mmx(
    src_ptr, src_pixels_per_line,
    dst_ptr, dst_pixels_per_line,
    vp9_bilinear_filters_mmx[xoffset], vp9_bilinear_filters_mmx[yoffset],
    &xsum, &xxsum
  );
  *sse = xxsum;
  return (xxsum - (((unsigned int)xsum * xsum) >> 4));
}


unsigned int vp9_sub_pixel_variance8x8_wmt
(
  const unsigned char  *src_ptr,
  int  src_pixels_per_line,
  int  xoffset,
  int  yoffset,
  const unsigned char *dst_ptr,
  int dst_pixels_per_line,
  unsigned int *sse
) {
  int xsum;
  unsigned int xxsum;

  if (xoffset == HALFNDX && yoffset == 0) {
    vp9_half_horiz_variance8x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 8,
      &xsum, &xxsum);
  } else if (xoffset == 0 && yoffset == HALFNDX) {
    vp9_half_vert_variance8x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 8,
      &xsum, &xxsum);
  } else if (xoffset == HALFNDX && yoffset == HALFNDX) {
    vp9_half_horiz_vert_variance8x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 8,
      &xsum, &xxsum);
  } else {
    vp9_filter_block2d_bil_var_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 8,
      xoffset, yoffset,
      &xsum, &xxsum);
  }

  *sse = xxsum;
  return (xxsum - (((unsigned int)xsum * xsum) >> 6));
}

unsigned int vp9_sub_pixel_variance16x16_wmt
(
  const unsigned char  *src_ptr,
  int  src_pixels_per_line,
  int  xoffset,
  int  yoffset,
  const unsigned char *dst_ptr,
  int dst_pixels_per_line,
  unsigned int *sse
) {
  int xsum0, xsum1;
  unsigned int xxsum0, xxsum1;


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
    vp9_filter_block2d_bil_var_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 16,
      xoffset, yoffset,
      &xsum0, &xxsum0
    );

    vp9_filter_block2d_bil_var_sse2(
      src_ptr + 8, src_pixels_per_line,
      dst_ptr + 8, dst_pixels_per_line, 16,
      xoffset, yoffset,
      &xsum1, &xxsum1
    );
    xsum0 += xsum1;
    xxsum0 += xxsum1;
  }

  *sse = xxsum0;
  return (xxsum0 - (((unsigned int)xsum0 * xsum0) >> 8));
}

unsigned int vp9_sub_pixel_mse16x16_wmt(
  const unsigned char  *src_ptr,
  int  src_pixels_per_line,
  int  xoffset,
  int  yoffset,
  const unsigned char *dst_ptr,
  int dst_pixels_per_line,
  unsigned int *sse
) {
  vp9_sub_pixel_variance16x16_wmt(src_ptr, src_pixels_per_line, xoffset, yoffset, dst_ptr, dst_pixels_per_line, sse);
  return *sse;
}

unsigned int vp9_sub_pixel_variance16x8_wmt
(
  const unsigned char  *src_ptr,
  int  src_pixels_per_line,
  int  xoffset,
  int  yoffset,
  const unsigned char *dst_ptr,
  int dst_pixels_per_line,
  unsigned int *sse

) {
  int xsum0, xsum1;
  unsigned int xxsum0, xxsum1;

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
    vp9_filter_block2d_bil_var_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 8,
      xoffset, yoffset,
      &xsum0, &xxsum0);

    vp9_filter_block2d_bil_var_sse2(
      src_ptr + 8, src_pixels_per_line,
      dst_ptr + 8, dst_pixels_per_line, 8,
      xoffset, yoffset,
      &xsum1, &xxsum1);
    xsum0 += xsum1;
    xxsum0 += xxsum1;
  }

  *sse = xxsum0;
  return (xxsum0 - (((unsigned int)xsum0 * xsum0) >> 7));
}

unsigned int vp9_sub_pixel_variance8x16_wmt
(
  const unsigned char  *src_ptr,
  int  src_pixels_per_line,
  int  xoffset,
  int  yoffset,
  const unsigned char *dst_ptr,
  int dst_pixels_per_line,
  unsigned int *sse
) {
  int xsum;
  unsigned int xxsum;

  if (xoffset == HALFNDX && yoffset == 0) {
    vp9_half_horiz_variance8x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 16,
      &xsum, &xxsum);
  } else if (xoffset == 0 && yoffset == HALFNDX) {
    vp9_half_vert_variance8x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 16,
      &xsum, &xxsum);
  } else if (xoffset == HALFNDX && yoffset == HALFNDX) {
    vp9_half_horiz_vert_variance8x_h_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 16,
      &xsum, &xxsum);
  } else {
    vp9_filter_block2d_bil_var_sse2(
      src_ptr, src_pixels_per_line,
      dst_ptr, dst_pixels_per_line, 16,
      xoffset, yoffset,
      &xsum, &xxsum);
  }

  *sse = xxsum;
  return (xxsum - (((unsigned int)xsum * xsum) >> 7));
}


unsigned int vp9_variance_halfpixvar16x16_h_wmt(
  const unsigned char *src_ptr,
  int  src_pixels_per_line,
  const unsigned char *dst_ptr,
  int  dst_pixels_per_line,
  unsigned int *sse) {
  int xsum0;
  unsigned int xxsum0;

  vp9_half_horiz_variance16x_h_sse2(
    src_ptr, src_pixels_per_line,
    dst_ptr, dst_pixels_per_line, 16,
    &xsum0, &xxsum0);

  *sse = xxsum0;
  return (xxsum0 - (((unsigned int)xsum0 * xsum0) >> 8));
}


unsigned int vp9_variance_halfpixvar16x16_v_wmt(
  const unsigned char *src_ptr,
  int  src_pixels_per_line,
  const unsigned char *dst_ptr,
  int  dst_pixels_per_line,
  unsigned int *sse) {
  int xsum0;
  unsigned int xxsum0;
  vp9_half_vert_variance16x_h_sse2(
    src_ptr, src_pixels_per_line,
    dst_ptr, dst_pixels_per_line, 16,
    &xsum0, &xxsum0);

  *sse = xxsum0;
  return (xxsum0 - (((unsigned int)xsum0 * xsum0) >> 8));
}


unsigned int vp9_variance_halfpixvar16x16_hv_wmt(
  const unsigned char *src_ptr,
  int  src_pixels_per_line,
  const unsigned char *dst_ptr,
  int  dst_pixels_per_line,
  unsigned int *sse) {
  int xsum0;
  unsigned int xxsum0;

  vp9_half_horiz_vert_variance16x_h_sse2(
    src_ptr, src_pixels_per_line,
    dst_ptr, dst_pixels_per_line, 16,
    &xsum0, &xxsum0);

  *sse = xxsum0;
  return (xxsum0 - (((unsigned int)xsum0 * xsum0) >> 8));
}
