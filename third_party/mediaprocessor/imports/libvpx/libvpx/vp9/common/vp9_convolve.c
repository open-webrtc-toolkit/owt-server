/*
 *  Copyright (c) 2013 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "vp9/common/vp9_convolve.h"

#include <assert.h>

#include "./vpx_config.h"
#include "./vp9_rtcd.h"
#include "vp9/common/vp9_common.h"
#include "vpx/vpx_integer.h"
#include "vpx_ports/mem.h"

#define VP9_FILTER_WEIGHT 128
#define VP9_FILTER_SHIFT  7

/* Assume a bank of 16 filters to choose from. There are two implementations
 * for filter wrapping behavior, since we want to be able to pick which filter
 * to start with. We could either:
 *
 * 1) make filter_ a pointer to the base of the filter array, and then add an
 *    additional offset parameter, to choose the starting filter.
 * 2) use a pointer to 2 periods worth of filters, so that even if the original
 *    phase offset is at 15/16, we'll have valid data to read. The filter
 *    tables become [32][8], and the second half is duplicated.
 * 3) fix the alignment of the filter tables, so that we know the 0/16 is
 *    always 256 byte aligned.
 *
 * Implementations 2 and 3 are likely preferable, as they avoid an extra 2
 * parameters, and switching between them is trivial, with the
 * ALIGN_FILTERS_256 macro, below.
 */
 #define ALIGN_FILTERS_256 1

static void convolve_horiz_c(const uint8_t *src, int src_stride,
                             uint8_t *dst, int dst_stride,
                             const int16_t *filter_x0, int x_step_q4,
                             const int16_t *filter_y, int y_step_q4,
                             int w, int h, int taps) {
  int x, y, k, sum;
  const int16_t *filter_x_base = filter_x0;

#if ALIGN_FILTERS_256
  filter_x_base = (const int16_t *)(((intptr_t)filter_x0) & ~(intptr_t)0xff);
#endif

  /* Adjust base pointer address for this source line */
  src -= taps / 2 - 1;

  for (y = 0; y < h; ++y) {
    /* Pointer to filter to use */
    const int16_t *filter_x = filter_x0;

    /* Initial phase offset */
    int x0_q4 = (filter_x - filter_x_base) / taps;
    int x_q4 = x0_q4;

    for (x = 0; x < w; ++x) {
      /* Per-pixel src offset */
      int src_x = (x_q4 - x0_q4) >> 4;

      for (sum = 0, k = 0; k < taps; ++k) {
        sum += src[src_x + k] * filter_x[k];
      }
      sum += (VP9_FILTER_WEIGHT >> 1);
      dst[x] = clip_pixel(sum >> VP9_FILTER_SHIFT);

      /* Adjust source and filter to use for the next pixel */
      x_q4 += x_step_q4;
      filter_x = filter_x_base + (x_q4 & 0xf) * taps;
    }
    src += src_stride;
    dst += dst_stride;
  }
}

static void convolve_avg_horiz_c(const uint8_t *src, int src_stride,
                                 uint8_t *dst, int dst_stride,
                                 const int16_t *filter_x0, int x_step_q4,
                                 const int16_t *filter_y, int y_step_q4,
                                 int w, int h, int taps) {
  int x, y, k, sum;
  const int16_t *filter_x_base = filter_x0;

#if ALIGN_FILTERS_256
  filter_x_base = (const int16_t *)(((intptr_t)filter_x0) & ~(intptr_t)0xff);
#endif

  /* Adjust base pointer address for this source line */
  src -= taps / 2 - 1;

  for (y = 0; y < h; ++y) {
    /* Pointer to filter to use */
    const int16_t *filter_x = filter_x0;

    /* Initial phase offset */
    int x0_q4 = (filter_x - filter_x_base) / taps;
    int x_q4 = x0_q4;

    for (x = 0; x < w; ++x) {
      /* Per-pixel src offset */
      int src_x = (x_q4 - x0_q4) >> 4;

      for (sum = 0, k = 0; k < taps; ++k) {
        sum += src[src_x + k] * filter_x[k];
      }
      sum += (VP9_FILTER_WEIGHT >> 1);
      dst[x] = (dst[x] + clip_pixel(sum >> VP9_FILTER_SHIFT) + 1) >> 1;

      /* Adjust source and filter to use for the next pixel */
      x_q4 += x_step_q4;
      filter_x = filter_x_base + (x_q4 & 0xf) * taps;
    }
    src += src_stride;
    dst += dst_stride;
  }
}

