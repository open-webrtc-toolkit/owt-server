/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_DECODER_X86_VP9_IDCT_MMX_H_
#define VP9_DECODER_X86_VP9_IDCT_MMX_H_


void vp9_dequant_dc_idct_add_mmx(short *input, const short *dq,
                                 unsigned char *pred, unsigned char *dest,
                                 int pitch, int stride, int Dc);

void vp9_dc_only_idct_add_mmx(short input_dc, const unsigned char *pred_ptr,
                              unsigned char *dst_ptr, int pitch, int stride);

void vp9_dequant_idct_add_mmx(short *input, const short *dq, unsigned char *pred,
                              unsigned char *dest, int pitch, int stride);

#endif /* VP9_DECODER_X86_VP9_IDCT_MMX_H_ */
