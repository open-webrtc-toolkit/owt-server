/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vpx_config.h"
#include "vp9/common/vp9_loopfilter.h"
#include "vp9/common/vp9_onyxc_int.h"
#include "vpx_mem/vpx_mem.h"

#include "vp9/common/vp9_seg_common.h"

static void lf_init_lut(loop_filter_info_n *lfi) {
  int filt_lvl;

  for (filt_lvl = 0; filt_lvl <= MAX_LOOP_FILTER; filt_lvl++) {
    if (filt_lvl >= 40) {
      lfi->hev_thr_lut[KEY_FRAME][filt_lvl] = 2;
      lfi->hev_thr_lut[INTER_FRAME][filt_lvl] = 3;
    } else if (filt_lvl >= 20) {
      lfi->hev_thr_lut[KEY_FRAME][filt_lvl] = 1;
      lfi->hev_thr_lut[INTER_FRAME][filt_lvl] = 2;
    } else if (filt_lvl >= 15) {
      lfi->hev_thr_lut[KEY_FRAME][filt_lvl] = 1;
      lfi->hev_thr_lut[INTER_FRAME][filt_lvl] = 1;
    } else {
      lfi->hev_thr_lut[KEY_FRAME][filt_lvl] = 0;
      lfi->hev_thr_lut[INTER_FRAME][filt_lvl] = 0;
    }
  }

  lfi->mode_lf_lut[DC_PRED] = 1;
  lfi->mode_lf_lut[D45_PRED] = 1;
  lfi->mode_lf_lut[D135_PRED] = 1;
  lfi->mode_lf_lut[D117_PRED] = 1;
  lfi->mode_lf_lut[D153_PRED] = 1;
  lfi->mode_lf_lut[D27_PRED] = 1;
  lfi->mode_lf_lut[D63_PRED] = 1;
  lfi->mode_lf_lut[V_PRED] = 1;
  lfi->mode_lf_lut[H_PRED] = 1;
  lfi->mode_lf_lut[TM_PRED] = 1;
  lfi->mode_lf_lut[B_PRED]  = 0;
  lfi->mode_lf_lut[I8X8_PRED] = 0;
  lfi->mode_lf_lut[ZEROMV]  = 1;
  lfi->mode_lf_lut[NEARESTMV] = 2;
  lfi->mode_lf_lut[NEARMV] = 2;
  lfi->mode_lf_lut[NEWMV] = 2;
  lfi->mode_lf_lut[SPLITMV] = 3;
}

void vp9_loop_filter_update_sharpness(loop_filter_info_n *lfi,
                                      int sharpness_lvl) {
  int i;

  /* For each possible value for the loop filter fill out limits */
  for (i = 0; i <= MAX_LOOP_FILTER; i++) {
    int filt_lvl = i;
    int block_inside_limit = 0;

    /* Set loop filter paramaeters that control sharpness. */
    block_inside_limit = filt_lvl >> (sharpness_lvl > 0);
    block_inside_limit = block_inside_limit >> (sharpness_lvl > 4);

    if (sharpness_lvl > 0) {
      if (block_inside_limit > (9 - sharpness_lvl))
        block_inside_limit = (9 - sharpness_lvl);
    }

    if (block_inside_limit < 1)
      block_inside_limit = 1;

    vpx_memset(lfi->lim[i], block_inside_limit, SIMD_WIDTH);
    vpx_memset(lfi->blim[i], (2 * filt_lvl + block_inside_limit),
               SIMD_WIDTH);
    vpx_memset(lfi->mblim[i], (2 * (filt_lvl + 2) + block_inside_limit),
               SIMD_WIDTH);
  }
}

void vp9_loop_filter_init(VP9_COMMON *cm) {
  loop_filter_info_n *lfi = &cm->lf_info;
  int i;

  /* init limits for given sharpness*/
  vp9_loop_filter_update_sharpness(lfi, cm->sharpness_level);
  cm->last_sharpness_level = cm->sharpness_level;

  /* init LUT for lvl  and hev thr picking */
  lf_init_lut(lfi);

  /* init hev threshold const vectors */
  for (i = 0; i < 4; i++) {
    vpx_memset(lfi->hev_thr[i], i, SIMD_WIDTH);
  }
}

