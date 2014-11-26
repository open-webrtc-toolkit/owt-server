/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_SWAPYV12BUFFER_H_
#define VP9_COMMON_VP9_SWAPYV12BUFFER_H_

#include "vpx_scale/yv12config.h"

void vp9_swap_yv12_buffer(YV12_BUFFER_CONFIG *new_frame,
                          YV12_BUFFER_CONFIG *last_frame);

#endif  // VP9_COMMON_VP9_SWAPYV12BUFFER_H_
