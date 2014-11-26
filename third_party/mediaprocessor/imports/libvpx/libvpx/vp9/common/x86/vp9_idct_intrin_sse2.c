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

// In order to improve performance, clip absolute diff values to [0, 255],
// which allows to keep the additions/subtractions in 8 bits.
void vp9_dc_only_idct_add_sse2(int input_dc, uint8_t *pred_ptr,
                               uint8_t *dst_ptr, int pitch, int stride) {
  int a1;
  int16_t out;
  uint8_t abs_diff;
  __m128i p0, p1, p2, p3;
  unsigned int extended_diff;
  __m128i diff;

  out = dct_const_round_shift(input_dc * cospi_16_64);
  out = dct_const_round_shift(out * cospi_16_64);
  a1 = ROUND_POWER_OF_TWO(out, 4);

  // Read prediction data.
  p0 = _mm_cvtsi32_si128 (*(const int *)(pred_ptr + 0 * pitch));
  p1 = _mm_cvtsi32_si128 (*(const int *)(pred_ptr + 1 * pitch));
  p2 = _mm_cvtsi32_si128 (*(const int *)(pred_ptr + 2 * pitch));
  p3 = _mm_cvtsi32_si128 (*(const int *)(pred_ptr + 3 * pitch));

  // Unpack prediction data, and store 4x4 array in 1 XMM register.
  p0 = _mm_unpacklo_epi32(p0, p1);
  p2 = _mm_unpacklo_epi32(p2, p3);
  p0 = _mm_unpacklo_epi64(p0, p2);

  // Clip dc value to [0, 255] range. Then, do addition or subtraction
  // according to its sign.
  if (a1 >= 0) {
    abs_diff = (a1 > 255) ? 255 : a1;
    extended_diff = abs_diff * 0x01010101u;
    diff = _mm_shuffle_epi32(_mm_cvtsi32_si128((int)extended_diff), 0);

    p1 = _mm_adds_epu8(p0, diff);
  } else {
    abs_diff = (a1 < -255) ? 255 : -a1;
    extended_diff = abs_diff * 0x01010101u;
    diff = _mm_shuffle_epi32(_mm_cvtsi32_si128((int)extended_diff), 0);

    p1 = _mm_subs_epu8(p0, diff);
  }

  // Store results to dst.
  *(int *)dst_ptr = _mm_cvtsi128_si32(p1);
  dst_ptr += stride;

  p1 = _mm_srli_si128(p1, 4);
  *(int *)dst_ptr = _mm_cvtsi128_si32(p1);
  dst_ptr += stride;

  p1 = _mm_srli_si128(p1, 4);
  *(int *)dst_ptr = _mm_cvtsi128_si32(p1);
  dst_ptr += stride;

  p1 = _mm_srli_si128(p1, 4);
  *(int *)dst_ptr = _mm_cvtsi128_si32(p1);
}

void vp9_short_idct4x4_sse2(int16_t *input, int16_t *output, int pitch) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i eight = _mm_set1_epi16(8);
  const __m128i cst = _mm_setr_epi16((int16_t)cospi_16_64, (int16_t)cospi_16_64,
                                    (int16_t)cospi_16_64, (int16_t)-cospi_16_64,
                                    (int16_t)cospi_24_64, (int16_t)-cospi_8_64,
                                    (int16_t)cospi_8_64, (int16_t)cospi_24_64);
  const __m128i rounding = _mm_set1_epi32(DCT_CONST_ROUNDING);
  const int half_pitch = pitch >> 1;
  __m128i input0, input1, input2, input3;

  // Rows
  input0 = _mm_loadl_epi64((__m128i *)input);
  input1 = _mm_loadl_epi64((__m128i *)(input + 4));
  input2 = _mm_loadl_epi64((__m128i *)(input + 8));
  input3 = _mm_loadl_epi64((__m128i *)(input + 12));

  // Construct i3, i1, i3, i1, i2, i0, i2, i0
  input0 = _mm_shufflelo_epi16(input0, 0xd8);
  input1 = _mm_shufflelo_epi16(input1, 0xd8);
  input2 = _mm_shufflelo_epi16(input2, 0xd8);
  input3 = _mm_shufflelo_epi16(input3, 0xd8);

  input0 = _mm_unpacklo_epi32(input0, input0);
  input1 = _mm_unpacklo_epi32(input1, input1);
  input2 = _mm_unpacklo_epi32(input2, input2);
  input3 = _mm_unpacklo_epi32(input3, input3);

  // Stage 1
  input0 = _mm_madd_epi16(input0, cst);
  input1 = _mm_madd_epi16(input1, cst);
  input2 = _mm_madd_epi16(input2, cst);
  input3 = _mm_madd_epi16(input3, cst);

  input0 = _mm_add_epi32(input0, rounding);
  input1 = _mm_add_epi32(input1, rounding);
  input2 = _mm_add_epi32(input2, rounding);
  input3 = _mm_add_epi32(input3, rounding);

  input0 = _mm_srai_epi32(input0, DCT_CONST_BITS);
  input1 = _mm_srai_epi32(input1, DCT_CONST_BITS);
  input2 = _mm_srai_epi32(input2, DCT_CONST_BITS);
  input3 = _mm_srai_epi32(input3, DCT_CONST_BITS);

  // Stage 2
  input0 = _mm_packs_epi32(input0, zero);
  input1 = _mm_packs_epi32(input1, zero);
  input2 = _mm_packs_epi32(input2, zero);
  input3 = _mm_packs_epi32(input3, zero);

  // Transpose
  input1 = _mm_unpacklo_epi16(input0, input1);
  input3 = _mm_unpacklo_epi16(input2, input3);
  input0 = _mm_unpacklo_epi32(input1, input3);
  input1 = _mm_unpackhi_epi32(input1, input3);

  // Switch column2, column 3, and then, we got:
  // input2: column1, column 0;  input3: column2, column 3.
  input1 = _mm_shuffle_epi32(input1, 0x4e);
  input2 = _mm_add_epi16(input0, input1);
  input3 = _mm_sub_epi16(input0, input1);

  // Columns
  // Construct i3, i1, i3, i1, i2, i0, i2, i0
  input0 = _mm_shufflelo_epi16(input2, 0xd8);
  input1 = _mm_shufflehi_epi16(input2, 0xd8);
  input2 = _mm_shufflehi_epi16(input3, 0xd8);
  input3 = _mm_shufflelo_epi16(input3, 0xd8);

  input0 = _mm_unpacklo_epi32(input0, input0);
  input1 = _mm_unpackhi_epi32(input1, input1);
  input2 = _mm_unpackhi_epi32(input2, input2);
  input3 = _mm_unpacklo_epi32(input3, input3);

  // Stage 1
  input0 = _mm_madd_epi16(input0, cst);
  input1 = _mm_madd_epi16(input1, cst);
  input2 = _mm_madd_epi16(input2, cst);
  input3 = _mm_madd_epi16(input3, cst);

  input0 = _mm_add_epi32(input0, rounding);
  input1 = _mm_add_epi32(input1, rounding);
  input2 = _mm_add_epi32(input2, rounding);
  input3 = _mm_add_epi32(input3, rounding);

  input0 = _mm_srai_epi32(input0, DCT_CONST_BITS);
  input1 = _mm_srai_epi32(input1, DCT_CONST_BITS);
  input2 = _mm_srai_epi32(input2, DCT_CONST_BITS);
  input3 = _mm_srai_epi32(input3, DCT_CONST_BITS);

  // Stage 2
  input0 = _mm_packs_epi32(input0, zero);
  input1 = _mm_packs_epi32(input1, zero);
  input2 = _mm_packs_epi32(input2, zero);
  input3 = _mm_packs_epi32(input3, zero);

  // Transpose
  input1 = _mm_unpacklo_epi16(input0, input1);
  input3 = _mm_unpacklo_epi16(input2, input3);
  input0 = _mm_unpacklo_epi32(input1, input3);
  input1 = _mm_unpackhi_epi32(input1, input3);

  // Switch column2, column 3, and then, we got:
  // input2: column1, column 0;  input3: column2, column 3.
  input1 = _mm_shuffle_epi32(input1, 0x4e);
  input2 = _mm_add_epi16(input0, input1);
  input3 = _mm_sub_epi16(input0, input1);

  // Final round and shift
  input2 = _mm_add_epi16(input2, eight);
  input3 = _mm_add_epi16(input3, eight);

  input2 = _mm_srai_epi16(input2, 4);
  input3 = _mm_srai_epi16(input3, 4);

  // Store results
  _mm_storel_epi64((__m128i *)output, input2);
  input2 = _mm_srli_si128(input2, 8);
  _mm_storel_epi64((__m128i *)(output + half_pitch), input2);

  _mm_storel_epi64((__m128i *)(output + 3 * half_pitch), input3);
  input3 = _mm_srli_si128(input3, 8);
  _mm_storel_epi64((__m128i *)(output + 2 * half_pitch), input3);
}

void vp9_idct4_1d_sse2(int16_t *input, int16_t *output) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i c1 = _mm_setr_epi16((int16_t)cospi_16_64, (int16_t)cospi_16_64,
                                    (int16_t)cospi_16_64, (int16_t)-cospi_16_64,
                                    (int16_t)cospi_24_64, (int16_t)-cospi_8_64,
                                    (int16_t)cospi_8_64, (int16_t)cospi_24_64);
  const __m128i c2 = _mm_setr_epi16(1, 1, 1, 1, 1, -1, 1, -1);

  const __m128i rounding = _mm_set1_epi32(DCT_CONST_ROUNDING);
  __m128i in, temp;

  // Load input data.
  in = _mm_loadl_epi64((__m128i *)input);

  // Construct i3, i1, i3, i1, i2, i0, i2, i0
  in = _mm_shufflelo_epi16(in, 0xd8);
  in = _mm_unpacklo_epi32(in, in);

  // Stage 1
  in = _mm_madd_epi16(in, c1);
  in = _mm_add_epi32(in, rounding);
  in = _mm_srai_epi32(in, DCT_CONST_BITS);
  in = _mm_packs_epi32(in, zero);

  // Stage 2
  temp = _mm_shufflelo_epi16(in, 0x9c);
  in = _mm_shufflelo_epi16(in, 0xc9);
  in = _mm_unpacklo_epi64(temp, in);
  in = _mm_madd_epi16(in, c2);
  in = _mm_packs_epi32(in, zero);

  // Store results
  _mm_storel_epi64((__m128i *)output, in);
}

#define TRANSPOSE_8X8(in0, in1, in2, in3, in4, in5, in6, in7, \
                      out0, out1, out2, out3, out4, out5, out6, out7) \
  {                                                     \
    const __m128i tr0_0 = _mm_unpacklo_epi16(in0, in1); \
    const __m128i tr0_1 = _mm_unpacklo_epi16(in2, in3); \
    const __m128i tr0_2 = _mm_unpackhi_epi16(in0, in1); \
    const __m128i tr0_3 = _mm_unpackhi_epi16(in2, in3); \
    const __m128i tr0_4 = _mm_unpacklo_epi16(in4, in5); \
    const __m128i tr0_5 = _mm_unpacklo_epi16(in6, in7); \
    const __m128i tr0_6 = _mm_unpackhi_epi16(in4, in5); \
    const __m128i tr0_7 = _mm_unpackhi_epi16(in6, in7); \
                                                        \
    const __m128i tr1_0 = _mm_unpacklo_epi32(tr0_0, tr0_1); \
    const __m128i tr1_1 = _mm_unpacklo_epi32(tr0_2, tr0_3); \
    const __m128i tr1_2 = _mm_unpackhi_epi32(tr0_0, tr0_1); \
    const __m128i tr1_3 = _mm_unpackhi_epi32(tr0_2, tr0_3); \
    const __m128i tr1_4 = _mm_unpacklo_epi32(tr0_4, tr0_5); \
    const __m128i tr1_5 = _mm_unpacklo_epi32(tr0_6, tr0_7); \
    const __m128i tr1_6 = _mm_unpackhi_epi32(tr0_4, tr0_5); \
    const __m128i tr1_7 = _mm_unpackhi_epi32(tr0_6, tr0_7); \
                                                            \
    out0 = _mm_unpacklo_epi64(tr1_0, tr1_4); \
    out1 = _mm_unpackhi_epi64(tr1_0, tr1_4); \
    out2 = _mm_unpacklo_epi64(tr1_2, tr1_6); \
    out3 = _mm_unpackhi_epi64(tr1_2, tr1_6); \
    out4 = _mm_unpacklo_epi64(tr1_1, tr1_5); \
    out5 = _mm_unpackhi_epi64(tr1_1, tr1_5); \
    out6 = _mm_unpacklo_epi64(tr1_3, tr1_7); \
    out7 = _mm_unpackhi_epi64(tr1_3, tr1_7); \
  }

#define TRANSPOSE_4X8(in0, in1, in2, in3, in4, in5, in6, in7, \
                      out0, out1, out2, out3, out4, out5, out6, out7) \
  {                                                     \
    const __m128i tr0_0 = _mm_unpacklo_epi16(in0, in1); \
    const __m128i tr0_1 = _mm_unpacklo_epi16(in2, in3); \
    const __m128i tr0_4 = _mm_unpacklo_epi16(in4, in5); \
    const __m128i tr0_5 = _mm_unpacklo_epi16(in6, in7); \
                                                        \
    const __m128i tr1_0 = _mm_unpacklo_epi32(tr0_0, tr0_1); \
    const __m128i tr1_2 = _mm_unpackhi_epi32(tr0_0, tr0_1); \
    const __m128i tr1_4 = _mm_unpacklo_epi32(tr0_4, tr0_5); \
    const __m128i tr1_6 = _mm_unpackhi_epi32(tr0_4, tr0_5); \
                                                            \
    out0 = _mm_unpacklo_epi64(tr1_0, tr1_4); \
    out1 = _mm_unpackhi_epi64(tr1_0, tr1_4); \
    out2 = _mm_unpacklo_epi64(tr1_2, tr1_6); \
    out3 = _mm_unpackhi_epi64(tr1_2, tr1_6); \
    out4 = out5 = out6 = out7 = zero; \
  }

