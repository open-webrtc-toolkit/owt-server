/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_ENCODER_VP9_ENCODEFRAME_H_
#define VP9_ENCODER_VP9_ENCODEFRAME_H_

struct macroblock;

extern void vp9_build_block_offsets(struct macroblock *x);

extern void vp9_setup_block_ptrs(struct macroblock *x);

#endif  // VP9_ENCODER_VP9_ENCODEFRAME_H_
