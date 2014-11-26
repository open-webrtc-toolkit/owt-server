/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_ENCODER_VP9_RDOPT_H_
#define VP9_ENCODER_VP9_RDOPT_H_

#define RDCOST(RM,DM,R,D) ( ((128+((int64_t)R)*(RM)) >> 8) + ((int64_t)DM)*(D) )
#define RDCOST_8x8(RM,DM,R,D) ( ((128+((int64_t)R)*(RM)) >> 8) + ((int64_t)DM)*(D) )

void vp9_initialize_rd_consts(VP9_COMP *cpi, int qindex);

void vp9_initialize_me_consts(VP9_COMP *cpi, int qindex);

void vp9_rd_pick_intra_mode(VP9_COMP *cpi, MACROBLOCK *x,
                            int *r, int *d);

void vp9_rd_pick_intra_mode_sb32(VP9_COMP *cpi, MACROBLOCK *x,
                                 int *r, int *d);

void vp9_rd_pick_intra_mode_sb64(VP9_COMP *cpi, MACROBLOCK *x,
                                 int *r, int *d);

void vp9_pick_mode_inter_macroblock(VP9_COMP *cpi, MACROBLOCK *x,
                                    int mb_row, int mb_col,
                                    int *r, int *d);

int64_t vp9_rd_pick_inter_mode_sb32(VP9_COMP *cpi, MACROBLOCK *x,
                                    int mb_row, int mb_col,
                                    int *r, int *d);

int64_t vp9_rd_pick_inter_mode_sb64(VP9_COMP *cpi, MACROBLOCK *x,
                                    int mb_row, int mb_col,
                                    int *r, int *d);

void vp9_init_me_luts();

void vp9_set_mbmode_and_mvs(MACROBLOCK *x,
                            MB_PREDICTION_MODE mb, int_mv *mv);

#endif  // VP9_ENCODER_VP9_RDOPT_H_
