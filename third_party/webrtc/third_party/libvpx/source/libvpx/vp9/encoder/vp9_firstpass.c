/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "math.h"
#include "limits.h"
#include "vp9/encoder/vp9_block.h"
#include "vp9/encoder/vp9_onyx_int.h"
#include "vp9/encoder/vp9_variance.h"
#include "vp9/encoder/vp9_encodeintra.h"
#include "vp9/common/vp9_setupintrarecon.h"
#include "vp9/encoder/vp9_mcomp.h"
#include "vp9/encoder/vp9_firstpass.h"
#include "vpx_scale/vpx_scale.h"
#include "vp9/encoder/vp9_encodeframe.h"
#include "vp9/encoder/vp9_encodemb.h"
#include "vp9/common/vp9_extend.h"
#include "vp9/common/vp9_systemdependent.h"
#include "vpx_mem/vpx_mem.h"
#include "vp9/common/vp9_swapyv12buffer.h"
#include <stdio.h>
#include "vp9/encoder/vp9_quantize.h"
#include "vp9/encoder/vp9_rdopt.h"
#include "vp9/encoder/vp9_ratectrl.h"
#include "vp9/common/vp9_quant_common.h"
#include "vp9/common/vp9_entropymv.h"
#include "vp9/encoder/vp9_encodemv.h"
#include "./vpx_scale_rtcd.h"

#define OUTPUT_FPF 0

#define IIFACTOR   12.5
#define IIKFACTOR1 12.5
#define IIKFACTOR2 15.0
#define RMAX       128.0
#define GF_RMAX    96.0
#define ERR_DIVISOR   150.0
#define MIN_DECAY_FACTOR 0.1

#define KF_MB_INTRA_MIN 150
#define GF_MB_INTRA_MIN 100

#define DOUBLE_DIVIDE_CHECK(X) ((X)<0?(X)-.000001:(X)+.000001)

#define POW1 (double)cpi->oxcf.two_pass_vbrbias/100.0
#define POW2 (double)cpi->oxcf.two_pass_vbrbias/100.0

static void find_next_key_frame(VP9_COMP *cpi, FIRSTPASS_STATS *this_frame);

static int select_cq_level(int qindex) {
  int ret_val = QINDEX_RANGE - 1;
  int i;

  double target_q = (vp9_convert_qindex_to_q(qindex) * 0.5847) + 1.0;

  for (i = 0; i < QINDEX_RANGE; i++) {
    if (target_q <= vp9_convert_qindex_to_q(i)) {
      ret_val = i;
      break;
    }
  }

  return ret_val;
}


// Resets the first pass file to the given position using a relative seek from the current position
static void reset_fpf_position(VP9_COMP *cpi, FIRSTPASS_STATS *Position) {
  cpi->twopass.stats_in = Position;
}

static int lookup_next_frame_stats(VP9_COMP *cpi, FIRSTPASS_STATS *next_frame) {
  if (cpi->twopass.stats_in >= cpi->twopass.stats_in_end)
    return EOF;

  *next_frame = *cpi->twopass.stats_in;
  return 1;
}

// Read frame stats at an offset from the current position
static int read_frame_stats(VP9_COMP *cpi,
                            FIRSTPASS_STATS *frame_stats,
                            int offset) {
  FIRSTPASS_STATS *fps_ptr = cpi->twopass.stats_in;

  // Check legality of offset
  if (offset >= 0) {
    if (&fps_ptr[offset] >= cpi->twopass.stats_in_end)
      return EOF;
  } else if (offset < 0) {
    if (&fps_ptr[offset] < cpi->twopass.stats_in_start)
      return EOF;
  }

  *frame_stats = fps_ptr[offset];
  return 1;
}

static int input_stats(VP9_COMP *cpi, FIRSTPASS_STATS *fps) {
  if (cpi->twopass.stats_in >= cpi->twopass.stats_in_end)
    return EOF;

  *fps = *cpi->twopass.stats_in;
  cpi->twopass.stats_in =
    (void *)((char *)cpi->twopass.stats_in + sizeof(FIRSTPASS_STATS));
  return 1;
}

static void output_stats(const VP9_COMP            *cpi,
                         struct vpx_codec_pkt_list *pktlist,
                         FIRSTPASS_STATS            *stats) {
  struct vpx_codec_cx_pkt pkt;
  pkt.kind = VPX_CODEC_STATS_PKT;
  pkt.data.twopass_stats.buf = stats;
  pkt.data.twopass_stats.sz = sizeof(FIRSTPASS_STATS);
  vpx_codec_pkt_list_add(pktlist, &pkt);

// TEMP debug code
#if OUTPUT_FPF

  {
    FILE *fpfile;
    fpfile = fopen("firstpass.stt", "a");

    fprintf(fpfile, "%12.0f %12.0f %12.0f %12.0f %12.0f %12.4f %12.4f"
            "%12.4f %12.4f %12.4f %12.4f %12.4f %12.4f %12.4f"
            "%12.0f %12.0f %12.4f %12.0f %12.0f %12.4f\n",
            stats->frame,
            stats->intra_error,
            stats->coded_error,
            stats->sr_coded_error,
            stats->ssim_weighted_pred_err,
            stats->pcnt_inter,
            stats->pcnt_motion,
            stats->pcnt_second_ref,
            stats->pcnt_neutral,
            stats->MVr,
            stats->mvr_abs,
            stats->MVc,
            stats->mvc_abs,
            stats->MVrv,
            stats->MVcv,
            stats->mv_in_out_count,
            stats->new_mv_count,
            stats->count,
            stats->duration);
    fclose(fpfile);
  }
#endif
}

static void zero_stats(FIRSTPASS_STATS *section) {
  section->frame      = 0.0;
  section->intra_error = 0.0;
  section->coded_error = 0.0;
  section->sr_coded_error = 0.0;
  section->ssim_weighted_pred_err = 0.0;
  section->pcnt_inter  = 0.0;
  section->pcnt_motion  = 0.0;
  section->pcnt_second_ref = 0.0;
  section->pcnt_neutral = 0.0;
  section->MVr        = 0.0;
  section->mvr_abs     = 0.0;
  section->MVc        = 0.0;
  section->mvc_abs     = 0.0;
  section->MVrv       = 0.0;
  section->MVcv       = 0.0;
  section->mv_in_out_count  = 0.0;
  section->new_mv_count = 0.0;
  section->count      = 0.0;
  section->duration   = 1.0;
}

static void accumulate_stats(FIRSTPASS_STATS *section, FIRSTPASS_STATS *frame) {
  section->frame += frame->frame;
  section->intra_error += frame->intra_error;
  section->coded_error += frame->coded_error;
  section->sr_coded_error += frame->sr_coded_error;
  section->ssim_weighted_pred_err += frame->ssim_weighted_pred_err;
  section->pcnt_inter  += frame->pcnt_inter;
  section->pcnt_motion += frame->pcnt_motion;
  section->pcnt_second_ref += frame->pcnt_second_ref;
  section->pcnt_neutral += frame->pcnt_neutral;
  section->MVr        += frame->MVr;
  section->mvr_abs     += frame->mvr_abs;
  section->MVc        += frame->MVc;
  section->mvc_abs     += frame->mvc_abs;
  section->MVrv       += frame->MVrv;
  section->MVcv       += frame->MVcv;
  section->mv_in_out_count  += frame->mv_in_out_count;
  section->new_mv_count += frame->new_mv_count;
  section->count      += frame->count;
  section->duration   += frame->duration;
}

static void subtract_stats(FIRSTPASS_STATS *section, FIRSTPASS_STATS *frame) {
  section->frame -= frame->frame;
  section->intra_error -= frame->intra_error;
  section->coded_error -= frame->coded_error;
  section->sr_coded_error -= frame->sr_coded_error;
  section->ssim_weighted_pred_err -= frame->ssim_weighted_pred_err;
  section->pcnt_inter  -= frame->pcnt_inter;
  section->pcnt_motion -= frame->pcnt_motion;
  section->pcnt_second_ref -= frame->pcnt_second_ref;
  section->pcnt_neutral -= frame->pcnt_neutral;
  section->MVr        -= frame->MVr;
  section->mvr_abs     -= frame->mvr_abs;
  section->MVc        -= frame->MVc;
  section->mvc_abs     -= frame->mvc_abs;
  section->MVrv       -= frame->MVrv;
  section->MVcv       -= frame->MVcv;
  section->mv_in_out_count  -= frame->mv_in_out_count;
  section->new_mv_count -= frame->new_mv_count;
  section->count      -= frame->count;
  section->duration   -= frame->duration;
}

static void avg_stats(FIRSTPASS_STATS *section) {
  if (section->count < 1.0)
    return;

  section->intra_error /= section->count;
  section->coded_error /= section->count;
  section->sr_coded_error /= section->count;
  section->ssim_weighted_pred_err /= section->count;
  section->pcnt_inter  /= section->count;
  section->pcnt_second_ref /= section->count;
  section->pcnt_neutral /= section->count;
  section->pcnt_motion /= section->count;
  section->MVr        /= section->count;
  section->mvr_abs     /= section->count;
  section->MVc        /= section->count;
  section->mvc_abs     /= section->count;
  section->MVrv       /= section->count;
  section->MVcv       /= section->count;
  section->mv_in_out_count   /= section->count;
  section->duration   /= section->count;
}

// Calculate a modified Error used in distributing bits between easier and harder frames
static double calculate_modified_err(VP9_COMP *cpi, FIRSTPASS_STATS *this_frame) {
  double av_err = (cpi->twopass.total_stats->ssim_weighted_pred_err /
                   cpi->twopass.total_stats->count);
  double this_err = this_frame->ssim_weighted_pred_err;
  double modified_err;

  if (this_err > av_err)
    modified_err = av_err * pow((this_err / DOUBLE_DIVIDE_CHECK(av_err)), POW1);
  else
    modified_err = av_err * pow((this_err / DOUBLE_DIVIDE_CHECK(av_err)), POW2);

  return modified_err;
}

static const double weight_table[256] = {
  0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
  0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
  0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
  0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
  0.020000, 0.031250, 0.062500, 0.093750, 0.125000, 0.156250, 0.187500, 0.218750,
  0.250000, 0.281250, 0.312500, 0.343750, 0.375000, 0.406250, 0.437500, 0.468750,
  0.500000, 0.531250, 0.562500, 0.593750, 0.625000, 0.656250, 0.687500, 0.718750,
  0.750000, 0.781250, 0.812500, 0.843750, 0.875000, 0.906250, 0.937500, 0.968750,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000
};

static double simple_weight(YV12_BUFFER_CONFIG *source) {
  int i, j;

  uint8_t *src = source->y_buffer;
  double sum_weights = 0.0;

  // Loop throught the Y plane raw examining levels and creating a weight for the image
  i = source->y_height;
  do {
    j = source->y_width;
    do {
      sum_weights += weight_table[ *src];
      src++;
    } while (--j);
    src -= source->y_width;
    src += source->y_stride;
  } while (--i);

  sum_weights /= (source->y_height * source->y_width);

  return sum_weights;
}


// This function returns the current per frame maximum bitrate target
static int frame_max_bits(VP9_COMP *cpi) {
  // Max allocation for a single frame based on the max section guidelines passed in and how many bits are left
  int max_bits;

  // For VBR base this on the bits and frames left plus the two_pass_vbrmax_section rate passed in by the user
  max_bits = (int)(((double)cpi->twopass.bits_left / (cpi->twopass.total_stats->count - (double)cpi->common.current_video_frame)) * ((double)cpi->oxcf.two_pass_vbrmax_section / 100.0));

  // Trap case where we are out of bits
  if (max_bits < 0)
    max_bits = 0;

  return max_bits;
}

void vp9_init_first_pass(VP9_COMP *cpi) {
  zero_stats(cpi->twopass.total_stats);
}

void vp9_end_first_pass(VP9_COMP *cpi) {
  output_stats(cpi, cpi->output_pkt_list, cpi->twopass.total_stats);
}

