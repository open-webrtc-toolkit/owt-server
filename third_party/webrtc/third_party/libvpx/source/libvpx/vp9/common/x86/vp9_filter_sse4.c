/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h> // for alignment checks
#include <smmintrin.h> // SSE4.1
#include "vp9/common/vp9_filter.h"
#include "vpx_ports/mem.h" // for DECLARE_ALIGNED
#include "vp9_rtcd.h"

// TODO(cd): After cleanup, commit faster versions for non 4x4 size. This is
//           just a quick partial snapshot so that other can already use some
//           speedup.
// TODO(cd): Use vectorized 8 tap filtering code as speedup to pure C 6 tap
//           filtering.
// TODO(cd): Reduce source size by using macros instead of current code
//           duplication.
// TODO(cd): Add some comments, better variable naming.
// TODO(cd): Maybe use _mm_maddubs_epi16 if smaller filter coeficients (no sum
//           of positive above 128), or have higher precision filter
//           coefficients.

DECLARE_ALIGNED(16, static const unsigned char, mask0123_c[16]) = {
  0x00, 0x01,
  0x01, 0x02,
  0x02, 0x03,
  0x03, 0x04,
  0x02, 0x03,
  0x03, 0x04,
  0x04, 0x05,
  0x05, 0x06,
};
DECLARE_ALIGNED(16, static const unsigned char, mask4567_c[16]) = {
  0x04, 0x05,
  0x05, 0x06,
  0x06, 0x07,
  0x07, 0x08,
  0x06, 0x07,
  0x07, 0x08,
  0x08, 0x09,
  0x09, 0x0A,
};
DECLARE_ALIGNED(16, static const unsigned int, rounding_c[4]) = {
  VP9_FILTER_WEIGHT >> 1,
  VP9_FILTER_WEIGHT >> 1,
  VP9_FILTER_WEIGHT >> 1,
  VP9_FILTER_WEIGHT >> 1,
};
DECLARE_ALIGNED(16, static const unsigned char, transpose_c[16]) = {
  0, 4,  8, 12,
  1, 5,  9, 13,
  2, 6, 10, 14,
  3, 7, 11, 15
};

// Creating a macro to do more than four pixels at once to hide instruction
// latency is actually slower :-(
#define DO_FOUR_PIXELS(result, offset)                                         \
  {                                                                            \
  /*load pixels*/                                                              \
  __m128i src  = _mm_loadu_si128((const __m128i *)(src_ptr + offset));         \
  /* extract the ones used for first column */                                 \
  __m128i src0123 = _mm_shuffle_epi8(src, mask0123);                           \
  __m128i src4567 = _mm_shuffle_epi8(src, mask4567);                           \
  __m128i src01_16 = _mm_unpacklo_epi8(src0123, zero);                         \
  __m128i src23_16 = _mm_unpackhi_epi8(src0123, zero);                         \
  __m128i src45_16 = _mm_unpacklo_epi8(src4567, zero);                         \
  __m128i src67_16 = _mm_unpackhi_epi8(src4567, zero);                         \
  /* multiply accumulate them */                                               \
  __m128i mad01 = _mm_madd_epi16(src01_16, fil01);                             \
  __m128i mad23 = _mm_madd_epi16(src23_16, fil23);                             \
  __m128i mad45 = _mm_madd_epi16(src45_16, fil45);                             \
  __m128i mad67 = _mm_madd_epi16(src67_16, fil67);                             \
  __m128i mad0123 = _mm_add_epi32(mad01, mad23);                               \
  __m128i mad4567 = _mm_add_epi32(mad45, mad67);                               \
  __m128i mad_all = _mm_add_epi32(mad0123, mad4567);                           \
  mad_all = _mm_add_epi32(mad_all, rounding);                                  \
  result = _mm_srai_epi32(mad_all, VP9_FILTER_SHIFT);                          \
  }

