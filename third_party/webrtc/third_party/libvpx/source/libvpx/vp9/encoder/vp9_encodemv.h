/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_ENCODER_VP9_ENCODEMV_H_
#define VP9_ENCODER_VP9_ENCODEMV_H_

#include "vp9/encoder/vp9_onyx_int.h"

void vp9_write_nmv_probs(VP9_COMP* const, int usehp, vp9_writer* const);
void vp9_encode_nmv(vp9_writer* const w, const MV* const mv,
                    const MV* const ref, const nmv_context* const mvctx);
void vp9_encode_nmv_fp(vp9_writer* const w, const MV* const mv,
                       const MV* const ref, const nmv_context* const mvctx,
                       int usehp);
void vp9_build_nmv_cost_table(int *mvjoint,
                              int *mvcost[2],
                              const nmv_context* const mvctx,
                              int usehp,
                              int mvc_flag_v,
                              int mvc_flag_h);
void vp9_update_nmv_count(VP9_COMP *cpi, MACROBLOCK *x,
                          int_mv *best_ref_mv, int_mv *second_best_ref_mv);

void print_nmvcounts(nmv_context_counts tnmvcounts);

#endif  // VP9_ENCODER_VP9_ENCODEMV_H_
