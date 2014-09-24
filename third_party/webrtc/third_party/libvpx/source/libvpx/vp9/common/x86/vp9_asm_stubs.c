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
#include "vpx_ports/mem.h"
#include "vp9/common/vp9_subpixel.h"

extern const short vp9_six_tap_mmx[8][6 * 8];

extern void vp9_filter_block1d_h6_mmx(unsigned char   *src_ptr,
                                      unsigned short  *output_ptr,
                                      unsigned int     src_pixels_per_line,
                                      unsigned int     pixel_step,
                                      unsigned int     output_height,
                                      unsigned int     output_width,
                                      const short     *vp9_filter);

extern void vp9_filter_block1dc_v6_mmx(unsigned short *src_ptr,
                                       unsigned char  *output_ptr,
                                       int             output_pitch,
                                       unsigned int    pixels_per_line,
                                       unsigned int    pixel_step,
                                       unsigned int    output_height,
                                       unsigned int    output_width,
                                       const short    *vp9_filter);

extern void vp9_filter_block1d8_h6_sse2(unsigned char  *src_ptr,
                                        unsigned short *output_ptr,
                                        unsigned int    src_pixels_per_line,
                                        unsigned int    pixel_step,
                                        unsigned int    output_height,
                                        unsigned int    output_width,
                                        const short    *vp9_filter);

extern void vp9_filter_block1d16_h6_sse2(unsigned char  *src_ptr,
                                         unsigned short *output_ptr,
                                         unsigned int    src_pixels_per_line,
                                         unsigned int    pixel_step,
                                         unsigned int    output_height,
                                         unsigned int    output_width,
                                         const short    *vp9_filter);

extern void vp9_filter_block1d8_v6_sse2(unsigned short *src_ptr,
                                        unsigned char *output_ptr,
                                        int dst_ptich,
                                        unsigned int pixels_per_line,
                                        unsigned int pixel_step,
                                        unsigned int output_height,
                                        unsigned int output_width,
                                        const short    *vp9_filter);

extern void vp9_filter_block1d16_v6_sse2(unsigned short *src_ptr,
                                         unsigned char *output_ptr,
                                         int dst_ptich,
                                         unsigned int pixels_per_line,
                                         unsigned int pixel_step,
                                         unsigned int output_height,
                                         unsigned int output_width,
                                         const short    *vp9_filter);

extern void vp9_unpack_block1d16_h6_sse2(unsigned char  *src_ptr,
                                         unsigned short *output_ptr,
                                         unsigned int    src_pixels_per_line,
                                         unsigned int    output_height,
                                         unsigned int    output_width);

extern void vp9_filter_block1d8_h6_only_sse2(unsigned char *src_ptr,
                                             unsigned int   src_pixels_per_line,
                                             unsigned char *output_ptr,
                                             int            dst_pitch,
                                             unsigned int   output_height,
                                             const short   *vp9_filter);

extern void vp9_filter_block1d16_h6_only_sse2(unsigned char *src_ptr,
                                              unsigned int   src_pixels_per_lin,
                                              unsigned char *output_ptr,
                                              int            dst_pitch,
                                              unsigned int   output_height,
                                              const short   *vp9_filter);

extern void vp9_filter_block1d8_v6_only_sse2(unsigned char *src_ptr,
                                             unsigned int   src_pixels_per_line,
                                             unsigned char *output_ptr,
                                             int            dst_pitch,
                                             unsigned int   output_height,
                                             const short   *vp9_filter);

