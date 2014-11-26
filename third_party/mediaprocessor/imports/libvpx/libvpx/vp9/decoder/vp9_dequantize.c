/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9_rtcd.h"
#include "vp9/decoder/vp9_dequantize.h"
#include "vpx_mem/vpx_mem.h"
#include "vp9/decoder/vp9_onyxd_int.h"
#include "vp9/common/vp9_common.h"


static void add_residual(const int16_t *diff, const uint8_t *pred, int pitch,
                         uint8_t *dest, int stride, int width, int height) {
  int r, c;

  for (r = 0; r < height; r++) {
    for (c = 0; c < width; c++)
      dest[c] = clip_pixel(diff[c] + pred[c]);

    dest += stride;
    diff += width;
    pred += pitch;
  }
}

void vp9_add_residual_4x4_c(const int16_t *diff, const uint8_t *pred, int pitch,
                         uint8_t *dest, int stride) {
  add_residual(diff, pred, pitch, dest, stride, 4, 4);
}

void vp9_add_residual_8x8_c(const int16_t *diff, const uint8_t *pred, int pitch,
                         uint8_t *dest, int stride) {
  add_residual(diff, pred, pitch, dest, stride, 8, 8);
}

void vp9_add_residual_16x16_c(const int16_t *diff, const uint8_t *pred,
                              int pitch, uint8_t *dest, int stride) {
  add_residual(diff, pred, pitch, dest, stride, 16, 16);
}

void vp9_add_residual_32x32_c(const int16_t *diff, const uint8_t *pred,
                              int pitch, uint8_t *dest, int stride) {
  add_residual(diff, pred, pitch, dest, stride, 32, 32);
}

static void add_constant_residual(const int16_t diff, const uint8_t *pred,
                                  int pitch, uint8_t *dest, int stride,
                                  int width, int height) {
  int r, c;

  for (r = 0; r < height; r++) {
    for (c = 0; c < width; c++)
      dest[c] = clip_pixel(diff + pred[c]);

    dest += stride;
    pred += pitch;
  }
}

void vp9_add_constant_residual_8x8_c(const int16_t diff, const uint8_t *pred,
                                     int pitch, uint8_t *dest, int stride) {
  add_constant_residual(diff, pred, pitch, dest, stride, 8, 8);
}

void vp9_add_constant_residual_16x16_c(const int16_t diff, const uint8_t *pred,
                                       int pitch, uint8_t *dest, int stride) {
  add_constant_residual(diff, pred, pitch, dest, stride, 16, 16);
}

void vp9_add_constant_residual_32x32_c(const int16_t diff, const uint8_t *pred,
                                       int pitch, uint8_t *dest, int stride) {
  add_constant_residual(diff, pred, pitch, dest, stride, 32, 32);
}

void vp9_ht_dequant_idct_add_c(TX_TYPE tx_type, int16_t *input,
                               const int16_t *dq,
                               uint8_t *pred, uint8_t *dest,
                               int pitch, int stride, int eob) {
  int i;
  DECLARE_ALIGNED_ARRAY(16, int16_t, output, 16);

  for (i = 0; i < 16; i++)
    input[i] *= dq[i];

  vp9_short_iht4x4(input, output, 4, tx_type);
  vpx_memset(input, 0, 32);
  vp9_add_residual_4x4(output, pred, pitch, dest, stride);
}

void vp9_ht_dequant_idct_add_8x8_c(TX_TYPE tx_type, int16_t *input,
                                   const int16_t *dq,
                                   uint8_t *pred, uint8_t *dest,
                                   int pitch, int stride, int eob) {
  DECLARE_ALIGNED_ARRAY(16, int16_t, output, 64);

  if (eob == 0) {
    // All 0 DCT coefficients
    vp9_copy_mem8x8(pred, pitch, dest, stride);
  } else if (eob > 0) {
    int i;

    input[0] *= dq[0];
    for (i = 1; i < 64; i++)
      input[i] *= dq[1];

    vp9_short_iht8x8(input, output, 8, tx_type);
    vpx_memset(input, 0, 128);
    vp9_add_residual_8x8(output, pred, pitch, dest, stride);
  }
}

void vp9_dequant_idct_add_c(int16_t *input, const int16_t *dq, uint8_t *pred,
                            uint8_t *dest, int pitch, int stride, int eob) {
  int i;
  DECLARE_ALIGNED_ARRAY(16, int16_t, output, 16);

  if (eob > 1) {
    for (i = 0; i < 16; i++)
      input[i] *= dq[i];

    // the idct halves ( >> 1) the pitch
    vp9_short_idct4x4(input, output, 4 << 1);

    vpx_memset(input, 0, 32);

    vp9_add_residual_4x4(output, pred, pitch, dest, stride);
  } else {
    vp9_dc_only_idct_add(input[0]*dq[0], pred, dest, pitch, stride);
    ((int *)input)[0] = 0;
  }
}

