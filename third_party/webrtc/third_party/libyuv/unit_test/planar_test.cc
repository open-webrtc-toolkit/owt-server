/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <time.h>

#include "libyuv/compare.h"
#include "libyuv/convert.h"
#include "libyuv/convert_argb.h"
#include "libyuv/convert_from.h"
#include "libyuv/convert_from_argb.h"
#include "libyuv/cpu_id.h"
#include "libyuv/format_conversion.h"
#include "libyuv/planar_functions.h"
#include "libyuv/rotate.h"
#include "../unit_test/unit_test.h"

#if defined(_MSC_VER)
#define SIMD_ALIGNED(var) __declspec(align(16)) var
#else  // __GNUC__
#define SIMD_ALIGNED(var) var __attribute__((aligned(16)))
#endif

namespace libyuv {

TEST_F(libyuvTest, TestAttenuate) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);
  SIMD_ALIGNED(uint8 atten_pixels[256][4]);
  SIMD_ALIGNED(uint8 unatten_pixels[256][4]);
  SIMD_ALIGNED(uint8 atten2_pixels[256][4]);

  // Test unattenuation clamps
  orig_pixels[0][0] = 200u;
  orig_pixels[0][1] = 129u;
  orig_pixels[0][2] = 127u;
  orig_pixels[0][3] = 128u;
  // Test unattenuation transparent and opaque are unaffected
  orig_pixels[1][0] = 16u;
  orig_pixels[1][1] = 64u;
  orig_pixels[1][2] = 192u;
  orig_pixels[1][3] = 0u;
  orig_pixels[2][0] = 16u;
  orig_pixels[2][1] = 64u;
  orig_pixels[2][2] = 192u;
  orig_pixels[2][3] = 255u;
  orig_pixels[3][0] = 16u;
  orig_pixels[3][1] = 64u;
  orig_pixels[3][2] = 192u;
  orig_pixels[3][3] = 128u;
  ARGBUnattenuate(&orig_pixels[0][0], 0, &unatten_pixels[0][0], 0, 4, 1);
  EXPECT_EQ(255u, unatten_pixels[0][0]);
  EXPECT_EQ(255u, unatten_pixels[0][1]);
  EXPECT_EQ(254u, unatten_pixels[0][2]);
  EXPECT_EQ(128u, unatten_pixels[0][3]);
  EXPECT_EQ(0u, unatten_pixels[1][0]);
  EXPECT_EQ(0u, unatten_pixels[1][1]);
  EXPECT_EQ(0u, unatten_pixels[1][2]);
  EXPECT_EQ(0u, unatten_pixels[1][3]);
  EXPECT_EQ(16u, unatten_pixels[2][0]);
  EXPECT_EQ(64u, unatten_pixels[2][1]);
  EXPECT_EQ(192u, unatten_pixels[2][2]);
  EXPECT_EQ(255u, unatten_pixels[2][3]);
  EXPECT_EQ(32u, unatten_pixels[3][0]);
  EXPECT_EQ(128u, unatten_pixels[3][1]);
  EXPECT_EQ(255u, unatten_pixels[3][2]);
  EXPECT_EQ(128u, unatten_pixels[3][3]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i;
  }
  ARGBAttenuate(&orig_pixels[0][0], 0, &atten_pixels[0][0], 0, 256, 1);
  ARGBUnattenuate(&atten_pixels[0][0], 0, &unatten_pixels[0][0], 0, 256, 1);
  for (int i = 0; i < benchmark_pixels_div256_; ++i) {
    ARGBAttenuate(&unatten_pixels[0][0], 0, &atten2_pixels[0][0], 0, 256, 1);
  }
  for (int i = 0; i < 256; ++i) {
    EXPECT_NEAR(atten_pixels[i][0], atten2_pixels[i][0], 2);
    EXPECT_NEAR(atten_pixels[i][1], atten2_pixels[i][1], 2);
    EXPECT_NEAR(atten_pixels[i][2], atten2_pixels[i][2], 2);
    EXPECT_NEAR(atten_pixels[i][3], atten2_pixels[i][3], 2);
  }
  // Make sure transparent, 50% and opaque are fully accurate.
  EXPECT_EQ(0, atten_pixels[0][0]);
  EXPECT_EQ(0, atten_pixels[0][1]);
  EXPECT_EQ(0, atten_pixels[0][2]);
  EXPECT_EQ(0, atten_pixels[0][3]);
  EXPECT_EQ(64, atten_pixels[128][0]);
  EXPECT_EQ(32, atten_pixels[128][1]);
  EXPECT_EQ(21,  atten_pixels[128][2]);
  EXPECT_EQ(128, atten_pixels[128][3]);
  EXPECT_NEAR(255, atten_pixels[255][0], 1);
  EXPECT_NEAR(127, atten_pixels[255][1], 1);
  EXPECT_NEAR(85,  atten_pixels[255][2], 1);
  EXPECT_EQ(255, atten_pixels[255][3]);
}

static int TestAttenuateI(int width, int height, int benchmark_iterations,
                          int invert, int off) {
  const int kBpp = 4;
  const int kStride = (width * kBpp + 15) & ~15;
  align_buffer_64(src_argb, kStride * height + off);
  align_buffer_64(dst_argb_c, kStride * height);
  align_buffer_64(dst_argb_opt, kStride * height);
  srandom(time(NULL));
  for (int i = 0; i < kStride * height; ++i) {
    src_argb[i + off] = (random() & 0xff);
  }
  memset(dst_argb_c, 0, kStride * height);
  memset(dst_argb_opt, 0, kStride * height);

  MaskCpuFlags(0);
  ARGBAttenuate(src_argb + off, kStride,
                dst_argb_c, kStride,
                width, invert * height);
  MaskCpuFlags(-1);
  for (int i = 0; i < benchmark_iterations; ++i) {
    ARGBAttenuate(src_argb + off, kStride,
                  dst_argb_opt, kStride,
                  width, invert * height);
  }
  int max_diff = 0;
  for (int i = 0; i < kStride * height; ++i) {
    int abs_diff =
        abs(static_cast<int>(dst_argb_c[i]) -
            static_cast<int>(dst_argb_opt[i]));
    if (abs_diff > max_diff) {
      max_diff = abs_diff;
    }
  }
  free_aligned_buffer_64(src_argb)
  free_aligned_buffer_64(dst_argb_c)
  free_aligned_buffer_64(dst_argb_opt)
  return max_diff;
}

