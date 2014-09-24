/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "math.h"
#include "vp9/common/vp9_alloccommon.h"
#include "vp9/common/vp9_modecont.h"
#include "vp9/common/vp9_common.h"
#include "vp9/encoder/vp9_ratectrl.h"
#include "vp9/common/vp9_entropymode.h"
#include "vpx_mem/vpx_mem.h"
#include "vp9/common/vp9_systemdependent.h"
#include "vp9/encoder/vp9_encodemv.h"
#include "vp9/common/vp9_quant_common.h"

#define MIN_BPB_FACTOR          0.005
#define MAX_BPB_FACTOR          50

#ifdef MODE_STATS
extern unsigned int y_modes[VP9_YMODES];
extern unsigned int uv_modes[VP9_UV_MODES];
extern unsigned int b_modes[B_MODE_COUNT];

extern unsigned int inter_y_modes[MB_MODE_COUNT];
extern unsigned int inter_uv_modes[VP9_UV_MODES];
extern unsigned int inter_b_modes[B_MODE_COUNT];
#endif

// Bits Per MB at different Q (Multiplied by 512)
#define BPER_MB_NORMBITS    9

// % adjustment to target kf size based on seperation from previous frame
static const int kf_boost_seperation_adjustment[16] = {
  30,   40,   50,   55,   60,   65,   70,   75,
  80,   85,   90,   95,  100,  100,  100,  100,
};

static const int gf_adjust_table[101] = {
  100,
  115, 130, 145, 160, 175, 190, 200, 210, 220, 230,
  240, 260, 270, 280, 290, 300, 310, 320, 330, 340,
  350, 360, 370, 380, 390, 400, 400, 400, 400, 400,
  400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
  400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
  400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
  400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
  400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
  400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
  400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
};

static const int gf_intra_usage_adjustment[20] = {
  125, 120, 115, 110, 105, 100,  95,  85,  80,  75,
  70,  65,  60,  55,  50,  50,  50,  50,  50,  50,
};

static const int gf_interval_table[101] = {
  7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
  10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
};

static const unsigned int prior_key_frame_weight[KEY_FRAME_CONTEXT] = { 1, 2, 3, 4, 5 };

// These functions use formulaic calculations to make playing with the
// quantizer tables easier. If necessary they can be replaced by lookup
// tables if and when things settle down in the experimental bitstream
double vp9_convert_qindex_to_q(int qindex) {
  // Convert the index to a real Q value (scaled down to match old Q values)
  return (double)vp9_ac_yquant(qindex) / 4.0;
}

int vp9_gfboost_qadjust(int qindex) {
  int retval;
  double q;

  q = vp9_convert_qindex_to_q(qindex);
  retval = (int)((0.00000828 * q * q * q) +
                 (-0.0055 * q * q) +
                 (1.32 * q) + 79.3);
  return retval;
}

static int kfboost_qadjust(int qindex) {
  int retval;
  double q;

  q = vp9_convert_qindex_to_q(qindex);
  retval = (int)((0.00000973 * q * q * q) +
                 (-0.00613 * q * q) +
                 (1.316 * q) + 121.2);
  return retval;
}

int vp9_bits_per_mb(FRAME_TYPE frame_type, int qindex) {
  if (frame_type == KEY_FRAME)
    return (int)(4500000 / vp9_convert_qindex_to_q(qindex));
  else
    return (int)(2850000 / vp9_convert_qindex_to_q(qindex));
}


