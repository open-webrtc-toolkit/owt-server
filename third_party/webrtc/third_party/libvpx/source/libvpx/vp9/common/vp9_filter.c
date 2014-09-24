/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <stdlib.h>
#include "vp9/common/vp9_filter.h"
#include "vpx_ports/mem.h"
#include "vp9_rtcd.h"
#include "vp9/common/vp9_common.h"

DECLARE_ALIGNED(16, const int16_t, vp9_bilinear_filters[SUBPEL_SHIFTS][2]) = {
  { 128,   0 },
  { 120,   8 },
  { 112,  16 },
  { 104,  24 },
  {  96,  32 },
  {  88,  40 },
  {  80,  48 },
  {  72,  56 },
  {  64,  64 },
  {  56,  72 },
  {  48,  80 },
  {  40,  88 },
  {  32,  96 },
  {  24, 104 },
  {  16, 112 },
  {   8, 120 }
};

#define FILTER_ALPHA       0
#define FILTER_ALPHA_SHARP 1
DECLARE_ALIGNED(16, const int16_t, vp9_sub_pel_filters_8[SUBPEL_SHIFTS][8]) = {
#if FILTER_ALPHA == 0
  /* Lagrangian interpolation filter */
  { 0,   0,   0, 128,   0,   0,   0,  0},
  { 0,   1,  -5, 126,   8,  -3,   1,  0},
  { -1,   3, -10, 122,  18,  -6,   2,  0},
  { -1,   4, -13, 118,  27,  -9,   3, -1},
  { -1,   4, -16, 112,  37, -11,   4, -1},
  { -1,   5, -18, 105,  48, -14,   4, -1},
  { -1,   5, -19,  97,  58, -16,   5, -1},
  { -1,   6, -19,  88,  68, -18,   5, -1},
  { -1,   6, -19,  78,  78, -19,   6, -1},
  { -1,   5, -18,  68,  88, -19,   6, -1},
  { -1,   5, -16,  58,  97, -19,   5, -1},
  { -1,   4, -14,  48, 105, -18,   5, -1},
  { -1,   4, -11,  37, 112, -16,   4, -1},
  { -1,   3,  -9,  27, 118, -13,   4, -1},
  { 0,   2,  -6,  18, 122, -10,   3, -1},
  { 0,   1,  -3,   8, 126,  -5,   1,  0}
#elif FILTER_ALPHA == 50
  /* Generated using MATLAB:
   * alpha = 0.5;
   * b=intfilt(8,4,alpha);
   * bi=round(128*b);
   * ba=flipud(reshape([bi 0], 8, 8));
   * disp(num2str(ba, '%d,'))
   */
  { 0,   0,   0, 128,   0,   0,   0,  0},
  { 0,   1,  -5, 126,   8,  -3,   1,  0},
  { 0,   2, -10, 122,  18,  -6,   2,  0},
  { -1,   3, -13, 118,  27,  -9,   3,  0},
  { -1,   4, -16, 112,  37, -11,   3,  0},
  { -1,   5, -17, 104,  48, -14,   4, -1},
  { -1,   5, -18,  96,  58, -16,   5, -1},
  { -1,   5, -19,  88,  68, -17,   5, -1},
  { -1,   5, -18,  78,  78, -18,   5, -1},
  { -1,   5, -17,  68,  88, -19,   5, -1},
  { -1,   5, -16,  58,  96, -18,   5, -1},
  { -1,   4, -14,  48, 104, -17,   5, -1},
  { 0,   3, -11,  37, 112, -16,   4, -1},
  { 0,   3,  -9,  27, 118, -13,   3, -1},
  { 0,   2,  -6,  18, 122, -10,   2,  0},
  { 0,   1,  -3,   8, 126,  -5,   1,  0}
#endif  /* FILTER_ALPHA */
};

DECLARE_ALIGNED(16, const int16_t, vp9_sub_pel_filters_8s[SUBPEL_SHIFTS][8]) = {
#if FILTER_ALPHA_SHARP == 1
  /* dct based filter */
  {0,   0,   0, 128,   0,   0,   0, 0},
  {-1,   3,  -7, 127,   8,  -3,   1, 0},
  {-2,   5, -13, 125,  17,  -6,   3, -1},
  {-3,   7, -17, 121,  27, -10,   5, -2},
  {-4,   9, -20, 115,  37, -13,   6, -2},
  {-4,  10, -23, 108,  48, -16,   8, -3},
  {-4,  10, -24, 100,  59, -19,   9, -3},
  {-4,  11, -24,  90,  70, -21,  10, -4},
  {-4,  11, -23,  80,  80, -23,  11, -4},
  {-4,  10, -21,  70,  90, -24,  11, -4},
  {-3,   9, -19,  59, 100, -24,  10, -4},
  {-3,   8, -16,  48, 108, -23,  10, -4},
  {-2,   6, -13,  37, 115, -20,   9, -4},
  {-2,   5, -10,  27, 121, -17,   7, -3},
  {-1,   3,  -6,  17, 125, -13,   5, -2},
  {0,   1,  -3,   8, 127,  -7,   3, -1}
#elif FILTER_ALPHA_SHARP == 75
  /* alpha = 0.75 */
  {0,   0,   0, 128,   0,   0,   0, 0},
  {-1,   2,  -6, 126,   9,  -3,   2, -1},
  {-1,   4, -11, 123,  18,  -7,   3, -1},
  {-2,   6, -16, 119,  28, -10,   5, -2},
  {-2,   7, -19, 113,  38, -13,   6, -2},
  {-3,   8, -21, 106,  49, -16,   7, -2},
  {-3,   9, -22,  99,  59, -19,   8, -3},
  {-3,   9, -23,  90,  70, -21,   9, -3},
  {-3,   9, -22,  80,  80, -22,   9, -3},
  {-3,   9, -21,  70,  90, -23,   9, -3},
  {-3,   8, -19,  59,  99, -22,   9, -3},
  {-2,   7, -16,  49, 106, -21,   8, -3},
  {-2,   6, -13,  38, 113, -19,   7, -2},
  {-2,   5, -10,  28, 119, -16,   6, -2},
  {-1,   3,  -7,  18, 123, -11,   4, -1},
  {-1,   2,  -3,   9, 126,  -6,   2, -1}
#endif  /* FILTER_ALPHA_SHARP */
};

