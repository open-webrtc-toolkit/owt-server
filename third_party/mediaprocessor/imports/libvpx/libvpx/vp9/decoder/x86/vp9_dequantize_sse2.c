/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>
#include <emmintrin.h>  // SSE2
#include "./vpx_config.h"
#include "vpx/vpx_integer.h"
#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_idct.h"

void vp9_add_residual_4x4_sse2(const int16_t *diff, const uint8_t *pred,
                               int pitch, uint8_t *dest, int stride) {
  const int width = 4;
  const __m128i zero = _mm_setzero_si128();

  // Diff data
  const __m128i d0 = _mm_loadl_epi64((const __m128i *)(diff + 0 * width));
  const __m128i d1 = _mm_loadl_epi64((const __m128i *)(diff + 1 * width));
  const __m128i d2 = _mm_loadl_epi64((const __m128i *)(diff + 2 * width));
  const __m128i d3 = _mm_loadl_epi64((const __m128i *)(diff + 3 * width));

  // Prediction data.
  __m128i p0 = _mm_cvtsi32_si128(*(const int *)(pred + 0 * pitch));
  __m128i p1 = _mm_cvtsi32_si128(*(const int *)(pred + 1 * pitch));
  __m128i p2 = _mm_cvtsi32_si128(*(const int *)(pred + 2 * pitch));
  __m128i p3 = _mm_cvtsi32_si128(*(const int *)(pred + 3 * pitch));

  p0 = _mm_unpacklo_epi8(p0, zero);
  p1 = _mm_unpacklo_epi8(p1, zero);
  p2 = _mm_unpacklo_epi8(p2, zero);
  p3 = _mm_unpacklo_epi8(p3, zero);

  p0 = _mm_add_epi16(p0, d0);
  p1 = _mm_add_epi16(p1, d1);
  p2 = _mm_add_epi16(p2, d2);
  p3 = _mm_add_epi16(p3, d3);

  p0 = _mm_packus_epi16(p0, p1);
  p2 = _mm_packus_epi16(p2, p3);

  *(int *)dest = _mm_cvtsi128_si32(p0);
  dest += stride;

  p0 = _mm_srli_si128(p0, 8);
  *(int *)dest = _mm_cvtsi128_si32(p0);
  dest += stride;

  *(int *)dest = _mm_cvtsi128_si32(p2);
  dest += stride;

  p2 = _mm_srli_si128(p2, 8);
  *(int *)dest = _mm_cvtsi128_si32(p2);
}