///////////////////////////////////////////////////////////////////////////
// the mmx function that does the bilinear filtering and var calculation //
// int one pass                                                          //
///////////////////////////////////////////////////////////////////////////
DECLARE_ALIGNED(16, const short, vp9_bilinear_filters_mmx[16][8]) = {
  { 128, 128, 128, 128,  0,  0,  0,  0 },
  { 120, 120, 120, 120,  8,  8,  8,  8 },
  { 112, 112, 112, 112, 16, 16, 16, 16 },
  { 104, 104, 104, 104, 24, 24, 24, 24 },
  {  96, 96, 96, 96, 32, 32, 32, 32 },
  {  88, 88, 88, 88, 40, 40, 40, 40 },
  {  80, 80, 80, 80, 48, 48, 48, 48 },
  {  72, 72, 72, 72, 56, 56, 56, 56 },
  {  64, 64, 64, 64, 64, 64, 64, 64 },
  {  56, 56, 56, 56, 72, 72, 72, 72 },
  {  48, 48, 48, 48, 80, 80, 80, 80 },
  {  40, 40, 40, 40, 88, 88, 88, 88 },
  {  32, 32, 32, 32, 96, 96, 96, 96 },
  {  24, 24, 24, 24, 104, 104, 104, 104 },
  {  16, 16, 16, 16, 112, 112, 112, 112 },
  {   8,  8,  8,  8, 120, 120, 120, 120 }
};

#if HAVE_MMX
void vp9_sixtap_predict4x4_mmx(unsigned char  *src_ptr,
                               int  src_pixels_per_line,
                               int  xoffset,
                               int  yoffset,
                               unsigned char *dst_ptr,
                               int  dst_pitch) {
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict4x4_mmx\n");
#endif
  /* Temp data bufffer used in filtering */
  DECLARE_ALIGNED_ARRAY(16, unsigned short, fdata2, 16 * 16);
  const short *hfilter, *vfilter;
  hfilter = vp9_six_tap_mmx[xoffset];
  vp9_filter_block1d_h6_mmx(src_ptr - (2 * src_pixels_per_line), fdata2,
                            src_pixels_per_line, 1, 9, 8, hfilter);
  vfilter = vp9_six_tap_mmx[yoffset];
  vp9_filter_block1dc_v6_mmx(fdata2 + 8, dst_ptr, dst_pitch,
                             8, 4, 4, 4, vfilter);
}

void vp9_sixtap_predict16x16_mmx(unsigned char  *src_ptr,
                                 int  src_pixels_per_line,
                                 int  xoffset,
                                 int  yoffset,
                                 unsigned char *dst_ptr,
                                 int dst_pitch) {
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict16x16_mmx\n");
#endif
  /* Temp data bufffer used in filtering */
  DECLARE_ALIGNED_ARRAY(16, unsigned short, fdata2, 24 * 24);
  const short *hfilter, *vfilter;

  hfilter = vp9_six_tap_mmx[xoffset];
  vp9_filter_block1d_h6_mmx(src_ptr - (2 * src_pixels_per_line),
                            fdata2,   src_pixels_per_line, 1, 21, 32,
                            hfilter);
  vp9_filter_block1d_h6_mmx(src_ptr - (2 * src_pixels_per_line) + 4,
                            fdata2 + 4, src_pixels_per_line, 1, 21, 32,
                            hfilter);
  vp9_filter_block1d_h6_mmx(src_ptr - (2 * src_pixels_per_line) + 8,
                            fdata2 + 8, src_pixels_per_line, 1, 21, 32,
                            hfilter);
  vp9_filter_block1d_h6_mmx(src_ptr - (2 * src_pixels_per_line) + 12,
                            fdata2 + 12, src_pixels_per_line, 1, 21, 32,
                            hfilter);

  vfilter = vp9_six_tap_mmx[yoffset];
  vp9_filter_block1dc_v6_mmx(fdata2 + 32, dst_ptr,      dst_pitch,
                             32, 16, 16, 16, vfilter);
  vp9_filter_block1dc_v6_mmx(fdata2 + 36, dst_ptr + 4,  dst_pitch,
                             32, 16, 16, 16, vfilter);
  vp9_filter_block1dc_v6_mmx(fdata2 + 40, dst_ptr + 8,  dst_pitch,
                             32, 16, 16, 16, vfilter);
  vp9_filter_block1dc_v6_mmx(fdata2 + 44, dst_ptr + 12, dst_pitch,
                             32, 16, 16, 16, vfilter);
}

