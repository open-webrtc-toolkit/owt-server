/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_ENCODER_VP9_ENCODEMB_H_
#define VP9_ENCODER_VP9_ENCODEMB_H_

#include "./vpx_config.h"
#include "vp9/encoder/vp9_block.h"

typedef struct {
  MB_PREDICTION_MODE mode;
  MV_REFERENCE_FRAME ref_frame;
  MV_REFERENCE_FRAME second_ref_frame;
} MODE_DEFINITION;


#include "vp9/encoder/vp9_onyx_int.h"
struct VP9_ENCODER_RTCD;
void vp9_encode_inter16x16(MACROBLOCK *x);

void vp9_transform_mbuv_4x4(MACROBLOCK *x);
void vp9_transform_mby_4x4(MACROBLOCK *x);

void vp9_optimize_mby_4x4(MACROBLOCK *x);
void vp9_optimize_mbuv_4x4(MACROBLOCK *x);
void vp9_encode_inter16x16y(MACROBLOCK *x);

void vp9_transform_mb_8x8(MACROBLOCK *mb);
void vp9_transform_mby_8x8(MACROBLOCK *x);
void vp9_transform_mbuv_8x8(MACROBLOCK *x);
void vp9_build_dcblock_8x8(MACROBLOCK *b);
void vp9_optimize_mby_8x8(MACROBLOCK *x);
void vp9_optimize_mbuv_8x8(MACROBLOCK *x);

void vp9_transform_mb_16x16(MACROBLOCK *mb);
void vp9_transform_mby_16x16(MACROBLOCK *x);
void vp9_optimize_mby_16x16(MACROBLOCK *x);

void vp9_transform_sby_32x32(MACROBLOCK *x);
void vp9_transform_sbuv_16x16(MACROBLOCK *x);

void vp9_fidct_mb(MACROBLOCK *x);

void vp9_subtract_4b_c(BLOCK *be, BLOCKD *bd, int pitch);

void vp9_subtract_mbuv_s_c(int16_t *diff, const uint8_t *usrc,
                           const uint8_t *vsrc, int src_stride,
                           const uint8_t *upred,
                           const uint8_t *vpred, int dst_stride);
void vp9_subtract_mby_s_c(int16_t *diff, const uint8_t *src,
                          int src_stride, const uint8_t *pred,
                          int dst_stride);
void vp9_subtract_sby_s_c(int16_t *diff, const uint8_t *src, int src_stride,
                          const uint8_t *pred, int dst_stride);
void vp9_subtract_sbuv_s_c(int16_t *diff, const uint8_t *usrc,
                           const uint8_t *vsrc, int src_stride,
                           const uint8_t *upred,
                           const uint8_t *vpred, int dst_stride);

#endif  // VP9_ENCODER_VP9_ENCODEMB_H_