TEST_F(libyuvTest, ARGBAttenuate_Any) {
  int max_diff = TestAttenuateI(benchmark_width_ - 1, benchmark_height_,
                                benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 2);
}

TEST_F(libyuvTest, ARGBAttenuate_Unaligned) {
  int max_diff = TestAttenuateI(benchmark_width_, benchmark_height_,
                                benchmark_iterations_, +1, 1);
  EXPECT_LE(max_diff, 2);
}

TEST_F(libyuvTest, ARGBAttenuate_Invert) {
  int max_diff = TestAttenuateI(benchmark_width_, benchmark_height_,
                                benchmark_iterations_, -1, 0);
  EXPECT_LE(max_diff, 2);
}

TEST_F(libyuvTest, ARGBAttenuate_Opt) {
  int max_diff = TestAttenuateI(benchmark_width_, benchmark_height_,
                                benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 2);
}

static int TestUnattenuateI(int width, int height, int benchmark_iterations,
                            int invert, int off) {
  const int kBpp = 4;
  const int kStride = (width * kBpp + 15) & ~15;
  align_buffer_64(src_argb, kStride * height + off);
  align_buffer_64(dst_argb_c, kStride * height);
  align_buffer_64(dst_argb_opt, kStride * height);
  srandom(time(NULL));
  for (int i = 0; i < kStride * height; ++i) {
    src_argb[i + off] = (random() & 0xff);
  }
  ARGBAttenuate(src_argb + off, kStride,
                src_argb + off, kStride,
                width, height);
  memset(dst_argb_c, 0, kStride * height);
  memset(dst_argb_opt, 0, kStride * height);

  MaskCpuFlags(0);
  ARGBUnattenuate(src_argb + off, kStride,
                  dst_argb_c, kStride,
                  width, invert * height);
  MaskCpuFlags(-1);
  for (int i = 0; i < benchmark_iterations; ++i) {
    ARGBUnattenuate(src_argb + off, kStride,
                    dst_argb_opt, kStride,
                    width, invert * height);
  }
  int max_diff = 0;
  for (int i = 0; i < kStride * height; ++i) {
    int abs_diff =
        abs(static_cast<int>(dst_argb_c[i]) -
            static_cast<int>(dst_argb_opt[i]));
    if (abs_diff > max_diff) {
      max_diff = abs_diff;
    }
  }
  free_aligned_buffer_64(src_argb)
  free_aligned_buffer_64(dst_argb_c)
  free_aligned_buffer_64(dst_argb_opt)
  return max_diff;
}

TEST_F(libyuvTest, ARGBUnattenuate_Any) {
  int max_diff = TestUnattenuateI(benchmark_width_ - 1, benchmark_height_,
                                  benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 2);
}

TEST_F(libyuvTest, ARGBUnattenuate_Unaligned) {
  int max_diff = TestUnattenuateI(benchmark_width_, benchmark_height_,
                                  benchmark_iterations_, +1, 1);
  EXPECT_LE(max_diff, 2);
}

TEST_F(libyuvTest, ARGBUnattenuate_Invert) {
  int max_diff = TestUnattenuateI(benchmark_width_, benchmark_height_,
                                  benchmark_iterations_, -1, 0);
  EXPECT_LE(max_diff, 2);
}

TEST_F(libyuvTest, ARGBUnattenuate_Opt) {
  int max_diff = TestUnattenuateI(benchmark_width_, benchmark_height_,
                                  benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 2);
}

TEST_F(libyuvTest, TestARGBComputeCumulativeSum) {
  SIMD_ALIGNED(uint8 orig_pixels[16][16][4]);
  SIMD_ALIGNED(int32 added_pixels[16][16][4]);

  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      orig_pixels[y][x][0] = 1u;
      orig_pixels[y][x][1] = 2u;
      orig_pixels[y][x][2] = 3u;
      orig_pixels[y][x][3] = 255u;
    }
  }

  ARGBComputeCumulativeSum(&orig_pixels[0][0][0], 16 * 4,
                           &added_pixels[0][0][0], 16 * 4,
                           16, 16);

  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      EXPECT_EQ((x + 1) * (y + 1), added_pixels[y][x][0]);
      EXPECT_EQ((x + 1) * (y + 1) * 2, added_pixels[y][x][1]);
      EXPECT_EQ((x + 1) * (y + 1) * 3, added_pixels[y][x][2]);
      EXPECT_EQ((x + 1) * (y + 1) * 255, added_pixels[y][x][3]);
    }
  }
}

TEST_F(libyuvTest, TestARGBGray) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);

  // Test blue
  orig_pixels[0][0] = 255u;
  orig_pixels[0][1] = 0u;
  orig_pixels[0][2] = 0u;
  orig_pixels[0][3] = 128u;
  // Test green
  orig_pixels[1][0] = 0u;
  orig_pixels[1][1] = 255u;
  orig_pixels[1][2] = 0u;
  orig_pixels[1][3] = 0u;
  // Test red
  orig_pixels[2][0] = 0u;
  orig_pixels[2][1] = 0u;
  orig_pixels[2][2] = 255u;
  orig_pixels[2][3] = 255u;
  // Test color
  orig_pixels[3][0] = 16u;
  orig_pixels[3][1] = 64u;
  orig_pixels[3][2] = 192u;
  orig_pixels[3][3] = 224u;
  // Do 16 to test asm version.
  ARGBGray(&orig_pixels[0][0], 0, 0, 0, 16, 1);
  EXPECT_EQ(27u, orig_pixels[0][0]);
  EXPECT_EQ(27u, orig_pixels[0][1]);
  EXPECT_EQ(27u, orig_pixels[0][2]);
  EXPECT_EQ(128u, orig_pixels[0][3]);
  EXPECT_EQ(151u, orig_pixels[1][0]);
  EXPECT_EQ(151u, orig_pixels[1][1]);
  EXPECT_EQ(151u, orig_pixels[1][2]);
  EXPECT_EQ(0u, orig_pixels[1][3]);
  EXPECT_EQ(75u, orig_pixels[2][0]);
  EXPECT_EQ(75u, orig_pixels[2][1]);
  EXPECT_EQ(75u, orig_pixels[2][2]);
  EXPECT_EQ(255u, orig_pixels[2][3]);
  EXPECT_EQ(96u, orig_pixels[3][0]);
  EXPECT_EQ(96u, orig_pixels[3][1]);
  EXPECT_EQ(96u, orig_pixels[3][2]);
  EXPECT_EQ(224u, orig_pixels[3][3]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i;
  }
  for (int i = 0; i < benchmark_pixels_div256_; ++i) {
    ARGBGray(&orig_pixels[0][0], 0, 0, 0, 256, 1);
  }
}

