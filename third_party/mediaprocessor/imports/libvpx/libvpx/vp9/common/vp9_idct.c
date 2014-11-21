/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>
#include <math.h>

#include "./vpx_config.h"
#include "./vp9_rtcd.h"
#include "vp9/common/vp9_systemdependent.h"
#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_idct.h"

void vp9_short_iwalsh4x4_c(int16_t *input, int16_t *output, int pitch) {
  int i;
  int a1, b1, c1, d1;
  int16_t *ip = input;
  int16_t *op = output;
  const int half_pitch = pitch >> 1;

  for (i = 0; i < 4; i++) {
    a1 = (ip[0] + ip[3]) >> WHT_UPSCALE_FACTOR;
    b1 = (ip[1] + ip[2]) >> WHT_UPSCALE_FACTOR;
    c1 = (ip[1] - ip[2]) >> WHT_UPSCALE_FACTOR;
    d1 = (ip[0] - ip[3]) >> WHT_UPSCALE_FACTOR;

    op[0] = (a1 + b1 + 1) >> 1;
    op[1] = (c1 + d1) >> 1;
    op[2] = (a1 - b1) >> 1;
    op[3] = (d1 - c1) >> 1;

    ip += 4;
    op += half_pitch;
  }

  ip = output;
  op = output;
  for (i = 0; i < 4; i++) {
    a1 = ip[half_pitch * 0] + ip[half_pitch * 3];
    b1 = ip[half_pitch * 1] + ip[half_pitch * 2];
    c1 = ip[half_pitch * 1] - ip[half_pitch * 2];
    d1 = ip[half_pitch * 0] - ip[half_pitch * 3];


    op[half_pitch * 0] = (a1 + b1 + 1) >> 1;
    op[half_pitch * 1] = (c1 + d1) >> 1;
    op[half_pitch * 2] = (a1 - b1) >> 1;
    op[half_pitch * 3] = (d1 - c1) >> 1;

    ip++;
    op++;
  }
}

void vp9_short_iwalsh4x4_1_c(int16_t *in, int16_t *out, int pitch) {
  int i;
  int16_t tmp[4];
  int16_t *ip = in;
  int16_t *op = tmp;
  const int half_pitch = pitch >> 1;

  op[0] = ((ip[0] >> WHT_UPSCALE_FACTOR) + 1) >> 1;
  op[1] = op[2] = op[3] = (ip[0] >> WHT_UPSCALE_FACTOR) >> 1;

  ip = tmp;
  op = out;
  for (i = 0; i < 4; i++) {
    op[half_pitch * 0] = (ip[0] + 1) >> 1;
    op[half_pitch * 1] = op[half_pitch * 2] = op[half_pitch * 3] = ip[0] >> 1;
    ip++;
    op++;
  }
}

void vp9_dc_only_inv_walsh_add_c(int input_dc, uint8_t *pred_ptr,
                                 uint8_t *dst_ptr,
                                 int pitch, int stride) {
  int r, c;
  int16_t dc = input_dc;
  int16_t tmp[4 * 4];
  vp9_short_iwalsh4x4_1_c(&dc, tmp, 4 << 1);

  for (r = 0; r < 4; r++) {
    for (c = 0; c < 4; c++)
      dst_ptr[c] = clip_pixel(tmp[r * 4 + c] + pred_ptr[c]);

    dst_ptr += stride;
    pred_ptr += pitch;
  }
}

void vp9_idct4_1d_c(int16_t *input, int16_t *output) {
  int16_t step[4];
  int temp1, temp2;
  // stage 1
  temp1 = (input[0] + input[2]) * cospi_16_64;
  temp2 = (input[0] - input[2]) * cospi_16_64;
  step[0] = dct_const_round_shift(temp1);
  step[1] = dct_const_round_shift(temp2);
  temp1 = input[1] * cospi_24_64 - input[3] * cospi_8_64;
  temp2 = input[1] * cospi_8_64 + input[3] * cospi_24_64;
  step[2] = dct_const_round_shift(temp1);
  step[3] = dct_const_round_shift(temp2);

  // stage 2
  output[0] = step[0] + step[3];
  output[1] = step[1] + step[2];
  output[2] = step[1] - step[2];
  output[3] = step[0] - step[3];
}

void vp9_short_idct4x4_c(int16_t *input, int16_t *output, int pitch) {
  int16_t out[4 * 4];
  int16_t *outptr = out;
  const int half_pitch = pitch >> 1;
  int i, j;
  int16_t temp_in[4], temp_out[4];

  // Rows
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j)
      temp_in[j] = input[j];
    vp9_idct4_1d(temp_in, outptr);
    input += 4;
    outptr += 4;
  }

  // Columns
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j)
      temp_in[j] = out[j * 4 + i];
    vp9_idct4_1d(temp_in, temp_out);
    for (j = 0; j < 4; ++j)
      output[j * half_pitch + i] = ROUND_POWER_OF_TWO(temp_out[j], 4);
  }
}

void vp9_short_idct4x4_1_c(int16_t *input, int16_t *output, int pitch) {
  int i;
  int a1;
  int16_t *op = output;
  const int half_pitch = pitch >> 1;
  int16_t out = dct_const_round_shift(input[0] * cospi_16_64);
  out = dct_const_round_shift(out * cospi_16_64);
  a1 = ROUND_POWER_OF_TWO(out, 4);

  for (i = 0; i < 4; i++) {
    op[0] = op[1] = op[2] = op[3] = a1;
    op += half_pitch;
  }
}

void vp9_dc_only_idct_add_c(int input_dc, uint8_t *pred_ptr,
                            uint8_t *dst_ptr, int pitch, int stride) {
  int a1;
  int r, c;
  int16_t out = dct_const_round_shift(input_dc * cospi_16_64);
  out = dct_const_round_shift(out * cospi_16_64);
  a1 = ROUND_POWER_OF_TWO(out, 4);

  for (r = 0; r < 4; r++) {
    for (c = 0; c < 4; c++)
      dst_ptr[c] = clip_pixel(a1 + pred_ptr[c]);

    dst_ptr += stride;
    pred_ptr += pitch;
  }
}

