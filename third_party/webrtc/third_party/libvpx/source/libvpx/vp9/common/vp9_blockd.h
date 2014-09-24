/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_COMMON_VP9_BLOCKD_H_
#define VP9_COMMON_VP9_BLOCKD_H_

void vpx_log(const char *format, ...);

#include "./vpx_config.h"
#include "vpx_scale/yv12config.h"
#include "vp9/common/vp9_mv.h"
#include "vp9/common/vp9_treecoder.h"
#include "vp9/common/vp9_subpixel.h"
#include "vpx_ports/mem.h"
#include "vp9/common/vp9_common.h"

#define TRUE    1
#define FALSE   0

// #define MODE_STATS

/*#define DCPRED 1*/
#define DCPREDSIMTHRESH 0
#define DCPREDCNTTHRESH 3

#define MB_FEATURE_TREE_PROBS   3
#define PREDICTION_PROBS 3

#define MBSKIP_CONTEXTS 3

#define MAX_MB_SEGMENTS         4

#define MAX_REF_LF_DELTAS       4
#define MAX_MODE_LF_DELTAS      4

/* Segment Feature Masks */
#define SEGMENT_DELTADATA   0
#define SEGMENT_ABSDATA     1
#define MAX_MV_REFS 9
#define MAX_MV_REF_CANDIDATES 4

#if CONFIG_DWTDCTHYBRID
#define DWT_MAX_LENGTH     64
#define DWT_TYPE           26    // 26/53/97
#define DWT_PRECISION_BITS 2
#define DWT_PRECISION_RND  ((1 << DWT_PRECISION_BITS) / 2)

#define DWTDCT16X16        0
#define DWTDCT16X16_LEAN   1
#define DWTDCT8X8          2
#define DWTDCT_TYPE        DWTDCT16X16_LEAN
#endif

typedef struct {
  int r, c;
} POS;

typedef enum PlaneType {
  PLANE_TYPE_Y_NO_DC = 0,
  PLANE_TYPE_Y2,
  PLANE_TYPE_UV,
  PLANE_TYPE_Y_WITH_DC,
} PLANE_TYPE;

typedef char ENTROPY_CONTEXT;
typedef struct {
  ENTROPY_CONTEXT y1[4];
  ENTROPY_CONTEXT u[2];
  ENTROPY_CONTEXT v[2];
  ENTROPY_CONTEXT y2;
} ENTROPY_CONTEXT_PLANES;

#define VP9_COMBINEENTROPYCONTEXTS( Dest, A, B) \
  Dest = ((A)!=0) + ((B)!=0);

typedef enum {
  KEY_FRAME = 0,
  INTER_FRAME = 1
} FRAME_TYPE;

typedef enum
{
#if CONFIG_ENABLE_6TAP
  SIXTAP,
#endif
  EIGHTTAP_SMOOTH,
  EIGHTTAP,
  EIGHTTAP_SHARP,
  BILINEAR,
  SWITCHABLE  /* should be the last one */
} INTERPOLATIONFILTERTYPE;

typedef enum
{
  DC_PRED,            /* average of above and left pixels */
  V_PRED,             /* vertical prediction */
  H_PRED,             /* horizontal prediction */
  D45_PRED,           /* Directional 45 deg prediction  [anti-clockwise from 0 deg hor] */
  D135_PRED,          /* Directional 135 deg prediction [anti-clockwise from 0 deg hor] */
  D117_PRED,          /* Directional 112 deg prediction [anti-clockwise from 0 deg hor] */
  D153_PRED,          /* Directional 157 deg prediction [anti-clockwise from 0 deg hor] */
  D27_PRED,           /* Directional 22 deg prediction  [anti-clockwise from 0 deg hor] */
  D63_PRED,           /* Directional 67 deg prediction  [anti-clockwise from 0 deg hor] */
  TM_PRED,            /* Truemotion prediction */
  I8X8_PRED,          /* 8x8 based prediction, each 8x8 has its own prediction mode */
  B_PRED,             /* block based prediction, each block has its own prediction mode */
  NEARESTMV,
  NEARMV,
  ZEROMV,
  NEWMV,
  SPLITMV,
  MB_MODE_COUNT
} MB_PREDICTION_MODE;

