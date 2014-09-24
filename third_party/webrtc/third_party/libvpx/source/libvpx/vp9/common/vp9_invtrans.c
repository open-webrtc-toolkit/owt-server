/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9/common/vp9_invtrans.h"
#include "./vp9_rtcd.h"

static void recon_dcblock(MACROBLOCKD *xd) {
  BLOCKD *b = &xd->block[24];
  int i;

  for (i = 0; i < 16; i++) {
    xd->block[i].dqcoeff[0] = b->diff[i];
  }
}

static void recon_dcblock_8x8(MACROBLOCKD *xd) {
  BLOCKD *b = &xd->block[24]; // for coeff 0, 2, 8, 10

  xd->block[0].dqcoeff[0] = b->diff[0];
  xd->block[4].dqcoeff[0] = b->diff[1];
  xd->block[8].dqcoeff[0] = b->diff[4];
  xd->block[12].dqcoeff[0] = b->diff[8];
}

void vp9_inverse_transform_b_4x4(MACROBLOCKD *xd, int block, int pitch) {
  BLOCKD *b = &xd->block[block];
  if (b->eob <= 1)
    xd->inv_xform4x4_1_x8(b->dqcoeff, b->diff, pitch);
  else
    xd->inv_xform4x4_x8(b->dqcoeff, b->diff, pitch);
}

void vp9_inverse_transform_mby_4x4(MACROBLOCKD *xd) {
  int i;
  BLOCKD *blockd = xd->block;
  int has_2nd_order = get_2nd_order_usage(xd);

  if (has_2nd_order) {
    /* do 2nd order transform on the dc block */
    vp9_short_inv_walsh4x4(blockd[24].dqcoeff, blockd[24].diff);
    recon_dcblock(xd);
  }

  for (i = 0; i < 16; i++) {
    TX_TYPE tx_type = get_tx_type_4x4(xd, &xd->block[i]);
    if (tx_type != DCT_DCT) {
      vp9_ihtllm(xd->block[i].dqcoeff, xd->block[i].diff, 32,
                   tx_type, 4, xd->block[i].eob);
    } else {
      vp9_inverse_transform_b_4x4(xd, i, 32);
    }
  }
}

void vp9_inverse_transform_mbuv_4x4(MACROBLOCKD *xd) {
  int i;

  for (i = 16; i < 24; i++) {
    vp9_inverse_transform_b_4x4(xd, i, 16);
  }
}

void vp9_inverse_transform_mb_4x4(MACROBLOCKD *xd) {
  vp9_inverse_transform_mby_4x4(xd);
  vp9_inverse_transform_mbuv_4x4(xd);
}

void vp9_inverse_transform_b_8x8(int16_t *input_dqcoeff, int16_t *output_coeff,
                                 int pitch) {
  vp9_short_idct8x8(input_dqcoeff, output_coeff, pitch);
}

void vp9_inverse_transform_mby_8x8(MACROBLOCKD *xd) {
  int i;
  BLOCKD *blockd = xd->block;
  int has_2nd_order = get_2nd_order_usage(xd);

  if (has_2nd_order) {
    // do 2nd order transform on the dc block
    vp9_short_ihaar2x2(blockd[24].dqcoeff, blockd[24].diff, 8);
    recon_dcblock_8x8(xd); // need to change for 8x8
  }

  for (i = 0; i < 9; i += 8) {
    TX_TYPE tx_type = get_tx_type_8x8(xd, &xd->block[i]);
    if (tx_type != DCT_DCT) {
      vp9_ihtllm(xd->block[i].dqcoeff, xd->block[i].diff, 32, tx_type, 8,
                 xd->block[i].eob);
    } else {
      vp9_inverse_transform_b_8x8(&blockd[i].dqcoeff[0],
                                  &blockd[i].diff[0], 32);
    }
  }
  for (i = 2; i < 11; i += 8) {
    TX_TYPE tx_type = get_tx_type_8x8(xd, &xd->block[i]);
    if (tx_type != DCT_DCT) {
      vp9_ihtllm(xd->block[i + 2].dqcoeff, xd->block[i].diff, 32, tx_type, 8,
                 xd->block[i + 2].eob);
    } else {
      vp9_inverse_transform_b_8x8(&blockd[i + 2].dqcoeff[0],
                                  &blockd[i].diff[0], 32);
    }
  }
}

void vp9_inverse_transform_mbuv_8x8(MACROBLOCKD *xd) {
  int i;
  BLOCKD *blockd = xd->block;

  for (i = 16; i < 24; i += 4) {
    vp9_inverse_transform_b_8x8(&blockd[i].dqcoeff[0],
                                &blockd[i].diff[0], 16);
  }
}

void vp9_inverse_transform_mb_8x8(MACROBLOCKD *xd) {
  vp9_inverse_transform_mby_8x8(xd);
  vp9_inverse_transform_mbuv_8x8(xd);
}

void vp9_inverse_transform_b_16x16(int16_t *input_dqcoeff,
                                   int16_t *output_coeff, int pitch) {
  vp9_short_idct16x16(input_dqcoeff, output_coeff, pitch);
}

void vp9_inverse_transform_mby_16x16(MACROBLOCKD *xd) {
  BLOCKD *bd = &xd->block[0];
  TX_TYPE tx_type = get_tx_type_16x16(xd, bd);
  if (tx_type != DCT_DCT) {
    vp9_ihtllm(bd->dqcoeff, bd->diff, 32, tx_type, 16, bd->eob);
  } else {
    vp9_inverse_transform_b_16x16(&xd->block[0].dqcoeff[0],
                                  &xd->block[0].diff[0], 32);
  }
}

void vp9_inverse_transform_mb_16x16(MACROBLOCKD *xd) {
  vp9_inverse_transform_mby_16x16(xd);
  vp9_inverse_transform_mbuv_8x8(xd);
}

void vp9_inverse_transform_sby_32x32(SUPERBLOCKD *xd_sb) {
  vp9_short_idct32x32(xd_sb->dqcoeff, xd_sb->diff, 64);
}

void vp9_inverse_transform_sbuv_16x16(SUPERBLOCKD *xd_sb) {
  vp9_inverse_transform_b_16x16(xd_sb->dqcoeff + 1024,
                                xd_sb->diff + 1024, 32);
  vp9_inverse_transform_b_16x16(xd_sb->dqcoeff + 1280,
                                xd_sb->diff + 1280, 32);
}