TEST_F(libyuvTest, TestARGBGrayTo) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);
  SIMD_ALIGNED(uint8 gray_pixels[256][4]);

  // Test blue
  orig_pixels[0][0] = 255u;
  orig_pixels[0][1] = 0u;
  orig_pixels[0][2] = 0u;
  orig_pixels[0][3] = 128u;
  // Test green
  orig_pixels[1][0] = 0u;
  orig_pixels[1][1] = 255u;
  orig_pixels[1][2] = 0u;
  orig_pixels[1][3] = 0u;
  // Test red
  orig_pixels[2][0] = 0u;
  orig_pixels[2][1] = 0u;
  orig_pixels[2][2] = 255u;
  orig_pixels[2][3] = 255u;
  // Test color
  orig_pixels[3][0] = 16u;
  orig_pixels[3][1] = 64u;
  orig_pixels[3][2] = 192u;
  orig_pixels[3][3] = 224u;
  // Do 16 to test asm version.
  ARGBGrayTo(&orig_pixels[0][0], 0, &gray_pixels[0][0], 0, 16, 1);
  EXPECT_EQ(27u, gray_pixels[0][0]);
  EXPECT_EQ(27u, gray_pixels[0][1]);
  EXPECT_EQ(27u, gray_pixels[0][2]);
  EXPECT_EQ(128u, gray_pixels[0][3]);
  EXPECT_EQ(151u, gray_pixels[1][0]);
  EXPECT_EQ(151u, gray_pixels[1][1]);
  EXPECT_EQ(151u, gray_pixels[1][2]);
  EXPECT_EQ(0u, gray_pixels[1][3]);
  EXPECT_EQ(75u, gray_pixels[2][0]);
  EXPECT_EQ(75u, gray_pixels[2][1]);
  EXPECT_EQ(75u, gray_pixels[2][2]);
  EXPECT_EQ(255u, gray_pixels[2][3]);
  EXPECT_EQ(96u, gray_pixels[3][0]);
  EXPECT_EQ(96u, gray_pixels[3][1]);
  EXPECT_EQ(96u, gray_pixels[3][2]);
  EXPECT_EQ(224u, gray_pixels[3][3]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i;
  }
  for (int i = 0; i < benchmark_pixels_div256_; ++i) {
    ARGBGrayTo(&orig_pixels[0][0], 0, &gray_pixels[0][0], 0, 256, 1);
  }
}

TEST_F(libyuvTest, TestARGBSepia) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);

  // Test blue
  orig_pixels[0][0] = 255u;
  orig_pixels[0][1] = 0u;
  orig_pixels[0][2] = 0u;
  orig_pixels[0][3] = 128u;
  // Test green
  orig_pixels[1][0] = 0u;
  orig_pixels[1][1] = 255u;
  orig_pixels[1][2] = 0u;
  orig_pixels[1][3] = 0u;
  // Test red
  orig_pixels[2][0] = 0u;
  orig_pixels[2][1] = 0u;
  orig_pixels[2][2] = 255u;
  orig_pixels[2][3] = 255u;
  // Test color
  orig_pixels[3][0] = 16u;
  orig_pixels[3][1] = 64u;
  orig_pixels[3][2] = 192u;
  orig_pixels[3][3] = 224u;
  // Do 16 to test asm version.
  ARGBSepia(&orig_pixels[0][0], 0, 0, 0, 16, 1);
  EXPECT_EQ(33u, orig_pixels[0][0]);
  EXPECT_EQ(43u, orig_pixels[0][1]);
  EXPECT_EQ(47u, orig_pixels[0][2]);
  EXPECT_EQ(128u, orig_pixels[0][3]);
  EXPECT_EQ(135u, orig_pixels[1][0]);
  EXPECT_EQ(175u, orig_pixels[1][1]);
  EXPECT_EQ(195u, orig_pixels[1][2]);
  EXPECT_EQ(0u, orig_pixels[1][3]);
  EXPECT_EQ(69u, orig_pixels[2][0]);
  EXPECT_EQ(89u, orig_pixels[2][1]);
  EXPECT_EQ(99u, orig_pixels[2][2]);
  EXPECT_EQ(255u, orig_pixels[2][3]);
  EXPECT_EQ(88u, orig_pixels[3][0]);
  EXPECT_EQ(114u, orig_pixels[3][1]);
  EXPECT_EQ(127u, orig_pixels[3][2]);
  EXPECT_EQ(224u, orig_pixels[3][3]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i;
  }
  for (int i = 0; i < benchmark_pixels_div256_; ++i) {
    ARGBSepia(&orig_pixels[0][0], 0, 0, 0, 256, 1);
  }
}