static void idct8_1d(int16_t *input, int16_t *output) {
  int16_t step1[8], step2[8];
  int temp1, temp2;
  // stage 1
  step1[0] = input[0];
  step1[2] = input[4];
  step1[1] = input[2];
  step1[3] = input[6];
  temp1 = input[1] * cospi_28_64 - input[7] * cospi_4_64;
  temp2 = input[1] * cospi_4_64 + input[7] * cospi_28_64;
  step1[4] = dct_const_round_shift(temp1);
  step1[7] = dct_const_round_shift(temp2);
  temp1 = input[5] * cospi_12_64 - input[3] * cospi_20_64;
  temp2 = input[5] * cospi_20_64 + input[3] * cospi_12_64;
  step1[5] = dct_const_round_shift(temp1);
  step1[6] = dct_const_round_shift(temp2);

  // stage 2 & stage 3 - even half
  vp9_idct4_1d(step1, step1);

  // stage 2 - odd half
  step2[4] = step1[4] + step1[5];
  step2[5] = step1[4] - step1[5];
  step2[6] = -step1[6] + step1[7];
  step2[7] = step1[6] + step1[7];

  // stage 3 -odd half
  step1[4] = step2[4];
  temp1 = (step2[6] - step2[5]) * cospi_16_64;
  temp2 = (step2[5] + step2[6]) * cospi_16_64;
  step1[5] = dct_const_round_shift(temp1);
  step1[6] = dct_const_round_shift(temp2);
  step1[7] = step2[7];

  // stage 4
  output[0] = step1[0] + step1[7];
  output[1] = step1[1] + step1[6];
  output[2] = step1[2] + step1[5];
  output[3] = step1[3] + step1[4];
  output[4] = step1[3] - step1[4];
  output[5] = step1[2] - step1[5];
  output[6] = step1[1] - step1[6];
  output[7] = step1[0] - step1[7];
}

void vp9_short_idct8x8_c(int16_t *input, int16_t *output, int pitch) {
  int16_t out[8 * 8];
  int16_t *outptr = out;
  const int half_pitch = pitch >> 1;
  int i, j;
  int16_t temp_in[8], temp_out[8];

  // Rows
  for (i = 0; i < 8; ++i) {
    idct8_1d(input, outptr);
    input += 8;
    outptr += 8;
  }

  // Columns
  for (i = 0; i < 8; ++i) {
    for (j = 0; j < 8; ++j)
      temp_in[j] = out[j * 8 + i];
    idct8_1d(temp_in, temp_out);
    for (j = 0; j < 8; ++j)
      output[j * half_pitch + i] = ROUND_POWER_OF_TWO(temp_out[j], 5);
  }
}

static void iadst4_1d(int16_t *input, int16_t *output) {
  int s0, s1, s2, s3, s4, s5, s6, s7;

  int x0 = input[0];
  int x1 = input[1];
  int x2 = input[2];
  int x3 = input[3];

  if (!(x0 | x1 | x2 | x3)) {
    output[0] = output[1] = output[2] = output[3] = 0;
    return;
  }

  s0 = sinpi_1_9 * x0;
  s1 = sinpi_2_9 * x0;
  s2 = sinpi_3_9 * x1;
  s3 = sinpi_4_9 * x2;
  s4 = sinpi_1_9 * x2;
  s5 = sinpi_2_9 * x3;
  s6 = sinpi_4_9 * x3;
  s7 = x0 - x2 + x3;

  x0 = s0 + s3 + s5;
  x1 = s1 - s4 - s6;
  x2 = sinpi_3_9 * s7;
  x3 = s2;

  s0 = x0 + x3;
  s1 = x1 + x3;
  s2 = x2;
  s3 = x0 + x1 - x3;

  // 1-D transform scaling factor is sqrt(2).
  // The overall dynamic range is 14b (input) + 14b (multiplication scaling)
  // + 1b (addition) = 29b.
  // Hence the output bit depth is 15b.
  output[0] = dct_const_round_shift(s0);
  output[1] = dct_const_round_shift(s1);
  output[2] = dct_const_round_shift(s2);
  output[3] = dct_const_round_shift(s3);
}

void vp9_short_iht4x4_c(int16_t *input, int16_t *output,
                        int pitch, int tx_type) {
  const transform_2d IHT_4[] = {
    { vp9_idct4_1d,  vp9_idct4_1d  },  // DCT_DCT  = 0
    { iadst4_1d, vp9_idct4_1d  },      // ADST_DCT = 1
    { vp9_idct4_1d,  iadst4_1d },      // DCT_ADST = 2
    { iadst4_1d, iadst4_1d }           // ADST_ADST = 3
  };

  int i, j;
  int16_t out[4 * 4];
  int16_t *outptr = out;
  int16_t temp_in[4], temp_out[4];

  // inverse transform row vectors
  for (i = 0; i < 4; ++i) {
    IHT_4[tx_type].rows(input, outptr);
    input  += 4;
    outptr += 4;
  }

  // inverse transform column vectors
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j)
      temp_in[j] = out[j * 4 + i];
    IHT_4[tx_type].cols(temp_in, temp_out);
    for (j = 0; j < 4; ++j)
      output[j * pitch + i] = ROUND_POWER_OF_TWO(temp_out[j], 4);
  }
}

static void iadst8_1d(int16_t *input, int16_t *output) {
  int s0, s1, s2, s3, s4, s5, s6, s7;

  int x0 = input[7];
  int x1 = input[0];
  int x2 = input[5];
  int x3 = input[2];
  int x4 = input[3];
  int x5 = input[4];
  int x6 = input[1];
  int x7 = input[6];

  if (!(x0 | x1 | x2 | x3 | x4 | x5 | x6 | x7)) {
    output[0] = output[1] = output[2] = output[3] = output[4]
              = output[5] = output[6] = output[7] = 0;
    return;
  }

  // stage 1
  s0 = cospi_2_64  * x0 + cospi_30_64 * x1;
  s1 = cospi_30_64 * x0 - cospi_2_64  * x1;
  s2 = cospi_10_64 * x2 + cospi_22_64 * x3;
  s3 = cospi_22_64 * x2 - cospi_10_64 * x3;
  s4 = cospi_18_64 * x4 + cospi_14_64 * x5;
  s5 = cospi_14_64 * x4 - cospi_18_64 * x5;
  s6 = cospi_26_64 * x6 + cospi_6_64  * x7;
  s7 = cospi_6_64  * x6 - cospi_26_64 * x7;

  x0 = dct_const_round_shift(s0 + s4);
  x1 = dct_const_round_shift(s1 + s5);
  x2 = dct_const_round_shift(s2 + s6);
  x3 = dct_const_round_shift(s3 + s7);
  x4 = dct_const_round_shift(s0 - s4);
  x5 = dct_const_round_shift(s1 - s5);
  x6 = dct_const_round_shift(s2 - s6);
  x7 = dct_const_round_shift(s3 - s7);

  // stage 2
  s0 = x0;
  s1 = x1;
  s2 = x2;
  s3 = x3;
  s4 =  cospi_8_64  * x4 + cospi_24_64 * x5;
  s5 =  cospi_24_64 * x4 - cospi_8_64  * x5;
  s6 = -cospi_24_64 * x6 + cospi_8_64  * x7;
  s7 =  cospi_8_64  * x6 + cospi_24_64 * x7;

  x0 = s0 + s2;
  x1 = s1 + s3;
  x2 = s0 - s2;
  x3 = s1 - s3;
  x4 = dct_const_round_shift(s4 + s6);
  x5 = dct_const_round_shift(s5 + s7);
  x6 = dct_const_round_shift(s4 - s6);
  x7 = dct_const_round_shift(s5 - s7);

  // stage 3
  s2 = cospi_16_64 * (x2 + x3);
  s3 = cospi_16_64 * (x2 - x3);
  s6 = cospi_16_64 * (x6 + x7);
  s7 = cospi_16_64 * (x6 - x7);

  x2 = dct_const_round_shift(s2);
  x3 = dct_const_round_shift(s3);
  x6 = dct_const_round_shift(s6);
  x7 = dct_const_round_shift(s7);

  output[0] =  x0;
  output[1] = -x4;
  output[2] =  x6;
  output[3] = -x2;
  output[4] =  x3;
  output[5] = -x7;
  output[6] =  x5;
  output[7] = -x1;
}