void vp9_dequant_dc_idct_add_c(int16_t *input, const int16_t *dq, uint8_t *pred,
                               uint8_t *dest, int pitch, int stride, int dc) {
  int i;
  DECLARE_ALIGNED_ARRAY(16, int16_t, output, 16);

  input[0] = dc;

  for (i = 1; i < 16; i++)
    input[i] *= dq[i];

  // the idct halves ( >> 1) the pitch
  vp9_short_idct4x4(input, output, 4 << 1);
  vpx_memset(input, 0, 32);
  vp9_add_residual_4x4(output, pred, pitch, dest, stride);
}

void vp9_dequant_idct_add_lossless_c(int16_t *input, const int16_t *dq,
                                     uint8_t *pred, uint8_t *dest,
                                     int pitch, int stride, int eob) {
  int i;
  DECLARE_ALIGNED_ARRAY(16, int16_t, output, 16);

  if (eob > 1) {
    for (i = 0; i < 16; i++)
      input[i] *= dq[i];

    vp9_short_iwalsh4x4_c(input, output, 4 << 1);

    vpx_memset(input, 0, 32);

    vp9_add_residual_4x4(output, pred, pitch, dest, stride);
  } else {
    vp9_dc_only_inv_walsh_add(input[0]*dq[0], pred, dest, pitch, stride);
    ((int *)input)[0] = 0;
  }
}

void vp9_dequant_dc_idct_add_lossless_c(int16_t *input, const int16_t *dq,
                                        uint8_t *pred,
                                        uint8_t *dest,
                                        int pitch, int stride, int dc) {
  int i;
  DECLARE_ALIGNED_ARRAY(16, int16_t, output, 16);

  input[0] = dc;

  for (i = 1; i < 16; i++)
    input[i] *= dq[i];

  vp9_short_iwalsh4x4_c(input, output, 4 << 1);
  vpx_memset(input, 0, 32);
  vp9_add_residual_4x4(output, pred, pitch, dest, stride);
}

void vp9_dequant_idct_add_8x8_c(int16_t *input, const int16_t *dq,
                                uint8_t *pred, uint8_t *dest, int pitch,
                                int stride, int eob) {
  DECLARE_ALIGNED_ARRAY(16, int16_t, output, 64);

  // If dc is 1, then input[0] is the reconstructed value, do not need
  // dequantization. Also, when dc is 1, dc is counted in eobs, namely eobs >=1.
  input[0] *= dq[0];

  // The calculation can be simplified if there are not many non-zero dct
  // coefficients. Use eobs to decide what to do.
  // TODO(yunqingwang): "eobs = 1" case is also handled in vp9_short_idct8x8_c.
  // Combine that with code here.
  if (eob == 0) {
    // All 0 DCT coefficients
    vp9_copy_mem8x8(pred, pitch, dest, stride);
  } else if (eob == 1) {
    // DC only DCT coefficient
    int16_t in = input[0];
    int16_t out;

     // Note: the idct1 will need to be modified accordingly whenever
     // vp9_short_idct8x8_c() is modified.
    vp9_short_idct1_8x8_c(&in, &out);
    input[0] = 0;

    vp9_add_constant_residual_8x8(out, pred, pitch, dest, stride);
#if !CONFIG_SCATTERSCAN
  } else if (eob <= 10) {
    input[1] *= dq[1];
    input[2] *= dq[1];
    input[3] *= dq[1];
    input[8] *= dq[1];
    input[9] *= dq[1];
    input[10] *= dq[1];
    input[16] *= dq[1];
    input[17] *= dq[1];
    input[24] *= dq[1];

    vp9_short_idct10_8x8(input, output, 16);

    input[0] = input[1] = input[2] = input[3] = 0;
    input[8] = input[9] = input[10] = 0;
    input[16] = input[17] = 0;
    input[24] = 0;

    vp9_add_residual_8x8(output, pred, pitch, dest, stride);
#endif
  } else {
    int i;

    // recover quantizer for 4 4x4 blocks
    for (i = 1; i < 64; i++)
      input[i] *= dq[1];

    // the idct halves ( >> 1) the pitch
    vp9_short_idct8x8(input, output, 8 << 1);
    vpx_memset(input, 0, 128);
    vp9_add_residual_8x8(output, pred, pitch, dest, stride);
  }
}

void vp9_ht_dequant_idct_add_16x16_c(TX_TYPE tx_type, int16_t *input,
                                     const int16_t *dq, uint8_t *pred,
                                     uint8_t *dest, int pitch, int stride,
                                     int eob) {
  DECLARE_ALIGNED_ARRAY(16, int16_t, output, 256);

  if (eob == 0) {
    // All 0 DCT coefficients
    vp9_copy_mem16x16(pred, pitch, dest, stride);
  } else if (eob > 0) {
    int i;

    input[0] *= dq[0];

    // recover quantizer for 4 4x4 blocks
    for (i = 1; i < 256; i++)
      input[i] *= dq[1];

    // inverse hybrid transform
    vp9_short_iht16x16(input, output, 16, tx_type);

    // the idct halves ( >> 1) the pitch
    // vp9_short_idct16x16(input, output, 32);

    vpx_memset(input, 0, 512);

    vp9_add_residual_16x16(output, pred, pitch, dest, stride);
  }
}

