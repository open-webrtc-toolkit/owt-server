/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <limits.h>
#include "vpx_mem/vpx_mem.h"
#include "vp9/encoder/vp9_segmentation.h"
#include "vp9/common/vp9_pred_common.h"
#include "vp9/common/vp9_tile_common.h"

void vp9_update_gf_useage_maps(VP9_COMP *cpi, VP9_COMMON *cm, MACROBLOCK *x) {
  int mb_row, mb_col;

  MODE_INFO *this_mb_mode_info = cm->mi;

  x->gf_active_ptr = (signed char *)cpi->gf_active_flags;

  if ((cm->frame_type == KEY_FRAME) || (cpi->refresh_golden_frame)) {
    // Reset Gf useage monitors
    vpx_memset(cpi->gf_active_flags, 1, (cm->mb_rows * cm->mb_cols));
    cpi->gf_active_count = cm->mb_rows * cm->mb_cols;
  } else {
    // for each macroblock row in image
    for (mb_row = 0; mb_row < cm->mb_rows; mb_row++) {
      // for each macroblock col in image
      for (mb_col = 0; mb_col < cm->mb_cols; mb_col++) {

        // If using golden then set GF active flag if not already set.
        // If using last frame 0,0 mode then leave flag as it is
        // else if using non 0,0 motion or intra modes then clear
        // flag if it is currently set
        if ((this_mb_mode_info->mbmi.ref_frame == GOLDEN_FRAME) ||
            (this_mb_mode_info->mbmi.ref_frame == ALTREF_FRAME)) {
          if (*(x->gf_active_ptr) == 0) {
            *(x->gf_active_ptr) = 1;
            cpi->gf_active_count++;
          }
        } else if ((this_mb_mode_info->mbmi.mode != ZEROMV) &&
                   *(x->gf_active_ptr)) {
          *(x->gf_active_ptr) = 0;
          cpi->gf_active_count--;
        }

        x->gf_active_ptr++;          // Step onto next entry
        this_mb_mode_info++;         // skip to next mb

      }

      // this is to account for the border
      this_mb_mode_info++;
    }
  }
}

void vp9_enable_segmentation(VP9_PTR ptr) {
  VP9_COMP *cpi = (VP9_COMP *)(ptr);

  // Set the appropriate feature bit
  cpi->mb.e_mbd.segmentation_enabled = 1;
  cpi->mb.e_mbd.update_mb_segmentation_map = 1;
  cpi->mb.e_mbd.update_mb_segmentation_data = 1;
}

void vp9_disable_segmentation(VP9_PTR ptr) {
  VP9_COMP *cpi = (VP9_COMP *)(ptr);

  // Clear the appropriate feature bit
  cpi->mb.e_mbd.segmentation_enabled = 0;
}

void vp9_set_segmentation_map(VP9_PTR ptr,
                              unsigned char *segmentation_map) {
  VP9_COMP *cpi = (VP9_COMP *)(ptr);

  // Copy in the new segmentation map
  vpx_memcpy(cpi->segmentation_map, segmentation_map,
             (cpi->common.mb_rows * cpi->common.mb_cols));

  // Signal that the map should be updated.
  cpi->mb.e_mbd.update_mb_segmentation_map = 1;
  cpi->mb.e_mbd.update_mb_segmentation_data = 1;
}

void vp9_set_segment_data(VP9_PTR ptr,
                          signed char *feature_data,
                          unsigned char abs_delta) {
  VP9_COMP *cpi = (VP9_COMP *)(ptr);

  cpi->mb.e_mbd.mb_segment_abs_delta = abs_delta;

  vpx_memcpy(cpi->mb.e_mbd.segment_feature_data, feature_data,
             sizeof(cpi->mb.e_mbd.segment_feature_data));

  // TBD ?? Set the feature mask
  // vpx_memcpy(cpi->mb.e_mbd.segment_feature_mask, 0,
  //            sizeof(cpi->mb.e_mbd.segment_feature_mask));
}