static const transform_2d IHT_8[] = {
  { idct8_1d,  idct8_1d  },  // DCT_DCT  = 0
  { iadst8_1d, idct8_1d  },  // ADST_DCT = 1
  { idct8_1d,  iadst8_1d },  // DCT_ADST = 2
  { iadst8_1d, iadst8_1d }   // ADST_ADST = 3
};

void vp9_short_iht8x8_c(int16_t *input, int16_t *output,
                        int pitch, int tx_type) {
  int i, j;
  int16_t out[8 * 8];
  int16_t *outptr = out;
  int16_t temp_in[8], temp_out[8];
  const transform_2d ht = IHT_8[tx_type];

  // inverse transform row vectors
  for (i = 0; i < 8; ++i) {
    ht.rows(input, outptr);
    input += 8;
    outptr += 8;
  }

  // inverse transform column vectors
  for (i = 0; i < 8; ++i) {
    for (j = 0; j < 8; ++j)
      temp_in[j] = out[j * 8 + i];
    ht.cols(temp_in, temp_out);
    for (j = 0; j < 8; ++j)
      output[j * pitch + i] = ROUND_POWER_OF_TWO(temp_out[j], 5);
  }
}

void vp9_short_idct10_8x8_c(int16_t *input, int16_t *output, int pitch) {
  int16_t out[8 * 8];
  int16_t *outptr = out;
  const int half_pitch = pitch >> 1;
  int i, j;
  int16_t temp_in[8], temp_out[8];

  vpx_memset(out, 0, sizeof(out));
  // First transform rows
  // only first 4 row has non-zero coefs
  for (i = 0; i < 4; ++i) {
    idct8_1d(input, outptr);
    input += 8;
    outptr += 8;
  }

  // Then transform columns
  for (i = 0; i < 8; ++i) {
    for (j = 0; j < 8; ++j)
      temp_in[j] = out[j * 8 + i];
    idct8_1d(temp_in, temp_out);
    for (j = 0; j < 8; ++j)
      output[j * half_pitch + i] = ROUND_POWER_OF_TWO(temp_out[j], 5);
  }
}

void vp9_short_idct1_8x8_c(int16_t *input, int16_t *output) {
  int16_t out = dct_const_round_shift(input[0] * cospi_16_64);
  out = dct_const_round_shift(out * cospi_16_64);
  output[0] = ROUND_POWER_OF_TWO(out, 5);
}