DECLARE_ALIGNED(16, const int16_t,
                vp9_sub_pel_filters_8lp[SUBPEL_SHIFTS][8]) = {
  /* 8-tap lowpass filter */
  /* Hamming window */
  {-1, -7, 32, 80, 32, -7, -1,  0},
  {-1, -8, 28, 80, 37, -7, -2,  1},
  { 0, -8, 24, 79, 41, -7, -2,  1},
  { 0, -8, 20, 78, 45, -5, -3,  1},
  { 0, -8, 16, 76, 50, -4, -3,  1},
  { 0, -7, 13, 74, 54, -3, -4,  1},
  { 1, -7,  9, 71, 58, -1, -4,  1},
  { 1, -6,  6, 68, 62,  1, -5,  1},
  { 1, -6,  4, 65, 65,  4, -6,  1},
  { 1, -5,  1, 62, 68,  6, -6,  1},
  { 1, -4, -1, 58, 71,  9, -7,  1},
  { 1, -4, -3, 54, 74, 13, -7,  0},
  { 1, -3, -4, 50, 76, 16, -8,  0},
  { 1, -3, -5, 45, 78, 20, -8,  0},
  { 1, -2, -7, 41, 79, 24, -8,  0},
  { 1, -2, -7, 37, 80, 28, -8, -1}
};

DECLARE_ALIGNED(16, const int16_t, vp9_sub_pel_filters_6[SUBPEL_SHIFTS][6]) = {
  {0,   0, 128,   0,   0, 0},
  {1,  -5, 125,   8,  -2, 1},
  {1,  -8, 122,  17,  -5, 1},
  {2, -11, 116,  27,  -8, 2},
  {3, -14, 110,  37, -10, 2},
  {3, -15, 103,  47, -12, 2},
  {3, -16,  95,  57, -14, 3},
  {3, -16,  86,  67, -15, 3},
  {3, -16,  77,  77, -16, 3},
  {3, -15,  67,  86, -16, 3},
  {3, -14,  57,  95, -16, 3},
  {2, -12,  47, 103, -15, 3},
  {2, -10,  37, 110, -14, 3},
  {2,  -8,  27, 116, -11, 2},
  {1,  -5,  17, 122,  -8, 1},
  {1,  -2,   8, 125,  -5, 1}
};

static void filter_block2d_first_pass_6(uint8_t *src_ptr,
                                        int *output_ptr,
                                        unsigned int src_pixels_per_line,
                                        unsigned int pixel_step,
                                        unsigned int output_height,
                                        unsigned int output_width,
                                        const int16_t *vp9_filter) {
  unsigned int i, j;
  int temp;

  for (i = 0; i < output_height; i++) {
    for (j = 0; j < output_width; j++) {
      temp = ((int)src_ptr[-2 * (int)pixel_step] * vp9_filter[0]) +
             ((int)src_ptr[-1 * (int)pixel_step] * vp9_filter[1]) +
             ((int)src_ptr[0]                    * vp9_filter[2]) +
             ((int)src_ptr[pixel_step]           * vp9_filter[3]) +
             ((int)src_ptr[2 * pixel_step]       * vp9_filter[4]) +
             ((int)src_ptr[3 * pixel_step]       * vp9_filter[5]) +
             (VP9_FILTER_WEIGHT >> 1);      /* Rounding */

      /* Normalize back to 0-255 */
      output_ptr[j] = clip_pixel(temp >> VP9_FILTER_SHIFT);
      src_ptr++;
    }

    /* Next row... */
    src_ptr    += src_pixels_per_line - output_width;
    output_ptr += output_width;
  }
}

static void filter_block2d_second_pass_6(int *src_ptr,
                                         uint8_t *output_ptr,
                                         int output_pitch,
                                         unsigned int src_pixels_per_line,
                                         unsigned int pixel_step,
                                         unsigned int output_height,
                                         unsigned int output_width,
                                         const int16_t *vp9_filter) {
  unsigned int i, j;
  int temp;

  for (i = 0; i < output_height; i++) {
    for (j = 0; j < output_width; j++) {
      /* Apply filter */
      temp = ((int)src_ptr[-2 * (int)pixel_step] * vp9_filter[0]) +
             ((int)src_ptr[-1 * (int)pixel_step] * vp9_filter[1]) +
             ((int)src_ptr[0]                    * vp9_filter[2]) +
             ((int)src_ptr[pixel_step]           * vp9_filter[3]) +
             ((int)src_ptr[2 * pixel_step]         * vp9_filter[4]) +
             ((int)src_ptr[3 * pixel_step]         * vp9_filter[5]) +
             (VP9_FILTER_WEIGHT >> 1);   /* Rounding */

      /* Normalize back to 0-255 */
      output_ptr[j] = clip_pixel(temp >> VP9_FILTER_SHIFT);
      src_ptr++;
    }

    /* Start next row */
    src_ptr    += src_pixels_per_line - output_width;
    output_ptr += output_pitch;
  }
}

/*
 * The only functional difference between filter_block2d_second_pass()
 * and this function is that filter_block2d_second_pass() does a sixtap
 * filter on the input and stores it in the output. This function
 * (filter_block2d_second_pass_avg()) does a sixtap filter on the input,
 * and then averages that with the content already present in the output
 * ((filter_result + dest + 1) >> 1) and stores that in the output.
 */
