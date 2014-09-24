/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "third_party/googletest/src/include/gtest/gtest.h"

extern "C" {
#include "vp9/common/vp9_entropy.h"
#include "./vp9_rtcd.h"
  void vp9_short_fdct32x32_c(int16_t *input, int16_t *out, int pitch);
  void vp9_short_idct32x32_c(short *input, short *output, int pitch);
}

#include "test/acm_random.h"
#include "vpx/vpx_integer.h"

using libvpx_test::ACMRandom;

namespace {
#ifdef _MSC_VER
static int round(double x) {
  if (x < 0)
    return (int)ceil(x - 0.5);
  else
    return (int)floor(x + 0.5);
}
#endif

#if !CONFIG_DWTDCTHYBRID
static const double kPi = 3.141592653589793238462643383279502884;
static void reference2_32x32_idct_2d(double *input, double *output) {
  double x;
  for (int l = 0; l < 32; ++l) {
    for (int k = 0; k < 32; ++k) {
      double s = 0;
      for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 32; ++j) {
          x = cos(kPi * j * (l + 0.5) / 32.0) *
              cos(kPi * i * (k + 0.5) / 32.0) * input[i * 32 + j] / 1024;
          if (i != 0)
            x *= sqrt(2.0);
          if (j != 0)
            x *= sqrt(2.0);
          s += x;
        }
      }
      output[k * 32 + l] = s / 4;
    }
  }
}

static void reference_32x32_dct_1d(double in[32], double out[32], int stride) {
  const double kInvSqrt2 = 0.707106781186547524400844362104;
  for (int k = 0; k < 32; k++) {
    out[k] = 0.0;
    for (int n = 0; n < 32; n++)
      out[k] += in[n] * cos(kPi * (2 * n + 1) * k / 64.0);
    if (k == 0)
      out[k] = out[k] * kInvSqrt2;
  }
}

static void reference_32x32_dct_2d(int16_t input[32*32], double output[32*32]) {
  // First transform columns
  for (int i = 0; i < 32; ++i) {
    double temp_in[32], temp_out[32];
    for (int j = 0; j < 32; ++j)
      temp_in[j] = input[j*32 + i];
    reference_32x32_dct_1d(temp_in, temp_out, 1);
    for (int j = 0; j < 32; ++j)
      output[j * 32 + i] = temp_out[j];
  }
  // Then transform rows
  for (int i = 0; i < 32; ++i) {
    double temp_in[32], temp_out[32];
    for (int j = 0; j < 32; ++j)
      temp_in[j] = output[j + i*32];
    reference_32x32_dct_1d(temp_in, temp_out, 1);
    // Scale by some magic number
    for (int j = 0; j < 32; ++j)
      output[j + i * 32] = temp_out[j] / 4;
  }
}


TEST(VP9Idct32x32Test, AccuracyCheck) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  const int count_test_block = 1000;
  for (int i = 0; i < count_test_block; ++i) {
    int16_t in[1024], coeff[1024];
    int16_t out_c[1024];
    double out_r[1024];

    // Initialize a test block with input range [-255, 255].
    for (int j = 0; j < 1024; ++j)
      in[j] = rnd.Rand8() - rnd.Rand8();

    reference_32x32_dct_2d(in, out_r);
    for (int j = 0; j < 1024; j++)
      coeff[j] = round(out_r[j]);
    vp9_short_idct32x32_c(coeff, out_c, 64);
    for (int j = 0; j < 1024; ++j) {
      const int diff = out_c[j] - in[j];
      const int error = diff * diff;
      EXPECT_GE(1, error)
          << "Error: 3x32 IDCT has error " << error
          << " at index " << j;
    }

    vp9_short_fdct32x32_c(in, out_c, 64);
    for (int j = 0; j < 1024; ++j) {
      const double diff = coeff[j] - out_c[j];
      const double error = diff * diff;
      EXPECT_GE(1.0, error)
          << "Error: 32x32 FDCT has error " << error
          << " at index " << j;
    }
  }
}
#else  // CONFIG_DWTDCTHYBRID
  // TODO(rbultje/debargha): add DWT-specific tests
#endif  // CONFIG_DWTDCTHYBRID
TEST(VP9Fdct32x32Test, AccuracyCheck) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  unsigned int max_error = 0;
  int64_t total_error = 0;
  const int count_test_block = 1000;
  for (int i = 0; i < count_test_block; ++i) {
    int16_t test_input_block[1024];
    int16_t test_temp_block[1024];
    int16_t test_output_block[1024];

    // Initialize a test block with input range [-255, 255].
    for (int j = 0; j < 1024; ++j)
      test_input_block[j] = rnd.Rand8() - rnd.Rand8();

    const int pitch = 64;
    vp9_short_fdct32x32_c(test_input_block, test_temp_block, pitch);
    vp9_short_idct32x32_c(test_temp_block, test_output_block, pitch);

    for (int j = 0; j < 1024; ++j) {
      const unsigned diff = test_input_block[j] - test_output_block[j];
      const unsigned error = diff * diff;
      if (max_error < error)
        max_error = error;
      total_error += error;
    }
  }

  EXPECT_GE(1u, max_error)
      << "Error: 32x32 FDCT/IDCT has an individual roundtrip error > 1";

  EXPECT_GE(count_test_block/10, total_error)
      << "Error: 32x32 FDCT/IDCT has average roundtrip error > 1/10 per block";
}

TEST(VP9Fdct32x32Test, CoeffSizeCheck) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  const int count_test_block = 1000;
  for (int i = 0; i < count_test_block; ++i) {
    int16_t input_block[1024], input_extreme_block[1024];
    int16_t output_block[1024], output_extreme_block[1024];

    // Initialize a test block with input range [-255, 255].
    for (int j = 0; j < 1024; ++j) {
      input_block[j] = rnd.Rand8() - rnd.Rand8();
      input_extreme_block[j] = rnd.Rand8() % 2 ? 255 : -255;
    }
    if (i == 0)
      for (int j = 0; j < 1024; ++j)
        input_extreme_block[j] = 255;

    const int pitch = 64;
    vp9_short_fdct32x32_c(input_block, output_block, pitch);
    vp9_short_fdct32x32_c(input_extreme_block, output_extreme_block, pitch);

    // The minimum quant value is 4.
    for (int j = 0; j < 1024; ++j) {
      EXPECT_GE(4*DCT_MAX_VALUE, abs(output_block[j]))
          << "Error: 32x32 FDCT has coefficient larger than 4*DCT_MAX_VALUE";
      EXPECT_GE(4*DCT_MAX_VALUE, abs(output_extreme_block[j]))
          << "Error: 32x32 FDCT extreme has coefficient larger than "
             "4*DCT_MAX_VALUE";
    }
  }
}
}  // namespace