void vp9_save_coding_context(VP9_COMP *cpi) {
  CODING_CONTEXT *const cc = &cpi->coding_context;
  VP9_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &cpi->mb.e_mbd;

  // Stores a snapshot of key state variables which can subsequently be
  // restored with a call to vp9_restore_coding_context. These functions are
  // intended for use in a re-code loop in vp9_compress_frame where the
  // quantizer value is adjusted between loop iterations.

  cc->nmvc = cm->fc.nmvc;
  vp9_copy(cc->nmvjointcost,  cpi->mb.nmvjointcost);
  vp9_copy(cc->nmvcosts,  cpi->mb.nmvcosts);
  vp9_copy(cc->nmvcosts_hp,  cpi->mb.nmvcosts_hp);

  vp9_copy(cc->vp9_mode_contexts, cm->fc.vp9_mode_contexts);

  vp9_copy(cc->ymode_prob, cm->fc.ymode_prob);
  vp9_copy(cc->sb_ymode_prob, cm->fc.sb_ymode_prob);
  vp9_copy(cc->bmode_prob, cm->fc.bmode_prob);
  vp9_copy(cc->uv_mode_prob, cm->fc.uv_mode_prob);
  vp9_copy(cc->i8x8_mode_prob, cm->fc.i8x8_mode_prob);
  vp9_copy(cc->sub_mv_ref_prob, cm->fc.sub_mv_ref_prob);
  vp9_copy(cc->mbsplit_prob, cm->fc.mbsplit_prob);

  // Stats
#ifdef MODE_STATS
  vp9_copy(cc->y_modes,       y_modes);
  vp9_copy(cc->uv_modes,      uv_modes);
  vp9_copy(cc->b_modes,       b_modes);
  vp9_copy(cc->inter_y_modes,  inter_y_modes);
  vp9_copy(cc->inter_uv_modes, inter_uv_modes);
  vp9_copy(cc->inter_b_modes,  inter_b_modes);
#endif

  vp9_copy(cc->segment_pred_probs, cm->segment_pred_probs);
  vp9_copy(cc->ref_pred_probs_update, cpi->ref_pred_probs_update);
  vp9_copy(cc->ref_pred_probs, cm->ref_pred_probs);
  vp9_copy(cc->prob_comppred, cm->prob_comppred);

  vpx_memcpy(cpi->coding_context.last_frame_seg_map_copy,
             cm->last_frame_seg_map, (cm->mb_rows * cm->mb_cols));

  vp9_copy(cc->last_ref_lf_deltas, xd->last_ref_lf_deltas);
  vp9_copy(cc->last_mode_lf_deltas, xd->last_mode_lf_deltas);

  vp9_copy(cc->coef_probs_4x4, cm->fc.coef_probs_4x4);
  vp9_copy(cc->hybrid_coef_probs_4x4, cm->fc.hybrid_coef_probs_4x4);
  vp9_copy(cc->coef_probs_8x8, cm->fc.coef_probs_8x8);
  vp9_copy(cc->hybrid_coef_probs_8x8, cm->fc.hybrid_coef_probs_8x8);
  vp9_copy(cc->coef_probs_16x16, cm->fc.coef_probs_16x16);
  vp9_copy(cc->hybrid_coef_probs_16x16, cm->fc.hybrid_coef_probs_16x16);
  vp9_copy(cc->coef_probs_32x32, cm->fc.coef_probs_32x32);
  vp9_copy(cc->switchable_interp_prob, cm->fc.switchable_interp_prob);
#if CONFIG_COMP_INTERINTRA_PRED
  cc->interintra_prob = cm->fc.interintra_prob;
#endif
}

