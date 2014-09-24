/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_ENCODER_VP9_QUANTIZE_H_
#define VP9_ENCODER_VP9_QUANTIZE_H_

#include "vp9/encoder/vp9_block.h"

#define prototype_quantize_block(sym) \
  void (sym)(BLOCK *b,BLOCKD *d)

#define prototype_quantize_block_pair(sym) \
  void (sym)(BLOCK *b1, BLOCK *b2, BLOCKD *d1, BLOCKD *d2)

#define prototype_quantize_mb(sym) \
  void (sym)(MACROBLOCK *x)

#if ARCH_X86 || ARCH_X86_64
#include "x86/vp9_quantize_x86.h"
#endif

#define prototype_quantize_block_type(sym) \
  void (sym)(BLOCK *b, BLOCKD *d, TX_TYPE type)
extern prototype_quantize_block_type(vp9_ht_quantize_b_4x4);

#ifndef vp9_quantize_quantb_4x4
#define vp9_quantize_quantb_4x4 vp9_regular_quantize_b_4x4
#endif
extern prototype_quantize_block(vp9_quantize_quantb_4x4);

#ifndef vp9_quantize_quantb_4x4_pair
#define vp9_quantize_quantb_4x4_pair vp9_regular_quantize_b_4x4_pair
#endif
extern prototype_quantize_block_pair(vp9_quantize_quantb_4x4_pair);

#ifndef vp9_quantize_quantb_8x8
#define vp9_quantize_quantb_8x8 vp9_regular_quantize_b_8x8
#endif
extern prototype_quantize_block(vp9_quantize_quantb_8x8);

#ifndef vp9_quantize_quantb_16x16
#define vp9_quantize_quantb_16x16 vp9_regular_quantize_b_16x16
#endif
extern prototype_quantize_block(vp9_quantize_quantb_16x16);

#ifndef vp9_quantize_quantb_2x2
#define vp9_quantize_quantb_2x2 vp9_regular_quantize_b_2x2
#endif
extern prototype_quantize_block(vp9_quantize_quantb_2x2);

#ifndef vp9_quantize_mb_4x4
#define vp9_quantize_mb_4x4 vp9_quantize_mb_4x4_c
#endif
extern prototype_quantize_mb(vp9_quantize_mb_4x4);
void vp9_quantize_mb_8x8(MACROBLOCK *x);

#ifndef vp9_quantize_mbuv_4x4
#define vp9_quantize_mbuv_4x4 vp9_quantize_mbuv_4x4_c
#endif
extern prototype_quantize_mb(vp9_quantize_mbuv_4x4);

#ifndef vp9_quantize_mby_4x4
#define vp9_quantize_mby_4x4 vp9_quantize_mby_4x4_c
#endif
extern prototype_quantize_mb(vp9_quantize_mby_4x4);

extern prototype_quantize_mb(vp9_quantize_mby_8x8);
extern prototype_quantize_mb(vp9_quantize_mbuv_8x8);

void vp9_quantize_mb_16x16(MACROBLOCK *x);
extern prototype_quantize_block(vp9_quantize_quantb_16x16);
extern prototype_quantize_mb(vp9_quantize_mby_16x16);

void vp9_quantize_sby_32x32(MACROBLOCK *x);
void vp9_quantize_sbuv_16x16(MACROBLOCK *x);

struct VP9_COMP;

extern void vp9_set_quantizer(struct VP9_COMP *cpi, int Q);

extern void vp9_frame_init_quantizer(struct VP9_COMP *cpi);

extern void vp9_update_zbin_extra(struct VP9_COMP *cpi, MACROBLOCK *x);

extern void vp9_mb_init_quantizer(struct VP9_COMP *cpi, MACROBLOCK *x);

extern void vp9_init_quantizer(struct VP9_COMP *cpi);

#endif  // VP9_ENCODER_VP9_QUANTIZE_H_
