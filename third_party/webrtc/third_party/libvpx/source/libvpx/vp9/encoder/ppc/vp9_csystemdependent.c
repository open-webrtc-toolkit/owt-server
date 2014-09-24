/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/encoder/vp9_variance.h"
#include "vp9/encoder/vp9_onyx_int.h"

SADFunction *vp9_sad16x16;
SADFunction *vp9_sad16x8;
SADFunction *vp9_sad8x16;
SADFunction *vp9_sad8x8;
SADFunction *vp9_sad4x4;

variance_function *vp9_variance4x4;
variance_function *vp9_variance8x8;
variance_function *vp9_variance8x16;
variance_function *vp9_variance16x8;
variance_function *vp9_variance16x16;

variance_function *vp9_mse16x16;

sub_pixel_variance_function *vp9_sub_pixel_variance4x4;
sub_pixel_variance_function *vp9_sub_pixel_variance8x8;
sub_pixel_variance_function *vp9_sub_pixel_variance8x16;
sub_pixel_variance_function *vp9_sub_pixel_variance16x8;
sub_pixel_variance_function *vp9_sub_pixel_variance16x16;

int (*vp9_block_error)(short *coeff, short *dqcoeff);
int (*vp9_mbblock_error)(MACROBLOCK *mb, int dc);

int (*vp9_mbuverror)(MACROBLOCK *mb);
unsigned int (*vp9_get_mb_ss)(short *);
void (*vp9_short_fdct4x4)(short *input, short *output, int pitch);
void (*vp9_short_fdct8x4)(short *input, short *output, int pitch);
void (*vp8_fast_fdct4x4)(short *input, short *output, int pitch);
void (*vp8_fast_fdct8x4)(short *input, short *output, int pitch);
void (*short_walsh4x4)(short *input, short *output, int pitch);

void (*vp9_subtract_b)(BLOCK *be, BLOCKD *bd, int pitch);
void (*vp9_subtract_mby)(short *diff, unsigned char *src, unsigned char *pred, int stride);
void (*vp9_subtract_mbuv)(short *diff, unsigned char *usrc, unsigned char *vsrc, unsigned char *pred, int stride);
void (*vp8_fast_quantize_b)(BLOCK *b, BLOCKD *d);

// c imports
extern int block_error_c(short *coeff, short *dqcoeff);
extern int vp9_mbblock_error_c(MACROBLOCK *mb, int dc);

extern int vp9_mbuverror_c(MACROBLOCK *mb);
extern unsigned int vp8_get8x8var_c(unsigned char *src_ptr, int  source_stride, unsigned char *ref_ptr, int  recon_stride, unsigned int *SSE, int *Sum);
extern void short_fdct4x4_c(short *input, short *output, int pitch);
extern void short_fdct8x4_c(short *input, short *output, int pitch);
extern void vp9_short_walsh4x4_c(short *input, short *output, int pitch);

extern void vp9_subtract_b_c(BLOCK *be, BLOCKD *bd, int pitch);
extern void subtract_mby_c(short *diff, unsigned char *src, unsigned char *pred, int stride);
extern void subtract_mbuv_c(short *diff, unsigned char *usrc, unsigned char *vsrc, unsigned char *pred, int stride);
extern void vp8_fast_quantize_b_c(BLOCK *b, BLOCKD *d);

extern SADFunction sad16x16_c;
extern SADFunction sad16x8_c;
extern SADFunction sad8x16_c;
extern SADFunction sad8x8_c;
extern SADFunction sad4x4_c;

extern variance_function variance16x16_c;
extern variance_function variance8x16_c;
extern variance_function variance16x8_c;
extern variance_function variance8x8_c;
extern variance_function variance4x4_c;
extern variance_function mse16x16_c;

extern sub_pixel_variance_function sub_pixel_variance4x4_c;
extern sub_pixel_variance_function sub_pixel_variance8x8_c;
extern sub_pixel_variance_function sub_pixel_variance8x16_c;
extern sub_pixel_variance_function sub_pixel_variance16x8_c;
extern sub_pixel_variance_function sub_pixel_variance16x16_c;

extern unsigned int vp9_get_mb_ss_c(short *);

// ppc
extern int vp9_block_error_ppc(short *coeff, short *dqcoeff);

