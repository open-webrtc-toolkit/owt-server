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
#include "vpx_ports/x86.h"
#include "vp9/decoder/vp9_onyxd_int.h"

#if HAVE_MMX
void vp9_dequantize_b_impl_mmx(short *sq, short *dq, short *q);

void vp9_dequantize_b_mmx(BLOCKD *d) {
  short *sq = (short *) d->qcoeff;
  short *dq = (short *) d->dqcoeff;
  short *q = (short *) d->dequant;
  vp9_dequantize_b_impl_mmx(sq, dq, q);
}
#endif