static void filter_block2d_second_pass_avg_6(int *src_ptr,
                                             uint8_t *output_ptr,
                                             int output_pitch,
                                             unsigned int src_pixels_per_line,
                                             unsigned int pixel_step,
                                             unsigned int output_height,
                                             unsigned int output_width,
                                             const int16_t *vp9_filter) {
  unsigned int i, j;
  int temp;

  for (i = 0; i < output_height; i++) {
    for (j = 0; j < output_width; j++) {
      /* Apply filter */
      temp = ((int)src_ptr[-2 * (int)pixel_step] * vp9_filter[0]) +
             ((int)src_ptr[-1 * (int)pixel_step] * vp9_filter[1]) +
             ((int)src_ptr[0]                    * vp9_filter[2]) +
             ((int)src_ptr[pixel_step]           * vp9_filter[3]) +
             ((int)src_ptr[2 * pixel_step]         * vp9_filter[4]) +
             ((int)src_ptr[3 * pixel_step]         * vp9_filter[5]) +
             (VP9_FILTER_WEIGHT >> 1);   /* Rounding */

      /* Normalize back to 0-255 */
      output_ptr[j] = (clip_pixel(temp >> VP9_FILTER_SHIFT) +
                       output_ptr[j] + 1) >> 1;
      src_ptr++;
    }

    /* Start next row */
    src_ptr    += src_pixels_per_line - output_width;
    output_ptr += output_pitch;
  }
}

#define Interp_Extend 3
static void filter_block2d_6(uint8_t *src_ptr,
                             uint8_t *output_ptr,
                             unsigned int src_pixels_per_line,
                             int output_pitch,
                             const int16_t *HFilter,
                             const int16_t *VFilter) {
  int FData[(3 + Interp_Extend * 2) * 4]; /* Temp data buffer */

  /* First filter 1-D horizontally... */
  filter_block2d_first_pass_6(
      src_ptr - ((Interp_Extend - 1) * src_pixels_per_line), FData,
      src_pixels_per_line, 1, 3 + Interp_Extend * 2, 4, HFilter);

  /* then filter vertically... */
  filter_block2d_second_pass_6(FData + 4 * (Interp_Extend - 1), output_ptr,
                               output_pitch, 4, 4, 4, 4, VFilter);
}


void vp9_sixtap_predict4x4_c(uint8_t *src_ptr,
                             int src_pixels_per_line,
                             int xoffset,
                             int yoffset,
                             uint8_t *dst_ptr,
                             int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_sub_pel_filters_6[xoffset];   /* 6 tap */
  VFilter = vp9_sub_pel_filters_6[yoffset];   /* 6 tap */

  filter_block2d_6(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter,
                   VFilter);
}

/*
 * The difference between filter_block2d_6() and filter_block2d_avg_6 is
 * that filter_block2d_6() does a 6-tap filter and stores it in the output
 * buffer, whereas filter_block2d_avg_6() does the same 6-tap filter, and
 * then averages that with the content already present in the output
 * ((filter_result + dest + 1) >> 1) and stores that in the output.
 */
static void filter_block2d_avg_6(uint8_t *src_ptr,
                                 uint8_t *output_ptr,
                                 unsigned int src_pixels_per_line,
                                 int output_pitch,
                                 const int16_t *HFilter,
                                 const int16_t *VFilter) {
  int FData[(3 + Interp_Extend * 2) * 4]; /* Temp data buffer */

  /* First filter 1-D horizontally... */
  filter_block2d_first_pass_6(
      src_ptr - ((Interp_Extend - 1) * src_pixels_per_line), FData,
      src_pixels_per_line, 1, 3 + Interp_Extend * 2, 4, HFilter);

  /* then filter vertically... */
  filter_block2d_second_pass_avg_6(FData + 4 * (Interp_Extend - 1), output_ptr,
                                   output_pitch, 4, 4, 4, 4, VFilter);
}

void vp9_sixtap_predict_avg4x4_c(uint8_t *src_ptr,
                                 int src_pixels_per_line,
                                 int xoffset,
                                 int yoffset,
                                 uint8_t *dst_ptr,
                                 int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_sub_pel_filters_6[xoffset];   /* 6 tap */
  VFilter = vp9_sub_pel_filters_6[yoffset];   /* 6 tap */

  filter_block2d_avg_6(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch,
                       HFilter, VFilter);
}

void vp9_sixtap_predict8x8_c(uint8_t *src_ptr,
                             int src_pixels_per_line,
                             int xoffset,
                             int yoffset,
                             uint8_t *dst_ptr,
                             int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;
  int FData[(7 + Interp_Extend * 2) * 8]; /* Temp data buffer */

  HFilter = vp9_sub_pel_filters_6[xoffset];   /* 6 tap */
  VFilter = vp9_sub_pel_filters_6[yoffset];   /* 6 tap */

  /* First filter 1-D horizontally... */
  filter_block2d_first_pass_6(
      src_ptr - ((Interp_Extend - 1) * src_pixels_per_line), FData,
      src_pixels_per_line, 1, 7 + Interp_Extend * 2, 8, HFilter);

  /* then filter vertically... */
  filter_block2d_second_pass_6(FData + 8 * (Interp_Extend - 1), dst_ptr,
                               dst_pitch, 8, 8, 8, 8, VFilter);

}

void vp9_sixtap_predict_avg8x8_c(uint8_t *src_ptr,
                                 int src_pixels_per_line,
                                 int xoffset,
                                 int yoffset,
                                 uint8_t *dst_ptr,
                                 int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;
  int FData[(7 + Interp_Extend * 2) * 8]; /* Temp data buffer */

  HFilter = vp9_sub_pel_filters_6[xoffset];   /* 6 tap */
  VFilter = vp9_sub_pel_filters_6[yoffset];   /* 6 tap */

  /* First filter 1-D horizontally... */
  filter_block2d_first_pass_6(
      src_ptr - ((Interp_Extend - 1) * src_pixels_per_line), FData,
      src_pixels_per_line, 1, 7 + Interp_Extend * 2, 8, HFilter);

  /* then filter vertically... */
  filter_block2d_second_pass_avg_6(FData + 8 * (Interp_Extend - 1), dst_ptr,
                                   dst_pitch, 8, 8, 8, 8, VFilter);
}