// Segment level features.
typedef enum {
  SEG_LVL_ALT_Q = 0,               // Use alternate Quantizer ....
  SEG_LVL_ALT_LF = 1,              // Use alternate loop filter value...
  SEG_LVL_REF_FRAME = 2,           // Optional Segment reference frame
  SEG_LVL_MODE = 3,                // Optional Segment mode
  SEG_LVL_EOB = 4,                 // EOB end stop marker.
  SEG_LVL_TRANSFORM = 5,           // Block transform size.
  SEG_LVL_MAX = 6                  // Number of MB level features supported
} SEG_LVL_FEATURES;

// Segment level features.
typedef enum {
  TX_4X4 = 0,                      // 4x4 dct transform
  TX_8X8 = 1,                      // 8x8 dct transform
  TX_16X16 = 2,                    // 16x16 dct transform
  TX_SIZE_MAX_MB = 3,              // Number of different transforms available
  TX_32X32 = TX_SIZE_MAX_MB,       // 32x32 dct transform
  TX_SIZE_MAX_SB,                  // Number of transforms available to SBs
} TX_SIZE;

typedef enum {
  DCT_DCT   = 0,                      // DCT  in both horizontal and vertical
  ADST_DCT  = 1,                      // ADST in vertical, DCT in horizontal
  DCT_ADST  = 2,                      // DCT  in vertical, ADST in horizontal
  ADST_ADST = 3                       // ADST in both directions
} TX_TYPE;

#define VP9_YMODES  (B_PRED + 1)
#define VP9_UV_MODES (TM_PRED + 1)
#define VP9_I8X8_MODES (TM_PRED + 1)
#define VP9_I32X32_MODES (TM_PRED + 1)

#define VP9_MVREFS (1 + SPLITMV - NEARESTMV)

#if CONFIG_LOSSLESS
#define WHT_UPSCALE_FACTOR 3
#define Y2_WHT_UPSCALE_FACTOR 2
#endif

typedef enum {
  B_DC_PRED,          /* average of above and left pixels */
  B_TM_PRED,

  B_VE_PRED,          /* vertical prediction */
  B_HE_PRED,          /* horizontal prediction */

  B_LD_PRED,
  B_RD_PRED,

  B_VR_PRED,
  B_VL_PRED,
  B_HD_PRED,
  B_HU_PRED,
#if CONFIG_NEWBINTRAMODES
  B_CONTEXT_PRED,
#endif

  LEFT4X4,
  ABOVE4X4,
  ZERO4X4,
  NEW4X4,

  B_MODE_COUNT
} B_PREDICTION_MODE;

#define VP9_BINTRAMODES (LEFT4X4)
#define VP9_SUBMVREFS (1 + NEW4X4 - LEFT4X4)

#if CONFIG_NEWBINTRAMODES
/* The number of B_PRED intra modes that are replaced by B_CONTEXT_PRED */
#define CONTEXT_PRED_REPLACEMENTS  0
#define VP9_KF_BINTRAMODES (VP9_BINTRAMODES - 1)
#define VP9_NKF_BINTRAMODES  (VP9_BINTRAMODES - CONTEXT_PRED_REPLACEMENTS)
#else
#define VP9_KF_BINTRAMODES (VP9_BINTRAMODES)   /* 10 */
#define VP9_NKF_BINTRAMODES (VP9_BINTRAMODES)  /* 10 */
#endif

typedef enum {
  PARTITIONING_16X8 = 0,
  PARTITIONING_8X16,
  PARTITIONING_8X8,
  PARTITIONING_4X4,
  NB_PARTITIONINGS,
} SPLITMV_PARTITIONING_TYPE;

/* For keyframes, intra block modes are predicted by the (already decoded)
   modes for the Y blocks to the left and above us; for interframes, there
   is a single probability table. */

union b_mode_info {
  struct {
    B_PREDICTION_MODE first;
    TX_TYPE           tx_type;
#if CONFIG_NEWBINTRAMODES
    B_PREDICTION_MODE context;
#endif
  } as_mode;
  struct {
    int_mv first;
    int_mv second;
  } as_mv;
};

