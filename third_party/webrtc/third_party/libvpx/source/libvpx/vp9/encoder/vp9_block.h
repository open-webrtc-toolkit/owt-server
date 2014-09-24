/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_ENCODER_VP9_BLOCK_H_
#define VP9_ENCODER_VP9_BLOCK_H_

#include "vp9/common/vp9_onyx.h"
#include "vp9/common/vp9_entropymv.h"
#include "vp9/common/vp9_entropy.h"
#include "vpx_ports/mem.h"
#include "vp9/common/vp9_onyxc_int.h"

// motion search site
typedef struct {
  MV mv;
  int offset;
} search_site;

typedef struct block {
  // 16 Y blocks, 4 U blocks, 4 V blocks each with 16 entries
  int16_t *src_diff;
  int16_t *coeff;

  // 16 Y blocks, 4 U blocks, 4 V blocks each with 16 entries
  int16_t *quant;
  int16_t *quant_fast;      // fast quant deprecated for now
  uint8_t *quant_shift;
  int16_t *zbin;
  int16_t *zbin_8x8;
  int16_t *zbin_16x16;
  int16_t *zbin_32x32;
  int16_t *zrun_zbin_boost;
  int16_t *zrun_zbin_boost_8x8;
  int16_t *zrun_zbin_boost_16x16;
  int16_t *zrun_zbin_boost_32x32;
  int16_t *round;

  // Zbin Over Quant value
  short zbin_extra;

  uint8_t **base_src;
  uint8_t **base_second_src;
  int src;
  int src_stride;

  int eob_max_offset;
  int eob_max_offset_8x8;
  int eob_max_offset_16x16;
  int eob_max_offset_32x32;
} BLOCK;

typedef struct {
  int count;
  struct {
    B_PREDICTION_MODE mode;
    int_mv mv;
    int_mv second_mv;
  } bmi[16];
} PARTITION_INFO;

// Structure to hold snapshot of coding context during the mode picking process
// TODO Do we need all of these?
typedef struct {
  MODE_INFO mic;
  PARTITION_INFO partition_info;
  int skip;
  int_mv best_ref_mv;
  int_mv second_best_ref_mv;
  int_mv ref_mvs[MAX_REF_FRAMES][MAX_MV_REF_CANDIDATES];
  int rate;
  int distortion;
  int64_t intra_error;
  int best_mode_index;
  int rddiv;
  int rdmult;
  int hybrid_pred_diff;
  int comp_pred_diff;
  int single_pred_diff;
  int64_t txfm_rd_diff[NB_TXFM_MODES];
} PICK_MODE_CONTEXT;

typedef struct superblock {
  DECLARE_ALIGNED(16, int16_t, src_diff[32*32+16*16*2]);
  DECLARE_ALIGNED(16, int16_t, coeff[32*32+16*16*2]);
} SUPERBLOCK;

typedef struct macroblock {
  DECLARE_ALIGNED(16, int16_t, src_diff[400]);  // 16x16 Y 8x8 U 8x8 V 4x4 2nd Y
  DECLARE_ALIGNED(16, int16_t, coeff[400]);     // 16x16 Y 8x8 U 8x8 V 4x4 2nd Y
  // 16 Y blocks, 4 U blocks, 4 V blocks,
  // 1 DC 2nd order block each with 16 entries
  BLOCK block[25];

  SUPERBLOCK sb_coeff_data;

  YV12_BUFFER_CONFIG src;

  MACROBLOCKD e_mbd;
  PARTITION_INFO *partition_info; /* work pointer */
  PARTITION_INFO *pi;   /* Corresponds to upper left visible macroblock */
  PARTITION_INFO *pip;  /* Base of allocated array */

  search_site *ss;
  int ss_count;
  int searches_per_step;

  int errorperbit;
  int sadperbit16;
  int sadperbit4;
  int rddiv;
  int rdmult;
  unsigned int *mb_activity_ptr;
  int *mb_norm_activity_ptr;
  signed int act_zbin_adj;

  int mv_best_ref_index[MAX_REF_FRAMES];

  int nmvjointcost[MV_JOINTS];
  int nmvcosts[2][MV_VALS];
  int *nmvcost[2];
  int nmvcosts_hp[2][MV_VALS];
  int *nmvcost_hp[2];
  int **mvcost;

  int nmvjointsadcost[MV_JOINTS];
  int nmvsadcosts[2][MV_VALS];
  int *nmvsadcost[2];
  int nmvsadcosts_hp[2][MV_VALS];
  int *nmvsadcost_hp[2];
  int **mvsadcost;

  int mbmode_cost[2][MB_MODE_COUNT];
  int intra_uv_mode_cost[2][MB_MODE_COUNT];
  int bmode_costs[VP9_KF_BINTRAMODES][VP9_KF_BINTRAMODES][VP9_KF_BINTRAMODES];
  int i8x8_mode_costs[MB_MODE_COUNT];
  int inter_bmode_costs[B_MODE_COUNT];
  int switchable_interp_costs[VP9_SWITCHABLE_FILTERS + 1]
                             [VP9_SWITCHABLE_FILTERS];

  // These define limits to motion vector components to prevent them
  // from extending outside the UMV borders
  int mv_col_min;
  int mv_col_max;
  int mv_row_min;
  int mv_row_max;

  int skip;

  int encode_breakout;

  // char * gf_active_ptr;
  signed char *gf_active_ptr;

  unsigned char *active_ptr;

  vp9_coeff_count token_costs[TX_SIZE_MAX_SB][BLOCK_TYPES_4X4];
  vp9_coeff_count hybrid_token_costs[TX_SIZE_MAX_SB][BLOCK_TYPES_4X4];

  int optimize;

  // Structure to hold context for each of the 4 MBs within a SB:
  // when encoded as 4 independent MBs:
  PICK_MODE_CONTEXT mb_context[4][4];
  // when 4 MBs share coding parameters:
  PICK_MODE_CONTEXT sb32_context[4];
  PICK_MODE_CONTEXT sb64_context;

  void (*vp9_short_fdct4x4)(int16_t *input, int16_t *output, int pitch);
  void (*vp9_short_fdct8x4)(int16_t *input, int16_t *output, int pitch);
  void (*short_walsh4x4)(int16_t *input, int16_t *output, int pitch);
  void (*quantize_b_4x4)(BLOCK *b, BLOCKD *d);
  void (*quantize_b_4x4_pair)(BLOCK *b1, BLOCK *b2, BLOCKD *d0, BLOCKD *d1);
  void (*vp9_short_fdct8x8)(int16_t *input, int16_t *output, int pitch);
  void (*vp9_short_fdct16x16)(int16_t *input, int16_t *output, int pitch);
  void (*short_fhaar2x2)(int16_t *input, int16_t *output, int pitch);
  void (*quantize_b_16x16)(BLOCK *b, BLOCKD *d);
  void (*quantize_b_8x8)(BLOCK *b, BLOCKD *d);
  void (*quantize_b_2x2)(BLOCK *b, BLOCKD *d);
} MACROBLOCK;

#endif  // VP9_ENCODER_VP9_BLOCK_H_