void vp9_sixtap_predict8x4_c(uint8_t *src_ptr,
                             int src_pixels_per_line,
                             int xoffset,
                             int yoffset,
                             uint8_t *dst_ptr,
                             int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;
  int FData[(3 + Interp_Extend * 2) * 8]; /* Temp data buffer */

  HFilter = vp9_sub_pel_filters_6[xoffset];   /* 6 tap */
  VFilter = vp9_sub_pel_filters_6[yoffset];   /* 6 tap */

  /* First filter 1-D horizontally... */
  filter_block2d_first_pass_6(
      src_ptr - ((Interp_Extend - 1) * src_pixels_per_line), FData,
      src_pixels_per_line, 1, 3 + Interp_Extend * 2, 8, HFilter);

  /* then filter vertically... */
  filter_block2d_second_pass_6(FData + 8 * (Interp_Extend - 1), dst_ptr,
                               dst_pitch, 8, 8, 4, 8, VFilter);
}

void vp9_sixtap_predict16x16_c(uint8_t *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               uint8_t *dst_ptr,
                               int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;
  int FData[(15 + Interp_Extend * 2) * 16]; /* Temp data buffer */

  HFilter = vp9_sub_pel_filters_6[xoffset];   /* 6 tap */
  VFilter = vp9_sub_pel_filters_6[yoffset];   /* 6 tap */

  /* First filter 1-D horizontally... */
  filter_block2d_first_pass_6(
      src_ptr - ((Interp_Extend - 1) * src_pixels_per_line), FData,
      src_pixels_per_line, 1, 15 + Interp_Extend * 2, 16, HFilter);

  /* then filter vertically... */
  filter_block2d_second_pass_6(FData + 16 * (Interp_Extend - 1), dst_ptr,
                               dst_pitch, 16, 16, 16, 16, VFilter);
}

void vp9_sixtap_predict_avg16x16_c(uint8_t *src_ptr,
                                   int src_pixels_per_line,
                                   int xoffset,
                                   int yoffset,
                                   uint8_t *dst_ptr,
                                   int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;
  int FData[(15 + Interp_Extend * 2) * 16]; /* Temp data buffer */

  HFilter = vp9_sub_pel_filters_6[xoffset];   /* 6 tap */
  VFilter = vp9_sub_pel_filters_6[yoffset];   /* 6 tap */

  /* First filter 1-D horizontally... */
  filter_block2d_first_pass_6(
      src_ptr - ((Interp_Extend - 1) * src_pixels_per_line), FData,
      src_pixels_per_line, 1, 15 + Interp_Extend * 2, 16, HFilter);

  /* then filter vertically... */
  filter_block2d_second_pass_avg_6(FData + 16 * (Interp_Extend - 1), dst_ptr,
                                   dst_pitch, 16, 16, 16, 16, VFilter);
}

typedef enum {
  VPX_FILTER_4x4 = 0,
  VPX_FILTER_8x8 = 1,
  VPX_FILTER_8x4 = 2,
  VPX_FILTER_16x16 = 3,
} filter_size_t;

static const unsigned int filter_size_to_wh[][2] = {
  {4, 4},
  {8, 8},
  {8, 4},
  {16,16},
};

static void filter_block2d_8_c(const uint8_t *src_ptr,
                               const unsigned int src_stride,
                               const int16_t *HFilter,
                               const int16_t *VFilter,
                               const filter_size_t filter_size,
                               uint8_t *dst_ptr,
                               unsigned int dst_stride) {
  const unsigned int output_width = filter_size_to_wh[filter_size][0];
  const unsigned int output_height = filter_size_to_wh[filter_size][1];

  // Between passes, we use an intermediate buffer whose height is extended to
  // have enough horizontally filtered values as input for the vertical pass.
  // This buffer is allocated to be big enough for the largest block type we
  // support.
  const int kInterp_Extend = 4;
  const unsigned int intermediate_height =
    (kInterp_Extend - 1) +     output_height + kInterp_Extend;

  /* Size of intermediate_buffer is max_intermediate_height * filter_max_width,
   * where max_intermediate_height = (kInterp_Extend - 1) + filter_max_height
   *                                 + kInterp_Extend
   *                               = 3 + 16 + 4
   *                               = 23
   * and filter_max_width = 16
   */
  uint8_t intermediate_buffer[23 * 16];
  const int intermediate_next_stride = 1 - intermediate_height * output_width;

  // Horizontal pass (src -> transposed intermediate).
  {
    uint8_t *output_ptr = intermediate_buffer;
    const int src_next_row_stride = src_stride - output_width;
    unsigned int i, j;
    src_ptr -= (kInterp_Extend - 1) * src_stride + (kInterp_Extend - 1);
    for (i = 0; i < intermediate_height; i++) {
      for (j = 0; j < output_width; j++) {
        // Apply filter...
        int temp = ((int)src_ptr[0] * HFilter[0]) +
                   ((int)src_ptr[1] * HFilter[1]) +
                   ((int)src_ptr[2] * HFilter[2]) +
                   ((int)src_ptr[3] * HFilter[3]) +
                   ((int)src_ptr[4] * HFilter[4]) +
                   ((int)src_ptr[5] * HFilter[5]) +
                   ((int)src_ptr[6] * HFilter[6]) +
                   ((int)src_ptr[7] * HFilter[7]) +
                   (VP9_FILTER_WEIGHT >> 1); // Rounding

        // Normalize back to 0-255...
        *output_ptr = clip_pixel(temp >> VP9_FILTER_SHIFT);
        src_ptr++;
        output_ptr += intermediate_height;
      }
      src_ptr += src_next_row_stride;
      output_ptr += intermediate_next_stride;
    }
  }

  // Vertical pass (transposed intermediate -> dst).
  {
    uint8_t *src_ptr = intermediate_buffer;
    const int dst_next_row_stride = dst_stride - output_width;
    unsigned int i, j;
    for (i = 0; i < output_height; i++) {
      for (j = 0; j < output_width; j++) {
        // Apply filter...
        int temp = ((int)src_ptr[0] * VFilter[0]) +
                   ((int)src_ptr[1] * VFilter[1]) +
                   ((int)src_ptr[2] * VFilter[2]) +
                   ((int)src_ptr[3] * VFilter[3]) +
                   ((int)src_ptr[4] * VFilter[4]) +
                   ((int)src_ptr[5] * VFilter[5]) +
                   ((int)src_ptr[6] * VFilter[6]) +
                   ((int)src_ptr[7] * VFilter[7]) +
                   (VP9_FILTER_WEIGHT >> 1); // Rounding

        // Normalize back to 0-255...
        *dst_ptr++ = clip_pixel(temp >> VP9_FILTER_SHIFT);
        src_ptr += intermediate_height;
      }
      src_ptr += intermediate_next_stride;
      dst_ptr += dst_next_row_stride;
    }
  }
}