static void zz_motion_search(VP9_COMP *cpi, MACROBLOCK *x, YV12_BUFFER_CONFIG *recon_buffer, int *best_motion_err, int recon_yoffset) {
  MACROBLOCKD *const xd = &x->e_mbd;
  BLOCK *b = &x->block[0];
  BLOCKD *d = &x->e_mbd.block[0];

  uint8_t *src_ptr = (*(b->base_src) + b->src);
  int src_stride = b->src_stride;
  uint8_t *ref_ptr;
  int ref_stride = d->pre_stride;

  // Set up pointers for this macro block recon buffer
  xd->pre.y_buffer = recon_buffer->y_buffer + recon_yoffset;

  ref_ptr = (uint8_t *)(*(d->base_pre) + d->pre);

  vp9_mse16x16(src_ptr, src_stride, ref_ptr, ref_stride,
               (unsigned int *)(best_motion_err));
}

static void first_pass_motion_search(VP9_COMP *cpi, MACROBLOCK *x,
                                     int_mv *ref_mv, MV *best_mv,
                                     YV12_BUFFER_CONFIG *recon_buffer,
                                     int *best_motion_err, int recon_yoffset) {
  MACROBLOCKD *const xd = &x->e_mbd;
  BLOCK *b = &x->block[0];
  BLOCKD *d = &x->e_mbd.block[0];
  int num00;

  int_mv tmp_mv;
  int_mv ref_mv_full;

  int tmp_err;
  int step_param = 3;
  int further_steps = (MAX_MVSEARCH_STEPS - 1) - step_param;
  int n;
  vp9_variance_fn_ptr_t v_fn_ptr = cpi->fn_ptr[BLOCK_16X16];
  int new_mv_mode_penalty = 256;

  // override the default variance function to use MSE
  v_fn_ptr.vf = vp9_mse16x16;

  // Set up pointers for this macro block recon buffer
  xd->pre.y_buffer = recon_buffer->y_buffer + recon_yoffset;

  // Initial step/diamond search centred on best mv
  tmp_mv.as_int = 0;
  ref_mv_full.as_mv.col = ref_mv->as_mv.col >> 3;
  ref_mv_full.as_mv.row = ref_mv->as_mv.row >> 3;
  tmp_err = cpi->diamond_search_sad(x, b, d, &ref_mv_full, &tmp_mv, step_param,
                                    x->sadperbit16, &num00, &v_fn_ptr,
                                    x->nmvjointcost,
                                    x->mvcost, ref_mv);
  if (tmp_err < INT_MAX - new_mv_mode_penalty)
    tmp_err += new_mv_mode_penalty;

  if (tmp_err < *best_motion_err) {
    *best_motion_err = tmp_err;
    best_mv->row = tmp_mv.as_mv.row;
    best_mv->col = tmp_mv.as_mv.col;
  }

  // Further step/diamond searches as necessary
  n = num00;
  num00 = 0;

  while (n < further_steps) {
    n++;

    if (num00)
      num00--;
    else {
      tmp_err = cpi->diamond_search_sad(x, b, d, &ref_mv_full, &tmp_mv,
                                        step_param + n, x->sadperbit16,
                                        &num00, &v_fn_ptr,
                                        x->nmvjointcost,
                                        x->mvcost, ref_mv);
      if (tmp_err < INT_MAX - new_mv_mode_penalty)
        tmp_err += new_mv_mode_penalty;

      if (tmp_err < *best_motion_err) {
        *best_motion_err = tmp_err;
        best_mv->row = tmp_mv.as_mv.row;
        best_mv->col = tmp_mv.as_mv.col;
      }
    }
  }
}

void vp9_first_pass(VP9_COMP *cpi) {
  int mb_row, mb_col;
  MACROBLOCK *const x = &cpi->mb;
  VP9_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;

  int recon_yoffset, recon_uvoffset;
  YV12_BUFFER_CONFIG *lst_yv12 = &cm->yv12_fb[cm->lst_fb_idx];
  YV12_BUFFER_CONFIG *new_yv12 = &cm->yv12_fb[cm->new_fb_idx];
  YV12_BUFFER_CONFIG *gld_yv12 = &cm->yv12_fb[cm->gld_fb_idx];
  int recon_y_stride = lst_yv12->y_stride;
  int recon_uv_stride = lst_yv12->uv_stride;
  int64_t intra_error = 0;
  int64_t coded_error = 0;
  int64_t sr_coded_error = 0;

  int sum_mvr = 0, sum_mvc = 0;
  int sum_mvr_abs = 0, sum_mvc_abs = 0;
  int sum_mvrs = 0, sum_mvcs = 0;
  int mvcount = 0;
  int intercount = 0;
  int second_ref_count = 0;
  int intrapenalty = 256;
  int neutral_count = 0;
  int new_mv_count = 0;
  int sum_in_vectors = 0;
  uint32_t lastmv_as_int = 0;

  int_mv zero_ref_mv;

  zero_ref_mv.as_int = 0;

  vp9_clear_system_state();  // __asm emms;

  x->src = * cpi->Source;
  xd->pre = *lst_yv12;
  xd->dst = *new_yv12;

  x->partition_info = x->pi;

  xd->mode_info_context = cm->mi;

  vp9_build_block_offsets(x);

  vp9_setup_block_dptrs(&x->e_mbd);

  vp9_setup_block_ptrs(x);

  // set up frame new frame for intra coded blocks
  vp9_setup_intra_recon(new_yv12);
  vp9_frame_init_quantizer(cpi);

  // Initialise the MV cost table to the defaults
  // if( cm->current_video_frame == 0)
  // if ( 0 )
  {
    vp9_init_mv_probs(cm);
    vp9_initialize_rd_consts(cpi, cm->base_qindex + cm->y1dc_delta_q);
  }

  // for each macroblock row in image
  for (mb_row = 0; mb_row < cm->mb_rows; mb_row++) {
    int_mv best_ref_mv;

    best_ref_mv.as_int = 0;

    // reset above block coeffs
    xd->up_available = (mb_row != 0);
    recon_yoffset = (mb_row * recon_y_stride * 16);
    recon_uvoffset = (mb_row * recon_uv_stride * 8);

    // Set up limit values for motion vectors to prevent them extending outside the UMV borders
    x->mv_row_min = -((mb_row * 16) + (VP9BORDERINPIXELS - 16));
    x->mv_row_max = ((cm->mb_rows - 1 - mb_row) * 16)
                    + (VP9BORDERINPIXELS - 16);


    // for each macroblock col in image
    for (mb_col = 0; mb_col < cm->mb_cols; mb_col++) {
      int this_error;
      int gf_motion_error = INT_MAX;
      int use_dc_pred = (mb_col || mb_row) && (!mb_col || !mb_row);

      xd->dst.y_buffer = new_yv12->y_buffer + recon_yoffset;
      xd->dst.u_buffer = new_yv12->u_buffer + recon_uvoffset;
      xd->dst.v_buffer = new_yv12->v_buffer + recon_uvoffset;
      xd->left_available = (mb_col != 0);

      // do intra 16x16 prediction
      this_error = vp9_encode_intra(cpi, x, use_dc_pred);

      // "intrapenalty" below deals with situations where the intra and inter error scores are very low (eg a plain black frame)
      // We do not have special cases in first pass for 0,0 and nearest etc so all inter modes carry an overhead cost estimate fot the mv.
      // When the error score is very low this causes us to pick all or lots of INTRA modes and throw lots of key frames.
      // This penalty adds a cost matching that of a 0,0 mv to the intra case.
      this_error += intrapenalty;

      // Cumulative intra error total
      intra_error += (int64_t)this_error;

      // Set up limit values for motion vectors to prevent them extending outside the UMV borders
      x->mv_col_min = -((mb_col * 16) + (VP9BORDERINPIXELS - 16));
      x->mv_col_max = ((cm->mb_cols - 1 - mb_col) * 16)
                      + (VP9BORDERINPIXELS - 16);

      // Other than for the first frame do a motion search
      if (cm->current_video_frame > 0) {
        int tmp_err;
        int motion_error = INT_MAX;
        int_mv mv, tmp_mv;

        // Simple 0,0 motion with no mv overhead
        zz_motion_search(cpi, x, lst_yv12, &motion_error, recon_yoffset);
        mv.as_int = tmp_mv.as_int = 0;

        // Test last reference frame using the previous best mv as the
        // starting point (best reference) for the search
        first_pass_motion_search(cpi, x, &best_ref_mv,
                                 &mv.as_mv, lst_yv12,
                                 &motion_error, recon_yoffset);

        // If the current best reference mv is not centred on 0,0 then do a 0,0 based search as well
        if (best_ref_mv.as_int) {
          tmp_err = INT_MAX;
          first_pass_motion_search(cpi, x, &zero_ref_mv, &tmp_mv.as_mv,
                                   lst_yv12, &tmp_err, recon_yoffset);

          if (tmp_err < motion_error) {
            motion_error = tmp_err;
            mv.as_int = tmp_mv.as_int;
          }
        }

        // Experimental search in an older reference frame
        if (cm->current_video_frame > 1) {
          // Simple 0,0 motion with no mv overhead
          zz_motion_search(cpi, x, gld_yv12,
                           &gf_motion_error, recon_yoffset);

          first_pass_motion_search(cpi, x, &zero_ref_mv,
                                   &tmp_mv.as_mv, gld_yv12,
                                   &gf_motion_error, recon_yoffset);

          if ((gf_motion_error < motion_error) &&
              (gf_motion_error < this_error)) {
            second_ref_count++;
          }

          // Reset to last frame as reference buffer
          xd->pre.y_buffer = lst_yv12->y_buffer + recon_yoffset;
          xd->pre.u_buffer = lst_yv12->u_buffer + recon_uvoffset;
          xd->pre.v_buffer = lst_yv12->v_buffer + recon_uvoffset;

          // In accumulating a score for the older reference frame
          // take the best of the motion predicted score and
          // the intra coded error (just as will be done for)
          // accumulation of "coded_error" for the last frame.
          if (gf_motion_error < this_error)
            sr_coded_error += gf_motion_error;
          else
            sr_coded_error += this_error;
        } else
          sr_coded_error += motion_error;

        /* Intra assumed best */
        best_ref_mv.as_int = 0;

        if (motion_error <= this_error) {
          // Keep a count of cases where the inter and intra were
          // very close and very low. This helps with scene cut
          // detection for example in cropped clips with black bars
          // at the sides or top and bottom.
          if ((((this_error - intrapenalty) * 9) <=
               (motion_error * 10)) &&
              (this_error < (2 * intrapenalty))) {
            neutral_count++;
          }

          mv.as_mv.row <<= 3;
          mv.as_mv.col <<= 3;
          this_error = motion_error;
          vp9_set_mbmode_and_mvs(x, NEWMV, &mv);
          xd->mode_info_context->mbmi.txfm_size = TX_4X4;
          vp9_encode_inter16x16y(x);
          sum_mvr += mv.as_mv.row;
          sum_mvr_abs += abs(mv.as_mv.row);
          sum_mvc += mv.as_mv.col;
          sum_mvc_abs += abs(mv.as_mv.col);
          sum_mvrs += mv.as_mv.row * mv.as_mv.row;
          sum_mvcs += mv.as_mv.col * mv.as_mv.col;
          intercount++;

          best_ref_mv.as_int = mv.as_int;

          // Was the vector non-zero
          if (mv.as_int) {
            mvcount++;

            // Was it different from the last non zero vector
            if (mv.as_int != lastmv_as_int)
              new_mv_count++;
            lastmv_as_int = mv.as_int;

            // Does the Row vector point inwards or outwards
            if (mb_row < cm->mb_rows / 2) {
              if (mv.as_mv.row > 0)
                sum_in_vectors--;
              else if (mv.as_mv.row < 0)
                sum_in_vectors++;
            } else if (mb_row > cm->mb_rows / 2) {
              if (mv.as_mv.row > 0)
                sum_in_vectors++;
              else if (mv.as_mv.row < 0)
                sum_in_vectors--;
            }

            // Does the Row vector point inwards or outwards
            if (mb_col < cm->mb_cols / 2) {
              if (mv.as_mv.col > 0)
                sum_in_vectors--;
              else if (mv.as_mv.col < 0)
                sum_in_vectors++;
            } else if (mb_col > cm->mb_cols / 2) {
              if (mv.as_mv.col > 0)
                sum_in_vectors++;
              else if (mv.as_mv.col < 0)
                sum_in_vectors--;
            }
          }
        }
      } else
        sr_coded_error += (int64_t)this_error;

      coded_error += (int64_t)this_error;

      // adjust to the next column of macroblocks
      x->src.y_buffer += 16;
      x->src.u_buffer += 8;
      x->src.v_buffer += 8;

      recon_yoffset += 16;
      recon_uvoffset += 8;
    }

    // adjust to the next row of mbs
    x->src.y_buffer += 16 * x->src.y_stride - 16 * cm->mb_cols;
    x->src.u_buffer += 8 * x->src.uv_stride - 8 * cm->mb_cols;
    x->src.v_buffer += 8 * x->src.uv_stride - 8 * cm->mb_cols;

    // extend the recon for intra prediction
    vp9_extend_mb_row(new_yv12, xd->dst.y_buffer + 16,
                      xd->dst.u_buffer + 8, xd->dst.v_buffer + 8);
    vp9_clear_system_state();  // __asm emms;
  }

  vp9_clear_system_state();  // __asm emms;
  {
    double weight = 0.0;

    FIRSTPASS_STATS fps;

    fps.frame      = cm->current_video_frame;
    fps.intra_error = (double)(intra_error >> 8);
    fps.coded_error = (double)(coded_error >> 8);
    fps.sr_coded_error = (double)(sr_coded_error >> 8);
    weight = simple_weight(cpi->Source);


    if (weight < 0.1)
      weight = 0.1;

    fps.ssim_weighted_pred_err = fps.coded_error * weight;

    fps.pcnt_inter  = 0.0;
    fps.pcnt_motion = 0.0;
    fps.MVr        = 0.0;
    fps.mvr_abs     = 0.0;
    fps.MVc        = 0.0;
    fps.mvc_abs     = 0.0;
    fps.MVrv       = 0.0;
    fps.MVcv       = 0.0;
    fps.mv_in_out_count  = 0.0;
    fps.new_mv_count = 0.0;
    fps.count      = 1.0;

    fps.pcnt_inter   = 1.0 * (double)intercount / cm->MBs;
    fps.pcnt_second_ref = 1.0 * (double)second_ref_count / cm->MBs;
    fps.pcnt_neutral = 1.0 * (double)neutral_count / cm->MBs;

    if (mvcount > 0) {
      fps.MVr = (double)sum_mvr / (double)mvcount;
      fps.mvr_abs = (double)sum_mvr_abs / (double)mvcount;
      fps.MVc = (double)sum_mvc / (double)mvcount;
      fps.mvc_abs = (double)sum_mvc_abs / (double)mvcount;
      fps.MVrv = ((double)sum_mvrs - (fps.MVr * fps.MVr / (double)mvcount)) / (double)mvcount;
      fps.MVcv = ((double)sum_mvcs - (fps.MVc * fps.MVc / (double)mvcount)) / (double)mvcount;
      fps.mv_in_out_count = (double)sum_in_vectors / (double)(mvcount * 2);
      fps.new_mv_count = new_mv_count;

      fps.pcnt_motion = 1.0 * (double)mvcount / cpi->common.MBs;
    }

    // TODO:  handle the case when duration is set to 0, or something less
    // than the full time between subsequent cpi->source_time_stamp s  .
    fps.duration = (double)(cpi->source->ts_end
                            - cpi->source->ts_start);

    // don't want to do output stats with a stack variable!
    memcpy(cpi->twopass.this_frame_stats,
           &fps,
           sizeof(FIRSTPASS_STATS));
    output_stats(cpi, cpi->output_pkt_list, cpi->twopass.this_frame_stats);
    accumulate_stats(cpi->twopass.total_stats, &fps);
  }

  // Copy the previous Last Frame back into gf and and arf buffers if
  // the prediction is good enough... but also dont allow it to lag too far
  if ((cpi->twopass.sr_update_lag > 3) ||
      ((cm->current_video_frame > 0) &&
       (cpi->twopass.this_frame_stats->pcnt_inter > 0.20) &&
       ((cpi->twopass.this_frame_stats->intra_error /
         cpi->twopass.this_frame_stats->coded_error) > 2.0))) {
    vp8_yv12_copy_frame(lst_yv12, gld_yv12);
    cpi->twopass.sr_update_lag = 1;
  } else
    cpi->twopass.sr_update_lag++;

  // swap frame pointers so last frame refers to the frame we just compressed
  vp9_swap_yv12_buffer(lst_yv12, new_yv12);
  vp8_yv12_extend_frame_borders(lst_yv12);

  // Special case for the first frame. Copy into the GF buffer as a second reference.
  if (cm->current_video_frame == 0) {
    vp8_yv12_copy_frame(lst_yv12, gld_yv12);
  }


  // use this to see what the first pass reconstruction looks like
  if (0) {
    char filename[512];
    FILE *recon_file;
    sprintf(filename, "enc%04d.yuv", (int) cm->current_video_frame);

    if (cm->current_video_frame == 0)
      recon_file = fopen(filename, "wb");
    else
      recon_file = fopen(filename, "ab");

    (void)fwrite(lst_yv12->buffer_alloc, lst_yv12->frame_size, 1, recon_file);
    fclose(recon_file);
  }

  cm->current_video_frame++;

}

