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
#include <vp9/encoder/vp9_encodeintra.h>
#include <vp9/encoder/vp9_rdopt.h>
#include <vp9/common/vp9_setupintrarecon.h>
#include <vp9/common/vp9_blockd.h>
#include <vp9/common/vp9_reconinter.h>
#include <vp9/common/vp9_systemdependent.h>
#include <vpx_mem/vpx_mem.h>
#include <vp9/encoder/vp9_segmentation.h>

static unsigned int do_16x16_motion_iteration(VP9_COMP *cpi,
                                              int_mv *ref_mv,
                                              int_mv *dst_mv) {
  MACROBLOCK   *const x  = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  BLOCK *b  = &x->block[0];
  BLOCKD *d = &xd->block[0];
  vp9_variance_fn_ptr_t v_fn_ptr = cpi->fn_ptr[BLOCK_16X16];
  unsigned int best_err;
  int step_param;

  int tmp_col_min = x->mv_col_min;
  int tmp_col_max = x->mv_col_max;
  int tmp_row_min = x->mv_row_min;
  int tmp_row_max = x->mv_row_max;
  int_mv ref_full;

  // Further step/diamond searches as necessary
  if (cpi->Speed < 8) {
    step_param = cpi->sf.first_step + ((cpi->Speed > 5) ? 1 : 0);
  } else {
    step_param = cpi->sf.first_step + 2;
  }

  vp9_clamp_mv_min_max(x, ref_mv);

  ref_full.as_mv.col = ref_mv->as_mv.col >> 3;
  ref_full.as_mv.row = ref_mv->as_mv.row >> 3;

  /*cpi->sf.search_method == HEX*/
  best_err = vp9_hex_search(
      x, b, d,
      &ref_full, dst_mv,
      step_param,
      x->errorperbit,
      &v_fn_ptr,
      NULL, NULL,
      NULL, NULL,
      ref_mv);

  // Try sub-pixel MC
  // if (bestsme > error_thresh && bestsme < INT_MAX)
  {
    int distortion;
    unsigned int sse;
    best_err = cpi->find_fractional_mv_step(
        x, b, d,
        dst_mv, ref_mv,
        x->errorperbit, &v_fn_ptr,
        NULL, NULL,
        & distortion, &sse);
  }

  vp9_set_mbmode_and_mvs(x, NEWMV, dst_mv);
  vp9_build_1st_inter16x16_predictors_mby(xd, xd->predictor, 16, 0);
  best_err = vp9_sad16x16(xd->dst.y_buffer, xd->dst.y_stride,
                          xd->predictor, 16, INT_MAX);

  /* restore UMV window */
  x->mv_col_min = tmp_col_min;
  x->mv_col_max = tmp_col_max;
  x->mv_row_min = tmp_row_min;
  x->mv_row_max = tmp_row_max;

  return best_err;
}

static int do_16x16_motion_search
(
  VP9_COMP *cpi,
  int_mv *ref_mv,
  int_mv *dst_mv,
  YV12_BUFFER_CONFIG *buf,
  int buf_mb_y_offset,
  YV12_BUFFER_CONFIG *ref,
  int mb_y_offset
) {
  MACROBLOCK   *const x  = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  unsigned int err, tmp_err;
  int_mv tmp_mv;
  int n;

  for (n = 0; n < 16; n++) {
    BLOCKD *d = &xd->block[n];
    BLOCK *b  = &x->block[n];

    b->base_src   = &buf->y_buffer;
    b->src_stride = buf->y_stride;
    b->src        = buf->y_stride * (n & 12) + (n & 3) * 4 + buf_mb_y_offset;

    d->base_pre   = &ref->y_buffer;
    d->pre_stride = ref->y_stride;
    d->pre        = ref->y_stride * (n & 12) + (n & 3) * 4 + mb_y_offset;
  }

  // Try zero MV first
  // FIXME should really use something like near/nearest MV and/or MV prediction
  xd->pre.y_buffer = ref->y_buffer + mb_y_offset;
  xd->pre.y_stride = ref->y_stride;
  err = vp9_sad16x16(ref->y_buffer + mb_y_offset, ref->y_stride,
                     xd->dst.y_buffer, xd->dst.y_stride, INT_MAX);
  dst_mv->as_int = 0;

  // Test last reference frame using the previous best mv as the
  // starting point (best reference) for the search
  tmp_err = do_16x16_motion_iteration(cpi, ref_mv, &tmp_mv);
  if (tmp_err < err) {
    err            = tmp_err;
    dst_mv->as_int = tmp_mv.as_int;
  }

  // If the current best reference mv is not centred on 0,0 then do a 0,0 based search as well
  if (ref_mv->as_int) {
    unsigned int tmp_err;
    int_mv zero_ref_mv, tmp_mv;

    zero_ref_mv.as_int = 0;
    tmp_err = do_16x16_motion_iteration(cpi, &zero_ref_mv, &tmp_mv);
    if (tmp_err < err) {
      dst_mv->as_int = tmp_mv.as_int;
      err = tmp_err;
    }
  }

  return err;
}

