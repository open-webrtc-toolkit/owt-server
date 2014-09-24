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

#include "libyuv/cpu_id.h"
#include "libyuv/scale_argb.h"
#include "../unit_test/unit_test.h"

namespace libyuv {

static int ARGBTestFilter(int src_width, int src_height,
                          int dst_width, int dst_height,
                          FilterMode f, int benchmark_iterations) {
  const int b = 128;
  int src_argb_plane_size = (src_width + b * 2) * (src_height + b * 2) * 4;
  int src_stride_argb = (b * 2 + src_width) * 4;

  align_buffer_64(src_argb, src_argb_plane_size)
  memset(src_argb, 1, src_argb_plane_size);

  int dst_argb_plane_size = (dst_width + b * 2) * (dst_height + b * 2) * 4;
  int dst_stride_argb = (b * 2 + dst_width) * 4;

  srandom(time(NULL));

  int i, j;
  for (i = b; i < (src_height + b); ++i) {
    for (j = b; j < (src_width + b) * 4; ++j) {
      src_argb[(i * src_stride_argb) + j] = (random() & 0xff);
    }
  }

  align_buffer_64(dst_argb_c, dst_argb_plane_size)
  align_buffer_64(dst_argb_opt, dst_argb_plane_size)
  memset(dst_argb_c, 2, dst_argb_plane_size);
  memset(dst_argb_opt, 3, dst_argb_plane_size);

  // Warm up both versions for consistent benchmarks.
  MaskCpuFlags(0);  // Disable all CPU optimization.
  ARGBScale(src_argb + (src_stride_argb * b) + b * 4, src_stride_argb,
            src_width, src_height,
            dst_argb_c + (dst_stride_argb * b) + b * 4, dst_stride_argb,
            dst_width, dst_height, f);
  MaskCpuFlags(-1);  // Enable all CPU optimization.
  ARGBScale(src_argb + (src_stride_argb * b) + b * 4, src_stride_argb,
            src_width, src_height,
            dst_argb_opt + (dst_stride_argb * b) + b * 4, dst_stride_argb,
            dst_width, dst_height, f);

  MaskCpuFlags(0);  // Disable all CPU optimization.
  double c_time = get_time();
  ARGBScale(src_argb + (src_stride_argb * b) + b * 4, src_stride_argb,
            src_width, src_height,
            dst_argb_c + (dst_stride_argb * b) + b * 4, dst_stride_argb,
            dst_width, dst_height, f);

  c_time = (get_time() - c_time);

  MaskCpuFlags(-1);  // Enable all CPU optimization.
  double opt_time = get_time();
  for (i = 0; i < benchmark_iterations; ++i) {
    ARGBScale(src_argb + (src_stride_argb * b) + b * 4, src_stride_argb,
              src_width, src_height,
              dst_argb_opt + (dst_stride_argb * b) + b * 4, dst_stride_argb,
              dst_width, dst_height, f);
  }
  opt_time = (get_time() - opt_time) / benchmark_iterations;

  // Report performance of C vs OPT
  printf("filter %d - %8d us C - %8d us OPT\n",
         f, static_cast<int>(c_time * 1e6), static_cast<int>(opt_time * 1e6));

  // C version may be a little off from the optimized. Order of
  //  operations may introduce rounding somewhere. So do a difference
  //  of the buffers and look to see that the max difference isn't
  //  over 2.
  int max_diff = 0;
  for (i = b; i < (dst_height + b); ++i) {
    for (j = b * 4; j < (dst_width + b) * 4; ++j) {
      int abs_diff = abs(dst_argb_c[(i * dst_stride_argb) + j] -
                         dst_argb_opt[(i * dst_stride_argb) + j]);
      if (abs_diff > max_diff) {
        max_diff = abs_diff;
      }
    }
  }

  free_aligned_buffer_64(dst_argb_c)
  free_aligned_buffer_64(dst_argb_opt)
  free_aligned_buffer_64(src_argb)
  return max_diff;
}

TEST_F(libyuvTest, ARGBScaleDownBy2_None) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 2;
  const int dst_height = src_height / 2;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterNone,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy2_Bilinear) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 2;
  const int dst_height = src_height / 2;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBilinear,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy2_Box) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 2;
  const int dst_height = src_height / 2;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBox,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy4_None) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 4;
  const int dst_height = src_height / 4;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterNone,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 2);  // This is the only scale factor with error of 2.
}

TEST_F(libyuvTest, ARGBScaleDownBy4_Bilinear) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 4;
  const int dst_height = src_height / 4;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBilinear,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 2);  // This is the only scale factor with error of 2.
}

TEST_F(libyuvTest, ARGBScaleDownBy4_Box) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 4;
  const int dst_height = src_height / 4;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBox,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 2);  // This is the only scale factor with error of 2.
}

TEST_F(libyuvTest, ARGBScaleDownBy5_None) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 5;
  const int dst_height = src_height / 5;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterNone,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy5_Bilinear) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 5;
  const int dst_height = src_height / 5;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBilinear,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy5_Box) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 5;
  const int dst_height = src_height / 5;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBox,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy8_None) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 8;
  const int dst_height = src_height / 8;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterNone,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy8_Bilinear) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 8;
  const int dst_height = src_height / 8;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBilinear,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy8_Box) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 8;
  const int dst_height = src_height / 8;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBox,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy16_None) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 16;
  const int dst_height = src_height / 16;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterNone,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy16_Bilinear) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width / 16;
  const int dst_height = src_height / 16;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBilinear,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy34_None) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width * 3 / 4;
  const int dst_height = src_height * 3 / 4;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterNone,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy34_Bilinear) {
  const int src_width = benchmark_width_;
  const int src_height = benchmark_height_;
  const int dst_width = src_width * 3 / 4;
  const int dst_height = src_height * 3 / 4;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBilinear,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy38_None) {
  int src_width = benchmark_width_;
  int src_height = benchmark_height_;
  int dst_width = src_width * 3 / 8;
  int dst_height = src_height * 3 / 8;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterNone,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleDownBy38_Bilinear) {
  int src_width = benchmark_width_;
  int src_height = benchmark_height_;
  int dst_width = src_width * 3 / 8;
  int dst_height = src_height * 3 / 8;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBilinear,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleTo1366x768_None) {
  int src_width = benchmark_width_;
  int src_height = benchmark_height_;
  int dst_width = 1366;
  int dst_height = 768;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterNone,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleTo1366x768_Bilinear) {
  int src_width = benchmark_width_;
  int src_height = benchmark_height_;
  int dst_width = 1366;
  int dst_height = 768;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBilinear,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}


TEST_F(libyuvTest, ARGBScaleTo1280x720_None) {
  int src_width = benchmark_width_;
  int src_height = benchmark_height_;
  int dst_width = 1280;
  int dst_height = 720;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterNone,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleTo1280x720_Bilinear) {
  int src_width = benchmark_width_;
  int src_height = benchmark_height_;
  int dst_width = 1280;
  int dst_height = 720;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBilinear,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleTo853x480_None) {
  int src_width = benchmark_width_;
  int src_height = benchmark_height_;
  int dst_width = 853;
  int dst_height = 480;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterNone,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

TEST_F(libyuvTest, ARGBScaleTo853x480_Bilinear) {
  int src_width = benchmark_width_;
  int src_height = benchmark_height_;
  int dst_width = 853;
  int dst_height = 480;

  int max_diff = ARGBTestFilter(src_width, src_height,
                                dst_width, dst_height,
                                kFilterBilinear,
                                benchmark_iterations_);
  EXPECT_LE(max_diff, 1);
}

}  // namespace libyuv