// Estimate a cost per mb attributable to overheads such as the coding of
// modes and motion vectors.
// Currently simplistic in its assumptions for testing.
//


static double bitcost(double prob) {
  return -(log(prob) / log(2.0));
}

static int64_t estimate_modemvcost(VP9_COMP *cpi,
                                     FIRSTPASS_STATS *fpstats) {
#if 0
  int mv_cost;
  int mode_cost;

  double av_pct_inter = fpstats->pcnt_inter / fpstats->count;
  double av_pct_motion = fpstats->pcnt_motion / fpstats->count;
  double av_intra = (1.0 - av_pct_inter);

  double zz_cost;
  double motion_cost;
  double intra_cost;

  zz_cost = bitcost(av_pct_inter - av_pct_motion);
  motion_cost = bitcost(av_pct_motion);
  intra_cost = bitcost(av_intra);

  // Estimate of extra bits per mv overhead for mbs
  // << 9 is the normalization to the (bits * 512) used in vp9_bits_per_mb
  mv_cost = ((int)(fpstats->new_mv_count / fpstats->count) * 8) << 9;

  // Crude estimate of overhead cost from modes
  // << 9 is the normalization to (bits * 512) used in vp9_bits_per_mb
  mode_cost =
    (int)((((av_pct_inter - av_pct_motion) * zz_cost) +
           (av_pct_motion * motion_cost) +
           (av_intra * intra_cost)) * cpi->common.MBs) << 9;

  // return mv_cost + mode_cost;
  // TODO PGW Fix overhead costs for extended Q range
#endif
  return 0;
}

static double calc_correction_factor(double err_per_mb,
                                     double err_divisor,
                                     double pt_low,
                                     double pt_high,
                                     int Q) {
  double power_term;
  double error_term = err_per_mb / err_divisor;
  double correction_factor;

  // Adjustment based on actual quantizer to power term.
  power_term = (vp9_convert_qindex_to_q(Q) * 0.01) + pt_low;
  power_term = (power_term > pt_high) ? pt_high : power_term;

  // Adjustments to error term
  // TBD

  // Calculate correction factor
  correction_factor = pow(error_term, power_term);

  // Clip range
  correction_factor =
    (correction_factor < 0.05)
    ? 0.05 : (correction_factor > 2.0) ? 2.0 : correction_factor;

  return correction_factor;
}

// Given a current maxQ value sets a range for future values.
// PGW TODO..
// This code removes direct dependency on QIndex to determin the range
// (now uses the actual quantizer) but has not been tuned.
static void adjust_maxq_qrange(VP9_COMP *cpi) {
  int i;
  double q;

  // Set the max corresponding to cpi->avg_q * 2.0
  q = cpi->avg_q * 2.0;
  cpi->twopass.maxq_max_limit = cpi->worst_quality;
  for (i = cpi->best_quality; i <= cpi->worst_quality; i++) {
    cpi->twopass.maxq_max_limit = i;
    if (vp9_convert_qindex_to_q(i) >= q)
      break;
  }

  // Set the min corresponding to cpi->avg_q * 0.5
  q = cpi->avg_q * 0.5;
  cpi->twopass.maxq_min_limit = cpi->best_quality;
  for (i = cpi->worst_quality; i >= cpi->best_quality; i--) {
    cpi->twopass.maxq_min_limit = i;
    if (vp9_convert_qindex_to_q(i) <= q)
      break;
  }
}

static int estimate_max_q(VP9_COMP *cpi,
                          FIRSTPASS_STATS *fpstats,
                          int section_target_bandwitdh,
                          int overhead_bits) {
  int Q;
  int num_mbs = cpi->common.MBs;
  int target_norm_bits_per_mb;

  double section_err = (fpstats->coded_error / fpstats->count);
  double sr_err_diff;
  double sr_correction;
  double err_per_mb = section_err / num_mbs;
  double err_correction_factor;
  double speed_correction = 1.0;
  double overhead_bits_per_mb;

  if (section_target_bandwitdh <= 0)
    return cpi->twopass.maxq_max_limit;          // Highest value allowed

  target_norm_bits_per_mb =
    (section_target_bandwitdh < (1 << 20))
    ? (512 * section_target_bandwitdh) / num_mbs
    : 512 * (section_target_bandwitdh / num_mbs);

  // Look at the drop in prediction quality between the last frame
  // and the GF buffer (which contained an older frame).
  sr_err_diff =
    (fpstats->sr_coded_error - fpstats->coded_error) /
    (fpstats->count * cpi->common.MBs);
  sr_correction = (sr_err_diff / 32.0);
  sr_correction = pow(sr_correction, 0.25);
  if (sr_correction < 0.75)
    sr_correction = 0.75;
  else if (sr_correction > 1.25)
    sr_correction = 1.25;

  // Calculate a corrective factor based on a rolling ratio of bits spent
  // vs target bits
  if ((cpi->rolling_target_bits > 0) &&
      (cpi->active_worst_quality < cpi->worst_quality)) {
    double rolling_ratio;

    rolling_ratio = (double)cpi->rolling_actual_bits /
                    (double)cpi->rolling_target_bits;

    if (rolling_ratio < 0.95)
      cpi->twopass.est_max_qcorrection_factor -= 0.005;
    else if (rolling_ratio > 1.05)
      cpi->twopass.est_max_qcorrection_factor += 0.005;

    cpi->twopass.est_max_qcorrection_factor =
      (cpi->twopass.est_max_qcorrection_factor < 0.1)
      ? 0.1
      : (cpi->twopass.est_max_qcorrection_factor > 10.0)
      ? 10.0 : cpi->twopass.est_max_qcorrection_factor;
  }

  // Corrections for higher compression speed settings
  // (reduced compression expected)
  if (cpi->compressor_speed == 1) {
    if (cpi->oxcf.cpu_used <= 5)
      speed_correction = 1.04 + (cpi->oxcf.cpu_used * 0.04);
    else
      speed_correction = 1.25;
  }

  // Estimate of overhead bits per mb
  // Correction to overhead bits for min allowed Q.
  // PGW TODO.. This code is broken for the extended Q range
  //            for now overhead set to 0.
  overhead_bits_per_mb = overhead_bits / num_mbs;
  overhead_bits_per_mb *= pow(0.98, (double)cpi->twopass.maxq_min_limit);

  // Try and pick a max Q that will be high enough to encode the
  // content at the given rate.
  for (Q = cpi->twopass.maxq_min_limit; Q < cpi->twopass.maxq_max_limit; Q++) {
    int bits_per_mb_at_this_q;

    err_correction_factor =
      calc_correction_factor(err_per_mb, ERR_DIVISOR, 0.4, 0.90, Q) *
      sr_correction * speed_correction *
      cpi->twopass.est_max_qcorrection_factor;

    if (err_correction_factor < 0.05)
      err_correction_factor = 0.05;
    else if (err_correction_factor > 5.0)
      err_correction_factor = 5.0;

    bits_per_mb_at_this_q =
      vp9_bits_per_mb(INTER_FRAME, Q) + (int)overhead_bits_per_mb;

    bits_per_mb_at_this_q = (int)(.5 + err_correction_factor *
                                  (double)bits_per_mb_at_this_q);

    // Mode and motion overhead
    // As Q rises in real encode loop rd code will force overhead down
    // We make a crude adjustment for this here as *.98 per Q step.
    // PGW TODO.. This code is broken for the extended Q range
    //            for now overhead set to 0.
    // overhead_bits_per_mb = (int)((double)overhead_bits_per_mb * 0.98);

    if (bits_per_mb_at_this_q <= target_norm_bits_per_mb)
      break;
  }

  // Restriction on active max q for constrained quality mode.
  if ((cpi->oxcf.end_usage == USAGE_CONSTRAINED_QUALITY) &&
      (Q < cpi->cq_target_quality)) {
    Q = cpi->cq_target_quality;
  }

  // Adjust maxq_min_limit and maxq_max_limit limits based on
  // averaga q observed in clip for non kf/gf/arf frames
  // Give average a chance to settle though.
  // PGW TODO.. This code is broken for the extended Q range
  if ((cpi->ni_frames >
       ((int)cpi->twopass.total_stats->count >> 8)) &&
      (cpi->ni_frames > 150)) {
    adjust_maxq_qrange(cpi);
  }

  return Q;
}

