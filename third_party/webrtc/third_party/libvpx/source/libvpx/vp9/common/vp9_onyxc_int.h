/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_ONYXC_INT_H_
#define VP9_COMMON_VP9_ONYXC_INT_H_

#include "vpx_config.h"
#include "vpx/internal/vpx_codec_internal.h"
#include "vp9_rtcd.h"
#include "vp9/common/vp9_loopfilter.h"
#include "vp9/common/vp9_entropymv.h"
#include "vp9/common/vp9_entropy.h"
#include "vp9/common/vp9_entropymode.h"
#if CONFIG_POSTPROC
#include "vp9/common/vp9_postproc.h"
#endif

/*#ifdef PACKET_TESTING*/
#include "vp9/common/vp9_header.h"
/*#endif*/

/* Create/destroy static data structures. */

void vp9_initialize_common(void);

#define MINQ 0

#define MAXQ 255
#define QINDEX_BITS 8

#define QINDEX_RANGE (MAXQ + 1)

#define NUM_YV12_BUFFERS 4

#define COMP_PRED_CONTEXTS   2

typedef struct frame_contexts {
  vp9_prob bmode_prob[VP9_NKF_BINTRAMODES - 1];
  vp9_prob ymode_prob[VP9_YMODES - 1]; /* interframe intra mode probs */
  vp9_prob sb_ymode_prob[VP9_I32X32_MODES - 1];
  vp9_prob uv_mode_prob[VP9_YMODES][VP9_UV_MODES - 1];
  vp9_prob i8x8_mode_prob[VP9_I8X8_MODES - 1];
  vp9_prob sub_mv_ref_prob[SUBMVREF_COUNT][VP9_SUBMVREFS - 1];
  vp9_prob mbsplit_prob[VP9_NUMMBSPLITS - 1];
  vp9_coeff_probs coef_probs_4x4[BLOCK_TYPES_4X4];
  vp9_coeff_probs hybrid_coef_probs_4x4[BLOCK_TYPES_4X4];
  vp9_coeff_probs coef_probs_8x8[BLOCK_TYPES_8X8];
  vp9_coeff_probs hybrid_coef_probs_8x8[BLOCK_TYPES_8X8];
  vp9_coeff_probs coef_probs_16x16[BLOCK_TYPES_16X16];
  vp9_coeff_probs hybrid_coef_probs_16x16[BLOCK_TYPES_16X16];
  vp9_coeff_probs coef_probs_32x32[BLOCK_TYPES_32X32];

  nmv_context nmvc;
  nmv_context pre_nmvc;
  vp9_prob pre_bmode_prob[VP9_NKF_BINTRAMODES - 1];
  vp9_prob pre_ymode_prob[VP9_YMODES - 1]; /* interframe intra mode probs */
  vp9_prob pre_sb_ymode_prob[VP9_I32X32_MODES - 1];
  vp9_prob pre_uv_mode_prob[VP9_YMODES][VP9_UV_MODES - 1];
  vp9_prob pre_i8x8_mode_prob[VP9_I8X8_MODES - 1];
  vp9_prob pre_sub_mv_ref_prob[SUBMVREF_COUNT][VP9_SUBMVREFS - 1];
  vp9_prob pre_mbsplit_prob[VP9_NUMMBSPLITS - 1];
  unsigned int bmode_counts[VP9_NKF_BINTRAMODES];
  unsigned int ymode_counts[VP9_YMODES];   /* interframe intra mode probs */
  unsigned int sb_ymode_counts[VP9_I32X32_MODES];
  unsigned int uv_mode_counts[VP9_YMODES][VP9_UV_MODES];
  unsigned int i8x8_mode_counts[VP9_I8X8_MODES];   /* interframe intra probs */
  unsigned int sub_mv_ref_counts[SUBMVREF_COUNT][VP9_SUBMVREFS];
  unsigned int mbsplit_counts[VP9_NUMMBSPLITS];

  vp9_coeff_probs pre_coef_probs_4x4[BLOCK_TYPES_4X4];
  vp9_coeff_probs pre_hybrid_coef_probs_4x4[BLOCK_TYPES_4X4];
  vp9_coeff_probs pre_coef_probs_8x8[BLOCK_TYPES_8X8];
  vp9_coeff_probs pre_hybrid_coef_probs_8x8[BLOCK_TYPES_8X8];
  vp9_coeff_probs pre_coef_probs_16x16[BLOCK_TYPES_16X16];
  vp9_coeff_probs pre_hybrid_coef_probs_16x16[BLOCK_TYPES_16X16];
  vp9_coeff_probs pre_coef_probs_32x32[BLOCK_TYPES_32X32];

  vp9_coeff_count coef_counts_4x4[BLOCK_TYPES_4X4];
  vp9_coeff_count hybrid_coef_counts_4x4[BLOCK_TYPES_4X4];
  vp9_coeff_count coef_counts_8x8[BLOCK_TYPES_8X8];
  vp9_coeff_count hybrid_coef_counts_8x8[BLOCK_TYPES_8X8];
  vp9_coeff_count coef_counts_16x16[BLOCK_TYPES_16X16];
  vp9_coeff_count hybrid_coef_counts_16x16[BLOCK_TYPES_16X16];
  vp9_coeff_count coef_counts_32x32[BLOCK_TYPES_32X32];

  nmv_context_counts NMVcount;
  vp9_prob switchable_interp_prob[VP9_SWITCHABLE_FILTERS + 1]
                                 [VP9_SWITCHABLE_FILTERS - 1];
#if CONFIG_COMP_INTERINTRA_PRED
  unsigned int interintra_counts[2];
  vp9_prob interintra_prob;
  vp9_prob pre_interintra_prob;
#endif

  int vp9_mode_contexts[INTER_MODE_CONTEXTS][4];
  unsigned int mv_ref_ct[INTER_MODE_CONTEXTS][4][2];
} FRAME_CONTEXT;