static int do_16x16_zerozero_search
(
  VP9_COMP *cpi,
  int_mv *dst_mv,
  YV12_BUFFER_CONFIG *buf,
  int buf_mb_y_offset,
  YV12_BUFFER_CONFIG *ref,
  int mb_y_offset
) {
  MACROBLOCK   *const x  = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  unsigned int err;
  int n;

  for (n = 0; n < 16; n++) {
    BLOCKD *d = &xd->block[n];
    BLOCK *b  = &x->block[n];

    b->base_src   = &buf->y_buffer;
    b->src_stride = buf->y_stride;
    b->src        = buf->y_stride * (n & 12) + (n & 3) * 4 + buf_mb_y_offset;

    d->base_pre   = &ref->y_buffer;
    d->pre_stride = ref->y_stride;
    d->pre        = ref->y_stride * (n & 12) + (n & 3) * 4 + mb_y_offset;
  }

  // Try zero MV first
  // FIXME should really use something like near/nearest MV and/or MV prediction
  xd->pre.y_buffer = ref->y_buffer + mb_y_offset;
  xd->pre.y_stride = ref->y_stride;
  err = vp9_sad16x16(ref->y_buffer + mb_y_offset, ref->y_stride,
                     xd->dst.y_buffer, xd->dst.y_stride, INT_MAX);

  dst_mv->as_int = 0;

  return err;
}
static int find_best_16x16_intra
(
  VP9_COMP *cpi,
  YV12_BUFFER_CONFIG *buf,
  int mb_y_offset,
  MB_PREDICTION_MODE *pbest_mode
) {
  MACROBLOCK   *const x  = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_PREDICTION_MODE best_mode = -1, mode;
  unsigned int best_err = INT_MAX;

  // calculate SATD for each intra prediction mode;
  // we're intentionally not doing 4x4, we just want a rough estimate
  for (mode = DC_PRED; mode <= TM_PRED; mode++) {
    unsigned int err;

    xd->mode_info_context->mbmi.mode = mode;
    vp9_build_intra_predictors_mby(xd);
    err = vp9_sad16x16(xd->predictor, 16, buf->y_buffer + mb_y_offset,
                       buf->y_stride, best_err);
    // find best
    if (err < best_err) {
      best_err  = err;
      best_mode = mode;
    }
  }

  if (pbest_mode)
    *pbest_mode = best_mode;

  return best_err;
}

static void update_mbgraph_mb_stats
(
  VP9_COMP *cpi,
  MBGRAPH_MB_STATS *stats,
  YV12_BUFFER_CONFIG *buf,
  int mb_y_offset,
  YV12_BUFFER_CONFIG *golden_ref,
  int_mv *prev_golden_ref_mv,
  int gld_y_offset,
  YV12_BUFFER_CONFIG *alt_ref,
  int_mv *prev_alt_ref_mv,
  int arf_y_offset
) {
  MACROBLOCK   *const x  = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  int intra_error;

  // FIXME in practice we're completely ignoring chroma here
  xd->dst.y_buffer = buf->y_buffer + mb_y_offset;

  // do intra 16x16 prediction
  intra_error = find_best_16x16_intra(cpi, buf, mb_y_offset, &stats->ref[INTRA_FRAME].m.mode);
  if (intra_error <= 0)
    intra_error = 1;
  stats->ref[INTRA_FRAME].err = intra_error;

  // Golden frame MV search, if it exists and is different than last frame
  if (golden_ref) {
    int g_motion_error = do_16x16_motion_search(cpi, prev_golden_ref_mv,
                                                &stats->ref[GOLDEN_FRAME].m.mv,
                                                buf, mb_y_offset,
                                                golden_ref, gld_y_offset);
    stats->ref[GOLDEN_FRAME].err = g_motion_error;
  } else {
    stats->ref[GOLDEN_FRAME].err = INT_MAX;
    stats->ref[GOLDEN_FRAME].m.mv.as_int = 0;
  }

  // Alt-ref frame MV search, if it exists and is different than last/golden frame
  if (alt_ref) {
    // int a_motion_error = do_16x16_motion_search(cpi, prev_alt_ref_mv,
    //                                            &stats->ref[ALTREF_FRAME].m.mv,
    //                                            buf, mb_y_offset,
    //                                            alt_ref, arf_y_offset);

    int a_motion_error =
      do_16x16_zerozero_search(cpi,
                               &stats->ref[ALTREF_FRAME].m.mv,
                               buf, mb_y_offset,
                               alt_ref, arf_y_offset);

    stats->ref[ALTREF_FRAME].err = a_motion_error;
  } else {
    stats->ref[ALTREF_FRAME].err = INT_MAX;
    stats->ref[ALTREF_FRAME].m.mv.as_int = 0;
  }
}