// For cq mode estimate a cq level that matches the observed
// complexity and data rate.
static int estimate_cq(VP9_COMP *cpi,
                       FIRSTPASS_STATS *fpstats,
                       int section_target_bandwitdh,
                       int overhead_bits) {
  int Q;
  int num_mbs = cpi->common.MBs;
  int target_norm_bits_per_mb;

  double section_err = (fpstats->coded_error / fpstats->count);
  double err_per_mb = section_err / num_mbs;
  double err_correction_factor;
  double sr_err_diff;
  double sr_correction;
  double speed_correction = 1.0;
  double clip_iiratio;
  double clip_iifactor;
  double overhead_bits_per_mb;


  target_norm_bits_per_mb = (section_target_bandwitdh < (1 << 20))
                            ? (512 * section_target_bandwitdh) / num_mbs
                            : 512 * (section_target_bandwitdh / num_mbs);

  // Estimate of overhead bits per mb
  overhead_bits_per_mb = overhead_bits / num_mbs;

  // Corrections for higher compression speed settings
  // (reduced compression expected)
  if (cpi->compressor_speed == 1) {
    if (cpi->oxcf.cpu_used <= 5)
      speed_correction = 1.04 + (cpi->oxcf.cpu_used * 0.04);
    else
      speed_correction = 1.25;
  }

  // Look at the drop in prediction quality between the last frame
  // and the GF buffer (which contained an older frame).
  sr_err_diff =
    (fpstats->sr_coded_error - fpstats->coded_error) /
    (fpstats->count * cpi->common.MBs);
  sr_correction = (sr_err_diff / 32.0);
  sr_correction = pow(sr_correction, 0.25);
  if (sr_correction < 0.75)
    sr_correction = 0.75;
  else if (sr_correction > 1.25)
    sr_correction = 1.25;

  // II ratio correction factor for clip as a whole
  clip_iiratio = cpi->twopass.total_stats->intra_error /
                 DOUBLE_DIVIDE_CHECK(cpi->twopass.total_stats->coded_error);
  clip_iifactor = 1.0 - ((clip_iiratio - 10.0) * 0.025);
  if (clip_iifactor < 0.80)
    clip_iifactor = 0.80;

  // Try and pick a Q that can encode the content at the given rate.
  for (Q = 0; Q < MAXQ; Q++) {
    int bits_per_mb_at_this_q;

    // Error per MB based correction factor
    err_correction_factor =
      calc_correction_factor(err_per_mb, 100.0, 0.4, 0.90, Q) *
      sr_correction * speed_correction * clip_iifactor;

    if (err_correction_factor < 0.05)
      err_correction_factor = 0.05;
    else if (err_correction_factor > 5.0)
      err_correction_factor = 5.0;

    bits_per_mb_at_this_q =
      vp9_bits_per_mb(INTER_FRAME, Q) + (int)overhead_bits_per_mb;

    bits_per_mb_at_this_q = (int)(.5 + err_correction_factor *
                                  (double)bits_per_mb_at_this_q);

    // Mode and motion overhead
    // As Q rises in real encode loop rd code will force overhead down
    // We make a crude adjustment for this here as *.98 per Q step.
    // PGW TODO.. This code is broken for the extended Q range
    //            for now overhead set to 0.
    overhead_bits_per_mb = (int)((double)overhead_bits_per_mb * 0.98);

    if (bits_per_mb_at_this_q <= target_norm_bits_per_mb)
      break;
  }

  // Clip value to range "best allowed to (worst allowed - 1)"
  Q = select_cq_level(Q);
  if (Q >= cpi->worst_quality)
    Q = cpi->worst_quality - 1;
  if (Q < cpi->best_quality)
    Q = cpi->best_quality;

  return Q;
}


extern void vp9_new_frame_rate(VP9_COMP *cpi, double framerate);

void vp9_init_second_pass(VP9_COMP *cpi) {
  FIRSTPASS_STATS this_frame;
  FIRSTPASS_STATS *start_pos;

  double lower_bounds_min_rate = FRAME_OVERHEAD_BITS * cpi->oxcf.frame_rate;
  double two_pass_min_rate = (double)(cpi->oxcf.target_bandwidth
                                      * cpi->oxcf.two_pass_vbrmin_section / 100);

  if (two_pass_min_rate < lower_bounds_min_rate)
    two_pass_min_rate = lower_bounds_min_rate;

  zero_stats(cpi->twopass.total_stats);
  zero_stats(cpi->twopass.total_left_stats);

  if (!cpi->twopass.stats_in_end)
    return;

  *cpi->twopass.total_stats = *cpi->twopass.stats_in_end;
  *cpi->twopass.total_left_stats = *cpi->twopass.total_stats;

  // each frame can have a different duration, as the frame rate in the source
  // isn't guaranteed to be constant.   The frame rate prior to the first frame
  // encoded in the second pass is a guess.  However the sum duration is not.
  // Its calculated based on the actual durations of all frames from the first
  // pass.
  vp9_new_frame_rate(cpi,
                     10000000.0 * cpi->twopass.total_stats->count /
                     cpi->twopass.total_stats->duration);

  cpi->output_frame_rate = cpi->oxcf.frame_rate;
  cpi->twopass.bits_left = (int64_t)(cpi->twopass.total_stats->duration *
                                     cpi->oxcf.target_bandwidth / 10000000.0);
  cpi->twopass.bits_left -= (int64_t)(cpi->twopass.total_stats->duration *
                                      two_pass_min_rate / 10000000.0);

  // Calculate a minimum intra value to be used in determining the IIratio
  // scores used in the second pass. We have this minimum to make sure
  // that clips that are static but "low complexity" in the intra domain
  // are still boosted appropriately for KF/GF/ARF
  cpi->twopass.kf_intra_err_min = KF_MB_INTRA_MIN * cpi->common.MBs;
  cpi->twopass.gf_intra_err_min = GF_MB_INTRA_MIN * cpi->common.MBs;

  // This variable monitors how far behind the second ref update is lagging
  cpi->twopass.sr_update_lag = 1;

  // Scan the first pass file and calculate an average Intra / Inter error score ratio for the sequence
  {
    double sum_iiratio = 0.0;
    double IIRatio;

    start_pos = cpi->twopass.stats_in;               // Note starting "file" position

    while (input_stats(cpi, &this_frame) != EOF) {
      IIRatio = this_frame.intra_error / DOUBLE_DIVIDE_CHECK(this_frame.coded_error);
      IIRatio = (IIRatio < 1.0) ? 1.0 : (IIRatio > 20.0) ? 20.0 : IIRatio;
      sum_iiratio += IIRatio;
    }

    cpi->twopass.avg_iiratio = sum_iiratio / DOUBLE_DIVIDE_CHECK((double)cpi->twopass.total_stats->count);

    // Reset file position
    reset_fpf_position(cpi, start_pos);
  }

  // Scan the first pass file and calculate a modified total error based upon the bias/power function
  // used to allocate bits
  {
    start_pos = cpi->twopass.stats_in;               // Note starting "file" position

    cpi->twopass.modified_error_total = 0.0;
    cpi->twopass.modified_error_used = 0.0;

    while (input_stats(cpi, &this_frame) != EOF) {
      cpi->twopass.modified_error_total += calculate_modified_err(cpi, &this_frame);
    }
    cpi->twopass.modified_error_left = cpi->twopass.modified_error_total;

    reset_fpf_position(cpi, start_pos);            // Reset file position

  }
}

void vp9_end_second_pass(VP9_COMP *cpi) {
}

// This function gives and estimate of how badly we believe
// the prediction quality is decaying from frame to frame.
static double get_prediction_decay_rate(VP9_COMP *cpi,
                                        FIRSTPASS_STATS *next_frame) {
  double prediction_decay_rate;
  double second_ref_decay;
  double mb_sr_err_diff;

  // Initial basis is the % mbs inter coded
  prediction_decay_rate = next_frame->pcnt_inter;

  // Look at the observed drop in prediction quality between the last frame
  // and the GF buffer (which contains an older frame).
  mb_sr_err_diff =
    (next_frame->sr_coded_error - next_frame->coded_error) /
    (cpi->common.MBs);
  second_ref_decay = 1.0 - (mb_sr_err_diff / 512.0);
  second_ref_decay = pow(second_ref_decay, 0.5);
  if (second_ref_decay < 0.85)
    second_ref_decay = 0.85;
  else if (second_ref_decay > 1.0)
    second_ref_decay = 1.0;

  if (second_ref_decay < prediction_decay_rate)
    prediction_decay_rate = second_ref_decay;

  return prediction_decay_rate;
}

// Function to test for a condition where a complex transition is followed
// by a static section. For example in slide shows where there is a fade
// between slides. This is to help with more optimal kf and gf positioning.
static int detect_transition_to_still(
  VP9_COMP *cpi,
  int frame_interval,
  int still_interval,
  double loop_decay_rate,
  double last_decay_rate) {
  int trans_to_still = FALSE;

  // Break clause to detect very still sections after motion
  // For example a static image after a fade or other transition
  // instead of a clean scene cut.
  if ((frame_interval > MIN_GF_INTERVAL) &&
      (loop_decay_rate >= 0.999) &&
      (last_decay_rate < 0.9)) {
    int j;
    FIRSTPASS_STATS *position = cpi->twopass.stats_in;
    FIRSTPASS_STATS tmp_next_frame;
    double zz_inter;

    // Look ahead a few frames to see if static condition
    // persists...
    for (j = 0; j < still_interval; j++) {
      if (EOF == input_stats(cpi, &tmp_next_frame))
        break;

      zz_inter =
        (tmp_next_frame.pcnt_inter - tmp_next_frame.pcnt_motion);
      if (zz_inter < 0.999)
        break;
    }
    // Reset file position
    reset_fpf_position(cpi, position);

    // Only if it does do we signal a transition to still
    if (j == still_interval)
      trans_to_still = TRUE;
  }

  return trans_to_still;
}