void vp9_loop_filter_frame_init(VP9_COMMON *cm,
                                MACROBLOCKD *xd,
                                int default_filt_lvl) {
  int seg,  /* segment number */
      ref,  /* index in ref_lf_deltas */
      mode; /* index in mode_lf_deltas */

  loop_filter_info_n *lfi = &cm->lf_info;

  /* update limits if sharpness has changed */
  // printf("vp9_loop_filter_frame_init %d\n", default_filt_lvl);
  // printf("sharpness level: %d [%d]\n",
  //        cm->sharpness_level, cm->last_sharpness_level);
  if (cm->last_sharpness_level != cm->sharpness_level) {
    vp9_loop_filter_update_sharpness(lfi, cm->sharpness_level);
    cm->last_sharpness_level = cm->sharpness_level;
  }

  for (seg = 0; seg < MAX_MB_SEGMENTS; seg++) {
    int lvl_seg = default_filt_lvl;
    int lvl_ref, lvl_mode;


    // Set the baseline filter values for each segment
    if (vp9_segfeature_active(xd, seg, SEG_LVL_ALT_LF)) {
      /* Abs value */
      if (xd->mb_segment_abs_delta == SEGMENT_ABSDATA) {
        lvl_seg = vp9_get_segdata(xd, seg, SEG_LVL_ALT_LF);
      } else { /* Delta Value */
        lvl_seg += vp9_get_segdata(xd, seg, SEG_LVL_ALT_LF);
        lvl_seg = clamp(lvl_seg, 0, 63);
      }
    }

    if (!xd->mode_ref_lf_delta_enabled) {
      /* we could get rid of this if we assume that deltas are set to
       * zero when not in use; encoder always uses deltas
       */
      vpx_memset(lfi->lvl[seg][0], lvl_seg, 4 * 4);
      continue;
    }

    lvl_ref = lvl_seg;

    /* INTRA_FRAME */
    ref = INTRA_FRAME;

    /* Apply delta for reference frame */
    lvl_ref += xd->ref_lf_deltas[ref];

    /* Apply delta for Intra modes */
    mode = 0; /* B_PRED */
    /* Only the split mode BPRED has a further special case */
    lvl_mode = clamp(lvl_ref +  xd->mode_lf_deltas[mode], 0, 63);

    lfi->lvl[seg][ref][mode] = lvl_mode;

    mode = 1; /* all the rest of Intra modes */
    lvl_mode = clamp(lvl_ref, 0, 63);
    lfi->lvl[seg][ref][mode] = lvl_mode;

    /* LAST, GOLDEN, ALT */
    for (ref = 1; ref < MAX_REF_FRAMES; ref++) {
      int lvl_ref = lvl_seg;

      /* Apply delta for reference frame */
      lvl_ref += xd->ref_lf_deltas[ref];

      /* Apply delta for Inter modes */
      for (mode = 1; mode < 4; mode++) {
        lvl_mode = clamp(lvl_ref + xd->mode_lf_deltas[mode], 0, 63);
        lfi->lvl[seg][ref][mode] = lvl_mode;
      }
    }
  }
}

// Determine if we should skip inner-MB loop filtering within a MB
// The current condition is that the loop filtering is skipped only
// the MB uses a prediction size of 16x16 and either 16x16 transform
// is used or there is no residue at all.
static int mb_lf_skip(const MB_MODE_INFO *const mbmi) {
  const MB_PREDICTION_MODE mode = mbmi->mode;
  const int skip_coef = mbmi->mb_skip_coeff;
  const int tx_size = mbmi->txfm_size;
  return mode != B_PRED && mode != I8X8_PRED && mode != SPLITMV &&
         (tx_size >= TX_16X16 || skip_coef);
}

// Determine if we should skip MB loop filtering on a MB edge within
// a superblock, the current condition is that MB loop filtering is
// skipped only when both MBs do not use inner MB loop filtering, and
// same motion vector with same reference frame
static int sb_mb_lf_skip(const MODE_INFO *const mip0,
                         const MODE_INFO *const mip1) {
  const MB_MODE_INFO *mbmi0 = &mip0->mbmi;
  const MB_MODE_INFO *mbmi1 = &mip0->mbmi;
  return mb_lf_skip(mbmi0) && mb_lf_skip(mbmi1) &&
         (mbmi0->ref_frame == mbmi1->ref_frame) &&
         (mbmi0->mv[mbmi0->ref_frame].as_int ==
          mbmi1->mv[mbmi1->ref_frame].as_int) &&
         mbmi0->ref_frame != INTRA_FRAME;
}

