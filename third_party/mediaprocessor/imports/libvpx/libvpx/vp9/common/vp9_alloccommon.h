/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_COMMON_VP9_ALLOCCOMMON_H_
#define VP9_COMMON_VP9_ALLOCCOMMON_H_

#include "vp9/common/vp9_onyxc_int.h"

void vp9_create_common(VP9_COMMON *oci);
void vp9_remove_common(VP9_COMMON *oci);
void vp9_de_alloc_frame_buffers(VP9_COMMON *oci);
int vp9_alloc_frame_buffers(VP9_COMMON *oci, int width, int height);
void vp9_setup_version(VP9_COMMON *oci);

void vp9_update_mode_info_border(VP9_COMMON *cpi, MODE_INFO *mi_base);
void vp9_update_mode_info_in_image(VP9_COMMON *cpi, MODE_INFO *mi);

#endif  // VP9_COMMON_VP9_ALLOCCOMMON_H_