void vp9_add_residual_8x8_sse2(const int16_t *diff, const uint8_t *pred,
                               int pitch, uint8_t *dest, int stride) {
  const int width = 8;
  const __m128i zero = _mm_setzero_si128();

  // Diff data
  const __m128i d0 = _mm_load_si128((const __m128i *)(diff + 0 * width));
  const __m128i d1 = _mm_load_si128((const __m128i *)(diff + 1 * width));
  const __m128i d2 = _mm_load_si128((const __m128i *)(diff + 2 * width));
  const __m128i d3 = _mm_load_si128((const __m128i *)(diff + 3 * width));
  const __m128i d4 = _mm_load_si128((const __m128i *)(diff + 4 * width));
  const __m128i d5 = _mm_load_si128((const __m128i *)(diff + 5 * width));
  const __m128i d6 = _mm_load_si128((const __m128i *)(diff + 6 * width));
  const __m128i d7 = _mm_load_si128((const __m128i *)(diff + 7 * width));

  // Prediction data.
  __m128i p0 = _mm_loadl_epi64((const __m128i *)(pred + 0 * pitch));
  __m128i p1 = _mm_loadl_epi64((const __m128i *)(pred + 1 * pitch));
  __m128i p2 = _mm_loadl_epi64((const __m128i *)(pred + 2 * pitch));
  __m128i p3 = _mm_loadl_epi64((const __m128i *)(pred + 3 * pitch));
  __m128i p4 = _mm_loadl_epi64((const __m128i *)(pred + 4 * pitch));
  __m128i p5 = _mm_loadl_epi64((const __m128i *)(pred + 5 * pitch));
  __m128i p6 = _mm_loadl_epi64((const __m128i *)(pred + 6 * pitch));
  __m128i p7 = _mm_loadl_epi64((const __m128i *)(pred + 7 * pitch));

  p0 = _mm_unpacklo_epi8(p0, zero);
  p1 = _mm_unpacklo_epi8(p1, zero);
  p2 = _mm_unpacklo_epi8(p2, zero);
  p3 = _mm_unpacklo_epi8(p3, zero);
  p4 = _mm_unpacklo_epi8(p4, zero);
  p5 = _mm_unpacklo_epi8(p5, zero);
  p6 = _mm_unpacklo_epi8(p6, zero);
  p7 = _mm_unpacklo_epi8(p7, zero);

  p0 = _mm_add_epi16(p0, d0);
  p1 = _mm_add_epi16(p1, d1);
  p2 = _mm_add_epi16(p2, d2);
  p3 = _mm_add_epi16(p3, d3);
  p4 = _mm_add_epi16(p4, d4);
  p5 = _mm_add_epi16(p5, d5);
  p6 = _mm_add_epi16(p6, d6);
  p7 = _mm_add_epi16(p7, d7);

  p0 = _mm_packus_epi16(p0, p1);
  p2 = _mm_packus_epi16(p2, p3);
  p4 = _mm_packus_epi16(p4, p5);
  p6 = _mm_packus_epi16(p6, p7);

  _mm_storel_epi64((__m128i *)(dest + 0 * stride), p0);
  p0 = _mm_srli_si128(p0, 8);
  _mm_storel_epi64((__m128i *)(dest + 1 * stride), p0);

  _mm_storel_epi64((__m128i *)(dest + 2 * stride), p2);
  p2 = _mm_srli_si128(p2, 8);
  _mm_storel_epi64((__m128i *)(dest + 3 * stride), p2);

  _mm_storel_epi64((__m128i *)(dest + 4 * stride), p4);
  p4 = _mm_srli_si128(p4, 8);
  _mm_storel_epi64((__m128i *)(dest + 5 * stride), p4);

  _mm_storel_epi64((__m128i *)(dest + 6 * stride), p6);
  p6 = _mm_srli_si128(p6, 8);
  _mm_storel_epi64((__m128i *)(dest + 7 * stride), p6);
}

void vp9_add_residual_16x16_sse2(const int16_t *diff, const uint8_t *pred,
                             int pitch, uint8_t *dest, int stride) {
  const int width = 16;
  int i = 4;
  const __m128i zero = _mm_setzero_si128();

  // Diff data
  __m128i d0, d1, d2, d3, d4, d5, d6, d7;
  __m128i p0, p1, p2, p3, p4, p5, p6, p7;

  do {
    d0 = _mm_load_si128((const __m128i *)(diff + 0 * width));
    d1 = _mm_load_si128((const __m128i *)(diff + 0 * width + 8));
    d2 = _mm_load_si128((const __m128i *)(diff + 1 * width));
    d3 = _mm_load_si128((const __m128i *)(diff + 1 * width + 8));
    d4 = _mm_load_si128((const __m128i *)(diff + 2 * width));
    d5 = _mm_load_si128((const __m128i *)(diff + 2 * width + 8));
    d6 = _mm_load_si128((const __m128i *)(diff + 3 * width));
    d7 = _mm_load_si128((const __m128i *)(diff + 3 * width + 8));

    // Prediction data.
    p1 = _mm_load_si128((const __m128i *)(pred + 0 * pitch));
    p3 = _mm_load_si128((const __m128i *)(pred + 1 * pitch));
    p5 = _mm_load_si128((const __m128i *)(pred + 2 * pitch));
    p7 = _mm_load_si128((const __m128i *)(pred + 3 * pitch));

    p0 = _mm_unpacklo_epi8(p1, zero);
    p1 = _mm_unpackhi_epi8(p1, zero);
    p2 = _mm_unpacklo_epi8(p3, zero);
    p3 = _mm_unpackhi_epi8(p3, zero);
    p4 = _mm_unpacklo_epi8(p5, zero);
    p5 = _mm_unpackhi_epi8(p5, zero);
    p6 = _mm_unpacklo_epi8(p7, zero);
    p7 = _mm_unpackhi_epi8(p7, zero);

    p0 = _mm_add_epi16(p0, d0);
    p1 = _mm_add_epi16(p1, d1);
    p2 = _mm_add_epi16(p2, d2);
    p3 = _mm_add_epi16(p3, d3);
    p4 = _mm_add_epi16(p4, d4);
    p5 = _mm_add_epi16(p5, d5);
    p6 = _mm_add_epi16(p6, d6);
    p7 = _mm_add_epi16(p7, d7);

    p0 = _mm_packus_epi16(p0, p1);
    p1 = _mm_packus_epi16(p2, p3);
    p2 = _mm_packus_epi16(p4, p5);
    p3 = _mm_packus_epi16(p6, p7);

    _mm_store_si128((__m128i *)(dest + 0 * stride), p0);
    _mm_store_si128((__m128i *)(dest + 1 * stride), p1);
    _mm_store_si128((__m128i *)(dest + 2 * stride), p2);
    _mm_store_si128((__m128i *)(dest + 3 * stride), p3);

    diff += 4 * width;
    pred += 4 * pitch;
    dest += 4 * stride;
  } while (--i);
}

