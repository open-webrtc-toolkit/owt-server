/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/common/vp9_onyxc_int.h"
#include "vp9/encoder/vp9_onyx_int.h"
#include "vp9/encoder/vp9_picklpf.h"
#include "vp9/encoder/vp9_quantize.h"
#include "vpx_mem/vpx_mem.h"
#include "vpx_scale/vpx_scale.h"
#include "vp9/common/vp9_alloccommon.h"
#include "vp9/common/vp9_loopfilter.h"
#include "./vpx_scale_rtcd.h"

void vp9_yv12_copy_partial_frame_c(YV12_BUFFER_CONFIG *src_ybc,
                                   YV12_BUFFER_CONFIG *dst_ybc, int Fraction) {
  uint8_t *src_y, *dst_y;
  int yheight;
  int ystride;
  int yoffset;
  int linestocopy;

  yheight  = src_ybc->y_height;
  ystride  = src_ybc->y_stride;

  linestocopy = (yheight >> (Fraction + 4));

  if (linestocopy < 1)
    linestocopy = 1;

  linestocopy <<= 4;

  yoffset  = ystride * ((yheight >> 5) * 16 - 8);
  src_y = src_ybc->y_buffer + yoffset;
  dst_y = dst_ybc->y_buffer + yoffset;

  vpx_memcpy(dst_y, src_y, ystride * (linestocopy + 16));
}

static int calc_partial_ssl_err(YV12_BUFFER_CONFIG *source,
                                YV12_BUFFER_CONFIG *dest, int Fraction) {
  int i, j;
  int Total = 0;
  int srcoffset, dstoffset;
  uint8_t *src = source->y_buffer;
  uint8_t *dst = dest->y_buffer;

  int linestocopy = (source->y_height >> (Fraction + 4));

  if (linestocopy < 1)
    linestocopy = 1;

  linestocopy <<= 4;


  srcoffset = source->y_stride   * (dest->y_height >> 5) * 16;
  dstoffset = dest->y_stride     * (dest->y_height >> 5) * 16;

  src += srcoffset;
  dst += dstoffset;

  // Loop through the Y plane raw and reconstruction data summing (square differences)
  for (i = 0; i < linestocopy; i += 16) {
    for (j = 0; j < source->y_width; j += 16) {
      unsigned int sse;
      Total += vp9_mse16x16(src + j, source->y_stride, dst + j, dest->y_stride,
                            &sse);
    }

    src += 16 * source->y_stride;
    dst += 16 * dest->y_stride;
  }

  return Total;
}

// Enforce a minimum filter level based upon baseline Q
static int get_min_filter_level(VP9_COMP *cpi, int base_qindex) {
  int min_filter_level;
  /*int q = (int) vp9_convert_qindex_to_q(base_qindex);

  if (cpi->source_alt_ref_active && cpi->common.refresh_golden_frame && !cpi->common.refresh_alt_ref_frame)
      min_filter_level = 0;
  else
  {
      if (q <= 10)
          min_filter_level = 0;
      else if (q <= 64)
          min_filter_level = 1;
      else
          min_filter_level = (q >> 6);
  }
  */
  min_filter_level = 0;

  return min_filter_level;
}

// Enforce a maximum filter level based upon baseline Q
static int get_max_filter_level(VP9_COMP *cpi, int base_qindex) {
  // PGW August 2006: Highest filter values almost always a bad idea

  // jbb chg: 20100118 - not so any more with this overquant stuff allow high values
  // with lots of intra coming in.
  int max_filter_level = MAX_LOOP_FILTER;// * 3 / 4;
  (void)base_qindex;

  if (cpi->twopass.section_intra_rating > 8)
    max_filter_level = MAX_LOOP_FILTER * 3 / 4;

  return max_filter_level;
}