static void idct16_1d(int16_t *input, int16_t *output) {
  int16_t step1[16], step2[16];
  int temp1, temp2;

  // stage 1
  step1[0] = input[0/2];
  step1[1] = input[16/2];
  step1[2] = input[8/2];
  step1[3] = input[24/2];
  step1[4] = input[4/2];
  step1[5] = input[20/2];
  step1[6] = input[12/2];
  step1[7] = input[28/2];
  step1[8] = input[2/2];
  step1[9] = input[18/2];
  step1[10] = input[10/2];
  step1[11] = input[26/2];
  step1[12] = input[6/2];
  step1[13] = input[22/2];
  step1[14] = input[14/2];
  step1[15] = input[30/2];

  // stage 2
  step2[0] = step1[0];
  step2[1] = step1[1];
  step2[2] = step1[2];
  step2[3] = step1[3];
  step2[4] = step1[4];
  step2[5] = step1[5];
  step2[6] = step1[6];
  step2[7] = step1[7];

  temp1 = step1[8] * cospi_30_64 - step1[15] * cospi_2_64;
  temp2 = step1[8] * cospi_2_64 + step1[15] * cospi_30_64;
  step2[8] = dct_const_round_shift(temp1);
  step2[15] = dct_const_round_shift(temp2);

  temp1 = step1[9] * cospi_14_64 - step1[14] * cospi_18_64;
  temp2 = step1[9] * cospi_18_64 + step1[14] * cospi_14_64;
  step2[9] = dct_const_round_shift(temp1);
  step2[14] = dct_const_round_shift(temp2);

  temp1 = step1[10] * cospi_22_64 - step1[13] * cospi_10_64;
  temp2 = step1[10] * cospi_10_64 + step1[13] * cospi_22_64;
  step2[10] = dct_const_round_shift(temp1);
  step2[13] = dct_const_round_shift(temp2);

  temp1 = step1[11] * cospi_6_64 - step1[12] * cospi_26_64;
  temp2 = step1[11] * cospi_26_64 + step1[12] * cospi_6_64;
  step2[11] = dct_const_round_shift(temp1);
  step2[12] = dct_const_round_shift(temp2);

  // stage 3
  step1[0] = step2[0];
  step1[1] = step2[1];
  step1[2] = step2[2];
  step1[3] = step2[3];

  temp1 = step2[4] * cospi_28_64 - step2[7] * cospi_4_64;
  temp2 = step2[4] * cospi_4_64 + step2[7] * cospi_28_64;
  step1[4] = dct_const_round_shift(temp1);
  step1[7] = dct_const_round_shift(temp2);
  temp1 = step2[5] * cospi_12_64 - step2[6] * cospi_20_64;
  temp2 = step2[5] * cospi_20_64 + step2[6] * cospi_12_64;
  step1[5] = dct_const_round_shift(temp1);
  step1[6] = dct_const_round_shift(temp2);

  step1[8] = step2[8] + step2[9];
  step1[9] = step2[8] - step2[9];
  step1[10] = -step2[10] + step2[11];
  step1[11] = step2[10] + step2[11];
  step1[12] = step2[12] + step2[13];
  step1[13] = step2[12] - step2[13];
  step1[14] = -step2[14] + step2[15];
  step1[15] = step2[14] + step2[15];

  temp1 = (step1[0] + step1[1]) * cospi_16_64;
  temp2 = (step1[0] - step1[1]) * cospi_16_64;
  step2[0] = dct_const_round_shift(temp1);
  step2[1] = dct_const_round_shift(temp2);
  temp1 = step1[2] * cospi_24_64 - step1[3] * cospi_8_64;
  temp2 = step1[2] * cospi_8_64 + step1[3] * cospi_24_64;
  step2[2] = dct_const_round_shift(temp1);
  step2[3] = dct_const_round_shift(temp2);
  step2[4] = step1[4] + step1[5];
  step2[5] = step1[4] - step1[5];
  step2[6] = -step1[6] + step1[7];
  step2[7] = step1[6] + step1[7];

  step2[8] = step1[8];
  step2[15] = step1[15];
  temp1 = -step1[9] * cospi_8_64 + step1[14] * cospi_24_64;
  temp2 = step1[9] * cospi_24_64 + step1[14] * cospi_8_64;
  step2[9] = dct_const_round_shift(temp1);
  step2[14] = dct_const_round_shift(temp2);
  temp1 = -step1[10] * cospi_24_64 - step1[13] * cospi_8_64;
  temp2 = -step1[10] * cospi_8_64 + step1[13] * cospi_24_64;
  step2[10] = dct_const_round_shift(temp1);
  step2[13] = dct_const_round_shift(temp2);
  step2[11] = step1[11];
  step2[12] = step1[12];

  // stage 5
  step1[0] = step2[0] + step2[3];
  step1[1] = step2[1] + step2[2];
  step1[2] = step2[1] - step2[2];
  step1[3] = step2[0] - step2[3];
  step1[4] = step2[4];
  temp1 = (step2[6] - step2[5]) * cospi_16_64;
  temp2 = (step2[5] + step2[6]) * cospi_16_64;
  step1[5] = dct_const_round_shift(temp1);
  step1[6] = dct_const_round_shift(temp2);
  step1[7] = step2[7];

  step1[8] = step2[8] + step2[11];
  step1[9] = step2[9] + step2[10];
  step1[10] = step2[9] - step2[10];
  step1[11] = step2[8] - step2[11];
  step1[12] = -step2[12] + step2[15];
  step1[13] = -step2[13] + step2[14];
  step1[14] = step2[13] + step2[14];
  step1[15] = step2[12] + step2[15];

  // stage 6
  step2[0] = step1[0] + step1[7];
  step2[1] = step1[1] + step1[6];
  step2[2] = step1[2] + step1[5];
  step2[3] = step1[3] + step1[4];
  step2[4] = step1[3] - step1[4];
  step2[5] = step1[2] - step1[5];
  step2[6] = step1[1] - step1[6];
  step2[7] = step1[0] - step1[7];
  step2[8] = step1[8];
  step2[9] = step1[9];
  temp1 = (-step1[10] + step1[13]) * cospi_16_64;
  temp2 = (step1[10] + step1[13]) * cospi_16_64;
  step2[10] = dct_const_round_shift(temp1);
  step2[13] = dct_const_round_shift(temp2);
  temp1 = (-step1[11] + step1[12]) * cospi_16_64;
  temp2 = (step1[11] + step1[12]) * cospi_16_64;
  step2[11] = dct_const_round_shift(temp1);
  step2[12] = dct_const_round_shift(temp2);
  step2[14] = step1[14];
  step2[15] = step1[15];

  // stage 7
  output[0] = step2[0] + step2[15];
  output[1] = step2[1] + step2[14];
  output[2] = step2[2] + step2[13];
  output[3] = step2[3] + step2[12];
  output[4] = step2[4] + step2[11];
  output[5] = step2[5] + step2[10];
  output[6] = step2[6] + step2[9];
  output[7] = step2[7] + step2[8];
  output[8] = step2[7] - step2[8];
  output[9] = step2[6] - step2[9];
  output[10] = step2[5] - step2[10];
  output[11] = step2[4] - step2[11];
  output[12] = step2[3] - step2[12];
  output[13] = step2[2] - step2[13];
  output[14] = step2[1] - step2[14];
  output[15] = step2[0] - step2[15];
}

void vp9_short_idct16x16_c(int16_t *input, int16_t *output, int pitch) {
  int16_t out[16 * 16];
  int16_t *outptr = out;
  const int half_pitch = pitch >> 1;
  int i, j;
  int16_t temp_in[16], temp_out[16];

  // First transform rows
  for (i = 0; i < 16; ++i) {
    idct16_1d(input, outptr);
    input += 16;
    outptr += 16;
  }

  // Then transform columns
  for (i = 0; i < 16; ++i) {
    for (j = 0; j < 16; ++j)
      temp_in[j] = out[j * 16 + i];
    idct16_1d(temp_in, temp_out);
    for (j = 0; j < 16; ++j)
      output[j * half_pitch + i] = ROUND_POWER_OF_TWO(temp_out[j], 6);
  }
}