// This function detects a flash through the high relative pcnt_second_ref
// score in the frame following a flash frame. The offset passed in should
// reflect this
static int detect_flash(VP9_COMP *cpi, int offset) {
  FIRSTPASS_STATS next_frame;

  int flash_detected = FALSE;

  // Read the frame data.
  // The return is FALSE (no flash detected) if not a valid frame
  if (read_frame_stats(cpi, &next_frame, offset) != EOF) {
    // What we are looking for here is a situation where there is a
    // brief break in prediction (such as a flash) but subsequent frames
    // are reasonably well predicted by an earlier (pre flash) frame.
    // The recovery after a flash is indicated by a high pcnt_second_ref
    // comapred to pcnt_inter.
    if ((next_frame.pcnt_second_ref > next_frame.pcnt_inter) &&
        (next_frame.pcnt_second_ref >= 0.5)) {
      flash_detected = TRUE;
    }
  }

  return flash_detected;
}

// Update the motion related elements to the GF arf boost calculation
static void accumulate_frame_motion_stats(
  VP9_COMP *cpi,
  FIRSTPASS_STATS *this_frame,
  double *this_frame_mv_in_out,
  double *mv_in_out_accumulator,
  double *abs_mv_in_out_accumulator,
  double *mv_ratio_accumulator) {
  // double this_frame_mv_in_out;
  double this_frame_mvr_ratio;
  double this_frame_mvc_ratio;
  double motion_pct;

  // Accumulate motion stats.
  motion_pct = this_frame->pcnt_motion;

  // Accumulate Motion In/Out of frame stats
  *this_frame_mv_in_out = this_frame->mv_in_out_count * motion_pct;
  *mv_in_out_accumulator += this_frame->mv_in_out_count * motion_pct;
  *abs_mv_in_out_accumulator +=
    fabs(this_frame->mv_in_out_count * motion_pct);

  // Accumulate a measure of how uniform (or conversely how random)
  // the motion field is. (A ratio of absmv / mv)
  if (motion_pct > 0.05) {
    this_frame_mvr_ratio = fabs(this_frame->mvr_abs) /
                           DOUBLE_DIVIDE_CHECK(fabs(this_frame->MVr));

    this_frame_mvc_ratio = fabs(this_frame->mvc_abs) /
                           DOUBLE_DIVIDE_CHECK(fabs(this_frame->MVc));

    *mv_ratio_accumulator +=
      (this_frame_mvr_ratio < this_frame->mvr_abs)
      ? (this_frame_mvr_ratio * motion_pct)
      : this_frame->mvr_abs * motion_pct;

    *mv_ratio_accumulator +=
      (this_frame_mvc_ratio < this_frame->mvc_abs)
      ? (this_frame_mvc_ratio * motion_pct)
      : this_frame->mvc_abs * motion_pct;

  }
}

// Calculate a baseline boost number for the current frame.
static double calc_frame_boost(
  VP9_COMP *cpi,
  FIRSTPASS_STATS *this_frame,
  double this_frame_mv_in_out) {
  double frame_boost;

  // Underlying boost factor is based on inter intra error ratio
  if (this_frame->intra_error > cpi->twopass.gf_intra_err_min)
    frame_boost = (IIFACTOR * this_frame->intra_error /
                   DOUBLE_DIVIDE_CHECK(this_frame->coded_error));
  else
    frame_boost = (IIFACTOR * cpi->twopass.gf_intra_err_min /
                   DOUBLE_DIVIDE_CHECK(this_frame->coded_error));

  // Increase boost for frames where new data coming into frame
  // (eg zoom out). Slightly reduce boost if there is a net balance
  // of motion out of the frame (zoom in).
  // The range for this_frame_mv_in_out is -1.0 to +1.0
  if (this_frame_mv_in_out > 0.0)
    frame_boost += frame_boost * (this_frame_mv_in_out * 2.0);
  // In extreme case boost is halved
  else
    frame_boost += frame_boost * (this_frame_mv_in_out / 2.0);

  // Clip to maximum
  if (frame_boost > GF_RMAX)
    frame_boost = GF_RMAX;

  return frame_boost;
}

static int calc_arf_boost(
  VP9_COMP *cpi,
  int offset,
  int f_frames,
  int b_frames,
  int *f_boost,
  int *b_boost) {
  FIRSTPASS_STATS this_frame;

  int i;
  double boost_score = 0.0;
  double mv_ratio_accumulator = 0.0;
  double decay_accumulator = 1.0;
  double this_frame_mv_in_out = 0.0;
  double mv_in_out_accumulator = 0.0;
  double abs_mv_in_out_accumulator = 0.0;
  int arf_boost;
  int flash_detected = FALSE;

  // Search forward from the proposed arf/next gf position
  for (i = 0; i < f_frames; i++) {
    if (read_frame_stats(cpi, &this_frame, (i + offset)) == EOF)
      break;

    // Update the motion related elements to the boost calculation
    accumulate_frame_motion_stats(cpi, &this_frame,
                                  &this_frame_mv_in_out, &mv_in_out_accumulator,
                                  &abs_mv_in_out_accumulator, &mv_ratio_accumulator);

    // We want to discount the the flash frame itself and the recovery
    // frame that follows as both will have poor scores.
    flash_detected = detect_flash(cpi, (i + offset)) ||
                     detect_flash(cpi, (i + offset + 1));

    // Cumulative effect of prediction quality decay
    if (!flash_detected) {
      decay_accumulator =
        decay_accumulator * get_prediction_decay_rate(cpi, &this_frame);
      decay_accumulator = decay_accumulator < MIN_DECAY_FACTOR
                          ? MIN_DECAY_FACTOR : decay_accumulator;
    }

    boost_score += (decay_accumulator *
                    calc_frame_boost(cpi, &this_frame, this_frame_mv_in_out));
  }

  *f_boost = (int)boost_score;

  // Reset for backward looking loop
  boost_score = 0.0;
  mv_ratio_accumulator = 0.0;
  decay_accumulator = 1.0;
  this_frame_mv_in_out = 0.0;
  mv_in_out_accumulator = 0.0;
  abs_mv_in_out_accumulator = 0.0;

  // Search backward towards last gf position
  for (i = -1; i >= -b_frames; i--) {
    if (read_frame_stats(cpi, &this_frame, (i + offset)) == EOF)
      break;

    // Update the motion related elements to the boost calculation
    accumulate_frame_motion_stats(cpi, &this_frame,
                                  &this_frame_mv_in_out, &mv_in_out_accumulator,
                                  &abs_mv_in_out_accumulator, &mv_ratio_accumulator);

    // We want to discount the the flash frame itself and the recovery
    // frame that follows as both will have poor scores.
    flash_detected = detect_flash(cpi, (i + offset)) ||
                     detect_flash(cpi, (i + offset + 1));

    // Cumulative effect of prediction quality decay
    if (!flash_detected) {
      decay_accumulator =
        decay_accumulator * get_prediction_decay_rate(cpi, &this_frame);
      decay_accumulator = decay_accumulator < MIN_DECAY_FACTOR
                          ? MIN_DECAY_FACTOR : decay_accumulator;
    }

    boost_score += (decay_accumulator *
                    calc_frame_boost(cpi, &this_frame, this_frame_mv_in_out));

  }
  *b_boost = (int)boost_score;

  arf_boost = (*f_boost + *b_boost);
  if (arf_boost < ((b_frames + f_frames) * 20))
    arf_boost = ((b_frames + f_frames) * 20);

  return arf_boost;
}

static void configure_arnr_filter(VP9_COMP *cpi, FIRSTPASS_STATS *this_frame) {
  int half_gf_int;
  int frames_after_arf;
  int frames_bwd = cpi->oxcf.arnr_max_frames - 1;
  int frames_fwd = cpi->oxcf.arnr_max_frames - 1;

  // Define the arnr filter width for this group of frames:
  // We only filter frames that lie within a distance of half
  // the GF interval from the ARF frame. We also have to trap
  // cases where the filter extends beyond the end of clip.
  // Note: this_frame->frame has been updated in the loop
  // so it now points at the ARF frame.
  half_gf_int = cpi->baseline_gf_interval >> 1;
  frames_after_arf = (int)(cpi->twopass.total_stats->count -
                           this_frame->frame - 1);

  switch (cpi->oxcf.arnr_type) {
    case 1: // Backward filter
      frames_fwd = 0;
      if (frames_bwd > half_gf_int)
        frames_bwd = half_gf_int;
      break;

    case 2: // Forward filter
      if (frames_fwd > half_gf_int)
        frames_fwd = half_gf_int;
      if (frames_fwd > frames_after_arf)
        frames_fwd = frames_after_arf;
      frames_bwd = 0;
      break;

    case 3: // Centered filter
    default:
      frames_fwd >>= 1;
      if (frames_fwd > frames_after_arf)
        frames_fwd = frames_after_arf;
      if (frames_fwd > half_gf_int)
        frames_fwd = half_gf_int;

      frames_bwd = frames_fwd;

      // For even length filter there is one more frame backward
      // than forward: e.g. len=6 ==> bbbAff, len=7 ==> bbbAfff.
      if (frames_bwd < half_gf_int)
        frames_bwd += (cpi->oxcf.arnr_max_frames + 1) & 0x1;
      break;
  }

  cpi->active_arnr_frames = frames_bwd + 1 + frames_fwd;
}