extern void vp9_short_fdct4x4_ppc(short *input, short *output, int pitch);
extern void vp9_short_fdct8x4_ppc(short *input, short *output, int pitch);

extern void vp9_subtract_mby_ppc(short *diff, unsigned char *src, unsigned char *pred, int stride);
extern void vp9_subtract_mbuv_ppc(short *diff, unsigned char *usrc, unsigned char *vsrc, unsigned char *pred, int stride);

extern SADFunction vp9_sad16x16_ppc;
extern SADFunction vp9_sad16x8_ppc;
extern SADFunction vp9_sad8x16_ppc;
extern SADFunction vp9_sad8x8_ppc;
extern SADFunction vp9_sad4x4_ppc;

extern variance_function vp9_variance16x16_ppc;
extern variance_function vp9_variance8x16_ppc;
extern variance_function vp9_variance16x8_ppc;
extern variance_function vp9_variance8x8_ppc;
extern variance_function vp9_variance4x4_ppc;
extern variance_function vp9_mse16x16_ppc;

extern sub_pixel_variance_function vp9_sub_pixel_variance4x4_ppc;
extern sub_pixel_variance_function vp9_sub_pixel_variance8x8_ppc;
extern sub_pixel_variance_function vp9_sub_pixel_variance8x16_ppc;
extern sub_pixel_variance_function vp9_sub_pixel_variance16x8_ppc;
extern sub_pixel_variance_function vp9_sub_pixel_variance16x16_ppc;

extern unsigned int vp8_get8x8var_ppc(unsigned char *src_ptr, int  source_stride, unsigned char *ref_ptr, int  recon_stride, unsigned int *SSE, int *Sum);
extern unsigned int vp8_get16x16var_ppc(unsigned char *src_ptr, int  source_stride, unsigned char *ref_ptr, int  recon_stride, unsigned int *SSE, int *Sum);

void vp9_cmachine_specific_config(void) {
  // Pure C:
  vp9_mbuverror               = vp9_mbuverror_c;
  vp8_fast_quantize_b           = vp8_fast_quantize_b_c;
  vp9_short_fdct4x4            = vp9_short_fdct4x4_ppc;
  vp9_short_fdct8x4            = vp9_short_fdct8x4_ppc;
  vp8_fast_fdct4x4             = vp9_short_fdct4x4_ppc;
  vp8_fast_fdct8x4             = vp9_short_fdct8x4_ppc;
  short_walsh4x4               = vp9_short_walsh4x4_c;

  vp9_variance4x4             = vp9_variance4x4_ppc;
  vp9_variance8x8             = vp9_variance8x8_ppc;
  vp9_variance8x16            = vp9_variance8x16_ppc;
  vp9_variance16x8            = vp9_variance16x8_ppc;
  vp9_variance16x16           = vp9_variance16x16_ppc;
  vp9_mse16x16                = vp9_mse16x16_ppc;

  vp9_sub_pixel_variance4x4     = vp9_sub_pixel_variance4x4_ppc;
  vp9_sub_pixel_variance8x8     = vp9_sub_pixel_variance8x8_ppc;
  vp9_sub_pixel_variance8x16    = vp9_sub_pixel_variance8x16_ppc;
  vp9_sub_pixel_variance16x8    = vp9_sub_pixel_variance16x8_ppc;
  vp9_sub_pixel_variance16x16   = vp9_sub_pixel_variance16x16_ppc;

  vp9_get_mb_ss                 = vp9_get_mb_ss_c;

  vp9_sad16x16                = vp9_sad16x16_ppc;
  vp9_sad16x8                 = vp9_sad16x8_ppc;
  vp9_sad8x16                 = vp9_sad8x16_ppc;
  vp9_sad8x8                  = vp9_sad8x8_ppc;
  vp9_sad4x4                  = vp9_sad4x4_ppc;

  vp9_block_error              = vp9_block_error_ppc;
  vp9_mbblock_error            = vp9_mbblock_error_c;

  vp9_subtract_b               = vp9_subtract_b_c;
  vp9_subtract_mby             = vp9_subtract_mby_ppc;
  vp9_subtract_mbuv            = vp9_subtract_mbuv_ppc;
}