void vp9_sixtap_predict8x8_mmx(unsigned char  *src_ptr,
                               int  src_pixels_per_line,
                               int  xoffset,
                               int  yoffset,
                               unsigned char *dst_ptr,
                               int  dst_pitch) {
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict8x8_mmx\n");
#endif
  /* Temp data bufffer used in filtering */
  DECLARE_ALIGNED_ARRAY(16, unsigned short, fdata2, 256);
  const short *hfilter, *vfilter;

  hfilter = vp9_six_tap_mmx[xoffset];
  vp9_filter_block1d_h6_mmx(src_ptr - (2 * src_pixels_per_line),
                            fdata2,   src_pixels_per_line, 1, 13, 16,
                            hfilter);
  vp9_filter_block1d_h6_mmx(src_ptr - (2 * src_pixels_per_line) + 4,
                            fdata2 + 4, src_pixels_per_line, 1, 13, 16,
                            hfilter);

  vfilter = vp9_six_tap_mmx[yoffset];
  vp9_filter_block1dc_v6_mmx(fdata2 + 16, dst_ptr,     dst_pitch,
                             16, 8, 8, 8, vfilter);
  vp9_filter_block1dc_v6_mmx(fdata2 + 20, dst_ptr + 4, dst_pitch,
                             16, 8, 8, 8, vfilter);
}

void vp9_sixtap_predict8x4_mmx(unsigned char  *src_ptr,
                               int  src_pixels_per_line,
                               int  xoffset,
                               int  yoffset,
                               unsigned char *dst_ptr,
                               int  dst_pitch) {
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict8x4_mmx\n");
#endif
  /* Temp data bufffer used in filtering */
  DECLARE_ALIGNED_ARRAY(16, unsigned short, fdata2, 256);
  const short *hfilter, *vfilter;

  hfilter = vp9_six_tap_mmx[xoffset];
  vp9_filter_block1d_h6_mmx(src_ptr - (2 * src_pixels_per_line),
                            fdata2,   src_pixels_per_line, 1, 9, 16, hfilter);
  vp9_filter_block1d_h6_mmx(src_ptr - (2 * src_pixels_per_line) + 4,
                            fdata2 + 4, src_pixels_per_line, 1, 9, 16, hfilter);

  vfilter = vp9_six_tap_mmx[yoffset];
  vp9_filter_block1dc_v6_mmx(fdata2 + 16, dst_ptr,     dst_pitch,
                             16, 8, 4, 8, vfilter);
  vp9_filter_block1dc_v6_mmx(fdata2 + 20, dst_ptr + 4, dst_pitch,
                             16, 8, 4, 8, vfilter);
}
#endif

#if HAVE_SSE2
void vp9_sixtap_predict16x16_sse2(unsigned char  *src_ptr,
                                  int  src_pixels_per_line,
                                  int  xoffset,
                                  int  yoffset,
                                  unsigned char *dst_ptr,
                                  int  dst_pitch) {
  /* Temp data bufffer used in filtering */
  DECLARE_ALIGNED_ARRAY(16, unsigned short, fdata2, 24 * 24);
  const short *hfilter, *vfilter;
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict16x16_sse2\n");
#endif

  if (xoffset) {
    if (yoffset) {
      hfilter = vp9_six_tap_mmx[xoffset];
      vp9_filter_block1d16_h6_sse2(src_ptr - (2 * src_pixels_per_line), fdata2,
                                   src_pixels_per_line, 1, 21, 32, hfilter);
      vfilter = vp9_six_tap_mmx[yoffset];
      vp9_filter_block1d16_v6_sse2(fdata2 + 32, dst_ptr, dst_pitch,
                                   32, 16, 16, dst_pitch, vfilter);
    } else {
      /* First-pass only */
      hfilter = vp9_six_tap_mmx[xoffset];
      vp9_filter_block1d16_h6_only_sse2(src_ptr, src_pixels_per_line,
                                        dst_ptr, dst_pitch, 16, hfilter);
    }
  } else {
    /* Second-pass only */
    vfilter = vp9_six_tap_mmx[yoffset];
    vp9_unpack_block1d16_h6_sse2(src_ptr - (2 * src_pixels_per_line), fdata2,
                                 src_pixels_per_line, 21, 32);
    vp9_filter_block1d16_v6_sse2(fdata2 + 32, dst_ptr, dst_pitch,
                                 32, 16, 16, dst_pitch, vfilter);
  }
}