typedef enum {
  NONE = -1,
  INTRA_FRAME = 0,
  LAST_FRAME = 1,
  GOLDEN_FRAME = 2,
  ALTREF_FRAME = 3,
  MAX_REF_FRAMES = 4
} MV_REFERENCE_FRAME;

typedef enum {
  BLOCK_SIZE_MB16X16 = 0,
  BLOCK_SIZE_SB32X32 = 1,
  BLOCK_SIZE_SB64X64 = 2,
} BLOCK_SIZE_TYPE;

typedef struct {
  MB_PREDICTION_MODE mode, uv_mode;
#if CONFIG_COMP_INTERINTRA_PRED
  MB_PREDICTION_MODE interintra_mode, interintra_uv_mode;
#endif
  MV_REFERENCE_FRAME ref_frame, second_ref_frame;
  TX_SIZE txfm_size;
  int_mv mv[2]; // for each reference frame used
  int_mv ref_mvs[MAX_REF_FRAMES][MAX_MV_REF_CANDIDATES];
  int_mv best_mv, best_second_mv;
#if CONFIG_NEW_MVREF
  int best_index, best_second_index;
#endif

  int mb_mode_context[MAX_REF_FRAMES];

  SPLITMV_PARTITIONING_TYPE partitioning;
  unsigned char mb_skip_coeff;                                /* does this mb has coefficients at all, 1=no coefficients, 0=need decode tokens */
  unsigned char need_to_clamp_mvs;
  unsigned char need_to_clamp_secondmv;
  unsigned char segment_id;                  /* Which set of segmentation parameters should be used for this MB */

  // Flags used for prediction status of various bistream signals
  unsigned char seg_id_predicted;
  unsigned char ref_predicted;

  // Indicates if the mb is part of the image (1) vs border (0)
  // This can be useful in determining whether the MB provides
  // a valid predictor
  unsigned char mb_in_image;

  INTERPOLATIONFILTERTYPE interp_filter;

  BLOCK_SIZE_TYPE sb_type;
} MB_MODE_INFO;

typedef struct {
  MB_MODE_INFO mbmi;
  union b_mode_info bmi[16];
} MODE_INFO;

typedef struct blockd {
  int16_t *qcoeff;
  int16_t *dqcoeff;
  uint8_t *predictor;
  int16_t *diff;
  int16_t *dequant;

  /* 16 Y blocks, 4 U blocks, 4 V blocks each with 16 entries */
  uint8_t **base_pre;
  uint8_t **base_second_pre;
  int pre;
  int pre_stride;

  uint8_t **base_dst;
  int dst;
  int dst_stride;

  int eob;

  union b_mode_info bmi;
} BLOCKD;

typedef struct superblockd {
  /* 32x32 Y and 16x16 U/V. No 2nd order transform yet. */
  DECLARE_ALIGNED(16, int16_t, diff[32*32+16*16*2]);
  DECLARE_ALIGNED(16, int16_t, qcoeff[32*32+16*16*2]);
  DECLARE_ALIGNED(16, int16_t, dqcoeff[32*32+16*16*2]);
} SUPERBLOCKD;

