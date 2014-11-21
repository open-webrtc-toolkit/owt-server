/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <emmintrin.h>  // SSE2
#include "vp9/common/vp9_idct.h"  // for cospi constants

void vp9_short_fdct4x4_sse2(int16_t *input, int16_t *output, int pitch) {
  // The 2D transform is done with two passes which are actually pretty
  // similar. In the first one, we transform the columns and transpose
  // the results. In the second one, we transform the rows. To achieve that,
  // as the first pass results are transposed, we tranpose the columns (that
  // is the transposed rows) and transpose the results (so that it goes back
  // in normal/row positions).
  const int stride = pitch >> 1;
  int pass;
  // Constants
  //    When we use them, in one case, they are all the same. In all others
  //    it's a pair of them that we need to repeat four times. This is done
  //    by constructing the 32 bit constant corresponding to that pair.
  const __m128i k__cospi_p16_p16 = _mm_set1_epi16(cospi_16_64);
  const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i k__cospi_p24_p08 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i k__cospi_m08_p24 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);
  const __m128i k__nonzero_bias_a = _mm_setr_epi16(0, 1, 1, 1, 1, 1, 1, 1);
  const __m128i k__nonzero_bias_b = _mm_setr_epi16(1, 0, 0, 0, 0, 0, 0, 0);
  const __m128i kOne = _mm_set1_epi16(1);
  __m128i in0, in1, in2, in3;
  // Load inputs.
  {
    in0  = _mm_loadl_epi64((const __m128i *)(input +  0 * stride));
    in1  = _mm_loadl_epi64((const __m128i *)(input +  1 * stride));
    in2  = _mm_loadl_epi64((const __m128i *)(input +  2 * stride));
    in3  = _mm_loadl_epi64((const __m128i *)(input +  3 * stride));
    // x = x << 4
    in0 = _mm_slli_epi16(in0, 4);
    in1 = _mm_slli_epi16(in1, 4);
    in2 = _mm_slli_epi16(in2, 4);
    in3 = _mm_slli_epi16(in3, 4);
    // if (i == 0 && input[0]) input[0] += 1;
    {
      // The mask will only contain wether the first value is zero, all
      // other comparison will fail as something shifted by 4 (above << 4)
      // can never be equal to one. To increment in the non-zero case, we
      // add the mask and one for the first element:
      //   - if zero, mask = -1, v = v - 1 + 1 = v
      //   - if non-zero, mask = 0, v = v + 0 + 1 = v + 1
      __m128i mask = _mm_cmpeq_epi16(in0, k__nonzero_bias_a);
      in0 = _mm_add_epi16(in0, mask);
      in0 = _mm_add_epi16(in0, k__nonzero_bias_b);
    }
  }
  // Do the two transform/transpose passes
  for (pass = 0; pass < 2; ++pass) {
    // Transform 1/2: Add/substract
    const __m128i r0 = _mm_add_epi16(in0, in3);
    const __m128i r1 = _mm_add_epi16(in1, in2);
    const __m128i r2 = _mm_sub_epi16(in1, in2);
    const __m128i r3 = _mm_sub_epi16(in0, in3);
    // Transform 1/2: Interleave to do the multiply by constants which gets us
    //                into 32 bits.
    const __m128i t0 = _mm_unpacklo_epi16(r0, r1);
    const __m128i t2 = _mm_unpacklo_epi16(r2, r3);
    const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p16_p16);
    const __m128i u2 = _mm_madd_epi16(t0, k__cospi_p16_m16);
    const __m128i u4 = _mm_madd_epi16(t2, k__cospi_p24_p08);
    const __m128i u6 = _mm_madd_epi16(t2, k__cospi_m08_p24);
    const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
    const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
    const __m128i v4 = _mm_add_epi32(u4, k__DCT_CONST_ROUNDING);
    const __m128i v6 = _mm_add_epi32(u6, k__DCT_CONST_ROUNDING);
    const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
    const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
    const __m128i w4 = _mm_srai_epi32(v4, DCT_CONST_BITS);
    const __m128i w6 = _mm_srai_epi32(v6, DCT_CONST_BITS);
    // Combine and transpose
    const __m128i res0 = _mm_packs_epi32(w0, w2);
    const __m128i res1 = _mm_packs_epi32(w4, w6);
    // 00 01 02 03 20 21 22 23
    // 10 11 12 13 30 31 32 33
    const __m128i tr0_0 = _mm_unpacklo_epi16(res0, res1);
    const __m128i tr0_1 = _mm_unpackhi_epi16(res0, res1);
    // 00 10 01 11 02 12 03 13
    // 20 30 21 31 22 32 23 33
    in0 = _mm_unpacklo_epi32(tr0_0, tr0_1);
    in2 = _mm_unpackhi_epi32(tr0_0, tr0_1);
    // 00 10 20 30 01 11 21 31      in0 contains 0 followed by 1
    // 02 12 22 32 03 13 23 33      in2 contains 2 followed by 3
    if (0 == pass) {
      // Extract values in the high part for second pass as transform code
      // only uses the first four values.
      in1 = _mm_unpackhi_epi64(in0, in0);
      in3 = _mm_unpackhi_epi64(in2, in2);
    } else {
      // Post-condition output and store it (v + 1) >> 2, taking advantage
      // of the fact 1/3 are stored just after 0/2.
      __m128i out01 = _mm_add_epi16(in0, kOne);
      __m128i out23 = _mm_add_epi16(in2, kOne);
      out01 = _mm_srai_epi16(out01, 2);
      out23 = _mm_srai_epi16(out23, 2);
      _mm_storeu_si128((__m128i *)(output + 0 * 4), out01);
      _mm_storeu_si128((__m128i *)(output + 2 * 4), out23);
    }
  }
}

void vp9_short_fdct8x4_sse2(int16_t *input, int16_t *output, int pitch) {
  vp9_short_fdct4x4_sse2(input, output, pitch);
  vp9_short_fdct4x4_sse2(input + 4, output + 16, pitch);
}

