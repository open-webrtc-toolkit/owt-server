/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9/common/vp9_tile_common.h"

#define MIN_TILE_WIDTH 256
#define MAX_TILE_WIDTH 4096
#define MIN_TILE_WIDTH_SBS (MIN_TILE_WIDTH >> 6)
#define MAX_TILE_WIDTH_SBS (MAX_TILE_WIDTH >> 6)

static void vp9_get_tile_offsets(VP9_COMMON *cm, int *min_tile_off,
                                 int *max_tile_off, int tile_idx,
                                 int log2_n_tiles, int n_mbs) {
  const int n_sbs = (n_mbs + 3) >> 2;
  const int sb_off1 =  (tile_idx      * n_sbs) >> log2_n_tiles;
  const int sb_off2 = ((tile_idx + 1) * n_sbs) >> log2_n_tiles;

  *min_tile_off = MIN(sb_off1 << 2, n_mbs);
  *max_tile_off = MIN(sb_off2 << 2, n_mbs);
}

void vp9_get_tile_col_offsets(VP9_COMMON *cm, int tile_col_idx) {
  cm->cur_tile_col_idx = tile_col_idx;
  vp9_get_tile_offsets(cm, &cm->cur_tile_mb_col_start,
                       &cm->cur_tile_mb_col_end, tile_col_idx,
                       cm->log2_tile_columns, cm->mb_cols);
}

void vp9_get_tile_row_offsets(VP9_COMMON *cm, int tile_row_idx) {
  cm->cur_tile_row_idx = tile_row_idx;
  vp9_get_tile_offsets(cm, &cm->cur_tile_mb_row_start,
                       &cm->cur_tile_mb_row_end, tile_row_idx,
                       cm->log2_tile_rows, cm->mb_rows);
}


void vp9_get_tile_n_bits(VP9_COMMON *cm, int *min_log2_n_tiles_ptr,
                         int *delta_log2_n_tiles) {
  const int sb_cols = (cm->mb_cols + 3) >> 2;
  int min_log2_n_tiles, max_log2_n_tiles;

  for (max_log2_n_tiles = 0;
       (sb_cols >> max_log2_n_tiles) >= MIN_TILE_WIDTH_SBS;
       max_log2_n_tiles++) {}
  for (min_log2_n_tiles = 0;
       (MAX_TILE_WIDTH_SBS << min_log2_n_tiles) < sb_cols;
       min_log2_n_tiles++) {}

  *min_log2_n_tiles_ptr = min_log2_n_tiles;
  *delta_log2_n_tiles = max_log2_n_tiles - min_log2_n_tiles;
}
