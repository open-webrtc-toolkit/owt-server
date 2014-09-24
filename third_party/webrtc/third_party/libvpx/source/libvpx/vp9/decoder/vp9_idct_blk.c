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
#include "vp9/common/vp9_blockd.h"
#if CONFIG_LOSSLESS
#include "vp9/decoder/vp9_dequantize.h"
#endif

void vp9_dequant_dc_idct_add_y_block_c(int16_t *q, const int16_t *dq,
                                       uint8_t *pre,
                                       uint8_t *dst,
                                       int stride, uint16_t *eobs,
                                       const int16_t *dc) {
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      if (*eobs++ > 1)
        vp9_dequant_dc_idct_add_c(q, dq, pre, dst, 16, stride, dc[0]);
      else
        vp9_dc_only_idct_add_c(dc[0], pre, dst, 16, stride);

      q   += 16;
      pre += 4;
      dst += 4;
      dc++;
    }

    pre += 64 - 16;
    dst += 4 * stride - 16;
  }
}

void vp9_dequant_dc_idct_add_y_block_4x4_inplace_c(int16_t *q,
                                                   const int16_t *dq,
                                                   uint8_t *dst,
                                                   int stride,
                                                   uint16_t *eobs,
                                                   const int16_t *dc,
                                                   MACROBLOCKD *xd) {
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      if (*eobs++ > 1)
        vp9_dequant_dc_idct_add_c(q, dq, dst, dst, stride, stride, dc[0]);
      else
        vp9_dc_only_idct_add_c(dc[0], dst, dst, stride, stride);

      q   += 16;
      dst += 4;
      dc++;
    }

    dst += 4 * stride - 16;
  }
}

void vp9_dequant_idct_add_y_block_c(int16_t *q, const int16_t *dq,
                                    uint8_t *pre,
                                    uint8_t *dst,
                                    int stride, uint16_t *eobs) {
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      if (*eobs++ > 1)
        vp9_dequant_idct_add_c(q, dq, pre, dst, 16, stride);
      else {
        vp9_dc_only_idct_add_c(q[0]*dq[0], pre, dst, 16, stride);
        ((int *)q)[0] = 0;
      }

      q   += 16;
      pre += 4;
      dst += 4;
    }

    pre += 64 - 16;
    dst += 4 * stride - 16;
  }
}

void vp9_dequant_idct_add_uv_block_c(int16_t *q, const int16_t *dq,
                                     uint8_t *pre, uint8_t *dstu,
                                     uint8_t *dstv, int stride,
                                     uint16_t *eobs) {
  int i, j;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 2; j++) {
      if (*eobs++ > 1)
        vp9_dequant_idct_add_c(q, dq, pre, dstu, 8, stride);
      else {
        vp9_dc_only_idct_add_c(q[0]*dq[0], pre, dstu, 8, stride);
        ((int *)q)[0] = 0;
      }

      q    += 16;
      pre  += 4;
      dstu += 4;
    }

    pre  += 32 - 8;
    dstu += 4 * stride - 8;
  }

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 2; j++) {
      if (*eobs++ > 1)
        vp9_dequant_idct_add_c(q, dq, pre, dstv, 8, stride);
      else {
        vp9_dc_only_idct_add_c(q[0]*dq[0], pre, dstv, 8, stride);
        ((int *)q)[0] = 0;
      }

      q    += 16;
      pre  += 4;
      dstv += 4;
    }

    pre  += 32 - 8;
    dstv += 4 * stride - 8;
  }
}

