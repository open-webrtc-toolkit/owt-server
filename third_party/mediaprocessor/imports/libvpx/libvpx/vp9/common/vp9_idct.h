/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_IDCT_H_
#define VP9_COMMON_VP9_IDCT_H_

#include <assert.h>

#include "./vpx_config.h"
#include "vpx/vpx_integer.h"
#include "vp9/common/vp9_common.h"

// Constants and Macros used by all idct/dct functions
#define DCT_CONST_BITS 14
#define DCT_CONST_ROUNDING  (1 << (DCT_CONST_BITS - 1))

#define pair_set_epi16(a, b) \
  _mm_set1_epi32(((uint16_t)(a)) + (((uint16_t)(b)) << 16))

// Constants are round(16384 * cos(k*Pi/64)) where k = 1 to 31.
// Note: sin(k*Pi/64) = cos((32-k)*Pi/64)
static const int cospi_1_64  = 16364;
static const int cospi_2_64  = 16305;
static const int cospi_3_64  = 16207;
static const int cospi_4_64  = 16069;
static const int cospi_5_64  = 15893;
static const int cospi_6_64  = 15679;
static const int cospi_7_64  = 15426;
static const int cospi_8_64  = 15137;
static const int cospi_9_64  = 14811;
static const int cospi_10_64 = 14449;
static const int cospi_11_64 = 14053;
static const int cospi_12_64 = 13623;
static const int cospi_13_64 = 13160;
static const int cospi_14_64 = 12665;
static const int cospi_15_64 = 12140;
static const int cospi_16_64 = 11585;
static const int cospi_17_64 = 11003;
static const int cospi_18_64 = 10394;
static const int cospi_19_64 = 9760;
static const int cospi_20_64 = 9102;
static const int cospi_21_64 = 8423;
static const int cospi_22_64 = 7723;
static const int cospi_23_64 = 7005;
static const int cospi_24_64 = 6270;
static const int cospi_25_64 = 5520;
static const int cospi_26_64 = 4756;
static const int cospi_27_64 = 3981;
static const int cospi_28_64 = 3196;
static const int cospi_29_64 = 2404;
static const int cospi_30_64 = 1606;
static const int cospi_31_64 = 804;

//  16384 * sqrt(2) * sin(kPi/9) * 2 / 3
static const int sinpi_1_9 = 5283;
static const int sinpi_2_9 = 9929;
static const int sinpi_3_9 = 13377;
static const int sinpi_4_9 = 15212;

static INLINE int dct_const_round_shift(int input) {
  int rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
  assert(INT16_MIN <= rv && rv <= INT16_MAX);
  return rv;
}

static INLINE int dct_32_round(int input) {
  int rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
  assert(-131072 <= rv && rv <= 131071);
  return rv;
}

typedef void (*transform_1d)(int16_t*, int16_t*);

typedef struct {
  transform_1d cols, rows;  // vertical and horizontal
} transform_2d;

#endif  // VP9_COMMON_VP9_IDCT_H_