typedef struct macroblockd {
  DECLARE_ALIGNED(16, int16_t,  diff[400]);      /* from idct diff */
  DECLARE_ALIGNED(16, uint8_t,  predictor[384]);
  DECLARE_ALIGNED(16, int16_t,  qcoeff[400]);
  DECLARE_ALIGNED(16, int16_t,  dqcoeff[400]);
  DECLARE_ALIGNED(16, uint16_t, eobs[25]);

  SUPERBLOCKD sb_coeff_data;

  /* 16 Y blocks, 4 U, 4 V, 1 DC 2nd order block, each with 16 entries. */
  BLOCKD block[25];
  int fullpixel_mask;

  YV12_BUFFER_CONFIG pre; /* Filtered copy of previous frame reconstruction */
  struct {
    uint8_t *y_buffer, *u_buffer, *v_buffer;
  } second_pre;
  YV12_BUFFER_CONFIG dst;

  MODE_INFO *prev_mode_info_context;
  MODE_INFO *mode_info_context;
  int mode_info_stride;

  FRAME_TYPE frame_type;

  int up_available;
  int left_available;

  /* Y,U,V,Y2 */
  ENTROPY_CONTEXT_PLANES *above_context;
  ENTROPY_CONTEXT_PLANES *left_context;

  /* 0 indicates segmentation at MB level is not enabled. Otherwise the individual bits indicate which features are active. */
  unsigned char segmentation_enabled;

  /* 0 (do not update) 1 (update) the macroblock segmentation map. */
  unsigned char update_mb_segmentation_map;

  /* 0 (do not update) 1 (update) the macroblock segmentation feature data. */
  unsigned char update_mb_segmentation_data;

  /* 0 (do not update) 1 (update) the macroblock segmentation feature data. */
  unsigned char mb_segment_abs_delta;

  /* Per frame flags that define which MB level features (such as quantizer or loop filter level) */
  /* are enabled and when enabled the proabilities used to decode the per MB flags in MB_MODE_INFO */

  // Probability Tree used to code Segment number
  vp9_prob mb_segment_tree_probs[MB_FEATURE_TREE_PROBS];

#if CONFIG_NEW_MVREF
  vp9_prob mb_mv_ref_probs[MAX_REF_FRAMES][MAX_MV_REF_CANDIDATES-1];
#endif

  // Segment features
  signed char segment_feature_data[MAX_MB_SEGMENTS][SEG_LVL_MAX];
  unsigned int segment_feature_mask[MAX_MB_SEGMENTS];

  /* mode_based Loop filter adjustment */
  unsigned char mode_ref_lf_delta_enabled;
  unsigned char mode_ref_lf_delta_update;

  /* Delta values have the range +/- MAX_LOOP_FILTER */
  signed char last_ref_lf_deltas[MAX_REF_LF_DELTAS];                /* 0 = Intra, Last, GF, ARF */
  signed char ref_lf_deltas[MAX_REF_LF_DELTAS];                     /* 0 = Intra, Last, GF, ARF */
  signed char last_mode_lf_deltas[MAX_MODE_LF_DELTAS];              /* 0 = BPRED, ZERO_MV, MV, SPLIT */
  signed char mode_lf_deltas[MAX_MODE_LF_DELTAS];                   /* 0 = BPRED, ZERO_MV, MV, SPLIT */

  /* Distance of MB away from frame edges */
  int mb_to_left_edge;
  int mb_to_right_edge;
  int mb_to_top_edge;
  int mb_to_bottom_edge;

  unsigned int frames_since_golden;
  unsigned int frames_till_alt_ref_frame;

  /* Inverse transform function pointers. */
  void (*inv_xform4x4_1_x8)(int16_t *input, int16_t *output, int pitch);
  void (*inv_xform4x4_x8)(int16_t *input, int16_t *output, int pitch);
  void (*inv_walsh4x4_1)(int16_t *in, int16_t *out);
  void (*inv_walsh4x4_lossless)(int16_t *in, int16_t *out);


  vp9_subpix_fn_t  subpixel_predict4x4;
  vp9_subpix_fn_t  subpixel_predict8x4;
  vp9_subpix_fn_t  subpixel_predict8x8;
  vp9_subpix_fn_t  subpixel_predict16x16;
  vp9_subpix_fn_t  subpixel_predict_avg4x4;
  vp9_subpix_fn_t  subpixel_predict_avg8x4;
  vp9_subpix_fn_t  subpixel_predict_avg8x8;
  vp9_subpix_fn_t  subpixel_predict_avg16x16;
  int allow_high_precision_mv;

  int corrupted;

  int sb_index;
  int mb_index;   // Index of the MB in the SB (0..3)
  int q_index;

} MACROBLOCKD;

#define ACTIVE_HT 110                // quantization stepsize threshold

#define ACTIVE_HT8 300

#define ACTIVE_HT16 300

// convert MB_PREDICTION_MODE to B_PREDICTION_MODE
static B_PREDICTION_MODE pred_mode_conv(MB_PREDICTION_MODE mode) {
  B_PREDICTION_MODE b_mode;
  switch (mode) {
    case DC_PRED:
      b_mode = B_DC_PRED;
      break;
    case V_PRED:
      b_mode = B_VE_PRED;
      break;
    case H_PRED:
      b_mode = B_HE_PRED;
      break;
    case TM_PRED:
      b_mode = B_TM_PRED;
      break;
    case D45_PRED:
      b_mode = B_LD_PRED;
      break;
    case D135_PRED:
      b_mode = B_RD_PRED;
      break;
    case D117_PRED:
      b_mode = B_VR_PRED;
      break;
    case D153_PRED:
      b_mode = B_HD_PRED;
      break;
    case D27_PRED:
      b_mode = B_HU_PRED;
      break;
    case D63_PRED:
      b_mode = B_VL_PRED;
      break;
    default :
      // for debug purpose, to be removed after full testing
      assert(0);
      break;
  }
  return b_mode;
}