void vp9_add_residual_32x32_sse2(const int16_t *diff, const uint8_t *pred,
                             int pitch, uint8_t *dest, int stride) {
  const int width = 32;
  int i = 16;
  const __m128i zero = _mm_setzero_si128();

  // Diff data
  __m128i d0, d1, d2, d3, d4, d5, d6, d7;
  __m128i p0, p1, p2, p3, p4, p5, p6, p7;

  do {
    d0 = _mm_load_si128((const __m128i *)(diff + 0 * width));
    d1 = _mm_load_si128((const __m128i *)(diff + 0 * width + 8));
    d2 = _mm_load_si128((const __m128i *)(diff + 0 * width + 16));
    d3 = _mm_load_si128((const __m128i *)(diff + 0 * width + 24));
    d4 = _mm_load_si128((const __m128i *)(diff + 1 * width));
    d5 = _mm_load_si128((const __m128i *)(diff + 1 * width + 8));
    d6 = _mm_load_si128((const __m128i *)(diff + 1 * width + 16));
    d7 = _mm_load_si128((const __m128i *)(diff + 1 * width + 24));

    // Prediction data.
    p1 = _mm_load_si128((const __m128i *)(pred + 0 * pitch));
    p3 = _mm_load_si128((const __m128i *)(pred + 0 * pitch + 16));
    p5 = _mm_load_si128((const __m128i *)(pred + 1 * pitch));
    p7 = _mm_load_si128((const __m128i *)(pred + 1 * pitch + 16));

    p0 = _mm_unpacklo_epi8(p1, zero);
    p1 = _mm_unpackhi_epi8(p1, zero);
    p2 = _mm_unpacklo_epi8(p3, zero);
    p3 = _mm_unpackhi_epi8(p3, zero);
    p4 = _mm_unpacklo_epi8(p5, zero);
    p5 = _mm_unpackhi_epi8(p5, zero);
    p6 = _mm_unpacklo_epi8(p7, zero);
    p7 = _mm_unpackhi_epi8(p7, zero);

    p0 = _mm_add_epi16(p0, d0);
    p1 = _mm_add_epi16(p1, d1);
    p2 = _mm_add_epi16(p2, d2);
    p3 = _mm_add_epi16(p3, d3);
    p4 = _mm_add_epi16(p4, d4);
    p5 = _mm_add_epi16(p5, d5);
    p6 = _mm_add_epi16(p6, d6);
    p7 = _mm_add_epi16(p7, d7);

    p0 = _mm_packus_epi16(p0, p1);
    p1 = _mm_packus_epi16(p2, p3);
    p2 = _mm_packus_epi16(p4, p5);
    p3 = _mm_packus_epi16(p6, p7);

    _mm_store_si128((__m128i *)(dest + 0 * stride), p0);
    _mm_store_si128((__m128i *)(dest + 0 * stride + 16), p1);
    _mm_store_si128((__m128i *)(dest + 1 * stride), p2);
    _mm_store_si128((__m128i *)(dest + 1 * stride + 16), p3);

    diff += 2 * width;
    pred += 2 * pitch;
    dest += 2 * stride;
  } while (--i);
}