void vp9_sixtap_predict8x8_sse2(unsigned char  *src_ptr,
                                int  src_pixels_per_line,
                                int  xoffset,
                                int  yoffset,
                                unsigned char *dst_ptr,
                                int  dst_pitch) {
  /* Temp data bufffer used in filtering */
  DECLARE_ALIGNED_ARRAY(16, unsigned short, fdata2, 256);
  const short *hfilter, *vfilter;
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict8x8_sse2\n");
#endif

  if (xoffset) {
    if (yoffset) {
      hfilter = vp9_six_tap_mmx[xoffset];
      vp9_filter_block1d8_h6_sse2(src_ptr - (2 * src_pixels_per_line), fdata2,
                                  src_pixels_per_line, 1, 13, 16, hfilter);
      vfilter = vp9_six_tap_mmx[yoffset];
      vp9_filter_block1d8_v6_sse2(fdata2 + 16, dst_ptr, dst_pitch,
                                  16, 8, 8, dst_pitch, vfilter);
    } else {
      /* First-pass only */
      hfilter = vp9_six_tap_mmx[xoffset];
      vp9_filter_block1d8_h6_only_sse2(src_ptr, src_pixels_per_line,
                                       dst_ptr, dst_pitch, 8, hfilter);
    }
  } else {
    /* Second-pass only */
    vfilter = vp9_six_tap_mmx[yoffset];
    vp9_filter_block1d8_v6_only_sse2(src_ptr - (2 * src_pixels_per_line),
                                     src_pixels_per_line,
                                     dst_ptr, dst_pitch, 8, vfilter);
  }
}

void vp9_sixtap_predict8x4_sse2(unsigned char  *src_ptr,
                                int  src_pixels_per_line,
                                int  xoffset,
                                int  yoffset,
                                unsigned char *dst_ptr,
                                int  dst_pitch) {
  /* Temp data bufffer used in filtering */
  DECLARE_ALIGNED_ARRAY(16, unsigned short, fdata2, 256);
  const short *hfilter, *vfilter;
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict8x4_sse2\n");
#endif

  if (xoffset) {
    if (yoffset) {
      hfilter = vp9_six_tap_mmx[xoffset];
      vp9_filter_block1d8_h6_sse2(src_ptr - (2 * src_pixels_per_line), fdata2,
                                  src_pixels_per_line, 1, 9, 16, hfilter);
      vfilter = vp9_six_tap_mmx[yoffset];
      vp9_filter_block1d8_v6_sse2(fdata2 + 16, dst_ptr, dst_pitch,
                                  16, 8, 4, dst_pitch, vfilter);
    } else {
      /* First-pass only */
      hfilter = vp9_six_tap_mmx[xoffset];
      vp9_filter_block1d8_h6_only_sse2(src_ptr, src_pixels_per_line,
                                       dst_ptr, dst_pitch, 4, hfilter);
    }
  } else {
    /* Second-pass only */
    vfilter = vp9_six_tap_mmx[yoffset];
    vp9_filter_block1d8_v6_only_sse2(src_ptr - (2 * src_pixels_per_line),
                                     src_pixels_per_line,
                                     dst_ptr, dst_pitch, 4, vfilter);
  }
}
#endif

