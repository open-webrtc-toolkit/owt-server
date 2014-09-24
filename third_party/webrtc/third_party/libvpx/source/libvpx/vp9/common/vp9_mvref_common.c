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

// clamp_mv
#define MV_BORDER (16 << 3) // Allow 16 pels in 1/8th pel units
static void clamp_mv(const MACROBLOCKD *xd, int_mv *mv) {

  if (mv->as_mv.col < (xd->mb_to_left_edge - MV_BORDER))
    mv->as_mv.col = xd->mb_to_left_edge - MV_BORDER;
  else if (mv->as_mv.col > xd->mb_to_right_edge + MV_BORDER)
    mv->as_mv.col = xd->mb_to_right_edge + MV_BORDER;

  if (mv->as_mv.row < (xd->mb_to_top_edge - MV_BORDER))
    mv->as_mv.row = xd->mb_to_top_edge - MV_BORDER;
  else if (mv->as_mv.row > xd->mb_to_bottom_edge + MV_BORDER)
    mv->as_mv.row = xd->mb_to_bottom_edge + MV_BORDER;
}

// Gets a candidate refenence motion vector from the given mode info
// structure if one exists that matches the given reference frame.
static int get_matching_candidate(
  const MODE_INFO *candidate_mi,
  MV_REFERENCE_FRAME ref_frame,
  int_mv *c_mv
) {
  int ret_val = TRUE;

  if (ref_frame == candidate_mi->mbmi.ref_frame) {
    c_mv->as_int = candidate_mi->mbmi.mv[0].as_int;
  } else if (ref_frame == candidate_mi->mbmi.second_ref_frame) {
    c_mv->as_int = candidate_mi->mbmi.mv[1].as_int;
  } else {
    ret_val = FALSE;
  }

  return ret_val;
}

// Gets candidate refenence motion vector(s) from the given mode info
// structure if they exists and do NOT match the given reference frame.
static void get_non_matching_candidates(
  const MODE_INFO *candidate_mi,
  MV_REFERENCE_FRAME ref_frame,
  MV_REFERENCE_FRAME *c_ref_frame,
  int_mv *c_mv,
  MV_REFERENCE_FRAME *c2_ref_frame,
  int_mv *c2_mv
) {

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
        (candidate_mi->mbmi.second_ref_frame != ref_frame)) {  // &&
        // (candidate_mi->mbmi.mv[1].as_int != 0) &&
        // (candidate_mi->mbmi.mv[1].as_int !=
        // candidate_mi->mbmi.mv[0].as_int)) {
      *c2_ref_frame = candidate_mi->mbmi.second_ref_frame;
      c2_mv->as_int = candidate_mi->mbmi.mv[1].as_int;
    }
  }
}

