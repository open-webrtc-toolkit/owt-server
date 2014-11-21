/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9/common/vp9_mvref_common.h"

#define MVREF_NEIGHBOURS 8

static int mb_mv_ref_search[MVREF_NEIGHBOURS][2] = {
    {0, -1}, {-1, 0}, {-1, -1}, {0, -2},
    {-2, 0}, {-1, -2}, {-2, -1}, {-2, -2}
};

static int mb_ref_distance_weight[MVREF_NEIGHBOURS] =
  { 3, 3, 2, 1, 1, 1, 1, 1 };

static int sb_mv_ref_search[MVREF_NEIGHBOURS][2] = {
    {0, -1}, {-1, 0}, {1, -1}, {-1, 1},
    {-1, -1}, {0, -2}, {-2, 0}, {-1, -2}
};

static int sb_ref_distance_weight[MVREF_NEIGHBOURS] =
  { 3, 3, 2, 2, 2, 1, 1, 1 };



static int sb64_mv_ref_search[MVREF_NEIGHBOURS][2] = {
    {0, -1}, {-1, 0}, {1, -1}, {-1, 1},
    {2, -1}, {-1, 2}, {3, -1}, {-1,-1}
};

static int sb64_ref_distance_weight[MVREF_NEIGHBOURS] =
  { 1, 1, 1, 1, 1, 1, 1, 1 };



// clamp_mv_ref
#define MV_BORDER (16 << 3) // Allow 16 pels in 1/8th pel units

static void clamp_mv_ref(const MACROBLOCKD *xd, int_mv *mv) {
  mv->as_mv.col = clamp(mv->as_mv.col, xd->mb_to_left_edge - MV_BORDER,
                                       xd->mb_to_right_edge + MV_BORDER);
  mv->as_mv.row = clamp(mv->as_mv.row, xd->mb_to_top_edge - MV_BORDER,
                                       xd->mb_to_bottom_edge + MV_BORDER);
}

// Gets a candidate refenence motion vector from the given mode info
// structure if one exists that matches the given reference frame.
static int get_matching_candidate(const MODE_INFO *candidate_mi,
                                  MV_REFERENCE_FRAME ref_frame,
                                  int_mv *c_mv) {
  if (ref_frame == candidate_mi->mbmi.ref_frame) {
    c_mv->as_int = candidate_mi->mbmi.mv[0].as_int;
  } else if (ref_frame == candidate_mi->mbmi.second_ref_frame) {
    c_mv->as_int = candidate_mi->mbmi.mv[1].as_int;
  } else {
    return 0;
  }

  return 1;
}

// Gets candidate refenence motion vector(s) from the given mode info
// structure if they exists and do NOT match the given reference frame.
static void get_non_matching_candidates(const MODE_INFO *candidate_mi,
                                        MV_REFERENCE_FRAME ref_frame,
                                        MV_REFERENCE_FRAME *c_ref_frame,
                                        int_mv *c_mv,
                                        MV_REFERENCE_FRAME *c2_ref_frame,
                                        int_mv *c2_mv) {

  c_mv->as_int = 0;
  c2_mv->as_int = 0;
  *c_ref_frame = INTRA_FRAME;
  *c2_ref_frame = INTRA_FRAME;

  // If first candidate not valid neither will be.
  if (candidate_mi->mbmi.ref_frame > INTRA_FRAME) {
    // First candidate
    if (candidate_mi->mbmi.ref_frame != ref_frame) {
      *c_ref_frame = candidate_mi->mbmi.ref_frame;
      c_mv->as_int = candidate_mi->mbmi.mv[0].as_int;
    }

    // Second candidate
    if ((candidate_mi->mbmi.second_ref_frame > INTRA_FRAME) &&
        (candidate_mi->mbmi.second_ref_frame != ref_frame) &&
        (candidate_mi->mbmi.mv[1].as_int != candidate_mi->mbmi.mv[0].as_int)) {
      *c2_ref_frame = candidate_mi->mbmi.second_ref_frame;
      c2_mv->as_int = candidate_mi->mbmi.mv[1].as_int;
    }
  }
}