// Analyse and define a gf/arf group .
static void define_gf_group(VP9_COMP *cpi, FIRSTPASS_STATS *this_frame) {
  FIRSTPASS_STATS next_frame;
  FIRSTPASS_STATS *start_pos;
  int i;
  double boost_score = 0.0;
  double old_boost_score = 0.0;
  double gf_group_err = 0.0;
  double gf_first_frame_err = 0.0;
  double mod_frame_err = 0.0;

  double mv_ratio_accumulator = 0.0;
  double decay_accumulator = 1.0;
  double zero_motion_accumulator = 1.0;

  double loop_decay_rate = 1.00;          // Starting decay rate
  double last_loop_decay_rate = 1.00;

  double this_frame_mv_in_out = 0.0;
  double mv_in_out_accumulator = 0.0;
  double abs_mv_in_out_accumulator = 0.0;

  int max_bits = frame_max_bits(cpi);     // Max for a single frame

  unsigned int allow_alt_ref =
    cpi->oxcf.play_alternate && cpi->oxcf.lag_in_frames;

  int f_boost = 0;
  int b_boost = 0;
  int flash_detected;

  cpi->twopass.gf_group_bits = 0;

  vp9_clear_system_state();  // __asm emms;

  start_pos = cpi->twopass.stats_in;

  vpx_memset(&next_frame, 0, sizeof(next_frame)); // assure clean

  // Load stats for the current frame.
  mod_frame_err = calculate_modified_err(cpi, this_frame);

  // Note the error of the frame at the start of the group (this will be
  // the GF frame error if we code a normal gf
  gf_first_frame_err = mod_frame_err;

  // Special treatment if the current frame is a key frame (which is also
  // a gf). If it is then its error score (and hence bit allocation) need
  // to be subtracted out from the calculation for the GF group
  if (cpi->common.frame_type == KEY_FRAME)
    gf_group_err -= gf_first_frame_err;

  // Scan forward to try and work out how many frames the next gf group
  // should contain and what level of boost is appropriate for the GF
  // or ARF that will be coded with the group
  i = 0;

  while (((i < cpi->twopass.static_scene_max_gf_interval) ||
          ((cpi->twopass.frames_to_key - i) < MIN_GF_INTERVAL)) &&
         (i < cpi->twopass.frames_to_key)) {
    i++;    // Increment the loop counter

    // Accumulate error score of frames in this gf group
    mod_frame_err = calculate_modified_err(cpi, this_frame);
    gf_group_err += mod_frame_err;

    if (EOF == input_stats(cpi, &next_frame))
      break;

    // Test for the case where there is a brief flash but the prediction
    // quality back to an earlier frame is then restored.
    flash_detected = detect_flash(cpi, 0);

    // Update the motion related elements to the boost calculation
    accumulate_frame_motion_stats(cpi, &next_frame,
                                  &this_frame_mv_in_out, &mv_in_out_accumulator,
                                  &abs_mv_in_out_accumulator, &mv_ratio_accumulator);

    // Cumulative effect of prediction quality decay
    if (!flash_detected) {
      last_loop_decay_rate = loop_decay_rate;
      loop_decay_rate = get_prediction_decay_rate(cpi, &next_frame);
      decay_accumulator = decay_accumulator * loop_decay_rate;

      // Monitor for static sections.
      if ((next_frame.pcnt_inter - next_frame.pcnt_motion) <
          zero_motion_accumulator) {
        zero_motion_accumulator =
          (next_frame.pcnt_inter - next_frame.pcnt_motion);
      }

      // Break clause to detect very still sections after motion
      // (for example a staic image after a fade or other transition).
      if (detect_transition_to_still(cpi, i, 5, loop_decay_rate,
                                     last_loop_decay_rate)) {
        allow_alt_ref = FALSE;
        break;
      }
    }

    // Calculate a boost number for this frame
    boost_score +=
      (decay_accumulator *
       calc_frame_boost(cpi, &next_frame, this_frame_mv_in_out));

    // Break out conditions.
    if (
      // Break at cpi->max_gf_interval unless almost totally static
      (i >= cpi->max_gf_interval && (zero_motion_accumulator < 0.995)) ||
      (
        // Dont break out with a very short interval
        (i > MIN_GF_INTERVAL) &&
        // Dont break out very close to a key frame
        ((cpi->twopass.frames_to_key - i) >= MIN_GF_INTERVAL) &&
        ((boost_score > 125.0) || (next_frame.pcnt_inter < 0.75)) &&
        (!flash_detected) &&
        ((mv_ratio_accumulator > 100.0) ||
         (abs_mv_in_out_accumulator > 3.0) ||
         (mv_in_out_accumulator < -2.0) ||
         ((boost_score - old_boost_score) < IIFACTOR))
      )) {
      boost_score = old_boost_score;
      break;
    }

    vpx_memcpy(this_frame, &next_frame, sizeof(*this_frame));

    old_boost_score = boost_score;
  }

  // Dont allow a gf too near the next kf
  if ((cpi->twopass.frames_to_key - i) < MIN_GF_INTERVAL) {
    while (i < cpi->twopass.frames_to_key) {
      i++;

      if (EOF == input_stats(cpi, this_frame))
        break;

      if (i < cpi->twopass.frames_to_key) {
        mod_frame_err = calculate_modified_err(cpi, this_frame);
        gf_group_err += mod_frame_err;
      }
    }
  }

  // Set the interval till the next gf or arf.
  cpi->baseline_gf_interval = i;

  // Should we use the alternate refernce frame
  if (allow_alt_ref &&
      (i < cpi->oxcf.lag_in_frames) &&
      (i >= MIN_GF_INTERVAL) &&
      // dont use ARF very near next kf
      (i <= (cpi->twopass.frames_to_key - MIN_GF_INTERVAL)) &&
      ((next_frame.pcnt_inter > 0.75) ||
       (next_frame.pcnt_second_ref > 0.5)) &&
      ((mv_in_out_accumulator / (double)i > -0.2) ||
       (mv_in_out_accumulator > -2.0)) &&
      (boost_score > 100)) {
    // Alterrnative boost calculation for alt ref
    cpi->gfu_boost = calc_arf_boost(cpi, 0, (i - 1), (i - 1), &f_boost, &b_boost);
    cpi->source_alt_ref_pending = TRUE;

    configure_arnr_filter(cpi, this_frame);
  } else {
    cpi->gfu_boost = (int)boost_score;
    cpi->source_alt_ref_pending = FALSE;
  }

  // Now decide how many bits should be allocated to the GF group as  a
  // proportion of those remaining in the kf group.
  // The final key frame group in the clip is treated as a special case
  // where cpi->twopass.kf_group_bits is tied to cpi->twopass.bits_left.
  // This is also important for short clips where there may only be one
  // key frame.
  if (cpi->twopass.frames_to_key >= (int)(cpi->twopass.total_stats->count -
                                          cpi->common.current_video_frame)) {
    cpi->twopass.kf_group_bits =
      (cpi->twopass.bits_left > 0) ? cpi->twopass.bits_left : 0;
  }

  // Calculate the bits to be allocated to the group as a whole
  if ((cpi->twopass.kf_group_bits > 0) &&
      (cpi->twopass.kf_group_error_left > 0)) {
    cpi->twopass.gf_group_bits =
      (int)((double)cpi->twopass.kf_group_bits *
            (gf_group_err / cpi->twopass.kf_group_error_left));
  } else
    cpi->twopass.gf_group_bits = 0;

  cpi->twopass.gf_group_bits =
    (cpi->twopass.gf_group_bits < 0)
    ? 0
    : (cpi->twopass.gf_group_bits > cpi->twopass.kf_group_bits)
    ? cpi->twopass.kf_group_bits : cpi->twopass.gf_group_bits;

  // Clip cpi->twopass.gf_group_bits based on user supplied data rate
  // variability limit (cpi->oxcf.two_pass_vbrmax_section)
  if (cpi->twopass.gf_group_bits > max_bits * cpi->baseline_gf_interval)
    cpi->twopass.gf_group_bits = max_bits * cpi->baseline_gf_interval;

  // Reset the file position
  reset_fpf_position(cpi, start_pos);

  // Update the record of error used so far (only done once per gf group)
  cpi->twopass.modified_error_used += gf_group_err;

  // Assign  bits to the arf or gf.
  for (i = 0; i <= (cpi->source_alt_ref_pending && cpi->common.frame_type != KEY_FRAME); i++) {
    int boost;
    int allocation_chunks;
    int Q = (cpi->oxcf.fixed_q < 0) ? cpi->last_q[INTER_FRAME] : cpi->oxcf.fixed_q;
    int gf_bits;

    boost = (cpi->gfu_boost * vp9_gfboost_qadjust(Q)) / 100;

    // Set max and minimum boost and hence minimum allocation
    if (boost > ((cpi->baseline_gf_interval + 1) * 200))
      boost = ((cpi->baseline_gf_interval + 1) * 200);
    else if (boost < 125)
      boost = 125;

    if (cpi->source_alt_ref_pending && i == 0)
      allocation_chunks =
        ((cpi->baseline_gf_interval + 1) * 100) + boost;
    else
      allocation_chunks =
        (cpi->baseline_gf_interval * 100) + (boost - 100);

    // Prevent overflow
    if (boost > 1028) {
      int divisor = boost >> 10;
      boost /= divisor;
      allocation_chunks /= divisor;
    }

    // Calculate the number of bits to be spent on the gf or arf based on
    // the boost number
    gf_bits = (int)((double)boost *
                    (cpi->twopass.gf_group_bits /
                     (double)allocation_chunks));

    // If the frame that is to be boosted is simpler than the average for
    // the gf/arf group then use an alternative calculation
    // based on the error score of the frame itself
    if (mod_frame_err < gf_group_err / (double)cpi->baseline_gf_interval) {
      double  alt_gf_grp_bits;
      int     alt_gf_bits;

      alt_gf_grp_bits =
        (double)cpi->twopass.kf_group_bits  *
        (mod_frame_err * (double)cpi->baseline_gf_interval) /
        DOUBLE_DIVIDE_CHECK(cpi->twopass.kf_group_error_left);

      alt_gf_bits = (int)((double)boost * (alt_gf_grp_bits /
                                           (double)allocation_chunks));

      if (gf_bits > alt_gf_bits) {
        gf_bits = alt_gf_bits;
      }
    }
    // Else if it is harder than other frames in the group make sure it at
    // least receives an allocation in keeping with its relative error
    // score, otherwise it may be worse off than an "un-boosted" frame
    else {
      int alt_gf_bits =
        (int)((double)cpi->twopass.kf_group_bits *
              mod_frame_err /
              DOUBLE_DIVIDE_CHECK(cpi->twopass.kf_group_error_left));

      if (alt_gf_bits > gf_bits) {
        gf_bits = alt_gf_bits;
      }
    }

    // Dont allow a negative value for gf_bits
    if (gf_bits < 0)
      gf_bits = 0;

    gf_bits += cpi->min_frame_bandwidth;                     // Add in minimum for a frame

    if (i == 0) {
      cpi->twopass.gf_bits = gf_bits;
    }
    if (i == 1 || (!cpi->source_alt_ref_pending && (cpi->common.frame_type != KEY_FRAME))) {
      cpi->per_frame_bandwidth = gf_bits;                 // Per frame bit target for this frame
    }
  }

  {
    // Adjust KF group bits and error remainin
    cpi->twopass.kf_group_error_left -= (int64_t)gf_group_err;
    cpi->twopass.kf_group_bits -= cpi->twopass.gf_group_bits;

    if (cpi->twopass.kf_group_bits < 0)
      cpi->twopass.kf_group_bits = 0;

    // Note the error score left in the remaining frames of the group.
    // For normal GFs we want to remove the error score for the first frame
    // of the group (except in Key frame case where this has already
    // happened)
    if (!cpi->source_alt_ref_pending && cpi->common.frame_type != KEY_FRAME)
      cpi->twopass.gf_group_error_left = (int64_t)(gf_group_err
                                                   - gf_first_frame_err);
    else
      cpi->twopass.gf_group_error_left = (int64_t)gf_group_err;

    cpi->twopass.gf_group_bits -= cpi->twopass.gf_bits - cpi->min_frame_bandwidth;

    if (cpi->twopass.gf_group_bits < 0)
      cpi->twopass.gf_group_bits = 0;

    // This condition could fail if there are two kfs very close together
    // despite (MIN_GF_INTERVAL) and would cause a devide by 0 in the
    // calculation of cpi->twopass.alt_extra_bits.
    if (cpi->baseline_gf_interval >= 3) {
      int boost = (cpi->source_alt_ref_pending)
                  ? b_boost : cpi->gfu_boost;

      if (boost >= 150) {
        int pct_extra;

        pct_extra = (boost - 100) / 50;
        pct_extra = (pct_extra > 20) ? 20 : pct_extra;

        cpi->twopass.alt_extra_bits = (int)
          ((cpi->twopass.gf_group_bits * pct_extra) / 100);
        cpi->twopass.gf_group_bits -= cpi->twopass.alt_extra_bits;
        cpi->twopass.alt_extra_bits /=
          ((cpi->baseline_gf_interval - 1) >> 1);
      } else
        cpi->twopass.alt_extra_bits = 0;
    } else
      cpi->twopass.alt_extra_bits = 0;
  }

  if (cpi->common.frame_type != KEY_FRAME) {
    FIRSTPASS_STATS sectionstats;

    zero_stats(&sectionstats);
    reset_fpf_position(cpi, start_pos);

    for (i = 0; i < cpi->baseline_gf_interval; i++) {
      input_stats(cpi, &next_frame);
      accumulate_stats(&sectionstats, &next_frame);
    }

    avg_stats(&sectionstats);

    cpi->twopass.section_intra_rating = (int)
      (sectionstats.intra_error /
      DOUBLE_DIVIDE_CHECK(sectionstats.coded_error));

    reset_fpf_position(cpi, start_pos);
  }
}