// Based on set of segment counts calculate a probability tree
static void calc_segtree_probs(MACROBLOCKD *xd,
                               int *segcounts,
                               vp9_prob *segment_tree_probs) {
  int count1, count2;

  // Total count for all segments
  count1 = segcounts[0] + segcounts[1];
  count2 = segcounts[2] + segcounts[3];

  // Work out probabilities of each segment
  segment_tree_probs[0] = get_binary_prob(count1, count2);
  segment_tree_probs[1] = get_prob(segcounts[0], count1);
  segment_tree_probs[2] = get_prob(segcounts[2], count2);
}

// Based on set of segment counts and probabilities calculate a cost estimate
static int cost_segmap(MACROBLOCKD *xd,
                       int *segcounts,
                       vp9_prob *probs) {
  int cost;
  int count1, count2;

  // Cost the top node of the tree
  count1 = segcounts[0] + segcounts[1];
  count2 = segcounts[2] + segcounts[3];
  cost = count1 * vp9_cost_zero(probs[0]) +
         count2 * vp9_cost_one(probs[0]);

  // Now add the cost of each individual segment branch
  if (count1 > 0)
    cost += segcounts[0] * vp9_cost_zero(probs[1]) +
            segcounts[1] * vp9_cost_one(probs[1]);

  if (count2 > 0)
    cost += segcounts[2] * vp9_cost_zero(probs[2]) +
            segcounts[3] * vp9_cost_one(probs[2]);

  return cost;
}

// Based on set of segment counts calculate a probability tree
static void calc_segtree_probs_pred(MACROBLOCKD *xd,
                                    int (*segcounts)[MAX_MB_SEGMENTS],
                                    vp9_prob *segment_tree_probs,
                                    vp9_prob *mod_probs) {
  int count[4];

  assert(!segcounts[0][0] && !segcounts[1][1] &&
         !segcounts[2][2] && !segcounts[3][3]);

  // Total count for all segments
  count[0] = segcounts[3][0] + segcounts[1][0] + segcounts[2][0];
  count[1] = segcounts[2][1] + segcounts[0][1] + segcounts[3][1];
  count[2] = segcounts[0][2] + segcounts[3][2] + segcounts[1][2];
  count[3] = segcounts[1][3] + segcounts[2][3] + segcounts[0][3];

  // Work out probabilities of each segment
  segment_tree_probs[0] = get_binary_prob(count[0] + count[1],
                                          count[2] + count[3]);
  segment_tree_probs[1] = get_binary_prob(count[0], count[1]);
  segment_tree_probs[2] = get_binary_prob(count[2], count[3]);

  // now work out modified counts that the decoder would have
  count[0] =        segment_tree_probs[0]  *        segment_tree_probs[1];
  count[1] =        segment_tree_probs[0]  * (256 - segment_tree_probs[1]);
  count[2] = (256 - segment_tree_probs[0]) *        segment_tree_probs[2];
  count[3] = (256 - segment_tree_probs[0]) * (256 - segment_tree_probs[2]);

  // Work out modified probabilties depending on what segment was predicted
  mod_probs[0] = get_binary_prob(count[1], count[2] + count[3]);
  mod_probs[1] = get_binary_prob(count[0], count[2] + count[3]);
  mod_probs[2] = get_binary_prob(count[0] + count[1], count[3]);
  mod_probs[3] = get_binary_prob(count[0] + count[1], count[2]);
}

// Based on set of segment counts and probabilities calculate a cost estimate
static int cost_segmap_pred(MACROBLOCKD *xd,
                            int (*segcounts)[MAX_MB_SEGMENTS],
                            vp9_prob *probs, vp9_prob *mod_probs) {
  int pred_seg, cost = 0;

  for (pred_seg = 0; pred_seg < MAX_MB_SEGMENTS; pred_seg++) {
    int count1, count2;

    // Cost the top node of the tree
    count1 = segcounts[pred_seg][0] + segcounts[pred_seg][1];
    count2 = segcounts[pred_seg][2] + segcounts[pred_seg][3];
    cost += count1 * vp9_cost_zero(mod_probs[pred_seg]) +
            count2 * vp9_cost_one(mod_probs[pred_seg]);

    // Now add the cost of each individual segment branch
    if (pred_seg >= 2 && count1) {
      cost += segcounts[pred_seg][0] * vp9_cost_zero(probs[1]) +
              segcounts[pred_seg][1] * vp9_cost_one(probs[1]);
    } else if (pred_seg < 2 && count2 > 0) {
      cost += segcounts[pred_seg][2] * vp9_cost_zero(probs[2]) +
              segcounts[pred_seg][3] * vp9_cost_one(probs[2]);
    }
  }

  return cost;
}