void vp9_restore_coding_context(VP9_COMP *cpi) {
  CODING_CONTEXT *const cc = &cpi->coding_context;
  VP9_COMMON *cm = &cpi->common;
  MACROBLOCKD *xd = &cpi->mb.e_mbd;

  // Restore key state variables to the snapshot state stored in the
  // previous call to vp9_save_coding_context.

  cm->fc.nmvc = cc->nmvc;
  vp9_copy(cpi->mb.nmvjointcost, cc->nmvjointcost);
  vp9_copy(cpi->mb.nmvcosts, cc->nmvcosts);
  vp9_copy(cpi->mb.nmvcosts_hp, cc->nmvcosts_hp);

  vp9_copy(cm->fc.vp9_mode_contexts, cc->vp9_mode_contexts);

  vp9_copy(cm->fc.ymode_prob, cc->ymode_prob);
  vp9_copy(cm->fc.sb_ymode_prob, cc->sb_ymode_prob);
  vp9_copy(cm->fc.bmode_prob, cc->bmode_prob);
  vp9_copy(cm->fc.i8x8_mode_prob, cc->i8x8_mode_prob);
  vp9_copy(cm->fc.uv_mode_prob, cc->uv_mode_prob);
  vp9_copy(cm->fc.sub_mv_ref_prob, cc->sub_mv_ref_prob);
  vp9_copy(cm->fc.mbsplit_prob, cc->mbsplit_prob);

  // Stats
#ifdef MODE_STATS
  vp9_copy(y_modes, cc->y_modes);
  vp9_copy(uv_modes, cc->uv_modes);
  vp9_copy(b_modes, cc->b_modes);
  vp9_copy(inter_y_modes, cc->inter_y_modes);
  vp9_copy(inter_uv_modes, cc->inter_uv_modes);
  vp9_copy(inter_b_modes, cc->inter_b_modes);
#endif

  vp9_copy(cm->segment_pred_probs, cc->segment_pred_probs);
  vp9_copy(cpi->ref_pred_probs_update, cc->ref_pred_probs_update);
  vp9_copy(cm->ref_pred_probs, cc->ref_pred_probs);
  vp9_copy(cm->prob_comppred, cc->prob_comppred);

  vpx_memcpy(cm->last_frame_seg_map,
             cpi->coding_context.last_frame_seg_map_copy,
             (cm->mb_rows * cm->mb_cols));

  vp9_copy(xd->last_ref_lf_deltas, cc->last_ref_lf_deltas);
  vp9_copy(xd->last_mode_lf_deltas, cc->last_mode_lf_deltas);

  vp9_copy(cm->fc.coef_probs_4x4, cc->coef_probs_4x4);
  vp9_copy(cm->fc.hybrid_coef_probs_4x4, cc->hybrid_coef_probs_4x4);
  vp9_copy(cm->fc.coef_probs_8x8, cc->coef_probs_8x8);
  vp9_copy(cm->fc.hybrid_coef_probs_8x8, cc->hybrid_coef_probs_8x8);
  vp9_copy(cm->fc.coef_probs_16x16, cc->coef_probs_16x16);
  vp9_copy(cm->fc.hybrid_coef_probs_16x16, cc->hybrid_coef_probs_16x16);
  vp9_copy(cm->fc.coef_probs_32x32, cc->coef_probs_32x32);
  vp9_copy(cm->fc.switchable_interp_prob, cc->switchable_interp_prob);
#if CONFIG_COMP_INTERINTRA_PRED
  cm->fc.interintra_prob = cc->interintra_prob;
#endif
}


void vp9_setup_key_frame(VP9_COMP *cpi) {
  VP9_COMMON *cm = &cpi->common;
  // Setup for Key frame:
  vp9_default_coef_probs(& cpi->common);
  vp9_kf_default_bmode_probs(cpi->common.kf_bmode_prob);
  vp9_init_mbmode_probs(& cpi->common);
  vp9_default_bmode_probs(cm->fc.bmode_prob);

  if(cm->last_frame_seg_map)
    vpx_memset(cm->last_frame_seg_map, 0, (cm->mb_rows * cm->mb_cols));

  vp9_init_mv_probs(& cpi->common);

  // cpi->common.filter_level = 0;      // Reset every key frame.
  cpi->common.filter_level = cpi->common.base_qindex * 3 / 8;

  // interval before next GF
  cpi->frames_till_gf_update_due = cpi->baseline_gf_interval;

  cpi->common.refresh_golden_frame = TRUE;
  cpi->common.refresh_alt_ref_frame = TRUE;

  vp9_init_mode_contexts(&cpi->common);
  vpx_memcpy(&cpi->common.lfc, &cpi->common.fc, sizeof(cpi->common.fc));
  vpx_memcpy(&cpi->common.lfc_a, &cpi->common.fc, sizeof(cpi->common.fc));

  vpx_memset(cm->prev_mip, 0,
    (cm->mb_cols + 1) * (cm->mb_rows + 1)* sizeof(MODE_INFO));
  vpx_memset(cm->mip, 0,
    (cm->mb_cols + 1) * (cm->mb_rows + 1)* sizeof(MODE_INFO));

  vp9_update_mode_info_border(cm, cm->mip);
  vp9_update_mode_info_in_image(cm, cm->mi);

#if CONFIG_NEW_MVREF
  if (1) {
    MACROBLOCKD *xd = &cpi->mb.e_mbd;

    // Defaults probabilities for encoding the MV ref id signal
    vpx_memset(xd->mb_mv_ref_probs, VP9_DEFAULT_MV_REF_PROB,
               sizeof(xd->mb_mv_ref_probs));
  }
#endif
}

