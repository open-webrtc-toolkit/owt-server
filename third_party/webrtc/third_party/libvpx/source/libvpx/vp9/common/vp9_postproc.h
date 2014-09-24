/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_COMMON_VP9_POSTPROC_H_
#define VP9_COMMON_VP9_POSTPROC_H_

#include "vpx_ports/mem.h"
struct postproc_state {
  int           last_q;
  int           last_noise;
  char          noise[3072];
  DECLARE_ALIGNED(16, char, blackclamp[16]);
  DECLARE_ALIGNED(16, char, whiteclamp[16]);
  DECLARE_ALIGNED(16, char, bothclamp[16]);
};
#include "vp9/common/vp9_onyxc_int.h"
#include "vp9/common/vp9_ppflags.h"
int vp9_post_proc_frame(struct VP9Common *oci, YV12_BUFFER_CONFIG *dest,
                        vp9_ppflags_t *flags);


void vp9_de_noise(YV12_BUFFER_CONFIG         *source,
                  YV12_BUFFER_CONFIG         *post,
                  int                         q,
                  int                         low_var_thresh,
                  int                         flag);

void vp9_deblock(YV12_BUFFER_CONFIG         *source,
                 YV12_BUFFER_CONFIG         *post,
                 int                         q,
                 int                         low_var_thresh,
                 int                         flag);

#endif  // VP9_COMMON_VP9_POSTPROC_H_