#if HAVE_SSSE3
extern void vp9_filter_block1d8_h6_ssse3(unsigned char  *src_ptr,
                                         unsigned int    src_pixels_per_line,
                                         unsigned char  *output_ptr,
                                         unsigned int    output_pitch,
                                         unsigned int    output_height,
                                         unsigned int    vp9_filter_index);

extern void vp9_filter_block1d16_h6_ssse3(unsigned char  *src_ptr,
                                          unsigned int    src_pixels_per_line,
                                          unsigned char  *output_ptr,
                                          unsigned int    output_pitch,
                                          unsigned int    output_height,
                                          unsigned int    vp9_filter_index);

extern void vp9_filter_block1d16_v6_ssse3(unsigned char *src_ptr,
                                          unsigned int   src_pitch,
                                          unsigned char *output_ptr,
                                          unsigned int   out_pitch,
                                          unsigned int   output_height,
                                          unsigned int   vp9_filter_index);

extern void vp9_filter_block1d8_v6_ssse3(unsigned char *src_ptr,
                                         unsigned int   src_pitch,
                                         unsigned char *output_ptr,
                                         unsigned int   out_pitch,
                                         unsigned int   output_height,
                                         unsigned int   vp9_filter_index);

extern void vp9_filter_block1d4_h6_ssse3(unsigned char  *src_ptr,
                                         unsigned int    src_pixels_per_line,
                                         unsigned char  *output_ptr,
                                         unsigned int    output_pitch,
                                         unsigned int    output_height,
                                         unsigned int    vp9_filter_index);

extern void vp9_filter_block1d4_v6_ssse3(unsigned char *src_ptr,
                                         unsigned int   src_pitch,
                                         unsigned char *output_ptr,
                                         unsigned int   out_pitch,
                                         unsigned int   output_height,
                                         unsigned int   vp9_filter_index);

void vp9_sixtap_predict16x16_ssse3(unsigned char  *src_ptr,
                                   int  src_pixels_per_line,
                                   int  xoffset,
                                   int  yoffset,
                                   unsigned char *dst_ptr,
                                   int  dst_pitch) {
  DECLARE_ALIGNED_ARRAY(16, unsigned char, fdata2, 24 * 24);
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict16x16_ssse3\n");
#endif

  if (xoffset) {
    if (yoffset) {
      vp9_filter_block1d16_h6_ssse3(src_ptr - (2 * src_pixels_per_line),
                                    src_pixels_per_line,
                                    fdata2, 16, 21, xoffset);
      vp9_filter_block1d16_v6_ssse3(fdata2, 16, dst_ptr, dst_pitch,
                                    16, yoffset);
    } else {
      /* First-pass only */
      vp9_filter_block1d16_h6_ssse3(src_ptr, src_pixels_per_line,
                                    dst_ptr, dst_pitch, 16, xoffset);
    }
  } else {
    /* Second-pass only */
    vp9_filter_block1d16_v6_ssse3(src_ptr - (2 * src_pixels_per_line),
                                  src_pixels_per_line,
                                  dst_ptr, dst_pitch, 16, yoffset);
  }
}

void vp9_sixtap_predict8x8_ssse3(unsigned char  *src_ptr,
                                 int  src_pixels_per_line,
                                 int  xoffset,
                                 int  yoffset,
                                 unsigned char *dst_ptr,
                                 int  dst_pitch) {
  DECLARE_ALIGNED_ARRAY(16, unsigned char, fdata2, 256);
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict8x8_ssse3\n");
#endif

  if (xoffset) {
    if (yoffset) {
      vp9_filter_block1d8_h6_ssse3(src_ptr - (2 * src_pixels_per_line),
                                   src_pixels_per_line, fdata2, 8, 13, xoffset);
      vp9_filter_block1d8_v6_ssse3(fdata2, 8, dst_ptr, dst_pitch, 8, yoffset);
    } else {
      vp9_filter_block1d8_h6_ssse3(src_ptr, src_pixels_per_line,
                                   dst_ptr, dst_pitch, 8, xoffset);
    }
  } else {
    /* Second-pass only */
    vp9_filter_block1d8_v6_ssse3(src_ptr - (2 * src_pixels_per_line),
                                 src_pixels_per_line,
                                 dst_ptr, dst_pitch, 8, yoffset);
  }
}