static void update_mbgraph_frame_stats
(
  VP9_COMP *cpi,
  MBGRAPH_FRAME_STATS *stats,
  YV12_BUFFER_CONFIG *buf,
  YV12_BUFFER_CONFIG *golden_ref,
  YV12_BUFFER_CONFIG *alt_ref
) {
  MACROBLOCK   *const x  = &cpi->mb;
  VP9_COMMON   *const cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  int mb_col, mb_row, offset = 0;
  int mb_y_offset = 0, arf_y_offset = 0, gld_y_offset = 0;
  int_mv arf_top_mv, gld_top_mv;
  MODE_INFO mi_local;

  // Set up limit values for motion vectors to prevent them extending outside the UMV borders
  arf_top_mv.as_int = 0;
  gld_top_mv.as_int = 0;
  x->mv_row_min     = -(VP9BORDERINPIXELS - 16 - VP9_INTERP_EXTEND);
  x->mv_row_max     = (cm->mb_rows - 1) * 16 + VP9BORDERINPIXELS
                      - 16 - VP9_INTERP_EXTEND;
  xd->up_available  = 0;
  xd->dst.y_stride  = buf->y_stride;
  xd->pre.y_stride  = buf->y_stride;
  xd->dst.uv_stride = buf->uv_stride;
  xd->mode_info_context = &mi_local;

  for (mb_row = 0; mb_row < cm->mb_rows; mb_row++) {
    int_mv arf_left_mv, gld_left_mv;
    int mb_y_in_offset  = mb_y_offset;
    int arf_y_in_offset = arf_y_offset;
    int gld_y_in_offset = gld_y_offset;

    // Set up limit values for motion vectors to prevent them extending outside the UMV borders
    arf_left_mv.as_int = arf_top_mv.as_int;
    gld_left_mv.as_int = gld_top_mv.as_int;
    x->mv_col_min      = -(VP9BORDERINPIXELS - 16 - VP9_INTERP_EXTEND);
    x->mv_col_max      = (cm->mb_cols - 1) * 16 + VP9BORDERINPIXELS
                         - 16 - VP9_INTERP_EXTEND;
    xd->left_available = 0;

    for (mb_col = 0; mb_col < cm->mb_cols; mb_col++) {
      MBGRAPH_MB_STATS *mb_stats = &stats->mb_stats[offset + mb_col];

      update_mbgraph_mb_stats(cpi, mb_stats, buf, mb_y_in_offset,
                              golden_ref, &gld_left_mv, gld_y_in_offset,
                              alt_ref,    &arf_left_mv, arf_y_in_offset);
      arf_left_mv.as_int = mb_stats->ref[ALTREF_FRAME].m.mv.as_int;
      gld_left_mv.as_int = mb_stats->ref[GOLDEN_FRAME].m.mv.as_int;
      if (mb_col == 0) {
        arf_top_mv.as_int = arf_left_mv.as_int;
        gld_top_mv.as_int = gld_left_mv.as_int;
      }
      xd->left_available = 1;
      mb_y_in_offset    += 16;
      gld_y_in_offset   += 16;
      arf_y_in_offset   += 16;
      x->mv_col_min     -= 16;
      x->mv_col_max     -= 16;
    }
    xd->up_available = 1;
    mb_y_offset     += buf->y_stride * 16;
    gld_y_offset    += golden_ref->y_stride * 16;
    if (alt_ref)
      arf_y_offset    += alt_ref->y_stride * 16;
    x->mv_row_min   -= 16;
    x->mv_row_max   -= 16;
    offset          += cm->mb_cols;
  }
}