void vp9_filter_block2d_4x4_8_c(const uint8_t *src_ptr,
                                const unsigned int src_stride,
                                const int16_t *HFilter_aligned16,
                                const int16_t *VFilter_aligned16,
                                uint8_t *dst_ptr,
                                unsigned int dst_stride) {
  filter_block2d_8_c(src_ptr, src_stride, HFilter_aligned16, VFilter_aligned16,
                     VPX_FILTER_4x4, dst_ptr, dst_stride);
}

void vp9_filter_block2d_8x4_8_c(const uint8_t *src_ptr,
                                const unsigned int src_stride,
                                const int16_t *HFilter_aligned16,
                                const int16_t *VFilter_aligned16,
                                uint8_t *dst_ptr,
                                unsigned int dst_stride) {
  filter_block2d_8_c(src_ptr, src_stride, HFilter_aligned16, VFilter_aligned16,
                     VPX_FILTER_8x4, dst_ptr, dst_stride);
}

void vp9_filter_block2d_8x8_8_c(const uint8_t *src_ptr,
                                const unsigned int src_stride,
                                const int16_t *HFilter_aligned16,
                                const int16_t *VFilter_aligned16,
                                uint8_t *dst_ptr,
                                unsigned int dst_stride) {
  filter_block2d_8_c(src_ptr, src_stride, HFilter_aligned16, VFilter_aligned16,
                     VPX_FILTER_8x8, dst_ptr, dst_stride);
}

void vp9_filter_block2d_16x16_8_c(const uint8_t *src_ptr,
                                  const unsigned int src_stride,
                                  const int16_t *HFilter_aligned16,
                                  const int16_t *VFilter_aligned16,
                                  uint8_t *dst_ptr,
                                  unsigned int dst_stride) {
  filter_block2d_8_c(src_ptr, src_stride, HFilter_aligned16, VFilter_aligned16,
                     VPX_FILTER_16x16, dst_ptr, dst_stride);
}

static void block2d_average_c(uint8_t *src,
                              unsigned int src_stride,
                              uint8_t *output_ptr,
                              unsigned int output_stride,
                              const filter_size_t filter_size) {
  const unsigned int output_width = filter_size_to_wh[filter_size][0];
  const unsigned int output_height = filter_size_to_wh[filter_size][1];

  unsigned int i, j;
  for (i = 0; i < output_height; i++) {
    for (j = 0; j < output_width; j++) {
      output_ptr[j] = (output_ptr[j] + src[i * src_stride + j] + 1) >> 1;
    }
    output_ptr += output_stride;
  }
}

#define block2d_average block2d_average_c

void vp9_eighttap_predict4x4_c(uint8_t *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               uint8_t *dst_ptr,
                               int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_sub_pel_filters_8[xoffset];
  VFilter = vp9_sub_pel_filters_8[yoffset];

  vp9_filter_block2d_4x4_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                           dst_ptr, dst_pitch);
}

void vp9_eighttap_predict_avg4x4_c(uint8_t *src_ptr,
                                   int src_pixels_per_line,
                                   int xoffset,
                                   int yoffset,
                                   uint8_t *dst_ptr,
                                   int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8[yoffset];
  uint8_t tmp[4 * 4];

  vp9_filter_block2d_4x4_8(src_ptr, src_pixels_per_line, HFilter, VFilter, tmp,
                           4);
  block2d_average(tmp, 4, dst_ptr, dst_pitch, VPX_FILTER_4x4);
}

void vp9_eighttap_predict4x4_sharp_c(uint8_t *src_ptr,
                                     int src_pixels_per_line,
                                     int xoffset,
                                     int yoffset,
                                     uint8_t *dst_ptr,
                                     int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_sub_pel_filters_8s[xoffset];
  VFilter = vp9_sub_pel_filters_8s[yoffset];

  vp9_filter_block2d_4x4_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                           dst_ptr, dst_pitch);
}

void vp9_eighttap_predict4x4_smooth_c(uint8_t *src_ptr,
                                      int src_pixels_per_line,
                                      int xoffset,
                                      int yoffset,
                                      uint8_t *dst_ptr,
                                      int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_sub_pel_filters_8lp[xoffset];
  VFilter = vp9_sub_pel_filters_8lp[yoffset];

  vp9_filter_block2d_4x4_8(src_ptr, src_pixels_per_line,
                           HFilter, VFilter,
                           dst_ptr, dst_pitch);
}

