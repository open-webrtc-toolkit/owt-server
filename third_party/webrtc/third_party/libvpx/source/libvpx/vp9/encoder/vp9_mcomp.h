/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_ENCODER_VP9_MCOMP_H_
#define VP9_ENCODER_VP9_MCOMP_H_

#include "vp9/encoder/vp9_block.h"
#include "vp9/encoder/vp9_variance.h"

#ifdef ENTROPY_STATS
extern void init_mv_ref_counts();
extern void accum_mv_refs(MB_PREDICTION_MODE, const int near_mv_ref_cts[4]);
void print_mode_context(void);
#endif


#define MAX_MVSEARCH_STEPS 8                                    // The maximum number of steps in a step search given the largest allowed initial step
#define MAX_FULL_PEL_VAL ((1 << (MAX_MVSEARCH_STEPS)) - 1)      // Max full pel mv specified in 1 pel units
#define MAX_FIRST_STEP (1 << (MAX_MVSEARCH_STEPS-1))            // Maximum size of the first step in full pel units

extern void vp9_clamp_mv_min_max(MACROBLOCK *x, int_mv *ref_mv);
extern int vp9_mv_bit_cost(int_mv *mv, int_mv *ref, int *mvjcost,
                           int *mvcost[2], int Weight, int ishp);
extern void vp9_init_dsmotion_compensation(MACROBLOCK *x, int stride);
extern void vp9_init3smotion_compensation(MACROBLOCK *x,  int stride);
// Runs sequence of diamond searches in smaller steps for RD
struct VP9_COMP;
int vp9_full_pixel_diamond(struct VP9_COMP *cpi, MACROBLOCK *x, BLOCK *b,
                           BLOCKD *d, int_mv *mvp_full, int step_param,
                           int sadpb, int further_steps, int do_refine,
                           vp9_variance_fn_ptr_t *fn_ptr,
                           int_mv *ref_mv, int_mv *dst_mv);

extern int vp9_hex_search
(
  MACROBLOCK *x,
  BLOCK *b,
  BLOCKD *d,
  int_mv *ref_mv,
  int_mv *best_mv,
  int search_param,
  int error_per_bit,
  const vp9_variance_fn_ptr_t *vf,
  int *mvjsadcost, int *mvsadcost[2],
  int *mvjcost, int *mvcost[2],
  int_mv *center_mv
);

typedef int (fractional_mv_step_fp) (MACROBLOCK *x, BLOCK *b, BLOCKD *d, int_mv
  *bestmv, int_mv *ref_mv, int error_per_bit, const vp9_variance_fn_ptr_t *vfp,
  int *mvjcost, int *mvcost[2], int *distortion, unsigned int *sse);
extern fractional_mv_step_fp vp9_find_best_sub_pixel_step_iteratively;
extern fractional_mv_step_fp vp9_find_best_sub_pixel_step;
extern fractional_mv_step_fp vp9_find_best_half_pixel_step;

typedef int (*vp9_full_search_fn_t)(MACROBLOCK *x, BLOCK *b, BLOCKD *d,
                                    int_mv *ref_mv, int sad_per_bit,
                                    int distance, vp9_variance_fn_ptr_t *fn_ptr,
                                    int *mvjcost, int *mvcost[2],
                                    int_mv *center_mv);

typedef int (*vp9_refining_search_fn_t)(MACROBLOCK *x, BLOCK *b, BLOCKD *d,
                                        int_mv *ref_mv, int sad_per_bit,
                                        int distance,
                                        vp9_variance_fn_ptr_t *fn_ptr,
                                        int *mvjcost, int *mvcost[2],
                                        int_mv *center_mv);

typedef int (*vp9_diamond_search_fn_t)(MACROBLOCK *x, BLOCK *b, BLOCKD *d,
                                       int_mv *ref_mv, int_mv *best_mv,
                                       int search_param, int sad_per_bit,
                                       int *num00,
                                       vp9_variance_fn_ptr_t *fn_ptr,
                                       int *mvjcost, int *mvcost[2],
                                       int_mv *center_mv);


#endif  // VP9_ENCODER_VP9_MCOMP_H_
