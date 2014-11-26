/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <stdlib.h>
#include <new>

#include "third_party/googletest/src/include/gtest/gtest.h"

#include "vpx/vpx_integer.h"
#include "vpx_config.h"
extern "C" {
#if CONFIG_VP8_ENCODER
# include "vp8/common/variance.h"
# include "vp8_rtcd.h"
#endif
#if CONFIG_VP9_ENCODER
# include "vp9/encoder/vp9_variance.h"
# include "vp9_rtcd.h"
#endif
}

namespace {

using ::std::tr1::get;
using ::std::tr1::make_tuple;
using ::std::tr1::tuple;

template<typename VarianceFunctionType>
class VarianceTest :
    public ::testing::TestWithParam<tuple<int, int, VarianceFunctionType> > {
 public:
  virtual void SetUp() {
    const tuple<int, int, VarianceFunctionType>& params = this->GetParam();
    width_  = get<0>(params);
    height_ = get<1>(params);
    variance_ = get<2>(params);

    block_size_ = width_ * height_;
    src_ = new uint8_t[block_size_];
    ref_ = new uint8_t[block_size_];
    ASSERT_TRUE(src_ != NULL);
    ASSERT_TRUE(ref_ != NULL);
  }

  virtual void TearDown() {
    delete[] src_;
    delete[] ref_;
  }

 protected:
  void ZeroTest();
  void OneQuarterTest();

  uint8_t* src_;
  uint8_t* ref_;
  int width_;
  int height_;
  int block_size_;
  VarianceFunctionType variance_;

};

template<typename VarianceFunctionType>
void VarianceTest<VarianceFunctionType>::ZeroTest() {
  for (int i = 0; i <= 255; ++i) {
    memset(src_, i, block_size_);
    for (int j = 0; j <= 255; ++j) {
      memset(ref_, j, block_size_);
      unsigned int sse;
      const unsigned int var = variance_(src_, width_, ref_, width_, &sse);
      EXPECT_EQ(0u, var) << "src values: " << i << "ref values: " << j;
    }
  }
}

template<typename VarianceFunctionType>
void VarianceTest<VarianceFunctionType>::OneQuarterTest() {
  memset(src_, 255, block_size_);
  const int half = block_size_ / 2;
  memset(ref_, 255, half);
  memset(ref_ + half, 0, half);
  unsigned int sse;
  const unsigned int var = variance_(src_, width_, ref_, width_, &sse);
  const unsigned int expected = block_size_ * 255 * 255 / 4;
  EXPECT_EQ(expected, var);
}

// -----------------------------------------------------------------------------
// VP8 test cases.

namespace vp8 {

#if CONFIG_VP8_ENCODER
typedef VarianceTest<vp8_variance_fn_t> VP8VarianceTest;

TEST_P(VP8VarianceTest, Zero) { ZeroTest(); }
TEST_P(VP8VarianceTest, OneQuarter) { OneQuarterTest(); }

const vp8_variance_fn_t variance4x4_c = vp8_variance4x4_c;
const vp8_variance_fn_t variance8x8_c = vp8_variance8x8_c;
const vp8_variance_fn_t variance8x16_c = vp8_variance8x16_c;
const vp8_variance_fn_t variance16x8_c = vp8_variance16x8_c;
const vp8_variance_fn_t variance16x16_c = vp8_variance16x16_c;
INSTANTIATE_TEST_CASE_P(
    C, VP8VarianceTest,
    ::testing::Values(make_tuple(4, 4, variance4x4_c),
                      make_tuple(8, 8, variance8x8_c),
                      make_tuple(8, 16, variance8x16_c),
                      make_tuple(16, 8, variance16x8_c),
                      make_tuple(16, 16, variance16x16_c)));

#if HAVE_MMX
const vp8_variance_fn_t variance4x4_mmx = vp8_variance4x4_mmx;
const vp8_variance_fn_t variance8x8_mmx = vp8_variance8x8_mmx;
const vp8_variance_fn_t variance8x16_mmx = vp8_variance8x16_mmx;
const vp8_variance_fn_t variance16x8_mmx = vp8_variance16x8_mmx;
const vp8_variance_fn_t variance16x16_mmx = vp8_variance16x16_mmx;
INSTANTIATE_TEST_CASE_P(
    MMX, VP8VarianceTest,
    ::testing::Values(make_tuple(4, 4, variance4x4_mmx),
                      make_tuple(8, 8, variance8x8_mmx),
                      make_tuple(8, 16, variance8x16_mmx),
                      make_tuple(16, 8, variance16x8_mmx),
                      make_tuple(16, 16, variance16x16_mmx)));
#endif

#if HAVE_SSE2
const vp8_variance_fn_t variance4x4_wmt = vp8_variance4x4_wmt;
const vp8_variance_fn_t variance8x8_wmt = vp8_variance8x8_wmt;
const vp8_variance_fn_t variance8x16_wmt = vp8_variance8x16_wmt;
const vp8_variance_fn_t variance16x8_wmt = vp8_variance16x8_wmt;
const vp8_variance_fn_t variance16x16_wmt = vp8_variance16x16_wmt;
INSTANTIATE_TEST_CASE_P(
    SSE2, VP8VarianceTest,
    ::testing::Values(make_tuple(4, 4, variance4x4_wmt),
                      make_tuple(8, 8, variance8x8_wmt),
                      make_tuple(8, 16, variance8x16_wmt),
                      make_tuple(16, 8, variance16x8_wmt),
                      make_tuple(16, 16, variance16x16_wmt)));
#endif
#endif  // CONFIG_VP8_ENCODER

}  // namespace vp8

// -----------------------------------------------------------------------------
// VP9 test cases.

namespace vp9 {

#if CONFIG_VP9_ENCODER
typedef VarianceTest<vp9_variance_fn_t> VP9VarianceTest;

TEST_P(VP9VarianceTest, Zero) { ZeroTest(); }
TEST_P(VP9VarianceTest, OneQuarter) { OneQuarterTest(); }

const vp9_variance_fn_t variance4x4_c = vp9_variance4x4_c;
const vp9_variance_fn_t variance8x8_c = vp9_variance8x8_c;
const vp9_variance_fn_t variance8x16_c = vp9_variance8x16_c;
const vp9_variance_fn_t variance16x8_c = vp9_variance16x8_c;
const vp9_variance_fn_t variance16x16_c = vp9_variance16x16_c;
INSTANTIATE_TEST_CASE_P(
    C, VP9VarianceTest,
    ::testing::Values(make_tuple(4, 4, variance4x4_c),
                      make_tuple(8, 8, variance8x8_c),
                      make_tuple(8, 16, variance8x16_c),
                      make_tuple(16, 8, variance16x8_c),
                      make_tuple(16, 16, variance16x16_c)));

#if HAVE_MMX
const vp9_variance_fn_t variance4x4_mmx = vp9_variance4x4_mmx;
const vp9_variance_fn_t variance8x8_mmx = vp9_variance8x8_mmx;
const vp9_variance_fn_t variance8x16_mmx = vp9_variance8x16_mmx;
const vp9_variance_fn_t variance16x8_mmx = vp9_variance16x8_mmx;
const vp9_variance_fn_t variance16x16_mmx = vp9_variance16x16_mmx;
INSTANTIATE_TEST_CASE_P(
    MMX, VP9VarianceTest,
    ::testing::Values(make_tuple(4, 4, variance4x4_mmx),
                      make_tuple(8, 8, variance8x8_mmx),
                      make_tuple(8, 16, variance8x16_mmx),
                      make_tuple(16, 8, variance16x8_mmx),
                      make_tuple(16, 16, variance16x16_mmx)));
#endif

#if HAVE_SSE2
const vp9_variance_fn_t variance4x4_wmt = vp9_variance4x4_wmt;
const vp9_variance_fn_t variance8x8_wmt = vp9_variance8x8_wmt;
const vp9_variance_fn_t variance8x16_wmt = vp9_variance8x16_wmt;
const vp9_variance_fn_t variance16x8_wmt = vp9_variance16x8_wmt;
const vp9_variance_fn_t variance16x16_wmt = vp9_variance16x16_wmt;
INSTANTIATE_TEST_CASE_P(
    SSE2, VP9VarianceTest,
    ::testing::Values(make_tuple(4, 4, variance4x4_wmt),
                      make_tuple(8, 8, variance8x8_wmt),
                      make_tuple(8, 16, variance8x16_wmt),
                      make_tuple(16, 8, variance16x8_wmt),
                      make_tuple(16, 16, variance16x16_wmt)));
#endif
#endif  // CONFIG_VP9_ENCODER

}  // namespace vp9

}  // namespace