#if CONFIG_IMPLICIT_COMPOUNDINTER_WEIGHT

static inline uint8_t combine_qtr(uint8_t a, uint8_t b) {
  return (((a) + (b) * 3 + 2) >> 2);
}

static inline uint8_t combine_3qtr(uint8_t a, uint8_t b) {
  return (((a) * 3 + (b) + 2) >> 2);
}

static inline uint8_t combine_1by8(uint8_t a, uint8_t b) {
  return (((a) * 1 + (b) * 7 + 4) >> 3);
}

static inline uint8_t combine_3by8(uint8_t a, uint8_t b) {
  return (((a) * 3 + (b) * 5 + 4) >> 3);
}

static inline uint8_t combine_5by8(uint8_t a, uint8_t b) {
  return (((a) * 5 + (b) * 3 + 4) >> 3);
}

static inline uint8_t combine_7by8(uint8_t a, uint8_t b) {
  return (((a) * 7 + (b) * 1 + 4) >> 3);
}

// TODO(debargha): Implment with a separate weight parameter
static void convolve_wtd_horiz_c(const uint8_t *src, int src_stride,
                                 uint8_t *dst, int dst_stride,
                                 const int16_t *filter_x0, int x_step_q4,
                                 const int16_t *filter_y, int y_step_q4,
                                 int w, int h, int taps,
                                 uint8_t (*combine)(uint8_t a, uint8_t b)) {
  int x, y, k, sum;
  const int16_t *filter_x_base = filter_x0;

#if ALIGN_FILTERS_256
  filter_x_base = (const int16_t *)(((intptr_t)filter_x0) & ~(intptr_t)0xff);
#endif

  /* Adjust base pointer address for this source line */
  src -= taps / 2 - 1;

  for (y = 0; y < h; ++y) {
    /* Pointer to filter to use */
    const int16_t *filter_x = filter_x0;

    /* Initial phase offset */
    int x0_q4 = (filter_x - filter_x_base) / taps;
    int x_q4 = x0_q4;

    for (x = 0; x < w; ++x) {
      /* Per-pixel src offset */
      int src_x = (x_q4 - x0_q4) >> 4;

      for (sum = 0, k = 0; k < taps; ++k) {
        sum += src[src_x + k] * filter_x[k];
      }
      sum += (VP9_FILTER_WEIGHT >> 1);
      dst[x] = combine(dst[x], clip_pixel(sum >> VP9_FILTER_SHIFT));

      /* Adjust source and filter to use for the next pixel */
      x_q4 += x_step_q4;
      filter_x = filter_x_base + (x_q4 & 0xf) * taps;
    }
    src += src_stride;
    dst += dst_stride;
  }
}

#endif

