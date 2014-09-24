/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9/common/vp9_onyxc_int.h"
#include "vp9/common/vp9_blockd.h"

#ifndef VP9_COMMON_VP9_MVREF_COMMON_H_
#define VP9_COMMON_VP9_MVREF_COMMON_H_

void vp9_find_mv_refs(MACROBLOCKD *xd,
                      MODE_INFO *here,
                      MODE_INFO *lf_here,
                      MV_REFERENCE_FRAME ref_frame,
                      int_mv *mv_ref_list,
                      int *ref_sign_bias);

#endif  // VP9_COMMON_VP9_MVREF_COMMON_H_