void vp9_filter_block2d_4x4_8_sse4_1
(
 const unsigned char *src_ptr, const unsigned int src_stride,
 const short *HFilter_aligned16, const short *VFilter_aligned16,
 unsigned char *dst_ptr, unsigned int dst_stride
) {
  __m128i intermediateA, intermediateB, intermediateC;

  const int kInterp_Extend = 4;

  const __m128i zero = _mm_set1_epi16(0);
  const __m128i mask0123 = _mm_load_si128((const __m128i *)mask0123_c);
  const __m128i mask4567 = _mm_load_si128((const __m128i *)mask4567_c);
  const __m128i rounding = _mm_load_si128((const __m128i *)rounding_c);
  const __m128i transpose = _mm_load_si128((const __m128i *)transpose_c);

  // check alignment
  assert(0 == ((long)HFilter_aligned16)%16);
  assert(0 == ((long)VFilter_aligned16)%16);

  {
    __m128i transpose3_0;
    __m128i transpose3_1;
    __m128i transpose3_2;
    __m128i transpose3_3;

    // Horizontal pass (src -> intermediate).
    {
      const __m128i HFilter = _mm_load_si128((const __m128i *)HFilter_aligned16);
      // get first two columns filter coefficients
      __m128i fil01 = _mm_shuffle_epi32(HFilter, _MM_SHUFFLE(0, 0, 0, 0));
      __m128i fil23 = _mm_shuffle_epi32(HFilter, _MM_SHUFFLE(1, 1, 1, 1));
      __m128i fil45 = _mm_shuffle_epi32(HFilter, _MM_SHUFFLE(2, 2, 2, 2));
      __m128i fil67 = _mm_shuffle_epi32(HFilter, _MM_SHUFFLE(3, 3, 3, 3));
      src_ptr -= (kInterp_Extend - 1) * src_stride + (kInterp_Extend - 1);

      {
        __m128i mad_all0;
        __m128i mad_all1;
        __m128i mad_all2;
        __m128i mad_all3;
        DO_FOUR_PIXELS(mad_all0, 0*src_stride)
        DO_FOUR_PIXELS(mad_all1, 1*src_stride)
        DO_FOUR_PIXELS(mad_all2, 2*src_stride)
        DO_FOUR_PIXELS(mad_all3, 3*src_stride)
        mad_all0 = _mm_packs_epi32(mad_all0, mad_all1);
        mad_all2 = _mm_packs_epi32(mad_all2, mad_all3);
        intermediateA = _mm_packus_epi16(mad_all0, mad_all2);
        // --
        src_ptr += src_stride*4;
        // --
        DO_FOUR_PIXELS(mad_all0, 0*src_stride)
        DO_FOUR_PIXELS(mad_all1, 1*src_stride)
        DO_FOUR_PIXELS(mad_all2, 2*src_stride)
        DO_FOUR_PIXELS(mad_all3, 3*src_stride)
        mad_all0 = _mm_packs_epi32(mad_all0, mad_all1);
        mad_all2 = _mm_packs_epi32(mad_all2, mad_all3);
        intermediateB = _mm_packus_epi16(mad_all0, mad_all2);
        // --
        src_ptr += src_stride*4;
        // --
        DO_FOUR_PIXELS(mad_all0, 0*src_stride)
        DO_FOUR_PIXELS(mad_all1, 1*src_stride)
        DO_FOUR_PIXELS(mad_all2, 2*src_stride)
        mad_all0 = _mm_packs_epi32(mad_all0, mad_all1);
        mad_all2 = _mm_packs_epi32(mad_all2, mad_all2);
        intermediateC = _mm_packus_epi16(mad_all0, mad_all2);
      }
    }

    // Transpose result (intermediate -> transpose3_x)
    {
      // 00 01 02 03 10 11 12 13 20 21 22 23 30 31 32 33
      // 40 41 42 43 50 51 52 53 60 61 62 63 70 71 72 73
      // 80 81 82 83 90 91 92 93 A0 A1 A2 A3 xx xx xx xx
      const __m128i transpose1_0 = _mm_shuffle_epi8(intermediateA, transpose);
      const __m128i transpose1_1 = _mm_shuffle_epi8(intermediateB, transpose);
      const __m128i transpose1_2 = _mm_shuffle_epi8(intermediateC, transpose);
      // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
      // 40 50 60 70 41 51 61 71 42 52 62 72 43 53 63 73
      // 80 90 A0 xx 81 91 A1 xx 82 92 A2 xx 83 93 A3 xx
      const __m128i transpose2_0 = _mm_unpacklo_epi32(transpose1_0, transpose1_1);
      const __m128i transpose2_1 = _mm_unpackhi_epi32(transpose1_0, transpose1_1);
      // 00 10 20 30 40 50 60 70 01 11 21 31 41 51 61 71
      // 02 12 22 32 42 52 62 72 03 13 23 33 43 53 63 73
      transpose3_0 = _mm_castps_si128(
                            _mm_shuffle_ps(_mm_castsi128_ps(transpose2_0),
                                           _mm_castsi128_ps(transpose1_2),
                                           _MM_SHUFFLE(0, 0, 1, 0)));
      transpose3_1 = _mm_castps_si128(
                            _mm_shuffle_ps(_mm_castsi128_ps(transpose2_0),
                                           _mm_castsi128_ps(transpose1_2),
                                           _MM_SHUFFLE(1, 1, 3, 2)));
      transpose3_2 = _mm_castps_si128(
                            _mm_shuffle_ps(_mm_castsi128_ps(transpose2_1),
                                           _mm_castsi128_ps(transpose1_2),
                                           _MM_SHUFFLE(2, 2, 1, 0)));
      transpose3_3 = _mm_castps_si128(
                            _mm_shuffle_ps(_mm_castsi128_ps(transpose2_1),
                                           _mm_castsi128_ps(transpose1_2),
                                           _MM_SHUFFLE(3, 3, 3, 2)));
      // 00 10 20 30 40 50 60 70 80 90 A0 xx xx xx xx xx
      // 01 11 21 31 41 51 61 71 81 91 A1 xx xx xx xx xx
      // 02 12 22 32 42 52 62 72 82 92 A2 xx xx xx xx xx
      // 03 13 23 33 43 53 63 73 83 93 A3 xx xx xx xx xx
    }

    // Vertical pass (transpose3_x -> dst).
    {
      const __m128i VFilter = _mm_load_si128((const __m128i *)VFilter_aligned16);
      // get first two columns filter coefficients
      __m128i fil01 = _mm_shuffle_epi32(VFilter, _MM_SHUFFLE(0, 0, 0, 0));
      __m128i fil23 = _mm_shuffle_epi32(VFilter, _MM_SHUFFLE(1, 1, 1, 1));
      __m128i fil45 = _mm_shuffle_epi32(VFilter, _MM_SHUFFLE(2, 2, 2, 2));
      __m128i fil67 = _mm_shuffle_epi32(VFilter, _MM_SHUFFLE(3, 3, 3, 3));
      __m128i col0, col1, col2, col3;
      {
        //load pixels
        __m128i src  = transpose3_0;
        // extract the ones used for first column
        __m128i src0123 = _mm_shuffle_epi8(src, mask0123);
        __m128i src4567 = _mm_shuffle_epi8(src, mask4567);
        __m128i src01_16 = _mm_unpacklo_epi8(src0123, zero);
        __m128i src23_16 = _mm_unpackhi_epi8(src0123, zero);
        __m128i src45_16 = _mm_unpacklo_epi8(src4567, zero);
        __m128i src67_16 = _mm_unpackhi_epi8(src4567, zero);
        // multiply accumulate them
        __m128i mad01 = _mm_madd_epi16(src01_16, fil01);
        __m128i mad23 = _mm_madd_epi16(src23_16, fil23);
        __m128i mad45 = _mm_madd_epi16(src45_16, fil45);
        __m128i mad67 = _mm_madd_epi16(src67_16, fil67);
        __m128i mad0123 = _mm_add_epi32(mad01, mad23);
        __m128i mad4567 = _mm_add_epi32(mad45, mad67);
        __m128i mad_all = _mm_add_epi32(mad0123, mad4567);
        mad_all = _mm_add_epi32(mad_all, rounding);
        mad_all = _mm_srai_epi32(mad_all, VP9_FILTER_SHIFT);
        mad_all = _mm_packs_epi32(mad_all, mad_all);
        col0 = _mm_packus_epi16(mad_all, mad_all);
      }
      {
        //load pixels
        __m128i src  = transpose3_1;
        // extract the ones used for first column
        __m128i src0123 = _mm_shuffle_epi8(src, mask0123);
        __m128i src4567 = _mm_shuffle_epi8(src, mask4567);
        __m128i src01_16 = _mm_unpacklo_epi8(src0123, zero);
        __m128i src23_16 = _mm_unpackhi_epi8(src0123, zero);
        __m128i src45_16 = _mm_unpacklo_epi8(src4567, zero);
        __m128i src67_16 = _mm_unpackhi_epi8(src4567, zero);
        // multiply accumulate them
        __m128i mad01 = _mm_madd_epi16(src01_16, fil01);
        __m128i mad23 = _mm_madd_epi16(src23_16, fil23);
        __m128i mad45 = _mm_madd_epi16(src45_16, fil45);
        __m128i mad67 = _mm_madd_epi16(src67_16, fil67);
        __m128i mad0123 = _mm_add_epi32(mad01, mad23);
        __m128i mad4567 = _mm_add_epi32(mad45, mad67);
        __m128i mad_all = _mm_add_epi32(mad0123, mad4567);
        mad_all = _mm_add_epi32(mad_all, rounding);
        mad_all = _mm_srai_epi32(mad_all, VP9_FILTER_SHIFT);
        mad_all = _mm_packs_epi32(mad_all, mad_all);
        col1 = _mm_packus_epi16(mad_all, mad_all);
      }
      {
        //load pixels
        __m128i src  = transpose3_2;
        // extract the ones used for first column
        __m128i src0123 = _mm_shuffle_epi8(src, mask0123);
        __m128i src4567 = _mm_shuffle_epi8(src, mask4567);
        __m128i src01_16 = _mm_unpacklo_epi8(src0123, zero);
        __m128i src23_16 = _mm_unpackhi_epi8(src0123, zero);
        __m128i src45_16 = _mm_unpacklo_epi8(src4567, zero);
        __m128i src67_16 = _mm_unpackhi_epi8(src4567, zero);
        // multiply accumulate them
        __m128i mad01 = _mm_madd_epi16(src01_16, fil01);
        __m128i mad23 = _mm_madd_epi16(src23_16, fil23);
        __m128i mad45 = _mm_madd_epi16(src45_16, fil45);
        __m128i mad67 = _mm_madd_epi16(src67_16, fil67);
        __m128i mad0123 = _mm_add_epi32(mad01, mad23);
        __m128i mad4567 = _mm_add_epi32(mad45, mad67);
        __m128i mad_all = _mm_add_epi32(mad0123, mad4567);
        mad_all = _mm_add_epi32(mad_all, rounding);
        mad_all = _mm_srai_epi32(mad_all, VP9_FILTER_SHIFT);
        mad_all = _mm_packs_epi32(mad_all, mad_all);
        col2 = _mm_packus_epi16(mad_all, mad_all);
      }
      {
        //load pixels
        __m128i src  = transpose3_3;
        // extract the ones used for first column
        __m128i src0123 = _mm_shuffle_epi8(src, mask0123);
        __m128i src4567 = _mm_shuffle_epi8(src, mask4567);
        __m128i src01_16 = _mm_unpacklo_epi8(src0123, zero);
        __m128i src23_16 = _mm_unpackhi_epi8(src0123, zero);
        __m128i src45_16 = _mm_unpacklo_epi8(src4567, zero);
        __m128i src67_16 = _mm_unpackhi_epi8(src4567, zero);
        // multiply accumulate them
        __m128i mad01 = _mm_madd_epi16(src01_16, fil01);
        __m128i mad23 = _mm_madd_epi16(src23_16, fil23);
        __m128i mad45 = _mm_madd_epi16(src45_16, fil45);
        __m128i mad67 = _mm_madd_epi16(src67_16, fil67);
        __m128i mad0123 = _mm_add_epi32(mad01, mad23);
        __m128i mad4567 = _mm_add_epi32(mad45, mad67);
        __m128i mad_all = _mm_add_epi32(mad0123, mad4567);
        mad_all = _mm_add_epi32(mad_all, rounding);
        mad_all = _mm_srai_epi32(mad_all, VP9_FILTER_SHIFT);
        mad_all = _mm_packs_epi32(mad_all, mad_all);
        col3 = _mm_packus_epi16(mad_all, mad_all);
      }
      {
        __m128i col01 = _mm_unpacklo_epi8(col0, col1);
        __m128i col23 = _mm_unpacklo_epi8(col2, col3);
        __m128i col0123 = _mm_unpacklo_epi16(col01, col23);
        //TODO(cd): look into Ronald's comment:
        //    Future suggestion: I believe here, too, you can merge the
        //    packs_epi32() and pacus_epi16() for the 4 cols above, so that
        //    you get the data in a single register, and then use pshufb
        //    (shuffle_epi8()) instead of the unpacks here. Should be
        //    2+3+2 instructions faster.
        *((unsigned int *)&dst_ptr[dst_stride * 0]) =
            _mm_extract_epi32(col0123, 0);
        *((unsigned int *)&dst_ptr[dst_stride * 1]) =
            _mm_extract_epi32(col0123, 1);
        *((unsigned int *)&dst_ptr[dst_stride * 2]) =
            _mm_extract_epi32(col0123, 2);
        *((unsigned int *)&dst_ptr[dst_stride * 3]) =
            _mm_extract_epi32(col0123, 3);
      }
    }
  }
}