void vp9_add_constant_residual_8x8_sse2(const int16_t diff, const uint8_t *pred,
                                        int pitch, uint8_t *dest, int stride) {
  uint8_t abs_diff;
  __m128i d;

  // Prediction data.
  __m128i p0 = _mm_loadl_epi64((const __m128i *)(pred + 0 * pitch));
  __m128i p1 = _mm_loadl_epi64((const __m128i *)(pred + 1 * pitch));
  __m128i p2 = _mm_loadl_epi64((const __m128i *)(pred + 2 * pitch));
  __m128i p3 = _mm_loadl_epi64((const __m128i *)(pred + 3 * pitch));
  __m128i p4 = _mm_loadl_epi64((const __m128i *)(pred + 4 * pitch));
  __m128i p5 = _mm_loadl_epi64((const __m128i *)(pred + 5 * pitch));
  __m128i p6 = _mm_loadl_epi64((const __m128i *)(pred + 6 * pitch));
  __m128i p7 = _mm_loadl_epi64((const __m128i *)(pred + 7 * pitch));

  p0 = _mm_unpacklo_epi64(p0, p1);
  p2 = _mm_unpacklo_epi64(p2, p3);
  p4 = _mm_unpacklo_epi64(p4, p5);
  p6 = _mm_unpacklo_epi64(p6, p7);

  // Clip diff value to [0, 255] range. Then, do addition or subtraction
  // according to its sign.
  if (diff >= 0) {
    abs_diff = (diff > 255) ? 255 : diff;
    d = _mm_shuffle_epi32(_mm_cvtsi32_si128((int)(abs_diff * 0x01010101u)), 0);

    p0 = _mm_adds_epu8(p0, d);
    p2 = _mm_adds_epu8(p2, d);
    p4 = _mm_adds_epu8(p4, d);
    p6 = _mm_adds_epu8(p6, d);
  } else {
    abs_diff = (diff < -255) ? 255 : -diff;
    d = _mm_shuffle_epi32(_mm_cvtsi32_si128((int)(abs_diff * 0x01010101u)), 0);

    p0 = _mm_subs_epu8(p0, d);
    p2 = _mm_subs_epu8(p2, d);
    p4 = _mm_subs_epu8(p4, d);
    p6 = _mm_subs_epu8(p6, d);
  }

  _mm_storel_epi64((__m128i *)(dest + 0 * stride), p0);
  p0 = _mm_srli_si128(p0, 8);
  _mm_storel_epi64((__m128i *)(dest + 1 * stride), p0);

  _mm_storel_epi64((__m128i *)(dest + 2 * stride), p2);
  p2 = _mm_srli_si128(p2, 8);
  _mm_storel_epi64((__m128i *)(dest + 3 * stride), p2);

  _mm_storel_epi64((__m128i *)(dest + 4 * stride), p4);
  p4 = _mm_srli_si128(p4, 8);
  _mm_storel_epi64((__m128i *)(dest + 5 * stride), p4);

  _mm_storel_epi64((__m128i *)(dest + 6 * stride), p6);
  p6 = _mm_srli_si128(p6, 8);
  _mm_storel_epi64((__m128i *)(dest + 7 * stride), p6);
}