TEST_F(libyuvTest, TestARGBColorMatrix) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);

  // Matrix for Sepia.
  static const int8 kARGBToSepia[] = {
    17, 68, 35, 0,
    22, 88, 45, 0,
    24, 98, 50, 0,
    0, 0, 0, 0,  // Unused but makes matrix 16 bytes.
  };

  // Test blue
  orig_pixels[0][0] = 255u;
  orig_pixels[0][1] = 0u;
  orig_pixels[0][2] = 0u;
  orig_pixels[0][3] = 128u;
  // Test green
  orig_pixels[1][0] = 0u;
  orig_pixels[1][1] = 255u;
  orig_pixels[1][2] = 0u;
  orig_pixels[1][3] = 0u;
  // Test red
  orig_pixels[2][0] = 0u;
  orig_pixels[2][1] = 0u;
  orig_pixels[2][2] = 255u;
  orig_pixels[2][3] = 255u;
  // Test color
  orig_pixels[3][0] = 16u;
  orig_pixels[3][1] = 64u;
  orig_pixels[3][2] = 192u;
  orig_pixels[3][3] = 224u;
  // Do 16 to test asm version.
  ARGBColorMatrix(&orig_pixels[0][0], 0, &kARGBToSepia[0], 0, 0, 16, 1);
  EXPECT_EQ(33u, orig_pixels[0][0]);
  EXPECT_EQ(43u, orig_pixels[0][1]);
  EXPECT_EQ(47u, orig_pixels[0][2]);
  EXPECT_EQ(128u, orig_pixels[0][3]);
  EXPECT_EQ(135u, orig_pixels[1][0]);
  EXPECT_EQ(175u, orig_pixels[1][1]);
  EXPECT_EQ(195u, orig_pixels[1][2]);
  EXPECT_EQ(0u, orig_pixels[1][3]);
  EXPECT_EQ(69u, orig_pixels[2][0]);
  EXPECT_EQ(89u, orig_pixels[2][1]);
  EXPECT_EQ(99u, orig_pixels[2][2]);
  EXPECT_EQ(255u, orig_pixels[2][3]);
  EXPECT_EQ(88u, orig_pixels[3][0]);
  EXPECT_EQ(114u, orig_pixels[3][1]);
  EXPECT_EQ(127u, orig_pixels[3][2]);
  EXPECT_EQ(224u, orig_pixels[3][3]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i;
  }
  for (int i = 0; i < benchmark_pixels_div256_; ++i) {
    ARGBColorMatrix(&orig_pixels[0][0], 0, &kARGBToSepia[0], 0, 0, 256, 1);
  }
}

TEST_F(libyuvTest, TestARGBColorTable) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);
  memset(orig_pixels, 0, sizeof(orig_pixels));

  // Matrix for Sepia.
  static const uint8 kARGBTable[256 * 4] = {
    1u, 2u, 3u, 4u,
    5u, 6u, 7u, 8u,
    9u, 10u, 11u, 12u,
    13u, 14u, 15u, 16u,
  };

  orig_pixels[0][0] = 0u;
  orig_pixels[0][1] = 0u;
  orig_pixels[0][2] = 0u;
  orig_pixels[0][3] = 0u;
  orig_pixels[1][0] = 1u;
  orig_pixels[1][1] = 1u;
  orig_pixels[1][2] = 1u;
  orig_pixels[1][3] = 1u;
  orig_pixels[2][0] = 2u;
  orig_pixels[2][1] = 2u;
  orig_pixels[2][2] = 2u;
  orig_pixels[2][3] = 2u;
  orig_pixels[3][0] = 0u;
  orig_pixels[3][1] = 1u;
  orig_pixels[3][2] = 2u;
  orig_pixels[3][3] = 3u;
  // Do 16 to test asm version.
  ARGBColorTable(&orig_pixels[0][0], 0, &kARGBTable[0], 0, 0, 16, 1);
  EXPECT_EQ(1u, orig_pixels[0][0]);
  EXPECT_EQ(2u, orig_pixels[0][1]);
  EXPECT_EQ(3u, orig_pixels[0][2]);
  EXPECT_EQ(4u, orig_pixels[0][3]);
  EXPECT_EQ(5u, orig_pixels[1][0]);
  EXPECT_EQ(6u, orig_pixels[1][1]);
  EXPECT_EQ(7u, orig_pixels[1][2]);
  EXPECT_EQ(8u, orig_pixels[1][3]);
  EXPECT_EQ(9u, orig_pixels[2][0]);
  EXPECT_EQ(10u, orig_pixels[2][1]);
  EXPECT_EQ(11u, orig_pixels[2][2]);
  EXPECT_EQ(12u, orig_pixels[2][3]);
  EXPECT_EQ(1u, orig_pixels[3][0]);
  EXPECT_EQ(6u, orig_pixels[3][1]);
  EXPECT_EQ(11u, orig_pixels[3][2]);
  EXPECT_EQ(16u, orig_pixels[3][3]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i;
  }
  for (int i = 0; i < benchmark_pixels_div256_; ++i) {
    ARGBColorTable(&orig_pixels[0][0], 0, &kARGBTable[0], 0, 0, 256, 1);
  }
}

TEST_F(libyuvTest, TestARGBQuantize) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i;
  }
  ARGBQuantize(&orig_pixels[0][0], 0,
               (65536 + (8 / 2)) / 8, 8, 8 / 2, 0, 0, 256, 1);

  for (int i = 0; i < 256; ++i) {
    EXPECT_EQ(i / 8 * 8 + 8 / 2, orig_pixels[i][0]);
    EXPECT_EQ(i / 2 / 8 * 8 + 8 / 2, orig_pixels[i][1]);
    EXPECT_EQ(i / 3 / 8 * 8 + 8 / 2, orig_pixels[i][2]);
    EXPECT_EQ(i, orig_pixels[i][3]);
  }
  for (int i = 0; i < benchmark_pixels_div256_; ++i) {
    ARGBQuantize(&orig_pixels[0][0], 0,
                 (65536 + (8 / 2)) / 8, 8, 8 / 2, 0, 0, 256, 1);
  }
}

TEST_F(libyuvTest, TestARGBMirror) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);
  SIMD_ALIGNED(uint8 dst_pixels[256][4]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i / 4;
  }
  ARGBMirror(&orig_pixels[0][0], 0, &dst_pixels[0][0], 0, 256, 1);

  for (int i = 0; i < 256; ++i) {
    EXPECT_EQ(i, dst_pixels[255 - i][0]);
    EXPECT_EQ(i / 2, dst_pixels[255 - i][1]);
    EXPECT_EQ(i / 3, dst_pixels[255 - i][2]);
    EXPECT_EQ(i / 4, dst_pixels[255 - i][3]);
  }
  for (int i = 0; i < benchmark_pixels_div256_; ++i) {
    ARGBMirror(&orig_pixels[0][0], 0, &dst_pixels[0][0], 0, 256, 1);
  }
}