void iadst16_1d(int16_t *input, int16_t *output) {
  int s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15;

  int x0 = input[15];
  int x1 = input[0];
  int x2 = input[13];
  int x3 = input[2];
  int x4 = input[11];
  int x5 = input[4];
  int x6 = input[9];
  int x7 = input[6];
  int x8 = input[7];
  int x9 = input[8];
  int x10 = input[5];
  int x11 = input[10];
  int x12 = input[3];
  int x13 = input[12];
  int x14 = input[1];
  int x15 = input[14];

  if (!(x0 | x1 | x2 | x3 | x4 | x5 | x6 | x7 | x8
           | x9 | x10 | x11 | x12 | x13 | x14 | x15)) {
    output[0] = output[1] = output[2] = output[3] = output[4]
              = output[5] = output[6] = output[7] = output[8]
              = output[9] = output[10] = output[11] = output[12]
              = output[13] = output[14] = output[15] = 0;
    return;
  }

  // stage 1
  s0 = x0 * cospi_1_64  + x1 * cospi_31_64;
  s1 = x0 * cospi_31_64 - x1 * cospi_1_64;
  s2 = x2 * cospi_5_64  + x3 * cospi_27_64;
  s3 = x2 * cospi_27_64 - x3 * cospi_5_64;
  s4 = x4 * cospi_9_64  + x5 * cospi_23_64;
  s5 = x4 * cospi_23_64 - x5 * cospi_9_64;
  s6 = x6 * cospi_13_64 + x7 * cospi_19_64;
  s7 = x6 * cospi_19_64 - x7 * cospi_13_64;
  s8 = x8 * cospi_17_64 + x9 * cospi_15_64;
  s9 = x8 * cospi_15_64 - x9 * cospi_17_64;
  s10 = x10 * cospi_21_64 + x11 * cospi_11_64;
  s11 = x10 * cospi_11_64 - x11 * cospi_21_64;
  s12 = x12 * cospi_25_64 + x13 * cospi_7_64;
  s13 = x12 * cospi_7_64  - x13 * cospi_25_64;
  s14 = x14 * cospi_29_64 + x15 * cospi_3_64;
  s15 = x14 * cospi_3_64  - x15 * cospi_29_64;

  x0 = dct_const_round_shift(s0 + s8);
  x1 = dct_const_round_shift(s1 + s9);
  x2 = dct_const_round_shift(s2 + s10);
  x3 = dct_const_round_shift(s3 + s11);
  x4 = dct_const_round_shift(s4 + s12);
  x5 = dct_const_round_shift(s5 + s13);
  x6 = dct_const_round_shift(s6 + s14);
  x7 = dct_const_round_shift(s7 + s15);
  x8  = dct_const_round_shift(s0 - s8);
  x9  = dct_const_round_shift(s1 - s9);
  x10 = dct_const_round_shift(s2 - s10);
  x11 = dct_const_round_shift(s3 - s11);
  x12 = dct_const_round_shift(s4 - s12);
  x13 = dct_const_round_shift(s5 - s13);
  x14 = dct_const_round_shift(s6 - s14);
  x15 = dct_const_round_shift(s7 - s15);

  // stage 2
  s0 = x0;
  s1 = x1;
  s2 = x2;
  s3 = x3;
  s4 = x4;
  s5 = x5;
  s6 = x6;
  s7 = x7;
  s8 =    x8 * cospi_4_64   + x9 * cospi_28_64;
  s9 =    x8 * cospi_28_64  - x9 * cospi_4_64;
  s10 =   x10 * cospi_20_64 + x11 * cospi_12_64;
  s11 =   x10 * cospi_12_64 - x11 * cospi_20_64;
  s12 = - x12 * cospi_28_64 + x13 * cospi_4_64;
  s13 =   x12 * cospi_4_64  + x13 * cospi_28_64;
  s14 = - x14 * cospi_12_64 + x15 * cospi_20_64;
  s15 =   x14 * cospi_20_64 + x15 * cospi_12_64;

  x0 = s0 + s4;
  x1 = s1 + s5;
  x2 = s2 + s6;
  x3 = s3 + s7;
  x4 = s0 - s4;
  x5 = s1 - s5;
  x6 = s2 - s6;
  x7 = s3 - s7;
  x8 = dct_const_round_shift(s8 + s12);
  x9 = dct_const_round_shift(s9 + s13);
  x10 = dct_const_round_shift(s10 + s14);
  x11 = dct_const_round_shift(s11 + s15);
  x12 = dct_const_round_shift(s8 - s12);
  x13 = dct_const_round_shift(s9 - s13);
  x14 = dct_const_round_shift(s10 - s14);
  x15 = dct_const_round_shift(s11 - s15);

  // stage 3
  s0 = x0;
  s1 = x1;
  s2 = x2;
  s3 = x3;
  s4 = x4 * cospi_8_64  + x5 * cospi_24_64;
  s5 = x4 * cospi_24_64 - x5 * cospi_8_64;
  s6 = - x6 * cospi_24_64 + x7 * cospi_8_64;
  s7 =   x6 * cospi_8_64  + x7 * cospi_24_64;
  s8 = x8;
  s9 = x9;
  s10 = x10;
  s11 = x11;
  s12 = x12 * cospi_8_64  + x13 * cospi_24_64;
  s13 = x12 * cospi_24_64 - x13 * cospi_8_64;
  s14 = - x14 * cospi_24_64 + x15 * cospi_8_64;
  s15 =   x14 * cospi_8_64  + x15 * cospi_24_64;

  x0 = s0 + s2;
  x1 = s1 + s3;
  x2 = s0 - s2;
  x3 = s1 - s3;
  x4 = dct_const_round_shift(s4 + s6);
  x5 = dct_const_round_shift(s5 + s7);
  x6 = dct_const_round_shift(s4 - s6);
  x7 = dct_const_round_shift(s5 - s7);
  x8 = s8 + s10;
  x9 = s9 + s11;
  x10 = s8 - s10;
  x11 = s9 - s11;
  x12 = dct_const_round_shift(s12 + s14);
  x13 = dct_const_round_shift(s13 + s15);
  x14 = dct_const_round_shift(s12 - s14);
  x15 = dct_const_round_shift(s13 - s15);

  // stage 4
  s2 = (- cospi_16_64) * (x2 + x3);
  s3 = cospi_16_64 * (x2 - x3);
  s6 = cospi_16_64 * (x6 + x7);
  s7 = cospi_16_64 * (- x6 + x7);
  s10 = cospi_16_64 * (x10 + x11);
  s11 = cospi_16_64 * (- x10 + x11);
  s14 = (- cospi_16_64) * (x14 + x15);
  s15 = cospi_16_64 * (x14 - x15);

  x2 = dct_const_round_shift(s2);
  x3 = dct_const_round_shift(s3);
  x6 = dct_const_round_shift(s6);
  x7 = dct_const_round_shift(s7);
  x10 = dct_const_round_shift(s10);
  x11 = dct_const_round_shift(s11);
  x14 = dct_const_round_shift(s14);
  x15 = dct_const_round_shift(s15);

  output[0] =  x0;
  output[1] = -x8;
  output[2] =  x12;
  output[3] = -x4;
  output[4] =  x6;
  output[5] =  x14;
  output[6] =  x10;
  output[7] =  x2;
  output[8] =  x3;
  output[9] =  x11;
  output[10] =  x15;
  output[11] =  x7;
  output[12] =  x5;
  output[13] = -x13;
  output[14] =  x9;
  output[15] = -x1;
}

static const transform_2d IHT_16[] = {
  { idct16_1d,  idct16_1d  },  // DCT_DCT  = 0
  { iadst16_1d, idct16_1d  },  // ADST_DCT = 1
  { idct16_1d,  iadst16_1d },  // DCT_ADST = 2
  { iadst16_1d, iadst16_1d }   // ADST_ADST = 3
};

void vp9_short_iht16x16_c(int16_t *input, int16_t *output,
                          int pitch, int tx_type) {
  int i, j;
  int16_t out[16 * 16];
  int16_t *outptr = out;
  int16_t temp_in[16], temp_out[16];
  const transform_2d ht = IHT_16[tx_type];

  // Rows
  for (i = 0; i < 16; ++i) {
    ht.rows(input, outptr);
    input += 16;
    outptr += 16;
  }

  // Columns
  for (i = 0; i < 16; ++i) {
    for (j = 0; j < 16; ++j)
      temp_in[j] = out[j * 16 + i];
    ht.cols(temp_in, temp_out);
    for (j = 0; j < 16; ++j)
      output[j * pitch + i] = ROUND_POWER_OF_TWO(temp_out[j], 6);
  }
}

