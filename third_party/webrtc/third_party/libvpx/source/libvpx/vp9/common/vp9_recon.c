/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "./vpx_config.h"
#include "vp9_rtcd.h"
#include "vp9/common/vp9_blockd.h"

void vp9_recon_b_c(uint8_t *pred_ptr,
                   int16_t *diff_ptr,
                   uint8_t *dst_ptr,
                   int stride) {
  int r, c;

  for (r = 0; r < 4; r++) {
    for (c = 0; c < 4; c++) {
      dst_ptr[c] = clip_pixel(diff_ptr[c] + pred_ptr[c]);
    }

    dst_ptr += stride;
    diff_ptr += 16;
    pred_ptr += 16;
  }
}

void vp9_recon_uv_b_c(uint8_t *pred_ptr,
                      int16_t *diff_ptr,
                      uint8_t *dst_ptr,
                      int stride) {
  int r, c;

  for (r = 0; r < 4; r++) {
    for (c = 0; c < 4; c++) {
      dst_ptr[c] = clip_pixel(diff_ptr[c] + pred_ptr[c]);
    }

    dst_ptr += stride;
    diff_ptr += 8;
    pred_ptr += 8;
  }
}

void vp9_recon4b_c(uint8_t *pred_ptr,
                   int16_t *diff_ptr,
                   uint8_t *dst_ptr,
                   int stride) {
  int r, c;

  for (r = 0; r < 4; r++) {
    for (c = 0; c < 16; c++) {
      dst_ptr[c] = clip_pixel(diff_ptr[c] + pred_ptr[c]);
    }

    dst_ptr += stride;
    diff_ptr += 16;
    pred_ptr += 16;
  }
}

void vp9_recon2b_c(uint8_t *pred_ptr,
                   int16_t *diff_ptr,
                   uint8_t *dst_ptr,
                   int stride) {
  int r, c;

  for (r = 0; r < 4; r++) {
    for (c = 0; c < 8; c++) {
      dst_ptr[c] = clip_pixel(diff_ptr[c] + pred_ptr[c]);
    }

    dst_ptr += stride;
    diff_ptr += 8;
    pred_ptr += 8;
  }
}

void vp9_recon_mby_s_c(MACROBLOCKD *xd, uint8_t *dst) {
  int x, y;
  BLOCKD *b = &xd->block[0];
  int stride = b->dst_stride;
  int16_t *diff = b->diff;

  for (y = 0; y < 16; y++) {
    for (x = 0; x < 16; x++) {
      dst[x] = clip_pixel(dst[x] + diff[x]);
    }
    dst += stride;
    diff += 16;
  }
}

void vp9_recon_mbuv_s_c(MACROBLOCKD *xd, uint8_t *udst, uint8_t *vdst) {
  int x, y, i;
  uint8_t *dst = udst;

  for (i = 0; i < 2; i++, dst = vdst) {
    BLOCKD *b = &xd->block[16 + 4 * i];
    int stride = b->dst_stride;
    int16_t *diff = b->diff;

    for (y = 0; y < 8; y++) {
      for (x = 0; x < 8; x++) {
        dst[x] = clip_pixel(dst[x] + diff[x]);
      }
      dst += stride;
      diff += 8;
    }
  }
}

void vp9_recon_sby_s_c(MACROBLOCKD *xd, uint8_t *dst) {
  int x, y, stride = xd->block[0].dst_stride;
  int16_t *diff = xd->sb_coeff_data.diff;

  for (y = 0; y < 32; y++) {
    for (x = 0; x < 32; x++) {
      dst[x] = clip_pixel(dst[x] + diff[x]);
    }
    dst += stride;
    diff += 32;
  }
}

void vp9_recon_sbuv_s_c(MACROBLOCKD *xd, uint8_t *udst, uint8_t *vdst) {
  int x, y, stride = xd->block[16].dst_stride;
  int16_t *udiff = xd->sb_coeff_data.diff + 1024;
  int16_t *vdiff = xd->sb_coeff_data.diff + 1280;

  for (y = 0; y < 16; y++) {
    for (x = 0; x < 16; x++) {
      udst[x] = clip_pixel(udst[x] + udiff[x]);
      vdst[x] = clip_pixel(vdst[x] + vdiff[x]);
    }
    udst += stride;
    vdst += stride;
    udiff += 16;
    vdiff += 16;
  }
}

void vp9_recon_mby_c(MACROBLOCKD *xd) {
  int i;

  for (i = 0; i < 16; i += 4) {
    BLOCKD *b = &xd->block[i];

    vp9_recon4b(b->predictor, b->diff, *(b->base_dst) + b->dst, b->dst_stride);
  }
}

void vp9_recon_mb_c(MACROBLOCKD *xd) {
  int i;

  for (i = 0; i < 16; i += 4) {
    BLOCKD *b = &xd->block[i];

    vp9_recon4b(b->predictor, b->diff, *(b->base_dst) + b->dst, b->dst_stride);
  }

  for (i = 16; i < 24; i += 2) {
    BLOCKD *b = &xd->block[i];

    vp9_recon2b(b->predictor, b->diff, *(b->base_dst) + b->dst, b->dst_stride);
  }
}