// void separate_arf_mbs_byzz
static void separate_arf_mbs(VP9_COMP *cpi) {
  VP9_COMMON *const cm = &cpi->common;
  int mb_col, mb_row, offset, i;
  int ncnt[4];
  int n_frames = cpi->mbgraph_n_frames;

  int *arf_not_zz;

  CHECK_MEM_ERROR(arf_not_zz,
                  vpx_calloc(cm->mb_rows * cm->mb_cols * sizeof(*arf_not_zz), 1));

  // We are not interested in results beyond the alt ref itself.
  if (n_frames > cpi->frames_till_gf_update_due)
    n_frames = cpi->frames_till_gf_update_due;

  // defer cost to reference frames
  for (i = n_frames - 1; i >= 0; i--) {
    MBGRAPH_FRAME_STATS *frame_stats = &cpi->mbgraph_stats[i];

    for (offset = 0, mb_row = 0; mb_row < cm->mb_rows;
         offset += cm->mb_cols, mb_row++) {
      for (mb_col = 0; mb_col < cm->mb_cols; mb_col++) {
        MBGRAPH_MB_STATS *mb_stats =
          &frame_stats->mb_stats[offset + mb_col];

        int altref_err = mb_stats->ref[ALTREF_FRAME].err;
        int intra_err  = mb_stats->ref[INTRA_FRAME ].err;
        int golden_err = mb_stats->ref[GOLDEN_FRAME].err;

        // Test for altref vs intra and gf and that its mv was 0,0.
        if ((altref_err > 1000) ||
            (altref_err > intra_err) ||
            (altref_err > golden_err)) {
          arf_not_zz[offset + mb_col]++;
        }
      }
    }
  }

  vpx_memset(ncnt, 0, sizeof(ncnt));
  for (offset = 0, mb_row = 0; mb_row < cm->mb_rows;
       offset += cm->mb_cols, mb_row++) {
    for (mb_col = 0; mb_col < cm->mb_cols; mb_col++) {
      // If any of the blocks in the sequence failed then the MB
      // goes in segment 0
      if (arf_not_zz[offset + mb_col]) {
        ncnt[0]++;
        cpi->segmentation_map[offset + mb_col] = 0;
      } else {
        ncnt[1]++;
        cpi->segmentation_map[offset + mb_col] = 1;
      }
    }
  }

  // Only bother with segmentation if over 10% of the MBs in static segment
  // if ( ncnt[1] && (ncnt[0] / ncnt[1] < 10) )
  if (1) {
    // Note % of blocks that are marked as static
    if (cm->MBs)
      cpi->static_mb_pct = (ncnt[1] * 100) / cm->MBs;

    // This error case should not be reachable as this function should
    // never be called with the common data structure unititialized.
    else
      cpi->static_mb_pct = 0;

    cpi->seg0_cnt = ncnt[0];
    vp9_enable_segmentation((VP9_PTR) cpi);
  } else {
    cpi->static_mb_pct = 0;
    vp9_disable_segmentation((VP9_PTR) cpi);
  }

  // Free localy allocated storage
  vpx_free(arf_not_zz);
}

void vp9_update_mbgraph_stats
(
  VP9_COMP *cpi
) {
  VP9_COMMON *const cm = &cpi->common;
  int i, n_frames = vp9_lookahead_depth(cpi->lookahead);
  YV12_BUFFER_CONFIG *golden_ref = &cm->yv12_fb[cm->gld_fb_idx];

  // we need to look ahead beyond where the ARF transitions into
  // being a GF - so exit if we don't look ahead beyond that
  if (n_frames <= cpi->frames_till_gf_update_due)
    return;
  if (n_frames > (int)cpi->common.frames_till_alt_ref_frame)
    n_frames = cpi->common.frames_till_alt_ref_frame;
  if (n_frames > MAX_LAG_BUFFERS)
    n_frames = MAX_LAG_BUFFERS;

  cpi->mbgraph_n_frames = n_frames;
  for (i = 0; i < n_frames; i++) {
    MBGRAPH_FRAME_STATS *frame_stats = &cpi->mbgraph_stats[i];
    vpx_memset(frame_stats->mb_stats, 0,
               cm->mb_rows * cm->mb_cols * sizeof(*cpi->mbgraph_stats[i].mb_stats));
  }

  // do motion search to find contribution of each reference to data
  // later on in this GF group
  // FIXME really, the GF/last MC search should be done forward, and
  // the ARF MC search backwards, to get optimal results for MV caching
  for (i = 0; i < n_frames; i++) {
    MBGRAPH_FRAME_STATS *frame_stats = &cpi->mbgraph_stats[i];
    struct lookahead_entry *q_cur =
      vp9_lookahead_peek(cpi->lookahead, i);

    assert(q_cur != NULL);

    update_mbgraph_frame_stats(cpi, frame_stats, &q_cur->img,
                               golden_ref, cpi->Source);
  }

  vp9_clear_system_state();  // __asm emms;

  separate_arf_mbs(cpi);
}