// Performs mv adjustment based on reference frame and clamps the MV
// if it goes off the edge of the buffer.
static void scale_mv(
  MACROBLOCKD *xd,
  MV_REFERENCE_FRAME this_ref_frame,
  MV_REFERENCE_FRAME candidate_ref_frame,
  int_mv *candidate_mv,
  int *ref_sign_bias
) {

  if (candidate_ref_frame != this_ref_frame) {

    //int frame_distances[MAX_REF_FRAMES];
    //int last_distance = 1;
    //int gf_distance = xd->frames_since_golden;
    //int arf_distance = xd->frames_till_alt_ref_frame;

    // Sign inversion where appropriate.
    if (ref_sign_bias[candidate_ref_frame] != ref_sign_bias[this_ref_frame]) {
      candidate_mv->as_mv.row = -candidate_mv->as_mv.row;
      candidate_mv->as_mv.col = -candidate_mv->as_mv.col;
    }

    // Scale based on frame distance if the reference frames not the same.
    /*frame_distances[INTRA_FRAME] = 1;   // should never be used
    frame_distances[LAST_FRAME] = 1;
    frame_distances[GOLDEN_FRAME] =
      (xd->frames_since_golden) ? xd->frames_since_golden : 1;
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

  // Clamp the MV so it does not point out of the frame buffer
  clamp_mv(xd, candidate_mv);
}

// Adds a new candidate reference vector to the list if indeed it is new.
// If it is not new then the score of the existing candidate that it matches
// is increased and the list is resorted.
static void addmv_and_shuffle(
  int_mv *mv_list,
  int *mv_scores,
  int *index,
  int_mv candidate_mv,
  int weight
) {

  int i;
  int insert_point;
  int duplicate_found = FALSE;

  // Check for duplicates. If there is one increase its score.
  // We only compare vs the current top candidates.
  insert_point = (*index < (MAX_MV_REF_CANDIDATES - 1))
                 ? *index : (MAX_MV_REF_CANDIDATES - 1);

  i = insert_point;
  if (*index > i)
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
    (*index)++;
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

// This function searches the neighbourhood of a given MB/SB and populates a
// list of candidate reference vectors.
//
void vp9_find_mv_refs(
  MACROBLOCKD *xd,
  MODE_INFO *here,
  MODE_INFO *lf_here,
  MV_REFERENCE_FRAME ref_frame,
  int_mv *mv_ref_list,
  int *ref_sign_bias
) {

  int i;
  MODE_INFO *candidate_mi;
  MB_MODE_INFO * mbmi = &xd->mode_info_context->mbmi;
  int_mv candidate_mvs[MAX_MV_REF_CANDIDATES];
  int_mv c_refmv;
  int_mv c2_refmv;
  MV_REFERENCE_FRAME c_ref_frame;
  MV_REFERENCE_FRAME c2_ref_frame;
  int candidate_scores[MAX_MV_REF_CANDIDATES];
  int index = 0;
  int split_count = 0;
  int (*mv_ref_search)[2];
  int *ref_distance_weight;

  // Blank the reference vector lists and other local structures.
  vpx_memset(mv_ref_list, 0, sizeof(int_mv) * MAX_MV_REF_CANDIDATES);
  vpx_memset(candidate_mvs, 0, sizeof(int_mv) * MAX_MV_REF_CANDIDATES);
  vpx_memset(candidate_scores, 0, sizeof(candidate_scores));

  if (mbmi->sb_type) {
    mv_ref_search = sb_mv_ref_search;
    ref_distance_weight = sb_ref_distance_weight;
  } else {
    mv_ref_search = mb_mv_ref_search;
    ref_distance_weight = mb_ref_distance_weight;
  }

  // We first scan for candidate vectors that match the current reference frame
  // Look at nearest neigbours
  for (i = 0; i < 2; ++i) {
    if (((mv_ref_search[i][0] << 7) >= xd->mb_to_left_edge) &&
        ((mv_ref_search[i][1] << 7) >= xd->mb_to_top_edge)) {

      candidate_mi = here + mv_ref_search[i][0] +
                     (mv_ref_search[i][1] * xd->mode_info_stride);

      if (get_matching_candidate(candidate_mi, ref_frame, &c_refmv)) {
        clamp_mv(xd, &c_refmv);
        addmv_and_shuffle(candidate_mvs, candidate_scores,
                          &index, c_refmv, ref_distance_weight[i] + 16);
      }
      split_count += (candidate_mi->mbmi.mode == SPLITMV);
    }
  }
  // Look in the last frame
  candidate_mi = lf_here;
  if (get_matching_candidate(candidate_mi, ref_frame, &c_refmv)) {
    clamp_mv(xd, &c_refmv);
    addmv_and_shuffle(candidate_mvs, candidate_scores,
                      &index, c_refmv, 18);
  }
  // More distant neigbours
  for (i = 2; (i < MVREF_NEIGHBOURS) &&
              (index < (MAX_MV_REF_CANDIDATES - 1)); ++i) {
    if (((mv_ref_search[i][0] << 7) >= xd->mb_to_left_edge) &&
        ((mv_ref_search[i][1] << 7) >= xd->mb_to_top_edge)) {
      candidate_mi = here + mv_ref_search[i][0] +
                     (mv_ref_search[i][1] * xd->mode_info_stride);

      if (get_matching_candidate(candidate_mi, ref_frame, &c_refmv)) {
        clamp_mv(xd, &c_refmv);
        addmv_and_shuffle(candidate_mvs, candidate_scores,
                          &index, c_refmv, ref_distance_weight[i] + 16);
      }
    }
  }

  // If we have not found enough candidates consider ones where the
  // reference frame does not match. Break out when we have
  // MAX_MV_REF_CANDIDATES candidates.
  // Look first at spatial neighbours
  if (index < (MAX_MV_REF_CANDIDATES - 1)) {
    for (i = 0; i < MVREF_NEIGHBOURS; ++i) {
      if (((mv_ref_search[i][0] << 7) >= xd->mb_to_left_edge) &&
          ((mv_ref_search[i][1] << 7) >= xd->mb_to_top_edge)) {

        candidate_mi = here + mv_ref_search[i][0] +
                       (mv_ref_search[i][1] * xd->mode_info_stride);

        get_non_matching_candidates(candidate_mi, ref_frame,
                                    &c_ref_frame, &c_refmv,
                                    &c2_ref_frame, &c2_refmv);

        if (c_ref_frame != INTRA_FRAME) {
          scale_mv(xd, ref_frame, c_ref_frame, &c_refmv, ref_sign_bias);
          addmv_and_shuffle(candidate_mvs, candidate_scores,
                            &index, c_refmv, ref_distance_weight[i]);
        }

        if (c2_ref_frame != INTRA_FRAME) {
          scale_mv(xd, ref_frame, c2_ref_frame, &c2_refmv, ref_sign_bias);
          addmv_and_shuffle(candidate_mvs, candidate_scores,
                            &index, c2_refmv, ref_distance_weight[i]);
        }
      }

      if (index >= (MAX_MV_REF_CANDIDATES - 1)) {
        break;
      }
    }
  }
  // Look at the last frame
  if (index < (MAX_MV_REF_CANDIDATES - 1)) {
    candidate_mi = lf_here;
    get_non_matching_candidates(candidate_mi, ref_frame,
                                &c_ref_frame, &c_refmv,
                                &c2_ref_frame, &c2_refmv);

    if (c_ref_frame != INTRA_FRAME) {
      scale_mv(xd, ref_frame, c_ref_frame, &c_refmv, ref_sign_bias);
      addmv_and_shuffle(candidate_mvs, candidate_scores,
                        &index, c_refmv, 2);
    }

    if (c2_ref_frame != INTRA_FRAME) {
      scale_mv(xd, ref_frame, c2_ref_frame, &c2_refmv, ref_sign_bias);
      addmv_and_shuffle(candidate_mvs, candidate_scores,
                        &index, c2_refmv, 2);
    }
  }

  // Define inter mode coding context.
  // 0,0 was best
  if (candidate_mvs[0].as_int == 0) {
    // 0,0 is only candidate
    if (index <= 1) {
      mbmi->mb_mode_context[ref_frame] = 0;
    // non zero candidates candidates available
    } else if (split_count == 0) {
      mbmi->mb_mode_context[ref_frame] = 1;
    } else {
      mbmi->mb_mode_context[ref_frame] = 2;
    }
  // Non zero best, No Split MV cases
  } else if (split_count == 0) {
    if (candidate_scores[0] >= 32) {
      mbmi->mb_mode_context[ref_frame] = 3;
    } else {
      mbmi->mb_mode_context[ref_frame] = 4;
    }
  // Non zero best, some split mv
  } else {
    if (candidate_scores[0] >= 32) {
      mbmi->mb_mode_context[ref_frame] = 5;
    } else {
      mbmi->mb_mode_context[ref_frame] = 6;
    }
  }

  // 0,0 is always a valid reference.
  for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i) {
    if (candidate_mvs[i].as_int == 0)
      break;
  }
  if (i == MAX_MV_REF_CANDIDATES) {
    candidate_mvs[MAX_MV_REF_CANDIDATES-1].as_int = 0;
  }

  // Copy over the candidate list.
  vpx_memcpy(mv_ref_list, candidate_mvs, sizeof(candidate_mvs));
}