void vp9_loop_filter_frame(VP9_COMMON *cm,
                           MACROBLOCKD *xd,
                           int frame_filter_level,
                           int y_only,
                           int dering) {
  YV12_BUFFER_CONFIG *post = cm->frame_to_show;
  loop_filter_info_n *lfi_n = &cm->lf_info;
  struct loop_filter_info lfi;
  const FRAME_TYPE frame_type = cm->frame_type;
  int mb_row, mb_col;
  uint8_t *y_ptr, *u_ptr, *v_ptr;

  /* Point at base of Mb MODE_INFO list */
  const MODE_INFO *mode_info_context = cm->mi;
  const int mis = cm->mode_info_stride;

  /* Initialize the loop filter for this frame. */
  vp9_loop_filter_frame_init(cm, xd, frame_filter_level);
  /* Set up the buffer pointers */
  y_ptr = post->y_buffer;
  if (y_only) {
    u_ptr = 0;
    v_ptr = 0;
  } else {
    u_ptr = post->u_buffer;
    v_ptr = post->v_buffer;
  }

  /* vp9_filter each macro block */
  for (mb_row = 0; mb_row < cm->mb_rows; mb_row++) {
    for (mb_col = 0; mb_col < cm->mb_cols; mb_col++) {
      const MB_PREDICTION_MODE mode = mode_info_context->mbmi.mode;
      const int mode_index = lfi_n->mode_lf_lut[mode];
      const int seg = mode_info_context->mbmi.segment_id;
      const int ref_frame = mode_info_context->mbmi.ref_frame;
      const int filter_level = lfi_n->lvl[seg][ref_frame][mode_index];
      if (filter_level) {
        const int skip_lf = mb_lf_skip(&mode_info_context->mbmi);
        const int tx_size = mode_info_context->mbmi.txfm_size;
        if (cm->filter_type == NORMAL_LOOPFILTER) {
          const int hev_index = lfi_n->hev_thr_lut[frame_type][filter_level];
          lfi.mblim = lfi_n->mblim[filter_level];
          lfi.blim = lfi_n->blim[filter_level];
          lfi.lim = lfi_n->lim[filter_level];
          lfi.hev_thr = lfi_n->hev_thr[hev_index];

          if (mb_col > 0 &&
              !((mb_col & 1) && mode_info_context->mbmi.sb_type &&
                (sb_mb_lf_skip(mode_info_context - 1, mode_info_context) ||
                 tx_size >= TX_32X32))
              ) {
            if (tx_size >= TX_16X16)
              vp9_lpf_mbv_w(y_ptr, u_ptr, v_ptr, post->y_stride,
                            post->uv_stride, &lfi);
            else
              vp9_loop_filter_mbv(y_ptr, u_ptr, v_ptr, post->y_stride,
                                  post->uv_stride, &lfi);
          }
          if (!skip_lf) {
            if (tx_size >= TX_8X8) {
              if (tx_size == TX_8X8 && (mode == I8X8_PRED || mode == SPLITMV))
                vp9_loop_filter_bv8x8(y_ptr, u_ptr, v_ptr, post->y_stride,
                                      post->uv_stride, &lfi);
              else
                vp9_loop_filter_bv8x8(y_ptr, NULL, NULL, post->y_stride,
                                      post->uv_stride, &lfi);
            } else {
              vp9_loop_filter_bv(y_ptr, u_ptr, v_ptr, post->y_stride,
                                 post->uv_stride, &lfi);
            }
          }
          /* don't apply across umv border */
          if (mb_row > 0 &&
              !((mb_row & 1) && mode_info_context->mbmi.sb_type &&
                (sb_mb_lf_skip(mode_info_context - mis, mode_info_context) ||
                tx_size >= TX_32X32))
              ) {
            if (tx_size >= TX_16X16)
              vp9_lpf_mbh_w(y_ptr, u_ptr, v_ptr, post->y_stride,
                            post->uv_stride, &lfi);
            else
              vp9_loop_filter_mbh(y_ptr, u_ptr, v_ptr, post->y_stride,
                                  post->uv_stride, &lfi);
          }
          if (!skip_lf) {
            if (tx_size >= TX_8X8) {
              if (tx_size == TX_8X8 && (mode == I8X8_PRED || mode == SPLITMV))
                vp9_loop_filter_bh8x8(y_ptr, u_ptr, v_ptr, post->y_stride,
                                      post->uv_stride, &lfi);
              else
                vp9_loop_filter_bh8x8(y_ptr, NULL, NULL, post->y_stride,
                                      post->uv_stride, &lfi);
            } else {
              vp9_loop_filter_bh(y_ptr, u_ptr, v_ptr, post->y_stride,
                                 post->uv_stride, &lfi);
            }
          }
#if CONFIG_LOOP_DERING
          if (dering) {
            if (mb_row && mb_row < cm->mb_rows - 1 &&
                mb_col && mb_col < cm->mb_cols - 1) {
              vp9_post_proc_down_and_across(y_ptr, y_ptr,
                                            post->y_stride, post->y_stride,
                                            16, 16, dering);
              if (!y_only) {
                vp9_post_proc_down_and_across(u_ptr, u_ptr,
                                              post->uv_stride, post->uv_stride,
                                              8, 8, dering);
                vp9_post_proc_down_and_across(v_ptr, v_ptr,
                                              post->uv_stride, post->uv_stride,
                                              8, 8, dering);
              }
            } else {
              // Adjust the filter so that no out-of-frame data is used.
              uint8_t *dr_y = y_ptr, *dr_u = u_ptr, *dr_v = v_ptr;
              int w_adjust = 0;
              int h_adjust = 0;

              if (mb_col == 0) {
                dr_y += 2;
                dr_u += 2;
                dr_v += 2;
                w_adjust += 2;
              }
              if (mb_col == cm->mb_cols - 1)
                w_adjust += 2;
              if (mb_row == 0) {
                dr_y += 2 * post->y_stride;
                dr_u += 2 * post->uv_stride;
                dr_v += 2 * post->uv_stride;
                h_adjust += 2;
              }
              if (mb_row == cm->mb_rows - 1)
                h_adjust += 2;
              vp9_post_proc_down_and_across_c(dr_y, dr_y,
                                              post->y_stride, post->y_stride,
                                              16 - w_adjust, 16 - h_adjust,
                                              dering);
              if (!y_only) {
                vp9_post_proc_down_and_across_c(dr_u, dr_u,
                                                post->uv_stride,
                                                post->uv_stride,
                                                8 - w_adjust, 8 - h_adjust,
                                                dering);
                vp9_post_proc_down_and_across_c(dr_v, dr_v,
                                                post->uv_stride,
                                                post->uv_stride,
                                                8 - w_adjust, 8 - h_adjust,
                                                dering);
              }
            }
          }
#endif
        } else {
          // FIXME: Not 8x8 aware
          if (mb_col > 0 &&
              !(skip_lf && mb_lf_skip(&mode_info_context[-1].mbmi)) &&
              !((mb_col & 1) && mode_info_context->mbmi.sb_type))
            vp9_loop_filter_simple_mbv(y_ptr, post->y_stride,
                                       lfi_n->mblim[filter_level]);
          if (!skip_lf)
            vp9_loop_filter_simple_bv(y_ptr, post->y_stride,
                                      lfi_n->blim[filter_level]);

          /* don't apply across umv border */
          if (mb_row > 0 &&
              !(skip_lf && mb_lf_skip(&mode_info_context[-mis].mbmi)) &&
              !((mb_row & 1) && mode_info_context->mbmi.sb_type))
            vp9_loop_filter_simple_mbh(y_ptr, post->y_stride,
                                       lfi_n->mblim[filter_level]);
          if (!skip_lf)
            vp9_loop_filter_simple_bh(y_ptr, post->y_stride,
                                      lfi_n->blim[filter_level]);
        }
      }
      y_ptr += 16;
      if (!y_only) {
        u_ptr += 8;
        v_ptr += 8;
      }
      mode_info_context++;     /* step to next MB */
    }
    y_ptr += post->y_stride  * 16 - post->y_width;
    if (!y_only) {
      u_ptr += post->uv_stride *  8 - post->uv_width;
      v_ptr += post->uv_stride *  8 - post->uv_width;
    }
    mode_info_context++;         /* Skip border mb */
  }
}


