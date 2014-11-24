/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_EXTEND_H_
#define VP9_COMMON_VP9_EXTEND_H_

#include "vpx_scale/yv12config.h"
#include "vpx/vpx_integer.h"


void vp9_copy_and_extend_frame(const YV12_BUFFER_CONFIG *src,
                               YV12_BUFFER_CONFIG *dst);

void vp9_copy_and_extend_frame_with_rect(const YV12_BUFFER_CONFIG *src,
                                         YV12_BUFFER_CONFIG *dst,
                                         int srcy, int srcx,
                                         int srch, int srcw);

void vp9_extend_mb_row(YV12_BUFFER_CONFIG *buf,
                       uint8_t *y, uint8_t *u, uint8_t *v);


#endif  // VP9_COMMON_VP9_EXTEND_H_