TEST_F(libyuvTest, TestShade) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);
  SIMD_ALIGNED(uint8 shade_pixels[256][4]);

  orig_pixels[0][0] = 10u;
  orig_pixels[0][1] = 20u;
  orig_pixels[0][2] = 40u;
  orig_pixels[0][3] = 80u;
  orig_pixels[1][0] = 0u;
  orig_pixels[1][1] = 0u;
  orig_pixels[1][2] = 0u;
  orig_pixels[1][3] = 255u;
  orig_pixels[2][0] = 0u;
  orig_pixels[2][1] = 0u;
  orig_pixels[2][2] = 0u;
  orig_pixels[2][3] = 0u;
  orig_pixels[3][0] = 0u;
  orig_pixels[3][1] = 0u;
  orig_pixels[3][2] = 0u;
  orig_pixels[3][3] = 0u;
  // Do 8 pixels to allow opt version to be used.
  ARGBShade(&orig_pixels[0][0], 0, &shade_pixels[0][0], 0, 8, 1, 0x80ffffff);
  EXPECT_EQ(10u, shade_pixels[0][0]);
  EXPECT_EQ(20u, shade_pixels[0][1]);
  EXPECT_EQ(40u, shade_pixels[0][2]);
  EXPECT_EQ(40u, shade_pixels[0][3]);
  EXPECT_EQ(0u, shade_pixels[1][0]);
  EXPECT_EQ(0u, shade_pixels[1][1]);
  EXPECT_EQ(0u, shade_pixels[1][2]);
  EXPECT_EQ(128u, shade_pixels[1][3]);
  EXPECT_EQ(0u, shade_pixels[2][0]);
  EXPECT_EQ(0u, shade_pixels[2][1]);
  EXPECT_EQ(0u, shade_pixels[2][2]);
  EXPECT_EQ(0u, shade_pixels[2][3]);
  EXPECT_EQ(0u, shade_pixels[3][0]);
  EXPECT_EQ(0u, shade_pixels[3][1]);
  EXPECT_EQ(0u, shade_pixels[3][2]);
  EXPECT_EQ(0u, shade_pixels[3][3]);

  ARGBShade(&orig_pixels[0][0], 0, &shade_pixels[0][0], 0, 8, 1, 0x80808080);
  EXPECT_EQ(5u, shade_pixels[0][0]);
  EXPECT_EQ(10u, shade_pixels[0][1]);
  EXPECT_EQ(20u, shade_pixels[0][2]);
  EXPECT_EQ(40u, shade_pixels[0][3]);

  ARGBShade(&orig_pixels[0][0], 0, &shade_pixels[0][0], 0, 8, 1, 0x10204080);
  EXPECT_EQ(5u, shade_pixels[0][0]);
  EXPECT_EQ(5u, shade_pixels[0][1]);
  EXPECT_EQ(5u, shade_pixels[0][2]);
  EXPECT_EQ(5u, shade_pixels[0][3]);

  for (int i = 0; i < benchmark_pixels_div256_; ++i) {
    ARGBShade(&orig_pixels[0][0], 0, &shade_pixels[0][0], 0, 256, 1,
              0x80808080);
  }
}

TEST_F(libyuvTest, TestInterpolate) {
  SIMD_ALIGNED(uint8 orig_pixels_0[256][4]);
  SIMD_ALIGNED(uint8 orig_pixels_1[256][4]);
  SIMD_ALIGNED(uint8 interpolate_pixels[256][4]);

  orig_pixels_0[0][0] = 16u;
  orig_pixels_0[0][1] = 32u;
  orig_pixels_0[0][2] = 64u;
  orig_pixels_0[0][3] = 128u;
  orig_pixels_0[1][0] = 0u;
  orig_pixels_0[1][1] = 0u;
  orig_pixels_0[1][2] = 0u;
  orig_pixels_0[1][3] = 255u;
  orig_pixels_0[2][0] = 0u;
  orig_pixels_0[2][1] = 0u;
  orig_pixels_0[2][2] = 0u;
  orig_pixels_0[2][3] = 0u;
  orig_pixels_0[3][0] = 0u;
  orig_pixels_0[3][1] = 0u;
  orig_pixels_0[3][2] = 0u;
  orig_pixels_0[3][3] = 0u;

  orig_pixels_1[0][0] = 0u;
  orig_pixels_1[0][1] = 0u;
  orig_pixels_1[0][2] = 0u;
  orig_pixels_1[0][3] = 0u;
  orig_pixels_1[1][0] = 0u;
  orig_pixels_1[1][1] = 0u;
  orig_pixels_1[1][2] = 0u;
  orig_pixels_1[1][3] = 0u;
  orig_pixels_1[2][0] = 0u;
  orig_pixels_1[2][1] = 0u;
  orig_pixels_1[2][2] = 0u;
  orig_pixels_1[2][3] = 0u;
  orig_pixels_1[3][0] = 255u;
  orig_pixels_1[3][1] = 255u;
  orig_pixels_1[3][2] = 255u;
  orig_pixels_1[3][3] = 255u;

  ARGBInterpolate(&orig_pixels_0[0][0], 0, &orig_pixels_1[0][0], 0,
                  &interpolate_pixels[0][0], 0, 4, 1, 128);
  EXPECT_EQ(8u, interpolate_pixels[0][0]);
  EXPECT_EQ(16u, interpolate_pixels[0][1]);
  EXPECT_EQ(32u, interpolate_pixels[0][2]);
  EXPECT_EQ(64u, interpolate_pixels[0][3]);
  EXPECT_EQ(0u, interpolate_pixels[1][0]);
  EXPECT_EQ(0u, interpolate_pixels[1][1]);
  EXPECT_EQ(0u, interpolate_pixels[1][2]);
  EXPECT_NEAR(128u, interpolate_pixels[1][3], 1);  // C = 127, SSE = 128.
  EXPECT_EQ(0u, interpolate_pixels[2][0]);
  EXPECT_EQ(0u, interpolate_pixels[2][1]);
  EXPECT_EQ(0u, interpolate_pixels[2][2]);
  EXPECT_EQ(0u, interpolate_pixels[2][3]);
  EXPECT_NEAR(128u, interpolate_pixels[3][0], 1);
  EXPECT_NEAR(128u, interpolate_pixels[3][1], 1);
  EXPECT_NEAR(128u, interpolate_pixels[3][2], 1);
  EXPECT_NEAR(128u, interpolate_pixels[3][3], 1);

  ARGBInterpolate(&orig_pixels_0[0][0], 0, &orig_pixels_1[0][0], 0,
                  &interpolate_pixels[0][0], 0, 4, 1, 0);
  EXPECT_EQ(16u, interpolate_pixels[0][0]);
  EXPECT_EQ(32u, interpolate_pixels[0][1]);
  EXPECT_EQ(64u, interpolate_pixels[0][2]);
  EXPECT_EQ(128u, interpolate_pixels[0][3]);

  ARGBInterpolate(&orig_pixels_0[0][0], 0, &orig_pixels_1[0][0], 0,
                  &interpolate_pixels[0][0], 0, 4, 1, 192);

  EXPECT_EQ(4u, interpolate_pixels[0][0]);
  EXPECT_EQ(8u, interpolate_pixels[0][1]);
  EXPECT_EQ(16u, interpolate_pixels[0][2]);
  EXPECT_EQ(32u, interpolate_pixels[0][3]);

  for (int i = 0; i < benchmark_pixels_div256_; ++i) {
    ARGBInterpolate(&orig_pixels_0[0][0], 0, &orig_pixels_1[0][0], 0,
                    &interpolate_pixels[0][0], 0, 256, 1, 128);
  }
}