void vp9_sixtap_predict8x4_ssse3(unsigned char  *src_ptr,
                                 int  src_pixels_per_line,
                                 int  xoffset,
                                 int  yoffset,
                                 unsigned char *dst_ptr,
                                 int  dst_pitch) {
  DECLARE_ALIGNED_ARRAY(16, unsigned char, fdata2, 256);
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict8x4_ssse3\n");
#endif

  if (xoffset) {
    if (yoffset) {
      vp9_filter_block1d8_h6_ssse3(src_ptr - (2 * src_pixels_per_line),
                                   src_pixels_per_line, fdata2, 8, 9, xoffset);
      vp9_filter_block1d8_v6_ssse3(fdata2, 8, dst_ptr, dst_pitch, 4, yoffset);
    } else {
      /* First-pass only */
      vp9_filter_block1d8_h6_ssse3(src_ptr, src_pixels_per_line,
                                   dst_ptr, dst_pitch, 4, xoffset);
    }
  } else {
    /* Second-pass only */
    vp9_filter_block1d8_v6_ssse3(src_ptr - (2 * src_pixels_per_line),
                                 src_pixels_per_line,
                                 dst_ptr, dst_pitch, 4, yoffset);
  }
}

void vp9_sixtap_predict4x4_ssse3(unsigned char  *src_ptr,
                                 int   src_pixels_per_line,
                                 int  xoffset,
                                 int  yoffset,
                                 unsigned char *dst_ptr,
                                 int dst_pitch) {
  DECLARE_ALIGNED_ARRAY(16, unsigned char, fdata2, 4 * 9);
#ifdef ANNOUNCE_FUNCTION
  printf("vp9_sixtap_predict4x4_ssse3\n");
#endif

  if (xoffset) {
    if (yoffset) {
      vp9_filter_block1d4_h6_ssse3(src_ptr - (2 * src_pixels_per_line),
                                   src_pixels_per_line, fdata2, 4, 9, xoffset);
      vp9_filter_block1d4_v6_ssse3(fdata2, 4, dst_ptr, dst_pitch, 4, yoffset);
    } else {
      vp9_filter_block1d4_h6_ssse3(src_ptr, src_pixels_per_line,
                                   dst_ptr, dst_pitch, 4, xoffset);
    }
  } else {
    vp9_filter_block1d4_v6_ssse3(src_ptr - (2 * src_pixels_per_line),
                                 src_pixels_per_line,
                                 dst_ptr, dst_pitch, 4, yoffset);
  }
}

void vp9_filter_block1d16_v8_ssse3(const unsigned char *src_ptr,
                                   const unsigned int src_pitch,
                                   unsigned char *output_ptr,
                                   unsigned int out_pitch,
                                   unsigned int output_height,
                                   const short *filter);

void vp9_filter_block1d16_h8_ssse3(const unsigned char *src_ptr,
                                   const unsigned int src_pitch,
                                   unsigned char *output_ptr,
                                   unsigned int out_pitch,
                                   unsigned int output_height,
                                   const short *filter);

