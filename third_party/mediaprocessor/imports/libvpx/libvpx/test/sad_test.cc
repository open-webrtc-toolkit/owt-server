/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <string.h>
#include <limits.h>
#include <stdio.h>

extern "C" {
#include "./vpx_config.h"
#if CONFIG_VP8_ENCODER
#include "./vp8_rtcd.h"
//#include "vp8/common/blockd.h"
#endif
#if CONFIG_VP9_ENCODER
#include "./vp9_rtcd.h"
#endif
#include "vpx_mem/vpx_mem.h"
}

#include "test/acm_random.h"
#include "test/register_state_check.h"
#include "test/util.h"
#include "third_party/googletest/src/include/gtest/gtest.h"


typedef unsigned int (*sad_m_by_n_fn_t)(const unsigned char *source_ptr,
                                        int source_stride,
                                        const unsigned char *reference_ptr,
                                        int reference_stride,
                                        unsigned int max_sad);

typedef void (*sad_n_by_n_by_4_fn_t)(const uint8_t *src_ptr,
                                     int src_stride,
                                     const unsigned char * const ref_ptr[],
                                     int ref_stride,
                                     unsigned int *sad_array);

using libvpx_test::ACMRandom;

namespace {
class SADTestBase : public ::testing::Test {
 public:
  SADTestBase(int width, int height) : width_(width), height_(height) {}

  static void SetUpTestCase() {
    source_data_ = reinterpret_cast<uint8_t*>(
        vpx_memalign(kDataAlignment, kDataBlockSize));
    reference_data_ = reinterpret_cast<uint8_t*>(
        vpx_memalign(kDataAlignment, kDataBufferSize));
  }

  static void TearDownTestCase() {
    vpx_free(source_data_);
    source_data_ = NULL;
    vpx_free(reference_data_);
    reference_data_ = NULL;
  }

 protected:
  // Handle blocks up to 4 blocks 64x64 with stride up to 128
  static const int kDataAlignment = 16;
  static const int kDataBlockSize = 64 * 128;
  static const int kDataBufferSize = 4 * kDataBlockSize;

  virtual void SetUp() {
    source_stride_ = (width_ + 31) & ~31;
    reference_stride_ = width_ * 2;
    rnd_.Reset(ACMRandom::DeterministicSeed());
  }

  virtual uint8_t* GetReference(int block_idx) {
    return reference_data_ + block_idx * kDataBlockSize;
  }

  // Sum of Absolute Differences. Given two blocks, calculate the absolute
  // difference between two pixels in the same relative location; accumulate.
  unsigned int ReferenceSAD(unsigned int max_sad, int block_idx = 0) {
    unsigned int sad = 0;
    const uint8_t* const reference = GetReference(block_idx);

    for (int h = 0; h < height_; ++h) {
      for (int w = 0; w < width_; ++w) {
        sad += abs(source_data_[h * source_stride_ + w]
               - reference[h * reference_stride_ + w]);
      }
      if (sad > max_sad) {
        break;
      }
    }
    return sad;
  }

  void FillConstant(uint8_t *data, int stride, uint8_t fill_constant) {
    for (int h = 0; h < height_; ++h) {
      for (int w = 0; w < width_; ++w) {
        data[h * stride + w] = fill_constant;
      }
    }
  }

  void FillRandom(uint8_t *data, int stride) {
    for (int h = 0; h < height_; ++h) {
      for (int w = 0; w < width_; ++w) {
        data[h * stride + w] = rnd_.Rand8();
      }
    }
  }

  int width_, height_;
  static uint8_t* source_data_;
  int source_stride_;
  static uint8_t* reference_data_;
  int reference_stride_;