void vp9_add_constant_residual_16x16_sse2(const int16_t diff,
                                          const uint8_t *pred, int pitch,
                                          uint8_t *dest, int stride) {
  uint8_t abs_diff;
  __m128i d;

  // Prediction data.
  __m128i p0 = _mm_load_si128((const __m128i *)(pred + 0 * pitch));
  __m128i p1 = _mm_load_si128((const __m128i *)(pred + 1 * pitch));
  __m128i p2 = _mm_load_si128((const __m128i *)(pred + 2 * pitch));
  __m128i p3 = _mm_load_si128((const __m128i *)(pred + 3 * pitch));
  __m128i p4 = _mm_load_si128((const __m128i *)(pred + 4 * pitch));
  __m128i p5 = _mm_load_si128((const __m128i *)(pred + 5 * pitch));
  __m128i p6 = _mm_load_si128((const __m128i *)(pred + 6 * pitch));
  __m128i p7 = _mm_load_si128((const __m128i *)(pred + 7 * pitch));
  __m128i p8 = _mm_load_si128((const __m128i *)(pred + 8 * pitch));
  __m128i p9 = _mm_load_si128((const __m128i *)(pred + 9 * pitch));
  __m128i p10 = _mm_load_si128((const __m128i *)(pred + 10 * pitch));
  __m128i p11 = _mm_load_si128((const __m128i *)(pred + 11 * pitch));
  __m128i p12 = _mm_load_si128((const __m128i *)(pred + 12 * pitch));
  __m128i p13 = _mm_load_si128((const __m128i *)(pred + 13 * pitch));
  __m128i p14 = _mm_load_si128((const __m128i *)(pred + 14 * pitch));
  __m128i p15 = _mm_load_si128((const __m128i *)(pred + 15 * pitch));

  // Clip diff value to [0, 255] range. Then, do addition or subtraction
  // according to its sign.
  if (diff >= 0) {
    abs_diff = (diff > 255) ? 255 : diff;
    d = _mm_shuffle_epi32(_mm_cvtsi32_si128((int)(abs_diff * 0x01010101u)), 0);

    p0 = _mm_adds_epu8(p0, d);
    p1 = _mm_adds_epu8(p1, d);
    p2 = _mm_adds_epu8(p2, d);
    p3 = _mm_adds_epu8(p3, d);
    p4 = _mm_adds_epu8(p4, d);
    p5 = _mm_adds_epu8(p5, d);
    p6 = _mm_adds_epu8(p6, d);
    p7 = _mm_adds_epu8(p7, d);
    p8 = _mm_adds_epu8(p8, d);
    p9 = _mm_adds_epu8(p9, d);
    p10 = _mm_adds_epu8(p10, d);
    p11 = _mm_adds_epu8(p11, d);
    p12 = _mm_adds_epu8(p12, d);
    p13 = _mm_adds_epu8(p13, d);
    p14 = _mm_adds_epu8(p14, d);
    p15 = _mm_adds_epu8(p15, d);
  } else {
    abs_diff = (diff < -255) ? 255 : -diff;
    d = _mm_shuffle_epi32(_mm_cvtsi32_si128((int)(abs_diff * 0x01010101u)), 0);

    p0 = _mm_subs_epu8(p0, d);
    p1 = _mm_subs_epu8(p1, d);
    p2 = _mm_subs_epu8(p2, d);
    p3 = _mm_subs_epu8(p3, d);
    p4 = _mm_subs_epu8(p4, d);
    p5 = _mm_subs_epu8(p5, d);
    p6 = _mm_subs_epu8(p6, d);
    p7 = _mm_subs_epu8(p7, d);
    p8 = _mm_subs_epu8(p8, d);
    p9 = _mm_subs_epu8(p9, d);
    p10 = _mm_subs_epu8(p10, d);
    p11 = _mm_subs_epu8(p11, d);
    p12 = _mm_subs_epu8(p12, d);
    p13 = _mm_subs_epu8(p13, d);
    p14 = _mm_subs_epu8(p14, d);
    p15 = _mm_subs_epu8(p15, d);
  }

  // Store results
  _mm_store_si128((__m128i *)(dest + 0 * stride), p0);
  _mm_store_si128((__m128i *)(dest + 1 * stride), p1);
  _mm_store_si128((__m128i *)(dest + 2 * stride), p2);
  _mm_store_si128((__m128i *)(dest + 3 * stride), p3);
  _mm_store_si128((__m128i *)(dest + 4 * stride), p4);
  _mm_store_si128((__m128i *)(dest + 5 * stride), p5);
  _mm_store_si128((__m128i *)(dest + 6 * stride), p6);
  _mm_store_si128((__m128i *)(dest + 7 * stride), p7);
  _mm_store_si128((__m128i *)(dest + 8 * stride), p8);
  _mm_store_si128((__m128i *)(dest + 9 * stride), p9);
  _mm_store_si128((__m128i *)(dest + 10 * stride), p10);
  _mm_store_si128((__m128i *)(dest + 11 * stride), p11);
  _mm_store_si128((__m128i *)(dest + 12 * stride), p12);
  _mm_store_si128((__m128i *)(dest + 13 * stride), p13);
  _mm_store_si128((__m128i *)(dest + 14 * stride), p14);
  _mm_store_si128((__m128i *)(dest + 15 * stride), p15);
}

