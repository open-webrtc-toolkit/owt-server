/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <emmintrin.h>  /* SSE2 */
#include "vpx/vpx_integer.h"
#include "vpx_ports/emmintrin_compat.h"

unsigned int vp9_sad16x3_sse2(
  const unsigned char *src_ptr,
  int  src_stride,
  const unsigned char *ref_ptr,
  int  ref_stride) {
  __m128i s0, s1, s2;
  __m128i r0, r1, r2;
  __m128i sad;

  s0 = _mm_loadu_si128((const __m128i *)(src_ptr + 0 * src_stride));
  s1 = _mm_loadu_si128((const __m128i *)(src_ptr + 1 * src_stride));
  s2 = _mm_loadu_si128((const __m128i *)(src_ptr + 2 * src_stride));

  r0 = _mm_loadu_si128((const __m128i *)(ref_ptr + 0 * ref_stride));
  r1 = _mm_loadu_si128((const __m128i *)(ref_ptr + 1 * ref_stride));
  r2 = _mm_loadu_si128((const __m128i *)(ref_ptr + 2 * ref_stride));

  sad = _mm_sad_epu8(s0, r0);
  sad = _mm_add_epi16(sad,  _mm_sad_epu8(s1, r1));
  sad = _mm_add_epi16(sad,  _mm_sad_epu8(s2, r2));
  sad = _mm_add_epi16(sad,  _mm_srli_si128(sad, 8));

  return _mm_cvtsi128_si32(sad);
}

unsigned int vp9_sad3x16_sse2(
  const unsigned char *src_ptr,
  int  src_stride,
  const unsigned char *ref_ptr,
  int  ref_stride) {
  int r;
  __m128i s0, s1, s2, s3;
  __m128i r0, r1, r2, r3;
  __m128i sad = _mm_setzero_si128();
  __m128i mask;
  const int offset = (uintptr_t)src_ptr & 3;

  /* In current use case, the offset is 1 if CONFIG_SUBPELREFMV is off.
   * Here, for offset=1, we adjust src_ptr to be 4-byte aligned. Then, movd
   * takes much less time.
   */
  if (offset == 1)
    src_ptr -= 1;

  /* mask = 0xffffffffffff0000ffffffffffff0000 */
  mask = _mm_cmpeq_epi32(sad, sad);
  mask = _mm_slli_epi64(mask, 16);

  for (r = 0; r < 16; r += 4) {
    s0 = _mm_cvtsi32_si128 (*(const int *)(src_ptr + 0 * src_stride));
    s1 = _mm_cvtsi32_si128 (*(const int *)(src_ptr + 1 * src_stride));
    s2 = _mm_cvtsi32_si128 (*(const int *)(src_ptr + 2 * src_stride));
    s3 = _mm_cvtsi32_si128 (*(const int *)(src_ptr + 3 * src_stride));
    r0 = _mm_cvtsi32_si128 (*(const int *)(ref_ptr + 0 * ref_stride));
    r1 = _mm_cvtsi32_si128 (*(const int *)(ref_ptr + 1 * ref_stride));
    r2 = _mm_cvtsi32_si128 (*(const int *)(ref_ptr + 2 * ref_stride));
    r3 = _mm_cvtsi32_si128 (*(const int *)(ref_ptr + 3 * ref_stride));

    s0 = _mm_unpacklo_epi8(s0, s1);
    r0 = _mm_unpacklo_epi8(r0, r1);
    s2 = _mm_unpacklo_epi8(s2, s3);
    r2 = _mm_unpacklo_epi8(r2, r3);
    s0 = _mm_unpacklo_epi64(s0, s2);
    r0 = _mm_unpacklo_epi64(r0, r2);

    // throw out extra byte
    if (offset == 1)
      s0 = _mm_and_si128(s0, mask);
    else
      s0 = _mm_slli_epi64(s0, 16);
    r0 = _mm_slli_epi64(r0, 16);

    sad = _mm_add_epi16(sad, _mm_sad_epu8(s0, r0));

    src_ptr += src_stride*4;
    ref_ptr += ref_stride*4;
  }

  sad = _mm_add_epi16(sad,  _mm_srli_si128(sad, 8));
  return _mm_cvtsi128_si32(sad);
}