void vp9_eighttap_predict_avg4x4_sharp_c(uint8_t *src_ptr,
                                         int src_pixels_per_line,
                                         int xoffset,
                                         int yoffset,
                                         uint8_t *dst_ptr,
                                         int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8s[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8s[yoffset];
  uint8_t tmp[4 * 4];

  vp9_filter_block2d_4x4_8(src_ptr, src_pixels_per_line, HFilter, VFilter, tmp,
                           4);
  block2d_average(tmp, 4, dst_ptr, dst_pitch, VPX_FILTER_4x4);
}

void vp9_eighttap_predict_avg4x4_smooth_c(uint8_t *src_ptr,
                                          int src_pixels_per_line,
                                          int xoffset,
                                          int yoffset,
                                          uint8_t *dst_ptr,
                                          int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8lp[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8lp[yoffset];
  uint8_t tmp[4 * 4];

  vp9_filter_block2d_4x4_8(src_ptr, src_pixels_per_line, HFilter, VFilter, tmp,
                           4);
  block2d_average(tmp, 4, dst_ptr, dst_pitch, VPX_FILTER_4x4);
}


void vp9_eighttap_predict8x8_c(uint8_t *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               uint8_t *dst_ptr,
                               int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8[yoffset];

  vp9_filter_block2d_8x8_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                           dst_ptr, dst_pitch);
}

void vp9_eighttap_predict8x8_sharp_c(uint8_t *src_ptr,
                                     int src_pixels_per_line,
                                     int xoffset,
                                     int yoffset,
                                     uint8_t *dst_ptr,
                                     int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8s[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8s[yoffset];

  vp9_filter_block2d_8x8_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                           dst_ptr, dst_pitch);
}

void vp9_eighttap_predict8x8_smooth_c(uint8_t *src_ptr,
                                      int src_pixels_per_line,
                                      int xoffset,
                                      int yoffset,
                                      uint8_t *dst_ptr,
                                      int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8lp[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8lp[yoffset];

  vp9_filter_block2d_8x8_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                           dst_ptr, dst_pitch);
}

void vp9_eighttap_predict_avg8x8_c(uint8_t *src_ptr,
                                   int src_pixels_per_line,
                                   int xoffset,
                                   int yoffset,
                                   uint8_t *dst_ptr,
                                   int dst_pitch) {
  uint8_t tmp[8 * 8];
  const int16_t *HFilter = vp9_sub_pel_filters_8[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8[yoffset];

  vp9_filter_block2d_8x8_8(src_ptr, src_pixels_per_line, HFilter, VFilter, tmp,
                           8);
  block2d_average(tmp, 8, dst_ptr, dst_pitch, VPX_FILTER_8x8);
}

void vp9_eighttap_predict_avg8x8_sharp_c(uint8_t *src_ptr,
                                         int src_pixels_per_line,
                                         int xoffset,
                                         int yoffset,
                                         uint8_t *dst_ptr,
                                         int dst_pitch) {
  uint8_t tmp[8 * 8];
  const int16_t *HFilter = vp9_sub_pel_filters_8s[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8s[yoffset];

  vp9_filter_block2d_8x8_8(src_ptr, src_pixels_per_line, HFilter, VFilter, tmp,
                           8);
  block2d_average(tmp, 8, dst_ptr, dst_pitch, VPX_FILTER_8x8);
}

void vp9_eighttap_predict_avg8x8_smooth_c(uint8_t *src_ptr,
                                          int src_pixels_per_line,
                                          int xoffset,
                                          int yoffset,
                                          uint8_t *dst_ptr,
                                          int dst_pitch) {
  uint8_t tmp[8 * 8];
  const int16_t *HFilter = vp9_sub_pel_filters_8lp[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8lp[yoffset];

  vp9_filter_block2d_8x8_8(src_ptr, src_pixels_per_line, HFilter, VFilter, tmp,
                           8);
  block2d_average(tmp, 8, dst_ptr, dst_pitch, VPX_FILTER_8x8);
}

void vp9_eighttap_predict8x4_c(uint8_t *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               uint8_t *dst_ptr,
                               int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8[yoffset];

  vp9_filter_block2d_8x4_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                           dst_ptr, dst_pitch);
}

void vp9_eighttap_predict8x4_sharp_c(uint8_t *src_ptr,
                                     int src_pixels_per_line,
                                     int xoffset,
                                     int yoffset,
                                     uint8_t *dst_ptr,
                                     int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8s[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8s[yoffset];

  vp9_filter_block2d_8x4_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                           dst_ptr, dst_pitch);
}

void vp9_eighttap_predict8x4_smooth_c(uint8_t *src_ptr,
                                      int src_pixels_per_line,
                                      int xoffset,
                                      int yoffset,
                                      uint8_t *dst_ptr,
                                      int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8lp[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8lp[yoffset];

  vp9_filter_block2d_8x4_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                           dst_ptr, dst_pitch);
}

void vp9_eighttap_predict16x16_c(uint8_t *src_ptr,
                                 int src_pixels_per_line,
                                 int xoffset,
                                 int yoffset,
                                 uint8_t *dst_ptr,
                                 int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8[yoffset];

  vp9_filter_block2d_16x16_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                             dst_ptr, dst_pitch);
}

void vp9_eighttap_predict16x16_sharp_c(uint8_t *src_ptr,
                                       int src_pixels_per_line,
                                       int xoffset,
                                       int yoffset,
                                       uint8_t *dst_ptr,
                                       int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8s[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8s[yoffset];

  vp9_filter_block2d_16x16_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                             dst_ptr, dst_pitch);
}

void vp9_eighttap_predict16x16_smooth_c(uint8_t *src_ptr,
                                        int src_pixels_per_line,
                                        int xoffset,
                                        int yoffset,
                                        uint8_t *dst_ptr,
                                        int dst_pitch) {
  const int16_t *HFilter = vp9_sub_pel_filters_8lp[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8lp[yoffset];

  vp9_filter_block2d_16x16_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                             dst_ptr, dst_pitch);
}

void vp9_eighttap_predict_avg16x16_c(uint8_t *src_ptr,
                                     int src_pixels_per_line,
                                     int xoffset,
                                     int yoffset,
                                     uint8_t *dst_ptr,
                                     int dst_pitch) {
  DECLARE_ALIGNED_ARRAY(16, uint8_t, tmp, 16 * 16);
  const int16_t *HFilter = vp9_sub_pel_filters_8[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8[yoffset];

  vp9_filter_block2d_16x16_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                             tmp, 16);
  block2d_average(tmp, 16, dst_ptr, dst_pitch, VPX_FILTER_16x16);
}

void vp9_eighttap_predict_avg16x16_sharp_c(uint8_t *src_ptr,
                                           int src_pixels_per_line,
                                           int xoffset,
                                           int yoffset,
                                           uint8_t *dst_ptr,
                                           int dst_pitch) {
  DECLARE_ALIGNED_ARRAY(16, uint8_t, tmp, 16 * 16);
  const int16_t *HFilter = vp9_sub_pel_filters_8s[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8s[yoffset];

  vp9_filter_block2d_16x16_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                             tmp, 16);
  block2d_average(tmp, 16, dst_ptr, dst_pitch, VPX_FILTER_16x16);
}

void vp9_eighttap_predict_avg16x16_smooth_c(uint8_t *src_ptr,
                                            int src_pixels_per_line,
                                            int xoffset,
                                            int yoffset,
                                            uint8_t *dst_ptr,
                                            int dst_pitch) {
  DECLARE_ALIGNED_ARRAY(16, uint8_t, tmp, 16 * 16);
  const int16_t *HFilter = vp9_sub_pel_filters_8lp[xoffset];
  const int16_t *VFilter = vp9_sub_pel_filters_8lp[yoffset];

  vp9_filter_block2d_16x16_8(src_ptr, src_pixels_per_line, HFilter, VFilter,
                             tmp, 16);
  block2d_average(tmp, 16, dst_ptr, dst_pitch, VPX_FILTER_16x16);
}

/****************************************************************************
 *
 *  ROUTINE       : filter_block2d_bil_first_pass
 *
 *  INPUTS        : uint8_t  *src_ptr    : Pointer to source block.
 *                  uint32_t  src_stride : Stride of source block.
 *                  uint32_t  height     : Block height.
 *                  uint32_t  width      : Block width.
 *                  int32_t  *vp9_filter : Array of 2 bi-linear filter taps.
 *
 *  OUTPUTS       : int32_t  *dst_ptr    : Pointer to filtered block.
 *
 *  RETURNS       : void
 *
 *  FUNCTION      : Applies a 1-D 2-tap bi-linear filter to the source block
 *                  in the horizontal direction to produce the filtered output
 *                  block. Used to implement first-pass of 2-D separable filter.
 *
 *  SPECIAL NOTES : Produces int32_t output to retain precision for next pass.
 *                  Two filter taps should sum to VP9_FILTER_WEIGHT.
 *
 ****************************************************************************/
static void filter_block2d_bil_first_pass(uint8_t *src_ptr,
                                          uint16_t *dst_ptr,
                                          unsigned int src_stride,
                                          unsigned int height,
                                          unsigned int width,
                                          const int16_t *vp9_filter) {
  unsigned int i, j;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      /* Apply bilinear filter */
      dst_ptr[j] = (((int)src_ptr[0] * vp9_filter[0]) +
                    ((int)src_ptr[1] * vp9_filter[1]) +
                    (VP9_FILTER_WEIGHT / 2)) >> VP9_FILTER_SHIFT;
      src_ptr++;
    }

    /* Next row... */
    src_ptr += src_stride - width;
    dst_ptr += width;
  }
}