static void convolve_vert_c(const uint8_t *src, int src_stride,
                            uint8_t *dst, int dst_stride,
                            const int16_t *filter_x, int x_step_q4,
                            const int16_t *filter_y0, int y_step_q4,
                            int w, int h, int taps) {
  int x, y, k, sum;

  const int16_t *filter_y_base = filter_y0;

#if ALIGN_FILTERS_256
  filter_y_base = (const int16_t *)(((intptr_t)filter_y0) & ~(intptr_t)0xff);
#endif

  /* Adjust base pointer address for this source column */
  src -= src_stride * (taps / 2 - 1);
  for (x = 0; x < w; ++x) {
    /* Pointer to filter to use */
    const int16_t *filter_y = filter_y0;

    /* Initial phase offset */
    int y0_q4 = (filter_y - filter_y_base) / taps;
    int y_q4 = y0_q4;

    for (y = 0; y < h; ++y) {
      /* Per-pixel src offset */
      int src_y = (y_q4 - y0_q4) >> 4;

      for (sum = 0, k = 0; k < taps; ++k) {
        sum += src[(src_y + k) * src_stride] * filter_y[k];
      }
      sum += (VP9_FILTER_WEIGHT >> 1);
      dst[y * dst_stride] = clip_pixel(sum >> VP9_FILTER_SHIFT);

      /* Adjust source and filter to use for the next pixel */
      y_q4 += y_step_q4;
      filter_y = filter_y_base + (y_q4 & 0xf) * taps;
    }
    ++src;
    ++dst;
  }
}

static void convolve_avg_vert_c(const uint8_t *src, int src_stride,
                                uint8_t *dst, int dst_stride,
                                const int16_t *filter_x, int x_step_q4,
                                const int16_t *filter_y0, int y_step_q4,
                                int w, int h, int taps) {
  int x, y, k, sum;

  const int16_t *filter_y_base = filter_y0;

#if ALIGN_FILTERS_256
  filter_y_base = (const int16_t *)(((intptr_t)filter_y0) & ~(intptr_t)0xff);
#endif

  /* Adjust base pointer address for this source column */
  src -= src_stride * (taps / 2 - 1);
  for (x = 0; x < w; ++x) {
    /* Pointer to filter to use */
    const int16_t *filter_y = filter_y0;

    /* Initial phase offset */
    int y0_q4 = (filter_y - filter_y_base) / taps;
    int y_q4 = y0_q4;

    for (y = 0; y < h; ++y) {
      /* Per-pixel src offset */
      int src_y = (y_q4 - y0_q4) >> 4;

      for (sum = 0, k = 0; k < taps; ++k) {
        sum += src[(src_y + k) * src_stride] * filter_y[k];
      }
      sum += (VP9_FILTER_WEIGHT >> 1);
      dst[y * dst_stride] =
          (dst[y * dst_stride] + clip_pixel(sum >> VP9_FILTER_SHIFT) + 1) >> 1;

      /* Adjust source and filter to use for the next pixel */
      y_q4 += y_step_q4;
      filter_y = filter_y_base + (y_q4 & 0xf) * taps;
    }
    ++src;
    ++dst;
  }
}

#if CONFIG_IMPLICIT_COMPOUNDINTER_WEIGHT
static void convolve_wtd_vert_c(const uint8_t *src, int src_stride,
                                uint8_t *dst, int dst_stride,
                                const int16_t *filter_x, int x_step_q4,
                                const int16_t *filter_y0, int y_step_q4,
                                int w, int h, int taps,
                                uint8_t (*combine)(uint8_t a, uint8_t b)) {
  int x, y, k, sum;

  const int16_t *filter_y_base = filter_y0;

#if ALIGN_FILTERS_256
  filter_y_base = (const int16_t *)(((intptr_t)filter_y0) & ~(intptr_t)0xff);
#endif

  /* Adjust base pointer address for this source column */
  src -= src_stride * (taps / 2 - 1);
  for (x = 0; x < w; ++x) {
    /* Pointer to filter to use */
    const int16_t *filter_y = filter_y0;

    /* Initial phase offset */
    int y0_q4 = (filter_y - filter_y_base) / taps;
    int y_q4 = y0_q4;

    for (y = 0; y < h; ++y) {
      /* Per-pixel src offset */
      int src_y = (y_q4 - y0_q4) >> 4;

      for (sum = 0, k = 0; k < taps; ++k) {
        sum += src[(src_y + k) * src_stride] * filter_y[k];
      }
      sum += (VP9_FILTER_WEIGHT >> 1);
      dst[y * dst_stride] = combine(dst[y * dst_stride],
                                    clip_pixel(sum >> VP9_FILTER_SHIFT));

      /* Adjust source and filter to use for the next pixel */
      y_q4 += y_step_q4;
      filter_y = filter_y_base + (y_q4 & 0xf) * taps;
    }
    ++src;
    ++dst;
  }
}
#endif