void vp9_pick_filter_level_fast(YV12_BUFFER_CONFIG *sd, VP9_COMP *cpi) {
  VP9_COMMON *cm = &cpi->common;

  int best_err = 0;
  int filt_err = 0;
  int min_filter_level = get_min_filter_level(cpi, cm->base_qindex);
  int max_filter_level = get_max_filter_level(cpi, cm->base_qindex);
  int filt_val;
  int best_filt_val = cm->filter_level;

  //  Make a copy of the unfiltered / processed recon buffer
  vp9_yv12_copy_partial_frame(cm->frame_to_show, &cpi->last_frame_uf, 3);

  if (cm->frame_type == KEY_FRAME)
    cm->sharpness_level = 0;
  else
    cm->sharpness_level = cpi->oxcf.Sharpness;

  if (cm->sharpness_level != cm->last_sharpness_level) {
    vp9_loop_filter_update_sharpness(&cm->lf_info, cm->sharpness_level);
    cm->last_sharpness_level = cm->sharpness_level;
  }

  // Start the search at the previous frame filter level unless it is now out of range.
  if (cm->filter_level < min_filter_level)
    cm->filter_level = min_filter_level;
  else if (cm->filter_level > max_filter_level)
    cm->filter_level = max_filter_level;

  filt_val = cm->filter_level;
  best_filt_val = filt_val;

  // Get the err using the previous frame's filter value.
  vp9_loop_filter_partial_frame(cm, &cpi->mb.e_mbd, filt_val);

  best_err = calc_partial_ssl_err(sd, cm->frame_to_show, 3);

  //  Re-instate the unfiltered frame
  vp9_yv12_copy_partial_frame(&cpi->last_frame_uf, cm->frame_to_show, 3);

  filt_val -= (1 + ((filt_val > 10) ? 1 : 0));

  // Search lower filter levels
  while (filt_val >= min_filter_level) {
    // Apply the loop filter
    vp9_loop_filter_partial_frame(cm, &cpi->mb.e_mbd, filt_val);

    // Get the err for filtered frame
    filt_err = calc_partial_ssl_err(sd, cm->frame_to_show, 3);

    //  Re-instate the unfiltered frame
    vp9_yv12_copy_partial_frame(&cpi->last_frame_uf, cm->frame_to_show, 3);


    // Update the best case record or exit loop.
    if (filt_err < best_err) {
      best_err = filt_err;
      best_filt_val = filt_val;
    } else
      break;

    // Adjust filter level
    filt_val -= (1 + ((filt_val > 10) ? 1 : 0));
  }

  // Search up (note that we have already done filt_val = cm->filter_level)
  filt_val = cm->filter_level + (1 + ((filt_val > 10) ? 1 : 0));

  if (best_filt_val == cm->filter_level) {
    // Resist raising filter level for very small gains
    best_err -= (best_err >> 10);

    while (filt_val < max_filter_level) {
      // Apply the loop filter
      vp9_loop_filter_partial_frame(cm, &cpi->mb.e_mbd, filt_val);

      // Get the err for filtered frame
      filt_err = calc_partial_ssl_err(sd, cm->frame_to_show, 3);

      //  Re-instate the unfiltered frame
      vp9_yv12_copy_partial_frame(&cpi->last_frame_uf,
                                      cm->frame_to_show, 3);

      // Update the best case record or exit loop.
      if (filt_err < best_err) {
        // Do not raise filter level if improvement is < 1 part in 4096
        best_err = filt_err - (filt_err >> 10);

        best_filt_val = filt_val;
      } else
        break;

      // Adjust filter level
      filt_val += (1 + ((filt_val > 10) ? 1 : 0));
    }
  }

  cm->filter_level = best_filt_val;

  if (cm->filter_level < min_filter_level)
    cm->filter_level = min_filter_level;

  if (cm->filter_level > max_filter_level)
    cm->filter_level = max_filter_level;
}

// Stub function for now Alt LF not used
void vp9_set_alt_lf_level(VP9_COMP *cpi, int filt_val) {
}

