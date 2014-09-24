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

#include "vpx_config.h"
extern "C" {
#include "vp9/encoder/vp9_variance.h"
#include "vpx/vpx_integer.h"
#include "vp9_rtcd.h"
}

namespace {

using ::std::tr1::get;
using ::std::tr1::make_tuple;
using ::std::tr1::tuple;

class VP9VarianceTest :
    public ::testing::TestWithParam<tuple<int, int, vp9_variance_fn_t> > {
 public:
  virtual void SetUp() {
    const tuple<int, int, vp9_variance_fn_t>& params = GetParam();
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
  uint8_t* src_;
  uint8_t* ref_;
  int width_;
  int height_;
  int block_size_;
  vp9_variance_fn_t variance_;
};

TEST_P(VP9VarianceTest, Zero) {
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

TEST_P(VP9VarianceTest, OneQuarter) {
  memset(src_, 255, block_size_);
  const int half = block_size_ / 2;
  memset(ref_, 255, half);
  memset(ref_ + half, 0, half);
  unsigned int sse;
  const unsigned int var = variance_(src_, width_, ref_, width_, &sse);
  const unsigned int expected = block_size_ * 255 * 255 / 4;
  EXPECT_EQ(expected, var);
}

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
}  // namespace
