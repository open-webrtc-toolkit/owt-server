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

#ifndef VP9_COMMON_VP9_PRED_COMMON_H_
#define VP9_COMMON_VP9_PRED_COMMON_H_


// Predicted items
typedef enum {
  PRED_SEG_ID = 0,               // Segment identifier
  PRED_REF = 1,
  PRED_COMP = 2,
  PRED_MBSKIP = 3,
  PRED_SWITCHABLE_INTERP = 4
} PRED_ID;

extern unsigned char vp9_get_pred_context(const VP9_COMMON *const cm,
                                          const MACROBLOCKD *const xd,
                                          PRED_ID pred_id);

extern vp9_prob vp9_get_pred_prob(const VP9_COMMON *const cm,
                                  const MACROBLOCKD *const xd,
                                  PRED_ID pred_id);

extern const vp9_prob *vp9_get_pred_probs(const VP9_COMMON *const cm,
                                          const MACROBLOCKD *const xd,
                                          PRED_ID pred_id);

extern unsigned char vp9_get_pred_flag(const MACROBLOCKD *const xd,
                                       PRED_ID pred_id);

extern void vp9_set_pred_flag(MACROBLOCKD *const xd,
                              PRED_ID pred_id,
                              unsigned char pred_flag);


extern unsigned char vp9_get_pred_mb_segid(const VP9_COMMON *const cm,
                                           const MACROBLOCKD *const xd,
                                           int MbIndex);

extern MV_REFERENCE_FRAME vp9_get_pred_ref(const VP9_COMMON *const cm,
                                       const MACROBLOCKD *const xd);
extern void vp9_compute_mod_refprobs(VP9_COMMON *const cm);

#endif  // VP9_COMMON_VP9_PRED_COMMON_H_