void vp9_pick_filter_level(YV12_BUFFER_CONFIG *sd, VP9_COMP *cpi) {
  VP9_COMMON *cm = &cpi->common;

  int best_err = 0;
  int filt_err = 0;
  int min_filter_level = get_min_filter_level(cpi, cm->base_qindex);
  int max_filter_level = get_max_filter_level(cpi, cm->base_qindex);

  int filter_step;
  int filt_high = 0;
  int filt_mid = cm->filter_level;      // Start search at previous frame filter level
  int filt_low = 0;
  int filt_best;
  int filt_direction = 0;

  int Bias = 0;                       // Bias against raising loop filter and in favour of lowering it

  //  Make a copy of the unfiltered / processed recon buffer
  vp8_yv12_copy_frame(cm->frame_to_show, &cpi->last_frame_uf);

  if (cm->frame_type == KEY_FRAME)
    cm->sharpness_level = 0;
  else
    cm->sharpness_level = cpi->oxcf.Sharpness;

  // Start the search at the previous frame filter level unless it is now out of range.
  filt_mid = cm->filter_level;

  if (filt_mid < min_filter_level)
    filt_mid = min_filter_level;
  else if (filt_mid > max_filter_level)
    filt_mid = max_filter_level;

  // Define the initial step size
  filter_step = (filt_mid < 16) ? 4 : filt_mid / 4;

  // Get baseline error score
  vp9_set_alt_lf_level(cpi, filt_mid);
  vp9_loop_filter_frame(cm, &cpi->mb.e_mbd, filt_mid, 1);

  best_err = vp9_calc_ss_err(sd, cm->frame_to_show);
  filt_best = filt_mid;

  //  Re-instate the unfiltered frame
  vp8_yv12_copy_y(&cpi->last_frame_uf, cm->frame_to_show);

  while (filter_step > 0) {
    Bias = (best_err >> (15 - (filt_mid / 8))) * filter_step; // PGW change 12/12/06 for small images

    // jbb chg: 20100118 - in sections with lots of new material coming in don't bias as much to a low filter value
    if (cpi->twopass.section_intra_rating < 20)
      Bias = Bias * cpi->twopass.section_intra_rating / 20;

    // yx, bias less for large block size
    if (cpi->common.txfm_mode != ONLY_4X4)
      Bias >>= 1;

    filt_high = ((filt_mid + filter_step) > max_filter_level) ? max_filter_level : (filt_mid + filter_step);
    filt_low = ((filt_mid - filter_step) < min_filter_level) ? min_filter_level : (filt_mid - filter_step);

    if ((filt_direction <= 0) && (filt_low != filt_mid)) {
      // Get Low filter error score
      vp9_set_alt_lf_level(cpi, filt_low);
      vp9_loop_filter_frame(cm, &cpi->mb.e_mbd, filt_low, 1);

      filt_err = vp9_calc_ss_err(sd, cm->frame_to_show);

      //  Re-instate the unfiltered frame
      vp8_yv12_copy_y(&cpi->last_frame_uf, cm->frame_to_show);

      // If value is close to the best so far then bias towards a lower loop filter value.
      if ((filt_err - Bias) < best_err) {
        // Was it actually better than the previous best?
        if (filt_err < best_err)
          best_err = filt_err;

        filt_best = filt_low;
      }
    }

    // Now look at filt_high
    if ((filt_direction >= 0) && (filt_high != filt_mid)) {
      vp9_set_alt_lf_level(cpi, filt_high);
      vp9_loop_filter_frame(cm, &cpi->mb.e_mbd, filt_high, 1);

      filt_err = vp9_calc_ss_err(sd, cm->frame_to_show);

      //  Re-instate the unfiltered frame
      vp8_yv12_copy_y(&cpi->last_frame_uf, cm->frame_to_show);

      // Was it better than the previous best?
      if (filt_err < (best_err - Bias)) {
        best_err = filt_err;
        filt_best = filt_high;
      }
    }

    // Half the step distance if the best filter value was the same as last time
    if (filt_best == filt_mid) {
      filter_step = filter_step / 2;
      filt_direction = 0;
    } else {
      filt_direction = (filt_best < filt_mid) ? -1 : 1;
      filt_mid = filt_best;
    }
  }

  cm->filter_level = filt_best;
}
