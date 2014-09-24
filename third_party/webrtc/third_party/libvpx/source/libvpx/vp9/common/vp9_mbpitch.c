/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/common/vp9_blockd.h"

typedef enum {
  PRED = 0,
  DEST = 1
} BLOCKSET;

static void setup_block(BLOCKD *b,
                        int mv_stride,
                        uint8_t **base,
                        uint8_t **base2,
                        int Stride,
                        int offset,
                        BLOCKSET bs) {
  if (bs == DEST) {
    b->dst_stride = Stride;
    b->dst = offset;
    b->base_dst = base;
  } else {
    b->pre_stride = Stride;
    b->pre = offset;
    b->base_pre = base;
    b->base_second_pre = base2;
  }
}

static void setup_macroblock(MACROBLOCKD *xd, BLOCKSET bs) {
  int block;

  uint8_t **y, **u, **v;
  uint8_t **y2 = NULL, **u2 = NULL, **v2 = NULL;
  BLOCKD *blockd = xd->block;
  int stride;

  if (bs == DEST) {
    y = &xd->dst.y_buffer;
    u = &xd->dst.u_buffer;
    v = &xd->dst.v_buffer;
  } else {
    y = &xd->pre.y_buffer;
    u = &xd->pre.u_buffer;
    v = &xd->pre.v_buffer;

    y2 = &xd->second_pre.y_buffer;
    u2 = &xd->second_pre.u_buffer;
    v2 = &xd->second_pre.v_buffer;
  }

  stride = xd->dst.y_stride;
  for (block = 0; block < 16; block++) { /* y blocks */
    setup_block(&blockd[block], stride, y, y2, stride,
                (block >> 2) * 4 * stride + (block & 3) * 4, bs);
  }

  stride = xd->dst.uv_stride;
  for (block = 16; block < 20; block++) { /* U and V blocks */
    setup_block(&blockd[block], stride, u, u2, stride,
      ((block - 16) >> 1) * 4 * stride + (block & 1) * 4, bs);

    setup_block(&blockd[block + 4], stride, v, v2, stride,
      ((block - 16) >> 1) * 4 * stride + (block & 1) * 4, bs);
  }
}

void vp9_setup_block_dptrs(MACROBLOCKD *xd) {
  int r, c;
  BLOCKD *blockd = xd->block;

  for (r = 0; r < 4; r++) {
    for (c = 0; c < 4; c++) {
      blockd[r * 4 + c].diff = &xd->diff[r * 4 * 16 + c * 4];
      blockd[r * 4 + c].predictor = xd->predictor + r * 4 * 16 + c * 4;
    }
  }

  for (r = 0; r < 2; r++) {
    for (c = 0; c < 2; c++) {
      blockd[16 + r * 2 + c].diff = &xd->diff[256 + r * 4 * 8 + c * 4];
      blockd[16 + r * 2 + c].predictor =
        xd->predictor + 256 + r * 4 * 8 + c * 4;

    }
  }

  for (r = 0; r < 2; r++) {
    for (c = 0; c < 2; c++) {
      blockd[20 + r * 2 + c].diff = &xd->diff[320 + r * 4 * 8 + c * 4];
      blockd[20 + r * 2 + c].predictor =
        xd->predictor + 320 + r * 4 * 8 + c * 4;

    }
  }

  blockd[24].diff = &xd->diff[384];

  for (r = 0; r < 25; r++) {
    blockd[r].qcoeff  = xd->qcoeff  + r * 16;
    blockd[r].dqcoeff = xd->dqcoeff + r * 16;
  }
}

void vp9_build_block_doffsets(MACROBLOCKD *xd) {
  /* handle the destination pitch features */
  setup_macroblock(xd, DEST);
  setup_macroblock(xd, PRED);
}
