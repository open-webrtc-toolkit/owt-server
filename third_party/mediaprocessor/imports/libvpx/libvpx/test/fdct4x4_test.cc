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
#include "vp9_rtcd.h"
}

#include "acm_random.h"
#include "vpx/vpx_integer.h"

using libvpx_test::ACMRandom;

namespace {

TEST(Vp9Fdct4x4Test, SignBiasCheck) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  int16_t test_input_block[16];
  int16_t test_output_block[16];
  const int pitch = 8;
  int count_sign_block[16][2];
  const int count_test_block = 1000000;

  memset(count_sign_block, 0, sizeof(count_sign_block));

  for (int i = 0; i < count_test_block; ++i) {
    // Initialize a test block with input range [-255, 255].
    for (int j = 0; j < 16; ++j)
      test_input_block[j] = rnd.Rand8() - rnd.Rand8();

    // TODO(Yaowu): this should be converted to a parameterized test
    // to test optimized versions of this function.
    vp9_short_fdct4x4_c(test_input_block, test_output_block, pitch);

    for (int j = 0; j < 16; ++j) {
      if (test_output_block[j] < 0)
        ++count_sign_block[j][0];
      else if (test_output_block[j] > 0)
        ++count_sign_block[j][1];
    }
  }

  for (int j = 0; j < 16; ++j) {
    const bool bias_acceptable = (abs(count_sign_block[j][0] -
                                      count_sign_block[j][1]) < 10000);
    EXPECT_TRUE(bias_acceptable)
        << "Error: 4x4 FDCT has a sign bias > 1%"
        << " for input range [-255, 255] at index " << j;
  }

  memset(count_sign_block, 0, sizeof(count_sign_block));

  for (int i = 0; i < count_test_block; ++i) {
    // Initialize a test block with input range [-15, 15].
    for (int j = 0; j < 16; ++j)
      test_input_block[j] = (rnd.Rand8() >> 4) - (rnd.Rand8() >> 4);

    // TODO(Yaowu): this should be converted to a parameterized test
    // to test optimized versions of this function.
    vp9_short_fdct4x4_c(test_input_block, test_output_block, pitch);

    for (int j = 0; j < 16; ++j) {
      if (test_output_block[j] < 0)
        ++count_sign_block[j][0];
      else if (test_output_block[j] > 0)
        ++count_sign_block[j][1];
    }
  }

  for (int j = 0; j < 16; ++j) {
    const bool bias_acceptable = (abs(count_sign_block[j][0] -
                                      count_sign_block[j][1]) < 100000);
    EXPECT_TRUE(bias_acceptable)
        << "Error: 4x4 FDCT has a sign bias > 10%"
        << " for input range [-15, 15] at index " << j;
  }
};

TEST(Vp9Fdct4x4Test, RoundTripErrorCheck) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  int max_error = 0;
  double total_error = 0;
  const int count_test_block = 1000000;
  for (int i = 0; i < count_test_block; ++i) {
    int16_t test_input_block[16];
    int16_t test_temp_block[16];
    int16_t test_output_block[16];

    // Initialize a test block with input range [-255, 255].
    for (int j = 0; j < 16; ++j)
      test_input_block[j] = rnd.Rand8() - rnd.Rand8();

    // TODO(Yaowu): this should be converted to a parameterized test
    // to test optimized versions of this function.
    const int pitch = 8;
    vp9_short_fdct4x4_c(test_input_block, test_temp_block, pitch);

    for (int j = 0; j < 16; ++j) {
        if(test_temp_block[j] > 0) {
          test_temp_block[j] += 2;
          test_temp_block[j] /= 4;
          test_temp_block[j] *= 4;
        } else {
          test_temp_block[j] -= 2;
          test_temp_block[j] /= 4;
          test_temp_block[j] *= 4;
        }
    }

    // Because the bitstream is not frozen yet, use the idct in the codebase.
    vp9_short_idct4x4_c(test_temp_block, test_output_block, pitch);

    for (int j = 0; j < 16; ++j) {
      const int diff = test_input_block[j] - test_output_block[j];
      const int error = diff * diff;
      if (max_error < error)
        max_error = error;
      total_error += error;
    }
  }
  EXPECT_GE(1, max_error)
      << "Error: FDCT/IDCT has an individual roundtrip error > 1";

  EXPECT_GE(count_test_block, total_error)
      << "Error: FDCT/IDCT has average roundtrip error > 1 per block";
};

}  // namespace