void vp9_setup_inter_frame(VP9_COMP *cpi) {
  if (cpi->common.refresh_alt_ref_frame) {
    vpx_memcpy(&cpi->common.fc,
               &cpi->common.lfc_a,
               sizeof(cpi->common.fc));
  } else {
    vpx_memcpy(&cpi->common.fc,
               &cpi->common.lfc,
               sizeof(cpi->common.fc));
  }
}


static int estimate_bits_at_q(int frame_kind, int Q, int MBs,
                              double correction_factor) {
  int Bpm = (int)(.5 + correction_factor * vp9_bits_per_mb(frame_kind, Q));

  /* Attempt to retain reasonable accuracy without overflow. The cutoff is
   * chosen such that the maximum product of Bpm and MBs fits 31 bits. The
   * largest Bpm takes 20 bits.
   */
  if (MBs > (1 << 11))
    return (Bpm >> BPER_MB_NORMBITS) * MBs;
  else
    return (Bpm * MBs) >> BPER_MB_NORMBITS;
}


static void calc_iframe_target_size(VP9_COMP *cpi) {
  // boost defaults to half second
  int target;

  // Clear down mmx registers to allow floating point in what follows
  vp9_clear_system_state();  // __asm emms;

  // New Two pass RC
  target = cpi->per_frame_bandwidth;

  if (cpi->oxcf.rc_max_intra_bitrate_pct) {
    int max_rate = cpi->per_frame_bandwidth
                 * cpi->oxcf.rc_max_intra_bitrate_pct / 100;

    if (target > max_rate)
      target = max_rate;
  }

  cpi->this_frame_target = target;

}


//  Do the best we can to define the parameteres for the next GF based
//  on what information we have available.
//
//  In this experimental code only two pass is supported
//  so we just use the interval determined in the two pass code.
static void calc_gf_params(VP9_COMP *cpi) {
  // Set the gf interval
  cpi->frames_till_gf_update_due = cpi->baseline_gf_interval;
}


static void calc_pframe_target_size(VP9_COMP *cpi) {
  int min_frame_target;

  min_frame_target = 0;

  min_frame_target = cpi->min_frame_bandwidth;

  if (min_frame_target < (cpi->av_per_frame_bandwidth >> 5))
    min_frame_target = cpi->av_per_frame_bandwidth >> 5;


  // Special alt reference frame case
  if (cpi->common.refresh_alt_ref_frame) {
    // Per frame bit target for the alt ref frame
    cpi->per_frame_bandwidth = cpi->twopass.gf_bits;
    cpi->this_frame_target = cpi->per_frame_bandwidth;
  }

  // Normal frames (gf,and inter)
  else {
    cpi->this_frame_target = cpi->per_frame_bandwidth;
  }

  // Sanity check that the total sum of adjustments is not above the maximum allowed
  // That is that having allowed for KF and GF penalties we have not pushed the
  // current interframe target to low. If the adjustment we apply here is not capable of recovering
  // all the extra bits we have spent in the KF or GF then the remainder will have to be recovered over
  // a longer time span via other buffer / rate control mechanisms.
  if (cpi->this_frame_target < min_frame_target)
    cpi->this_frame_target = min_frame_target;

  if (!cpi->common.refresh_alt_ref_frame)
    // Note the baseline target data rate for this inter frame.
    cpi->inter_frame_target = cpi->this_frame_target;

  // Adjust target frame size for Golden Frames:
  if (cpi->frames_till_gf_update_due == 0) {
    // int Boost = 0;
    int Q = (cpi->oxcf.fixed_q < 0) ? cpi->last_q[INTER_FRAME] : cpi->oxcf.fixed_q;

    cpi->common.refresh_golden_frame = TRUE;

    calc_gf_params(cpi);

    // If we are using alternate ref instead of gf then do not apply the boost
    // It will instead be applied to the altref update
    // Jims modified boost
    if (!cpi->source_alt_ref_active) {
      if (cpi->oxcf.fixed_q < 0) {
        // The spend on the GF is defined in the two pass code
        // for two pass encodes
        cpi->this_frame_target = cpi->per_frame_bandwidth;
      } else
        cpi->this_frame_target =
          (estimate_bits_at_q(1, Q, cpi->common.MBs, 1.0)
           * cpi->last_boost) / 100;

    }
    // If there is an active ARF at this location use the minimum
    // bits on this frame even if it is a contructed arf.
    // The active maximum quantizer insures that an appropriate
    // number of bits will be spent if needed for contstructed ARFs.
    else {
      cpi->this_frame_target = 0;
    }

    cpi->current_gf_interval = cpi->frames_till_gf_update_due;
  }
}


