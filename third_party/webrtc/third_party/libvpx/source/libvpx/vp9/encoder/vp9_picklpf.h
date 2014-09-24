/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_ENCODER_VP9_PICKLPF_H_
#define VP9_ENCODER_VP9_PICKLPF_H_

struct yv12_buffer_config;
struct VP9_COMP;

extern void vp9_pick_filter_level_fast(struct yv12_buffer_config *sd,
                                       struct VP9_COMP *cpi);

extern void vp9_set_alt_lf_level(struct VP9_COMP *cpi, int filt_val);

extern void vp9_pick_filter_level(struct yv12_buffer_config *sd,
                                  struct VP9_COMP *cpi);

#endif  // VP9_ENCODER_VP9_PICKLPF_H_