static void convolve_c(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int x_step_q4,
                       const int16_t *filter_y, int y_step_q4,
                       int w, int h, int taps) {
  /* Fixed size intermediate buffer places limits on parameters.
   * Maximum intermediate_height is 39, for y_step_q4 == 32,
   * h == 16, taps == 8.
   */
  uint8_t temp[16 * 39];
  int intermediate_height = ((h * y_step_q4) >> 4) + taps - 1;

  assert(w <= 16);
  assert(h <= 16);
  assert(taps <= 8);
  assert(y_step_q4 <= 32);

  if (intermediate_height < h)
    intermediate_height = h;

  convolve_horiz_c(src - src_stride * (taps / 2 - 1), src_stride,
                   temp, 16,
                   filter_x, x_step_q4, filter_y, y_step_q4,
                   w, intermediate_height, taps);
  convolve_vert_c(temp + 16 * (taps / 2 - 1), 16, dst, dst_stride,
                  filter_x, x_step_q4, filter_y, y_step_q4,
                  w, h, taps);
}

static void convolve_avg_c(const uint8_t *src, int src_stride,
                           uint8_t *dst, int dst_stride,
                           const int16_t *filter_x, int x_step_q4,
                           const int16_t *filter_y, int y_step_q4,
                           int w, int h, int taps) {
  /* Fixed size intermediate buffer places limits on parameters.
   * Maximum intermediate_height is 39, for y_step_q4 == 32,
   * h == 16, taps == 8.
   */
  uint8_t temp[16 * 39];
  int intermediate_height = ((h * y_step_q4) >> 4) + taps - 1;

  assert(w <= 16);
  assert(h <= 16);
  assert(taps <= 8);
  assert(y_step_q4 <= 32);

  if (intermediate_height < h)
    intermediate_height = h;

  convolve_horiz_c(src - src_stride * (taps / 2 - 1), src_stride,
                   temp, 16,
                   filter_x, x_step_q4, filter_y, y_step_q4,
                   w, intermediate_height, taps);
  convolve_avg_vert_c(temp + 16 * (taps / 2 - 1), 16, dst, dst_stride,
                      filter_x, x_step_q4, filter_y, y_step_q4,
                      w, h, taps);
}

void vp9_convolve8_horiz_c(const uint8_t *src, int src_stride,
                           uint8_t *dst, int dst_stride,
                           const int16_t *filter_x, int x_step_q4,
                           const int16_t *filter_y, int y_step_q4,
                           int w, int h) {
  convolve_horiz_c(src, src_stride, dst, dst_stride,
                   filter_x, x_step_q4, filter_y, y_step_q4,
                   w, h, 8);
}

void vp9_convolve8_avg_horiz_c(const uint8_t *src, int src_stride,
                               uint8_t *dst, int dst_stride,
                               const int16_t *filter_x, int x_step_q4,
                               const int16_t *filter_y, int y_step_q4,
                               int w, int h) {
  convolve_avg_horiz_c(src, src_stride, dst, dst_stride,
                       filter_x, x_step_q4, filter_y, y_step_q4,
                       w, h, 8);
}

#if CONFIG_IMPLICIT_COMPOUNDINTER_WEIGHT
void vp9_convolve8_1by8_horiz_c(const uint8_t *src, int src_stride,
                                uint8_t *dst, int dst_stride,
                                const int16_t *filter_x, int x_step_q4,
                                const int16_t *filter_y, int y_step_q4,
                                int w, int h) {
  convolve_wtd_horiz_c(src, src_stride, dst, dst_stride,
                       filter_x, x_step_q4, filter_y, y_step_q4,
                       w, h, 8, combine_1by8);
}