void vp9_update_rate_correction_factors(VP9_COMP *cpi, int damp_var) {
  int    Q = cpi->common.base_qindex;
  int    correction_factor = 100;
  double rate_correction_factor;
  double adjustment_limit;

  int    projected_size_based_on_q = 0;

  // Clear down mmx registers to allow floating point in what follows
  vp9_clear_system_state();  // __asm emms;

  if (cpi->common.frame_type == KEY_FRAME) {
    rate_correction_factor = cpi->key_frame_rate_correction_factor;
  } else {
    if (cpi->common.refresh_alt_ref_frame || cpi->common.refresh_golden_frame)
      rate_correction_factor = cpi->gf_rate_correction_factor;
    else
      rate_correction_factor = cpi->rate_correction_factor;
  }

  // Work out how big we would have expected the frame to be at this Q given the current correction factor.
  // Stay in double to avoid int overflow when values are large
  projected_size_based_on_q =
    (int)(((.5 + rate_correction_factor *
            vp9_bits_per_mb(cpi->common.frame_type, Q)) *
           cpi->common.MBs) / (1 << BPER_MB_NORMBITS));

  // Make some allowance for cpi->zbin_over_quant
  if (cpi->zbin_over_quant > 0) {
    int Z = cpi->zbin_over_quant;
    double Factor = 0.99;
    double factor_adjustment = 0.01 / 256.0; // (double)ZBIN_OQ_MAX;

    while (Z > 0) {
      Z--;
      projected_size_based_on_q =
        (int)(Factor * projected_size_based_on_q);
      Factor += factor_adjustment;

      if (Factor  >= 0.999)
        Factor = 0.999;
    }
  }

  // Work out a size correction factor.
  // if ( cpi->this_frame_target > 0 )
  //  correction_factor = (100 * cpi->projected_frame_size) / cpi->this_frame_target;
  if (projected_size_based_on_q > 0)
    correction_factor = (100 * cpi->projected_frame_size) / projected_size_based_on_q;

  // More heavily damped adjustment used if we have been oscillating either side of target
  switch (damp_var) {
    case 0:
      adjustment_limit = 0.75;
      break;
    case 1:
      adjustment_limit = 0.375;
      break;
    case 2:
    default:
      adjustment_limit = 0.25;
      break;
  }

  // if ( (correction_factor > 102) && (Q < cpi->active_worst_quality) )
  if (correction_factor > 102) {
    // We are not already at the worst allowable quality
    correction_factor = (int)(100.5 + ((correction_factor - 100) * adjustment_limit));
    rate_correction_factor = ((rate_correction_factor * correction_factor) / 100);

    // Keep rate_correction_factor within limits
    if (rate_correction_factor > MAX_BPB_FACTOR)
      rate_correction_factor = MAX_BPB_FACTOR;
  }
  // else if ( (correction_factor < 99) && (Q > cpi->active_best_quality) )
  else if (correction_factor < 99) {
    // We are not already at the best allowable quality
    correction_factor = (int)(100.5 - ((100 - correction_factor) * adjustment_limit));
    rate_correction_factor = ((rate_correction_factor * correction_factor) / 100);

    // Keep rate_correction_factor within limits
    if (rate_correction_factor < MIN_BPB_FACTOR)
      rate_correction_factor = MIN_BPB_FACTOR;
  }

  if (cpi->common.frame_type == KEY_FRAME)
    cpi->key_frame_rate_correction_factor = rate_correction_factor;
  else {
    if (cpi->common.refresh_alt_ref_frame || cpi->common.refresh_golden_frame)
      cpi->gf_rate_correction_factor = rate_correction_factor;
    else
      cpi->rate_correction_factor = rate_correction_factor;
  }
}