static void count_segs(VP9_COMP *cpi,
                       MODE_INFO *mi,
                       int *no_pred_segcounts,
                       int (*temporal_predictor_count)[2],
                       int (*t_unpred_seg_counts)[MAX_MB_SEGMENTS],
                       int mb_size, int mb_row, int mb_col) {
  VP9_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &cpi->mb.e_mbd;
  const int segmap_index = mb_row * cm->mb_cols + mb_col;
  const int segment_id = mi->mbmi.segment_id;

  xd->mode_info_context = mi;
  set_mb_row(cm, xd, mb_row, mb_size);
  set_mb_col(cm, xd, mb_col, mb_size);

  // Count the number of hits on each segment with no prediction
  no_pred_segcounts[segment_id]++;

  // Temporal prediction not allowed on key frames
  if (cm->frame_type != KEY_FRAME) {
    // Test to see if the segment id matches the predicted value.
    const int pred_seg_id = vp9_get_pred_mb_segid(cm, xd, segmap_index);
    const int seg_predicted = (segment_id == pred_seg_id);

    // Get the segment id prediction context
    const int pred_context = vp9_get_pred_context(cm, xd, PRED_SEG_ID);

    // Store the prediction status for this mb and update counts
    // as appropriate
    vp9_set_pred_flag(xd, PRED_SEG_ID, seg_predicted);
    temporal_predictor_count[pred_context][seg_predicted]++;

    if (!seg_predicted)
      // Update the "unpredicted" segment count
      t_unpred_seg_counts[pred_seg_id][segment_id]++;
  }
}