void vp9_short_fdct8x8_sse2(int16_t *input, int16_t *output, int pitch) {
  const int stride = pitch >> 1;
  int pass;
  // Constants
  //    When we use them, in one case, they are all the same. In all others
  //    it's a pair of them that we need to repeat four times. This is done
  //    by constructing the 32 bit constant corresponding to that pair.
  const __m128i k__cospi_p16_p16 = _mm_set1_epi16(cospi_16_64);
  const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i k__cospi_p24_p08 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i k__cospi_m08_p24 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i k__cospi_p28_p04 = pair_set_epi16(cospi_28_64, cospi_4_64);
  const __m128i k__cospi_m04_p28 = pair_set_epi16(-cospi_4_64, cospi_28_64);
  const __m128i k__cospi_p12_p20 = pair_set_epi16(cospi_12_64, cospi_20_64);
  const __m128i k__cospi_m20_p12 = pair_set_epi16(-cospi_20_64, cospi_12_64);
  const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);
  // Load input
  __m128i in0  = _mm_loadu_si128((const __m128i *)(input + 0 * stride));
  __m128i in1  = _mm_loadu_si128((const __m128i *)(input + 1 * stride));
  __m128i in2  = _mm_loadu_si128((const __m128i *)(input + 2 * stride));
  __m128i in3  = _mm_loadu_si128((const __m128i *)(input + 3 * stride));
  __m128i in4  = _mm_loadu_si128((const __m128i *)(input + 4 * stride));
  __m128i in5  = _mm_loadu_si128((const __m128i *)(input + 5 * stride));
  __m128i in6  = _mm_loadu_si128((const __m128i *)(input + 6 * stride));
  __m128i in7  = _mm_loadu_si128((const __m128i *)(input + 7 * stride));
  // Pre-condition input (shift by two)
  in0 = _mm_slli_epi16(in0, 2);
  in1 = _mm_slli_epi16(in1, 2);
  in2 = _mm_slli_epi16(in2, 2);
  in3 = _mm_slli_epi16(in3, 2);
  in4 = _mm_slli_epi16(in4, 2);
  in5 = _mm_slli_epi16(in5, 2);
  in6 = _mm_slli_epi16(in6, 2);
  in7 = _mm_slli_epi16(in7, 2);

  // We do two passes, first the columns, then the rows. The results of the
  // first pass are transposed so that the same column code can be reused. The
  // results of the second pass are also transposed so that the rows (processed
  // as columns) are put back in row positions.
  for (pass = 0; pass < 2; pass++) {
    // To store results of each pass before the transpose.
    __m128i res0, res1, res2, res3, res4, res5, res6, res7;
    // Add/substract
    const __m128i q0 = _mm_add_epi16(in0, in7);
    const __m128i q1 = _mm_add_epi16(in1, in6);
    const __m128i q2 = _mm_add_epi16(in2, in5);
    const __m128i q3 = _mm_add_epi16(in3, in4);
    const __m128i q4 = _mm_sub_epi16(in3, in4);
    const __m128i q5 = _mm_sub_epi16(in2, in5);
    const __m128i q6 = _mm_sub_epi16(in1, in6);
    const __m128i q7 = _mm_sub_epi16(in0, in7);
    // Work on first four results
    {
      // Add/substract
      const __m128i r0 = _mm_add_epi16(q0, q3);
      const __m128i r1 = _mm_add_epi16(q1, q2);
      const __m128i r2 = _mm_sub_epi16(q1, q2);
      const __m128i r3 = _mm_sub_epi16(q0, q3);
      // Interleave to do the multiply by constants which gets us into 32bits
      const __m128i t0 = _mm_unpacklo_epi16(r0, r1);
      const __m128i t1 = _mm_unpackhi_epi16(r0, r1);
      const __m128i t2 = _mm_unpacklo_epi16(r2, r3);
      const __m128i t3 = _mm_unpackhi_epi16(r2, r3);
      const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p16_p16);
      const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p16_p16);
      const __m128i u2 = _mm_madd_epi16(t0, k__cospi_p16_m16);
      const __m128i u3 = _mm_madd_epi16(t1, k__cospi_p16_m16);
      const __m128i u4 = _mm_madd_epi16(t2, k__cospi_p24_p08);
      const __m128i u5 = _mm_madd_epi16(t3, k__cospi_p24_p08);
      const __m128i u6 = _mm_madd_epi16(t2, k__cospi_m08_p24);
      const __m128i u7 = _mm_madd_epi16(t3, k__cospi_m08_p24);
      // dct_const_round_shift
      const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
      const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
      const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
      const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
      const __m128i v4 = _mm_add_epi32(u4, k__DCT_CONST_ROUNDING);
      const __m128i v5 = _mm_add_epi32(u5, k__DCT_CONST_ROUNDING);
      const __m128i v6 = _mm_add_epi32(u6, k__DCT_CONST_ROUNDING);
      const __m128i v7 = _mm_add_epi32(u7, k__DCT_CONST_ROUNDING);
      const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
      const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
      const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
      const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
      const __m128i w4 = _mm_srai_epi32(v4, DCT_CONST_BITS);
      const __m128i w5 = _mm_srai_epi32(v5, DCT_CONST_BITS);
      const __m128i w6 = _mm_srai_epi32(v6, DCT_CONST_BITS);
      const __m128i w7 = _mm_srai_epi32(v7, DCT_CONST_BITS);
      // Combine
      res0 = _mm_packs_epi32(w0, w1);
      res4 = _mm_packs_epi32(w2, w3);
      res2 = _mm_packs_epi32(w4, w5);
      res6 = _mm_packs_epi32(w6, w7);
    }
    // Work on next four results
    {
      // Interleave to do the multiply by constants which gets us into 32bits
      const __m128i d0 = _mm_unpacklo_epi16(q6, q5);
      const __m128i d1 = _mm_unpackhi_epi16(q6, q5);
      const __m128i e0 = _mm_madd_epi16(d0, k__cospi_p16_m16);
      const __m128i e1 = _mm_madd_epi16(d1, k__cospi_p16_m16);
      const __m128i e2 = _mm_madd_epi16(d0, k__cospi_p16_p16);
      const __m128i e3 = _mm_madd_epi16(d1, k__cospi_p16_p16);
      // dct_const_round_shift
      const __m128i f0 = _mm_add_epi32(e0, k__DCT_CONST_ROUNDING);
      const __m128i f1 = _mm_add_epi32(e1, k__DCT_CONST_ROUNDING);
      const __m128i f2 = _mm_add_epi32(e2, k__DCT_CONST_ROUNDING);
      const __m128i f3 = _mm_add_epi32(e3, k__DCT_CONST_ROUNDING);
      const __m128i s0 = _mm_srai_epi32(f0, DCT_CONST_BITS);
      const __m128i s1 = _mm_srai_epi32(f1, DCT_CONST_BITS);
      const __m128i s2 = _mm_srai_epi32(f2, DCT_CONST_BITS);
      const __m128i s3 = _mm_srai_epi32(f3, DCT_CONST_BITS);
      // Combine
      const __m128i r0 = _mm_packs_epi32(s0, s1);
      const __m128i r1 = _mm_packs_epi32(s2, s3);
      // Add/substract
      const __m128i x0 = _mm_add_epi16(q4, r0);
      const __m128i x1 = _mm_sub_epi16(q4, r0);
      const __m128i x2 = _mm_sub_epi16(q7, r1);
      const __m128i x3 = _mm_add_epi16(q7, r1);
      // Interleave to do the multiply by constants which gets us into 32bits
      const __m128i t0 = _mm_unpacklo_epi16(x0, x3);
      const __m128i t1 = _mm_unpackhi_epi16(x0, x3);
      const __m128i t2 = _mm_unpacklo_epi16(x1, x2);
      const __m128i t3 = _mm_unpackhi_epi16(x1, x2);
      const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p28_p04);
      const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p28_p04);
      const __m128i u2 = _mm_madd_epi16(t0, k__cospi_m04_p28);
      const __m128i u3 = _mm_madd_epi16(t1, k__cospi_m04_p28);
      const __m128i u4 = _mm_madd_epi16(t2, k__cospi_p12_p20);
      const __m128i u5 = _mm_madd_epi16(t3, k__cospi_p12_p20);
      const __m128i u6 = _mm_madd_epi16(t2, k__cospi_m20_p12);
      const __m128i u7 = _mm_madd_epi16(t3, k__cospi_m20_p12);
      // dct_const_round_shift
      const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
      const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
      const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
      const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
      const __m128i v4 = _mm_add_epi32(u4, k__DCT_CONST_ROUNDING);
      const __m128i v5 = _mm_add_epi32(u5, k__DCT_CONST_ROUNDING);
      const __m128i v6 = _mm_add_epi32(u6, k__DCT_CONST_ROUNDING);
      const __m128i v7 = _mm_add_epi32(u7, k__DCT_CONST_ROUNDING);
      const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
      const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
      const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
      const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
      const __m128i w4 = _mm_srai_epi32(v4, DCT_CONST_BITS);
      const __m128i w5 = _mm_srai_epi32(v5, DCT_CONST_BITS);
      const __m128i w6 = _mm_srai_epi32(v6, DCT_CONST_BITS);
      const __m128i w7 = _mm_srai_epi32(v7, DCT_CONST_BITS);
      // Combine
      res1 = _mm_packs_epi32(w0, w1);
      res7 = _mm_packs_epi32(w2, w3);
      res5 = _mm_packs_epi32(w4, w5);
      res3 = _mm_packs_epi32(w6, w7);
    }
    // Transpose the 8x8.
    {
      // 00 01 02 03 04 05 06 07
      // 10 11 12 13 14 15 16 17
      // 20 21 22 23 24 25 26 27
      // 30 31 32 33 34 35 36 37
      // 40 41 42 43 44 45 46 47
      // 50 51 52 53 54 55 56 57
      // 60 61 62 63 64 65 66 67
      // 70 71 72 73 74 75 76 77
      const __m128i tr0_0 = _mm_unpacklo_epi16(res0, res1);
      const __m128i tr0_1 = _mm_unpacklo_epi16(res2, res3);
      const __m128i tr0_2 = _mm_unpackhi_epi16(res0, res1);
      const __m128i tr0_3 = _mm_unpackhi_epi16(res2, res3);
      const __m128i tr0_4 = _mm_unpacklo_epi16(res4, res5);
      const __m128i tr0_5 = _mm_unpacklo_epi16(res6, res7);
      const __m128i tr0_6 = _mm_unpackhi_epi16(res4, res5);
      const __m128i tr0_7 = _mm_unpackhi_epi16(res6, res7);
      // 00 10 01 11 02 12 03 13
      // 20 30 21 31 22 32 23 33
      // 04 14 05 15 06 16 07 17
      // 24 34 25 35 26 36 27 37
      // 40 50 41 51 42 52 43 53
      // 60 70 61 71 62 72 63 73
      // 54 54 55 55 56 56 57 57
      // 64 74 65 75 66 76 67 77
      const __m128i tr1_0 = _mm_unpacklo_epi32(tr0_0, tr0_1);
      const __m128i tr1_1 = _mm_unpacklo_epi32(tr0_2, tr0_3);
      const __m128i tr1_2 = _mm_unpackhi_epi32(tr0_0, tr0_1);
      const __m128i tr1_3 = _mm_unpackhi_epi32(tr0_2, tr0_3);
      const __m128i tr1_4 = _mm_unpacklo_epi32(tr0_4, tr0_5);
      const __m128i tr1_5 = _mm_unpacklo_epi32(tr0_6, tr0_7);
      const __m128i tr1_6 = _mm_unpackhi_epi32(tr0_4, tr0_5);
      const __m128i tr1_7 = _mm_unpackhi_epi32(tr0_6, tr0_7);
      // 00 10 20 30 01 11 21 31
      // 40 50 60 70 41 51 61 71
      // 02 12 22 32 03 13 23 33
      // 42 52 62 72 43 53 63 73
      // 04 14 24 34 05 15 21 36
      // 44 54 64 74 45 55 61 76
      // 06 16 26 36 07 17 27 37
      // 46 56 66 76 47 57 67 77
      in0 = _mm_unpacklo_epi64(tr1_0, tr1_4);
      in1 = _mm_unpackhi_epi64(tr1_0, tr1_4);
      in2 = _mm_unpacklo_epi64(tr1_2, tr1_6);
      in3 = _mm_unpackhi_epi64(tr1_2, tr1_6);
      in4 = _mm_unpacklo_epi64(tr1_1, tr1_5);
      in5 = _mm_unpackhi_epi64(tr1_1, tr1_5);
      in6 = _mm_unpacklo_epi64(tr1_3, tr1_7);
      in7 = _mm_unpackhi_epi64(tr1_3, tr1_7);
      // 00 10 20 30 40 50 60 70
      // 01 11 21 31 41 51 61 71
      // 02 12 22 32 42 52 62 72
      // 03 13 23 33 43 53 63 73
      // 04 14 24 34 44 54 64 74
      // 05 15 25 35 45 55 65 75
      // 06 16 26 36 46 56 66 76
      // 07 17 27 37 47 57 67 77
    }
  }
  // Post-condition output and store it
  {
    // Post-condition (division by two)
    //    division of two 16 bits signed numbers using shifts
    //    n / 2 = (n - (n >> 15)) >> 1
    const __m128i sign_in0 = _mm_srai_epi16(in0, 15);
    const __m128i sign_in1 = _mm_srai_epi16(in1, 15);
    const __m128i sign_in2 = _mm_srai_epi16(in2, 15);
    const __m128i sign_in3 = _mm_srai_epi16(in3, 15);
    const __m128i sign_in4 = _mm_srai_epi16(in4, 15);
    const __m128i sign_in5 = _mm_srai_epi16(in5, 15);
    const __m128i sign_in6 = _mm_srai_epi16(in6, 15);
    const __m128i sign_in7 = _mm_srai_epi16(in7, 15);
    in0 = _mm_sub_epi16(in0, sign_in0);
    in1 = _mm_sub_epi16(in1, sign_in1);
    in2 = _mm_sub_epi16(in2, sign_in2);
    in3 = _mm_sub_epi16(in3, sign_in3);
    in4 = _mm_sub_epi16(in4, sign_in4);
    in5 = _mm_sub_epi16(in5, sign_in5);
    in6 = _mm_sub_epi16(in6, sign_in6);
    in7 = _mm_sub_epi16(in7, sign_in7);
    in0 = _mm_srai_epi16(in0, 1);
    in1 = _mm_srai_epi16(in1, 1);
    in2 = _mm_srai_epi16(in2, 1);
    in3 = _mm_srai_epi16(in3, 1);
    in4 = _mm_srai_epi16(in4, 1);
    in5 = _mm_srai_epi16(in5, 1);
    in6 = _mm_srai_epi16(in6, 1);
    in7 = _mm_srai_epi16(in7, 1);
    // store results
    _mm_storeu_si128((__m128i *)(output + 0 * 8), in0);
    _mm_storeu_si128((__m128i *)(output + 1 * 8), in1);
    _mm_storeu_si128((__m128i *)(output + 2 * 8), in2);
    _mm_storeu_si128((__m128i *)(output + 3 * 8), in3);
    _mm_storeu_si128((__m128i *)(output + 4 * 8), in4);
    _mm_storeu_si128((__m128i *)(output + 5 * 8), in5);
    _mm_storeu_si128((__m128i *)(output + 6 * 8), in6);
    _mm_storeu_si128((__m128i *)(output + 7 * 8), in7);
  }
}

