/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


extern "C" {
#include "./vpx_config.h"
#include "./vp9_rtcd.h"
#include "vp9/common/vp9_filter.h"
#include "vpx_mem/vpx_mem.h"
#include "vpx_ports/mem.h"
}
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "test/acm_random.h"
#include "test/register_state_check.h"
#include "test/util.h"

namespace {
typedef void (*convolve_fn_t)(const uint8_t *src, int src_stride,
                              uint8_t *dst, int dst_stride,
                              const int16_t *filter_x, int filter_x_stride,
                              const int16_t *filter_y, int filter_y_stride,
                              int w, int h);

struct ConvolveFunctions {
  ConvolveFunctions(convolve_fn_t h8, convolve_fn_t h8_avg,
                    convolve_fn_t v8, convolve_fn_t v8_avg,
                    convolve_fn_t hv8, convolve_fn_t hv8_avg)
      : h8_(h8), v8_(v8), hv8_(hv8), h8_avg_(h8_avg), v8_avg_(v8_avg),
        hv8_avg_(hv8_avg) {}

  convolve_fn_t h8_;
  convolve_fn_t v8_;
  convolve_fn_t hv8_;
  convolve_fn_t h8_avg_;
  convolve_fn_t v8_avg_;
  convolve_fn_t hv8_avg_;
};

// Reference 8-tap subpixel filter, slightly modified to fit into this test.
#define VP9_FILTER_WEIGHT 128
#define VP9_FILTER_SHIFT 7
static uint8_t clip_pixel(int x) {
  return x < 0 ? 0 :
         x > 255 ? 255 :
         x;
}

static void filter_block2d_8_c(const uint8_t *src_ptr,
                               const unsigned int src_stride,
                               const int16_t *HFilter,
                               const int16_t *VFilter,
                               uint8_t *dst_ptr,
                               unsigned int dst_stride,
                               unsigned int output_width,
                               unsigned int output_height) {
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
    for (i = 0; i < intermediate_height; ++i) {
      for (j = 0; j < output_width; ++j) {
        // Apply filter...
        int temp = ((int)src_ptr[0] * HFilter[0]) +
                   ((int)src_ptr[1] * HFilter[1]) +
                   ((int)src_ptr[2] * HFilter[2]) +
                   ((int)src_ptr[3] * HFilter[3]) +
                   ((int)src_ptr[4] * HFilter[4]) +
                   ((int)src_ptr[5] * HFilter[5]) +
                   ((int)src_ptr[6] * HFilter[6]) +
                   ((int)src_ptr[7] * HFilter[7]) +
                   (VP9_FILTER_WEIGHT >> 1);  // Rounding

        // Normalize back to 0-255...
        *output_ptr = clip_pixel(temp >> VP9_FILTER_SHIFT);
        ++src_ptr;
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
    for (i = 0; i < output_height; ++i) {
      for (j = 0; j < output_width; ++j) {
        // Apply filter...
        int temp = ((int)src_ptr[0] * VFilter[0]) +
                   ((int)src_ptr[1] * VFilter[1]) +
                   ((int)src_ptr[2] * VFilter[2]) +
                   ((int)src_ptr[3] * VFilter[3]) +
                   ((int)src_ptr[4] * VFilter[4]) +
                   ((int)src_ptr[5] * VFilter[5]) +
                   ((int)src_ptr[6] * VFilter[6]) +
                   ((int)src_ptr[7] * VFilter[7]) +
                   (VP9_FILTER_WEIGHT >> 1);  // Rounding

        // Normalize back to 0-255...
        *dst_ptr++ = clip_pixel(temp >> VP9_FILTER_SHIFT);
        src_ptr += intermediate_height;
      }
      src_ptr += intermediate_next_stride;
      dst_ptr += dst_next_row_stride;
    }
  }
}

static void block2d_average_c(uint8_t *src,
                              unsigned int src_stride,
                              uint8_t *output_ptr,
                              unsigned int output_stride,
                              unsigned int output_width,
                              unsigned int output_height) {
  unsigned int i, j;
  for (i = 0; i < output_height; ++i) {
    for (j = 0; j < output_width; ++j) {
      output_ptr[j] = (output_ptr[j] + src[i * src_stride + j] + 1) >> 1;
    }
    output_ptr += output_stride;
  }
}

static void filter_average_block2d_8_c(const uint8_t *src_ptr,
                                       const unsigned int src_stride,
                                       const int16_t *HFilter,
                                       const int16_t *VFilter,
                                       uint8_t *dst_ptr,
                                       unsigned int dst_stride,
                                       unsigned int output_width,
                                       unsigned int output_height) {
  uint8_t tmp[16*16];

  assert(output_width <= 16);
  assert(output_height <= 16);
  filter_block2d_8_c(src_ptr, src_stride, HFilter, VFilter, tmp, 16,
                     output_width, output_height);
  block2d_average_c(tmp, 16, dst_ptr, dst_stride,
                    output_width, output_height);
}

class ConvolveTest : public PARAMS(int, int, const ConvolveFunctions*) {
 public:
  static void SetUpTestCase() {
    // Force input_ to be unaligned, output to be 16 byte aligned.
    input_ = reinterpret_cast<uint8_t*>(
        vpx_memalign(kDataAlignment, kOuterBlockSize * kOuterBlockSize + 1))
        + 1;
    output_ = reinterpret_cast<uint8_t*>(
        vpx_memalign(kDataAlignment, kOuterBlockSize * kOuterBlockSize));
  }

  static void TearDownTestCase() {
    vpx_free(input_ - 1);
    input_ = NULL;
    vpx_free(output_);
    output_ = NULL;
  }

  protected:
    static const int kDataAlignment = 16;
    static const int kOuterBlockSize = 32;
    static const int kInputStride = kOuterBlockSize;
    static const int kOutputStride = kOuterBlockSize;
    static const int kMaxDimension = 16;

    int Width() const { return GET_PARAM(0); }
    int Height() const { return GET_PARAM(1); }
    int BorderLeft() const {
      const int center = (kOuterBlockSize - Width()) / 2;
      return (center + (kDataAlignment - 1)) & ~(kDataAlignment - 1);
    }
    int BorderTop() const { return (kOuterBlockSize - Height()) / 2; }

    bool IsIndexInBorder(int i) {
      return (i < BorderTop() * kOuterBlockSize ||
              i >= (BorderTop() + Height()) * kOuterBlockSize ||
              i % kOuterBlockSize < BorderLeft() ||
              i % kOuterBlockSize >= (BorderLeft() + Width()));
    }

    virtual void SetUp() {
      UUT_ = GET_PARAM(2);
      memset(input_, 0, sizeof(input_));
      /* Set up guard blocks for an inner block cetered in the outer block */
      for (int i = 0; i < kOuterBlockSize * kOuterBlockSize; ++i) {
        if (IsIndexInBorder(i))
          output_[i] = 255;
        else
          output_[i] = 0;
      }

      ::libvpx_test::ACMRandom prng;
      for (int i = 0; i < kOuterBlockSize * kOuterBlockSize; ++i)
        input_[i] = prng.Rand8();
    }

    void CheckGuardBlocks() {
      for (int i = 0; i < kOuterBlockSize * kOuterBlockSize; ++i) {
        if (IsIndexInBorder(i))
          EXPECT_EQ(255, output_[i]);
      }
    }

    uint8_t* input() {
      return input_ + BorderTop() * kOuterBlockSize + BorderLeft();
    }

    uint8_t* output() {
      return output_ + BorderTop() * kOuterBlockSize + BorderLeft();
    }

    const ConvolveFunctions* UUT_;
    static uint8_t* input_;
    static uint8_t* output_;
};
uint8_t* ConvolveTest::input_ = NULL;
uint8_t* ConvolveTest::output_ = NULL;

TEST_P(ConvolveTest, GuardBlocks) {
  CheckGuardBlocks();
}

TEST_P(ConvolveTest, CopyHoriz) {
  uint8_t* const in = input();
  uint8_t* const out = output();
  const int16_t filter8[8] = {0, 0, 0, 128, 0, 0, 0, 0};

  REGISTER_STATE_CHECK(
      UUT_->h8_(in, kInputStride, out, kOutputStride, filter8, 16, filter8, 16,
                Width(), Height()));

  CheckGuardBlocks();

  for (int y = 0; y < Height(); ++y)
    for (int x = 0; x < Width(); ++x)
      ASSERT_EQ(out[y * kOutputStride + x], in[y * kInputStride + x])
          << "(" << x << "," << y << ")";
}

TEST_P(ConvolveTest, CopyVert) {
  uint8_t* const in = input();
  uint8_t* const out = output();
  const int16_t filter8[8] = {0, 0, 0, 128, 0, 0, 0, 0};

  REGISTER_STATE_CHECK(
      UUT_->v8_(in, kInputStride, out, kOutputStride, filter8, 16, filter8, 16,
                Width(), Height()));

  CheckGuardBlocks();

  for (int y = 0; y < Height(); ++y)
    for (int x = 0; x < Width(); ++x)
      ASSERT_EQ(out[y * kOutputStride + x], in[y * kInputStride + x])
          << "(" << x << "," << y << ")";
}

TEST_P(ConvolveTest, Copy2D) {
  uint8_t* const in = input();
  uint8_t* const out = output();
  const int16_t filter8[8] = {0, 0, 0, 128, 0, 0, 0, 0};

  REGISTER_STATE_CHECK(
      UUT_->hv8_(in, kInputStride, out, kOutputStride, filter8, 16, filter8, 16,
                 Width(), Height()));

  CheckGuardBlocks();

  for (int y = 0; y < Height(); ++y)
    for (int x = 0; x < Width(); ++x)
      ASSERT_EQ(out[y * kOutputStride + x], in[y * kInputStride + x])
          << "(" << x << "," << y << ")";
}

const int16_t (*kTestFilterList[])[8] = {
  vp9_bilinear_filters,
  vp9_sub_pel_filters_6,
  vp9_sub_pel_filters_8,
  vp9_sub_pel_filters_8s,
  vp9_sub_pel_filters_8lp
};

const int16_t kInvalidFilter[8] = { 0 };

TEST_P(ConvolveTest, MatchesReferenceSubpixelFilter) {
  uint8_t* const in = input();
  uint8_t* const out = output();
  uint8_t ref[kOutputStride * kMaxDimension];

  const int kNumFilterBanks = sizeof(kTestFilterList) /
      sizeof(kTestFilterList[0]);

  for (int filter_bank = 0; filter_bank < kNumFilterBanks; ++filter_bank) {
    const int16_t (*filters)[8] = kTestFilterList[filter_bank];
    const int kNumFilters = 16;

    for (int filter_x = 0; filter_x < kNumFilters; ++filter_x) {
      for (int filter_y = 0; filter_y < kNumFilters; ++filter_y) {
        filter_block2d_8_c(in, kInputStride,
                           filters[filter_x], filters[filter_y],
                           ref, kOutputStride,
                           Width(), Height());

        if (filters == vp9_sub_pel_filters_8lp || (filter_x && filter_y))
          REGISTER_STATE_CHECK(
              UUT_->hv8_(in, kInputStride, out, kOutputStride,
                         filters[filter_x], 16, filters[filter_y], 16,
                         Width(), Height()));
        else if (filter_y)
          REGISTER_STATE_CHECK(
              UUT_->v8_(in, kInputStride, out, kOutputStride,
                        kInvalidFilter, 16, filters[filter_y], 16,
                        Width(), Height()));
        else
          REGISTER_STATE_CHECK(
              UUT_->h8_(in, kInputStride, out, kOutputStride,
                        filters[filter_x], 16, kInvalidFilter, 16,
                        Width(), Height()));

        CheckGuardBlocks();

        for (int y = 0; y < Height(); ++y)
          for (int x = 0; x < Width(); ++x)
            ASSERT_EQ(ref[y * kOutputStride + x], out[y * kOutputStride + x])
                << "mismatch at (" << x << "," << y << "), "
                << "filters (" << filter_bank << ","
                << filter_x << "," << filter_y << ")";
      }
    }
  }
}

TEST_P(ConvolveTest, MatchesReferenceAveragingSubpixelFilter) {
  uint8_t* const in = input();
  uint8_t* const out = output();
  uint8_t ref[kOutputStride * kMaxDimension];

  // Populate ref and out with some random data
  ::libvpx_test::ACMRandom prng;
  for (int y = 0; y < Height(); ++y) {
    for (int x = 0; x < Width(); ++x) {
      const uint8_t r = prng.Rand8();

      out[y * kOutputStride + x] = r;
      ref[y * kOutputStride + x] = r;
    }
  }

  const int kNumFilterBanks = sizeof(kTestFilterList) /
      sizeof(kTestFilterList[0]);

  for (int filter_bank = 0; filter_bank < kNumFilterBanks; ++filter_bank) {
    const int16_t (*filters)[8] = kTestFilterList[filter_bank];
    const int kNumFilters = 16;

    for (int filter_x = 0; filter_x < kNumFilters; ++filter_x) {
      for (int filter_y = 0; filter_y < kNumFilters; ++filter_y) {
        filter_average_block2d_8_c(in, kInputStride,
                                   filters[filter_x], filters[filter_y],
                                   ref, kOutputStride,
                                   Width(), Height());

        if (filters == vp9_sub_pel_filters_8lp || (filter_x && filter_y))
          REGISTER_STATE_CHECK(
              UUT_->hv8_avg_(in, kInputStride, out, kOutputStride,
                             filters[filter_x], 16, filters[filter_y], 16,
                             Width(), Height()));
        else if (filter_y)
          REGISTER_STATE_CHECK(
              UUT_->v8_avg_(in, kInputStride, out, kOutputStride,
                            filters[filter_x], 16, filters[filter_y], 16,
                            Width(), Height()));
        else
          REGISTER_STATE_CHECK(
              UUT_->h8_avg_(in, kInputStride, out, kOutputStride,
                            filters[filter_x], 16, filters[filter_y], 16,
                            Width(), Height()));

        CheckGuardBlocks();

        for (int y = 0; y < Height(); ++y)
          for (int x = 0; x < Width(); ++x)
            ASSERT_EQ(ref[y * kOutputStride + x], out[y * kOutputStride + x])
                << "mismatch at (" << x << "," << y << "), "
                << "filters (" << filter_bank << ","
                << filter_x << "," << filter_y << ")";
      }
    }
  }
}

DECLARE_ALIGNED(256, const int16_t, kChangeFilters[16][8]) = {
    { 0,   0,   0,   0,   0,   0,   0, 128},
    { 0,   0,   0,   0,   0,   0, 128},
    { 0,   0,   0,   0,   0, 128},
    { 0,   0,   0,   0, 128},
    { 0,   0,   0, 128},
    { 0,   0, 128},
    { 0, 128},
    { 128},
    { 0,   0,   0,   0,   0,   0,   0, 128},
    { 0,   0,   0,   0,   0,   0, 128},
    { 0,   0,   0,   0,   0, 128},
    { 0,   0,   0,   0, 128},
    { 0,   0,   0, 128},
    { 0,   0, 128},
    { 0, 128},
    { 128}
};

TEST_P(ConvolveTest, ChangeFilterWorks) {
  uint8_t* const in = input();
  uint8_t* const out = output();

  REGISTER_STATE_CHECK(UUT_->h8_(in, kInputStride, out, kOutputStride,
                                 kChangeFilters[8], 17, kChangeFilters[4], 16,
                                 Width(), Height()));

  for (int x = 0; x < Width(); ++x) {
    if (x < 8)
      ASSERT_EQ(in[4], out[x]) << "x == " << x;
    else
      ASSERT_EQ(in[12], out[x]) << "x == " << x;
  }

  REGISTER_STATE_CHECK(UUT_->v8_(in, kInputStride, out, kOutputStride,
                                 kChangeFilters[4], 16, kChangeFilters[8], 17,
                                 Width(), Height()));

  for (int y = 0; y < Height(); ++y) {
    if (y < 8)
      ASSERT_EQ(in[4 * kInputStride], out[y * kOutputStride]) << "y == " << y;
    else
      ASSERT_EQ(in[12 * kInputStride], out[y * kOutputStride]) << "y == " << y;
  }

  REGISTER_STATE_CHECK(UUT_->hv8_(in, kInputStride, out, kOutputStride,
                                  kChangeFilters[8], 17, kChangeFilters[8], 17,
                                  Width(), Height()));

  for (int y = 0; y < Height(); ++y) {
    for (int x = 0; x < Width(); ++x) {
      const int ref_x = x < 8 ? 4 : 12;
      const int ref_y = y < 8 ? 4 : 12;

      ASSERT_EQ(in[ref_y * kInputStride + ref_x], out[y * kOutputStride + x])
          << "x == " << x << ", y == " << y;
    }
  }
}


using std::tr1::make_tuple;

const ConvolveFunctions convolve8_c(
    vp9_convolve8_horiz_c, vp9_convolve8_avg_horiz_c,
    vp9_convolve8_vert_c, vp9_convolve8_avg_vert_c,
    vp9_convolve8_c, vp9_convolve8_avg_c);

INSTANTIATE_TEST_CASE_P(C, ConvolveTest, ::testing::Values(
    make_tuple(4, 4, &convolve8_c),
    make_tuple(8, 4, &convolve8_c),
    make_tuple(8, 8, &convolve8_c),
    make_tuple(16, 8, &convolve8_c),
    make_tuple(16, 16, &convolve8_c)));
}

#if HAVE_SSSE3
const ConvolveFunctions convolve8_ssse3(
    vp9_convolve8_horiz_ssse3, vp9_convolve8_avg_horiz_c,
    vp9_convolve8_vert_ssse3, vp9_convolve8_avg_vert_c,
    vp9_convolve8_ssse3, vp9_convolve8_avg_c);

INSTANTIATE_TEST_CASE_P(SSSE3, ConvolveTest, ::testing::Values(
    make_tuple(4, 4, &convolve8_ssse3),
    make_tuple(8, 4, &convolve8_ssse3),
    make_tuple(8, 8, &convolve8_ssse3),
    make_tuple(16, 8, &convolve8_ssse3),
    make_tuple(16, 16, &convolve8_ssse3)));
#endif