/****************************************************************************
 *
 *  ROUTINE       : filter_block2d_bil_second_pass
 *
 *  INPUTS        : int32_t  *src_ptr    : Pointer to source block.
 *                  uint32_t  dst_pitch  : Destination block pitch.
 *                  uint32_t  height     : Block height.
 *                  uint32_t  width      : Block width.
 *                  int32_t  *vp9_filter : Array of 2 bi-linear filter taps.
 *
 *  OUTPUTS       : uint16_t *dst_ptr    : Pointer to filtered block.
 *
 *  RETURNS       : void
 *
 *  FUNCTION      : Applies a 1-D 2-tap bi-linear filter to the source block
 *                  in the vertical direction to produce the filtered output
 *                  block. Used to implement second-pass of 2-D separable filter.
 *
 *  SPECIAL NOTES : Requires 32-bit input as produced by filter_block2d_bil_first_pass.
 *                  Two filter taps should sum to VP9_FILTER_WEIGHT.
 *
 ****************************************************************************/
static void filter_block2d_bil_second_pass(uint16_t *src_ptr,
                                           uint8_t *dst_ptr,
                                           int dst_pitch,
                                           unsigned int height,
                                           unsigned int width,
                                           const int16_t *vp9_filter) {
  unsigned int i, j;
  int temp;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      /* Apply filter */
      temp = ((int)src_ptr[0]     * vp9_filter[0]) +
             ((int)src_ptr[width] * vp9_filter[1]) +
             (VP9_FILTER_WEIGHT / 2);
      dst_ptr[j] = (unsigned int)(temp >> VP9_FILTER_SHIFT);
      src_ptr++;
    }

    /* Next row... */
    dst_ptr += dst_pitch;
  }
}

/*
 * As before for filter_block2d_second_pass_avg(), the functional difference
 * between filter_block2d_bil_second_pass() and filter_block2d_bil_second_pass_avg()
 * is that filter_block2d_bil_second_pass() does a bilinear filter on input
 * and stores the result in output; filter_block2d_bil_second_pass_avg(),
 * instead, does a bilinear filter on input, averages the resulting value
 * with the values already present in the output and stores the result of
 * that back into the output ((filter_result + dest + 1) >> 1).
 */