// transform mapping
static TX_TYPE txfm_map(B_PREDICTION_MODE bmode) {
  // map transform type
  TX_TYPE tx_type;
  switch (bmode) {
    case B_TM_PRED :
    case B_RD_PRED :
      tx_type = ADST_ADST;
      break;

    case B_VE_PRED :
    case B_VR_PRED :
      tx_type = ADST_DCT;
      break;

    case B_HE_PRED :
    case B_HD_PRED :
    case B_HU_PRED :
      tx_type = DCT_ADST;
      break;

#if CONFIG_NEWBINTRAMODES
    case B_CONTEXT_PRED:
      assert(0);
      break;
#endif

    default :
      tx_type = DCT_DCT;
      break;
  }
  return tx_type;
}

extern const uint8_t vp9_block2left[TX_SIZE_MAX_SB][25];
extern const uint8_t vp9_block2above[TX_SIZE_MAX_SB][25];

#define USE_ADST_FOR_I16X16_8X8   0
#define USE_ADST_FOR_I16X16_4X4   0
#define USE_ADST_FOR_I8X8_4X4     1
#define USE_ADST_PERIPHERY_ONLY   1

static TX_TYPE get_tx_type_4x4(const MACROBLOCKD *xd, const BLOCKD *b) {
  // TODO(debargha): explore different patterns for ADST usage when blocksize
  // is smaller than the prediction size
  TX_TYPE tx_type = DCT_DCT;
  int ib = (int)(b - xd->block);
  if (ib >= 16)
    return tx_type;
  // TODO(rbultje, debargha): Explore ADST usage for superblocks
  if (xd->mode_info_context->mbmi.sb_type)
    return tx_type;
  if (xd->mode_info_context->mbmi.mode == B_PRED &&
      xd->q_index < ACTIVE_HT) {
    tx_type = txfm_map(
#if CONFIG_NEWBINTRAMODES
        b->bmi.as_mode.first == B_CONTEXT_PRED ? b->bmi.as_mode.context :
#endif
        b->bmi.as_mode.first);
  } else if (xd->mode_info_context->mbmi.mode == I8X8_PRED &&
             xd->q_index < ACTIVE_HT) {
#if USE_ADST_FOR_I8X8_4X4
#if USE_ADST_PERIPHERY_ONLY
    // Use ADST for periphery blocks only
    int ic = (ib & 10);
    b += ic - ib;
    tx_type = (ic != 10) ?
         txfm_map(pred_mode_conv((MB_PREDICTION_MODE)b->bmi.as_mode.first)) :
         DCT_DCT;
#else
    // Use ADST
    tx_type = txfm_map(pred_mode_conv(
        (MB_PREDICTION_MODE)b->bmi.as_mode.first));
#endif
#else
    // Use 2D DCT
    tx_type = DCT_DCT;
#endif
  } else if (xd->mode_info_context->mbmi.mode < I8X8_PRED &&
             xd->q_index < ACTIVE_HT) {
#if USE_ADST_FOR_I16X16_4X4
#if USE_ADST_PERIPHERY_ONLY
    // Use ADST for periphery blocks only
    tx_type = (ib < 4 || ((ib & 3) == 0)) ?
        txfm_map(pred_mode_conv(xd->mode_info_context->mbmi.mode)) : DCT_DCT;
#else
    // Use ADST
    tx_type = txfm_map(pred_mode_conv(xd->mode_info_context->mbmi.mode));
#endif
#else
    // Use 2D DCT
    tx_type = DCT_DCT;
#endif
  }
  return tx_type;
}