#define TRANSPOSE_8X4(in0, in1, in2, in3, out0, out1, out2, out3) \
  {                                                     \
    const __m128i tr0_0 = _mm_unpacklo_epi16(in0, in1); \
    const __m128i tr0_1 = _mm_unpacklo_epi16(in2, in3); \
    const __m128i tr0_2 = _mm_unpackhi_epi16(in0, in1); \
    const __m128i tr0_3 = _mm_unpackhi_epi16(in2, in3); \
                                                        \
    in0 = _mm_unpacklo_epi32(tr0_0, tr0_1);  /* i1 i0 */  \
    in1 = _mm_unpackhi_epi32(tr0_0, tr0_1);  /* i3 i2 */  \
    in2 = _mm_unpacklo_epi32(tr0_2, tr0_3);  /* i5 i4 */  \
    in3 = _mm_unpackhi_epi32(tr0_2, tr0_3);  /* i7 i6 */  \
  }

// Define Macro for multiplying elements by constants and adding them together.
#define MULTIPLICATION_AND_ADD(lo_0, hi_0, lo_1, hi_1, \
                               cst0, cst1, cst2, cst3, res0, res1, res2, res3) \
  {   \
      tmp0 = _mm_madd_epi16(lo_0, cst0); \
      tmp1 = _mm_madd_epi16(hi_0, cst0); \
      tmp2 = _mm_madd_epi16(lo_0, cst1); \
      tmp3 = _mm_madd_epi16(hi_0, cst1); \
      tmp4 = _mm_madd_epi16(lo_1, cst2); \
      tmp5 = _mm_madd_epi16(hi_1, cst2); \
      tmp6 = _mm_madd_epi16(lo_1, cst3); \
      tmp7 = _mm_madd_epi16(hi_1, cst3); \
      \
      tmp0 = _mm_add_epi32(tmp0, rounding); \
      tmp1 = _mm_add_epi32(tmp1, rounding); \
      tmp2 = _mm_add_epi32(tmp2, rounding); \
      tmp3 = _mm_add_epi32(tmp3, rounding); \
      tmp4 = _mm_add_epi32(tmp4, rounding); \
      tmp5 = _mm_add_epi32(tmp5, rounding); \
      tmp6 = _mm_add_epi32(tmp6, rounding); \
      tmp7 = _mm_add_epi32(tmp7, rounding); \
      \
      tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS); \
      tmp1 = _mm_srai_epi32(tmp1, DCT_CONST_BITS); \
      tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS); \
      tmp3 = _mm_srai_epi32(tmp3, DCT_CONST_BITS); \
      tmp4 = _mm_srai_epi32(tmp4, DCT_CONST_BITS); \
      tmp5 = _mm_srai_epi32(tmp5, DCT_CONST_BITS); \
      tmp6 = _mm_srai_epi32(tmp6, DCT_CONST_BITS); \
      tmp7 = _mm_srai_epi32(tmp7, DCT_CONST_BITS); \
      \
      res0 = _mm_packs_epi32(tmp0, tmp1); \
      res1 = _mm_packs_epi32(tmp2, tmp3); \
      res2 = _mm_packs_epi32(tmp4, tmp5); \
      res3 = _mm_packs_epi32(tmp6, tmp7); \
  }

#define IDCT8x8_1D  \
  /* Stage1 */      \
  { \
    const __m128i lo_17 = _mm_unpacklo_epi16(in1, in7); \
    const __m128i hi_17 = _mm_unpackhi_epi16(in1, in7); \
    const __m128i lo_35 = _mm_unpacklo_epi16(in3, in5); \
    const __m128i hi_35 = _mm_unpackhi_epi16(in3, in5); \
    \
    MULTIPLICATION_AND_ADD(lo_17, hi_17, lo_35, hi_35, stg1_0, \
                          stg1_1, stg1_2, stg1_3, stp1_4,      \
                          stp1_7, stp1_5, stp1_6)              \
  } \
    \
  /* Stage2 */ \
  { \
    const __m128i lo_04 = _mm_unpacklo_epi16(in0, in4); \
    const __m128i hi_04 = _mm_unpackhi_epi16(in0, in4); \
    const __m128i lo_26 = _mm_unpacklo_epi16(in2, in6); \
    const __m128i hi_26 = _mm_unpackhi_epi16(in2, in6); \
    \
    MULTIPLICATION_AND_ADD(lo_04, hi_04, lo_26, hi_26, stg2_0, \
                           stg2_1, stg2_2, stg2_3, stp2_0,     \
                           stp2_1, stp2_2, stp2_3)             \
    \
    stp2_4 = _mm_adds_epi16(stp1_4, stp1_5); \
    stp2_5 = _mm_subs_epi16(stp1_4, stp1_5); \
    stp2_6 = _mm_subs_epi16(stp1_7, stp1_6); \
    stp2_7 = _mm_adds_epi16(stp1_7, stp1_6); \
  } \
    \
  /* Stage3 */ \
  { \
    const __m128i lo_56 = _mm_unpacklo_epi16(stp2_6, stp2_5); \
    const __m128i hi_56 = _mm_unpackhi_epi16(stp2_6, stp2_5); \
    \
    stp1_0 = _mm_adds_epi16(stp2_0, stp2_3); \
    stp1_1 = _mm_adds_epi16(stp2_1, stp2_2); \
    stp1_2 = _mm_subs_epi16(stp2_1, stp2_2); \
    stp1_3 = _mm_subs_epi16(stp2_0, stp2_3); \
    \
    tmp0 = _mm_madd_epi16(lo_56, stg2_1); \
    tmp1 = _mm_madd_epi16(hi_56, stg2_1); \
    tmp2 = _mm_madd_epi16(lo_56, stg2_0); \
    tmp3 = _mm_madd_epi16(hi_56, stg2_0); \
    \
    tmp0 = _mm_add_epi32(tmp0, rounding); \
    tmp1 = _mm_add_epi32(tmp1, rounding); \
    tmp2 = _mm_add_epi32(tmp2, rounding); \
    tmp3 = _mm_add_epi32(tmp3, rounding); \
    \
    tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS); \
    tmp1 = _mm_srai_epi32(tmp1, DCT_CONST_BITS); \
    tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS); \
    tmp3 = _mm_srai_epi32(tmp3, DCT_CONST_BITS); \
    \
    stp1_5 = _mm_packs_epi32(tmp0, tmp1); \
    stp1_6 = _mm_packs_epi32(tmp2, tmp3); \
  } \
  \
  /* Stage4  */ \
  in0 = _mm_adds_epi16(stp1_0, stp2_7); \
  in1 = _mm_adds_epi16(stp1_1, stp1_6); \
  in2 = _mm_adds_epi16(stp1_2, stp1_5); \
  in3 = _mm_adds_epi16(stp1_3, stp2_4); \
  in4 = _mm_subs_epi16(stp1_3, stp2_4); \
  in5 = _mm_subs_epi16(stp1_2, stp1_5); \
  in6 = _mm_subs_epi16(stp1_1, stp1_6); \
  in7 = _mm_subs_epi16(stp1_0, stp2_7);

void vp9_short_idct8x8_sse2(int16_t *input, int16_t *output, int pitch) {
  const int half_pitch = pitch >> 1;
  const __m128i rounding = _mm_set1_epi32(DCT_CONST_ROUNDING);
  const __m128i final_rounding = _mm_set1_epi16(1<<4);
  const __m128i stg1_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg1_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg1_2 = pair_set_epi16(-cospi_20_64, cospi_12_64);
  const __m128i stg1_3 = pair_set_epi16(cospi_12_64, cospi_20_64);
  const __m128i stg2_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg2_2 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i stg2_3 = pair_set_epi16(cospi_8_64, cospi_24_64);

  __m128i in0, in1, in2, in3, in4, in5, in6, in7;
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7;
  __m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  int i;

  // Load input data.
  in0 = _mm_load_si128((__m128i *)input);
  in1 = _mm_load_si128((__m128i *)(input + 8 * 1));
  in2 = _mm_load_si128((__m128i *)(input + 8 * 2));
  in3 = _mm_load_si128((__m128i *)(input + 8 * 3));
  in4 = _mm_load_si128((__m128i *)(input + 8 * 4));
  in5 = _mm_load_si128((__m128i *)(input + 8 * 5));
  in6 = _mm_load_si128((__m128i *)(input + 8 * 6));
  in7 = _mm_load_si128((__m128i *)(input + 8 * 7));

  // 2-D
  for (i = 0; i < 2; i++) {
    // 8x8 Transpose is copied from vp9_short_fdct8x8_sse2()
    TRANSPOSE_8X8(in0, in1, in2, in3, in4, in5, in6, in7, in0, in1, in2, in3,
                  in4, in5, in6, in7);

    // 4-stage 1D idct8x8
    IDCT8x8_1D
  }

  // Final rounding and shift
  in0 = _mm_adds_epi16(in0, final_rounding);
  in1 = _mm_adds_epi16(in1, final_rounding);
  in2 = _mm_adds_epi16(in2, final_rounding);
  in3 = _mm_adds_epi16(in3, final_rounding);
  in4 = _mm_adds_epi16(in4, final_rounding);
  in5 = _mm_adds_epi16(in5, final_rounding);
  in6 = _mm_adds_epi16(in6, final_rounding);
  in7 = _mm_adds_epi16(in7, final_rounding);

  in0 = _mm_srai_epi16(in0, 5);
  in1 = _mm_srai_epi16(in1, 5);
  in2 = _mm_srai_epi16(in2, 5);
  in3 = _mm_srai_epi16(in3, 5);
  in4 = _mm_srai_epi16(in4, 5);
  in5 = _mm_srai_epi16(in5, 5);
  in6 = _mm_srai_epi16(in6, 5);
  in7 = _mm_srai_epi16(in7, 5);

  // Store results
  _mm_store_si128((__m128i *)output, in0);
  _mm_store_si128((__m128i *)(output + half_pitch * 1), in1);
  _mm_store_si128((__m128i *)(output + half_pitch * 2), in2);
  _mm_store_si128((__m128i *)(output + half_pitch * 3), in3);
  _mm_store_si128((__m128i *)(output + half_pitch * 4), in4);
  _mm_store_si128((__m128i *)(output + half_pitch * 5), in5);
  _mm_store_si128((__m128i *)(output + half_pitch * 6), in6);
  _mm_store_si128((__m128i *)(output + half_pitch * 7), in7);
}

void vp9_short_idct10_8x8_sse2(int16_t *input, int16_t *output, int pitch) {
  const int half_pitch = pitch >> 1;
  const __m128i zero = _mm_setzero_si128();
  const __m128i rounding = _mm_set1_epi32(DCT_CONST_ROUNDING);
  const __m128i final_rounding = _mm_set1_epi16(1<<4);
  const __m128i stg1_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg1_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg1_2 = pair_set_epi16(-cospi_20_64, cospi_12_64);
  const __m128i stg1_3 = pair_set_epi16(cospi_12_64, cospi_20_64);
  const __m128i stg2_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg2_2 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i stg2_3 = pair_set_epi16(cospi_8_64, cospi_24_64);
  const __m128i stg3_0 = pair_set_epi16(-cospi_16_64, cospi_16_64);

  __m128i in0, in1, in2, in3, in4, in5, in6, in7;
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7;
  __m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

  // Rows. Load 4-row input data.
  in0 = _mm_load_si128((__m128i *)input);
  in1 = _mm_load_si128((__m128i *)(input + 8 * 1));
  in2 = _mm_load_si128((__m128i *)(input + 8 * 2));
  in3 = _mm_load_si128((__m128i *)(input + 8 * 3));

  // 8x4 Transpose
  TRANSPOSE_8X4(in0, in1, in2, in3, in0, in1, in2, in3)

  // Stage1
  {
    const __m128i lo_17 = _mm_unpackhi_epi16(in0, in3);
    const __m128i lo_35 = _mm_unpackhi_epi16(in1, in2);

    tmp0 = _mm_madd_epi16(lo_17, stg1_0);
    tmp2 = _mm_madd_epi16(lo_17, stg1_1);
    tmp4 = _mm_madd_epi16(lo_35, stg1_2);
    tmp6 = _mm_madd_epi16(lo_35, stg1_3);

    tmp0 = _mm_add_epi32(tmp0, rounding);
    tmp2 = _mm_add_epi32(tmp2, rounding);
    tmp4 = _mm_add_epi32(tmp4, rounding);
    tmp6 = _mm_add_epi32(tmp6, rounding);
    tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS);
    tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS);
    tmp4 = _mm_srai_epi32(tmp4, DCT_CONST_BITS);
    tmp6 = _mm_srai_epi32(tmp6, DCT_CONST_BITS);

    stp1_4 = _mm_packs_epi32(tmp0, zero);
    stp1_7 = _mm_packs_epi32(tmp2, zero);
    stp1_5 = _mm_packs_epi32(tmp4, zero);
    stp1_6 = _mm_packs_epi32(tmp6, zero);
  }

  // Stage2
  {
    const __m128i lo_04 = _mm_unpacklo_epi16(in0, in2);
    const __m128i lo_26 = _mm_unpacklo_epi16(in1, in3);

    tmp0 = _mm_madd_epi16(lo_04, stg2_0);
    tmp2 = _mm_madd_epi16(lo_04, stg2_1);
    tmp4 = _mm_madd_epi16(lo_26, stg2_2);
    tmp6 = _mm_madd_epi16(lo_26, stg2_3);

    tmp0 = _mm_add_epi32(tmp0, rounding);
    tmp2 = _mm_add_epi32(tmp2, rounding);
    tmp4 = _mm_add_epi32(tmp4, rounding);
    tmp6 = _mm_add_epi32(tmp6, rounding);
    tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS);
    tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS);
    tmp4 = _mm_srai_epi32(tmp4, DCT_CONST_BITS);
    tmp6 = _mm_srai_epi32(tmp6, DCT_CONST_BITS);

    stp2_0 = _mm_packs_epi32(tmp0, zero);
    stp2_1 = _mm_packs_epi32(tmp2, zero);
    stp2_2 = _mm_packs_epi32(tmp4, zero);
    stp2_3 = _mm_packs_epi32(tmp6, zero);

    stp2_4 = _mm_adds_epi16(stp1_4, stp1_5);
    stp2_5 = _mm_subs_epi16(stp1_4, stp1_5);
    stp2_6 = _mm_subs_epi16(stp1_7, stp1_6);
    stp2_7 = _mm_adds_epi16(stp1_7, stp1_6);
  }

  // Stage3
  {
    const __m128i lo_56 = _mm_unpacklo_epi16(stp2_5, stp2_6);
    stp1_0 = _mm_adds_epi16(stp2_0, stp2_3);
    stp1_1 = _mm_adds_epi16(stp2_1, stp2_2);
    stp1_2 = _mm_subs_epi16(stp2_1, stp2_2);
    stp1_3 = _mm_subs_epi16(stp2_0, stp2_3);

    tmp0 = _mm_madd_epi16(lo_56, stg3_0);
    tmp2 = _mm_madd_epi16(lo_56, stg2_0);  // stg3_1 = stg2_0

    tmp0 = _mm_add_epi32(tmp0, rounding);
    tmp2 = _mm_add_epi32(tmp2, rounding);
    tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS);
    tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS);

    stp1_5 = _mm_packs_epi32(tmp0, zero);
    stp1_6 = _mm_packs_epi32(tmp2, zero);
  }

  // Stage4
  in0 = _mm_adds_epi16(stp1_0, stp2_7);
  in1 = _mm_adds_epi16(stp1_1, stp1_6);
  in2 = _mm_adds_epi16(stp1_2, stp1_5);
  in3 = _mm_adds_epi16(stp1_3, stp2_4);
  in4 = _mm_subs_epi16(stp1_3, stp2_4);
  in5 = _mm_subs_epi16(stp1_2, stp1_5);
  in6 = _mm_subs_epi16(stp1_1, stp1_6);
  in7 = _mm_subs_epi16(stp1_0, stp2_7);

  // Columns. 4x8 Transpose
  TRANSPOSE_4X8(in0, in1, in2, in3, in4, in5, in6, in7, in0, in1, in2, in3,
                in4, in5, in6, in7)

  // 1D idct8x8
  IDCT8x8_1D

  // Final rounding and shift
  in0 = _mm_adds_epi16(in0, final_rounding);
  in1 = _mm_adds_epi16(in1, final_rounding);
  in2 = _mm_adds_epi16(in2, final_rounding);
  in3 = _mm_adds_epi16(in3, final_rounding);
  in4 = _mm_adds_epi16(in4, final_rounding);
  in5 = _mm_adds_epi16(in5, final_rounding);
  in6 = _mm_adds_epi16(in6, final_rounding);
  in7 = _mm_adds_epi16(in7, final_rounding);

  in0 = _mm_srai_epi16(in0, 5);
  in1 = _mm_srai_epi16(in1, 5);
  in2 = _mm_srai_epi16(in2, 5);
  in3 = _mm_srai_epi16(in3, 5);
  in4 = _mm_srai_epi16(in4, 5);
  in5 = _mm_srai_epi16(in5, 5);
  in6 = _mm_srai_epi16(in6, 5);
  in7 = _mm_srai_epi16(in7, 5);

  // Store results
  _mm_store_si128((__m128i *)output, in0);
  _mm_store_si128((__m128i *)(output + half_pitch * 1), in1);
  _mm_store_si128((__m128i *)(output + half_pitch * 2), in2);
  _mm_store_si128((__m128i *)(output + half_pitch * 3), in3);
  _mm_store_si128((__m128i *)(output + half_pitch * 4), in4);
  _mm_store_si128((__m128i *)(output + half_pitch * 5), in5);
  _mm_store_si128((__m128i *)(output + half_pitch * 6), in6);
  _mm_store_si128((__m128i *)(output + half_pitch * 7), in7);
}

