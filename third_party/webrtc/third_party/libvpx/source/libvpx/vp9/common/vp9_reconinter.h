/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_RECONINTER_H_
#define VP9_COMMON_VP9_RECONINTER_H_

#include "vpx/vpx_integer.h"
#include "vp9/common/vp9_onyxc_int.h"

extern void vp9_build_1st_inter16x16_predictors_mby(MACROBLOCKD *xd,
                                                    uint8_t *dst_y,
                                                    int dst_ystride,
                                                    int clamp_mvs);

extern void vp9_build_1st_inter16x16_predictors_mbuv(MACROBLOCKD *xd,
                                                     uint8_t *dst_u,
                                                     uint8_t *dst_v,
                                                     int dst_uvstride);

extern void vp9_build_1st_inter16x16_predictors_mb(MACROBLOCKD *xd,
                                                   uint8_t *dst_y,
                                                   uint8_t *dst_u,
                                                   uint8_t *dst_v,
                                                   int dst_ystride,
                                                   int dst_uvstride);

extern void vp9_build_2nd_inter16x16_predictors_mby(MACROBLOCKD *xd,
                                                    uint8_t *dst_y,
                                                    int dst_ystride);

extern void vp9_build_2nd_inter16x16_predictors_mbuv(MACROBLOCKD *xd,
                                                     uint8_t *dst_u,
                                                     uint8_t *dst_v,
                                                     int dst_uvstride);

extern void vp9_build_2nd_inter16x16_predictors_mb(MACROBLOCKD *xd,
                                                   uint8_t *dst_y,
                                                   uint8_t *dst_u,
                                                   uint8_t *dst_v,
                                                   int dst_ystride,
                                                   int dst_uvstride);

extern void vp9_build_inter32x32_predictors_sb(MACROBLOCKD *x,
                                               uint8_t *dst_y,
                                               uint8_t *dst_u,
                                               uint8_t *dst_v,
                                               int dst_ystride,
                                               int dst_uvstride);

extern void vp9_build_inter64x64_predictors_sb(MACROBLOCKD *x,
                                               uint8_t *dst_y,
                                               uint8_t *dst_u,
                                               uint8_t *dst_v,
                                               int dst_ystride,
                                               int dst_uvstride);

extern void vp9_build_inter_predictors_mb(MACROBLOCKD *xd);

extern void vp9_build_inter_predictors_b(BLOCKD *d, int pitch,
                                         vp9_subpix_fn_t sppf);

extern void vp9_build_2nd_inter_predictors_b(BLOCKD *d, int pitch,
                                             vp9_subpix_fn_t sppf);

extern void vp9_build_inter_predictors4b(MACROBLOCKD *xd, BLOCKD *d,
                                         int pitch);

extern void vp9_build_2nd_inter_predictors4b(MACROBLOCKD *xd,
                                             BLOCKD *d, int pitch);

extern void vp9_build_inter4x4_predictors_mbuv(MACROBLOCKD *xd);

extern void vp9_setup_interp_filters(MACROBLOCKD *xd,
                                     INTERPOLATIONFILTERTYPE filter,
                                     VP9_COMMON *cm);

#endif  // VP9_COMMON_VP9_RECONINTER_H_