// Performs mv sign inversion if indicated by the reference frame combination.
static void scale_mv(MACROBLOCKD *xd, MV_REFERENCE_FRAME this_ref_frame,
                     MV_REFERENCE_FRAME candidate_ref_frame,
                     int_mv *candidate_mv, int *ref_sign_bias) {
  // int frame_distances[MAX_REF_FRAMES];
  // int last_distance = 1;
  // int gf_distance = xd->frames_since_golden;
  // int arf_distance = xd->frames_till_alt_ref_frame;

  // Sign inversion where appropriate.
  if (ref_sign_bias[candidate_ref_frame] != ref_sign_bias[this_ref_frame]) {
    candidate_mv->as_mv.row = -candidate_mv->as_mv.row;
    candidate_mv->as_mv.col = -candidate_mv->as_mv.col;
  }

  /*
  // Scale based on frame distance if the reference frames not the same.
  frame_distances[INTRA_FRAME] = 1;   // should never be used
  frame_distances[LAST_FRAME] = 1;
  frame_distances[GOLDEN_FRAME] =
    (xd->frames_since_golden) ? xd->frames_si nce_golden : 1;
  frame_distances[ALTREF_FRAME] =
    (xd->frames_till_alt_ref_frame) ? xd->frames_till_alt_ref_frame : 1;

  if (frame_distances[this_ref_frame] &&
      frame_distances[candidate_ref_frame]) {
    candidate_mv->as_mv.row =
      (short)(((int)(candidate_mv->as_mv.row) *
               frame_distances[this_ref_frame]) /
              frame_distances[candidate_ref_frame]);

    candidate_mv->as_mv.col =
      (short)(((int)(candidate_mv->as_mv.col) *
               frame_distances[this_ref_frame]) /
              frame_distances[candidate_ref_frame]);
  }
  */
}

/*
// Adds a new candidate reference vector to the sorted list.
// If it is a repeat the weight of the existing entry is increased
// and the order of the list is resorted.
// This method of add plus sort has been deprecated for now as there is a
// further sort of the best candidates in vp9_find_best_ref_mvs() and the
// incremental benefit of both is small. If the decision is made to remove
// the sort in vp9_find_best_ref_mvs() for performance reasons then it may be
// worth re-instating some sort of list reordering by weight here.
//
static void addmv_and_shuffle(
  int_mv *mv_list,
  int *mv_scores,
  int *refmv_count,
  int_mv candidate_mv,
  int weight
) {

  int i;
  int insert_point;
  int duplicate_found = FALSE;

  // Check for duplicates. If there is one increase its score.
  // We only compare vs the current top candidates.
  insert_point = (*refmv_count < (MAX_MV_REF_CANDIDATES - 1))
                 ? *refmv_count : (MAX_MV_REF_CANDIDATES - 1);

  i = insert_point;
  if (*refmv_count > i)
    i++;
  while (i > 0) {
    i--;
    if (candidate_mv.as_int == mv_list[i].as_int) {
      duplicate_found = TRUE;
      mv_scores[i] += weight;
      break;
    }
  }

  // If no duplicate and the new candidate is good enough then add it.
  if (!duplicate_found ) {
    if (weight > mv_scores[insert_point]) {
      mv_list[insert_point].as_int = candidate_mv.as_int;
      mv_scores[insert_point] = weight;
      i = insert_point;
    }
    (*refmv_count)++;
  }

  // Reshuffle the list so that highest scoring mvs at the top.
  while (i > 0) {
    if (mv_scores[i] > mv_scores[i-1]) {
      int tmp_score = mv_scores[i-1];
      int_mv tmp_mv = mv_list[i-1];

      mv_scores[i-1] = mv_scores[i];
      mv_list[i-1] = mv_list[i];
      mv_scores[i] = tmp_score;
      mv_list[i] = tmp_mv;
      i--;
    } else
      break;
  }
}
*/

// Adds a new candidate reference vector to the list.
// The mv is thrown out if it is already in the list.
// Unlike the addmv_and_shuffle() this does not reorder the list
// but assumes that candidates are added in the order most likely to
// match distance and reference frame bias.
static void add_candidate_mv(int_mv *mv_list,  int *mv_scores,
                             int *candidate_count, int_mv candidate_mv,
                             int weight) {
  int i;

  // Make sure we dont insert off the end of the list
  const int insert_point = MIN(*candidate_count, MAX_MV_REF_CANDIDATES - 1);

  // Look for duplicates
  for (i = 0; i <= insert_point; ++i) {
    if (candidate_mv.as_int == mv_list[i].as_int)
      break;
  }

  // Add the candidate. If the list is already full it is only desirable that
  // it should overwrite if it has a higher weight than the last entry.
  if (i >= insert_point && weight > mv_scores[insert_point]) {
    mv_list[insert_point].as_int = candidate_mv.as_int;
    mv_scores[insert_point] = weight;
    *candidate_count += (*candidate_count < MAX_MV_REF_CANDIDATES);
  }
}