// Allocate bits to a normal frame that is neither a gf an arf or a key frame.
static void assign_std_frame_bits(VP9_COMP *cpi, FIRSTPASS_STATS *this_frame) {
  int    target_frame_size;                                                             // gf_group_error_left

  double modified_err;
  double err_fraction;                                                                 // What portion of the remaining GF group error is used by this frame

  int max_bits = frame_max_bits(cpi);    // Max for a single frame

  // Calculate modified prediction error used in bit allocation
  modified_err = calculate_modified_err(cpi, this_frame);

  if (cpi->twopass.gf_group_error_left > 0)
    err_fraction = modified_err / cpi->twopass.gf_group_error_left;                              // What portion of the remaining GF group error is used by this frame
  else
    err_fraction = 0.0;

  target_frame_size = (int)((double)cpi->twopass.gf_group_bits * err_fraction);                    // How many of those bits available for allocation should we give it?

  // Clip to target size to 0 - max_bits (or cpi->twopass.gf_group_bits) at the top end.
  if (target_frame_size < 0)
    target_frame_size = 0;
  else {
    if (target_frame_size > max_bits)
      target_frame_size = max_bits;

    if (target_frame_size > cpi->twopass.gf_group_bits)
      target_frame_size = (int)cpi->twopass.gf_group_bits;
  }

  // Adjust error remaining
  cpi->twopass.gf_group_error_left -= (int64_t)modified_err;
  cpi->twopass.gf_group_bits -= target_frame_size;                                                // Adjust bits remaining

  if (cpi->twopass.gf_group_bits < 0)
    cpi->twopass.gf_group_bits = 0;

  target_frame_size += cpi->min_frame_bandwidth;                                          // Add in the minimum number of bits that is set aside for every frame.


  cpi->per_frame_bandwidth = target_frame_size;                                           // Per frame bit target for this frame
}

// Make a damped adjustment to the active max q.
static int adjust_active_maxq(int old_maxqi, int new_maxqi) {
  int i;
  int ret_val = new_maxqi;
  double old_q;
  double new_q;
  double target_q;

  old_q = vp9_convert_qindex_to_q(old_maxqi);
  new_q = vp9_convert_qindex_to_q(new_maxqi);

  target_q = ((old_q * 7.0) + new_q) / 8.0;

  if (target_q > old_q) {
    for (i = old_maxqi; i <= new_maxqi; i++) {
      if (vp9_convert_qindex_to_q(i) >= target_q) {
        ret_val = i;
        break;
      }
    }
  } else {
    for (i = old_maxqi; i >= new_maxqi; i--) {
      if (vp9_convert_qindex_to_q(i) <= target_q) {
        ret_val = i;
        break;
      }
    }
  }

  return ret_val;
}

void vp9_second_pass(VP9_COMP *cpi) {
  int tmp_q;
  int frames_left = (int)(cpi->twopass.total_stats->count - cpi->common.current_video_frame);

  FIRSTPASS_STATS this_frame;
  FIRSTPASS_STATS this_frame_copy;

  double this_frame_intra_error;
  double this_frame_coded_error;

  int overhead_bits;

  if (!cpi->twopass.stats_in) {
    return;
  }

  vp9_clear_system_state();

  vpx_memset(&this_frame, 0, sizeof(FIRSTPASS_STATS));

  if (EOF == input_stats(cpi, &this_frame))
    return;

  this_frame_intra_error = this_frame.intra_error;
  this_frame_coded_error = this_frame.coded_error;

  // keyframe and section processing !
  if (cpi->twopass.frames_to_key == 0) {
    // Define next KF group and assign bits to it
    vpx_memcpy(&this_frame_copy, &this_frame, sizeof(this_frame));
    find_next_key_frame(cpi, &this_frame_copy);
  }

  // Is this a GF / ARF (Note that a KF is always also a GF)
  if (cpi->frames_till_gf_update_due == 0) {
    // Define next gf group and assign bits to it
    vpx_memcpy(&this_frame_copy, &this_frame, sizeof(this_frame));
    define_gf_group(cpi, &this_frame_copy);

    // If we are going to code an altref frame at the end of the group and the current frame is not a key frame....
    // If the previous group used an arf this frame has already benefited from that arf boost and it should not be given extra bits
    // If the previous group was NOT coded using arf we may want to apply some boost to this GF as well
    if (cpi->source_alt_ref_pending && (cpi->common.frame_type != KEY_FRAME)) {
      // Assign a standard frames worth of bits from those allocated to the GF group
      int bak = cpi->per_frame_bandwidth;
      vpx_memcpy(&this_frame_copy, &this_frame, sizeof(this_frame));
      assign_std_frame_bits(cpi, &this_frame_copy);
      cpi->per_frame_bandwidth = bak;
    }
  }

  // Otherwise this is an ordinary frame
  else {
    // Assign bits from those allocated to the GF group
    vpx_memcpy(&this_frame_copy, &this_frame, sizeof(this_frame));
    assign_std_frame_bits(cpi, &this_frame_copy);
  }

  // Keep a globally available copy of this and the next frame's iiratio.
  cpi->twopass.this_iiratio = (int)(this_frame_intra_error /
                              DOUBLE_DIVIDE_CHECK(this_frame_coded_error));
  {
    FIRSTPASS_STATS next_frame;
    if (lookup_next_frame_stats(cpi, &next_frame) != EOF) {
      cpi->twopass.next_iiratio = (int)(next_frame.intra_error /
                                  DOUBLE_DIVIDE_CHECK(next_frame.coded_error));
    }
  }

  // Set nominal per second bandwidth for this frame
  cpi->target_bandwidth = (int)(cpi->per_frame_bandwidth
                                * cpi->output_frame_rate);
  if (cpi->target_bandwidth < 0)
    cpi->target_bandwidth = 0;


  // Account for mv, mode and other overheads.
  overhead_bits = (int)estimate_modemvcost(
                        cpi, cpi->twopass.total_left_stats);

  // Special case code for first frame.
  if (cpi->common.current_video_frame == 0) {
    cpi->twopass.est_max_qcorrection_factor = 1.0;

    // Set a cq_level in constrained quality mode.
    if (cpi->oxcf.end_usage == USAGE_CONSTRAINED_QUALITY) {
      int est_cq;

      est_cq =
        estimate_cq(cpi,
                    cpi->twopass.total_left_stats,
                    (int)(cpi->twopass.bits_left / frames_left),
                    overhead_bits);

      cpi->cq_target_quality = cpi->oxcf.cq_level;
      if (est_cq > cpi->cq_target_quality)
        cpi->cq_target_quality = est_cq;
    }

    // guess at maxq needed in 2nd pass
    cpi->twopass.maxq_max_limit = cpi->worst_quality;
    cpi->twopass.maxq_min_limit = cpi->best_quality;

    tmp_q = estimate_max_q(
              cpi,
              cpi->twopass.total_left_stats,
              (int)(cpi->twopass.bits_left / frames_left),
              overhead_bits);

    cpi->active_worst_quality         = tmp_q;
    cpi->ni_av_qi                     = tmp_q;
    cpi->avg_q                        = vp9_convert_qindex_to_q(tmp_q);

    // Limit the maxq value returned subsequently.
    // This increases the risk of overspend or underspend if the initial
    // estimate for the clip is bad, but helps prevent excessive
    // variation in Q, especially near the end of a clip
    // where for example a small overspend may cause Q to crash
    adjust_maxq_qrange(cpi);
  }

  // The last few frames of a clip almost always have to few or too many
  // bits and for the sake of over exact rate control we dont want to make
  // radical adjustments to the allowed quantizer range just to use up a
  // few surplus bits or get beneath the target rate.
  else if ((cpi->common.current_video_frame <
            (((unsigned int)cpi->twopass.total_stats->count * 255) >> 8)) &&
           ((cpi->common.current_video_frame + cpi->baseline_gf_interval) <
            (unsigned int)cpi->twopass.total_stats->count)) {
    if (frames_left < 1)
      frames_left = 1;

    tmp_q = estimate_max_q(
              cpi,
              cpi->twopass.total_left_stats,
              (int)(cpi->twopass.bits_left / frames_left),
              overhead_bits);

    // Make a damped adjustment to active max Q
    cpi->active_worst_quality =
      adjust_active_maxq(cpi->active_worst_quality, tmp_q);
  }

  cpi->twopass.frames_to_key--;

  // Update the total stats remaining sturcture
  subtract_stats(cpi->twopass.total_left_stats, &this_frame);
}