void vp9_convolve8_qtr_horiz_c(const uint8_t *src, int src_stride,
                               uint8_t *dst, int dst_stride,
                               const int16_t *filter_x, int x_step_q4,
                               const int16_t *filter_y, int y_step_q4,
                               int w, int h) {
  convolve_wtd_horiz_c(src, src_stride, dst, dst_stride,
                       filter_x, x_step_q4, filter_y, y_step_q4,
                       w, h, 8, combine_qtr);
}

void vp9_convolve8_3by8_horiz_c(const uint8_t *src, int src_stride,
                                uint8_t *dst, int dst_stride,
                                const int16_t *filter_x, int x_step_q4,
                                const int16_t *filter_y, int y_step_q4,
                                int w, int h) {
  convolve_wtd_horiz_c(src, src_stride, dst, dst_stride,
                       filter_x, x_step_q4, filter_y, y_step_q4,
                       w, h, 8, combine_3by8);
}

void vp9_convolve8_5by8_horiz_c(const uint8_t *src, int src_stride,
                                uint8_t *dst, int dst_stride,
                                const int16_t *filter_x, int x_step_q4,
                                const int16_t *filter_y, int y_step_q4,
                                int w, int h) {
  convolve_wtd_horiz_c(src, src_stride, dst, dst_stride,
                       filter_x, x_step_q4, filter_y, y_step_q4,
                       w, h, 8, combine_5by8);
}

void vp9_convolve8_3qtr_horiz_c(const uint8_t *src, int src_stride,
                                uint8_t *dst, int dst_stride,
                                const int16_t *filter_x, int x_step_q4,
                                const int16_t *filter_y, int y_step_q4,
                                int w, int h) {
  convolve_wtd_horiz_c(src, src_stride, dst, dst_stride,
                       filter_x, x_step_q4, filter_y, y_step_q4,
                       w, h, 8, combine_3qtr);
}

void vp9_convolve8_7by8_horiz_c(const uint8_t *src, int src_stride,
                                uint8_t *dst, int dst_stride,
                                const int16_t *filter_x, int x_step_q4,
                                const int16_t *filter_y, int y_step_q4,
                                int w, int h) {
  convolve_wtd_horiz_c(src, src_stride, dst, dst_stride,
                       filter_x, x_step_q4, filter_y, y_step_q4,
                       w, h, 8, combine_7by8);
}
#endif

void vp9_convolve8_vert_c(const uint8_t *src, int src_stride,
                          uint8_t *dst, int dst_stride,
                          const int16_t *filter_x, int x_step_q4,
                          const int16_t *filter_y, int y_step_q4,
                          int w, int h) {
  convolve_vert_c(src, src_stride, dst, dst_stride,
                  filter_x, x_step_q4, filter_y, y_step_q4,
                  w, h, 8);
}

void vp9_convolve8_avg_vert_c(const uint8_t *src, int src_stride,
                              uint8_t *dst, int dst_stride,
                              const int16_t *filter_x, int x_step_q4,
                              const int16_t *filter_y, int y_step_q4,
                              int w, int h) {
  convolve_avg_vert_c(src, src_stride, dst, dst_stride,
                      filter_x, x_step_q4, filter_y, y_step_q4,
                      w, h, 8);
}

#if CONFIG_IMPLICIT_COMPOUNDINTER_WEIGHT
void vp9_convolve8_1by8_vert_c(const uint8_t *src, int src_stride,
                               uint8_t *dst, int dst_stride,
                               const int16_t *filter_x, int x_step_q4,
                               const int16_t *filter_y, int y_step_q4,
                               int w, int h) {
  convolve_wtd_vert_c(src, src_stride, dst, dst_stride,
                      filter_x, x_step_q4, filter_y, y_step_q4,
                      w, h, 8, combine_1by8);
}

void vp9_convolve8_qtr_vert_c(const uint8_t *src, int src_stride,
                              uint8_t *dst, int dst_stride,
                              const int16_t *filter_x, int x_step_q4,
                              const int16_t *filter_y, int y_step_q4,
                              int w, int h) {
  convolve_wtd_vert_c(src, src_stride, dst, dst_stride,
                      filter_x, x_step_q4, filter_y, y_step_q4,
                      w, h, 8, combine_qtr);
}