int vp9_regulate_q(VP9_COMP *cpi, int target_bits_per_frame) {
  int Q = cpi->active_worst_quality;

  int i;
  int last_error = INT_MAX;
  int target_bits_per_mb;
  int bits_per_mb_at_this_q;
  double correction_factor;

  // Reset Zbin OQ value
  cpi->zbin_over_quant = 0;

  // Select the appropriate correction factor based upon type of frame.
  if (cpi->common.frame_type == KEY_FRAME)
    correction_factor = cpi->key_frame_rate_correction_factor;
  else {
    if (cpi->common.refresh_alt_ref_frame || cpi->common.refresh_golden_frame)
      correction_factor = cpi->gf_rate_correction_factor;
    else
      correction_factor = cpi->rate_correction_factor;
  }

  // Calculate required scaling factor based on target frame size and size of frame produced using previous Q
  if (target_bits_per_frame >= (INT_MAX >> BPER_MB_NORMBITS))
    target_bits_per_mb = (target_bits_per_frame / cpi->common.MBs) << BPER_MB_NORMBITS;       // Case where we would overflow int
  else
    target_bits_per_mb = (target_bits_per_frame << BPER_MB_NORMBITS) / cpi->common.MBs;

  i = cpi->active_best_quality;

  do {
    bits_per_mb_at_this_q =
      (int)(.5 + correction_factor *
            vp9_bits_per_mb(cpi->common.frame_type, i));

    if (bits_per_mb_at_this_q <= target_bits_per_mb) {
      if ((target_bits_per_mb - bits_per_mb_at_this_q) <= last_error)
        Q = i;
      else
        Q = i - 1;

      break;
    } else
      last_error = bits_per_mb_at_this_q - target_bits_per_mb;
  } while (++i <= cpi->active_worst_quality);


  // If we are at MAXQ then enable Q over-run which seeks to claw back additional bits through things like
  // the RD multiplier and zero bin size.
  if (Q >= MAXQ) {
    int zbin_oqmax;

    double Factor = 0.99;
    double factor_adjustment = 0.01 / 256.0; // (double)ZBIN_OQ_MAX;

    if (cpi->common.frame_type == KEY_FRAME)
      zbin_oqmax = 0; // ZBIN_OQ_MAX/16
    else if (cpi->common.refresh_alt_ref_frame || (cpi->common.refresh_golden_frame && !cpi->source_alt_ref_active))
      zbin_oqmax = 16;
    else
      zbin_oqmax = ZBIN_OQ_MAX;

    // Each incrment in the zbin is assumed to have a fixed effect on bitrate. This is not of course true.
    // The effect will be highly clip dependent and may well have sudden steps.
    // The idea here is to acheive higher effective quantizers than the normal maximum by expanding the zero
    // bin and hence decreasing the number of low magnitude non zero coefficients.
    while (cpi->zbin_over_quant < zbin_oqmax) {
      cpi->zbin_over_quant++;

      if (cpi->zbin_over_quant > zbin_oqmax)
        cpi->zbin_over_quant = zbin_oqmax;

      // Adjust bits_per_mb_at_this_q estimate
      bits_per_mb_at_this_q = (int)(Factor * bits_per_mb_at_this_q);
      Factor += factor_adjustment;

      if (Factor  >= 0.999)
        Factor = 0.999;

      if (bits_per_mb_at_this_q <= target_bits_per_mb)    // Break out if we get down to the target rate
        break;
    }

  }

  return Q;
}