#define IDCT16x16_1D \
  /* Stage2 */ \
  { \
    const __m128i lo_1_15 = _mm_unpacklo_epi16(in1, in15); \
    const __m128i hi_1_15 = _mm_unpackhi_epi16(in1, in15); \
    const __m128i lo_9_7 = _mm_unpacklo_epi16(in9, in7);   \
    const __m128i hi_9_7 = _mm_unpackhi_epi16(in9, in7);   \
    const __m128i lo_5_11 = _mm_unpacklo_epi16(in5, in11); \
    const __m128i hi_5_11 = _mm_unpackhi_epi16(in5, in11); \
    const __m128i lo_13_3 = _mm_unpacklo_epi16(in13, in3); \
    const __m128i hi_13_3 = _mm_unpackhi_epi16(in13, in3); \
    \
    MULTIPLICATION_AND_ADD(lo_1_15, hi_1_15, lo_9_7, hi_9_7, \
                           stg2_0, stg2_1, stg2_2, stg2_3, \
                           stp2_8, stp2_15, stp2_9, stp2_14) \
    \
    MULTIPLICATION_AND_ADD(lo_5_11, hi_5_11, lo_13_3, hi_13_3, \
                           stg2_4, stg2_5, stg2_6, stg2_7, \
                           stp2_10, stp2_13, stp2_11, stp2_12) \
  } \
    \
  /* Stage3 */ \
  { \
    const __m128i lo_2_14 = _mm_unpacklo_epi16(in2, in14); \
    const __m128i hi_2_14 = _mm_unpackhi_epi16(in2, in14); \
    const __m128i lo_10_6 = _mm_unpacklo_epi16(in10, in6); \
    const __m128i hi_10_6 = _mm_unpackhi_epi16(in10, in6); \
    \
    MULTIPLICATION_AND_ADD(lo_2_14, hi_2_14, lo_10_6, hi_10_6, \
                           stg3_0, stg3_1, stg3_2, stg3_3, \
                           stp1_4, stp1_7, stp1_5, stp1_6) \
    \
    stp1_8_0 = _mm_add_epi16(stp2_8, stp2_9);  \
    stp1_9 = _mm_sub_epi16(stp2_8, stp2_9);    \
    stp1_10 = _mm_sub_epi16(stp2_11, stp2_10); \
    stp1_11 = _mm_add_epi16(stp2_11, stp2_10); \
    \
    stp1_12_0 = _mm_add_epi16(stp2_12, stp2_13); \
    stp1_13 = _mm_sub_epi16(stp2_12, stp2_13); \
    stp1_14 = _mm_sub_epi16(stp2_15, stp2_14); \
    stp1_15 = _mm_add_epi16(stp2_15, stp2_14); \
  } \
  \
  /* Stage4 */ \
  { \
    const __m128i lo_0_8 = _mm_unpacklo_epi16(in0, in8); \
    const __m128i hi_0_8 = _mm_unpackhi_epi16(in0, in8); \
    const __m128i lo_4_12 = _mm_unpacklo_epi16(in4, in12); \
    const __m128i hi_4_12 = _mm_unpackhi_epi16(in4, in12); \
    \
    const __m128i lo_9_14 = _mm_unpacklo_epi16(stp1_9, stp1_14); \
    const __m128i hi_9_14 = _mm_unpackhi_epi16(stp1_9, stp1_14); \
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13); \
    const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13); \
    \
    MULTIPLICATION_AND_ADD(lo_0_8, hi_0_8, lo_4_12, hi_4_12, \
                           stg4_0, stg4_1, stg4_2, stg4_3, \
                           stp2_0, stp2_1, stp2_2, stp2_3) \
    \
    stp2_4 = _mm_add_epi16(stp1_4, stp1_5); \
    stp2_5 = _mm_sub_epi16(stp1_4, stp1_5); \
    stp2_6 = _mm_sub_epi16(stp1_7, stp1_6); \
    stp2_7 = _mm_add_epi16(stp1_7, stp1_6); \
    \
    MULTIPLICATION_AND_ADD(lo_9_14, hi_9_14, lo_10_13, hi_10_13, \
                           stg4_4, stg4_5, stg4_6, stg4_7, \
                           stp2_9, stp2_14, stp2_10, stp2_13) \
  } \
    \
  /* Stage5 */ \
  { \
    const __m128i lo_6_5 = _mm_unpacklo_epi16(stp2_6, stp2_5); \
    const __m128i hi_6_5 = _mm_unpackhi_epi16(stp2_6, stp2_5); \
    \
    stp1_0 = _mm_add_epi16(stp2_0, stp2_3); \
    stp1_1 = _mm_add_epi16(stp2_1, stp2_2); \
    stp1_2 = _mm_sub_epi16(stp2_1, stp2_2); \
    stp1_3 = _mm_sub_epi16(stp2_0, stp2_3); \
    \
    tmp0 = _mm_madd_epi16(lo_6_5, stg4_1); \
    tmp1 = _mm_madd_epi16(hi_6_5, stg4_1); \
    tmp2 = _mm_madd_epi16(lo_6_5, stg4_0); \
    tmp3 = _mm_madd_epi16(hi_6_5, stg4_0); \
    \
    tmp0 = _mm_add_epi32(tmp0, rounding); \
    tmp1 = _mm_add_epi32(tmp1, rounding); \
    tmp2 = _mm_add_epi32(tmp2, rounding); \
    tmp3 = _mm_add_epi32(tmp3, rounding); \
    \
    tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS); \
    tmp1 = _mm_srai_epi32(tmp1, DCT_CONST_BITS); \
    tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS); \
    tmp3 = _mm_srai_epi32(tmp3, DCT_CONST_BITS); \
    \
    stp1_5 = _mm_packs_epi32(tmp0, tmp1); \
    stp1_6 = _mm_packs_epi32(tmp2, tmp3); \
    \
    stp1_8 = _mm_add_epi16(stp1_8_0, stp1_11);  \
    stp1_9 = _mm_add_epi16(stp2_9, stp2_10);    \
    stp1_10 = _mm_sub_epi16(stp2_9, stp2_10);   \
    stp1_11 = _mm_sub_epi16(stp1_8_0, stp1_11); \
    \
    stp1_12 = _mm_sub_epi16(stp1_15, stp1_12_0); \
    stp1_13 = _mm_sub_epi16(stp2_14, stp2_13);   \
    stp1_14 = _mm_add_epi16(stp2_14, stp2_13);   \
    stp1_15 = _mm_add_epi16(stp1_15, stp1_12_0); \
  } \
    \
  /* Stage6 */ \
  { \
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13); \
    const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13); \
    const __m128i lo_11_12 = _mm_unpacklo_epi16(stp1_11, stp1_12); \
    const __m128i hi_11_12 = _mm_unpackhi_epi16(stp1_11, stp1_12); \
    \
    stp2_0 = _mm_add_epi16(stp1_0, stp2_7); \
    stp2_1 = _mm_add_epi16(stp1_1, stp1_6); \
    stp2_2 = _mm_add_epi16(stp1_2, stp1_5); \
    stp2_3 = _mm_add_epi16(stp1_3, stp2_4); \
    stp2_4 = _mm_sub_epi16(stp1_3, stp2_4); \
    stp2_5 = _mm_sub_epi16(stp1_2, stp1_5); \
    stp2_6 = _mm_sub_epi16(stp1_1, stp1_6); \
    stp2_7 = _mm_sub_epi16(stp1_0, stp2_7); \
    \
    MULTIPLICATION_AND_ADD(lo_10_13, hi_10_13, lo_11_12, hi_11_12, \
                           stg6_0, stg4_0, stg6_0, stg4_0, \
                           stp2_10, stp2_13, stp2_11, stp2_12) \
  }