// This function searches the neighbourhood of a given MB/SB and populates a
// list of candidate reference vectors.
//
void vp9_find_mv_refs(VP9_COMMON *cm, MACROBLOCKD *xd, MODE_INFO *here,
                      MODE_INFO *lf_here, MV_REFERENCE_FRAME ref_frame,
                      int_mv *mv_ref_list, int *ref_sign_bias) {
  int i;
  MODE_INFO *candidate_mi;
  MB_MODE_INFO * mbmi = &xd->mode_info_context->mbmi;
  int_mv candidate_mvs[MAX_MV_REF_CANDIDATES];
  int_mv c_refmv;
  int_mv c2_refmv;
  MV_REFERENCE_FRAME c_ref_frame;
  MV_REFERENCE_FRAME c2_ref_frame;
  int candidate_scores[MAX_MV_REF_CANDIDATES];
  int refmv_count = 0;
  int split_count = 0;
  int (*mv_ref_search)[2];
  int *ref_distance_weight;
  int zero_seen = FALSE;
  const int mb_col = (-xd->mb_to_left_edge) >> 7;

  // Blank the reference vector lists and other local structures.
  vpx_memset(mv_ref_list, 0, sizeof(int_mv) * MAX_MV_REF_CANDIDATES);
  vpx_memset(candidate_mvs, 0, sizeof(int_mv) * MAX_MV_REF_CANDIDATES);
  vpx_memset(candidate_scores, 0, sizeof(candidate_scores));

  if (mbmi->sb_type == BLOCK_SIZE_SB64X64) {
    mv_ref_search = sb64_mv_ref_search;
    ref_distance_weight = sb64_ref_distance_weight;
  } else if (mbmi->sb_type == BLOCK_SIZE_SB32X32) {
    mv_ref_search = sb_mv_ref_search;
    ref_distance_weight = sb_ref_distance_weight;
  } else {
    mv_ref_search = mb_mv_ref_search;
    ref_distance_weight = mb_ref_distance_weight;
  }

  // We first scan for candidate vectors that match the current reference frame
  // Look at nearest neigbours
  for (i = 0; i < 2; ++i) {
    const int mb_search_col = mb_col + mv_ref_search[i][0];

    if ((mb_search_col >= cm->cur_tile_mb_col_start) &&
        (mb_search_col < cm->cur_tile_mb_col_end) &&
        ((mv_ref_search[i][1] << 7) >= xd->mb_to_top_edge)) {

      candidate_mi = here + mv_ref_search[i][0] +
                     (mv_ref_search[i][1] * xd->mode_info_stride);

      if (get_matching_candidate(candidate_mi, ref_frame, &c_refmv)) {
        add_candidate_mv(candidate_mvs, candidate_scores,
                         &refmv_count, c_refmv, ref_distance_weight[i] + 16);
      }
      split_count += (candidate_mi->mbmi.mode == SPLITMV);
    }
  }
  // Look in the last frame if it exists
  if (lf_here) {
    candidate_mi = lf_here;
    if (get_matching_candidate(candidate_mi, ref_frame, &c_refmv)) {
      add_candidate_mv(candidate_mvs, candidate_scores,
                       &refmv_count, c_refmv, 18);
    }
  }
  // More distant neigbours
  for (i = 2; (i < MVREF_NEIGHBOURS) &&
              (refmv_count < (MAX_MV_REF_CANDIDATES - 1)); ++i) {
    const int mb_search_col = mb_col + mv_ref_search[i][0];

    if ((mb_search_col >= cm->cur_tile_mb_col_start) &&
        (mb_search_col < cm->cur_tile_mb_col_end) &&
        ((mv_ref_search[i][1] << 7) >= xd->mb_to_top_edge)) {
      candidate_mi = here + mv_ref_search[i][0] +
                     (mv_ref_search[i][1] * xd->mode_info_stride);

      if (get_matching_candidate(candidate_mi, ref_frame, &c_refmv)) {
        add_candidate_mv(candidate_mvs, candidate_scores,
                         &refmv_count, c_refmv, ref_distance_weight[i] + 16);
      }
    }
  }

  // If we have not found enough candidates consider ones where the
  // reference frame does not match. Break out when we have
  // MAX_MV_REF_CANDIDATES candidates.
  // Look first at spatial neighbours
  if (refmv_count < (MAX_MV_REF_CANDIDATES - 1)) {
    for (i = 0; i < MVREF_NEIGHBOURS; ++i) {
      const int mb_search_col = mb_col + mv_ref_search[i][0];

      if ((mb_search_col >= cm->cur_tile_mb_col_start) &&
          (mb_search_col < cm->cur_tile_mb_col_end) &&
          ((mv_ref_search[i][1] << 7) >= xd->mb_to_top_edge)) {

        candidate_mi = here + mv_ref_search[i][0] +
                       (mv_ref_search[i][1] * xd->mode_info_stride);

        get_non_matching_candidates(candidate_mi, ref_frame,
                                    &c_ref_frame, &c_refmv,
                                    &c2_ref_frame, &c2_refmv);

        if (c_ref_frame != INTRA_FRAME) {
          scale_mv(xd, ref_frame, c_ref_frame, &c_refmv, ref_sign_bias);
          add_candidate_mv(candidate_mvs, candidate_scores,
                           &refmv_count, c_refmv, ref_distance_weight[i]);
        }

        if (c2_ref_frame != INTRA_FRAME) {
          scale_mv(xd, ref_frame, c2_ref_frame, &c2_refmv, ref_sign_bias);
          add_candidate_mv(candidate_mvs, candidate_scores,
                           &refmv_count, c2_refmv, ref_distance_weight[i]);
        }
      }

      if (refmv_count >= (MAX_MV_REF_CANDIDATES - 1)) {
        break;
      }
    }
  }
  // Look at the last frame if it exists
  if (refmv_count < (MAX_MV_REF_CANDIDATES - 1) && lf_here) {
    candidate_mi = lf_here;
    get_non_matching_candidates(candidate_mi, ref_frame,
                                &c_ref_frame, &c_refmv,
                                &c2_ref_frame, &c2_refmv);

    if (c_ref_frame != INTRA_FRAME) {
      scale_mv(xd, ref_frame, c_ref_frame, &c_refmv, ref_sign_bias);
      add_candidate_mv(candidate_mvs, candidate_scores,
                       &refmv_count, c_refmv, 2);
    }

    if (c2_ref_frame != INTRA_FRAME) {
      scale_mv(xd, ref_frame, c2_ref_frame, &c2_refmv, ref_sign_bias);
      add_candidate_mv(candidate_mvs, candidate_scores,
                       &refmv_count, c2_refmv, 2);
    }
  }

  // Define inter mode coding context.
  // 0,0 was best
  if (candidate_mvs[0].as_int == 0) {
    // 0,0 is only candidate
    if (refmv_count <= 1) {
      mbmi->mb_mode_context[ref_frame] = 0;
    // non zero candidates candidates available
    } else if (split_count == 0) {
      mbmi->mb_mode_context[ref_frame] = 1;
    } else {
      mbmi->mb_mode_context[ref_frame] = 2;
    }
  } else if (split_count == 0) {
    // Non zero best, No Split MV cases
    mbmi->mb_mode_context[ref_frame] = candidate_scores[0] >= 16 ? 3 : 4;
  } else {
    // Non zero best, some split mv
    mbmi->mb_mode_context[ref_frame] = candidate_scores[0] >= 16 ? 5 : 6;
  }

  // Scan for 0,0 case and clamp non zero choices
  for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i) {
    if (candidate_mvs[i].as_int == 0) {
      zero_seen = TRUE;
    } else {
      clamp_mv_ref(xd, &candidate_mvs[i]);
    }
  }
  // 0,0 is always a valid reference. Add it if not already seen.
  if (!zero_seen)
    candidate_mvs[MAX_MV_REF_CANDIDATES-1].as_int = 0;

  // Copy over the candidate list.
  vpx_memcpy(mv_ref_list, candidate_mvs, sizeof(candidate_mvs));
}