static int estimate_keyframe_frequency(VP9_COMP *cpi) {
  int i;

  // Average key frame frequency
  int av_key_frame_frequency = 0;

  /* First key frame at start of sequence is a special case. We have no
   * frequency data.
   */
  if (cpi->key_frame_count == 1) {
    /* Assume a default of 1 kf every 2 seconds, or the max kf interval,
     * whichever is smaller.
     */
    int key_freq = cpi->oxcf.key_freq > 0 ? cpi->oxcf.key_freq : 1;
    av_key_frame_frequency = (int)cpi->output_frame_rate * 2;

    if (cpi->oxcf.auto_key && av_key_frame_frequency > key_freq)
      av_key_frame_frequency = cpi->oxcf.key_freq;

    cpi->prior_key_frame_distance[KEY_FRAME_CONTEXT - 1]
      = av_key_frame_frequency;
  } else {
    unsigned int total_weight = 0;
    int last_kf_interval =
      (cpi->frames_since_key > 0) ? cpi->frames_since_key : 1;

    /* reset keyframe context and calculate weighted average of last
     * KEY_FRAME_CONTEXT keyframes
     */
    for (i = 0; i < KEY_FRAME_CONTEXT; i++) {
      if (i < KEY_FRAME_CONTEXT - 1)
        cpi->prior_key_frame_distance[i]
          = cpi->prior_key_frame_distance[i + 1];
      else
        cpi->prior_key_frame_distance[i] = last_kf_interval;

      av_key_frame_frequency += prior_key_frame_weight[i]
                                * cpi->prior_key_frame_distance[i];
      total_weight += prior_key_frame_weight[i];
    }

    av_key_frame_frequency  /= total_weight;

  }
  return av_key_frame_frequency;
}


void vp9_adjust_key_frame_context(VP9_COMP *cpi) {
  // Clear down mmx registers to allow floating point in what follows
  vp9_clear_system_state();

  cpi->frames_since_key = 0;
  cpi->key_frame_count++;
}


void vp9_compute_frame_size_bounds(VP9_COMP *cpi, int *frame_under_shoot_limit,
                                   int *frame_over_shoot_limit) {
  // Set-up bounds on acceptable frame size:
  if (cpi->oxcf.fixed_q >= 0) {
    // Fixed Q scenario: frame size never outranges target (there is no target!)
    *frame_under_shoot_limit = 0;
    *frame_over_shoot_limit  = INT_MAX;
  } else {
    if (cpi->common.frame_type == KEY_FRAME) {
      *frame_over_shoot_limit  = cpi->this_frame_target * 9 / 8;
      *frame_under_shoot_limit = cpi->this_frame_target * 7 / 8;
    } else {
      if (cpi->common.refresh_alt_ref_frame || cpi->common.refresh_golden_frame) {
        *frame_over_shoot_limit  = cpi->this_frame_target * 9 / 8;
        *frame_under_shoot_limit = cpi->this_frame_target * 7 / 8;
      } else {
        // Stron overshoot limit for constrained quality
        if (cpi->oxcf.end_usage == USAGE_CONSTRAINED_QUALITY) {
          *frame_over_shoot_limit  = cpi->this_frame_target * 11 / 8;
          *frame_under_shoot_limit = cpi->this_frame_target * 2 / 8;
        } else {
          *frame_over_shoot_limit  = cpi->this_frame_target * 11 / 8;
          *frame_under_shoot_limit = cpi->this_frame_target * 5 / 8;
        }
      }
    }

    // For very small rate targets where the fractional adjustment
    // (eg * 7/8) may be tiny make sure there is at least a minimum
    // range.
    *frame_over_shoot_limit += 200;
    *frame_under_shoot_limit -= 200;
    if (*frame_under_shoot_limit < 0)
      *frame_under_shoot_limit = 0;
  }
}


// return of 0 means drop frame
int vp9_pick_frame_size(VP9_COMP *cpi) {
  VP9_COMMON *cm = &cpi->common;

  if (cm->frame_type == KEY_FRAME)
    calc_iframe_target_size(cpi);
  else
    calc_pframe_target_size(cpi);

  return 1;
}