void vp9_short_idct16x16_sse2(int16_t *input, int16_t *output, int pitch) {
  const int half_pitch = pitch >> 1;
  const __m128i rounding = _mm_set1_epi32(DCT_CONST_ROUNDING);
  const __m128i final_rounding = _mm_set1_epi16(1<<5);
  const __m128i zero = _mm_setzero_si128();

  const __m128i stg2_0 = pair_set_epi16(cospi_30_64, -cospi_2_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_2_64, cospi_30_64);
  const __m128i stg2_2 = pair_set_epi16(cospi_14_64, -cospi_18_64);
  const __m128i stg2_3 = pair_set_epi16(cospi_18_64, cospi_14_64);
  const __m128i stg2_4 = pair_set_epi16(cospi_22_64, -cospi_10_64);
  const __m128i stg2_5 = pair_set_epi16(cospi_10_64, cospi_22_64);
  const __m128i stg2_6 = pair_set_epi16(cospi_6_64, -cospi_26_64);
  const __m128i stg2_7 = pair_set_epi16(cospi_26_64, cospi_6_64);

  const __m128i stg3_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg3_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg3_2 = pair_set_epi16(cospi_12_64, -cospi_20_64);
  const __m128i stg3_3 = pair_set_epi16(cospi_20_64, cospi_12_64);

  const __m128i stg4_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg4_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg4_2 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i stg4_3 = pair_set_epi16(cospi_8_64, cospi_24_64);
  const __m128i stg4_4 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i stg4_5 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i stg4_6 = pair_set_epi16(-cospi_24_64, -cospi_8_64);
  const __m128i stg4_7 = pair_set_epi16(-cospi_8_64, cospi_24_64);

  const __m128i stg6_0 = pair_set_epi16(-cospi_16_64, cospi_16_64);

  __m128i in0 = zero, in1 = zero, in2 = zero, in3 = zero, in4 = zero,
          in5 = zero, in6 = zero, in7 = zero, in8 = zero, in9 = zero,
          in10 = zero, in11 = zero, in12 = zero, in13 = zero,
          in14 = zero, in15 = zero;
  __m128i l0 = zero, l1 = zero, l2 = zero, l3 = zero, l4 = zero, l5 = zero,
          l6 = zero, l7 = zero, l8 = zero, l9 = zero, l10 = zero, l11 = zero,
          l12 = zero, l13 = zero, l14 = zero, l15 = zero;
  __m128i r0 = zero, r1 = zero, r2 = zero, r3 = zero, r4 = zero, r5 = zero,
          r6 = zero, r7 = zero, r8 = zero, r9 = zero, r10 = zero, r11 = zero,
          r12 = zero, r13 = zero, r14 = zero, r15 = zero;
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7,
          stp1_8, stp1_9, stp1_10, stp1_11, stp1_12, stp1_13, stp1_14, stp1_15,
          stp1_8_0, stp1_12_0;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7,
          stp2_8, stp2_9, stp2_10, stp2_11, stp2_12, stp2_13, stp2_14, stp2_15;
  __m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  int i;

  // We work on a 8x16 block each time, and loop 4 times for 2-D 16x16 idct.
  for (i = 0; i < 4; i++) {
    // 1-D idct
    if (i < 2) {
      if (i == 1) input += 128;

      // Load input data.
      in0 = _mm_load_si128((__m128i *)input);
      in8 = _mm_load_si128((__m128i *)(input + 8 * 1));
      in1 = _mm_load_si128((__m128i *)(input + 8 * 2));
      in9 = _mm_load_si128((__m128i *)(input + 8 * 3));
      in2 = _mm_load_si128((__m128i *)(input + 8 * 4));
      in10 = _mm_load_si128((__m128i *)(input + 8 * 5));
      in3 = _mm_load_si128((__m128i *)(input + 8 * 6));
      in11 = _mm_load_si128((__m128i *)(input + 8 * 7));
      in4 = _mm_load_si128((__m128i *)(input + 8 * 8));
      in12 = _mm_load_si128((__m128i *)(input + 8 * 9));
      in5 = _mm_load_si128((__m128i *)(input + 8 * 10));
      in13 = _mm_load_si128((__m128i *)(input + 8 * 11));
      in6 = _mm_load_si128((__m128i *)(input + 8 * 12));
      in14 = _mm_load_si128((__m128i *)(input + 8 * 13));
      in7 = _mm_load_si128((__m128i *)(input + 8 * 14));
      in15 = _mm_load_si128((__m128i *)(input + 8 * 15));

      TRANSPOSE_8X8(in0, in1, in2, in3, in4, in5, in6, in7, in0, in1, in2, in3,
                    in4, in5, in6, in7);
      TRANSPOSE_8X8(in8, in9, in10, in11, in12, in13, in14, in15, in8, in9,
                    in10, in11, in12, in13, in14, in15);
    }

    if (i == 2) {
      TRANSPOSE_8X8(l0, l1, l2, l3, l4, l5, l6, l7, in0, in1, in2, in3, in4,
                    in5, in6, in7);
      TRANSPOSE_8X8(r0, r1, r2, r3, r4, r5, r6, r7, in8, in9, in10, in11, in12,
                    in13, in14, in15);
    }

    if (i == 3) {
      TRANSPOSE_8X8(l8, l9, l10, l11, l12, l13, l14, l15, in0, in1, in2, in3,
                    in4, in5, in6, in7);
      TRANSPOSE_8X8(r8, r9, r10, r11, r12, r13, r14, r15, in8, in9, in10, in11,
                    in12, in13, in14, in15);
    }

    IDCT16x16_1D

    // Stage7
    if (i == 0) {
      // Left 8x16
      l0 = _mm_add_epi16(stp2_0, stp1_15);
      l1 = _mm_add_epi16(stp2_1, stp1_14);
      l2 = _mm_add_epi16(stp2_2, stp2_13);
      l3 = _mm_add_epi16(stp2_3, stp2_12);
      l4 = _mm_add_epi16(stp2_4, stp2_11);
      l5 = _mm_add_epi16(stp2_5, stp2_10);
      l6 = _mm_add_epi16(stp2_6, stp1_9);
      l7 = _mm_add_epi16(stp2_7, stp1_8);
      l8 = _mm_sub_epi16(stp2_7, stp1_8);
      l9 = _mm_sub_epi16(stp2_6, stp1_9);
      l10 = _mm_sub_epi16(stp2_5, stp2_10);
      l11 = _mm_sub_epi16(stp2_4, stp2_11);
      l12 = _mm_sub_epi16(stp2_3, stp2_12);
      l13 = _mm_sub_epi16(stp2_2, stp2_13);
      l14 = _mm_sub_epi16(stp2_1, stp1_14);
      l15 = _mm_sub_epi16(stp2_0, stp1_15);
    } else if (i == 1) {
      // Right 8x16
      r0 = _mm_add_epi16(stp2_0, stp1_15);
      r1 = _mm_add_epi16(stp2_1, stp1_14);
      r2 = _mm_add_epi16(stp2_2, stp2_13);
      r3 = _mm_add_epi16(stp2_3, stp2_12);
      r4 = _mm_add_epi16(stp2_4, stp2_11);
      r5 = _mm_add_epi16(stp2_5, stp2_10);
      r6 = _mm_add_epi16(stp2_6, stp1_9);
      r7 = _mm_add_epi16(stp2_7, stp1_8);
      r8 = _mm_sub_epi16(stp2_7, stp1_8);
      r9 = _mm_sub_epi16(stp2_6, stp1_9);
      r10 = _mm_sub_epi16(stp2_5, stp2_10);
      r11 = _mm_sub_epi16(stp2_4, stp2_11);
      r12 = _mm_sub_epi16(stp2_3, stp2_12);
      r13 = _mm_sub_epi16(stp2_2, stp2_13);
      r14 = _mm_sub_epi16(stp2_1, stp1_14);
      r15 = _mm_sub_epi16(stp2_0, stp1_15);
    } else {
      // 2-D
      in0 = _mm_add_epi16(stp2_0, stp1_15);
      in1 = _mm_add_epi16(stp2_1, stp1_14);
      in2 = _mm_add_epi16(stp2_2, stp2_13);
      in3 = _mm_add_epi16(stp2_3, stp2_12);
      in4 = _mm_add_epi16(stp2_4, stp2_11);
      in5 = _mm_add_epi16(stp2_5, stp2_10);
      in6 = _mm_add_epi16(stp2_6, stp1_9);
      in7 = _mm_add_epi16(stp2_7, stp1_8);
      in8 = _mm_sub_epi16(stp2_7, stp1_8);
      in9 = _mm_sub_epi16(stp2_6, stp1_9);
      in10 = _mm_sub_epi16(stp2_5, stp2_10);
      in11 = _mm_sub_epi16(stp2_4, stp2_11);
      in12 = _mm_sub_epi16(stp2_3, stp2_12);
      in13 = _mm_sub_epi16(stp2_2, stp2_13);
      in14 = _mm_sub_epi16(stp2_1, stp1_14);
      in15 = _mm_sub_epi16(stp2_0, stp1_15);

      // Final rounding and shift
      in0 = _mm_adds_epi16(in0, final_rounding);
      in1 = _mm_adds_epi16(in1, final_rounding);
      in2 = _mm_adds_epi16(in2, final_rounding);
      in3 = _mm_adds_epi16(in3, final_rounding);
      in4 = _mm_adds_epi16(in4, final_rounding);
      in5 = _mm_adds_epi16(in5, final_rounding);
      in6 = _mm_adds_epi16(in6, final_rounding);
      in7 = _mm_adds_epi16(in7, final_rounding);
      in8 = _mm_adds_epi16(in8, final_rounding);
      in9 = _mm_adds_epi16(in9, final_rounding);
      in10 = _mm_adds_epi16(in10, final_rounding);
      in11 = _mm_adds_epi16(in11, final_rounding);
      in12 = _mm_adds_epi16(in12, final_rounding);
      in13 = _mm_adds_epi16(in13, final_rounding);
      in14 = _mm_adds_epi16(in14, final_rounding);
      in15 = _mm_adds_epi16(in15, final_rounding);

      in0 = _mm_srai_epi16(in0, 6);
      in1 = _mm_srai_epi16(in1, 6);
      in2 = _mm_srai_epi16(in2, 6);
      in3 = _mm_srai_epi16(in3, 6);
      in4 = _mm_srai_epi16(in4, 6);
      in5 = _mm_srai_epi16(in5, 6);
      in6 = _mm_srai_epi16(in6, 6);
      in7 = _mm_srai_epi16(in7, 6);
      in8 = _mm_srai_epi16(in8, 6);
      in9 = _mm_srai_epi16(in9, 6);
      in10 = _mm_srai_epi16(in10, 6);
      in11 = _mm_srai_epi16(in11, 6);
      in12 = _mm_srai_epi16(in12, 6);
      in13 = _mm_srai_epi16(in13, 6);
      in14 = _mm_srai_epi16(in14, 6);
      in15 = _mm_srai_epi16(in15, 6);

      // Store results
      _mm_store_si128((__m128i *)output, in0);
      _mm_store_si128((__m128i *)(output + half_pitch * 1), in1);
      _mm_store_si128((__m128i *)(output + half_pitch * 2), in2);
      _mm_store_si128((__m128i *)(output + half_pitch * 3), in3);
      _mm_store_si128((__m128i *)(output + half_pitch * 4), in4);
      _mm_store_si128((__m128i *)(output + half_pitch * 5), in5);
      _mm_store_si128((__m128i *)(output + half_pitch * 6), in6);
      _mm_store_si128((__m128i *)(output + half_pitch * 7), in7);
      _mm_store_si128((__m128i *)(output + half_pitch * 8), in8);
      _mm_store_si128((__m128i *)(output + half_pitch * 9), in9);
      _mm_store_si128((__m128i *)(output + half_pitch * 10), in10);
      _mm_store_si128((__m128i *)(output + half_pitch * 11), in11);
      _mm_store_si128((__m128i *)(output + half_pitch * 12), in12);
      _mm_store_si128((__m128i *)(output + half_pitch * 13), in13);
      _mm_store_si128((__m128i *)(output + half_pitch * 14), in14);
      _mm_store_si128((__m128i *)(output + half_pitch * 15), in15);

      output += 8;
    }
  }
}