void vp9_dequant_idct_add_uv_block_4x4_inplace_c(int16_t *q, const int16_t *dq,
                                                 uint8_t *dstu,
                                                 uint8_t *dstv,
                                                 int stride,
                                                 uint16_t *eobs,
                                                 MACROBLOCKD *xd) {
  int i, j;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 2; j++) {
      if (*eobs++ > 1) {
        vp9_dequant_idct_add_c(q, dq, dstu, dstu, stride, stride);
      } else {
        vp9_dc_only_idct_add_c(q[0]*dq[0], dstu, dstu, stride, stride);
        ((int *)q)[0] = 0;
      }

      q    += 16;
      dstu += 4;
    }

    dstu += 4 * stride - 8;
  }

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 2; j++) {
      if (*eobs++ > 1) {
        vp9_dequant_idct_add_c(q, dq, dstv, dstv, stride, stride);
      } else {
        vp9_dc_only_idct_add_c(q[0]*dq[0], dstv, dstv, stride, stride);
        ((int *)q)[0] = 0;
      }

      q    += 16;
      dstv += 4;
    }

    dstv += 4 * stride - 8;
  }
}

void vp9_dequant_dc_idct_add_y_block_8x8_c(int16_t *q, const int16_t *dq,
                                           uint8_t *pre,
                                           uint8_t *dst,
                                           int stride, uint16_t *eobs,
                                           const int16_t *dc,
                                           MACROBLOCKD *xd) {
  q[0] = dc[0];
  vp9_dequant_idct_add_8x8_c(q, dq, pre, dst, 16, stride, 1, xd->eobs[0]);

  q[64] = dc[1];
  vp9_dequant_idct_add_8x8_c(&q[64], dq, pre + 8, dst + 8, 16, stride, 1,
                             xd->eobs[4]);

  q[128] = dc[4];
  vp9_dequant_idct_add_8x8_c(&q[128], dq, pre + 8 * 16,
                                dst + 8 * stride, 16, stride, 1, xd->eobs[8]);

  q[192] = dc[8];
  vp9_dequant_idct_add_8x8_c(&q[192], dq, pre + 8 * 16 + 8,
                                dst + 8 * stride + 8, 16, stride, 1,
                                xd->eobs[12]);
}

void vp9_dequant_dc_idct_add_y_block_8x8_inplace_c(int16_t *q,
                                                   const int16_t *dq,
                                                   uint8_t *dst,
                                                   int stride,
                                                   uint16_t *eobs,
                                                   const int16_t *dc,
                                                   MACROBLOCKD *xd) {
  q[0] = dc[0];
  vp9_dequant_idct_add_8x8_c(q, dq, dst, dst, stride, stride, 1, xd->eobs[0]);

  q[64] = dc[1];
  vp9_dequant_idct_add_8x8_c(&q[64], dq, dst + 8,
                                dst + 8, stride, stride, 1, xd->eobs[4]);

  q[128] = dc[4];
  vp9_dequant_idct_add_8x8_c(&q[128], dq, dst + 8 * stride,
                                dst + 8 * stride, stride, stride, 1,
                                xd->eobs[8]);

  q[192] = dc[8];
  vp9_dequant_idct_add_8x8_c(&q[192], dq, dst + 8 * stride + 8,
                                dst + 8 * stride + 8, stride, stride, 1,
                                xd->eobs[12]);
}

void vp9_dequant_idct_add_y_block_8x8_c(int16_t *q, const int16_t *dq,
                                        uint8_t *pre,
                                        uint8_t *dst,
                                        int stride, uint16_t *eobs,
                                        MACROBLOCKD *xd) {
  uint8_t *origdest = dst;
  uint8_t *origpred = pre;

  vp9_dequant_idct_add_8x8_c(q, dq, pre, dst, 16, stride, 0, xd->eobs[0]);
  vp9_dequant_idct_add_8x8_c(&q[64], dq, origpred + 8,
                             origdest + 8, 16, stride, 0, xd->eobs[4]);
  vp9_dequant_idct_add_8x8_c(&q[128], dq, origpred + 8 * 16,
                             origdest + 8 * stride, 16, stride, 0, xd->eobs[8]);
  vp9_dequant_idct_add_8x8_c(&q[192], dq, origpred + 8 * 16 + 8,
                             origdest + 8 * stride + 8, 16, stride, 0,
                             xd->eobs[12]);
}

