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

#ifdef _MSC_VER
static int round(double x) {
  if(x < 0)
    return (int)ceil(x - 0.5);
  else
    return (int)floor(x + 0.5);
}
#endif

void reference_dct_1d(double input[8], double output[8]) {
  const double kPi = 3.141592653589793238462643383279502884;
  const double kInvSqrt2 = 0.707106781186547524400844362104;
  for (int k = 0; k < 8; k++) {
    output[k] = 0.0;
    for (int n = 0; n < 8; n++)
      output[k] += input[n]*cos(kPi*(2*n+1)*k/16.0);
    if (k == 0)
      output[k] = output[k]*kInvSqrt2;
  }
}

void reference_dct_2d(int16_t input[64], double output[64]) {
  // First transform columns
  for (int i = 0; i < 8; ++i) {
    double temp_in[8], temp_out[8];
    for (int j = 0; j < 8; ++j)
      temp_in[j] = input[j*8 + i];
    reference_dct_1d(temp_in, temp_out);
    for (int j = 0; j < 8; ++j)
      output[j*8 + i] = temp_out[j];
  }
  // Then transform rows
  for (int i = 0; i < 8; ++i) {
    double temp_in[8], temp_out[8];
    for (int j = 0; j < 8; ++j)
      temp_in[j] = output[j + i*8];
    reference_dct_1d(temp_in, temp_out);
    for (int j = 0; j < 8; ++j)
      output[j + i*8] = temp_out[j];
  }
  // Scale by some magic number
  for (int i = 0; i < 64; ++i)
    output[i] *= 2;
}

void reference_idct_1d(double input[8], double output[8]) {
  const double kPi = 3.141592653589793238462643383279502884;
  const double kSqrt2 = 1.414213562373095048801688724209698;
  for (int k = 0; k < 8; k++) {
    output[k] = 0.0;
    for (int n = 0; n < 8; n++) {
      output[k] += input[n]*cos(kPi*(2*k+1)*n/16.0);
      if (n == 0)
        output[k] = output[k]/kSqrt2;
    }
  }
}

void reference_idct_2d(double input[64], int16_t output[64]) {
  double out[64], out2[64];
  // First transform rows
  for (int i = 0; i < 8; ++i) {
    double temp_in[8], temp_out[8];
    for (int j = 0; j < 8; ++j)
      temp_in[j] = input[j + i*8];
    reference_idct_1d(temp_in, temp_out);
    for (int j = 0; j < 8; ++j)
      out[j + i*8] = temp_out[j];
  }
  // Then transform columns
  for (int i = 0; i < 8; ++i) {
    double temp_in[8], temp_out[8];
    for (int j = 0; j < 8; ++j)
      temp_in[j] = out[j*8 + i];
    reference_idct_1d(temp_in, temp_out);
    for (int j = 0; j < 8; ++j)
      out2[j*8 + i] = temp_out[j];
  }
  for (int i = 0; i < 64; ++i)
    output[i] = round(out2[i]/32);
}

TEST(VP9Idct8x8Test, AccuracyCheck) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  const int count_test_block = 10000;
  for (int i = 0; i < count_test_block; ++i) {
    int16_t input[64], coeff[64];
    int16_t output_c[64];
    double output_r[64];

    // Initialize a test block with input range [-255, 255].
    for (int j = 0; j < 64; ++j)
      input[j] = rnd.Rand8() - rnd.Rand8();

    const int pitch = 16;
    vp9_short_fdct8x8_c(input, output_c, pitch);
    reference_dct_2d(input, output_r);

    for (int j = 0; j < 64; ++j) {
      const double diff = output_c[j] - output_r[j];
      const double error = diff * diff;
      // An error in a DCT coefficient isn't that bad.
      // We care more about the reconstructed pixels.
      EXPECT_GE(2.0, error)
          << "Error: 8x8 FDCT/IDCT has error " << error
          << " at index " << j;
    }

#if 0
    // Tests that the reference iDCT and fDCT match.
    reference_dct_2d(input, output_r);
    reference_idct_2d(output_r, output_c);
    for (int j = 0; j < 64; ++j) {
      const int diff = output_c[j] -input[j];
      const int error = diff * diff;
      EXPECT_EQ(0, error)
          << "Error: 8x8 FDCT/IDCT has error " << error
          << " at index " << j;
    }
#endif
    reference_dct_2d(input, output_r);
    for (int j = 0; j < 64; ++j)
      coeff[j] = round(output_r[j]);
    vp9_short_idct8x8_c(coeff, output_c, pitch);
    for (int j = 0; j < 64; ++j) {
      const int diff = output_c[j] -input[j];
      const int error = diff * diff;
      EXPECT_GE(1, error)
          << "Error: 8x8 FDCT/IDCT has error " << error
          << " at index " << j;
    }
  }
}

}  // namespace
