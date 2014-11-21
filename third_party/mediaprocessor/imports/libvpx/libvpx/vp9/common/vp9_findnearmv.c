/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <limits.h>

#include "vp9/common/vp9_findnearmv.h"
#include "vp9/common/vp9_sadmxn.h"
#include "vp9/common/vp9_subpelvar.h"

const uint8_t vp9_mbsplit_offset[4][16] = {
  { 0,  8,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0},
  { 0,  2,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0},
  { 0,  2,  8, 10,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0},
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15}
};

static void lower_mv_precision(int_mv *mv, int usehp)
{
  if (!usehp || !vp9_use_nmv_hp(&mv->as_mv)) {
    if (mv->as_mv.row & 1)
      mv->as_mv.row += (mv->as_mv.row > 0 ? -1 : 1);
    if (mv->as_mv.col & 1)
      mv->as_mv.col += (mv->as_mv.col > 0 ? -1 : 1);
  }
}

vp9_prob *vp9_mv_ref_probs(VP9_COMMON *pc,
                           vp9_prob p[4], const int context) {
  p[0] = pc->fc.vp9_mode_contexts[context][0];
  p[1] = pc->fc.vp9_mode_contexts[context][1];
  p[2] = pc->fc.vp9_mode_contexts[context][2];
  p[3] = pc->fc.vp9_mode_contexts[context][3];
  return p;
}

#define SP(x) (((x) & 7) << 1)
unsigned int vp9_sad3x16_c(const uint8_t *src_ptr,
                           int  src_stride,
                           const uint8_t *ref_ptr,
                           int  ref_stride) {
  return sad_mx_n_c(src_ptr, src_stride, ref_ptr, ref_stride, 3, 16);
}
unsigned int vp9_sad16x3_c(const uint8_t *src_ptr,
                           int  src_stride,
                           const uint8_t *ref_ptr,
                           int  ref_stride) {
  return sad_mx_n_c(src_ptr, src_stride, ref_ptr, ref_stride, 16, 3);
}


unsigned int vp9_variance2x16_c(const uint8_t *src_ptr,
                                int  source_stride,
                                const uint8_t *ref_ptr,
                                int  recon_stride,
                                unsigned int *sse) {
  int sum;
  variance(src_ptr, source_stride, ref_ptr, recon_stride, 2, 16, sse, &sum);
  return (*sse - (((unsigned int)sum * sum) >> 5));
}

unsigned int vp9_variance16x2_c(const uint8_t *src_ptr,
                                int  source_stride,
                                const uint8_t *ref_ptr,
                                int  recon_stride,
                                unsigned int *sse) {
  int sum;
  variance(src_ptr, source_stride, ref_ptr, recon_stride, 16, 2, sse, &sum);
  return (*sse - (((unsigned int)sum * sum) >> 5));
}

unsigned int vp9_sub_pixel_variance16x2_c(const uint8_t *src_ptr,
                                          int  source_stride,
                                          int  xoffset,
                                          int  yoffset,
                                          const uint8_t *ref_ptr,
                                          int ref_stride,
                                          unsigned int *sse) {
  uint16_t FData3[16 * 3];  // Temp data buffer used in filtering
  uint8_t temp2[2 * 16];
  const int16_t *HFilter, *VFilter;

  HFilter = VP9_BILINEAR_FILTERS_2TAP(xoffset);
  VFilter = VP9_BILINEAR_FILTERS_2TAP(yoffset);

  var_filter_block2d_bil_first_pass(src_ptr, FData3,
                                    source_stride, 1, 3, 16, HFilter);
  var_filter_block2d_bil_second_pass(FData3, temp2, 16, 16, 2, 16, VFilter);

  return vp9_variance16x2_c(temp2, 16, ref_ptr, ref_stride, sse);
}

unsigned int vp9_sub_pixel_variance2x16_c(const uint8_t *src_ptr,
                                          int  source_stride,
                                          int  xoffset,
                                          int  yoffset,
                                          const uint8_t *ref_ptr,
                                          int ref_stride,
                                          unsigned int *sse) {
  uint16_t FData3[2 * 17];  // Temp data buffer used in filtering
  uint8_t temp2[2 * 16];
  const int16_t *HFilter, *VFilter;

  HFilter = VP9_BILINEAR_FILTERS_2TAP(xoffset);
  VFilter = VP9_BILINEAR_FILTERS_2TAP(yoffset);

  var_filter_block2d_bil_first_pass(src_ptr, FData3,
                                    source_stride, 1, 17, 2, HFilter);
  var_filter_block2d_bil_second_pass(FData3, temp2, 2, 2, 16, 2, VFilter);

  return vp9_variance2x16_c(temp2, 2, ref_ptr, ref_stride, sse);
}