void vp9_short_idct10_16x16_sse2(int16_t *input, int16_t *output, int pitch) {
  const int half_pitch = pitch >> 1;
  const __m128i rounding = _mm_set1_epi32(DCT_CONST_ROUNDING);
  const __m128i final_rounding = _mm_set1_epi16(1<<5);
  const __m128i zero = _mm_setzero_si128();

  const __m128i stg2_0 = pair_set_epi16(cospi_30_64, -cospi_2_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_2_64, cospi_30_64);
  const __m128i stg2_2 = pair_set_epi16(cospi_14_64, -cospi_18_64);
  const __m128i stg2_3 = pair_set_epi16(cospi_18_64, cospi_14_64);
  const __m128i stg2_4 = pair_set_epi16(cospi_22_64, -cospi_10_64);
  const __m128i stg2_5 = pair_set_epi16(cospi_10_64, cospi_22_64);
  const __m128i stg2_6 = pair_set_epi16(cospi_6_64, -cospi_26_64);
  const __m128i stg2_7 = pair_set_epi16(cospi_26_64, cospi_6_64);

  const __m128i stg3_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg3_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg3_2 = pair_set_epi16(cospi_12_64, -cospi_20_64);
  const __m128i stg3_3 = pair_set_epi16(cospi_20_64, cospi_12_64);

  const __m128i stg4_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg4_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg4_2 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i stg4_3 = pair_set_epi16(cospi_8_64, cospi_24_64);
  const __m128i stg4_4 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i stg4_5 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i stg4_6 = pair_set_epi16(-cospi_24_64, -cospi_8_64);
  const __m128i stg4_7 = pair_set_epi16(-cospi_8_64, cospi_24_64);

  const __m128i stg6_0 = pair_set_epi16(-cospi_16_64, cospi_16_64);

  __m128i in0 = zero, in1 = zero, in2 = zero, in3 = zero, in4 = zero,
          in5 = zero, in6 = zero, in7 = zero, in8 = zero, in9 = zero,
          in10 = zero, in11 = zero, in12 = zero, in13 = zero,
          in14 = zero, in15 = zero;
  __m128i l0 = zero, l1 = zero, l2 = zero, l3 = zero, l4 = zero, l5 = zero,
          l6 = zero, l7 = zero, l8 = zero, l9 = zero, l10 = zero, l11 = zero,
          l12 = zero, l13 = zero, l14 = zero, l15 = zero;

  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7,
          stp1_8, stp1_9, stp1_10, stp1_11, stp1_12, stp1_13, stp1_14, stp1_15,
          stp1_8_0, stp1_12_0;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7,
          stp2_8, stp2_9, stp2_10, stp2_11, stp2_12, stp2_13, stp2_14, stp2_15;
  __m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  int i;

  // 1-D idct. Load input data.
  in0 = _mm_load_si128((__m128i *)input);
  in8 = _mm_load_si128((__m128i *)(input + 8 * 1));
  in1 = _mm_load_si128((__m128i *)(input + 8 * 2));
  in9 = _mm_load_si128((__m128i *)(input + 8 * 3));
  in2 = _mm_load_si128((__m128i *)(input + 8 * 4));
  in10 = _mm_load_si128((__m128i *)(input + 8 * 5));
  in3 = _mm_load_si128((__m128i *)(input + 8 * 6));
  in11 = _mm_load_si128((__m128i *)(input + 8 * 7));

  TRANSPOSE_8X4(in0, in1, in2, in3, in0, in1, in2, in3);
  TRANSPOSE_8X4(in8, in9, in10, in11, in8, in9, in10, in11);

  // Stage2
  {
    const __m128i lo_1_15 = _mm_unpackhi_epi16(in0, in11);
    const __m128i lo_9_7 = _mm_unpackhi_epi16(in8, in3);
    const __m128i lo_5_11 = _mm_unpackhi_epi16(in2, in9);
    const __m128i lo_13_3 = _mm_unpackhi_epi16(in10, in1);

    tmp0 = _mm_madd_epi16(lo_1_15, stg2_0);
    tmp2 = _mm_madd_epi16(lo_1_15, stg2_1);
    tmp4 = _mm_madd_epi16(lo_9_7, stg2_2);
    tmp6 = _mm_madd_epi16(lo_9_7, stg2_3);
    tmp1 = _mm_madd_epi16(lo_5_11, stg2_4);
    tmp3 = _mm_madd_epi16(lo_5_11, stg2_5);
    tmp5 = _mm_madd_epi16(lo_13_3, stg2_6);
    tmp7 = _mm_madd_epi16(lo_13_3, stg2_7);

    tmp0 = _mm_add_epi32(tmp0, rounding);
    tmp2 = _mm_add_epi32(tmp2, rounding);
    tmp4 = _mm_add_epi32(tmp4, rounding);
    tmp6 = _mm_add_epi32(tmp6, rounding);
    tmp1 = _mm_add_epi32(tmp1, rounding);
    tmp3 = _mm_add_epi32(tmp3, rounding);
    tmp5 = _mm_add_epi32(tmp5, rounding);
    tmp7 = _mm_add_epi32(tmp7, rounding);

    tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS);
    tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS);
    tmp4 = _mm_srai_epi32(tmp4, DCT_CONST_BITS);
    tmp6 = _mm_srai_epi32(tmp6, DCT_CONST_BITS);
    tmp1 = _mm_srai_epi32(tmp1, DCT_CONST_BITS);
    tmp3 = _mm_srai_epi32(tmp3, DCT_CONST_BITS);
    tmp5 = _mm_srai_epi32(tmp5, DCT_CONST_BITS);
    tmp7 = _mm_srai_epi32(tmp7, DCT_CONST_BITS);

    stp2_8 = _mm_packs_epi32(tmp0, zero);
    stp2_15 = _mm_packs_epi32(tmp2, zero);
    stp2_9 = _mm_packs_epi32(tmp4, zero);
    stp2_14 = _mm_packs_epi32(tmp6, zero);

    stp2_10 = _mm_packs_epi32(tmp1, zero);
    stp2_13 = _mm_packs_epi32(tmp3, zero);
    stp2_11 = _mm_packs_epi32(tmp5, zero);
    stp2_12 = _mm_packs_epi32(tmp7, zero);
  }

  // Stage3
  {
    const __m128i lo_2_14 = _mm_unpacklo_epi16(in1, in11);
    const __m128i lo_10_6 = _mm_unpacklo_epi16(in9, in3);

    tmp0 = _mm_madd_epi16(lo_2_14, stg3_0);
    tmp2 = _mm_madd_epi16(lo_2_14, stg3_1);
    tmp4 = _mm_madd_epi16(lo_10_6, stg3_2);
    tmp6 = _mm_madd_epi16(lo_10_6, stg3_3);

    tmp0 = _mm_add_epi32(tmp0, rounding);
    tmp2 = _mm_add_epi32(tmp2, rounding);
    tmp4 = _mm_add_epi32(tmp4, rounding);
    tmp6 = _mm_add_epi32(tmp6, rounding);

    tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS);
    tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS);
    tmp4 = _mm_srai_epi32(tmp4, DCT_CONST_BITS);
    tmp6 = _mm_srai_epi32(tmp6, DCT_CONST_BITS);

    stp1_4 = _mm_packs_epi32(tmp0, zero);
    stp1_7 = _mm_packs_epi32(tmp2, zero);
    stp1_5 = _mm_packs_epi32(tmp4, zero);
    stp1_6 = _mm_packs_epi32(tmp6, zero);

    stp1_8_0 = _mm_add_epi16(stp2_8, stp2_9);
    stp1_9 = _mm_sub_epi16(stp2_8, stp2_9);
    stp1_10 = _mm_sub_epi16(stp2_11, stp2_10);
    stp1_11 = _mm_add_epi16(stp2_11, stp2_10);

    stp1_12_0 = _mm_add_epi16(stp2_12, stp2_13);
    stp1_13 = _mm_sub_epi16(stp2_12, stp2_13);
    stp1_14 = _mm_sub_epi16(stp2_15, stp2_14);
    stp1_15 = _mm_add_epi16(stp2_15, stp2_14);
  }

  // Stage4
  {
    const __m128i lo_0_8 = _mm_unpacklo_epi16(in0, in8);
    const __m128i lo_4_12 = _mm_unpacklo_epi16(in2, in10);
    const __m128i lo_9_14 = _mm_unpacklo_epi16(stp1_9, stp1_14);
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);

    tmp0 = _mm_madd_epi16(lo_0_8, stg4_0);
    tmp2 = _mm_madd_epi16(lo_0_8, stg4_1);
    tmp4 = _mm_madd_epi16(lo_4_12, stg4_2);
    tmp6 = _mm_madd_epi16(lo_4_12, stg4_3);
    tmp1 = _mm_madd_epi16(lo_9_14, stg4_4);
    tmp3 = _mm_madd_epi16(lo_9_14, stg4_5);
    tmp5 = _mm_madd_epi16(lo_10_13, stg4_6);
    tmp7 = _mm_madd_epi16(lo_10_13, stg4_7);

    tmp0 = _mm_add_epi32(tmp0, rounding);
    tmp2 = _mm_add_epi32(tmp2, rounding);
    tmp4 = _mm_add_epi32(tmp4, rounding);
    tmp6 = _mm_add_epi32(tmp6, rounding);
    tmp1 = _mm_add_epi32(tmp1, rounding);
    tmp3 = _mm_add_epi32(tmp3, rounding);
    tmp5 = _mm_add_epi32(tmp5, rounding);
    tmp7 = _mm_add_epi32(tmp7, rounding);

    tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS);
    tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS);
    tmp4 = _mm_srai_epi32(tmp4, DCT_CONST_BITS);
    tmp6 = _mm_srai_epi32(tmp6, DCT_CONST_BITS);
    tmp1 = _mm_srai_epi32(tmp1, DCT_CONST_BITS);
    tmp3 = _mm_srai_epi32(tmp3, DCT_CONST_BITS);
    tmp5 = _mm_srai_epi32(tmp5, DCT_CONST_BITS);
    tmp7 = _mm_srai_epi32(tmp7, DCT_CONST_BITS);

    stp2_0 = _mm_packs_epi32(tmp0, zero);
    stp2_1 = _mm_packs_epi32(tmp2, zero);
    stp2_2 = _mm_packs_epi32(tmp4, zero);
    stp2_3 = _mm_packs_epi32(tmp6, zero);
    stp2_9 = _mm_packs_epi32(tmp1, zero);
    stp2_14 = _mm_packs_epi32(tmp3, zero);
    stp2_10 = _mm_packs_epi32(tmp5, zero);
    stp2_13 = _mm_packs_epi32(tmp7, zero);

    stp2_4 = _mm_add_epi16(stp1_4, stp1_5);
    stp2_5 = _mm_sub_epi16(stp1_4, stp1_5);
    stp2_6 = _mm_sub_epi16(stp1_7, stp1_6);
    stp2_7 = _mm_add_epi16(stp1_7, stp1_6);
  }

  // Stage5 and Stage6
  {
    stp1_0 = _mm_add_epi16(stp2_0, stp2_3);
    stp1_1 = _mm_add_epi16(stp2_1, stp2_2);
    stp1_2 = _mm_sub_epi16(stp2_1, stp2_2);
    stp1_3 = _mm_sub_epi16(stp2_0, stp2_3);

    stp1_8 = _mm_add_epi16(stp1_8_0, stp1_11);
    stp1_9 = _mm_add_epi16(stp2_9, stp2_10);
    stp1_10 = _mm_sub_epi16(stp2_9, stp2_10);
    stp1_11 = _mm_sub_epi16(stp1_8_0, stp1_11);

    stp1_12 = _mm_sub_epi16(stp1_15, stp1_12_0);
    stp1_13 = _mm_sub_epi16(stp2_14, stp2_13);
    stp1_14 = _mm_add_epi16(stp2_14, stp2_13);
    stp1_15 = _mm_add_epi16(stp1_15, stp1_12_0);
  }

  // Stage6
  {
    const __m128i lo_6_5 = _mm_unpacklo_epi16(stp2_6, stp2_5);
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);
    const __m128i lo_11_12 = _mm_unpacklo_epi16(stp1_11, stp1_12);

    tmp1 = _mm_madd_epi16(lo_6_5, stg4_1);
    tmp3 = _mm_madd_epi16(lo_6_5, stg4_0);
    tmp0 = _mm_madd_epi16(lo_10_13, stg6_0);
    tmp2 = _mm_madd_epi16(lo_10_13, stg4_0);
    tmp4 = _mm_madd_epi16(lo_11_12, stg6_0);
    tmp6 = _mm_madd_epi16(lo_11_12, stg4_0);

    tmp1 = _mm_add_epi32(tmp1, rounding);
    tmp3 = _mm_add_epi32(tmp3, rounding);
    tmp0 = _mm_add_epi32(tmp0, rounding);
    tmp2 = _mm_add_epi32(tmp2, rounding);
    tmp4 = _mm_add_epi32(tmp4, rounding);
    tmp6 = _mm_add_epi32(tmp6, rounding);

    tmp1 = _mm_srai_epi32(tmp1, DCT_CONST_BITS);
    tmp3 = _mm_srai_epi32(tmp3, DCT_CONST_BITS);
    tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS);
    tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS);
    tmp4 = _mm_srai_epi32(tmp4, DCT_CONST_BITS);
    tmp6 = _mm_srai_epi32(tmp6, DCT_CONST_BITS);

    stp1_5 = _mm_packs_epi32(tmp1, zero);
    stp1_6 = _mm_packs_epi32(tmp3, zero);
    stp2_10 = _mm_packs_epi32(tmp0, zero);
    stp2_13 = _mm_packs_epi32(tmp2, zero);
    stp2_11 = _mm_packs_epi32(tmp4, zero);
    stp2_12 = _mm_packs_epi32(tmp6, zero);

    stp2_0 = _mm_add_epi16(stp1_0, stp2_7);
    stp2_1 = _mm_add_epi16(stp1_1, stp1_6);
    stp2_2 = _mm_add_epi16(stp1_2, stp1_5);
    stp2_3 = _mm_add_epi16(stp1_3, stp2_4);
    stp2_4 = _mm_sub_epi16(stp1_3, stp2_4);
    stp2_5 = _mm_sub_epi16(stp1_2, stp1_5);
    stp2_6 = _mm_sub_epi16(stp1_1, stp1_6);
    stp2_7 = _mm_sub_epi16(stp1_0, stp2_7);
  }

  // Stage7. Left 8x16 only.
  l0 = _mm_add_epi16(stp2_0, stp1_15);
  l1 = _mm_add_epi16(stp2_1, stp1_14);
  l2 = _mm_add_epi16(stp2_2, stp2_13);
  l3 = _mm_add_epi16(stp2_3, stp2_12);
  l4 = _mm_add_epi16(stp2_4, stp2_11);
  l5 = _mm_add_epi16(stp2_5, stp2_10);
  l6 = _mm_add_epi16(stp2_6, stp1_9);
  l7 = _mm_add_epi16(stp2_7, stp1_8);
  l8 = _mm_sub_epi16(stp2_7, stp1_8);
  l9 = _mm_sub_epi16(stp2_6, stp1_9);
  l10 = _mm_sub_epi16(stp2_5, stp2_10);
  l11 = _mm_sub_epi16(stp2_4, stp2_11);
  l12 = _mm_sub_epi16(stp2_3, stp2_12);
  l13 = _mm_sub_epi16(stp2_2, stp2_13);
  l14 = _mm_sub_epi16(stp2_1, stp1_14);
  l15 = _mm_sub_epi16(stp2_0, stp1_15);

  // 2-D idct. We do 2 8x16 blocks.
  for (i = 0; i < 2; i++) {
    if (i == 0)
      TRANSPOSE_4X8(l0, l1, l2, l3, l4, l5, l6, l7, in0, in1, in2, in3, in4,
                    in5, in6, in7);

    if (i == 1)
      TRANSPOSE_4X8(l8, l9, l10, l11, l12, l13, l14, l15, in0, in1, in2, in3,
                    in4, in5, in6, in7);

    in8 = in9 = in10 = in11 = in12 = in13 = in14 = in15 = zero;

    IDCT16x16_1D

    // Stage7
    in0 = _mm_add_epi16(stp2_0, stp1_15);
    in1 = _mm_add_epi16(stp2_1, stp1_14);
    in2 = _mm_add_epi16(stp2_2, stp2_13);
    in3 = _mm_add_epi16(stp2_3, stp2_12);
    in4 = _mm_add_epi16(stp2_4, stp2_11);
    in5 = _mm_add_epi16(stp2_5, stp2_10);
    in6 = _mm_add_epi16(stp2_6, stp1_9);
    in7 = _mm_add_epi16(stp2_7, stp1_8);
    in8 = _mm_sub_epi16(stp2_7, stp1_8);
    in9 = _mm_sub_epi16(stp2_6, stp1_9);
    in10 = _mm_sub_epi16(stp2_5, stp2_10);
    in11 = _mm_sub_epi16(stp2_4, stp2_11);
    in12 = _mm_sub_epi16(stp2_3, stp2_12);
    in13 = _mm_sub_epi16(stp2_2, stp2_13);
    in14 = _mm_sub_epi16(stp2_1, stp1_14);
    in15 = _mm_sub_epi16(stp2_0, stp1_15);

    // Final rounding and shift
    in0 = _mm_adds_epi16(in0, final_rounding);
    in1 = _mm_adds_epi16(in1, final_rounding);
    in2 = _mm_adds_epi16(in2, final_rounding);
    in3 = _mm_adds_epi16(in3, final_rounding);
    in4 = _mm_adds_epi16(in4, final_rounding);
    in5 = _mm_adds_epi16(in5, final_rounding);
    in6 = _mm_adds_epi16(in6, final_rounding);
    in7 = _mm_adds_epi16(in7, final_rounding);
    in8 = _mm_adds_epi16(in8, final_rounding);
    in9 = _mm_adds_epi16(in9, final_rounding);
    in10 = _mm_adds_epi16(in10, final_rounding);
    in11 = _mm_adds_epi16(in11, final_rounding);
    in12 = _mm_adds_epi16(in12, final_rounding);
    in13 = _mm_adds_epi16(in13, final_rounding);
    in14 = _mm_adds_epi16(in14, final_rounding);
    in15 = _mm_adds_epi16(in15, final_rounding);

    in0 = _mm_srai_epi16(in0, 6);
    in1 = _mm_srai_epi16(in1, 6);
    in2 = _mm_srai_epi16(in2, 6);
    in3 = _mm_srai_epi16(in3, 6);
    in4 = _mm_srai_epi16(in4, 6);
    in5 = _mm_srai_epi16(in5, 6);
    in6 = _mm_srai_epi16(in6, 6);
    in7 = _mm_srai_epi16(in7, 6);
    in8 = _mm_srai_epi16(in8, 6);
    in9 = _mm_srai_epi16(in9, 6);
    in10 = _mm_srai_epi16(in10, 6);
    in11 = _mm_srai_epi16(in11, 6);
    in12 = _mm_srai_epi16(in12, 6);
    in13 = _mm_srai_epi16(in13, 6);
    in14 = _mm_srai_epi16(in14, 6);
    in15 = _mm_srai_epi16(in15, 6);

    // Store results
    _mm_store_si128((__m128i *)output, in0);
    _mm_store_si128((__m128i *)(output + half_pitch * 1), in1);
    _mm_store_si128((__m128i *)(output + half_pitch * 2), in2);
    _mm_store_si128((__m128i *)(output + half_pitch * 3), in3);
    _mm_store_si128((__m128i *)(output + half_pitch * 4), in4);
    _mm_store_si128((__m128i *)(output + half_pitch * 5), in5);
    _mm_store_si128((__m128i *)(output + half_pitch * 6), in6);
    _mm_store_si128((__m128i *)(output + half_pitch * 7), in7);
    _mm_store_si128((__m128i *)(output + half_pitch * 8), in8);
    _mm_store_si128((__m128i *)(output + half_pitch * 9), in9);
    _mm_store_si128((__m128i *)(output + half_pitch * 10), in10);
    _mm_store_si128((__m128i *)(output + half_pitch * 11), in11);
    _mm_store_si128((__m128i *)(output + half_pitch * 12), in12);
    _mm_store_si128((__m128i *)(output + half_pitch * 13), in13);
    _mm_store_si128((__m128i *)(output + half_pitch * 14), in14);
    _mm_store_si128((__m128i *)(output + half_pitch * 15), in15);
    output += 8;
  }
}