typedef enum {
  RECON_CLAMP_REQUIRED        = 0,
  RECON_CLAMP_NOTREQUIRED     = 1
} CLAMP_TYPE;

typedef enum {
  SINGLE_PREDICTION_ONLY = 0,
  COMP_PREDICTION_ONLY   = 1,
  HYBRID_PREDICTION      = 2,
  NB_PREDICTION_TYPES    = 3,
} COMPPREDMODE_TYPE;

typedef enum {
  ONLY_4X4            = 0,
  ALLOW_8X8           = 1,
  ALLOW_16X16         = 2,
  ALLOW_32X32         = 3,
  TX_MODE_SELECT      = 4,
  NB_TXFM_MODES       = 5,
} TXFM_MODE;

typedef struct VP9Common {
  struct vpx_internal_error_info  error;

  DECLARE_ALIGNED(16, int16_t, Y1dequant[QINDEX_RANGE][16]);
  DECLARE_ALIGNED(16, int16_t, Y2dequant[QINDEX_RANGE][16]);
  DECLARE_ALIGNED(16, int16_t, UVdequant[QINDEX_RANGE][16]);

  int Width;
  int Height;
  int horiz_scale;
  int vert_scale;

  YUV_TYPE clr_type;
  CLAMP_TYPE  clamp_type;

  YV12_BUFFER_CONFIG *frame_to_show;

  YV12_BUFFER_CONFIG yv12_fb[NUM_YV12_BUFFERS];
  int fb_idx_ref_cnt[NUM_YV12_BUFFERS];
  int new_fb_idx, lst_fb_idx, gld_fb_idx, alt_fb_idx;

  YV12_BUFFER_CONFIG post_proc_buffer;
  YV12_BUFFER_CONFIG temp_scale_frame;


  FRAME_TYPE last_frame_type;  /* Save last frame's frame type for motion search. */
  FRAME_TYPE frame_type;

  int show_frame;

  int frame_flags;
  int MBs;
  int mb_rows;
  int mb_cols;
  int mode_info_stride;

  /* profile settings */
  int experimental;
  int mb_no_coeff_skip;
  TXFM_MODE txfm_mode;
  COMPPREDMODE_TYPE comp_pred_mode;
  int no_lpf;
  int use_bilinear_mc_filter;
  int full_pixel;

  int base_qindex;
  int last_kf_gf_q;  /* Q used on the last GF or KF */

  int y1dc_delta_q;
  int y2dc_delta_q;
  int y2ac_delta_q;
  int uvdc_delta_q;
  int uvac_delta_q;

  unsigned int frames_since_golden;
  unsigned int frames_till_alt_ref_frame;

  /* We allocate a MODE_INFO struct for each macroblock, together with
     an extra row on top and column on the left to simplify prediction. */

  MODE_INFO *mip; /* Base of allocated array */
  MODE_INFO *mi;  /* Corresponds to upper left visible macroblock */
  MODE_INFO *prev_mip; /* MODE_INFO array 'mip' from last decoded frame */
  MODE_INFO *prev_mi;  /* 'mi' from last frame (points into prev_mip) */


  // Persistent mb segment id map used in prediction.
  unsigned char *last_frame_seg_map;

  INTERPOLATIONFILTERTYPE mcomp_filter_type;
  LOOPFILTERTYPE filter_type;

  loop_filter_info_n lf_info;

  int filter_level;
  int last_sharpness_level;
  int sharpness_level;

  int refresh_last_frame;       /* Two state 0 = NO, 1 = YES */
  int refresh_golden_frame;     /* Two state 0 = NO, 1 = YES */
  int refresh_alt_ref_frame;     /* Two state 0 = NO, 1 = YES */

  int copy_buffer_to_gf;         /* 0 none, 1 Last to GF, 2 ARF to GF */
  int copy_buffer_to_arf;        /* 0 none, 1 Last to ARF, 2 GF to ARF */

  int refresh_entropy_probs;    /* Two state 0 = NO, 1 = YES */

  int ref_frame_sign_bias[MAX_REF_FRAMES];    /* Two state 0, 1 */

  /* Y,U,V,Y2 */
  ENTROPY_CONTEXT_PLANES *above_context;   /* row of context for each plane */
  ENTROPY_CONTEXT_PLANES left_context[4];  /* (up to) 4 contexts "" */

  /* keyframe block modes are predicted by their above, left neighbors */

  vp9_prob kf_bmode_prob[VP9_KF_BINTRAMODES]
                        [VP9_KF_BINTRAMODES]
                        [VP9_KF_BINTRAMODES - 1];
  vp9_prob kf_ymode_prob[8][VP9_YMODES - 1]; /* keyframe "" */
  vp9_prob sb_kf_ymode_prob[8][VP9_I32X32_MODES - 1];
  int kf_ymode_probs_index;
  int kf_ymode_probs_update;
  vp9_prob kf_uv_mode_prob[VP9_YMODES] [VP9_UV_MODES - 1];

  vp9_prob prob_intra_coded;
  vp9_prob prob_last_coded;
  vp9_prob prob_gf_coded;
  vp9_prob sb32_coded;
  vp9_prob sb64_coded;

  // Context probabilities when using predictive coding of segment id
  vp9_prob segment_pred_probs[PREDICTION_PROBS];
  unsigned char temporal_update;

  // Context probabilities for reference frame prediction
  unsigned char ref_scores[MAX_REF_FRAMES];
  vp9_prob ref_pred_probs[PREDICTION_PROBS];
  vp9_prob mod_refprobs[MAX_REF_FRAMES][PREDICTION_PROBS];

  vp9_prob prob_comppred[COMP_PRED_CONTEXTS];

  // FIXME contextualize
  vp9_prob prob_tx[TX_SIZE_MAX_SB - 1];

  vp9_prob mbskip_pred_probs[MBSKIP_CONTEXTS];

  FRAME_CONTEXT lfc_a; /* last alt ref entropy */
  FRAME_CONTEXT lfc; /* last frame entropy */
  FRAME_CONTEXT fc;  /* this frame entropy */

  unsigned int current_video_frame;
  int near_boffset[3];
  int version;

#ifdef PACKET_TESTING
  VP9_HEADER oh;
#endif
  double bitrate;
  double framerate;

#if CONFIG_POSTPROC
  struct postproc_state  postproc_state;
#endif

#if CONFIG_COMP_INTERINTRA_PRED
  int use_interintra;
#endif

} VP9_COMMON;

#endif  // VP9_COMMON_VP9_ONYXC_INT_H_