#if CONFIG_USESELECTREFMV
/* check a list of motion vectors by sad score using a number rows of pixels
 * above and a number cols of pixels in the left to select the one with best
 * score to use as ref motion vector
 */

void vp9_find_best_ref_mvs(MACROBLOCKD *xd,
                           uint8_t *ref_y_buffer,
                           int ref_y_stride,
                           int_mv *mvlist,
                           int_mv *nearest,
                           int_mv *near) {
  int i, j;
  uint8_t *above_src;
  uint8_t *above_ref;
#if !CONFIG_ABOVESPREFMV
  uint8_t *left_src;
  uint8_t *left_ref;
#endif
  unsigned int score;
  unsigned int sse;
  unsigned int ref_scores[MAX_MV_REF_CANDIDATES] = {0};
  int_mv sorted_mvs[MAX_MV_REF_CANDIDATES];
  int zero_seen = FALSE;

  if (ref_y_buffer) {

    // Default all to 0,0 if nothing else available
    nearest->as_int = near->as_int = 0;
    vpx_memset(sorted_mvs, 0, sizeof(sorted_mvs));

    above_src = xd->dst.y_buffer - xd->dst.y_stride * 2;
    above_ref = ref_y_buffer - ref_y_stride * 2;
#if CONFIG_ABOVESPREFMV
    above_src -= 4;
    above_ref -= 4;
#else
    left_src  = xd->dst.y_buffer - 2;
    left_ref  = ref_y_buffer - 2;
#endif

    // Limit search to the predicted best few candidates
    for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i) {
      int_mv this_mv;
      int offset = 0;
      int row_offset, col_offset;

      this_mv.as_int = mvlist[i].as_int;

      // If we see a 0,0 vector for a second time we have reached the end of
      // the list of valid candidate vectors.
      if (!this_mv.as_int && zero_seen)
        break;

      zero_seen = zero_seen || !this_mv.as_int;

#if !CONFIG_ABOVESPREFMV
      clamp_mv(&this_mv,
               xd->mb_to_left_edge - LEFT_TOP_MARGIN + 24,
               xd->mb_to_right_edge + RIGHT_BOTTOM_MARGIN,
               xd->mb_to_top_edge - LEFT_TOP_MARGIN + 24,
               xd->mb_to_bottom_edge + RIGHT_BOTTOM_MARGIN);
#else
      clamp_mv(&this_mv,
               xd->mb_to_left_edge - LEFT_TOP_MARGIN + 32,
               xd->mb_to_right_edge + RIGHT_BOTTOM_MARGIN,
               xd->mb_to_top_edge - LEFT_TOP_MARGIN + 24,
               xd->mb_to_bottom_edge + RIGHT_BOTTOM_MARGIN);
#endif

      row_offset = this_mv.as_mv.row >> 3;
      col_offset = this_mv.as_mv.col >> 3;
      offset = ref_y_stride * row_offset + col_offset;
      score = 0;
#if !CONFIG_ABOVESPREFMV
      if (xd->up_available) {
#else
      if (xd->up_available && xd->left_available) {
#endif
        vp9_sub_pixel_variance16x2(above_ref + offset, ref_y_stride,
                                   SP(this_mv.as_mv.col),
                                   SP(this_mv.as_mv.row),
                                   above_src, xd->dst.y_stride, &sse);
        score += sse;
        if (xd->mode_info_context->mbmi.sb_type >= BLOCK_SIZE_SB32X32) {
          vp9_sub_pixel_variance16x2(above_ref + offset + 16,
                                     ref_y_stride,
                                     SP(this_mv.as_mv.col),
                                     SP(this_mv.as_mv.row),
                                     above_src + 16, xd->dst.y_stride, &sse);
          score += sse;
        }
        if (xd->mode_info_context->mbmi.sb_type >= BLOCK_SIZE_SB64X64) {
          vp9_sub_pixel_variance16x2(above_ref + offset + 32,
                                     ref_y_stride,
                                     SP(this_mv.as_mv.col),
                                     SP(this_mv.as_mv.row),
                                     above_src + 32, xd->dst.y_stride, &sse);
          score += sse;
          vp9_sub_pixel_variance16x2(above_ref + offset + 48,
                                     ref_y_stride,
                                     SP(this_mv.as_mv.col),
                                     SP(this_mv.as_mv.row),
                                     above_src + 48, xd->dst.y_stride, &sse);
          score += sse;
        }
      }
#if !CONFIG_ABOVESPREFMV
      if (xd->left_available) {
        vp9_sub_pixel_variance2x16_c(left_ref + offset, ref_y_stride,
                                     SP(this_mv.as_mv.col),
                                     SP(this_mv.as_mv.row),
                                     left_src, xd->dst.y_stride, &sse);
        score += sse;
        if (xd->mode_info_context->mbmi.sb_type >= BLOCK_SIZE_SB32X32) {
          vp9_sub_pixel_variance2x16_c(left_ref + offset + ref_y_stride * 16,
                                       ref_y_stride,
                                       SP(this_mv.as_mv.col),
                                       SP(this_mv.as_mv.row),
                                       left_src + xd->dst.y_stride * 16,
                                       xd->dst.y_stride, &sse);
          score += sse;
        }
        if (xd->mode_info_context->mbmi.sb_type >= BLOCK_SIZE_SB64X64) {
          vp9_sub_pixel_variance2x16_c(left_ref + offset + ref_y_stride * 32,
                                     ref_y_stride,
                                       SP(this_mv.as_mv.col),
                                       SP(this_mv.as_mv.row),
                                       left_src + xd->dst.y_stride * 32,
                                       xd->dst.y_stride, &sse);
          score += sse;
          vp9_sub_pixel_variance2x16_c(left_ref + offset + ref_y_stride * 48,
                                       ref_y_stride,
                                       SP(this_mv.as_mv.col),
                                       SP(this_mv.as_mv.row),
                                       left_src + xd->dst.y_stride * 48,
                                       xd->dst.y_stride, &sse);
          score += sse;
        }
      }
#endif
      // Add the entry to our list and then resort the list on score.
      ref_scores[i] = score;
      sorted_mvs[i].as_int = this_mv.as_int;
      j = i;
      while (j > 0) {
        if (ref_scores[j] < ref_scores[j-1]) {
          ref_scores[j] = ref_scores[j-1];
          sorted_mvs[j].as_int = sorted_mvs[j-1].as_int;
          ref_scores[j-1] = score;
          sorted_mvs[j-1].as_int = this_mv.as_int;
          j--;
        } else {
          break;
        }
      }
    }
  } else {
    vpx_memcpy(sorted_mvs, mvlist, sizeof(sorted_mvs));
  }

  // Make sure all the candidates are properly clamped etc
  for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i) {
    lower_mv_precision(&sorted_mvs[i], xd->allow_high_precision_mv);
    clamp_mv2(&sorted_mvs[i], xd);
  }

  // Nearest may be a 0,0 or non zero vector and now matches the chosen
  // "best reference". This has advantages when it is used as part of a
  // compound predictor as it means a non zero vector can be paired using
  // this mode with a 0 vector. The Near vector is still forced to be a
  // non zero candidate if one is avaialble.
  nearest->as_int = sorted_mvs[0].as_int;
  if ( sorted_mvs[1].as_int ) {
    near->as_int = sorted_mvs[1].as_int;
  } else {
    near->as_int = sorted_mvs[2].as_int;
  }

  // Copy back the re-ordered mv list
  vpx_memcpy(mvlist, sorted_mvs, sizeof(sorted_mvs));
}
#else
void vp9_find_best_ref_mvs(MACROBLOCKD *xd,
                           uint8_t *ref_y_buffer,
                           int ref_y_stride,
                           int_mv *mvlist,
                           int_mv *nearest,
                           int_mv *near) {
  int i;
  // Make sure all the candidates are properly clamped etc
  for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i) {
    lower_mv_precision(&mvlist[i], xd->allow_high_precision_mv);
    clamp_mv2(&mvlist[i], xd);
  }
  *nearest = mvlist[0];
  *near = mvlist[1];
}
#endif