void vp9_short_idct32x32_sse2(int16_t *input, int16_t *output, int pitch) {
  const int half_pitch = pitch >> 1;
  const __m128i rounding = _mm_set1_epi32(DCT_CONST_ROUNDING);
  const __m128i final_rounding = _mm_set1_epi16(1<<5);

  // idct constants for each stage
  const __m128i stg1_0 = pair_set_epi16(cospi_31_64, -cospi_1_64);
  const __m128i stg1_1 = pair_set_epi16(cospi_1_64, cospi_31_64);
  const __m128i stg1_2 = pair_set_epi16(cospi_15_64, -cospi_17_64);
  const __m128i stg1_3 = pair_set_epi16(cospi_17_64, cospi_15_64);
  const __m128i stg1_4 = pair_set_epi16(cospi_23_64, -cospi_9_64);
  const __m128i stg1_5 = pair_set_epi16(cospi_9_64, cospi_23_64);
  const __m128i stg1_6 = pair_set_epi16(cospi_7_64, -cospi_25_64);
  const __m128i stg1_7 = pair_set_epi16(cospi_25_64, cospi_7_64);
  const __m128i stg1_8 = pair_set_epi16(cospi_27_64, -cospi_5_64);
  const __m128i stg1_9 = pair_set_epi16(cospi_5_64, cospi_27_64);
  const __m128i stg1_10 = pair_set_epi16(cospi_11_64, -cospi_21_64);
  const __m128i stg1_11 = pair_set_epi16(cospi_21_64, cospi_11_64);
  const __m128i stg1_12 = pair_set_epi16(cospi_19_64, -cospi_13_64);
  const __m128i stg1_13 = pair_set_epi16(cospi_13_64, cospi_19_64);
  const __m128i stg1_14 = pair_set_epi16(cospi_3_64, -cospi_29_64);
  const __m128i stg1_15 = pair_set_epi16(cospi_29_64, cospi_3_64);

  const __m128i stg2_0 = pair_set_epi16(cospi_30_64, -cospi_2_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_2_64, cospi_30_64);
  const __m128i stg2_2 = pair_set_epi16(cospi_14_64, -cospi_18_64);
  const __m128i stg2_3 = pair_set_epi16(cospi_18_64, cospi_14_64);
  const __m128i stg2_4 = pair_set_epi16(cospi_22_64, -cospi_10_64);
  const __m128i stg2_5 = pair_set_epi16(cospi_10_64, cospi_22_64);
  const __m128i stg2_6 = pair_set_epi16(cospi_6_64, -cospi_26_64);
  const __m128i stg2_7 = pair_set_epi16(cospi_26_64, cospi_6_64);

  const __m128i stg3_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg3_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg3_2 = pair_set_epi16(cospi_12_64, -cospi_20_64);
  const __m128i stg3_3 = pair_set_epi16(cospi_20_64, cospi_12_64);
  const __m128i stg3_4 = pair_set_epi16(-cospi_4_64, cospi_28_64);
  const __m128i stg3_5 = pair_set_epi16(cospi_28_64, cospi_4_64);
  const __m128i stg3_6 = pair_set_epi16(-cospi_28_64, -cospi_4_64);
  const __m128i stg3_8 = pair_set_epi16(-cospi_20_64, cospi_12_64);
  const __m128i stg3_9 = pair_set_epi16(cospi_12_64, cospi_20_64);
  const __m128i stg3_10 = pair_set_epi16(-cospi_12_64, -cospi_20_64);

  const __m128i stg4_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg4_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg4_2 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i stg4_3 = pair_set_epi16(cospi_8_64, cospi_24_64);
  const __m128i stg4_4 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i stg4_5 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i stg4_6 = pair_set_epi16(-cospi_24_64, -cospi_8_64);

  const __m128i stg6_0 = pair_set_epi16(-cospi_16_64, cospi_16_64);

  __m128i in0, in1, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12,
          in13, in14, in15, in16, in17, in18, in19, in20, in21, in22, in23,
          in24, in25, in26, in27, in28, in29, in30, in31;
  __m128i col[128];
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7,
          stp1_8, stp1_9, stp1_10, stp1_11, stp1_12, stp1_13, stp1_14, stp1_15,
          stp1_16, stp1_17, stp1_18, stp1_19, stp1_20, stp1_21, stp1_22,
          stp1_23, stp1_24, stp1_25, stp1_26, stp1_27, stp1_28, stp1_29,
          stp1_30, stp1_31;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7,
          stp2_8, stp2_9, stp2_10, stp2_11, stp2_12, stp2_13, stp2_14, stp2_15,
          stp2_16, stp2_17, stp2_18, stp2_19, stp2_20, stp2_21, stp2_22,
          stp2_23, stp2_24, stp2_25, stp2_26, stp2_27, stp2_28, stp2_29,
          stp2_30, stp2_31;
  __m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  int i, j;

  // We work on a 8x32 block each time, and loop 8 times for 2-D 32x32 idct.
  for (i = 0; i < 8; i++) {
    if (i < 4) {
      // First 1-D idct
      // Load input data.
      in0 = _mm_load_si128((__m128i *)input);
      in8 = _mm_load_si128((__m128i *)(input + 8 * 1));
      in16 = _mm_load_si128((__m128i *)(input + 8 * 2));
      in24 = _mm_load_si128((__m128i *)(input + 8 * 3));
      in1 = _mm_load_si128((__m128i *)(input + 8 * 4));
      in9 = _mm_load_si128((__m128i *)(input + 8 * 5));
      in17 = _mm_load_si128((__m128i *)(input + 8 * 6));
      in25 = _mm_load_si128((__m128i *)(input + 8 * 7));
      in2 = _mm_load_si128((__m128i *)(input + 8 * 8));
      in10 = _mm_load_si128((__m128i *)(input + 8 * 9));
      in18 = _mm_load_si128((__m128i *)(input + 8 * 10));
      in26 = _mm_load_si128((__m128i *)(input + 8 * 11));
      in3 = _mm_load_si128((__m128i *)(input + 8 * 12));
      in11 = _mm_load_si128((__m128i *)(input + 8 * 13));
      in19 = _mm_load_si128((__m128i *)(input + 8 * 14));
      in27 = _mm_load_si128((__m128i *)(input + 8 * 15));

      in4 = _mm_load_si128((__m128i *)(input + 8 * 16));
      in12 = _mm_load_si128((__m128i *)(input + 8 * 17));
      in20 = _mm_load_si128((__m128i *)(input + 8 * 18));
      in28 = _mm_load_si128((__m128i *)(input + 8 * 19));
      in5 = _mm_load_si128((__m128i *)(input + 8 * 20));
      in13 = _mm_load_si128((__m128i *)(input + 8 * 21));
      in21 = _mm_load_si128((__m128i *)(input + 8 * 22));
      in29 = _mm_load_si128((__m128i *)(input + 8 * 23));
      in6 = _mm_load_si128((__m128i *)(input + 8 * 24));
      in14 = _mm_load_si128((__m128i *)(input + 8 * 25));
      in22 = _mm_load_si128((__m128i *)(input + 8 * 26));
      in30 = _mm_load_si128((__m128i *)(input + 8 * 27));
      in7 = _mm_load_si128((__m128i *)(input + 8 * 28));
      in15 = _mm_load_si128((__m128i *)(input + 8 * 29));
      in23 = _mm_load_si128((__m128i *)(input + 8 * 30));
      in31 = _mm_load_si128((__m128i *)(input + 8 * 31));

      input += 256;

      // Transpose 32x8 block to 8x32 block
      TRANSPOSE_8X8(in0, in1, in2, in3, in4, in5, in6, in7, in0, in1, in2, in3,
                    in4, in5, in6, in7);
      TRANSPOSE_8X8(in8, in9, in10, in11, in12, in13, in14, in15, in8, in9,
                    in10, in11, in12, in13, in14, in15);
      TRANSPOSE_8X8(in16, in17, in18, in19, in20, in21, in22, in23, in16, in17,
                    in18, in19, in20, in21, in22, in23);
      TRANSPOSE_8X8(in24, in25, in26, in27, in28, in29, in30, in31, in24, in25,
                    in26, in27, in28, in29, in30, in31);
    } else {
      // Second 1-D idct
      j = i - 4;

      // Transpose 32x8 block to 8x32 block
      TRANSPOSE_8X8(col[j * 8 + 0], col[j * 8 + 1], col[j * 8 + 2],
                    col[j * 8 + 3], col[j * 8 + 4], col[j * 8 + 5],
                    col[j * 8 + 6], col[j * 8 + 7], in0, in1, in2, in3, in4,
                    in5, in6, in7);
      j += 4;
      TRANSPOSE_8X8(col[j * 8 + 0], col[j * 8 + 1], col[j * 8 + 2],
                    col[j * 8 + 3], col[j * 8 + 4], col[j * 8 + 5],
                    col[j * 8 + 6], col[j * 8 + 7], in8, in9, in10,
                    in11, in12, in13, in14, in15);
      j += 4;
      TRANSPOSE_8X8(col[j * 8 + 0], col[j * 8 + 1], col[j * 8 + 2],
                    col[j * 8 + 3], col[j * 8 + 4], col[j * 8 + 5],
                    col[j * 8 + 6], col[j * 8 + 7], in16, in17, in18,
                    in19, in20, in21, in22, in23);
      j += 4;
      TRANSPOSE_8X8(col[j * 8 + 0], col[j * 8 + 1], col[j * 8 + 2],
                    col[j * 8 + 3], col[j * 8 + 4], col[j * 8 + 5],
                    col[j * 8 + 6], col[j * 8 + 7], in24, in25, in26, in27,
                    in28, in29, in30, in31);
    }

    // Stage1
    {
      const __m128i lo_1_31 = _mm_unpacklo_epi16(in1, in31);
      const __m128i hi_1_31 = _mm_unpackhi_epi16(in1, in31);
      const __m128i lo_17_15 = _mm_unpacklo_epi16(in17, in15);
      const __m128i hi_17_15 = _mm_unpackhi_epi16(in17, in15);

      const __m128i lo_9_23 = _mm_unpacklo_epi16(in9, in23);
      const __m128i hi_9_23 = _mm_unpackhi_epi16(in9, in23);
      const __m128i lo_25_7= _mm_unpacklo_epi16(in25, in7);
      const __m128i hi_25_7 = _mm_unpackhi_epi16(in25, in7);

      const __m128i lo_5_27 = _mm_unpacklo_epi16(in5, in27);
      const __m128i hi_5_27 = _mm_unpackhi_epi16(in5, in27);
      const __m128i lo_21_11 = _mm_unpacklo_epi16(in21, in11);
      const __m128i hi_21_11 = _mm_unpackhi_epi16(in21, in11);

      const __m128i lo_13_19 = _mm_unpacklo_epi16(in13, in19);
      const __m128i hi_13_19 = _mm_unpackhi_epi16(in13, in19);
      const __m128i lo_29_3 = _mm_unpacklo_epi16(in29, in3);
      const __m128i hi_29_3 = _mm_unpackhi_epi16(in29, in3);

      MULTIPLICATION_AND_ADD(lo_1_31, hi_1_31, lo_17_15, hi_17_15, stg1_0,
                             stg1_1, stg1_2, stg1_3, stp1_16, stp1_31,
                             stp1_17, stp1_30)
      MULTIPLICATION_AND_ADD(lo_9_23, hi_9_23, lo_25_7, hi_25_7, stg1_4,
                             stg1_5, stg1_6, stg1_7, stp1_18, stp1_29,
                             stp1_19, stp1_28)
      MULTIPLICATION_AND_ADD(lo_5_27, hi_5_27, lo_21_11, hi_21_11, stg1_8,
                             stg1_9, stg1_10, stg1_11, stp1_20, stp1_27,
                             stp1_21, stp1_26)
      MULTIPLICATION_AND_ADD(lo_13_19, hi_13_19, lo_29_3, hi_29_3, stg1_12,
                             stg1_13, stg1_14, stg1_15, stp1_22, stp1_25,
                             stp1_23, stp1_24)
    }

    // Stage2
    {
      const __m128i lo_2_30 = _mm_unpacklo_epi16(in2, in30);
      const __m128i hi_2_30 = _mm_unpackhi_epi16(in2, in30);
      const __m128i lo_18_14 = _mm_unpacklo_epi16(in18, in14);
      const __m128i hi_18_14 = _mm_unpackhi_epi16(in18, in14);

      const __m128i lo_10_22 = _mm_unpacklo_epi16(in10, in22);
      const __m128i hi_10_22 = _mm_unpackhi_epi16(in10, in22);
      const __m128i lo_26_6 = _mm_unpacklo_epi16(in26, in6);
      const __m128i hi_26_6 = _mm_unpackhi_epi16(in26, in6);

      MULTIPLICATION_AND_ADD(lo_2_30, hi_2_30, lo_18_14, hi_18_14, stg2_0,
                             stg2_1, stg2_2, stg2_3, stp2_8, stp2_15, stp2_9,
                             stp2_14)
      MULTIPLICATION_AND_ADD(lo_10_22, hi_10_22, lo_26_6, hi_26_6, stg2_4,
                             stg2_5, stg2_6, stg2_7, stp2_10, stp2_13,
                             stp2_11, stp2_12)

      stp2_16 = _mm_add_epi16(stp1_16, stp1_17);
      stp2_17 = _mm_sub_epi16(stp1_16, stp1_17);
      stp2_18 = _mm_sub_epi16(stp1_19, stp1_18);
      stp2_19 = _mm_add_epi16(stp1_19, stp1_18);

      stp2_20 = _mm_add_epi16(stp1_20, stp1_21);
      stp2_21 = _mm_sub_epi16(stp1_20, stp1_21);
      stp2_22 = _mm_sub_epi16(stp1_23, stp1_22);
      stp2_23 = _mm_add_epi16(stp1_23, stp1_22);

      stp2_24 = _mm_add_epi16(stp1_24, stp1_25);
      stp2_25 = _mm_sub_epi16(stp1_24, stp1_25);
      stp2_26 = _mm_sub_epi16(stp1_27, stp1_26);
      stp2_27 = _mm_add_epi16(stp1_27, stp1_26);

      stp2_28 = _mm_add_epi16(stp1_28, stp1_29);
      stp2_29 = _mm_sub_epi16(stp1_28, stp1_29);
      stp2_30 = _mm_sub_epi16(stp1_31, stp1_30);
      stp2_31 = _mm_add_epi16(stp1_31, stp1_30);
    }

    // Stage3
    {
      const __m128i lo_4_28 = _mm_unpacklo_epi16(in4, in28);
      const __m128i hi_4_28 = _mm_unpackhi_epi16(in4, in28);
      const __m128i lo_20_12 = _mm_unpacklo_epi16(in20, in12);
      const __m128i hi_20_12 = _mm_unpackhi_epi16(in20, in12);

      const __m128i lo_17_30 = _mm_unpacklo_epi16(stp2_17, stp2_30);
      const __m128i hi_17_30 = _mm_unpackhi_epi16(stp2_17, stp2_30);
      const __m128i lo_18_29 = _mm_unpacklo_epi16(stp2_18, stp2_29);
      const __m128i hi_18_29 = _mm_unpackhi_epi16(stp2_18, stp2_29);

      const __m128i lo_21_26 = _mm_unpacklo_epi16(stp2_21, stp2_26);
      const __m128i hi_21_26 = _mm_unpackhi_epi16(stp2_21, stp2_26);
      const __m128i lo_22_25 = _mm_unpacklo_epi16(stp2_22, stp2_25);
      const __m128i hi_22_25 = _mm_unpackhi_epi16(stp2_22, stp2_25);

      MULTIPLICATION_AND_ADD(lo_4_28, hi_4_28, lo_20_12, hi_20_12, stg3_0,
                             stg3_1, stg3_2, stg3_3, stp1_4, stp1_7, stp1_5,
                             stp1_6)

      stp1_8 = _mm_add_epi16(stp2_8, stp2_9);
      stp1_9 = _mm_sub_epi16(stp2_8, stp2_9);
      stp1_10 = _mm_sub_epi16(stp2_11, stp2_10);
      stp1_11 = _mm_add_epi16(stp2_11, stp2_10);
      stp1_12 = _mm_add_epi16(stp2_12, stp2_13);
      stp1_13 = _mm_sub_epi16(stp2_12, stp2_13);
      stp1_14 = _mm_sub_epi16(stp2_15, stp2_14);
      stp1_15 = _mm_add_epi16(stp2_15, stp2_14);

      MULTIPLICATION_AND_ADD(lo_17_30, hi_17_30, lo_18_29, hi_18_29, stg3_4,
                             stg3_5, stg3_6, stg3_4, stp1_17, stp1_30,
                             stp1_18, stp1_29)
      MULTIPLICATION_AND_ADD(lo_21_26, hi_21_26, lo_22_25, hi_22_25, stg3_8,
                             stg3_9, stg3_10, stg3_8, stp1_21, stp1_26,
                             stp1_22, stp1_25)

      stp1_16 = stp2_16;
      stp1_31 = stp2_31;
      stp1_19 = stp2_19;
      stp1_20 = stp2_20;
      stp1_23 = stp2_23;
      stp1_24 = stp2_24;
      stp1_27 = stp2_27;
      stp1_28 = stp2_28;
    }

    // Stage4
    {
      const __m128i lo_0_16 = _mm_unpacklo_epi16(in0, in16);
      const __m128i hi_0_16 = _mm_unpackhi_epi16(in0, in16);
      const __m128i lo_8_24 = _mm_unpacklo_epi16(in8, in24);
      const __m128i hi_8_24 = _mm_unpackhi_epi16(in8, in24);

      const __m128i lo_9_14 = _mm_unpacklo_epi16(stp1_9, stp1_14);
      const __m128i hi_9_14 = _mm_unpackhi_epi16(stp1_9, stp1_14);
      const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);
      const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13);

      MULTIPLICATION_AND_ADD(lo_0_16, hi_0_16, lo_8_24, hi_8_24, stg4_0,
                             stg4_1, stg4_2, stg4_3, stp2_0, stp2_1,
                             stp2_2, stp2_3)

      stp2_4 = _mm_add_epi16(stp1_4, stp1_5);
      stp2_5 = _mm_sub_epi16(stp1_4, stp1_5);
      stp2_6 = _mm_sub_epi16(stp1_7, stp1_6);
      stp2_7 = _mm_add_epi16(stp1_7, stp1_6);

      MULTIPLICATION_AND_ADD(lo_9_14, hi_9_14, lo_10_13, hi_10_13, stg4_4,
                             stg4_5, stg4_6, stg4_4, stp2_9, stp2_14,
                             stp2_10, stp2_13)

      stp2_8 = stp1_8;
      stp2_15 = stp1_15;
      stp2_11 = stp1_11;
      stp2_12 = stp1_12;

      stp2_16 = _mm_add_epi16(stp1_16, stp1_19);
      stp2_17 = _mm_add_epi16(stp1_17, stp1_18);
      stp2_18 = _mm_sub_epi16(stp1_17, stp1_18);
      stp2_19 = _mm_sub_epi16(stp1_16, stp1_19);
      stp2_20 = _mm_sub_epi16(stp1_23, stp1_20);
      stp2_21 = _mm_sub_epi16(stp1_22, stp1_21);
      stp2_22 = _mm_add_epi16(stp1_22, stp1_21);
      stp2_23 = _mm_add_epi16(stp1_23, stp1_20);

      stp2_24 = _mm_add_epi16(stp1_24, stp1_27);
      stp2_25 = _mm_add_epi16(stp1_25, stp1_26);
      stp2_26 = _mm_sub_epi16(stp1_25, stp1_26);
      stp2_27 = _mm_sub_epi16(stp1_24, stp1_27);
      stp2_28 = _mm_sub_epi16(stp1_31, stp1_28);
      stp2_29 = _mm_sub_epi16(stp1_30, stp1_29);
      stp2_30 = _mm_add_epi16(stp1_29, stp1_30);
      stp2_31 = _mm_add_epi16(stp1_28, stp1_31);
    }

    // Stage5
    {
      const __m128i lo_6_5 = _mm_unpacklo_epi16(stp2_6, stp2_5);
      const __m128i hi_6_5 = _mm_unpackhi_epi16(stp2_6, stp2_5);
      const __m128i lo_18_29 = _mm_unpacklo_epi16(stp2_18, stp2_29);
      const __m128i hi_18_29 = _mm_unpackhi_epi16(stp2_18, stp2_29);

      const __m128i lo_19_28 = _mm_unpacklo_epi16(stp2_19, stp2_28);
      const __m128i hi_19_28 = _mm_unpackhi_epi16(stp2_19, stp2_28);
      const __m128i lo_20_27 = _mm_unpacklo_epi16(stp2_20, stp2_27);
      const __m128i hi_20_27 = _mm_unpackhi_epi16(stp2_20, stp2_27);

      const __m128i lo_21_26 = _mm_unpacklo_epi16(stp2_21, stp2_26);
      const __m128i hi_21_26 = _mm_unpackhi_epi16(stp2_21, stp2_26);

      stp1_0 = _mm_add_epi16(stp2_0, stp2_3);
      stp1_1 = _mm_add_epi16(stp2_1, stp2_2);
      stp1_2 = _mm_sub_epi16(stp2_1, stp2_2);
      stp1_3 = _mm_sub_epi16(stp2_0, stp2_3);

      tmp0 = _mm_madd_epi16(lo_6_5, stg4_1);
      tmp1 = _mm_madd_epi16(hi_6_5, stg4_1);
      tmp2 = _mm_madd_epi16(lo_6_5, stg4_0);
      tmp3 = _mm_madd_epi16(hi_6_5, stg4_0);

      tmp0 = _mm_add_epi32(tmp0, rounding);
      tmp1 = _mm_add_epi32(tmp1, rounding);
      tmp2 = _mm_add_epi32(tmp2, rounding);
      tmp3 = _mm_add_epi32(tmp3, rounding);

      tmp0 = _mm_srai_epi32(tmp0, DCT_CONST_BITS);
      tmp1 = _mm_srai_epi32(tmp1, DCT_CONST_BITS);
      tmp2 = _mm_srai_epi32(tmp2, DCT_CONST_BITS);
      tmp3 = _mm_srai_epi32(tmp3, DCT_CONST_BITS);

      stp1_5 = _mm_packs_epi32(tmp0, tmp1);
      stp1_6 = _mm_packs_epi32(tmp2, tmp3);

      stp1_4 = stp2_4;
      stp1_7 = stp2_7;

      stp1_8 = _mm_add_epi16(stp2_8, stp2_11);
      stp1_9 = _mm_add_epi16(stp2_9, stp2_10);
      stp1_10 = _mm_sub_epi16(stp2_9, stp2_10);
      stp1_11 = _mm_sub_epi16(stp2_8, stp2_11);
      stp1_12 = _mm_sub_epi16(stp2_15, stp2_12);
      stp1_13 = _mm_sub_epi16(stp2_14, stp2_13);
      stp1_14 = _mm_add_epi16(stp2_14, stp2_13);
      stp1_15 = _mm_add_epi16(stp2_15, stp2_12);

      stp1_16 = stp2_16;
      stp1_17 = stp2_17;

      MULTIPLICATION_AND_ADD(lo_18_29, hi_18_29, lo_19_28, hi_19_28, stg4_4,
                             stg4_5, stg4_4, stg4_5, stp1_18, stp1_29,
                             stp1_19, stp1_28)
      MULTIPLICATION_AND_ADD(lo_20_27, hi_20_27, lo_21_26, hi_21_26, stg4_6,
                             stg4_4, stg4_6, stg4_4, stp1_20, stp1_27,
                             stp1_21, stp1_26)

      stp1_22 = stp2_22;
      stp1_23 = stp2_23;
      stp1_24 = stp2_24;
      stp1_25 = stp2_25;
      stp1_30 = stp2_30;
      stp1_31 = stp2_31;
    }

    // Stage6
    {
      const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);
      const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13);
      const __m128i lo_11_12 = _mm_unpacklo_epi16(stp1_11, stp1_12);
      const __m128i hi_11_12 = _mm_unpackhi_epi16(stp1_11, stp1_12);

      stp2_0 = _mm_add_epi16(stp1_0, stp1_7);
      stp2_1 = _mm_add_epi16(stp1_1, stp1_6);
      stp2_2 = _mm_add_epi16(stp1_2, stp1_5);
      stp2_3 = _mm_add_epi16(stp1_3, stp1_4);
      stp2_4 = _mm_sub_epi16(stp1_3, stp1_4);
      stp2_5 = _mm_sub_epi16(stp1_2, stp1_5);
      stp2_6 = _mm_sub_epi16(stp1_1, stp1_6);
      stp2_7 = _mm_sub_epi16(stp1_0, stp1_7);

      stp2_8 = stp1_8;
      stp2_9 = stp1_9;
      stp2_14 = stp1_14;
      stp2_15 = stp1_15;

      MULTIPLICATION_AND_ADD(lo_10_13, hi_10_13, lo_11_12, hi_11_12,
                             stg6_0, stg4_0, stg6_0, stg4_0, stp2_10,
                             stp2_13, stp2_11, stp2_12)

      stp2_16 = _mm_add_epi16(stp1_16, stp1_23);
      stp2_17 = _mm_add_epi16(stp1_17, stp1_22);
      stp2_18 = _mm_add_epi16(stp1_18, stp1_21);
      stp2_19 = _mm_add_epi16(stp1_19, stp1_20);
      stp2_20 = _mm_sub_epi16(stp1_19, stp1_20);
      stp2_21 = _mm_sub_epi16(stp1_18, stp1_21);
      stp2_22 = _mm_sub_epi16(stp1_17, stp1_22);
      stp2_23 = _mm_sub_epi16(stp1_16, stp1_23);

      stp2_24 = _mm_sub_epi16(stp1_31, stp1_24);
      stp2_25 = _mm_sub_epi16(stp1_30, stp1_25);
      stp2_26 = _mm_sub_epi16(stp1_29, stp1_26);
      stp2_27 = _mm_sub_epi16(stp1_28, stp1_27);
      stp2_28 = _mm_add_epi16(stp1_27, stp1_28);
      stp2_29 = _mm_add_epi16(stp1_26, stp1_29);
      stp2_30 = _mm_add_epi16(stp1_25, stp1_30);
      stp2_31 = _mm_add_epi16(stp1_24, stp1_31);
    }

    // Stage7
    {
      const __m128i lo_20_27 = _mm_unpacklo_epi16(stp2_20, stp2_27);
      const __m128i hi_20_27 = _mm_unpackhi_epi16(stp2_20, stp2_27);
      const __m128i lo_21_26 = _mm_unpacklo_epi16(stp2_21, stp2_26);
      const __m128i hi_21_26 = _mm_unpackhi_epi16(stp2_21, stp2_26);

      const __m128i lo_22_25 = _mm_unpacklo_epi16(stp2_22, stp2_25);
      const __m128i hi_22_25 = _mm_unpackhi_epi16(stp2_22, stp2_25);
      const __m128i lo_23_24 = _mm_unpacklo_epi16(stp2_23, stp2_24);
      const __m128i hi_23_24 = _mm_unpackhi_epi16(stp2_23, stp2_24);

      stp1_0 = _mm_add_epi16(stp2_0, stp2_15);
      stp1_1 = _mm_add_epi16(stp2_1, stp2_14);
      stp1_2 = _mm_add_epi16(stp2_2, stp2_13);
      stp1_3 = _mm_add_epi16(stp2_3, stp2_12);
      stp1_4 = _mm_add_epi16(stp2_4, stp2_11);
      stp1_5 = _mm_add_epi16(stp2_5, stp2_10);
      stp1_6 = _mm_add_epi16(stp2_6, stp2_9);
      stp1_7 = _mm_add_epi16(stp2_7, stp2_8);
      stp1_8 = _mm_sub_epi16(stp2_7, stp2_8);
      stp1_9 = _mm_sub_epi16(stp2_6, stp2_9);
      stp1_10 = _mm_sub_epi16(stp2_5, stp2_10);
      stp1_11 = _mm_sub_epi16(stp2_4, stp2_11);
      stp1_12 = _mm_sub_epi16(stp2_3, stp2_12);
      stp1_13 = _mm_sub_epi16(stp2_2, stp2_13);
      stp1_14 = _mm_sub_epi16(stp2_1, stp2_14);
      stp1_15 = _mm_sub_epi16(stp2_0, stp2_15);

      stp1_16 = stp2_16;
      stp1_17 = stp2_17;
      stp1_18 = stp2_18;
      stp1_19 = stp2_19;

      MULTIPLICATION_AND_ADD(lo_20_27, hi_20_27, lo_21_26, hi_21_26, stg6_0,
                             stg4_0, stg6_0, stg4_0, stp1_20, stp1_27,
                             stp1_21, stp1_26)
      MULTIPLICATION_AND_ADD(lo_22_25, hi_22_25, lo_23_24, hi_23_24, stg6_0,
                             stg4_0, stg6_0, stg4_0, stp1_22, stp1_25,
                             stp1_23, stp1_24)

      stp1_28 = stp2_28;
      stp1_29 = stp2_29;
      stp1_30 = stp2_30;
      stp1_31 = stp2_31;
    }

    // final stage
    if (i < 4) {
      // 1_D: Store 32 intermediate results for each 8x32 block.
      col[i * 32 + 0] = _mm_add_epi16(stp1_0, stp1_31);
      col[i * 32 + 1] = _mm_add_epi16(stp1_1, stp1_30);
      col[i * 32 + 2] = _mm_add_epi16(stp1_2, stp1_29);
      col[i * 32 + 3] = _mm_add_epi16(stp1_3, stp1_28);
      col[i * 32 + 4] = _mm_add_epi16(stp1_4, stp1_27);
      col[i * 32 + 5] = _mm_add_epi16(stp1_5, stp1_26);
      col[i * 32 + 6] = _mm_add_epi16(stp1_6, stp1_25);
      col[i * 32 + 7] = _mm_add_epi16(stp1_7, stp1_24);
      col[i * 32 + 8] = _mm_add_epi16(stp1_8, stp1_23);
      col[i * 32 + 9] = _mm_add_epi16(stp1_9, stp1_22);
      col[i * 32 + 10] = _mm_add_epi16(stp1_10, stp1_21);
      col[i * 32 + 11] = _mm_add_epi16(stp1_11, stp1_20);
      col[i * 32 + 12] = _mm_add_epi16(stp1_12, stp1_19);
      col[i * 32 + 13] = _mm_add_epi16(stp1_13, stp1_18);
      col[i * 32 + 14] = _mm_add_epi16(stp1_14, stp1_17);
      col[i * 32 + 15] = _mm_add_epi16(stp1_15, stp1_16);
      col[i * 32 + 16] = _mm_sub_epi16(stp1_15, stp1_16);
      col[i * 32 + 17] = _mm_sub_epi16(stp1_14, stp1_17);
      col[i * 32 + 18] = _mm_sub_epi16(stp1_13, stp1_18);
      col[i * 32 + 19] = _mm_sub_epi16(stp1_12, stp1_19);
      col[i * 32 + 20] = _mm_sub_epi16(stp1_11, stp1_20);
      col[i * 32 + 21] = _mm_sub_epi16(stp1_10, stp1_21);
      col[i * 32 + 22] = _mm_sub_epi16(stp1_9, stp1_22);
      col[i * 32 + 23] = _mm_sub_epi16(stp1_8, stp1_23);
      col[i * 32 + 24] = _mm_sub_epi16(stp1_7, stp1_24);
      col[i * 32 + 25] = _mm_sub_epi16(stp1_6, stp1_25);
      col[i * 32 + 26] = _mm_sub_epi16(stp1_5, stp1_26);
      col[i * 32 + 27] = _mm_sub_epi16(stp1_4, stp1_27);
      col[i * 32 + 28] = _mm_sub_epi16(stp1_3, stp1_28);
      col[i * 32 + 29] = _mm_sub_epi16(stp1_2, stp1_29);
      col[i * 32 + 30] = _mm_sub_epi16(stp1_1, stp1_30);
      col[i * 32 + 31] = _mm_sub_epi16(stp1_0, stp1_31);
    } else {
      // 2_D: Calculate the results and store them to destination.
      in0 = _mm_add_epi16(stp1_0, stp1_31);
      in1 = _mm_add_epi16(stp1_1, stp1_30);
      in2 = _mm_add_epi16(stp1_2, stp1_29);
      in3 = _mm_add_epi16(stp1_3, stp1_28);
      in4 = _mm_add_epi16(stp1_4, stp1_27);
      in5 = _mm_add_epi16(stp1_5, stp1_26);
      in6 = _mm_add_epi16(stp1_6, stp1_25);
      in7 = _mm_add_epi16(stp1_7, stp1_24);
      in8 = _mm_add_epi16(stp1_8, stp1_23);
      in9 = _mm_add_epi16(stp1_9, stp1_22);
      in10 = _mm_add_epi16(stp1_10, stp1_21);
      in11 = _mm_add_epi16(stp1_11, stp1_20);
      in12 = _mm_add_epi16(stp1_12, stp1_19);
      in13 = _mm_add_epi16(stp1_13, stp1_18);
      in14 = _mm_add_epi16(stp1_14, stp1_17);
      in15 = _mm_add_epi16(stp1_15, stp1_16);
      in16 = _mm_sub_epi16(stp1_15, stp1_16);
      in17 = _mm_sub_epi16(stp1_14, stp1_17);
      in18 = _mm_sub_epi16(stp1_13, stp1_18);
      in19 = _mm_sub_epi16(stp1_12, stp1_19);
      in20 = _mm_sub_epi16(stp1_11, stp1_20);
      in21 = _mm_sub_epi16(stp1_10, stp1_21);
      in22 = _mm_sub_epi16(stp1_9, stp1_22);
      in23 = _mm_sub_epi16(stp1_8, stp1_23);
      in24 = _mm_sub_epi16(stp1_7, stp1_24);
      in25 = _mm_sub_epi16(stp1_6, stp1_25);
      in26 = _mm_sub_epi16(stp1_5, stp1_26);
      in27 = _mm_sub_epi16(stp1_4, stp1_27);
      in28 = _mm_sub_epi16(stp1_3, stp1_28);
      in29 = _mm_sub_epi16(stp1_2, stp1_29);
      in30 = _mm_sub_epi16(stp1_1, stp1_30);
      in31 = _mm_sub_epi16(stp1_0, stp1_31);

      // Final rounding and shift
      in0 = _mm_adds_epi16(in0, final_rounding);
      in1 = _mm_adds_epi16(in1, final_rounding);
      in2 = _mm_adds_epi16(in2, final_rounding);
      in3 = _mm_adds_epi16(in3, final_rounding);
      in4 = _mm_adds_epi16(in4, final_rounding);
      in5 = _mm_adds_epi16(in5, final_rounding);
      in6 = _mm_adds_epi16(in6, final_rounding);
      in7 = _mm_adds_epi16(in7, final_rounding);
      in8 = _mm_adds_epi16(in8, final_rounding);
      in9 = _mm_adds_epi16(in9, final_rounding);
      in10 = _mm_adds_epi16(in10, final_rounding);
      in11 = _mm_adds_epi16(in11, final_rounding);
      in12 = _mm_adds_epi16(in12, final_rounding);
      in13 = _mm_adds_epi16(in13, final_rounding);
      in14 = _mm_adds_epi16(in14, final_rounding);
      in15 = _mm_adds_epi16(in15, final_rounding);
      in16 = _mm_adds_epi16(in16, final_rounding);
      in17 = _mm_adds_epi16(in17, final_rounding);
      in18 = _mm_adds_epi16(in18, final_rounding);
      in19 = _mm_adds_epi16(in19, final_rounding);
      in20 = _mm_adds_epi16(in20, final_rounding);
      in21 = _mm_adds_epi16(in21, final_rounding);
      in22 = _mm_adds_epi16(in22, final_rounding);
      in23 = _mm_adds_epi16(in23, final_rounding);
      in24 = _mm_adds_epi16(in24, final_rounding);
      in25 = _mm_adds_epi16(in25, final_rounding);
      in26 = _mm_adds_epi16(in26, final_rounding);
      in27 = _mm_adds_epi16(in27, final_rounding);
      in28 = _mm_adds_epi16(in28, final_rounding);
      in29 = _mm_adds_epi16(in29, final_rounding);
      in30 = _mm_adds_epi16(in30, final_rounding);
      in31 = _mm_adds_epi16(in31, final_rounding);

      in0 = _mm_srai_epi16(in0, 6);
      in1 = _mm_srai_epi16(in1, 6);
      in2 = _mm_srai_epi16(in2, 6);
      in3 = _mm_srai_epi16(in3, 6);
      in4 = _mm_srai_epi16(in4, 6);
      in5 = _mm_srai_epi16(in5, 6);
      in6 = _mm_srai_epi16(in6, 6);
      in7 = _mm_srai_epi16(in7, 6);
      in8 = _mm_srai_epi16(in8, 6);
      in9 = _mm_srai_epi16(in9, 6);
      in10 = _mm_srai_epi16(in10, 6);
      in11 = _mm_srai_epi16(in11, 6);
      in12 = _mm_srai_epi16(in12, 6);
      in13 = _mm_srai_epi16(in13, 6);
      in14 = _mm_srai_epi16(in14, 6);
      in15 = _mm_srai_epi16(in15, 6);
      in16 = _mm_srai_epi16(in16, 6);
      in17 = _mm_srai_epi16(in17, 6);
      in18 = _mm_srai_epi16(in18, 6);
      in19 = _mm_srai_epi16(in19, 6);
      in20 = _mm_srai_epi16(in20, 6);
      in21 = _mm_srai_epi16(in21, 6);
      in22 = _mm_srai_epi16(in22, 6);
      in23 = _mm_srai_epi16(in23, 6);
      in24 = _mm_srai_epi16(in24, 6);
      in25 = _mm_srai_epi16(in25, 6);
      in26 = _mm_srai_epi16(in26, 6);
      in27 = _mm_srai_epi16(in27, 6);
      in28 = _mm_srai_epi16(in28, 6);
      in29 = _mm_srai_epi16(in29, 6);
      in30 = _mm_srai_epi16(in30, 6);
      in31 = _mm_srai_epi16(in31, 6);

      // Store results
      _mm_store_si128((__m128i *)output, in0);
      _mm_store_si128((__m128i *)(output + half_pitch * 1), in1);
      _mm_store_si128((__m128i *)(output + half_pitch * 2), in2);
      _mm_store_si128((__m128i *)(output + half_pitch * 3), in3);
      _mm_store_si128((__m128i *)(output + half_pitch * 4), in4);
      _mm_store_si128((__m128i *)(output + half_pitch * 5), in5);
      _mm_store_si128((__m128i *)(output + half_pitch * 6), in6);
      _mm_store_si128((__m128i *)(output + half_pitch * 7), in7);
      _mm_store_si128((__m128i *)(output + half_pitch * 8), in8);
      _mm_store_si128((__m128i *)(output + half_pitch * 9), in9);
      _mm_store_si128((__m128i *)(output + half_pitch * 10), in10);
      _mm_store_si128((__m128i *)(output + half_pitch * 11), in11);
      _mm_store_si128((__m128i *)(output + half_pitch * 12), in12);
      _mm_store_si128((__m128i *)(output + half_pitch * 13), in13);
      _mm_store_si128((__m128i *)(output + half_pitch * 14), in14);
      _mm_store_si128((__m128i *)(output + half_pitch * 15), in15);
      _mm_store_si128((__m128i *)(output + half_pitch * 16), in16);
      _mm_store_si128((__m128i *)(output + half_pitch * 17), in17);
      _mm_store_si128((__m128i *)(output + half_pitch * 18), in18);
      _mm_store_si128((__m128i *)(output + half_pitch * 19), in19);
      _mm_store_si128((__m128i *)(output + half_pitch * 20), in20);
      _mm_store_si128((__m128i *)(output + half_pitch * 21), in21);
      _mm_store_si128((__m128i *)(output + half_pitch * 22), in22);
      _mm_store_si128((__m128i *)(output + half_pitch * 23), in23);
      _mm_store_si128((__m128i *)(output + half_pitch * 24), in24);
      _mm_store_si128((__m128i *)(output + half_pitch * 25), in25);
      _mm_store_si128((__m128i *)(output + half_pitch * 26), in26);
      _mm_store_si128((__m128i *)(output + half_pitch * 27), in27);
      _mm_store_si128((__m128i *)(output + half_pitch * 28), in28);
      _mm_store_si128((__m128i *)(output + half_pitch * 29), in29);
      _mm_store_si128((__m128i *)(output + half_pitch * 30), in30);
      _mm_store_si128((__m128i *)(output + half_pitch * 31), in31);

      output += 8;
    }
  }
}