void vp9_convolve8_3by8_vert_c(const uint8_t *src, int src_stride,
                               uint8_t *dst, int dst_stride,
                               const int16_t *filter_x, int x_step_q4,
                               const int16_t *filter_y, int y_step_q4,
                               int w, int h) {
  convolve_wtd_vert_c(src, src_stride, dst, dst_stride,
                      filter_x, x_step_q4, filter_y, y_step_q4,
                      w, h, 8, combine_3by8);
}

void vp9_convolve8_5by8_vert_c(const uint8_t *src, int src_stride,
                               uint8_t *dst, int dst_stride,
                               const int16_t *filter_x, int x_step_q4,
                               const int16_t *filter_y, int y_step_q4,
                               int w, int h) {
  convolve_wtd_vert_c(src, src_stride, dst, dst_stride,
                      filter_x, x_step_q4, filter_y, y_step_q4,
                      w, h, 8, combine_5by8);
}

void vp9_convolve8_3qtr_vert_c(const uint8_t *src, int src_stride,
                               uint8_t *dst, int dst_stride,
                               const int16_t *filter_x, int x_step_q4,
                               const int16_t *filter_y, int y_step_q4,
                               int w, int h) {
  convolve_wtd_vert_c(src, src_stride, dst, dst_stride,
                      filter_x, x_step_q4, filter_y, y_step_q4,
                      w, h, 8, combine_3qtr);
}

void vp9_convolve8_7by8_vert_c(const uint8_t *src, int src_stride,
                               uint8_t *dst, int dst_stride,
                               const int16_t *filter_x, int x_step_q4,
                               const int16_t *filter_y, int y_step_q4,
                               int w, int h) {
  convolve_wtd_vert_c(src, src_stride, dst, dst_stride,
                      filter_x, x_step_q4, filter_y, y_step_q4,
                      w, h, 8, combine_7by8);
}
#endif

void vp9_convolve8_c(const uint8_t *src, int src_stride,
                     uint8_t *dst, int dst_stride,
                     const int16_t *filter_x, int x_step_q4,
                     const int16_t *filter_y, int y_step_q4,
                     int w, int h) {
  convolve_c(src, src_stride, dst, dst_stride,
             filter_x, x_step_q4, filter_y, y_step_q4,
             w, h, 8);
}

void vp9_convolve8_avg_c(const uint8_t *src, int src_stride,
                         uint8_t *dst, int dst_stride,
                         const int16_t *filter_x, int x_step_q4,
                         const int16_t *filter_y, int y_step_q4,
                         int w, int h) {
  /* Fixed size intermediate buffer places limits on parameters. */
  DECLARE_ALIGNED_ARRAY(16, uint8_t, temp, 16 * 16);
  assert(w <= 16);
  assert(h <= 16);

  vp9_convolve8(src, src_stride,
                temp, 16,
                filter_x, x_step_q4,
                filter_y, y_step_q4,
                w, h);
  vp9_convolve_avg(temp, 16,
                   dst, dst_stride,
                   NULL, 0, /* These unused parameter should be removed! */
                   NULL, 0, /* These unused parameter should be removed! */
                   w, h);
}

#if CONFIG_IMPLICIT_COMPOUNDINTER_WEIGHT
void vp9_convolve8_1by8_c(const uint8_t *src, int src_stride,
                         uint8_t *dst, int dst_stride,
                         const int16_t *filter_x, int x_step_q4,
                         const int16_t *filter_y, int y_step_q4,
                         int w, int h) {
  /* Fixed size intermediate buffer places limits on parameters. */
  DECLARE_ALIGNED_ARRAY(16, uint8_t, temp, 16 * 16);
  assert(w <= 16);
  assert(h <= 16);

  vp9_convolve8(src, src_stride,
                temp, 16,
                filter_x, x_step_q4,
                filter_y, y_step_q4,
                w, h);
  vp9_convolve_1by8(temp, 16,
                    dst, dst_stride,
                    NULL, 0, /* These unused parameter should be removed! */
                    NULL, 0, /* These unused parameter should be removed! */
                    w, h);
}