void vp9_loop_filter_partial_frame(VP9_COMMON *cm, MACROBLOCKD *xd,
                                   int default_filt_lvl) {
  YV12_BUFFER_CONFIG *post = cm->frame_to_show;

  uint8_t *y_ptr;
  int mb_row;
  int mb_col;
  int mb_cols = post->y_width  >> 4;

  int linestocopy, i;

  loop_filter_info_n *lfi_n = &cm->lf_info;
  struct loop_filter_info lfi;

  int filter_level;
  int alt_flt_enabled = xd->segmentation_enabled;
  FRAME_TYPE frame_type = cm->frame_type;

  const MODE_INFO *mode_info_context;

  int lvl_seg[MAX_MB_SEGMENTS];

  mode_info_context = cm->mi + (post->y_height >> 5) * (mb_cols + 1);

  /* 3 is a magic number. 4 is probably magic too */
  linestocopy = (post->y_height >> (4 + 3));

  if (linestocopy < 1)
    linestocopy = 1;

  linestocopy <<= 4;

  /* Note the baseline filter values for each segment */
  /* See vp9_loop_filter_frame_init. Rather than call that for each change
   * to default_filt_lvl, copy the relevant calculation here.
   */
  if (alt_flt_enabled) {
    for (i = 0; i < MAX_MB_SEGMENTS; i++) {
      if (xd->mb_segment_abs_delta == SEGMENT_ABSDATA) {
        // Abs value
        lvl_seg[i] = vp9_get_segdata(xd, i, SEG_LVL_ALT_LF);
      } else {
        // Delta Value
        lvl_seg[i] = default_filt_lvl + vp9_get_segdata(xd, i, SEG_LVL_ALT_LF);
        lvl_seg[i] = clamp(lvl_seg[i], 0, 63);
      }
    }
  }

  /* Set up the buffer pointers */
  y_ptr = post->y_buffer + (post->y_height >> 5) * 16 * post->y_stride;

  /* vp9_filter each macro block */
  for (mb_row = 0; mb_row < (linestocopy >> 4); mb_row++) {
    for (mb_col = 0; mb_col < mb_cols; mb_col++) {
      int skip_lf = (mode_info_context->mbmi.mode != B_PRED &&
                     mode_info_context->mbmi.mode != I8X8_PRED &&
                     mode_info_context->mbmi.mode != SPLITMV &&
                     mode_info_context->mbmi.mb_skip_coeff);

      if (alt_flt_enabled)
        filter_level = lvl_seg[mode_info_context->mbmi.segment_id];
      else
        filter_level = default_filt_lvl;

      if (filter_level) {
        if (cm->filter_type == NORMAL_LOOPFILTER) {
          const int hev_index = lfi_n->hev_thr_lut[frame_type][filter_level];
          lfi.mblim = lfi_n->mblim[filter_level];
          lfi.blim = lfi_n->blim[filter_level];
          lfi.lim = lfi_n->lim[filter_level];
          lfi.hev_thr = lfi_n->hev_thr[hev_index];

          if (mb_col > 0)
            vp9_loop_filter_mbv(y_ptr, 0, 0, post->y_stride, 0, &lfi);

          if (!skip_lf)
            vp9_loop_filter_bv(y_ptr, 0, 0, post->y_stride, 0, &lfi);

          vp9_loop_filter_mbh(y_ptr, 0, 0, post->y_stride, 0, &lfi);

          if (!skip_lf)
            vp9_loop_filter_bh(y_ptr, 0, 0, post->y_stride, 0, &lfi);
        } else {
          if (mb_col > 0)
            vp9_loop_filter_simple_mbv (y_ptr, post->y_stride,
                                        lfi_n->mblim[filter_level]);

          if (!skip_lf)
            vp9_loop_filter_simple_bv(y_ptr, post->y_stride,
                                      lfi_n->blim[filter_level]);

          vp9_loop_filter_simple_mbh(y_ptr, post->y_stride,
                                     lfi_n->mblim[filter_level]);

          if (!skip_lf)
            vp9_loop_filter_simple_bh(y_ptr, post->y_stride,
                                      lfi_n->blim[filter_level]);
        }
      }

      y_ptr += 16;
      mode_info_context += 1;      /* step to next MB */
    }

    y_ptr += post->y_stride  * 16 - post->y_width;
    mode_info_context += 1;          /* Skip border mb */
  }
}