void vp9_dequant_idct_add_16x16_c(int16_t *input, const int16_t *dq,
                                  uint8_t *pred, uint8_t *dest, int pitch,
                                  int stride, int eob) {
  DECLARE_ALIGNED_ARRAY(16, int16_t, output, 256);

  /* The calculation can be simplified if there are not many non-zero dct
   * coefficients. Use eobs to separate different cases. */
  if (eob == 0) {
    /* All 0 DCT coefficient */
    vp9_copy_mem16x16(pred, pitch, dest, stride);
  } else if (eob == 1) {
    /* DC only DCT coefficient. */
    int16_t in = input[0] * dq[0];
    int16_t out;
    /* Note: the idct1 will need to be modified accordingly whenever
     * vp9_short_idct16x16() is modified. */
    vp9_short_idct1_16x16_c(&in, &out);
    input[0] = 0;

    vp9_add_constant_residual_16x16(out, pred, pitch, dest, stride);
#if !CONFIG_SCATTERSCAN
  } else if (eob <= 10) {
    input[0] *= dq[0];

    input[1] *= dq[1];
    input[2] *= dq[1];
    input[3] *= dq[1];
    input[16] *= dq[1];
    input[17] *= dq[1];
    input[18] *= dq[1];
    input[32] *= dq[1];
    input[33] *= dq[1];
    input[48] *= dq[1];

    // the idct halves ( >> 1) the pitch
    vp9_short_idct10_16x16(input, output, 32);

    input[0] = input[1] = input[2] = input[3] = 0;
    input[16] = input[17] = input[18] = 0;
    input[32] = input[33] = 0;
    input[48] = 0;

    vp9_add_residual_16x16(output, pred, pitch, dest, stride);
#endif
  } else {
    int i;

    input[0] *= dq[0];

    // recover quantizer for 4 4x4 blocks
    for (i = 1; i < 256; i++)
      input[i] *= dq[1];

    // the idct halves ( >> 1) the pitch
    vp9_short_idct16x16(input, output, 16 << 1);

    vpx_memset(input, 0, 512);

    vp9_add_residual_16x16(output, pred, pitch, dest, stride);
  }
}

void vp9_dequant_idct_add_32x32_c(int16_t *input, const int16_t *dq,
                                  uint8_t *pred, uint8_t *dest, int pitch,
                                  int stride, int eob) {
  DECLARE_ALIGNED_ARRAY(16, int16_t, output, 1024);

  if (eob) {
    input[0] = input[0] * dq[0] / 2;
    if (eob == 1) {
      vp9_short_idct1_32x32(input, output);
      vp9_add_constant_residual_32x32(output[0], pred, pitch, dest, stride);
      input[0] = 0;
#if !CONFIG_SCATTERSCAN
    } else if (eob <= 10) {
      input[1] = input[1] * dq[1] / 2;
      input[2] = input[2] * dq[1] / 2;
      input[3] = input[3] * dq[1] / 2;
      input[32] = input[32] * dq[1] / 2;
      input[33] = input[33] * dq[1] / 2;
      input[34] = input[34] * dq[1] / 2;
      input[64] = input[64] * dq[1] / 2;
      input[65] = input[65] * dq[1] / 2;
      input[96] = input[96] * dq[1] / 2;

      // the idct halves ( >> 1) the pitch
      vp9_short_idct10_32x32(input, output, 64);

      input[0] = input[1] = input[2] = input[3] = 0;
      input[32] = input[33] = input[34] = 0;
      input[64] = input[65] = 0;
      input[96] = 0;

      vp9_add_residual_32x32(output, pred, pitch, dest, stride);
#endif
    } else {
      int i;
      for (i = 1; i < 1024; i++)
        input[i] = input[i] * dq[1] / 2;
      vp9_short_idct32x32(input, output, 64);
      vpx_memset(input, 0, 2048);
      vp9_add_residual_32x32(output, pred, pitch, dest, stride);
    }
  }
}

void vp9_dequant_idct_add_uv_block_16x16_c(int16_t *q, const int16_t *dq,
                                           uint8_t *dstu,
                                           uint8_t *dstv,
                                           int stride,
                                           MACROBLOCKD *xd) {
  vp9_dequant_idct_add_16x16_c(q, dq, dstu, dstu, stride, stride,
                               xd->eobs[64]);
  vp9_dequant_idct_add_16x16_c(q + 256, dq, dstv, dstv, stride, stride,
                               xd->eobs[80]);
}