void vp9_convolve8_qtr_c(const uint8_t *src, int src_stride,
                         uint8_t *dst, int dst_stride,
                         const int16_t *filter_x, int x_step_q4,
                         const int16_t *filter_y, int y_step_q4,
                         int w, int h) {
  /* Fixed size intermediate buffer places limits on parameters. */
  DECLARE_ALIGNED_ARRAY(16, uint8_t, temp, 16 * 16);
  assert(w <= 16);
  assert(h <= 16);

  vp9_convolve8(src, src_stride,
                temp, 16,
                filter_x, x_step_q4,
                filter_y, y_step_q4,
                w, h);
  vp9_convolve_qtr(temp, 16,
                   dst, dst_stride,
                   NULL, 0, /* These unused parameter should be removed! */
                   NULL, 0, /* These unused parameter should be removed! */
                   w, h);
}

void vp9_convolve8_3by8_c(const uint8_t *src, int src_stride,
                         uint8_t *dst, int dst_stride,
                         const int16_t *filter_x, int x_step_q4,
                         const int16_t *filter_y, int y_step_q4,
                         int w, int h) {
  /* Fixed size intermediate buffer places limits on parameters. */
  DECLARE_ALIGNED_ARRAY(16, uint8_t, temp, 16 * 16);
  assert(w <= 16);
  assert(h <= 16);

  vp9_convolve8(src, src_stride,
                temp, 16,
                filter_x, x_step_q4,
                filter_y, y_step_q4,
                w, h);
  vp9_convolve_3by8(temp, 16,
                    dst, dst_stride,
                    NULL, 0, /* These unused parameter should be removed! */
                    NULL, 0, /* These unused parameter should be removed! */
                    w, h);
}

void vp9_convolve8_5by8_c(const uint8_t *src, int src_stride,
                         uint8_t *dst, int dst_stride,
                         const int16_t *filter_x, int x_step_q4,
                         const int16_t *filter_y, int y_step_q4,
                         int w, int h) {
  /* Fixed size intermediate buffer places limits on parameters. */
  DECLARE_ALIGNED_ARRAY(16, uint8_t, temp, 16 * 16);
  assert(w <= 16);
  assert(h <= 16);

  vp9_convolve8(src, src_stride,
                temp, 16,
                filter_x, x_step_q4,
                filter_y, y_step_q4,
                w, h);
  vp9_convolve_5by8(temp, 16,
                    dst, dst_stride,
                    NULL, 0, /* These unused parameter should be removed! */
                    NULL, 0, /* These unused parameter should be removed! */
                    w, h);
}

void vp9_convolve8_3qtr_c(const uint8_t *src, int src_stride,
                          uint8_t *dst, int dst_stride,
                          const int16_t *filter_x, int x_step_q4,
                          const int16_t *filter_y, int y_step_q4,
                          int w, int h) {
  /* Fixed size intermediate buffer places limits on parameters. */
  DECLARE_ALIGNED_ARRAY(16, uint8_t, temp, 16 * 16);
  assert(w <= 16);
  assert(h <= 16);

  vp9_convolve8(src, src_stride,
                temp, 16,
                filter_x, x_step_q4,
                filter_y, y_step_q4,
                w, h);
  vp9_convolve_3qtr(temp, 16,
                    dst, dst_stride,
                    NULL, 0, /* These unused parameter should be removed! */
                    NULL, 0, /* These unused parameter should be removed! */
                    w, h);
}