void vp9_short_idct10_16x16_c(int16_t *input, int16_t *output, int pitch) {
    int16_t out[16 * 16];
    int16_t *outptr = out;
    const int half_pitch = pitch >> 1;
    int i, j;
    int16_t temp_in[16], temp_out[16];

    /* First transform rows. Since all non-zero dct coefficients are in
     * upper-left 4x4 area, we only need to calculate first 4 rows here.
     */
    vpx_memset(out, 0, sizeof(out));
    for (i = 0; i < 4; ++i) {
      idct16_1d(input, outptr);
      input += 16;
      outptr += 16;
    }

    // Then transform columns
    for (i = 0; i < 16; ++i) {
      for (j = 0; j < 16; ++j)
        temp_in[j] = out[j*16 + i];
      idct16_1d(temp_in, temp_out);
      for (j = 0; j < 16; ++j)
        output[j * half_pitch + i] = ROUND_POWER_OF_TWO(temp_out[j], 6);
    }
}


void vp9_short_idct1_16x16_c(int16_t *input, int16_t *output) {
  int16_t out = dct_const_round_shift(input[0] * cospi_16_64);
  out = dct_const_round_shift(out * cospi_16_64);
  output[0] = ROUND_POWER_OF_TWO(out, 6);
}

static void idct32_1d(int16_t *input, int16_t *output) {
  int16_t step1[32], step2[32];
  int temp1, temp2;

  // stage 1
  step1[0] = input[0];
  step1[1] = input[16];
  step1[2] = input[8];
  step1[3] = input[24];
  step1[4] = input[4];
  step1[5] = input[20];
  step1[6] = input[12];
  step1[7] = input[28];
  step1[8] = input[2];
  step1[9] = input[18];
  step1[10] = input[10];
  step1[11] = input[26];
  step1[12] = input[6];
  step1[13] = input[22];
  step1[14] = input[14];
  step1[15] = input[30];

  temp1 = input[1] * cospi_31_64 - input[31] * cospi_1_64;
  temp2 = input[1] * cospi_1_64 + input[31] * cospi_31_64;
  step1[16] = dct_const_round_shift(temp1);
  step1[31] = dct_const_round_shift(temp2);

  temp1 = input[17] * cospi_15_64 - input[15] * cospi_17_64;
  temp2 = input[17] * cospi_17_64 + input[15] * cospi_15_64;
  step1[17] = dct_const_round_shift(temp1);
  step1[30] = dct_const_round_shift(temp2);

  temp1 = input[9] * cospi_23_64 - input[23] * cospi_9_64;
  temp2 = input[9] * cospi_9_64 + input[23] * cospi_23_64;
  step1[18] = dct_const_round_shift(temp1);
  step1[29] = dct_const_round_shift(temp2);

  temp1 = input[25] * cospi_7_64 - input[7] * cospi_25_64;
  temp2 = input[25] * cospi_25_64 + input[7] * cospi_7_64;
  step1[19] = dct_const_round_shift(temp1);
  step1[28] = dct_const_round_shift(temp2);

  temp1 = input[5] * cospi_27_64 - input[27] * cospi_5_64;
  temp2 = input[5] * cospi_5_64 + input[27] * cospi_27_64;
  step1[20] = dct_const_round_shift(temp1);
  step1[27] = dct_const_round_shift(temp2);

  temp1 = input[21] * cospi_11_64 - input[11] * cospi_21_64;
  temp2 = input[21] * cospi_21_64 + input[11] * cospi_11_64;
  step1[21] = dct_const_round_shift(temp1);
  step1[26] = dct_const_round_shift(temp2);

  temp1 = input[13] * cospi_19_64 - input[19] * cospi_13_64;
  temp2 = input[13] * cospi_13_64 + input[19] * cospi_19_64;
  step1[22] = dct_const_round_shift(temp1);
  step1[25] = dct_const_round_shift(temp2);

  temp1 = input[29] * cospi_3_64 - input[3] * cospi_29_64;
  temp2 = input[29] * cospi_29_64 + input[3] * cospi_3_64;
  step1[23] = dct_const_round_shift(temp1);
  step1[24] = dct_const_round_shift(temp2);

  // stage 2
  step2[0] = step1[0];
  step2[1] = step1[1];
  step2[2] = step1[2];
  step2[3] = step1[3];
  step2[4] = step1[4];
  step2[5] = step1[5];
  step2[6] = step1[6];
  step2[7] = step1[7];

  temp1 = step1[8] * cospi_30_64 - step1[15] * cospi_2_64;
  temp2 = step1[8] * cospi_2_64 + step1[15] * cospi_30_64;
  step2[8] = dct_const_round_shift(temp1);
  step2[15] = dct_const_round_shift(temp2);

  temp1 = step1[9] * cospi_14_64 - step1[14] * cospi_18_64;
  temp2 = step1[9] * cospi_18_64 + step1[14] * cospi_14_64;
  step2[9] = dct_const_round_shift(temp1);
  step2[14] = dct_const_round_shift(temp2);

  temp1 = step1[10] * cospi_22_64 - step1[13] * cospi_10_64;
  temp2 = step1[10] * cospi_10_64 + step1[13] * cospi_22_64;
  step2[10] = dct_const_round_shift(temp1);
  step2[13] = dct_const_round_shift(temp2);

  temp1 = step1[11] * cospi_6_64 - step1[12] * cospi_26_64;
  temp2 = step1[11] * cospi_26_64 + step1[12] * cospi_6_64;
  step2[11] = dct_const_round_shift(temp1);
  step2[12] = dct_const_round_shift(temp2);

  step2[16] = step1[16] + step1[17];
  step2[17] = step1[16] - step1[17];
  step2[18] = -step1[18] + step1[19];
  step2[19] = step1[18] + step1[19];
  step2[20] = step1[20] + step1[21];
  step2[21] = step1[20] - step1[21];
  step2[22] = -step1[22] + step1[23];
  step2[23] = step1[22] + step1[23];
  step2[24] = step1[24] + step1[25];
  step2[25] = step1[24] - step1[25];
  step2[26] = -step1[26] + step1[27];
  step2[27] = step1[26] + step1[27];
  step2[28] = step1[28] + step1[29];
  step2[29] = step1[28] - step1[29];
  step2[30] = -step1[30] + step1[31];
  step2[31] = step1[30] + step1[31];

  // stage 3
  step1[0] = step2[0];
  step1[1] = step2[1];
  step1[2] = step2[2];
  step1[3] = step2[3];

  temp1 = step2[4] * cospi_28_64 - step2[7] * cospi_4_64;
  temp2 = step2[4] * cospi_4_64 + step2[7] * cospi_28_64;
  step1[4] = dct_const_round_shift(temp1);
  step1[7] = dct_const_round_shift(temp2);
  temp1 = step2[5] * cospi_12_64 - step2[6] * cospi_20_64;
  temp2 = step2[5] * cospi_20_64 + step2[6] * cospi_12_64;
  step1[5] = dct_const_round_shift(temp1);
  step1[6] = dct_const_round_shift(temp2);

  step1[8] = step2[8] + step2[9];
  step1[9] = step2[8] - step2[9];
  step1[10] = -step2[10] + step2[11];
  step1[11] = step2[10] + step2[11];
  step1[12] = step2[12] + step2[13];
  step1[13] = step2[12] - step2[13];
  step1[14] = -step2[14] + step2[15];
  step1[15] = step2[14] + step2[15];

  step1[16] = step2[16];
  step1[31] = step2[31];
  temp1 = -step2[17] * cospi_4_64 + step2[30] * cospi_28_64;
  temp2 = step2[17] * cospi_28_64 + step2[30] * cospi_4_64;
  step1[17] = dct_const_round_shift(temp1);
  step1[30] = dct_const_round_shift(temp2);
  temp1 = -step2[18] * cospi_28_64 - step2[29] * cospi_4_64;
  temp2 = -step2[18] * cospi_4_64 + step2[29] * cospi_28_64;
  step1[18] = dct_const_round_shift(temp1);
  step1[29] = dct_const_round_shift(temp2);
  step1[19] = step2[19];
  step1[20] = step2[20];
  temp1 = -step2[21] * cospi_20_64 + step2[26] * cospi_12_64;
  temp2 = step2[21] * cospi_12_64 + step2[26] * cospi_20_64;
  step1[21] = dct_const_round_shift(temp1);
  step1[26] = dct_const_round_shift(temp2);
  temp1 = -step2[22] * cospi_12_64 - step2[25] * cospi_20_64;
  temp2 = -step2[22] * cospi_20_64 + step2[25] * cospi_12_64;
  step1[22] = dct_const_round_shift(temp1);
  step1[25] = dct_const_round_shift(temp2);
  step1[23] = step2[23];
  step1[24] = step2[24];
  step1[27] = step2[27];
  step1[28] = step2[28];

  // stage 4
  temp1 = (step1[0] + step1[1]) * cospi_16_64;
  temp2 = (step1[0] - step1[1]) * cospi_16_64;
  step2[0] = dct_const_round_shift(temp1);
  step2[1] = dct_const_round_shift(temp2);
  temp1 = step1[2] * cospi_24_64 - step1[3] * cospi_8_64;
  temp2 = step1[2] * cospi_8_64 + step1[3] * cospi_24_64;
  step2[2] = dct_const_round_shift(temp1);
  step2[3] = dct_const_round_shift(temp2);
  step2[4] = step1[4] + step1[5];
  step2[5] = step1[4] - step1[5];
  step2[6] = -step1[6] + step1[7];
  step2[7] = step1[6] + step1[7];

  step2[8] = step1[8];
  step2[15] = step1[15];
  temp1 = -step1[9] * cospi_8_64 + step1[14] * cospi_24_64;
  temp2 = step1[9] * cospi_24_64 + step1[14] * cospi_8_64;
  step2[9] = dct_const_round_shift(temp1);
  step2[14] = dct_const_round_shift(temp2);
  temp1 = -step1[10] * cospi_24_64 - step1[13] * cospi_8_64;
  temp2 = -step1[10] * cospi_8_64 + step1[13] * cospi_24_64;
  step2[10] = dct_const_round_shift(temp1);
  step2[13] = dct_const_round_shift(temp2);
  step2[11] = step1[11];
  step2[12] = step1[12];

  step2[16] = step1[16] + step1[19];
  step2[17] = step1[17] + step1[18];
  step2[18] = step1[17] - step1[18];
  step2[19] = step1[16] - step1[19];
  step2[20] = -step1[20] + step1[23];
  step2[21] = -step1[21] + step1[22];
  step2[22] = step1[21] + step1[22];
  step2[23] = step1[20] + step1[23];

  step2[24] = step1[24] + step1[27];
  step2[25] = step1[25] + step1[26];
  step2[26] = step1[25] - step1[26];
  step2[27] = step1[24] - step1[27];
  step2[28] = -step1[28] + step1[31];
  step2[29] = -step1[29] + step1[30];
  step2[30] = step1[29] + step1[30];
  step2[31] = step1[28] + step1[31];

  // stage 5
  step1[0] = step2[0] + step2[3];
  step1[1] = step2[1] + step2[2];
  step1[2] = step2[1] - step2[2];
  step1[3] = step2[0] - step2[3];
  step1[4] = step2[4];
  temp1 = (step2[6] - step2[5]) * cospi_16_64;
  temp2 = (step2[5] + step2[6]) * cospi_16_64;
  step1[5] = dct_const_round_shift(temp1);
  step1[6] = dct_const_round_shift(temp2);
  step1[7] = step2[7];

  step1[8] = step2[8] + step2[11];
  step1[9] = step2[9] + step2[10];
  step1[10] = step2[9] - step2[10];
  step1[11] = step2[8] - step2[11];
  step1[12] = -step2[12] + step2[15];
  step1[13] = -step2[13] + step2[14];
  step1[14] = step2[13] + step2[14];
  step1[15] = step2[12] + step2[15];

  step1[16] = step2[16];
  step1[17] = step2[17];
  temp1 = -step2[18] * cospi_8_64 + step2[29] * cospi_24_64;
  temp2 = step2[18] * cospi_24_64 + step2[29] * cospi_8_64;
  step1[18] = dct_const_round_shift(temp1);
  step1[29] = dct_const_round_shift(temp2);
  temp1 = -step2[19] * cospi_8_64 + step2[28] * cospi_24_64;
  temp2 = step2[19] * cospi_24_64 + step2[28] * cospi_8_64;
  step1[19] = dct_const_round_shift(temp1);
  step1[28] = dct_const_round_shift(temp2);
  temp1 = -step2[20] * cospi_24_64 - step2[27] * cospi_8_64;
  temp2 = -step2[20] * cospi_8_64 + step2[27] * cospi_24_64;
  step1[20] = dct_const_round_shift(temp1);
  step1[27] = dct_const_round_shift(temp2);
  temp1 = -step2[21] * cospi_24_64 - step2[26] * cospi_8_64;
  temp2 = -step2[21] * cospi_8_64 + step2[26] * cospi_24_64;
  step1[21] = dct_const_round_shift(temp1);
  step1[26] = dct_const_round_shift(temp2);
  step1[22] = step2[22];
  step1[23] = step2[23];
  step1[24] = step2[24];
  step1[25] = step2[25];
  step1[30] = step2[30];
  step1[31] = step2[31];

  // stage 6
  step2[0] = step1[0] + step1[7];
  step2[1] = step1[1] + step1[6];
  step2[2] = step1[2] + step1[5];
  step2[3] = step1[3] + step1[4];
  step2[4] = step1[3] - step1[4];
  step2[5] = step1[2] - step1[5];
  step2[6] = step1[1] - step1[6];
  step2[7] = step1[0] - step1[7];
  step2[8] = step1[8];
  step2[9] = step1[9];
  temp1 = (-step1[10] + step1[13]) * cospi_16_64;
  temp2 = (step1[10] + step1[13]) * cospi_16_64;
  step2[10] = dct_const_round_shift(temp1);
  step2[13] = dct_const_round_shift(temp2);
  temp1 = (-step1[11] + step1[12]) * cospi_16_64;
  temp2 = (step1[11] + step1[12]) * cospi_16_64;
  step2[11] = dct_const_round_shift(temp1);
  step2[12] = dct_const_round_shift(temp2);
  step2[14] = step1[14];
  step2[15] = step1[15];

  step2[16] = step1[16] + step1[23];
  step2[17] = step1[17] + step1[22];
  step2[18] = step1[18] + step1[21];
  step2[19] = step1[19] + step1[20];
  step2[20] = step1[19] - step1[20];
  step2[21] = step1[18] - step1[21];
  step2[22] = step1[17] - step1[22];
  step2[23] = step1[16] - step1[23];

  step2[24] = -step1[24] + step1[31];
  step2[25] = -step1[25] + step1[30];
  step2[26] = -step1[26] + step1[29];
  step2[27] = -step1[27] + step1[28];
  step2[28] = step1[27] + step1[28];
  step2[29] = step1[26] + step1[29];
  step2[30] = step1[25] + step1[30];
  step2[31] = step1[24] + step1[31];

  // stage 7
  step1[0] = step2[0] + step2[15];
  step1[1] = step2[1] + step2[14];
  step1[2] = step2[2] + step2[13];
  step1[3] = step2[3] + step2[12];
  step1[4] = step2[4] + step2[11];
  step1[5] = step2[5] + step2[10];
  step1[6] = step2[6] + step2[9];
  step1[7] = step2[7] + step2[8];
  step1[8] = step2[7] - step2[8];
  step1[9] = step2[6] - step2[9];
  step1[10] = step2[5] - step2[10];
  step1[11] = step2[4] - step2[11];
  step1[12] = step2[3] - step2[12];
  step1[13] = step2[2] - step2[13];
  step1[14] = step2[1] - step2[14];
  step1[15] = step2[0] - step2[15];

  step1[16] = step2[16];
  step1[17] = step2[17];
  step1[18] = step2[18];
  step1[19] = step2[19];
  temp1 = (-step2[20] + step2[27]) * cospi_16_64;
  temp2 = (step2[20] + step2[27]) * cospi_16_64;
  step1[20] = dct_const_round_shift(temp1);
  step1[27] = dct_const_round_shift(temp2);
  temp1 = (-step2[21] + step2[26]) * cospi_16_64;
  temp2 = (step2[21] + step2[26]) * cospi_16_64;
  step1[21] = dct_const_round_shift(temp1);
  step1[26] = dct_const_round_shift(temp2);
  temp1 = (-step2[22] + step2[25]) * cospi_16_64;
  temp2 = (step2[22] + step2[25]) * cospi_16_64;
  step1[22] = dct_const_round_shift(temp1);
  step1[25] = dct_const_round_shift(temp2);
  temp1 = (-step2[23] + step2[24]) * cospi_16_64;
  temp2 = (step2[23] + step2[24]) * cospi_16_64;
  step1[23] = dct_const_round_shift(temp1);
  step1[24] = dct_const_round_shift(temp2);
  step1[28] = step2[28];
  step1[29] = step2[29];
  step1[30] = step2[30];
  step1[31] = step2[31];

  // final stage
  output[0] = step1[0] + step1[31];
  output[1] = step1[1] + step1[30];
  output[2] = step1[2] + step1[29];
  output[3] = step1[3] + step1[28];
  output[4] = step1[4] + step1[27];
  output[5] = step1[5] + step1[26];
  output[6] = step1[6] + step1[25];
  output[7] = step1[7] + step1[24];
  output[8] = step1[8] + step1[23];
  output[9] = step1[9] + step1[22];
  output[10] = step1[10] + step1[21];
  output[11] = step1[11] + step1[20];
  output[12] = step1[12] + step1[19];
  output[13] = step1[13] + step1[18];
  output[14] = step1[14] + step1[17];
  output[15] = step1[15] + step1[16];
  output[16] = step1[15] - step1[16];
  output[17] = step1[14] - step1[17];
  output[18] = step1[13] - step1[18];
  output[19] = step1[12] - step1[19];
  output[20] = step1[11] - step1[20];
  output[21] = step1[10] - step1[21];
  output[22] = step1[9] - step1[22];
  output[23] = step1[8] - step1[23];
  output[24] = step1[7] - step1[24];
  output[25] = step1[6] - step1[25];
  output[26] = step1[5] - step1[26];
  output[27] = step1[4] - step1[27];
  output[28] = step1[3] - step1[28];
  output[29] = step1[2] - step1[29];
  output[30] = step1[1] - step1[30];
  output[31] = step1[0] - step1[31];
}