void vp9_filter_block2d_16x16_8_ssse3(const unsigned char *src_ptr,
                                      const unsigned int src_stride,
                                      const short *hfilter_aligned16,
                                      const short *vfilter_aligned16,
                                      unsigned char *dst_ptr,
                                      unsigned int dst_stride) {
  if (hfilter_aligned16[3] != 128 && vfilter_aligned16[3] != 128) {
    DECLARE_ALIGNED_ARRAY(16, unsigned char, fdata2, 23 * 16);

    vp9_filter_block1d16_h8_ssse3(src_ptr - (3 * src_stride), src_stride,
                                  fdata2, 16, 23, hfilter_aligned16);
    vp9_filter_block1d16_v8_ssse3(fdata2, 16, dst_ptr, dst_stride, 16,
                                  vfilter_aligned16);
  } else {
    if (hfilter_aligned16[3] != 128) {
      vp9_filter_block1d16_h8_ssse3(src_ptr, src_stride, dst_ptr, dst_stride,
                                    16, hfilter_aligned16);
    } else {
      vp9_filter_block1d16_v8_ssse3(src_ptr - (3 * src_stride), src_stride,
                                    dst_ptr, dst_stride, 16, vfilter_aligned16);
    }
  }
}

void vp9_filter_block1d8_v8_ssse3(const unsigned char *src_ptr,
                                   const unsigned int src_pitch,
                                   unsigned char *output_ptr,
                                   unsigned int out_pitch,
                                   unsigned int output_height,
                                   const short *filter);

void vp9_filter_block1d8_h8_ssse3(const unsigned char *src_ptr,
                                   const unsigned int src_pitch,
                                   unsigned char *output_ptr,
                                   unsigned int out_pitch,
                                   unsigned int output_height,
                                   const short *filter);

void vp9_filter_block2d_8x8_8_ssse3(const unsigned char *src_ptr,
                                    const unsigned int src_stride,
                                    const short *hfilter_aligned16,
                                    const short *vfilter_aligned16,
                                    unsigned char *dst_ptr,
                                    unsigned int dst_stride) {
  if (hfilter_aligned16[3] != 128 && vfilter_aligned16[3] != 128) {
    DECLARE_ALIGNED_ARRAY(16, unsigned char, fdata2, 23 * 16);

    vp9_filter_block1d8_h8_ssse3(src_ptr - (3 * src_stride), src_stride,
                                 fdata2, 16, 15, hfilter_aligned16);
    vp9_filter_block1d8_v8_ssse3(fdata2, 16, dst_ptr, dst_stride, 8,
                                 vfilter_aligned16);
  } else {
    if (hfilter_aligned16[3] != 128) {
      vp9_filter_block1d8_h8_ssse3(src_ptr, src_stride, dst_ptr, dst_stride, 8,
                                   hfilter_aligned16);
    } else {
      vp9_filter_block1d8_v8_ssse3(src_ptr - (3 * src_stride), src_stride,
                                   dst_ptr, dst_stride, 8, vfilter_aligned16);
    }
  }
}

void vp9_filter_block2d_8x4_8_ssse3(const unsigned char *src_ptr,
                                    const unsigned int src_stride,
                                    const short *hfilter_aligned16,
                                    const short *vfilter_aligned16,
                                    unsigned char *dst_ptr,
                                    unsigned int dst_stride) {
  if (hfilter_aligned16[3] !=128 && vfilter_aligned16[3] != 128) {
      DECLARE_ALIGNED_ARRAY(16, unsigned char, fdata2, 23 * 16);

      vp9_filter_block1d8_h8_ssse3(src_ptr - (3 * src_stride), src_stride,
                                   fdata2, 16, 11, hfilter_aligned16);
      vp9_filter_block1d8_v8_ssse3(fdata2, 16, dst_ptr, dst_stride, 4,
                                   vfilter_aligned16);
  } else {
    if (hfilter_aligned16[3] != 128) {
      vp9_filter_block1d8_h8_ssse3(src_ptr, src_stride, dst_ptr, dst_stride, 4,
                                   hfilter_aligned16);
    } else {
      vp9_filter_block1d8_v8_ssse3(src_ptr - (3 * src_stride), src_stride,
                                   dst_ptr, dst_stride, 4, vfilter_aligned16);
    }
  }
}
#endif
