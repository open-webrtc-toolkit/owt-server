/*
 *  Copyright (c) 2011 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_FILTER_H_
#define VP9_COMMON_VP9_FILTER_H_

#include "vpx_config.h"
#include "vpx_scale/yv12config.h"
#include "vpx/vpx_integer.h"

#define BLOCK_HEIGHT_WIDTH 4
#define VP9_FILTER_WEIGHT 128
#define VP9_FILTER_SHIFT  7

#define SUBPEL_SHIFTS 16

extern const int16_t vp9_bilinear_filters[SUBPEL_SHIFTS][8];
extern const int16_t vp9_sub_pel_filters_6[SUBPEL_SHIFTS][8];
extern const int16_t vp9_sub_pel_filters_8[SUBPEL_SHIFTS][8];
extern const int16_t vp9_sub_pel_filters_8s[SUBPEL_SHIFTS][8];
extern const int16_t vp9_sub_pel_filters_8lp[SUBPEL_SHIFTS][8];

// The VP9_BILINEAR_FILTERS_2TAP macro returns a pointer to the bilinear
// filter kernel as a 2 tap filter.
#define BF_LENGTH (sizeof(vp9_bilinear_filters[0]) / \
                   sizeof(vp9_bilinear_filters[0][0]))
#define BF_OFFSET (BF_LENGTH / 2 - 1)
#define VP9_BILINEAR_FILTERS_2TAP(x) (vp9_bilinear_filters[x] + BF_OFFSET)

#endif  // VP9_COMMON_VP9_FILTER_H_