static TX_TYPE get_tx_type_8x8(const MACROBLOCKD *xd, const BLOCKD *b) {
  // TODO(debargha): explore different patterns for ADST usage when blocksize
  // is smaller than the prediction size
  TX_TYPE tx_type = DCT_DCT;
  int ib = (int)(b - xd->block);
  if (ib >= 16)
    return tx_type;
  // TODO(rbultje, debargha): Explore ADST usage for superblocks
  if (xd->mode_info_context->mbmi.sb_type)
    return tx_type;
  if (xd->mode_info_context->mbmi.mode == I8X8_PRED &&
      xd->q_index < ACTIVE_HT8) {
    // TODO(rbultje): MB_PREDICTION_MODE / B_PREDICTION_MODE should be merged
    // or the relationship otherwise modified to address this type conversion.
    tx_type = txfm_map(pred_mode_conv(
           (MB_PREDICTION_MODE)b->bmi.as_mode.first));
  } else if (xd->mode_info_context->mbmi.mode < I8X8_PRED &&
             xd->q_index < ACTIVE_HT8) {
#if USE_ADST_FOR_I8X8_4X4
#if USE_ADST_PERIPHERY_ONLY
    // Use ADST for periphery blocks only
    tx_type = (ib != 10) ?
        txfm_map(pred_mode_conv(xd->mode_info_context->mbmi.mode)) : DCT_DCT;
#else
    // Use ADST
    tx_type = txfm_map(pred_mode_conv(xd->mode_info_context->mbmi.mode));
#endif
#else
    // Use 2D DCT
    tx_type = DCT_DCT;
#endif
  }
  return tx_type;
}

static TX_TYPE get_tx_type_16x16(const MACROBLOCKD *xd, const BLOCKD *b) {
  TX_TYPE tx_type = DCT_DCT;
  int ib = (int)(b - xd->block);
  if (ib >= 16)
    return tx_type;
  // TODO(rbultje, debargha): Explore ADST usage for superblocks
  if (xd->mode_info_context->mbmi.sb_type)
    return tx_type;
  if (xd->mode_info_context->mbmi.mode < I8X8_PRED &&
      xd->q_index < ACTIVE_HT16) {
    tx_type = txfm_map(pred_mode_conv(xd->mode_info_context->mbmi.mode));
  }
  return tx_type;
}

static TX_TYPE get_tx_type(const MACROBLOCKD *xd, const BLOCKD *b) {
  TX_TYPE tx_type = DCT_DCT;
  int ib = (int)(b - xd->block);
  if (ib >= 16)
    return tx_type;
  if (xd->mode_info_context->mbmi.txfm_size == TX_16X16) {
    tx_type = get_tx_type_16x16(xd, b);
  }
  if (xd->mode_info_context->mbmi.txfm_size  == TX_8X8) {
    ib = (ib & 8) + ((ib & 4) >> 1);
    tx_type = get_tx_type_8x8(xd, &xd->block[ib]);
  }
  if (xd->mode_info_context->mbmi.txfm_size  == TX_4X4) {
    tx_type = get_tx_type_4x4(xd, b);
  }
  return tx_type;
}

static int get_2nd_order_usage(const MACROBLOCKD *xd) {
  int has_2nd_order = (xd->mode_info_context->mbmi.mode != SPLITMV &&
                       xd->mode_info_context->mbmi.mode != I8X8_PRED &&
                       xd->mode_info_context->mbmi.mode != B_PRED &&
                       xd->mode_info_context->mbmi.txfm_size != TX_16X16);
  if (has_2nd_order)
    has_2nd_order = (get_tx_type(xd, xd->block) == DCT_DCT);
  return has_2nd_order;
}

extern void vp9_build_block_doffsets(MACROBLOCKD *xd);
extern void vp9_setup_block_dptrs(MACROBLOCKD *xd);

static void update_blockd_bmi(MACROBLOCKD *xd) {
  int i;
  int is_4x4;
  is_4x4 = (xd->mode_info_context->mbmi.mode == SPLITMV) ||
           (xd->mode_info_context->mbmi.mode == I8X8_PRED) ||
           (xd->mode_info_context->mbmi.mode == B_PRED);

  if (is_4x4) {
    for (i = 0; i < 16; i++) {
      xd->block[i].bmi = xd->mode_info_context->bmi[i];
    }
  }
}
#endif  // VP9_COMMON_VP9_BLOCKD_H_