#define TESTTERP(FMT_A, BPP_A, STRIDE_A,                                       \
                 FMT_B, BPP_B, STRIDE_B,                                       \
                 W1280, TERP, DIFF, N, NEG, OFF)                               \
TEST_F(libyuvTest, ARGBInterpolate##TERP##N) {                                 \
  const int kWidth = W1280;                                                    \
  const int kHeight = benchmark_height_;                                       \
  const int kStrideA = (kWidth * BPP_A + STRIDE_A - 1) / STRIDE_A * STRIDE_A;  \
  const int kStrideB = (kWidth * BPP_B + STRIDE_B - 1) / STRIDE_B * STRIDE_B;  \
  align_buffer_64(src_argb_a, kStrideA * kHeight + OFF);                       \
  align_buffer_64(src_argb_b, kStrideA * kHeight + OFF);                       \
  align_buffer_64(dst_argb_c, kStrideB * kHeight);                             \
  align_buffer_64(dst_argb_opt, kStrideB * kHeight);                           \
  srandom(time(NULL));                                                         \
  for (int i = 0; i < kStrideA * kHeight; ++i) {                               \
    src_argb_a[i + OFF] = (random() & 0xff);                                   \
    src_argb_b[i + OFF] = (random() & 0xff);                                   \
  }                                                                            \
  MaskCpuFlags(0);                                                             \
  ARGBInterpolate(src_argb_a + OFF, kStrideA,                                  \
                  src_argb_b + OFF, kStrideA,                                  \
                  dst_argb_c, kStrideB,                                        \
                  kWidth, NEG kHeight, TERP);                                  \
  MaskCpuFlags(-1);                                                            \
  for (int i = 0; i < benchmark_iterations_; ++i) {                            \
    ARGBInterpolate(src_argb_a + OFF, kStrideA,                                \
                    src_argb_b + OFF, kStrideA,                                \
                    dst_argb_opt, kStrideB,                                    \
                    kWidth, NEG kHeight, TERP);                                \
  }                                                                            \
  int max_diff = 0;                                                            \
  for (int i = 0; i < kStrideB * kHeight; ++i) {                               \
    int abs_diff =                                                             \
        abs(static_cast<int>(dst_argb_c[i]) -                                  \
            static_cast<int>(dst_argb_opt[i]));                                \
    if (abs_diff > max_diff) {                                                 \
      max_diff = abs_diff;                                                     \
    }                                                                          \
  }                                                                            \
  EXPECT_LE(max_diff, DIFF);                                                   \
  free_aligned_buffer_64(src_argb_a)                                           \
  free_aligned_buffer_64(src_argb_b)                                           \
  free_aligned_buffer_64(dst_argb_c)                                           \
  free_aligned_buffer_64(dst_argb_opt)                                         \
}

#define TESTINTERPOLATE(TERP)                                                  \
    TESTTERP(ARGB, 4, 1, ARGB, 4, 1,                                           \
             benchmark_width_ - 1, TERP, 1, _Any, +, 0)                        \
    TESTTERP(ARGB, 4, 1, ARGB, 4, 1,                                           \
             benchmark_width_, TERP, 1, _Unaligned, +, 1)                      \
    TESTTERP(ARGB, 4, 1, ARGB, 4, 1,                                           \
             benchmark_width_, TERP, 1, _Invert, -, 0)                         \
    TESTTERP(ARGB, 4, 1, ARGB, 4, 1,                                           \
             benchmark_width_, TERP, 1, _Opt, +, 0)

TESTINTERPOLATE(0)
TESTINTERPOLATE(64)
TESTINTERPOLATE(128)
TESTINTERPOLATE(192)
TESTINTERPOLATE(255)
TESTINTERPOLATE(85)

static int TestBlend(int width, int height, int benchmark_iterations,
                     int invert, int off) {
  const int kBpp = 4;
  const int kStride = width * kBpp;
  align_buffer_64(src_argb_a, kStride * height + off);
  align_buffer_64(src_argb_b, kStride * height + off);
  align_buffer_64(dst_argb_c, kStride * height);
  align_buffer_64(dst_argb_opt, kStride * height);
  srandom(time(NULL));
  for (int i = 0; i < kStride * height; ++i) {
    src_argb_a[i + off] = (random() & 0xff);
    src_argb_b[i + off] = (random() & 0xff);
  }
  ARGBAttenuate(src_argb_a + off, kStride, src_argb_a + off, kStride, width,
                height);
  ARGBAttenuate(src_argb_b + off, kStride, src_argb_b + off, kStride, width,
                height);
  memset(dst_argb_c, 255, kStride * height);
  memset(dst_argb_opt, 255, kStride * height);

  MaskCpuFlags(0);
  ARGBBlend(src_argb_a + off, kStride,
            src_argb_b + off, kStride,
            dst_argb_c, kStride,
            width, invert * height);
  MaskCpuFlags(-1);
  for (int i = 0; i < benchmark_iterations; ++i) {
    ARGBBlend(src_argb_a + off, kStride,
              src_argb_b + off, kStride,
              dst_argb_opt, kStride,
              width, invert * height);
  }
  int max_diff = 0;
  for (int i = 0; i < kStride * height; ++i) {
    int abs_diff =
        abs(static_cast<int>(dst_argb_c[i]) -
            static_cast<int>(dst_argb_opt[i]));
    if (abs_diff > max_diff) {
      max_diff = abs_diff;
    }
  }
  free_aligned_buffer_64(src_argb_a)
  free_aligned_buffer_64(src_argb_b)
  free_aligned_buffer_64(dst_argb_c)
  free_aligned_buffer_64(dst_argb_opt)
  return max_diff;
}

