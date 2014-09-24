/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_ENCODER_VP9_ENCODEINTRA_H_
#define VP9_ENCODER_VP9_ENCODEINTRA_H_

#include "vp9/encoder/vp9_onyx_int.h"

int vp9_encode_intra(VP9_COMP *cpi, MACROBLOCK *x, int use_16x16_pred);
void vp9_encode_intra16x16mby(MACROBLOCK *x);
void vp9_encode_intra16x16mbuv(MACROBLOCK *x);
void vp9_encode_intra4x4mby(MACROBLOCK *mb);
void vp9_encode_intra4x4block(MACROBLOCK *x, int ib);
void vp9_encode_intra8x8mby(MACROBLOCK *x);
void vp9_encode_intra8x8mbuv(MACROBLOCK *x);
void vp9_encode_intra8x8(MACROBLOCK *x, int ib);

#endif  // VP9_ENCODER_VP9_ENCODEINTRA_H_
