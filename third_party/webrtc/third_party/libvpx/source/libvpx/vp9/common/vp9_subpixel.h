/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_SUBPIXEL_H_
#define VP9_COMMON_VP9_SUBPIXEL_H_

#define prototype_subpixel_predict(sym) \
  void sym(uint8_t *src, int src_pitch, int xofst, int yofst, \
           uint8_t *dst, int dst_pitch)

typedef prototype_subpixel_predict((*vp9_subpix_fn_t));

#endif  // VP9_COMMON_VP9_SUBPIXEL_H_