void vp9_short_idct32x32_c(int16_t *input, int16_t *output, int pitch) {
  int16_t out[32 * 32];
  int16_t *outptr = out;
  const int half_pitch = pitch >> 1;
  int i, j;
  int16_t temp_in[32], temp_out[32];

  // Rows
  for (i = 0; i < 32; ++i) {
    idct32_1d(input, outptr);
    input += 32;
    outptr += 32;
  }

  // Columns
  for (i = 0; i < 32; ++i) {
    for (j = 0; j < 32; ++j)
      temp_in[j] = out[j * 32 + i];
    idct32_1d(temp_in, temp_out);
    for (j = 0; j < 32; ++j)
      output[j * half_pitch + i] = ROUND_POWER_OF_TWO(temp_out[j], 6);
  }
}

void vp9_short_idct1_32x32_c(int16_t *input, int16_t *output) {
  int16_t out = dct_const_round_shift(input[0] * cospi_16_64);
  out = dct_const_round_shift(out * cospi_16_64);
  output[0] = ROUND_POWER_OF_TWO(out, 6);
}

void vp9_short_idct10_32x32_c(int16_t *input, int16_t *output, int pitch) {
  int16_t out[32 * 32];
  int16_t *outptr = out;
  const int half_pitch = pitch >> 1;
  int i, j;
  int16_t temp_in[32], temp_out[32];

  /* First transform rows. Since all non-zero dct coefficients are in
   * upper-left 4x4 area, we only need to calculate first 4 rows here.
   */
  vpx_memset(out, 0, sizeof(out));
  for (i = 0; i < 4; ++i) {
    idct32_1d(input, outptr);
    input += 32;
    outptr += 32;
  }

  // Columns
  for (i = 0; i < 32; ++i) {
    for (j = 0; j < 32; ++j)
      temp_in[j] = out[j * 32 + i];
    idct32_1d(temp_in, temp_out);
    for (j = 0; j < 32; ++j)
      output[j * half_pitch + i] = ROUND_POWER_OF_TWO(temp_out[j], 6);
  }
}