TEST_F(libyuvTest, ARGBBlend_Any) {
  int max_diff = TestBlend(benchmark_width_ - 4, benchmark_height_,
                           benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBBlend_Unaligned) {
  int max_diff = TestBlend(benchmark_width_, benchmark_height_,
                           benchmark_iterations_, +1, 1);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBBlend_Invert) {
  int max_diff = TestBlend(benchmark_width_, benchmark_height_,
                           benchmark_iterations_, -1, 0);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBBlend_Opt) {
  int max_diff = TestBlend(benchmark_width_, benchmark_height_,
                           benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, TestAffine) {
  SIMD_ALIGNED(uint8 orig_pixels_0[256][4]);
  SIMD_ALIGNED(uint8 interpolate_pixels_C[256][4]);

  for (int i = 0; i < 256; ++i) {
    for (int j = 0; j < 4; ++j) {
      orig_pixels_0[i][j] = i;
    }
  }

  float uv_step[4] = { 0.f, 0.f, 0.75f, 0.f };

  ARGBAffineRow_C(&orig_pixels_0[0][0], 0, &interpolate_pixels_C[0][0],
                  uv_step, 256);
  EXPECT_EQ(0u, interpolate_pixels_C[0][0]);
  EXPECT_EQ(96u, interpolate_pixels_C[128][0]);
  EXPECT_EQ(191u, interpolate_pixels_C[255][3]);

#if defined(HAS_ARGBAFFINEROW_SSE2)
  SIMD_ALIGNED(uint8 interpolate_pixels_Opt[256][4]);
  ARGBAffineRow_SSE2(&orig_pixels_0[0][0], 0, &interpolate_pixels_Opt[0][0],
                     uv_step, 256);
  EXPECT_EQ(0, memcmp(interpolate_pixels_Opt, interpolate_pixels_C, 256 * 4));

  int has_sse2 = TestCpuFlag(kCpuHasSSE2);
  if (has_sse2) {
    for (int i = 0; i < benchmark_pixels_div256_; ++i) {
      ARGBAffineRow_SSE2(&orig_pixels_0[0][0], 0, &interpolate_pixels_Opt[0][0],
                         uv_step, 256);
    }
  }
#endif
}

TEST_F(libyuvTest, TestCopyPlane) {
  int err = 0;
  int yw = benchmark_width_;
  int yh = benchmark_height_;
  int b = 12;
  int i, j;

  int y_plane_size = (yw + b * 2) * (yh + b * 2);
  srandom(time(NULL));
  align_buffer_64(orig_y, y_plane_size)
  align_buffer_64(dst_c, y_plane_size)
  align_buffer_64(dst_opt, y_plane_size);

  memset(orig_y, 0, y_plane_size);
  memset(dst_c, 0, y_plane_size);
  memset(dst_opt, 0, y_plane_size);

  // Fill image buffers with random data.
  for (i = b; i < (yh + b); ++i) {
    for (j = b; j < (yw + b); ++j) {
      orig_y[i * (yw + b * 2) + j] = random() & 0xff;
    }
  }

  // Fill destination buffers with random data.
  for (i = 0; i < y_plane_size; ++i) {
    uint8 random_number = random() & 0x7f;
    dst_c[i] = random_number;
    dst_opt[i] = dst_c[i];
  }

  int y_off = b * (yw + b * 2) + b;

  int y_st = yw + b * 2;
  int stride = 8;

  // Disable all optimizations.
  MaskCpuFlags(0);
  double c_time = get_time();
  for (j = 0; j < benchmark_iterations_; j++) {
    CopyPlane(orig_y + y_off, y_st, dst_c + y_off, stride, yw, yh);
  }
  c_time = (get_time() - c_time) / benchmark_iterations_;

  // Enable optimizations.
  MaskCpuFlags(-1);
  double opt_time = get_time();
  for (j = 0; j < benchmark_iterations_; j++) {
    CopyPlane(orig_y + y_off, y_st, dst_opt + y_off, stride, yw, yh);
  }
  opt_time = (get_time() - opt_time) / benchmark_iterations_;
  printf(" %8d us C - %8d us OPT\n",
         static_cast<int>(c_time * 1e6), static_cast<int>(opt_time * 1e6));

  for (i = 0; i < y_plane_size; ++i) {
    if (dst_c[i] != dst_opt[i])
      ++err;
  }

  free_aligned_buffer_64(orig_y)
  free_aligned_buffer_64(dst_c)
  free_aligned_buffer_64(dst_opt)

  EXPECT_EQ(0, err);
}

static int TestMultiply(int width, int height, int benchmark_iterations,
                        int invert, int off) {
  const int kBpp = 4;
  const int kStride = (width * kBpp + 15) & ~15;
  align_buffer_64(src_argb_a, kStride * height + off);
  align_buffer_64(src_argb_b, kStride * height + off);
  align_buffer_64(dst_argb_c, kStride * height);
  align_buffer_64(dst_argb_opt, kStride * height);
  srandom(time(NULL));
  for (int i = 0; i < kStride * height; ++i) {
    src_argb_a[i + off] = (random() & 0xff);
    src_argb_b[i + off] = (random() & 0xff);
  }
  memset(dst_argb_c, 0, kStride * height);
  memset(dst_argb_opt, 0, kStride * height);

  MaskCpuFlags(0);
  ARGBMultiply(src_argb_a + off, kStride,
               src_argb_b + off, kStride,
               dst_argb_c, kStride,
               width, invert * height);
  MaskCpuFlags(-1);
  for (int i = 0; i < benchmark_iterations; ++i) {
    ARGBMultiply(src_argb_a + off, kStride,
                 src_argb_b + off, kStride,
                 dst_argb_opt, kStride,
                 width, invert * height);
  }
  int max_diff = 0;
  for (int i = 0; i < kStride * height; ++i) {
    int abs_diff =
        abs(static_cast<int>(dst_argb_c[i]) -
            static_cast<int>(dst_argb_opt[i]));
    if (abs_diff > max_diff) {
      max_diff = abs_diff;
    }
  }
  free_aligned_buffer_64(src_argb_a)
  free_aligned_buffer_64(src_argb_b)
  free_aligned_buffer_64(dst_argb_c)
  free_aligned_buffer_64(dst_argb_opt)
  return max_diff;
}

TEST_F(libyuvTest, ARGBMultiply_Any) {
  int max_diff = TestMultiply(benchmark_width_ - 1, benchmark_height_,
                              benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBMultiply_Unaligned) {
  int max_diff = TestMultiply(benchmark_width_, benchmark_height_,
                              benchmark_iterations_, +1, 1);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBMultiply_Invert) {
  int max_diff = TestMultiply(benchmark_width_, benchmark_height_,
                              benchmark_iterations_, -1, 0);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBMultiply_Opt) {
  int max_diff = TestMultiply(benchmark_width_, benchmark_height_,
                              benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 1);
}

static int TestAdd(int width, int height, int benchmark_iterations,
                   int invert, int off) {
  const int kBpp = 4;
  const int kStride = (width * kBpp + 15) & ~15;
  align_buffer_64(src_argb_a, kStride * height + off);
  align_buffer_64(src_argb_b, kStride * height + off);
  align_buffer_64(dst_argb_c, kStride * height);
  align_buffer_64(dst_argb_opt, kStride * height);
  srandom(time(NULL));
  for (int i = 0; i < kStride * height; ++i) {
    src_argb_a[i + off] = (random() & 0xff);
    src_argb_b[i + off] = (random() & 0xff);
  }
  memset(dst_argb_c, 0, kStride * height);
  memset(dst_argb_opt, 0, kStride * height);

  MaskCpuFlags(0);
  ARGBAdd(src_argb_a + off, kStride,
          src_argb_b + off, kStride,
          dst_argb_c, kStride,
          width, invert * height);
  MaskCpuFlags(-1);
  for (int i = 0; i < benchmark_iterations; ++i) {
    ARGBAdd(src_argb_a + off, kStride,
            src_argb_b + off, kStride,
            dst_argb_opt, kStride,
            width, invert * height);
  }
  int max_diff = 0;
  for (int i = 0; i < kStride * height; ++i) {
    int abs_diff =
        abs(static_cast<int>(dst_argb_c[i]) -
            static_cast<int>(dst_argb_opt[i]));
    if (abs_diff > max_diff) {
      max_diff = abs_diff;
    }
  }
  free_aligned_buffer_64(src_argb_a)
  free_aligned_buffer_64(src_argb_b)
  free_aligned_buffer_64(dst_argb_c)
  free_aligned_buffer_64(dst_argb_opt)
  return max_diff;
}

TEST_F(libyuvTest, ARGBAdd_Any) {
  int max_diff = TestAdd(benchmark_width_ - 1, benchmark_height_,
                         benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBAdd_Unaligned) {
  int max_diff = TestAdd(benchmark_width_, benchmark_height_,
                         benchmark_iterations_, +1, 1);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBAdd_Invert) {
  int max_diff = TestAdd(benchmark_width_, benchmark_height_,
                         benchmark_iterations_, -1, 0);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBAdd_Opt) {
  int max_diff = TestAdd(benchmark_width_, benchmark_height_,
                         benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 1);
}

static int TestSubtract(int width, int height, int benchmark_iterations,
                        int invert, int off) {
  const int kBpp = 4;
  const int kStride = (width * kBpp + 15) & ~15;
  align_buffer_64(src_argb_a, kStride * height + off);
  align_buffer_64(src_argb_b, kStride * height + off);
  align_buffer_64(dst_argb_c, kStride * height);
  align_buffer_64(dst_argb_opt, kStride * height);
  srandom(time(NULL));
  for (int i = 0; i < kStride * height; ++i) {
    src_argb_a[i + off] = (random() & 0xff);
    src_argb_b[i + off] = (random() & 0xff);
  }
  memset(dst_argb_c, 0, kStride * height);
  memset(dst_argb_opt, 0, kStride * height);

  MaskCpuFlags(0);
  ARGBSubtract(src_argb_a + off, kStride,
               src_argb_b + off, kStride,
               dst_argb_c, kStride,
               width, invert * height);
  MaskCpuFlags(-1);
  for (int i = 0; i < benchmark_iterations; ++i) {
    ARGBSubtract(src_argb_a + off, kStride,
                 src_argb_b + off, kStride,
                 dst_argb_opt, kStride,
                 width, invert * height);
  }
  int max_diff = 0;
  for (int i = 0; i < kStride * height; ++i) {
    int abs_diff =
        abs(static_cast<int>(dst_argb_c[i]) -
            static_cast<int>(dst_argb_opt[i]));
    if (abs_diff > max_diff) {
      max_diff = abs_diff;
    }
  }
  free_aligned_buffer_64(src_argb_a)
  free_aligned_buffer_64(src_argb_b)
  free_aligned_buffer_64(dst_argb_c)
  free_aligned_buffer_64(dst_argb_opt)
  return max_diff;
}

TEST_F(libyuvTest, ARGBSubtract_Any) {
  int max_diff = TestSubtract(benchmark_width_ - 1, benchmark_height_,
                              benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBSubtract_Unaligned) {
  int max_diff = TestSubtract(benchmark_width_, benchmark_height_,
                              benchmark_iterations_, +1, 1);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBSubtract_Invert) {
  int max_diff = TestSubtract(benchmark_width_, benchmark_height_,
                              benchmark_iterations_, -1, 0);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBSubtract_Opt) {
  int max_diff = TestSubtract(benchmark_width_, benchmark_height_,
                              benchmark_iterations_, +1, 0);
  EXPECT_LE(max_diff, 1);
}

}  // namespace libyuv