void vp9_short_fdct16x16_sse2(int16_t *input, int16_t *output, int pitch) {
  // The 2D transform is done with two passes which are actually pretty
  // similar. In the first one, we transform the columns and transpose
  // the results. In the second one, we transform the rows. To achieve that,
  // as the first pass results are transposed, we tranpose the columns (that
  // is the transposed rows) and transpose the results (so that it goes back
  // in normal/row positions).
  const int stride = pitch >> 1;
  int pass;
  // We need an intermediate buffer between passes.
  int16_t intermediate[256];
  int16_t *in = input;
  int16_t *out = intermediate;
  // Constants
  //    When we use them, in one case, they are all the same. In all others
  //    it's a pair of them that we need to repeat four times. This is done
  //    by constructing the 32 bit constant corresponding to that pair.
  const __m128i k__cospi_p16_p16 = _mm_set1_epi16(cospi_16_64);
  const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i k__cospi_p24_p08 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i k__cospi_m24_m08 = pair_set_epi16(-cospi_24_64, -cospi_8_64);
  const __m128i k__cospi_m08_p24 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i k__cospi_p28_p04 = pair_set_epi16(cospi_28_64, cospi_4_64);
  const __m128i k__cospi_m04_p28 = pair_set_epi16(-cospi_4_64, cospi_28_64);
  const __m128i k__cospi_p12_p20 = pair_set_epi16(cospi_12_64, cospi_20_64);
  const __m128i k__cospi_m20_p12 = pair_set_epi16(-cospi_20_64, cospi_12_64);
  const __m128i k__cospi_p30_p02 = pair_set_epi16(cospi_30_64, cospi_2_64);
  const __m128i k__cospi_p14_p18 = pair_set_epi16(cospi_14_64, cospi_18_64);
  const __m128i k__cospi_m02_p30 = pair_set_epi16(-cospi_2_64, cospi_30_64);
  const __m128i k__cospi_m18_p14 = pair_set_epi16(-cospi_18_64, cospi_14_64);
  const __m128i k__cospi_p22_p10 = pair_set_epi16(cospi_22_64, cospi_10_64);
  const __m128i k__cospi_p06_p26 = pair_set_epi16(cospi_6_64, cospi_26_64);
  const __m128i k__cospi_m10_p22 = pair_set_epi16(-cospi_10_64, cospi_22_64);
  const __m128i k__cospi_m26_p06 = pair_set_epi16(-cospi_26_64, cospi_6_64);
  const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);
  const __m128i kOne = _mm_set1_epi16(1);
  // Do the two transform/transpose passes
  for (pass = 0; pass < 2; ++pass) {
    // We process eight columns (transposed rows in second pass) at a time.
    int column_start;
    for (column_start = 0; column_start < 16; column_start += 8) {
      __m128i in00, in01, in02, in03, in04, in05, in06, in07;
      __m128i in08, in09, in10, in11, in12, in13, in14, in15;
      __m128i input0, input1, input2, input3, input4, input5, input6, input7;
      __m128i step1_0, step1_1, step1_2, step1_3;
      __m128i step1_4, step1_5, step1_6, step1_7;
      __m128i step2_1, step2_2, step2_3, step2_4, step2_5, step2_6;
      __m128i step3_0, step3_1, step3_2, step3_3;
      __m128i step3_4, step3_5, step3_6, step3_7;
      __m128i res00, res01, res02, res03, res04, res05, res06, res07;
      __m128i res08, res09, res10, res11, res12, res13, res14, res15;
      // Load and pre-condition input.
      if (0 == pass) {
        in00  = _mm_loadu_si128((const __m128i *)(in +  0 * stride));
        in01  = _mm_loadu_si128((const __m128i *)(in +  1 * stride));
        in02  = _mm_loadu_si128((const __m128i *)(in +  2 * stride));
        in03  = _mm_loadu_si128((const __m128i *)(in +  3 * stride));
        in04  = _mm_loadu_si128((const __m128i *)(in +  4 * stride));
        in05  = _mm_loadu_si128((const __m128i *)(in +  5 * stride));
        in06  = _mm_loadu_si128((const __m128i *)(in +  6 * stride));
        in07  = _mm_loadu_si128((const __m128i *)(in +  7 * stride));
        in08  = _mm_loadu_si128((const __m128i *)(in +  8 * stride));
        in09  = _mm_loadu_si128((const __m128i *)(in +  9 * stride));
        in10  = _mm_loadu_si128((const __m128i *)(in + 10 * stride));
        in11  = _mm_loadu_si128((const __m128i *)(in + 11 * stride));
        in12  = _mm_loadu_si128((const __m128i *)(in + 12 * stride));
        in13  = _mm_loadu_si128((const __m128i *)(in + 13 * stride));
        in14  = _mm_loadu_si128((const __m128i *)(in + 14 * stride));
        in15  = _mm_loadu_si128((const __m128i *)(in + 15 * stride));
        // x = x << 2
        in00 = _mm_slli_epi16(in00, 2);
        in01 = _mm_slli_epi16(in01, 2);
        in02 = _mm_slli_epi16(in02, 2);
        in03 = _mm_slli_epi16(in03, 2);
        in04 = _mm_slli_epi16(in04, 2);
        in05 = _mm_slli_epi16(in05, 2);
        in06 = _mm_slli_epi16(in06, 2);
        in07 = _mm_slli_epi16(in07, 2);
        in08 = _mm_slli_epi16(in08, 2);
        in09 = _mm_slli_epi16(in09, 2);
        in10 = _mm_slli_epi16(in10, 2);
        in11 = _mm_slli_epi16(in11, 2);
        in12 = _mm_slli_epi16(in12, 2);
        in13 = _mm_slli_epi16(in13, 2);
        in14 = _mm_slli_epi16(in14, 2);
        in15 = _mm_slli_epi16(in15, 2);
      } else {
        in00  = _mm_loadu_si128((const __m128i *)(in +  0 * 16));
        in01  = _mm_loadu_si128((const __m128i *)(in +  1 * 16));
        in02  = _mm_loadu_si128((const __m128i *)(in +  2 * 16));
        in03  = _mm_loadu_si128((const __m128i *)(in +  3 * 16));
        in04  = _mm_loadu_si128((const __m128i *)(in +  4 * 16));
        in05  = _mm_loadu_si128((const __m128i *)(in +  5 * 16));
        in06  = _mm_loadu_si128((const __m128i *)(in +  6 * 16));
        in07  = _mm_loadu_si128((const __m128i *)(in +  7 * 16));
        in08  = _mm_loadu_si128((const __m128i *)(in +  8 * 16));
        in09  = _mm_loadu_si128((const __m128i *)(in +  9 * 16));
        in10  = _mm_loadu_si128((const __m128i *)(in + 10 * 16));
        in11  = _mm_loadu_si128((const __m128i *)(in + 11 * 16));
        in12  = _mm_loadu_si128((const __m128i *)(in + 12 * 16));
        in13  = _mm_loadu_si128((const __m128i *)(in + 13 * 16));
        in14  = _mm_loadu_si128((const __m128i *)(in + 14 * 16));
        in15  = _mm_loadu_si128((const __m128i *)(in + 15 * 16));
        // x = (x + 1) >> 2
        in00 = _mm_add_epi16(in00, kOne);
        in01 = _mm_add_epi16(in01, kOne);
        in02 = _mm_add_epi16(in02, kOne);
        in03 = _mm_add_epi16(in03, kOne);
        in04 = _mm_add_epi16(in04, kOne);
        in05 = _mm_add_epi16(in05, kOne);
        in06 = _mm_add_epi16(in06, kOne);
        in07 = _mm_add_epi16(in07, kOne);
        in08 = _mm_add_epi16(in08, kOne);
        in09 = _mm_add_epi16(in09, kOne);
        in10 = _mm_add_epi16(in10, kOne);
        in11 = _mm_add_epi16(in11, kOne);
        in12 = _mm_add_epi16(in12, kOne);
        in13 = _mm_add_epi16(in13, kOne);
        in14 = _mm_add_epi16(in14, kOne);
        in15 = _mm_add_epi16(in15, kOne);
        in00 = _mm_srai_epi16(in00, 2);
        in01 = _mm_srai_epi16(in01, 2);
        in02 = _mm_srai_epi16(in02, 2);
        in03 = _mm_srai_epi16(in03, 2);
        in04 = _mm_srai_epi16(in04, 2);
        in05 = _mm_srai_epi16(in05, 2);
        in06 = _mm_srai_epi16(in06, 2);
        in07 = _mm_srai_epi16(in07, 2);
        in08 = _mm_srai_epi16(in08, 2);
        in09 = _mm_srai_epi16(in09, 2);
        in10 = _mm_srai_epi16(in10, 2);
        in11 = _mm_srai_epi16(in11, 2);
        in12 = _mm_srai_epi16(in12, 2);
        in13 = _mm_srai_epi16(in13, 2);
        in14 = _mm_srai_epi16(in14, 2);
        in15 = _mm_srai_epi16(in15, 2);
      }
      in += 8;
      // Calculate input for the first 8 results.
      {
        input0 = _mm_add_epi16(in00, in15);
        input1 = _mm_add_epi16(in01, in14);
        input2 = _mm_add_epi16(in02, in13);
        input3 = _mm_add_epi16(in03, in12);
        input4 = _mm_add_epi16(in04, in11);
        input5 = _mm_add_epi16(in05, in10);
        input6 = _mm_add_epi16(in06, in09);
        input7 = _mm_add_epi16(in07, in08);
      }
      // Calculate input for the next 8 results.
      {
        step1_0 = _mm_sub_epi16(in07, in08);
        step1_1 = _mm_sub_epi16(in06, in09);
        step1_2 = _mm_sub_epi16(in05, in10);
        step1_3 = _mm_sub_epi16(in04, in11);
        step1_4 = _mm_sub_epi16(in03, in12);
        step1_5 = _mm_sub_epi16(in02, in13);
        step1_6 = _mm_sub_epi16(in01, in14);
        step1_7 = _mm_sub_epi16(in00, in15);
      }
      // Work on the first eight values; fdct8_1d(input, even_results);
      {
        // Add/substract
        const __m128i q0 = _mm_add_epi16(input0, input7);
        const __m128i q1 = _mm_add_epi16(input1, input6);
        const __m128i q2 = _mm_add_epi16(input2, input5);
        const __m128i q3 = _mm_add_epi16(input3, input4);
        const __m128i q4 = _mm_sub_epi16(input3, input4);
        const __m128i q5 = _mm_sub_epi16(input2, input5);
        const __m128i q6 = _mm_sub_epi16(input1, input6);
        const __m128i q7 = _mm_sub_epi16(input0, input7);
        // Work on first four results
        {
          // Add/substract
          const __m128i r0 = _mm_add_epi16(q0, q3);
          const __m128i r1 = _mm_add_epi16(q1, q2);
          const __m128i r2 = _mm_sub_epi16(q1, q2);
          const __m128i r3 = _mm_sub_epi16(q0, q3);
          // Interleave to do the multiply by constants which gets us
          // into 32 bits.
          const __m128i t0 = _mm_unpacklo_epi16(r0, r1);
          const __m128i t1 = _mm_unpackhi_epi16(r0, r1);
          const __m128i t2 = _mm_unpacklo_epi16(r2, r3);
          const __m128i t3 = _mm_unpackhi_epi16(r2, r3);
          const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p16_p16);
          const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p16_p16);
          const __m128i u2 = _mm_madd_epi16(t0, k__cospi_p16_m16);
          const __m128i u3 = _mm_madd_epi16(t1, k__cospi_p16_m16);
          const __m128i u4 = _mm_madd_epi16(t2, k__cospi_p24_p08);
          const __m128i u5 = _mm_madd_epi16(t3, k__cospi_p24_p08);
          const __m128i u6 = _mm_madd_epi16(t2, k__cospi_m08_p24);
          const __m128i u7 = _mm_madd_epi16(t3, k__cospi_m08_p24);
          // dct_const_round_shift
          const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
          const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
          const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
          const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
          const __m128i v4 = _mm_add_epi32(u4, k__DCT_CONST_ROUNDING);
          const __m128i v5 = _mm_add_epi32(u5, k__DCT_CONST_ROUNDING);
          const __m128i v6 = _mm_add_epi32(u6, k__DCT_CONST_ROUNDING);
          const __m128i v7 = _mm_add_epi32(u7, k__DCT_CONST_ROUNDING);
          const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
          const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
          const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
          const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
          const __m128i w4 = _mm_srai_epi32(v4, DCT_CONST_BITS);
          const __m128i w5 = _mm_srai_epi32(v5, DCT_CONST_BITS);
          const __m128i w6 = _mm_srai_epi32(v6, DCT_CONST_BITS);
          const __m128i w7 = _mm_srai_epi32(v7, DCT_CONST_BITS);
          // Combine
          res00 = _mm_packs_epi32(w0, w1);
          res08 = _mm_packs_epi32(w2, w3);
          res04 = _mm_packs_epi32(w4, w5);
          res12 = _mm_packs_epi32(w6, w7);
        }
        // Work on next four results
        {
          // Interleave to do the multiply by constants which gets us
          // into 32 bits.
          const __m128i d0 = _mm_unpacklo_epi16(q6, q5);
          const __m128i d1 = _mm_unpackhi_epi16(q6, q5);
          const __m128i e0 = _mm_madd_epi16(d0, k__cospi_p16_m16);
          const __m128i e1 = _mm_madd_epi16(d1, k__cospi_p16_m16);
          const __m128i e2 = _mm_madd_epi16(d0, k__cospi_p16_p16);
          const __m128i e3 = _mm_madd_epi16(d1, k__cospi_p16_p16);
          // dct_const_round_shift
          const __m128i f0 = _mm_add_epi32(e0, k__DCT_CONST_ROUNDING);
          const __m128i f1 = _mm_add_epi32(e1, k__DCT_CONST_ROUNDING);
          const __m128i f2 = _mm_add_epi32(e2, k__DCT_CONST_ROUNDING);
          const __m128i f3 = _mm_add_epi32(e3, k__DCT_CONST_ROUNDING);
          const __m128i s0 = _mm_srai_epi32(f0, DCT_CONST_BITS);
          const __m128i s1 = _mm_srai_epi32(f1, DCT_CONST_BITS);
          const __m128i s2 = _mm_srai_epi32(f2, DCT_CONST_BITS);
          const __m128i s3 = _mm_srai_epi32(f3, DCT_CONST_BITS);
          // Combine
          const __m128i r0 = _mm_packs_epi32(s0, s1);
          const __m128i r1 = _mm_packs_epi32(s2, s3);
          // Add/substract
          const __m128i x0 = _mm_add_epi16(q4, r0);
          const __m128i x1 = _mm_sub_epi16(q4, r0);
          const __m128i x2 = _mm_sub_epi16(q7, r1);
          const __m128i x3 = _mm_add_epi16(q7, r1);
          // Interleave to do the multiply by constants which gets us
          // into 32 bits.
          const __m128i t0 = _mm_unpacklo_epi16(x0, x3);
          const __m128i t1 = _mm_unpackhi_epi16(x0, x3);
          const __m128i t2 = _mm_unpacklo_epi16(x1, x2);
          const __m128i t3 = _mm_unpackhi_epi16(x1, x2);
          const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p28_p04);
          const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p28_p04);
          const __m128i u2 = _mm_madd_epi16(t0, k__cospi_m04_p28);
          const __m128i u3 = _mm_madd_epi16(t1, k__cospi_m04_p28);
          const __m128i u4 = _mm_madd_epi16(t2, k__cospi_p12_p20);
          const __m128i u5 = _mm_madd_epi16(t3, k__cospi_p12_p20);
          const __m128i u6 = _mm_madd_epi16(t2, k__cospi_m20_p12);
          const __m128i u7 = _mm_madd_epi16(t3, k__cospi_m20_p12);
          // dct_const_round_shift
          const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
          const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
          const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
          const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
          const __m128i v4 = _mm_add_epi32(u4, k__DCT_CONST_ROUNDING);
          const __m128i v5 = _mm_add_epi32(u5, k__DCT_CONST_ROUNDING);
          const __m128i v6 = _mm_add_epi32(u6, k__DCT_CONST_ROUNDING);
          const __m128i v7 = _mm_add_epi32(u7, k__DCT_CONST_ROUNDING);
          const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
          const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
          const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
          const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
          const __m128i w4 = _mm_srai_epi32(v4, DCT_CONST_BITS);
          const __m128i w5 = _mm_srai_epi32(v5, DCT_CONST_BITS);
          const __m128i w6 = _mm_srai_epi32(v6, DCT_CONST_BITS);
          const __m128i w7 = _mm_srai_epi32(v7, DCT_CONST_BITS);
          // Combine
          res02 = _mm_packs_epi32(w0, w1);
          res14 = _mm_packs_epi32(w2, w3);
          res10 = _mm_packs_epi32(w4, w5);
          res06 = _mm_packs_epi32(w6, w7);
        }
      }
      // Work on the next eight values; step1 -> odd_results
      {
        // step 2
        {
          const __m128i t0 = _mm_unpacklo_epi16(step1_5, step1_2);
          const __m128i t1 = _mm_unpackhi_epi16(step1_5, step1_2);
          const __m128i t2 = _mm_unpacklo_epi16(step1_4, step1_3);
          const __m128i t3 = _mm_unpackhi_epi16(step1_4, step1_3);
          const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p16_m16);
          const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p16_m16);
          const __m128i u2 = _mm_madd_epi16(t2, k__cospi_p16_m16);
          const __m128i u3 = _mm_madd_epi16(t3, k__cospi_p16_m16);
          // dct_const_round_shift
          const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
          const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
          const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
          const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
          const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
          const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
          const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
          const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
          // Combine
          step2_2 = _mm_packs_epi32(w0, w1);
          step2_3 = _mm_packs_epi32(w2, w3);
        }
        {
          const __m128i t0 = _mm_unpacklo_epi16(step1_5, step1_2);
          const __m128i t1 = _mm_unpackhi_epi16(step1_5, step1_2);
          const __m128i t2 = _mm_unpacklo_epi16(step1_4, step1_3);
          const __m128i t3 = _mm_unpackhi_epi16(step1_4, step1_3);
          const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p16_p16);
          const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p16_p16);
          const __m128i u2 = _mm_madd_epi16(t2, k__cospi_p16_p16);
          const __m128i u3 = _mm_madd_epi16(t3, k__cospi_p16_p16);
          // dct_const_round_shift
          const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
          const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
          const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
          const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
          const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
          const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
          const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
          const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
          // Combine
          step2_5 = _mm_packs_epi32(w0, w1);
          step2_4 = _mm_packs_epi32(w2, w3);
        }
        // step 3
        {
          step3_0 = _mm_add_epi16(step1_0, step2_3);
          step3_1 = _mm_add_epi16(step1_1, step2_2);
          step3_2 = _mm_sub_epi16(step1_1, step2_2);
          step3_3 = _mm_sub_epi16(step1_0, step2_3);
          step3_4 = _mm_sub_epi16(step1_7, step2_4);
          step3_5 = _mm_sub_epi16(step1_6, step2_5);
          step3_6 = _mm_add_epi16(step1_6, step2_5);
          step3_7 = _mm_add_epi16(step1_7, step2_4);
        }
        // step 4
        {
          const __m128i t0 = _mm_unpacklo_epi16(step3_1, step3_6);
          const __m128i t1 = _mm_unpackhi_epi16(step3_1, step3_6);
          const __m128i t2 = _mm_unpacklo_epi16(step3_2, step3_5);
          const __m128i t3 = _mm_unpackhi_epi16(step3_2, step3_5);
          const __m128i u0 = _mm_madd_epi16(t0, k__cospi_m08_p24);
          const __m128i u1 = _mm_madd_epi16(t1, k__cospi_m08_p24);
          const __m128i u2 = _mm_madd_epi16(t2, k__cospi_m24_m08);
          const __m128i u3 = _mm_madd_epi16(t3, k__cospi_m24_m08);
          // dct_const_round_shift
          const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
          const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
          const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
          const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
          const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
          const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
          const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
          const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
          // Combine
          step2_1 = _mm_packs_epi32(w0, w1);
          step2_2 = _mm_packs_epi32(w2, w3);
        }
        {
          const __m128i t0 = _mm_unpacklo_epi16(step3_1, step3_6);
          const __m128i t1 = _mm_unpackhi_epi16(step3_1, step3_6);
          const __m128i t2 = _mm_unpacklo_epi16(step3_2, step3_5);
          const __m128i t3 = _mm_unpackhi_epi16(step3_2, step3_5);
          const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p24_p08);
          const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p24_p08);
          const __m128i u2 = _mm_madd_epi16(t2, k__cospi_m08_p24);
          const __m128i u3 = _mm_madd_epi16(t3, k__cospi_m08_p24);
          // dct_const_round_shift
          const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
          const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
          const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
          const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
          const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
          const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
          const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
          const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
          // Combine
          step2_6 = _mm_packs_epi32(w0, w1);
          step2_5 = _mm_packs_epi32(w2, w3);
        }
        // step 5
        {
          step1_0 = _mm_add_epi16(step3_0, step2_1);
          step1_1 = _mm_sub_epi16(step3_0, step2_1);
          step1_2 = _mm_sub_epi16(step3_3, step2_2);
          step1_3 = _mm_add_epi16(step3_3, step2_2);
          step1_4 = _mm_add_epi16(step3_4, step2_5);
          step1_5 = _mm_sub_epi16(step3_4, step2_5);
          step1_6 = _mm_sub_epi16(step3_7, step2_6);
          step1_7 = _mm_add_epi16(step3_7, step2_6);
        }
        // step 6
        {
          const __m128i t0 = _mm_unpacklo_epi16(step1_0, step1_7);
          const __m128i t1 = _mm_unpackhi_epi16(step1_0, step1_7);
          const __m128i t2 = _mm_unpacklo_epi16(step1_1, step1_6);
          const __m128i t3 = _mm_unpackhi_epi16(step1_1, step1_6);
          const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p30_p02);
          const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p30_p02);
          const __m128i u2 = _mm_madd_epi16(t2, k__cospi_p14_p18);
          const __m128i u3 = _mm_madd_epi16(t3, k__cospi_p14_p18);
          // dct_const_round_shift
          const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
          const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
          const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
          const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
          const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
          const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
          const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
          const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
          // Combine
          res01 = _mm_packs_epi32(w0, w1);
          res09 = _mm_packs_epi32(w2, w3);
        }
        {
          const __m128i t0 = _mm_unpacklo_epi16(step1_2, step1_5);
          const __m128i t1 = _mm_unpackhi_epi16(step1_2, step1_5);
          const __m128i t2 = _mm_unpacklo_epi16(step1_3, step1_4);
          const __m128i t3 = _mm_unpackhi_epi16(step1_3, step1_4);
          const __m128i u0 = _mm_madd_epi16(t0, k__cospi_p22_p10);
          const __m128i u1 = _mm_madd_epi16(t1, k__cospi_p22_p10);
          const __m128i u2 = _mm_madd_epi16(t2, k__cospi_p06_p26);
          const __m128i u3 = _mm_madd_epi16(t3, k__cospi_p06_p26);
          // dct_const_round_shift
          const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
          const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
          const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
          const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
          const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
          const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
          const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
          const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
          // Combine
          res05 = _mm_packs_epi32(w0, w1);
          res13 = _mm_packs_epi32(w2, w3);
        }
        {
          const __m128i t0 = _mm_unpacklo_epi16(step1_2, step1_5);
          const __m128i t1 = _mm_unpackhi_epi16(step1_2, step1_5);
          const __m128i t2 = _mm_unpacklo_epi16(step1_3, step1_4);
          const __m128i t3 = _mm_unpackhi_epi16(step1_3, step1_4);
          const __m128i u0 = _mm_madd_epi16(t0, k__cospi_m10_p22);
          const __m128i u1 = _mm_madd_epi16(t1, k__cospi_m10_p22);
          const __m128i u2 = _mm_madd_epi16(t2, k__cospi_m26_p06);
          const __m128i u3 = _mm_madd_epi16(t3, k__cospi_m26_p06);
          // dct_const_round_shift
          const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
          const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
          const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
          const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
          const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
          const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
          const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
          const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
          // Combine
          res11 = _mm_packs_epi32(w0, w1);
          res03 = _mm_packs_epi32(w2, w3);
        }
        {
          const __m128i t0 = _mm_unpacklo_epi16(step1_0, step1_7);
          const __m128i t1 = _mm_unpackhi_epi16(step1_0, step1_7);
          const __m128i t2 = _mm_unpacklo_epi16(step1_1, step1_6);
          const __m128i t3 = _mm_unpackhi_epi16(step1_1, step1_6);
          const __m128i u0 = _mm_madd_epi16(t0, k__cospi_m02_p30);
          const __m128i u1 = _mm_madd_epi16(t1, k__cospi_m02_p30);
          const __m128i u2 = _mm_madd_epi16(t2, k__cospi_m18_p14);
          const __m128i u3 = _mm_madd_epi16(t3, k__cospi_m18_p14);
          // dct_const_round_shift
          const __m128i v0 = _mm_add_epi32(u0, k__DCT_CONST_ROUNDING);
          const __m128i v1 = _mm_add_epi32(u1, k__DCT_CONST_ROUNDING);
          const __m128i v2 = _mm_add_epi32(u2, k__DCT_CONST_ROUNDING);
          const __m128i v3 = _mm_add_epi32(u3, k__DCT_CONST_ROUNDING);
          const __m128i w0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
          const __m128i w1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
          const __m128i w2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
          const __m128i w3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
          // Combine
          res15 = _mm_packs_epi32(w0, w1);
          res07 = _mm_packs_epi32(w2, w3);
        }
      }
      // Transpose the results, do it as two 8x8 transposes.
      {
        // 00 01 02 03 04 05 06 07
        // 10 11 12 13 14 15 16 17
        // 20 21 22 23 24 25 26 27
        // 30 31 32 33 34 35 36 37
        // 40 41 42 43 44 45 46 47
        // 50 51 52 53 54 55 56 57
        // 60 61 62 63 64 65 66 67
        // 70 71 72 73 74 75 76 77
        const __m128i tr0_0 = _mm_unpacklo_epi16(res00, res01);
        const __m128i tr0_1 = _mm_unpacklo_epi16(res02, res03);
        const __m128i tr0_2 = _mm_unpackhi_epi16(res00, res01);
        const __m128i tr0_3 = _mm_unpackhi_epi16(res02, res03);
        const __m128i tr0_4 = _mm_unpacklo_epi16(res04, res05);
        const __m128i tr0_5 = _mm_unpacklo_epi16(res06, res07);
        const __m128i tr0_6 = _mm_unpackhi_epi16(res04, res05);
        const __m128i tr0_7 = _mm_unpackhi_epi16(res06, res07);
        // 00 10 01 11 02 12 03 13
        // 20 30 21 31 22 32 23 33
        // 04 14 05 15 06 16 07 17
        // 24 34 25 35 26 36 27 37
        // 40 50 41 51 42 52 43 53
        // 60 70 61 71 62 72 63 73
        // 54 54 55 55 56 56 57 57
        // 64 74 65 75 66 76 67 77
        const __m128i tr1_0 = _mm_unpacklo_epi32(tr0_0, tr0_1);
        const __m128i tr1_1 = _mm_unpacklo_epi32(tr0_2, tr0_3);
        const __m128i tr1_2 = _mm_unpackhi_epi32(tr0_0, tr0_1);
        const __m128i tr1_3 = _mm_unpackhi_epi32(tr0_2, tr0_3);
        const __m128i tr1_4 = _mm_unpacklo_epi32(tr0_4, tr0_5);
        const __m128i tr1_5 = _mm_unpacklo_epi32(tr0_6, tr0_7);
        const __m128i tr1_6 = _mm_unpackhi_epi32(tr0_4, tr0_5);
        const __m128i tr1_7 = _mm_unpackhi_epi32(tr0_6, tr0_7);
        // 00 10 20 30 01 11 21 31
        // 40 50 60 70 41 51 61 71
        // 02 12 22 32 03 13 23 33
        // 42 52 62 72 43 53 63 73
        // 04 14 24 34 05 15 21 36
        // 44 54 64 74 45 55 61 76
        // 06 16 26 36 07 17 27 37
        // 46 56 66 76 47 57 67 77
        const __m128i tr2_0 = _mm_unpacklo_epi64(tr1_0, tr1_4);
        const __m128i tr2_1 = _mm_unpackhi_epi64(tr1_0, tr1_4);
        const __m128i tr2_2 = _mm_unpacklo_epi64(tr1_2, tr1_6);
        const __m128i tr2_3 = _mm_unpackhi_epi64(tr1_2, tr1_6);
        const __m128i tr2_4 = _mm_unpacklo_epi64(tr1_1, tr1_5);
        const __m128i tr2_5 = _mm_unpackhi_epi64(tr1_1, tr1_5);
        const __m128i tr2_6 = _mm_unpacklo_epi64(tr1_3, tr1_7);
        const __m128i tr2_7 = _mm_unpackhi_epi64(tr1_3, tr1_7);
        // 00 10 20 30 40 50 60 70
        // 01 11 21 31 41 51 61 71
        // 02 12 22 32 42 52 62 72
        // 03 13 23 33 43 53 63 73
        // 04 14 24 34 44 54 64 74
        // 05 15 25 35 45 55 65 75
        // 06 16 26 36 46 56 66 76
        // 07 17 27 37 47 57 67 77
        _mm_storeu_si128((__m128i *)(out + 0 * 16), tr2_0);
        _mm_storeu_si128((__m128i *)(out + 1 * 16), tr2_1);
        _mm_storeu_si128((__m128i *)(out + 2 * 16), tr2_2);
        _mm_storeu_si128((__m128i *)(out + 3 * 16), tr2_3);
        _mm_storeu_si128((__m128i *)(out + 4 * 16), tr2_4);
        _mm_storeu_si128((__m128i *)(out + 5 * 16), tr2_5);
        _mm_storeu_si128((__m128i *)(out + 6 * 16), tr2_6);
        _mm_storeu_si128((__m128i *)(out + 7 * 16), tr2_7);
      }
      {
        // 00 01 02 03 04 05 06 07
        // 10 11 12 13 14 15 16 17
        // 20 21 22 23 24 25 26 27
        // 30 31 32 33 34 35 36 37
        // 40 41 42 43 44 45 46 47
        // 50 51 52 53 54 55 56 57
        // 60 61 62 63 64 65 66 67
        // 70 71 72 73 74 75 76 77
        const __m128i tr0_0 = _mm_unpacklo_epi16(res08, res09);
        const __m128i tr0_1 = _mm_unpacklo_epi16(res10, res11);
        const __m128i tr0_2 = _mm_unpackhi_epi16(res08, res09);
        const __m128i tr0_3 = _mm_unpackhi_epi16(res10, res11);
        const __m128i tr0_4 = _mm_unpacklo_epi16(res12, res13);
        const __m128i tr0_5 = _mm_unpacklo_epi16(res14, res15);
        const __m128i tr0_6 = _mm_unpackhi_epi16(res12, res13);
        const __m128i tr0_7 = _mm_unpackhi_epi16(res14, res15);
        // 00 10 01 11 02 12 03 13
        // 20 30 21 31 22 32 23 33
        // 04 14 05 15 06 16 07 17
        // 24 34 25 35 26 36 27 37
        // 40 50 41 51 42 52 43 53
        // 60 70 61 71 62 72 63 73
        // 54 54 55 55 56 56 57 57
        // 64 74 65 75 66 76 67 77
        const __m128i tr1_0 = _mm_unpacklo_epi32(tr0_0, tr0_1);
        const __m128i tr1_1 = _mm_unpacklo_epi32(tr0_2, tr0_3);
        const __m128i tr1_2 = _mm_unpackhi_epi32(tr0_0, tr0_1);
        const __m128i tr1_3 = _mm_unpackhi_epi32(tr0_2, tr0_3);
        const __m128i tr1_4 = _mm_unpacklo_epi32(tr0_4, tr0_5);
        const __m128i tr1_5 = _mm_unpacklo_epi32(tr0_6, tr0_7);
        const __m128i tr1_6 = _mm_unpackhi_epi32(tr0_4, tr0_5);
        const __m128i tr1_7 = _mm_unpackhi_epi32(tr0_6, tr0_7);
        // 00 10 20 30 01 11 21 31
        // 40 50 60 70 41 51 61 71
        // 02 12 22 32 03 13 23 33
        // 42 52 62 72 43 53 63 73
        // 04 14 24 34 05 15 21 36
        // 44 54 64 74 45 55 61 76
        // 06 16 26 36 07 17 27 37
        // 46 56 66 76 47 57 67 77
        const __m128i tr2_0 = _mm_unpacklo_epi64(tr1_0, tr1_4);
        const __m128i tr2_1 = _mm_unpackhi_epi64(tr1_0, tr1_4);
        const __m128i tr2_2 = _mm_unpacklo_epi64(tr1_2, tr1_6);
        const __m128i tr2_3 = _mm_unpackhi_epi64(tr1_2, tr1_6);
        const __m128i tr2_4 = _mm_unpacklo_epi64(tr1_1, tr1_5);
        const __m128i tr2_5 = _mm_unpackhi_epi64(tr1_1, tr1_5);
        const __m128i tr2_6 = _mm_unpacklo_epi64(tr1_3, tr1_7);
        const __m128i tr2_7 = _mm_unpackhi_epi64(tr1_3, tr1_7);
        // 00 10 20 30 40 50 60 70
        // 01 11 21 31 41 51 61 71
        // 02 12 22 32 42 52 62 72
        // 03 13 23 33 43 53 63 73
        // 04 14 24 34 44 54 64 74
        // 05 15 25 35 45 55 65 75
        // 06 16 26 36 46 56 66 76
        // 07 17 27 37 47 57 67 77
        // Store results
        _mm_storeu_si128((__m128i *)(out + 8 + 0 * 16), tr2_0);
        _mm_storeu_si128((__m128i *)(out + 8 + 1 * 16), tr2_1);
        _mm_storeu_si128((__m128i *)(out + 8 + 2 * 16), tr2_2);
        _mm_storeu_si128((__m128i *)(out + 8 + 3 * 16), tr2_3);
        _mm_storeu_si128((__m128i *)(out + 8 + 4 * 16), tr2_4);
        _mm_storeu_si128((__m128i *)(out + 8 + 5 * 16), tr2_5);
        _mm_storeu_si128((__m128i *)(out + 8 + 6 * 16), tr2_6);
        _mm_storeu_si128((__m128i *)(out + 8 + 7 * 16), tr2_7);
      }
      out += 8*16;
    }
    // Setup in/out for next pass.
    in = intermediate;
    out = output;
  }
}
