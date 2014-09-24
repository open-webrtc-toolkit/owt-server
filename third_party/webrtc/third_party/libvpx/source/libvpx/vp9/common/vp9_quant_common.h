/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_QUANT_COMMON_H_
#define VP9_COMMON_VP9_QUANT_COMMON_H_

#include "string.h"
#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_onyxc_int.h"

extern void vp9_init_quant_tables(void);
extern int vp9_ac_yquant(int QIndex);
extern int vp9_dc_quant(int QIndex, int Delta);
extern int vp9_dc2quant(int QIndex, int Delta);
extern int vp9_ac2quant(int QIndex, int Delta);
extern int vp9_dc_uv_quant(int QIndex, int Delta);
extern int vp9_ac_uv_quant(int QIndex, int Delta);

#endif  // VP9_COMMON_VP9_QUANT_COMMON_H_
