/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_DECODER_VP9_DECODEMV_H_
#define VP9_DECODER_VP9_DECODEMV_H_

#include "vp9/decoder/vp9_onyxd_int.h"

void vp9_decode_mb_mode_mv(VP9D_COMP* const pbi,
                           MACROBLOCKD* const xd,
                           int mb_row,
                           int mb_col,
                           BOOL_DECODER* const bc);
void vp9_decode_mode_mvs_init(VP9D_COMP* const pbi, BOOL_DECODER* const bc);

#endif  // VP9_DECODER_VP9_DECODEMV_H_