static void filter_block2d_bil_second_pass_avg(uint16_t *src_ptr,
                                               uint8_t *dst_ptr,
                                               int dst_pitch,
                                               unsigned int height,
                                               unsigned int width,
                                               const int16_t *vp9_filter) {
  unsigned int i, j;
  int temp;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      /* Apply filter */
      temp = (((int)src_ptr[0]     * vp9_filter[0]) +
              ((int)src_ptr[width] * vp9_filter[1]) +
              (VP9_FILTER_WEIGHT / 2)) >> VP9_FILTER_SHIFT;
      dst_ptr[j] = (unsigned int)((temp + dst_ptr[j] + 1) >> 1);
      src_ptr++;
    }

    /* Next row... */
    dst_ptr += dst_pitch;
  }
}

/****************************************************************************
 *
 *  ROUTINE       : filter_block2d_bil
 *
 *  INPUTS        : uint8_t  *src_ptr          : Pointer to source block.
 *                  uint32_t  src_pitch        : Stride of source block.
 *                  uint32_t  dst_pitch        : Stride of destination block.
 *                  int32_t  *HFilter          : Array of 2 horizontal filter taps.
 *                  int32_t  *VFilter          : Array of 2 vertical filter taps.
 *                  int32_t  Width             : Block width
 *                  int32_t  Height            : Block height
 *
 *  OUTPUTS       : uint16_t *dst_ptr       : Pointer to filtered block.
 *
 *  RETURNS       : void
 *
 *  FUNCTION      : 2-D filters an input block by applying a 2-tap
 *                  bi-linear filter horizontally followed by a 2-tap
 *                  bi-linear filter vertically on the result.
 *
 *  SPECIAL NOTES : The largest block size can be handled here is 16x16
 *
 ****************************************************************************/
static void filter_block2d_bil(uint8_t *src_ptr,
                               uint8_t *dst_ptr,
                               unsigned int src_pitch,
                               unsigned int dst_pitch,
                               const int16_t *HFilter,
                               const int16_t *VFilter,
                               int Width,
                               int Height) {

  uint16_t FData[17 * 16];  /* Temp data buffer used in filtering */

  /* First filter 1-D horizontally... */
  filter_block2d_bil_first_pass(src_ptr, FData, src_pitch, Height + 1, Width, HFilter);

  /* then 1-D vertically... */
  filter_block2d_bil_second_pass(FData, dst_ptr, dst_pitch, Height, Width, VFilter);
}

static void filter_block2d_bil_avg(uint8_t *src_ptr,
                                   uint8_t *dst_ptr,
                                   unsigned int src_pitch,
                                   unsigned int dst_pitch,
                                   const int16_t *HFilter,
                                   const int16_t *VFilter,
                                   int Width,
                                   int Height) {
  uint16_t FData[17 * 16];  /* Temp data buffer used in filtering */

  /* First filter 1-D horizontally... */
  filter_block2d_bil_first_pass(src_ptr, FData, src_pitch, Height + 1, Width, HFilter);

  /* then 1-D vertically... */
  filter_block2d_bil_second_pass_avg(FData, dst_ptr, dst_pitch, Height, Width, VFilter);
}

void vp9_bilinear_predict4x4_c(uint8_t *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               uint8_t *dst_ptr,
                               int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_bilinear_filters[xoffset];
  VFilter = vp9_bilinear_filters[yoffset];

  filter_block2d_bil(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 4, 4);
}

void vp9_bilinear_predict_avg4x4_c(uint8_t *src_ptr,
                                   int src_pixels_per_line,
                                   int xoffset,
                                   int yoffset,
                                   uint8_t *dst_ptr,
                                   int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_bilinear_filters[xoffset];
  VFilter = vp9_bilinear_filters[yoffset];

  filter_block2d_bil_avg(src_ptr, dst_ptr, src_pixels_per_line,
                         dst_pitch, HFilter, VFilter, 4, 4);
}

void vp9_bilinear_predict8x8_c(uint8_t *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               uint8_t *dst_ptr,
                               int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_bilinear_filters[xoffset];
  VFilter = vp9_bilinear_filters[yoffset];

  filter_block2d_bil(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 8, 8);

}

void vp9_bilinear_predict_avg8x8_c(uint8_t *src_ptr,
                                   int src_pixels_per_line,
                                   int xoffset,
                                   int yoffset,
                                   uint8_t *dst_ptr,
                                   int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_bilinear_filters[xoffset];
  VFilter = vp9_bilinear_filters[yoffset];

  filter_block2d_bil_avg(src_ptr, dst_ptr, src_pixels_per_line,
                         dst_pitch, HFilter, VFilter, 8, 8);
}

void vp9_bilinear_predict8x4_c(uint8_t *src_ptr,
                               int src_pixels_per_line,
                               int xoffset,
                               int yoffset,
                               uint8_t *dst_ptr,
                               int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_bilinear_filters[xoffset];
  VFilter = vp9_bilinear_filters[yoffset];

  filter_block2d_bil(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 8, 4);

}

void vp9_bilinear_predict16x16_c(uint8_t *src_ptr,
                                 int src_pixels_per_line,
                                 int xoffset,
                                 int yoffset,
                                 uint8_t *dst_ptr,
                                 int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_bilinear_filters[xoffset];
  VFilter = vp9_bilinear_filters[yoffset];

  filter_block2d_bil(src_ptr, dst_ptr, src_pixels_per_line, dst_pitch, HFilter, VFilter, 16, 16);
}

void vp9_bilinear_predict_avg16x16_c(uint8_t *src_ptr,
                                     int src_pixels_per_line,
                                     int xoffset,
                                     int yoffset,
                                     uint8_t *dst_ptr,
                                     int dst_pitch) {
  const int16_t *HFilter;
  const int16_t *VFilter;

  HFilter = vp9_bilinear_filters[xoffset];
  VFilter = vp9_bilinear_filters[yoffset];

  filter_block2d_bil_avg(src_ptr, dst_ptr, src_pixels_per_line,
                         dst_pitch, HFilter, VFilter, 16, 16);
}