void vp9_add_constant_residual_32x32_sse2(const int16_t diff,
                                          const uint8_t *pred, int pitch,
                                          uint8_t *dest, int stride) {
  uint8_t abs_diff;
  __m128i d;
  int i = 8;

  if (diff >= 0) {
    abs_diff = (diff > 255) ? 255 : diff;
    d = _mm_shuffle_epi32(_mm_cvtsi32_si128((int)(abs_diff * 0x01010101u)), 0);
  } else {
    abs_diff = (diff < -255) ? 255 : -diff;
    d = _mm_shuffle_epi32(_mm_cvtsi32_si128((int)(abs_diff * 0x01010101u)), 0);
  }

  do {
    // Prediction data.
    __m128i p0 = _mm_load_si128((const __m128i *)(pred + 0 * pitch));
    __m128i p1 = _mm_load_si128((const __m128i *)(pred + 0 * pitch + 16));
    __m128i p2 = _mm_load_si128((const __m128i *)(pred + 1 * pitch));
    __m128i p3 = _mm_load_si128((const __m128i *)(pred + 1 * pitch + 16));
    __m128i p4 = _mm_load_si128((const __m128i *)(pred + 2 * pitch));
    __m128i p5 = _mm_load_si128((const __m128i *)(pred + 2 * pitch + 16));
    __m128i p6 = _mm_load_si128((const __m128i *)(pred + 3 * pitch));
    __m128i p7 = _mm_load_si128((const __m128i *)(pred + 3 * pitch + 16));

    // Clip diff value to [0, 255] range. Then, do addition or subtraction
    // according to its sign.
    if (diff >= 0) {
      p0 = _mm_adds_epu8(p0, d);
      p1 = _mm_adds_epu8(p1, d);
      p2 = _mm_adds_epu8(p2, d);
      p3 = _mm_adds_epu8(p3, d);
      p4 = _mm_adds_epu8(p4, d);
      p5 = _mm_adds_epu8(p5, d);
      p6 = _mm_adds_epu8(p6, d);
      p7 = _mm_adds_epu8(p7, d);
    } else {
      p0 = _mm_subs_epu8(p0, d);
      p1 = _mm_subs_epu8(p1, d);
      p2 = _mm_subs_epu8(p2, d);
      p3 = _mm_subs_epu8(p3, d);
      p4 = _mm_subs_epu8(p4, d);
      p5 = _mm_subs_epu8(p5, d);
      p6 = _mm_subs_epu8(p6, d);
      p7 = _mm_subs_epu8(p7, d);
    }

    // Store results
    _mm_store_si128((__m128i *)(dest + 0 * stride), p0);
    _mm_store_si128((__m128i *)(dest + 0 * stride + 16), p1);
    _mm_store_si128((__m128i *)(dest + 1 * stride), p2);
    _mm_store_si128((__m128i *)(dest + 1 * stride + 16), p3);
    _mm_store_si128((__m128i *)(dest + 2 * stride), p4);
    _mm_store_si128((__m128i *)(dest + 2 * stride + 16), p5);
    _mm_store_si128((__m128i *)(dest + 3 * stride), p6);
    _mm_store_si128((__m128i *)(dest + 3 * stride + 16), p7);

    pred += 4 * pitch;
    dest += 4 * stride;
  } while (--i);
}
