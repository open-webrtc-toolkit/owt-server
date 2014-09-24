/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_RECONINTRA_H_
#define VP9_COMMON_VP9_RECONINTRA_H_

#include "vpx/vpx_integer.h"
#include "vp9/common/vp9_blockd.h"

extern void vp9_recon_intra_mbuv(MACROBLOCKD *xd);
extern B_PREDICTION_MODE vp9_find_dominant_direction(uint8_t *ptr,
                                                     int stride, int n);
extern B_PREDICTION_MODE vp9_find_bpred_context(BLOCKD *x);
#if CONFIG_COMP_INTERINTRA_PRED
extern void vp9_build_interintra_16x16_predictors_mb(MACROBLOCKD *xd,
                                                     uint8_t *ypred,
                                                     uint8_t *upred,
                                                     uint8_t *vpred,
                                                     int ystride,
                                                     int uvstride);
extern void vp9_build_interintra_16x16_predictors_mby(MACROBLOCKD *xd,
                                                      uint8_t *ypred,
                                                      int ystride);
extern void vp9_build_interintra_16x16_predictors_mbuv(MACROBLOCKD *xd,
                                                       uint8_t *upred,
                                                       uint8_t *vpred,
                                                       int uvstride);
#endif  // CONFIG_COMP_INTERINTRA_PRED

extern void vp9_build_interintra_32x32_predictors_sb(MACROBLOCKD *xd,
                                                     uint8_t *ypred,
                                                     uint8_t *upred,
                                                     uint8_t *vpred,
                                                     int ystride,
                                                     int uvstride);
extern void vp9_build_interintra_64x64_predictors_sb(MACROBLOCKD *xd,
                                                     uint8_t *ypred,
                                                     uint8_t *upred,
                                                     uint8_t *vpred,
                                                     int ystride,
                                                     int uvstride);

#endif  // VP9_COMMON_VP9_RECONINTRA_H_