static int test_candidate_kf(VP9_COMP *cpi,
                             FIRSTPASS_STATS *last_frame,
                             FIRSTPASS_STATS *this_frame,
                             FIRSTPASS_STATS *next_frame) {
  int is_viable_kf = FALSE;

  // Does the frame satisfy the primary criteria of a key frame
  //      If so, then examine how well it predicts subsequent frames
  if ((this_frame->pcnt_second_ref < 0.10) &&
      (next_frame->pcnt_second_ref < 0.10) &&
      ((this_frame->pcnt_inter < 0.05) ||
       (
         ((this_frame->pcnt_inter - this_frame->pcnt_neutral) < .35) &&
         ((this_frame->intra_error / DOUBLE_DIVIDE_CHECK(this_frame->coded_error)) < 2.5) &&
         ((fabs(last_frame->coded_error - this_frame->coded_error) / DOUBLE_DIVIDE_CHECK(this_frame->coded_error) > .40) ||
          (fabs(last_frame->intra_error - this_frame->intra_error) / DOUBLE_DIVIDE_CHECK(this_frame->intra_error) > .40) ||
          ((next_frame->intra_error / DOUBLE_DIVIDE_CHECK(next_frame->coded_error)) > 3.5)
         )
       )
      )
     ) {
    int i;
    FIRSTPASS_STATS *start_pos;

    FIRSTPASS_STATS local_next_frame;

    double boost_score = 0.0;
    double old_boost_score = 0.0;
    double decay_accumulator = 1.0;
    double next_iiratio;

    vpx_memcpy(&local_next_frame, next_frame, sizeof(*next_frame));

    // Note the starting file position so we can reset to it
    start_pos = cpi->twopass.stats_in;

    // Examine how well the key frame predicts subsequent frames
    for (i = 0; i < 16; i++) {
      next_iiratio = (IIKFACTOR1 * local_next_frame.intra_error / DOUBLE_DIVIDE_CHECK(local_next_frame.coded_error));

      if (next_iiratio > RMAX)
        next_iiratio = RMAX;

      // Cumulative effect of decay in prediction quality
      if (local_next_frame.pcnt_inter > 0.85)
        decay_accumulator = decay_accumulator * local_next_frame.pcnt_inter;
      else
        decay_accumulator = decay_accumulator * ((0.85 + local_next_frame.pcnt_inter) / 2.0);

      // decay_accumulator = decay_accumulator * local_next_frame.pcnt_inter;

      // Keep a running total
      boost_score += (decay_accumulator * next_iiratio);

      // Test various breakout clauses
      if ((local_next_frame.pcnt_inter < 0.05) ||
          (next_iiratio < 1.5) ||
          (((local_next_frame.pcnt_inter -
             local_next_frame.pcnt_neutral) < 0.20) &&
           (next_iiratio < 3.0)) ||
          ((boost_score - old_boost_score) < 3.0) ||
          (local_next_frame.intra_error < 200)
         ) {
        break;
      }

      old_boost_score = boost_score;

      // Get the next frame details
      if (EOF == input_stats(cpi, &local_next_frame))
        break;
    }

    // If there is tolerable prediction for at least the next 3 frames then break out else discard this pottential key frame and move on
    if (boost_score > 30.0 && (i > 3))
      is_viable_kf = TRUE;
    else {
      // Reset the file position
      reset_fpf_position(cpi, start_pos);

      is_viable_kf = FALSE;
    }
  }

  return is_viable_kf;
}
static void find_next_key_frame(VP9_COMP *cpi, FIRSTPASS_STATS *this_frame) {
  int i, j;
  FIRSTPASS_STATS last_frame;
  FIRSTPASS_STATS first_frame;
  FIRSTPASS_STATS next_frame;
  FIRSTPASS_STATS *start_position;

  double decay_accumulator = 1.0;
  double zero_motion_accumulator = 1.0;
  double boost_score = 0;
  double old_boost_score = 0.0;
  double loop_decay_rate;

  double kf_mod_err = 0.0;
  double kf_group_err = 0.0;
  double kf_group_intra_err = 0.0;
  double kf_group_coded_err = 0.0;
  double recent_loop_decay[8] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

  vpx_memset(&next_frame, 0, sizeof(next_frame)); // assure clean

  vp9_clear_system_state();  // __asm emms;
  start_position = cpi->twopass.stats_in;

  cpi->common.frame_type = KEY_FRAME;

  // is this a forced key frame by interval
  cpi->this_key_frame_forced = cpi->next_key_frame_forced;

  // Clear the alt ref active flag as this can never be active on a key frame
  cpi->source_alt_ref_active = FALSE;

  // Kf is always a gf so clear frames till next gf counter
  cpi->frames_till_gf_update_due = 0;

  cpi->twopass.frames_to_key = 1;

  // Take a copy of the initial frame details
  vpx_memcpy(&first_frame, this_frame, sizeof(*this_frame));

  cpi->twopass.kf_group_bits = 0;        // Total bits avaialable to kf group
  cpi->twopass.kf_group_error_left = 0;  // Group modified error score.

  kf_mod_err = calculate_modified_err(cpi, this_frame);

  // find the next keyframe
  i = 0;
  while (cpi->twopass.stats_in < cpi->twopass.stats_in_end) {
    // Accumulate kf group error
    kf_group_err += calculate_modified_err(cpi, this_frame);

    // These figures keep intra and coded error counts for all frames including key frames in the group.
    // The effect of the key frame itself can be subtracted out using the first_frame data collected above
    kf_group_intra_err += this_frame->intra_error;
    kf_group_coded_err += this_frame->coded_error;

    // load a the next frame's stats
    vpx_memcpy(&last_frame, this_frame, sizeof(*this_frame));
    input_stats(cpi, this_frame);

    // Provided that we are not at the end of the file...
    if (cpi->oxcf.auto_key
        && lookup_next_frame_stats(cpi, &next_frame) != EOF) {
      // Normal scene cut check
      if (test_candidate_kf(cpi, &last_frame, this_frame, &next_frame)) {
        break;
      }

      // How fast is prediction quality decaying
      loop_decay_rate = get_prediction_decay_rate(cpi, &next_frame);

      // We want to know something about the recent past... rather than
      // as used elsewhere where we are concened with decay in prediction
      // quality since the last GF or KF.
      recent_loop_decay[i % 8] = loop_decay_rate;
      decay_accumulator = 1.0;
      for (j = 0; j < 8; j++) {
        decay_accumulator = decay_accumulator * recent_loop_decay[j];
      }

      // Special check for transition or high motion followed by a
      // to a static scene.
      if (detect_transition_to_still(cpi, i,
                                     (cpi->key_frame_frequency - i),
                                     loop_decay_rate,
                                     decay_accumulator)) {
        break;
      }


      // Step on to the next frame
      cpi->twopass.frames_to_key++;

      // If we don't have a real key frame within the next two
      // forcekeyframeevery intervals then break out of the loop.
      if (cpi->twopass.frames_to_key >= 2 * (int)cpi->key_frame_frequency)
        break;
    } else
      cpi->twopass.frames_to_key++;

    i++;
  }

  // If there is a max kf interval set by the user we must obey it.
  // We already breakout of the loop above at 2x max.
  // This code centers the extra kf if the actual natural
  // interval is between 1x and 2x
  if (cpi->oxcf.auto_key
      && cpi->twopass.frames_to_key > (int)cpi->key_frame_frequency) {
    FIRSTPASS_STATS *current_pos = cpi->twopass.stats_in;
    FIRSTPASS_STATS tmp_frame;

    cpi->twopass.frames_to_key /= 2;

    // Copy first frame details
    vpx_memcpy(&tmp_frame, &first_frame, sizeof(first_frame));

    // Reset to the start of the group
    reset_fpf_position(cpi, start_position);

    kf_group_err = 0;
    kf_group_intra_err = 0;
    kf_group_coded_err = 0;

    // Rescan to get the correct error data for the forced kf group
    for (i = 0; i < cpi->twopass.frames_to_key; i++) {
      // Accumulate kf group errors
      kf_group_err += calculate_modified_err(cpi, &tmp_frame);
      kf_group_intra_err += tmp_frame.intra_error;
      kf_group_coded_err += tmp_frame.coded_error;

      // Load a the next frame's stats
      input_stats(cpi, &tmp_frame);
    }

    // Reset to the start of the group
    reset_fpf_position(cpi, current_pos);

    cpi->next_key_frame_forced = TRUE;
  } else
    cpi->next_key_frame_forced = FALSE;

  // Special case for the last frame of the file
  if (cpi->twopass.stats_in >= cpi->twopass.stats_in_end) {
    // Accumulate kf group error
    kf_group_err += calculate_modified_err(cpi, this_frame);

    // These figures keep intra and coded error counts for all frames including key frames in the group.
    // The effect of the key frame itself can be subtracted out using the first_frame data collected above
    kf_group_intra_err += this_frame->intra_error;
    kf_group_coded_err += this_frame->coded_error;
  }

  // Calculate the number of bits that should be assigned to the kf group.
  if ((cpi->twopass.bits_left > 0) && (cpi->twopass.modified_error_left > 0.0)) {
    // Max for a single normal frame (not key frame)
    int max_bits = frame_max_bits(cpi);

    // Maximum bits for the kf group
    int64_t max_grp_bits;

    // Default allocation based on bits left and relative
    // complexity of the section
    cpi->twopass.kf_group_bits = (int64_t)(cpi->twopass.bits_left *
                                           (kf_group_err /
                                            cpi->twopass.modified_error_left));

    // Clip based on maximum per frame rate defined by the user.
    max_grp_bits = (int64_t)max_bits * (int64_t)cpi->twopass.frames_to_key;
    if (cpi->twopass.kf_group_bits > max_grp_bits)
      cpi->twopass.kf_group_bits = max_grp_bits;
  } else
    cpi->twopass.kf_group_bits = 0;

  // Reset the first pass file position
  reset_fpf_position(cpi, start_position);

  // determine how big to make this keyframe based on how well the subsequent frames use inter blocks
  decay_accumulator = 1.0;
  boost_score = 0.0;
  loop_decay_rate = 1.00;       // Starting decay rate

  for (i = 0; i < cpi->twopass.frames_to_key; i++) {
    double r;

    if (EOF == input_stats(cpi, &next_frame))
      break;

    if (next_frame.intra_error > cpi->twopass.kf_intra_err_min)
      r = (IIKFACTOR2 * next_frame.intra_error /
           DOUBLE_DIVIDE_CHECK(next_frame.coded_error));
    else
      r = (IIKFACTOR2 * cpi->twopass.kf_intra_err_min /
           DOUBLE_DIVIDE_CHECK(next_frame.coded_error));

    if (r > RMAX)
      r = RMAX;

    // Monitor for static sections.
    if ((next_frame.pcnt_inter - next_frame.pcnt_motion) <
        zero_motion_accumulator) {
      zero_motion_accumulator =
        (next_frame.pcnt_inter - next_frame.pcnt_motion);
    }

    // How fast is prediction quality decaying
    if (!detect_flash(cpi, 0)) {
      loop_decay_rate = get_prediction_decay_rate(cpi, &next_frame);
      decay_accumulator = decay_accumulator * loop_decay_rate;
      decay_accumulator = decay_accumulator < MIN_DECAY_FACTOR
                            ? MIN_DECAY_FACTOR : decay_accumulator;
    }

    boost_score += (decay_accumulator * r);

    if ((i > MIN_GF_INTERVAL) &&
        ((boost_score - old_boost_score) < 6.25)) {
      break;
    }

    old_boost_score = boost_score;
  }

  {
    FIRSTPASS_STATS sectionstats;

    zero_stats(&sectionstats);
    reset_fpf_position(cpi, start_position);

    for (i = 0; i < cpi->twopass.frames_to_key; i++) {
      input_stats(cpi, &next_frame);
      accumulate_stats(&sectionstats, &next_frame);
    }

    avg_stats(&sectionstats);

    cpi->twopass.section_intra_rating = (int)
      (sectionstats.intra_error
      / DOUBLE_DIVIDE_CHECK(sectionstats.coded_error));
  }

  // Reset the first pass file position
  reset_fpf_position(cpi, start_position);

  // Work out how many bits to allocate for the key frame itself
  if (1) {
    int kf_boost = (int)boost_score;
    int allocation_chunks;
    int alt_kf_bits;

    if (kf_boost < (cpi->twopass.frames_to_key * 5))
      kf_boost = (cpi->twopass.frames_to_key * 5);

    if (kf_boost < 300) // Min KF boost
      kf_boost = 300;

    // Make a note of baseline boost and the zero motion
    // accumulator value for use elsewhere.
    cpi->kf_boost = kf_boost;
    cpi->kf_zeromotion_pct = (int)(zero_motion_accumulator * 100.0);

    // We do three calculations for kf size.
    // The first is based on the error score for the whole kf group.
    // The second (optionaly) on the key frames own error if this is
    // smaller than the average for the group.
    // The final one insures that the frame receives at least the
    // allocation it would have received based on its own error score vs
    // the error score remaining
    // Special case if the sequence appears almost totaly static
    // In this case we want to spend almost all of the bits on the
    // key frame.
    // cpi->twopass.frames_to_key-1 because key frame itself is taken
    // care of by kf_boost.
    if (zero_motion_accumulator >= 0.99) {
      allocation_chunks =
        ((cpi->twopass.frames_to_key - 1) * 10) + kf_boost;
    } else {
      allocation_chunks =
        ((cpi->twopass.frames_to_key - 1) * 100) + kf_boost;
    }

    // Prevent overflow
    if (kf_boost > 1028) {
      int divisor = kf_boost >> 10;
      kf_boost /= divisor;
      allocation_chunks /= divisor;
    }

    cpi->twopass.kf_group_bits = (cpi->twopass.kf_group_bits < 0) ? 0 : cpi->twopass.kf_group_bits;

    // Calculate the number of bits to be spent on the key frame
    cpi->twopass.kf_bits  = (int)((double)kf_boost * ((double)cpi->twopass.kf_group_bits / (double)allocation_chunks));

    // If the key frame is actually easier than the average for the
    // kf group (which does sometimes happen... eg a blank intro frame)
    // Then use an alternate calculation based on the kf error score
    // which should give a smaller key frame.
    if (kf_mod_err < kf_group_err / cpi->twopass.frames_to_key) {
      double  alt_kf_grp_bits =
        ((double)cpi->twopass.bits_left *
         (kf_mod_err * (double)cpi->twopass.frames_to_key) /
         DOUBLE_DIVIDE_CHECK(cpi->twopass.modified_error_left));

      alt_kf_bits = (int)((double)kf_boost *
                          (alt_kf_grp_bits / (double)allocation_chunks));

      if (cpi->twopass.kf_bits > alt_kf_bits) {
        cpi->twopass.kf_bits = alt_kf_bits;
      }
    }
    // Else if it is much harder than other frames in the group make sure
    // it at least receives an allocation in keeping with its relative
    // error score
    else {
      alt_kf_bits =
        (int)((double)cpi->twopass.bits_left *
              (kf_mod_err /
               DOUBLE_DIVIDE_CHECK(cpi->twopass.modified_error_left)));

      if (alt_kf_bits > cpi->twopass.kf_bits) {
        cpi->twopass.kf_bits = alt_kf_bits;
      }
    }

    cpi->twopass.kf_group_bits -= cpi->twopass.kf_bits;
    // Add in the minimum frame allowance
    cpi->twopass.kf_bits += cpi->min_frame_bandwidth;

    // Peer frame bit target for this frame
    cpi->per_frame_bandwidth = cpi->twopass.kf_bits;
    // Convert to a per second bitrate
    cpi->target_bandwidth = (int)(cpi->twopass.kf_bits *
                                  cpi->output_frame_rate);
  }

  // Note the total error score of the kf group minus the key frame itself
  cpi->twopass.kf_group_error_left = (int)(kf_group_err - kf_mod_err);

  // Adjust the count of total modified error left.
  // The count of bits left is adjusted elsewhere based on real coded frame sizes
  cpi->twopass.modified_error_left -= kf_group_err;
}