  ACMRandom rnd_;
};

class SADTest : public SADTestBase,
    public ::testing::WithParamInterface<
        std::tr1::tuple<int, int, sad_m_by_n_fn_t> > {
 public:
  SADTest() : SADTestBase(GET_PARAM(0), GET_PARAM(1)) {}

 protected:
  unsigned int SAD(unsigned int max_sad, int block_idx = 0) {
    unsigned int ret;
    const uint8_t* const reference = GetReference(block_idx);

    REGISTER_STATE_CHECK(ret = GET_PARAM(2)(source_data_, source_stride_,
                                            reference, reference_stride_,
                                            max_sad));
    return ret;
  }

  void CheckSad(unsigned int max_sad) {
    unsigned int reference_sad, exp_sad;

    reference_sad = ReferenceSAD(max_sad);
    exp_sad = SAD(max_sad);

    if (reference_sad <= max_sad) {
      ASSERT_EQ(exp_sad, reference_sad);
    } else {
      // Alternative implementations are not required to check max_sad
      ASSERT_GE(exp_sad, reference_sad);
    }
  }
};

class SADx4Test : public SADTestBase,
    public ::testing::WithParamInterface<
        std::tr1::tuple<int, int, sad_n_by_n_by_4_fn_t> > {
 public:
  SADx4Test() : SADTestBase(GET_PARAM(0), GET_PARAM(1)) {}

 protected:
  void SADs(unsigned int *results) {
    const uint8_t* refs[] = {GetReference(0), GetReference(1),
                             GetReference(2), GetReference(3)};

    REGISTER_STATE_CHECK(GET_PARAM(2)(source_data_, source_stride_,
                                      refs, reference_stride_,
                                      results));
  }

  void CheckSADs() {
    unsigned int reference_sad, exp_sad[4];

    SADs(exp_sad);
    for (int block = 0; block < 4; block++) {
      reference_sad = ReferenceSAD(UINT_MAX, block);

      EXPECT_EQ(exp_sad[block], reference_sad) << "block " << block;
    }
  }
};

uint8_t* SADTestBase::source_data_ = NULL;
uint8_t* SADTestBase::reference_data_ = NULL;

TEST_P(SADTest, MaxRef) {
  FillConstant(source_data_, source_stride_, 0);
  FillConstant(reference_data_, reference_stride_, 255);
  CheckSad(UINT_MAX);
}

TEST_P(SADx4Test, MaxRef) {
  FillConstant(source_data_, source_stride_, 0);
  FillConstant(GetReference(0), reference_stride_, 255);
  FillConstant(GetReference(1), reference_stride_, 255);
  FillConstant(GetReference(2), reference_stride_, 255);
  FillConstant(GetReference(3), reference_stride_, 255);
  CheckSADs();
}

TEST_P(SADTest, MaxSrc) {
  FillConstant(source_data_, source_stride_, 255);
  FillConstant(reference_data_, reference_stride_, 0);
  CheckSad(UINT_MAX);
}

TEST_P(SADx4Test, MaxSrc) {
  FillConstant(source_data_, source_stride_, 255);
  FillConstant(GetReference(0), reference_stride_, 0);
  FillConstant(GetReference(1), reference_stride_, 0);
  FillConstant(GetReference(2), reference_stride_, 0);
  FillConstant(GetReference(3), reference_stride_, 0);
  CheckSADs();
}

TEST_P(SADTest, ShortRef) {
  int tmp_stride = reference_stride_;
  reference_stride_ >>= 1;
  FillRandom(source_data_, source_stride_);
  FillRandom(reference_data_, reference_stride_);
  CheckSad(UINT_MAX);
  reference_stride_ = tmp_stride;
}

TEST_P(SADx4Test, ShortRef) {
  int tmp_stride = reference_stride_;
  reference_stride_ >>= 1;
  FillRandom(source_data_, source_stride_);
  FillRandom(GetReference(0), reference_stride_);
  FillRandom(GetReference(1), reference_stride_);
  FillRandom(GetReference(2), reference_stride_);
  FillRandom(GetReference(3), reference_stride_);
  CheckSADs();
  reference_stride_ = tmp_stride;
}

TEST_P(SADTest, UnalignedRef) {
  // The reference frame, but not the source frame, may be unaligned for
  // certain types of searches.
  int tmp_stride = reference_stride_;
  reference_stride_ -= 1;
  FillRandom(source_data_, source_stride_);
  FillRandom(reference_data_, reference_stride_);
  CheckSad(UINT_MAX);
  reference_stride_ = tmp_stride;
}

TEST_P(SADx4Test, UnalignedRef) {
  // The reference frame, but not the source frame, may be unaligned for
  // certain types of searches.
  int tmp_stride = reference_stride_;
  reference_stride_ -= 1;
  FillRandom(source_data_, source_stride_);
  FillRandom(GetReference(0), reference_stride_);
  FillRandom(GetReference(1), reference_stride_);
  FillRandom(GetReference(2), reference_stride_);
  FillRandom(GetReference(3), reference_stride_);
  CheckSADs();
  reference_stride_ = tmp_stride;
}

TEST_P(SADTest, ShortSrc) {
  int tmp_stride = source_stride_;
  source_stride_ >>= 1;
  FillRandom(source_data_, source_stride_);
  FillRandom(reference_data_, reference_stride_);
  CheckSad(UINT_MAX);
  source_stride_ = tmp_stride;
}

TEST_P(SADx4Test, ShortSrc) {
  int tmp_stride = source_stride_;
  source_stride_ >>= 1;
  FillRandom(source_data_, source_stride_);
  FillRandom(GetReference(0), reference_stride_);
  FillRandom(GetReference(1), reference_stride_);
  FillRandom(GetReference(2), reference_stride_);
  FillRandom(GetReference(3), reference_stride_);
  CheckSADs();
  source_stride_ = tmp_stride;
}

TEST_P(SADTest, MaxSAD) {
  // Verify that, when max_sad is set, the implementation does not return a
  // value lower than the reference.
  FillConstant(source_data_, source_stride_, 255);
  FillConstant(reference_data_, reference_stride_, 0);
  CheckSad(128);
}

using std::tr1::make_tuple;

#if CONFIG_VP8_ENCODER && CONFIG_VP9_ENCODER
#define VP8_VP9_SEPARATOR ,
#else
#define VP8_VP9_SEPARATOR
#endif

#if CONFIG_VP8_ENCODER
const sad_m_by_n_fn_t sad_16x16_c = vp8_sad16x16_c;
const sad_m_by_n_fn_t sad_8x16_c = vp8_sad8x16_c;
const sad_m_by_n_fn_t sad_16x8_c = vp8_sad16x8_c;
const sad_m_by_n_fn_t sad_8x8_c = vp8_sad8x8_c;
const sad_m_by_n_fn_t sad_4x4_c = vp8_sad4x4_c;
#endif
#if CONFIG_VP9_ENCODER
const sad_m_by_n_fn_t sad_64x64_c_vp9 = vp9_sad64x64_c;
const sad_m_by_n_fn_t sad_32x32_c_vp9 = vp9_sad32x32_c;
const sad_m_by_n_fn_t sad_16x16_c_vp9 = vp9_sad16x16_c;
const sad_m_by_n_fn_t sad_8x16_c_vp9 = vp9_sad8x16_c;
const sad_m_by_n_fn_t sad_16x8_c_vp9 = vp9_sad16x8_c;
const sad_m_by_n_fn_t sad_8x8_c_vp9 = vp9_sad8x8_c;
const sad_m_by_n_fn_t sad_4x4_c_vp9 = vp9_sad4x4_c;
#endif
INSTANTIATE_TEST_CASE_P(C, SADTest, ::testing::Values(
#if CONFIG_VP8_ENCODER
                        make_tuple(16, 16, sad_16x16_c),
                        make_tuple(8, 16, sad_8x16_c),
                        make_tuple(16, 8, sad_16x8_c),
                        make_tuple(8, 8, sad_8x8_c),
                        make_tuple(4, 4, sad_4x4_c)
#endif
                        VP8_VP9_SEPARATOR
#if CONFIG_VP9_ENCODER
                        make_tuple(64, 64, sad_64x64_c_vp9),
                        make_tuple(32, 32, sad_32x32_c_vp9),
                        make_tuple(16, 16, sad_16x16_c_vp9),
                        make_tuple(8, 16, sad_8x16_c_vp9),
                        make_tuple(16, 8, sad_16x8_c_vp9),
                        make_tuple(8, 8, sad_8x8_c_vp9),
                        make_tuple(4, 4, sad_4x4_c_vp9)
#endif
                        ));

#if CONFIG_VP9_ENCODER
const sad_n_by_n_by_4_fn_t sad_64x64x4d_c = vp9_sad64x64x4d_c;
const sad_n_by_n_by_4_fn_t sad_32x32x4d_c = vp9_sad32x32x4d_c;
const sad_n_by_n_by_4_fn_t sad_16x16x4d_c = vp9_sad16x16x4d_c;
const sad_n_by_n_by_4_fn_t sad_8x8x4d_c = vp9_sad8x8x4d_c;
const sad_n_by_n_by_4_fn_t sad_4x4x4d_c = vp9_sad4x4x4d_c;
INSTANTIATE_TEST_CASE_P(C, SADx4Test, ::testing::Values(
                        make_tuple(64, 64, sad_64x64x4d_c),
                        make_tuple(32, 32, sad_32x32x4d_c),
                        make_tuple(16, 16, sad_16x16x4d_c),
                        make_tuple(8, 8, sad_8x8x4d_c),
                        make_tuple(4, 4, sad_4x4x4d_c)));
#endif

// ARM tests
#if HAVE_MEDIA
const sad_m_by_n_fn_t sad_16x16_armv6 = vp8_sad16x16_armv6;
INSTANTIATE_TEST_CASE_P(MEDIA, SADTest, ::testing::Values(
                        make_tuple(16, 16, sad_16x16_armv6)));

#endif
#if HAVE_NEON
const sad_m_by_n_fn_t sad_16x16_neon = vp8_sad16x16_neon;
const sad_m_by_n_fn_t sad_8x16_neon = vp8_sad8x16_neon;
const sad_m_by_n_fn_t sad_16x8_neon = vp8_sad16x8_neon;
const sad_m_by_n_fn_t sad_8x8_neon = vp8_sad8x8_neon;
const sad_m_by_n_fn_t sad_4x4_neon = vp8_sad4x4_neon;
INSTANTIATE_TEST_CASE_P(NEON, SADTest, ::testing::Values(
                        make_tuple(16, 16, sad_16x16_neon),
                        make_tuple(8, 16, sad_8x16_neon),
                        make_tuple(16, 8, sad_16x8_neon),
                        make_tuple(8, 8, sad_8x8_neon),
                        make_tuple(4, 4, sad_4x4_neon)));
#endif

// X86 tests
#if HAVE_MMX
#if CONFIG_VP8_ENCODER
const sad_m_by_n_fn_t sad_16x16_mmx = vp8_sad16x16_mmx;
const sad_m_by_n_fn_t sad_8x16_mmx = vp8_sad8x16_mmx;
const sad_m_by_n_fn_t sad_16x8_mmx = vp8_sad16x8_mmx;
const sad_m_by_n_fn_t sad_8x8_mmx = vp8_sad8x8_mmx;
const sad_m_by_n_fn_t sad_4x4_mmx = vp8_sad4x4_mmx;
#endif
#if CONFIG_VP9_ENCODER
const sad_m_by_n_fn_t sad_16x16_mmx_vp9 = vp9_sad16x16_mmx;
const sad_m_by_n_fn_t sad_8x16_mmx_vp9 = vp9_sad8x16_mmx;
const sad_m_by_n_fn_t sad_16x8_mmx_vp9 = vp9_sad16x8_mmx;
const sad_m_by_n_fn_t sad_8x8_mmx_vp9 = vp9_sad8x8_mmx;
const sad_m_by_n_fn_t sad_4x4_mmx_vp9 = vp9_sad4x4_mmx;
#endif

INSTANTIATE_TEST_CASE_P(MMX, SADTest, ::testing::Values(
#if CONFIG_VP8_ENCODER
                        make_tuple(16, 16, sad_16x16_mmx),
                        make_tuple(8, 16, sad_8x16_mmx),
                        make_tuple(16, 8, sad_16x8_mmx),
                        make_tuple(8, 8, sad_8x8_mmx),
                        make_tuple(4, 4, sad_4x4_mmx)
#endif
                        VP8_VP9_SEPARATOR
#if CONFIG_VP9_ENCODER
                        make_tuple(16, 16, sad_16x16_mmx_vp9),
                        make_tuple(8, 16, sad_8x16_mmx_vp9),
                        make_tuple(16, 8, sad_16x8_mmx_vp9),
                        make_tuple(8, 8, sad_8x8_mmx_vp9),
                        make_tuple(4, 4, sad_4x4_mmx_vp9)
#endif
                        ));
#endif

#if HAVE_SSE
#if CONFIG_VP9_ENCODER
const sad_m_by_n_fn_t sad_4x4_sse_vp9 = vp9_sad4x4_sse;
INSTANTIATE_TEST_CASE_P(SSE, SADTest, ::testing::Values(
                        make_tuple(4, 4, sad_4x4_sse_vp9)));

const sad_n_by_n_by_4_fn_t sad_4x4x4d_sse = vp9_sad4x4x4d_sse;
INSTANTIATE_TEST_CASE_P(SSE, SADx4Test, ::testing::Values(
                        make_tuple(4, 4, sad_4x4x4d_sse)));
#endif
#endif

#if HAVE_SSE2
#if CONFIG_VP8_ENCODER
const sad_m_by_n_fn_t sad_16x16_wmt = vp8_sad16x16_wmt;
const sad_m_by_n_fn_t sad_8x16_wmt = vp8_sad8x16_wmt;
const sad_m_by_n_fn_t sad_16x8_wmt = vp8_sad16x8_wmt;
const sad_m_by_n_fn_t sad_8x8_wmt = vp8_sad8x8_wmt;
const sad_m_by_n_fn_t sad_4x4_wmt = vp8_sad4x4_wmt;
#endif
#if CONFIG_VP9_ENCODER
const sad_m_by_n_fn_t sad_64x64_sse2_vp9 = vp9_sad64x64_sse2;
const sad_m_by_n_fn_t sad_32x32_sse2_vp9 = vp9_sad32x32_sse2;
const sad_m_by_n_fn_t sad_16x16_sse2_vp9 = vp9_sad16x16_sse2;
const sad_m_by_n_fn_t sad_8x16_sse2_vp9 = vp9_sad8x16_sse2;
const sad_m_by_n_fn_t sad_16x8_sse2_vp9 = vp9_sad16x8_sse2;
const sad_m_by_n_fn_t sad_8x8_sse2_vp9 = vp9_sad8x8_sse2;
#endif
INSTANTIATE_TEST_CASE_P(SSE2, SADTest, ::testing::Values(
#if CONFIG_VP8_ENCODER
                        make_tuple(16, 16, sad_16x16_wmt),
                        make_tuple(8, 16, sad_8x16_wmt),
                        make_tuple(16, 8, sad_16x8_wmt),
                        make_tuple(8, 8, sad_8x8_wmt),
                        make_tuple(4, 4, sad_4x4_wmt)
#endif
                        VP8_VP9_SEPARATOR
#if CONFIG_VP9_ENCODER
                        make_tuple(64, 64, sad_64x64_sse2_vp9),
                        make_tuple(32, 32, sad_32x32_sse2_vp9),
                        make_tuple(16, 16, sad_16x16_sse2_vp9),
                        make_tuple(8, 16, sad_8x16_sse2_vp9),
                        make_tuple(16, 8, sad_16x8_sse2_vp9),
                        make_tuple(8, 8, sad_8x8_sse2_vp9)
#endif
                        ));

#if CONFIG_VP9_ENCODER
const sad_n_by_n_by_4_fn_t sad_64x64x4d_sse2 = vp9_sad64x64x4d_sse2;
const sad_n_by_n_by_4_fn_t sad_32x32x4d_sse2 = vp9_sad32x32x4d_sse2;
const sad_n_by_n_by_4_fn_t sad_16x16x4d_sse2 = vp9_sad16x16x4d_sse2;
const sad_n_by_n_by_4_fn_t sad_16x8x4d_sse2 = vp9_sad16x8x4d_sse2;
const sad_n_by_n_by_4_fn_t sad_8x16x4d_sse2 = vp9_sad8x16x4d_sse2;
const sad_n_by_n_by_4_fn_t sad_8x8x4d_sse2 = vp9_sad8x8x4d_sse2;
INSTANTIATE_TEST_CASE_P(SSE2, SADx4Test, ::testing::Values(
                        make_tuple(64, 64, sad_64x64x4d_sse2),
                        make_tuple(32, 32, sad_32x32x4d_sse2),
                        make_tuple(16, 16, sad_16x16x4d_sse2),
                        make_tuple(16, 8, sad_16x8x4d_sse2),
                        make_tuple(8, 16, sad_8x16x4d_sse2),
                        make_tuple(8, 8, sad_8x8x4d_sse2)));
#endif
#endif

#if HAVE_SSE3
#if CONFIG_VP8_ENCODER
const sad_n_by_n_by_4_fn_t sad_16x16x4d_sse3 = vp8_sad16x16x4d_sse3;
const sad_n_by_n_by_4_fn_t sad_16x8x4d_sse3 = vp8_sad16x8x4d_sse3;
const sad_n_by_n_by_4_fn_t sad_8x16x4d_sse3 = vp8_sad8x16x4d_sse3;
const sad_n_by_n_by_4_fn_t sad_8x8x4d_sse3 = vp8_sad8x8x4d_sse3;
const sad_n_by_n_by_4_fn_t sad_4x4x4d_sse3 = vp8_sad4x4x4d_sse3;
INSTANTIATE_TEST_CASE_P(SSE3, SADx4Test, ::testing::Values(
                        make_tuple(16, 16, sad_16x16x4d_sse3),
                        make_tuple(16, 8, sad_16x8x4d_sse3),
                        make_tuple(8, 16, sad_8x16x4d_sse3),
                        make_tuple(8, 8, sad_8x8x4d_sse3),
                        make_tuple(4, 4, sad_4x4x4d_sse3)));
#endif
#endif

#if HAVE_SSSE3
const sad_m_by_n_fn_t sad_16x16_sse3 = vp8_sad16x16_sse3;
INSTANTIATE_TEST_CASE_P(SSE3, SADTest, ::testing::Values(
                        make_tuple(16, 16, sad_16x16_sse3)));
#endif

}  // namespace