void vp9_filter_block2d_8x4_8_sse4_1
(
 const unsigned char *src_ptr, const unsigned int src_stride,
 const short *HFilter_aligned16, const short *VFilter_aligned16,
 unsigned char *dst_ptr, unsigned int dst_stride
) {
  int j;
  for (j=0; j<8; j+=4) {
    vp9_filter_block2d_4x4_8_sse4_1(src_ptr + j, src_stride,
                                    HFilter_aligned16, VFilter_aligned16,
                                    dst_ptr + j, dst_stride);
  }
}

void vp9_filter_block2d_8x8_8_sse4_1
(
 const unsigned char *src_ptr, const unsigned int src_stride,
 const short *HFilter_aligned16, const short *VFilter_aligned16,
 unsigned char *dst_ptr, unsigned int dst_stride
) {
  int i, j;
  for (i=0; i<8; i+=4) {
    for (j=0; j<8; j+=4) {
      vp9_filter_block2d_4x4_8_sse4_1(src_ptr + j + i*src_stride, src_stride,
                                      HFilter_aligned16, VFilter_aligned16,
                                      dst_ptr + j + i*dst_stride, dst_stride);
    }
  }
}

void vp9_filter_block2d_16x16_8_sse4_1
(
 const unsigned char *src_ptr, const unsigned int src_stride,
 const short *HFilter_aligned16, const short *VFilter_aligned16,
 unsigned char *dst_ptr, unsigned int dst_stride
) {
  int i, j;
  for (i=0; i<16; i+=4) {
    for (j=0; j<16; j+=4) {
      vp9_filter_block2d_4x4_8_sse4_1(src_ptr + j + i*src_stride, src_stride,
                                      HFilter_aligned16, VFilter_aligned16,
                                      dst_ptr + j + i*dst_stride, dst_stride);
    }
  }
}