void vp9_convolve8_7by8_c(const uint8_t *src, int src_stride,
                         uint8_t *dst, int dst_stride,
                         const int16_t *filter_x, int x_step_q4,
                         const int16_t *filter_y, int y_step_q4,
                         int w, int h) {
  /* Fixed size intermediate buffer places limits on parameters. */
  DECLARE_ALIGNED_ARRAY(16, uint8_t, temp, 16 * 16);
  assert(w <= 16);
  assert(h <= 16);

  vp9_convolve8(src, src_stride,
                temp, 16,
                filter_x, x_step_q4,
                filter_y, y_step_q4,
                w, h);
  vp9_convolve_7by8(temp, 16,
                    dst, dst_stride,
                    NULL, 0, /* These unused parameter should be removed! */
                    NULL, 0, /* These unused parameter should be removed! */
                    w, h);
}
#endif

void vp9_convolve_copy(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int filter_x_stride,
                       const int16_t *filter_y, int filter_y_stride,
                       int w, int h) {
  if (w == 16 && h == 16) {
    vp9_copy_mem16x16(src, src_stride, dst, dst_stride);
  } else if (w == 8 && h == 8) {
    vp9_copy_mem8x8(src, src_stride, dst, dst_stride);
  } else if (w == 8 && h == 4) {
    vp9_copy_mem8x4(src, src_stride, dst, dst_stride);
  } else {
    int r;

    for (r = h; r > 0; --r) {
      memcpy(dst, src, w);
      src += src_stride;
      dst += dst_stride;
    }
  }
}

void vp9_convolve_avg(const uint8_t *src, int src_stride,
                      uint8_t *dst, int dst_stride,
                      const int16_t *filter_x, int filter_x_stride,
                      const int16_t *filter_y, int filter_y_stride,
                      int w, int h) {
  int x, y;

  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) {
      dst[x] = (dst[x] + src[x] + 1) >> 1;
    }
    src += src_stride;
    dst += dst_stride;
  }
}

#if CONFIG_IMPLICIT_COMPOUNDINTER_WEIGHT
void vp9_convolve_1by8(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int filter_x_stride,
                       const int16_t *filter_y, int filter_y_stride,
                       int w, int h) {
  int x, y;

  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) {
      dst[x] = combine_1by8(dst[x], src[x]);
    }
    src += src_stride;
    dst += dst_stride;
  }
}

void vp9_convolve_qtr(const uint8_t *src, int src_stride,
                      uint8_t *dst, int dst_stride,
                      const int16_t *filter_x, int filter_x_stride,
                      const int16_t *filter_y, int filter_y_stride,
                      int w, int h) {
  int x, y;

  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) {
      dst[x] = combine_qtr(dst[x], src[x]);
    }
    src += src_stride;
    dst += dst_stride;
  }
}

void vp9_convolve_3by8(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int filter_x_stride,
                       const int16_t *filter_y, int filter_y_stride,
                       int w, int h) {
  int x, y;

  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) {
      dst[x] = combine_3by8(dst[x], src[x]);
    }
    src += src_stride;
    dst += dst_stride;
  }
}

void vp9_convolve_5by8(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int filter_x_stride,
                       const int16_t *filter_y, int filter_y_stride,
                       int w, int h) {
  int x, y;

  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) {
      dst[x] = combine_5by8(dst[x], src[x]);
    }
    src += src_stride;
    dst += dst_stride;
  }
}

void vp9_convolve_3qtr(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int filter_x_stride,
                       const int16_t *filter_y, int filter_y_stride,
                       int w, int h) {
  int x, y;

  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) {
      dst[x] = combine_3qtr(dst[x], src[x]);
    }
    src += src_stride;
    dst += dst_stride;
  }
}

void vp9_convolve_7by8(const uint8_t *src, int src_stride,
                       uint8_t *dst, int dst_stride,
                       const int16_t *filter_x, int filter_x_stride,
                       const int16_t *filter_y, int filter_y_stride,
                       int w, int h) {
  int x, y;

  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) {
      dst[x] = combine_7by8(dst[x], src[x]);
    }
    src += src_stride;
    dst += dst_stride;
  }
}
#endif