void vp9_choose_segmap_coding_method(VP9_COMP *cpi) {
  VP9_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &cpi->mb.e_mbd;

  int no_pred_cost;
  int t_pred_cost = INT_MAX;

  int i;
  int tile_col, mb_row, mb_col;

  int temporal_predictor_count[PREDICTION_PROBS][2];
  int no_pred_segcounts[MAX_MB_SEGMENTS];
  int t_unpred_seg_counts[MAX_MB_SEGMENTS][MAX_MB_SEGMENTS];

  vp9_prob no_pred_tree[MB_FEATURE_TREE_PROBS];
  vp9_prob t_pred_tree[MB_FEATURE_TREE_PROBS];
  vp9_prob t_pred_tree_mod[MAX_MB_SEGMENTS];
  vp9_prob t_nopred_prob[PREDICTION_PROBS];

  const int mis = cm->mode_info_stride;
  MODE_INFO *mi_ptr, *mi;

  // Set default state for the segment tree probabilities and the
  // temporal coding probabilities
  vpx_memset(xd->mb_segment_tree_probs, 255,
             sizeof(xd->mb_segment_tree_probs));
  vpx_memset(cm->segment_pred_probs, 255,
             sizeof(cm->segment_pred_probs));

  vpx_memset(no_pred_segcounts, 0, sizeof(no_pred_segcounts));
  vpx_memset(t_unpred_seg_counts, 0, sizeof(t_unpred_seg_counts));
  vpx_memset(temporal_predictor_count, 0, sizeof(temporal_predictor_count));

  // First of all generate stats regarding how well the last segment map
  // predicts this one

  for (tile_col = 0; tile_col < cm->tile_columns; tile_col++) {
    vp9_get_tile_col_offsets(cm, tile_col);
    mi_ptr = cm->mi + cm->cur_tile_mb_col_start;
    for (mb_row = 0; mb_row < cm->mb_rows; mb_row += 4, mi_ptr += 4 * mis) {
      mi = mi_ptr;
      for (mb_col = cm->cur_tile_mb_col_start;
           mb_col < cm->cur_tile_mb_col_end; mb_col += 4, mi += 4) {
        if (mi->mbmi.sb_type == BLOCK_SIZE_SB64X64) {
          count_segs(cpi, mi, no_pred_segcounts, temporal_predictor_count,
                     t_unpred_seg_counts, 4, mb_row, mb_col);
        } else {
          for (i = 0; i < 4; i++) {
            int x_idx = (i & 1) << 1, y_idx = i & 2;
            MODE_INFO *sb_mi = mi + y_idx * mis + x_idx;

            if (mb_col + x_idx >= cm->mb_cols ||
                mb_row + y_idx >= cm->mb_rows) {
              continue;
            }

            if (sb_mi->mbmi.sb_type) {
              assert(sb_mi->mbmi.sb_type == BLOCK_SIZE_SB32X32);
              count_segs(cpi, sb_mi, no_pred_segcounts,
                         temporal_predictor_count, t_unpred_seg_counts, 2,
                         mb_row + y_idx, mb_col + x_idx);
            } else {
              int j;

              for (j = 0; j < 4; j++) {
                const int x_idx_mb = x_idx + (j & 1);
                const int y_idx_mb = y_idx + (j >> 1);
                MODE_INFO *mb_mi = mi + x_idx_mb + y_idx_mb * mis;

                if (mb_col + x_idx_mb >= cm->mb_cols ||
                    mb_row + y_idx_mb >= cm->mb_rows) {
                  continue;
                }

                assert(mb_mi->mbmi.sb_type == BLOCK_SIZE_MB16X16);
                count_segs(cpi, mb_mi, no_pred_segcounts,
                           temporal_predictor_count, t_unpred_seg_counts,
                           1, mb_row + y_idx_mb, mb_col + x_idx_mb);
              }
            }
          }
        }
      }
    }
  }

  // Work out probability tree for coding segments without prediction
  // and the cost.
  calc_segtree_probs(xd, no_pred_segcounts, no_pred_tree);
  no_pred_cost = cost_segmap(xd, no_pred_segcounts, no_pred_tree);

  // Key frames cannot use temporal prediction
  if (cm->frame_type != KEY_FRAME) {
    // Work out probability tree for coding those segments not
    // predicted using the temporal method and the cost.
    calc_segtree_probs_pred(xd, t_unpred_seg_counts, t_pred_tree,
                            t_pred_tree_mod);
    t_pred_cost = cost_segmap_pred(xd, t_unpred_seg_counts, t_pred_tree,
                                   t_pred_tree_mod);

    // Add in the cost of the signalling for each prediction context
    for (i = 0; i < PREDICTION_PROBS; i++) {
      t_nopred_prob[i] = get_binary_prob(temporal_predictor_count[i][0],
                                         temporal_predictor_count[i][1]);

      // Add in the predictor signaling cost
      t_pred_cost += (temporal_predictor_count[i][0] *
                      vp9_cost_zero(t_nopred_prob[i])) +
                     (temporal_predictor_count[i][1] *
                      vp9_cost_one(t_nopred_prob[i]));
    }
  }

  // Now choose which coding method to use.
  if (t_pred_cost < no_pred_cost) {
    cm->temporal_update = 1;
    vpx_memcpy(xd->mb_segment_tree_probs,
               t_pred_tree, sizeof(t_pred_tree));
    vpx_memcpy(xd->mb_segment_mispred_tree_probs,
               t_pred_tree_mod, sizeof(t_pred_tree_mod));
    vpx_memcpy(&cm->segment_pred_probs,
               t_nopred_prob, sizeof(t_nopred_prob));
  } else {
    cm->temporal_update = 0;
    vpx_memcpy(xd->mb_segment_tree_probs,
               no_pred_tree, sizeof(no_pred_tree));
  }
}