void vp9_dequant_idct_add_uv_block_8x8_c(int16_t *q, const int16_t *dq,
                                         uint8_t *pre,
                                         uint8_t *dstu,
                                         uint8_t *dstv,
                                         int stride, uint16_t *eobs,
                                         MACROBLOCKD *xd) {
  vp9_dequant_idct_add_8x8_c(q, dq, pre, dstu, 8, stride, 0, xd->eobs[16]);

  q    += 64;
  pre  += 64;

  vp9_dequant_idct_add_8x8_c(q, dq, pre, dstv, 8, stride, 0, xd->eobs[20]);
}

void vp9_dequant_idct_add_uv_block_8x8_inplace_c(int16_t *q, const int16_t *dq,
                                                 uint8_t *dstu,
                                                 uint8_t *dstv,
                                                 int stride,
                                                 uint16_t *eobs,
                                                 MACROBLOCKD *xd) {
  vp9_dequant_idct_add_8x8_c(q, dq, dstu, dstu, stride, stride, 0,
                             xd->eobs[16]);

  q += 64;
  vp9_dequant_idct_add_8x8_c(q, dq, dstv, dstv, stride, stride, 0,
                             xd->eobs[20]);
}

#if CONFIG_LOSSLESS
void vp9_dequant_dc_idct_add_y_block_lossless_c(int16_t *q, const int16_t *dq,
                                                uint8_t *pre,
                                                uint8_t *dst,
                                                int stride,
                                                uint16_t *eobs,
                                                const int16_t *dc) {
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      if (*eobs++ > 1)
        vp9_dequant_dc_idct_add_lossless_c(q, dq, pre, dst, 16, stride, dc[0]);
      else
        vp9_dc_only_inv_walsh_add_c(dc[0], pre, dst, 16, stride);

      q   += 16;
      pre += 4;
      dst += 4;
      dc++;
    }

    pre += 64 - 16;
    dst += 4 * stride - 16;
  }
}

void vp9_dequant_idct_add_y_block_lossless_c(int16_t *q, const int16_t *dq,
                                             uint8_t *pre,
                                             uint8_t *dst,
                                             int stride, uint16_t *eobs) {
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      if (*eobs++ > 1)
        vp9_dequant_idct_add_lossless_c(q, dq, pre, dst, 16, stride);
      else {
        vp9_dc_only_inv_walsh_add_c(q[0]*dq[0], pre, dst, 16, stride);
        ((int *)q)[0] = 0;
      }

      q   += 16;
      pre += 4;
      dst += 4;
    }

    pre += 64 - 16;
    dst += 4 * stride - 16;
  }
}

void vp9_dequant_idct_add_uv_block_lossless_c(int16_t *q, const int16_t *dq,
                                              uint8_t *pre,
                                              uint8_t *dstu,
                                              uint8_t *dstv,
                                              int stride,
                                              uint16_t *eobs) {
  int i, j;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 2; j++) {
      if (*eobs++ > 1)
        vp9_dequant_idct_add_lossless_c(q, dq, pre, dstu, 8, stride);
      else {
        vp9_dc_only_inv_walsh_add_c(q[0]*dq[0], pre, dstu, 8, stride);
        ((int *)q)[0] = 0;
      }

      q    += 16;
      pre  += 4;
      dstu += 4;
    }

    pre  += 32 - 8;
    dstu += 4 * stride - 8;
  }

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 2; j++) {
      if (*eobs++ > 1)
        vp9_dequant_idct_add_lossless_c(q, dq, pre, dstv, 8, stride);
      else {
        vp9_dc_only_inv_walsh_add_c(q[0]*dq[0], pre, dstv, 8, stride);
        ((int *)q)[0] = 0;
      }

      q    += 16;
      pre  += 4;
      dstv += 4;
    }

    pre  += 32 - 8;
    dstv += 4 * stride - 8;
  }
}
#endif

