/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/common/vp9_blockd.h"
#include "vp9/decoder/vp9_onyxd_int.h"
#include "vpx_mem/vpx_mem.h"
#include "vpx_ports/mem.h"
#include "vp9/decoder/vp9_detokenize.h"
#include "vp9/common/vp9_seg_common.h"

#define EOB_CONTEXT_NODE            0
#define ZERO_CONTEXT_NODE           1
#define ONE_CONTEXT_NODE            2
#define LOW_VAL_CONTEXT_NODE        3
#define TWO_CONTEXT_NODE            4
#define THREE_CONTEXT_NODE          5
#define HIGH_LOW_CONTEXT_NODE       6
#define CAT_ONE_CONTEXT_NODE        7
#define CAT_THREEFOUR_CONTEXT_NODE  8
#define CAT_THREE_CONTEXT_NODE      9
#define CAT_FIVE_CONTEXT_NODE       10

#define CAT1_MIN_VAL    5
#define CAT2_MIN_VAL    7
#define CAT3_MIN_VAL   11
#define CAT4_MIN_VAL   19
#define CAT5_MIN_VAL   35
#define CAT6_MIN_VAL   67
#define CAT1_PROB0    159
#define CAT2_PROB0    145
#define CAT2_PROB1    165

#define CAT3_PROB0 140
#define CAT3_PROB1 148
#define CAT3_PROB2 173

#define CAT4_PROB0 135
#define CAT4_PROB1 140
#define CAT4_PROB2 155
#define CAT4_PROB3 176

#define CAT5_PROB0 130
#define CAT5_PROB1 134
#define CAT5_PROB2 141
#define CAT5_PROB3 157
#define CAT5_PROB4 180

static const vp9_prob cat6_prob[15] = {
  254, 254, 254, 252, 249, 243, 230, 196, 177, 153, 140, 133, 130, 129, 0
};

DECLARE_ALIGNED(16, extern const uint8_t, vp9_norm[256]);

static int get_signed(BOOL_DECODER *br, int value_to_sign) {
  return decode_bool(br, 128) ? -value_to_sign : value_to_sign;
}

#if CONFIG_NEWCOEFCONTEXT
#define PT pn
#define INCREMENT_COUNT(token)                       \
  do {                                               \
    coef_counts[type][coef_bands[c]][pn][token]++;   \
    pn = pt = vp9_prev_token_class[token];           \
    if (c < seg_eob - 1 && NEWCOEFCONTEXT_BAND_COND(coef_bands[c + 1]))  \
      pn = vp9_get_coef_neighbor_context(            \
          qcoeff_ptr, nodc, neighbors, scan[c + 1]); \
  } while (0)
#else
#define PT pt
#define INCREMENT_COUNT(token)               \
  do {                                       \
    coef_counts[type][coef_bands[c]][pt][token]++; \
    pt = vp9_prev_token_class[token];              \
  } while (0)
#endif  /* CONFIG_NEWCOEFCONTEXT */

#define WRITE_COEF_CONTINUE(val, token)                       \
  {                                                           \
    qcoeff_ptr[scan[c]] = (int16_t) get_signed(br, val);        \
    INCREMENT_COUNT(token);                                   \
    c++;                                                      \
    continue;                                                 \
  }

#define ADJUST_COEF(prob, bits_count)  \
  do {                                 \
    if (vp9_read(br, prob))            \
      val += (uint16_t)(1 << bits_count);\
  } while (0);

static int decode_coefs(VP9D_COMP *dx, const MACROBLOCKD *xd,
                        BOOL_DECODER* const br,
                        ENTROPY_CONTEXT *a, ENTROPY_CONTEXT *l,
                        PLANE_TYPE type,
                        TX_TYPE tx_type,
                        int seg_eob, int16_t *qcoeff_ptr,
                        const int *const scan, TX_SIZE txfm_size,
                        const int *coef_bands) {
  FRAME_CONTEXT *const fc = &dx->common.fc;
#if CONFIG_NEWCOEFCONTEXT
  const int *neighbors;
  int pn;
#endif
  int nodc = (type == PLANE_TYPE_Y_NO_DC);
  int pt, c = nodc;
  vp9_coeff_probs *coef_probs;
  vp9_prob *prob;
  vp9_coeff_count *coef_counts;

  switch (txfm_size) {
    default:
    case TX_4X4:
      if (tx_type == DCT_DCT) {
        coef_probs  = fc->coef_probs_4x4;
        coef_counts = fc->coef_counts_4x4;
      } else {
        coef_probs  = fc->hybrid_coef_probs_4x4;
        coef_counts = fc->hybrid_coef_counts_4x4;
      }
      break;
    case TX_8X8:
      if (tx_type == DCT_DCT) {
        coef_probs  = fc->coef_probs_8x8;
        coef_counts = fc->coef_counts_8x8;
      } else {
        coef_probs  = fc->hybrid_coef_probs_8x8;
        coef_counts = fc->hybrid_coef_counts_8x8;
      }
      break;
    case TX_16X16:
      if (tx_type == DCT_DCT) {
        coef_probs  = fc->coef_probs_16x16;
        coef_counts = fc->coef_counts_16x16;
      } else {
        coef_probs  = fc->hybrid_coef_probs_16x16;
        coef_counts = fc->hybrid_coef_counts_16x16;
      }
      break;
    case TX_32X32:
      coef_probs = fc->coef_probs_32x32;
      coef_counts = fc->coef_counts_32x32;
      break;
  }

  VP9_COMBINEENTROPYCONTEXTS(pt, *a, *l);
#if CONFIG_NEWCOEFCONTEXT
  pn = pt;
  neighbors = vp9_get_coef_neighbors_handle(scan);
#endif
  while (1) {
    int val;
    const uint8_t *cat6 = cat6_prob;
    if (c >= seg_eob) break;
    prob = coef_probs[type][coef_bands[c]][PT];
    if (!vp9_read(br, prob[EOB_CONTEXT_NODE]))
      break;
SKIP_START:
    if (c >= seg_eob) break;
    if (!vp9_read(br, prob[ZERO_CONTEXT_NODE])) {
      INCREMENT_COUNT(ZERO_TOKEN);
      ++c;
      prob = coef_probs[type][coef_bands[c]][PT];
      goto SKIP_START;
    }
    // ONE_CONTEXT_NODE_0_
    if (!vp9_read(br, prob[ONE_CONTEXT_NODE])) {
      WRITE_COEF_CONTINUE(1, ONE_TOKEN);
    }
    // LOW_VAL_CONTEXT_NODE_0_
    if (!vp9_read(br, prob[LOW_VAL_CONTEXT_NODE])) {
      if (!vp9_read(br, prob[TWO_CONTEXT_NODE])) {
        WRITE_COEF_CONTINUE(2, TWO_TOKEN);
      }
      if (!vp9_read(br, prob[THREE_CONTEXT_NODE])) {
        WRITE_COEF_CONTINUE(3, THREE_TOKEN);
      }
      WRITE_COEF_CONTINUE(4, FOUR_TOKEN);
    }
    // HIGH_LOW_CONTEXT_NODE_0_
    if (!vp9_read(br, prob[HIGH_LOW_CONTEXT_NODE])) {
      if (!vp9_read(br, prob[CAT_ONE_CONTEXT_NODE])) {
        val = CAT1_MIN_VAL;
        ADJUST_COEF(CAT1_PROB0, 0);
        WRITE_COEF_CONTINUE(val, DCT_VAL_CATEGORY1);
      }
      val = CAT2_MIN_VAL;
      ADJUST_COEF(CAT2_PROB1, 1);
      ADJUST_COEF(CAT2_PROB0, 0);
      WRITE_COEF_CONTINUE(val, DCT_VAL_CATEGORY2);
    }
    // CAT_THREEFOUR_CONTEXT_NODE_0_
    if (!vp9_read(br, prob[CAT_THREEFOUR_CONTEXT_NODE])) {
      if (!vp9_read(br, prob[CAT_THREE_CONTEXT_NODE])) {
        val = CAT3_MIN_VAL;
        ADJUST_COEF(CAT3_PROB2, 2);
        ADJUST_COEF(CAT3_PROB1, 1);
        ADJUST_COEF(CAT3_PROB0, 0);
        WRITE_COEF_CONTINUE(val, DCT_VAL_CATEGORY3);
      }
      val = CAT4_MIN_VAL;
      ADJUST_COEF(CAT4_PROB3, 3);
      ADJUST_COEF(CAT4_PROB2, 2);
      ADJUST_COEF(CAT4_PROB1, 1);
      ADJUST_COEF(CAT4_PROB0, 0);
      WRITE_COEF_CONTINUE(val, DCT_VAL_CATEGORY4);
    }
    // CAT_FIVE_CONTEXT_NODE_0_:
    if (!vp9_read(br, prob[CAT_FIVE_CONTEXT_NODE])) {
      val = CAT5_MIN_VAL;
      ADJUST_COEF(CAT5_PROB4, 4);
      ADJUST_COEF(CAT5_PROB3, 3);
      ADJUST_COEF(CAT5_PROB2, 2);
      ADJUST_COEF(CAT5_PROB1, 1);
      ADJUST_COEF(CAT5_PROB0, 0);
      WRITE_COEF_CONTINUE(val, DCT_VAL_CATEGORY5);
    }
    val = 0;
    while (*cat6) {
      val = (val << 1) | vp9_read(br, *cat6++);
    }
    val += CAT6_MIN_VAL;
    WRITE_COEF_CONTINUE(val, DCT_VAL_CATEGORY6);
  }

  if (c < seg_eob)
    coef_counts[type][coef_bands[c]][PT][DCT_EOB_TOKEN]++;

  a[0] = l[0] = (c > !type);

  return c;
}

static int get_eob(MACROBLOCKD* const xd, int segment_id, int eob_max) {
  int active = vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB);
  int eob = vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);

  if (!active || eob > eob_max)
    eob = eob_max;
  return eob;
}

int vp9_decode_sb_tokens(VP9D_COMP* const pbi,
                         MACROBLOCKD* const xd,
                         BOOL_DECODER* const bc) {
  ENTROPY_CONTEXT* const A = (ENTROPY_CONTEXT *)xd->above_context;
  ENTROPY_CONTEXT* const L = (ENTROPY_CONTEXT *)xd->left_context;
  ENTROPY_CONTEXT* const A1 = (ENTROPY_CONTEXT *)(&xd->above_context[1]);
  ENTROPY_CONTEXT* const L1 = (ENTROPY_CONTEXT *)(&xd->left_context[1]);
  uint16_t *const eobs = xd->eobs;
  const int segment_id = xd->mode_info_context->mbmi.segment_id;
  int c, i, eobtotal = 0, seg_eob;

  // Luma block
#if CONFIG_CNVCONTEXT
  ENTROPY_CONTEXT above_ec = (A[0] + A[1] + A[2] + A[3] +
                              A1[0] + A1[1] + A1[2] + A1[3]) != 0;
  ENTROPY_CONTEXT left_ec =  (L[0] + L[1] + L[2] + L[3] +
                              L1[0] + L1[1] + L1[2] + L1[3]) != 0;
#else
  ENTROPY_CONTEXT above_ec = A[0];
  ENTROPY_CONTEXT left_ec =  L[0];
#endif
  eobs[0] = c = decode_coefs(pbi, xd, bc, &above_ec, &left_ec,
                             PLANE_TYPE_Y_WITH_DC,
                             DCT_DCT, get_eob(xd, segment_id, 1024),
                             xd->sb_coeff_data.qcoeff,
                             vp9_default_zig_zag1d_32x32,
                             TX_32X32, vp9_coef_bands_32x32);
  A[1] = A[2] = A[3] = A[0] = above_ec;
  L[1] = L[2] = L[3] = L[0] = left_ec;
  A1[1] = A1[2] = A1[3] = A1[0] = above_ec;
  L1[1] = L1[2] = L1[3] = L1[0] = left_ec;

  eobtotal += c;

  // 16x16 chroma blocks
  seg_eob = get_eob(xd, segment_id, 256);

  for (i = 16; i < 24; i += 4) {
    ENTROPY_CONTEXT* const a = A + vp9_block2above[TX_16X16][i];
    ENTROPY_CONTEXT* const l = L + vp9_block2left[TX_16X16][i];
    ENTROPY_CONTEXT* const a1 = A1 + vp9_block2above[TX_16X16][i];
    ENTROPY_CONTEXT* const l1 = L1 + vp9_block2left[TX_16X16][i];
#if CONFIG_CNVCONTEXT
    above_ec = (a[0] + a[1] + a1[0] + a1[1]) != 0;
    left_ec = (l[0] + l[1] + l1[0] + l1[1]) != 0;
#else
    above_ec = a[0];
    left_ec = l[0];
#endif

    eobs[i] = c = decode_coefs(pbi, xd, bc,
                               &above_ec, &left_ec,
                               PLANE_TYPE_UV,
                               DCT_DCT, seg_eob,
                               xd->sb_coeff_data.qcoeff + 1024 + (i - 16) * 64,
                               vp9_default_zig_zag1d_16x16,
                               TX_16X16, vp9_coef_bands_16x16);

    a1[1] = a1[0] = a[1] = a[0] = above_ec;
    l1[1] = l1[0] = l[1] = l[0] = left_ec;
    eobtotal += c;
  }
  // no Y2 block
  A[8] = L[8] = A1[8] = L1[8] = 0;
  return eobtotal;
}

static int vp9_decode_mb_tokens_16x16(VP9D_COMP* const pbi,
                                      MACROBLOCKD* const xd,
                                      BOOL_DECODER* const bc) {
  ENTROPY_CONTEXT* const A = (ENTROPY_CONTEXT *)xd->above_context;
  ENTROPY_CONTEXT* const L = (ENTROPY_CONTEXT *)xd->left_context;
  uint16_t *const eobs = xd->eobs;
  const int segment_id = xd->mode_info_context->mbmi.segment_id;
  int c, i, eobtotal = 0, seg_eob;
  // Luma block

#if CONFIG_CNVCONTEXT
  ENTROPY_CONTEXT above_ec = (A[0] + A[1] + A[2] + A[3]) != 0;
  ENTROPY_CONTEXT left_ec = (L[0] + L[1] + L[2] + L[3]) != 0;
#else
  ENTROPY_CONTEXT above_ec = A[0];
  ENTROPY_CONTEXT left_ec = L[0];
#endif
  eobs[0] = c = decode_coefs(pbi, xd, bc, &above_ec, &left_ec,
                             PLANE_TYPE_Y_WITH_DC,
                             get_tx_type(xd, &xd->block[0]),
                             get_eob(xd, segment_id, 256),
                             xd->qcoeff, vp9_default_zig_zag1d_16x16,
                             TX_16X16, vp9_coef_bands_16x16);
  A[1] = A[2] = A[3] = A[0] = above_ec;
  L[1] = L[2] = L[3] = L[0] = left_ec;
  eobtotal += c;

  // 8x8 chroma blocks
  seg_eob = get_eob(xd, segment_id, 64);
  for (i = 16; i < 24; i += 4) {
    ENTROPY_CONTEXT* const a = A + vp9_block2above[TX_8X8][i];
    ENTROPY_CONTEXT* const l = L + vp9_block2left[TX_8X8][i];
#if CONFIG_CNVCONTEXT
    above_ec = (a[0] + a[1]) != 0;
    left_ec = (l[0] + l[1]) != 0;
#else
    above_ec = a[0];
    left_ec = l[0];
#endif
    eobs[i] = c = decode_coefs(pbi, xd, bc,
                               &above_ec, &left_ec,
                               PLANE_TYPE_UV,
                               DCT_DCT, seg_eob, xd->block[i].qcoeff,
                               vp9_default_zig_zag1d_8x8,
                               TX_8X8, vp9_coef_bands_8x8);
    a[1] = a[0] = above_ec;
    l[1] = l[0] = left_ec;
    eobtotal += c;
  }
  A[8] = 0;
  L[8] = 0;
  return eobtotal;
}

static int vp9_decode_mb_tokens_8x8(VP9D_COMP* const pbi,
                                    MACROBLOCKD* const xd,
                                    BOOL_DECODER* const bc) {
  ENTROPY_CONTEXT *const A = (ENTROPY_CONTEXT *)xd->above_context;
  ENTROPY_CONTEXT *const L = (ENTROPY_CONTEXT *)xd->left_context;
  uint16_t *const eobs = xd->eobs;
  PLANE_TYPE type;
  int c, i, eobtotal = 0, seg_eob;
  const int segment_id = xd->mode_info_context->mbmi.segment_id;

  int has_2nd_order = get_2nd_order_usage(xd);
  // 2nd order DC block
  if (has_2nd_order) {
    ENTROPY_CONTEXT *const a = A + vp9_block2above[TX_8X8][24];
    ENTROPY_CONTEXT *const l = L + vp9_block2left[TX_8X8][24];

    eobs[24] = c = decode_coefs(pbi, xd, bc, a, l, PLANE_TYPE_Y2,
                                DCT_DCT, get_eob(xd, segment_id, 4),
                                xd->block[24].qcoeff,
                                vp9_default_zig_zag1d_4x4, TX_8X8,
                                vp9_coef_bands_4x4);
    eobtotal += c - 4;
    type = PLANE_TYPE_Y_NO_DC;
  } else {
    xd->above_context->y2 = 0;
    xd->left_context->y2 = 0;
    eobs[24] = 0;
    type = PLANE_TYPE_Y_WITH_DC;
  }

  // luma blocks
  seg_eob = get_eob(xd, segment_id, 64);
  for (i = 0; i < 16; i += 4) {
    ENTROPY_CONTEXT *const a = A + vp9_block2above[TX_8X8][i];
    ENTROPY_CONTEXT *const l = L + vp9_block2left[TX_8X8][i];
#if CONFIG_CNVCONTEXT
    ENTROPY_CONTEXT above_ec = (a[0] + a[1]) != 0;
    ENTROPY_CONTEXT left_ec = (l[0] + l[1]) != 0;
#else
    ENTROPY_CONTEXT above_ec = a[0];
    ENTROPY_CONTEXT left_ec = l[0];
#endif
    eobs[i] = c = decode_coefs(pbi, xd, bc, &above_ec, &left_ec, type,
                               type == PLANE_TYPE_Y_WITH_DC ?
                               get_tx_type(xd, xd->block + i) : DCT_DCT,
                               seg_eob, xd->block[i].qcoeff,
                               vp9_default_zig_zag1d_8x8,
                               TX_8X8, vp9_coef_bands_8x8);
    a[1] = a[0] = above_ec;
    l[1] = l[0] = left_ec;
    eobtotal += c;
  }

  // chroma blocks
  if (xd->mode_info_context->mbmi.mode == I8X8_PRED ||
      xd->mode_info_context->mbmi.mode == SPLITMV) {
    // use 4x4 transform for U, V components in I8X8/splitmv prediction mode
    seg_eob = get_eob(xd, segment_id, 16);
    for (i = 16; i < 24; i++) {
      ENTROPY_CONTEXT *const a = A + vp9_block2above[TX_4X4][i];
      ENTROPY_CONTEXT *const l = L + vp9_block2left[TX_4X4][i];

      eobs[i] = c = decode_coefs(pbi, xd, bc, a, l, PLANE_TYPE_UV,
                                 DCT_DCT, seg_eob, xd->block[i].qcoeff,
                                 vp9_default_zig_zag1d_4x4, TX_4X4,
                                 vp9_coef_bands_4x4);
      eobtotal += c;
    }
  } else {
    for (i = 16; i < 24; i += 4) {
      ENTROPY_CONTEXT *const a = A + vp9_block2above[TX_8X8][i];
      ENTROPY_CONTEXT *const l = L + vp9_block2left[TX_8X8][i];
#if CONFIG_CNVCONTEXT
      ENTROPY_CONTEXT above_ec = (a[0] + a[1]) != 0;
      ENTROPY_CONTEXT left_ec = (l[0] + l[1]) != 0;
#else
      ENTROPY_CONTEXT above_ec = a[0];
      ENTROPY_CONTEXT left_ec = l[0];
#endif
      eobs[i] = c = decode_coefs(pbi, xd, bc,
                                 &above_ec, &left_ec,
                                 PLANE_TYPE_UV,
                                 DCT_DCT, seg_eob, xd->block[i].qcoeff,
                                 vp9_default_zig_zag1d_8x8,
                                 TX_8X8, vp9_coef_bands_8x8);
      a[1] = a[0] = above_ec;
      l[1] = l[0] = left_ec;
      eobtotal += c;
    }
  }

  return eobtotal;
}

static int decode_coefs_4x4(VP9D_COMP *dx, MACROBLOCKD *xd,
                            BOOL_DECODER* const bc,
                            PLANE_TYPE type, int i, int seg_eob,
                            TX_TYPE tx_type, const int *scan) {
  ENTROPY_CONTEXT *const A = (ENTROPY_CONTEXT *)xd->above_context;
  ENTROPY_CONTEXT *const L = (ENTROPY_CONTEXT *)xd->left_context;
  ENTROPY_CONTEXT *const a = A + vp9_block2above[TX_4X4][i];
  ENTROPY_CONTEXT *const l = L + vp9_block2left[TX_4X4][i];
  uint16_t *const eobs = xd->eobs;
  int c;

  c = decode_coefs(dx, xd, bc, a, l, type, tx_type, seg_eob,
                   xd->block[i].qcoeff, scan, TX_4X4, vp9_coef_bands_4x4);
  eobs[i] = c;

  return c;
}

static int decode_coefs_4x4_y(VP9D_COMP *dx, MACROBLOCKD *xd,
                              BOOL_DECODER* const bc,
                              PLANE_TYPE type, int i, int seg_eob) {
  const TX_TYPE tx_type = (type == PLANE_TYPE_Y_WITH_DC) ?
                          get_tx_type(xd, &xd->block[i]) : DCT_DCT;
  const int *scan;

  switch (tx_type) {
    case ADST_DCT:
      scan = vp9_row_scan_4x4;
      break;
    case DCT_ADST:
      scan = vp9_col_scan_4x4;
      break;
    default:
      scan = vp9_default_zig_zag1d_4x4;
      break;
  }

  return decode_coefs_4x4(dx, xd, bc, type, i, seg_eob, tx_type, scan);
}

int vp9_decode_coefs_4x4(VP9D_COMP *dx, MACROBLOCKD *xd,
                         BOOL_DECODER* const bc,
                         PLANE_TYPE type, int i) {
  const int segment_id = xd->mode_info_context->mbmi.segment_id;
  const int seg_eob = get_eob(xd, segment_id, 16);

  return decode_coefs_4x4_y(dx, xd, bc, type, i, seg_eob);
}

static int decode_mb_tokens_4x4_uv(VP9D_COMP* const dx,
                                   MACROBLOCKD* const xd,
                                   BOOL_DECODER* const bc,
                                   int seg_eob) {
  int eobtotal = 0, i;

  // chroma blocks
  for (i = 16; i < 24; i++) {
    eobtotal += decode_coefs_4x4(dx, xd, bc, PLANE_TYPE_UV, i, seg_eob,
                                 DCT_DCT, vp9_default_zig_zag1d_4x4);
  }

  return eobtotal;
}

int vp9_decode_mb_tokens_4x4_uv(VP9D_COMP* const dx,
                                MACROBLOCKD* const xd,
                                BOOL_DECODER* const bc) {
  const int segment_id = xd->mode_info_context->mbmi.segment_id;
  const int seg_eob = get_eob(xd, segment_id, 16);

  return decode_mb_tokens_4x4_uv(dx, xd, bc, seg_eob);
}

static int vp9_decode_mb_tokens_4x4(VP9D_COMP* const dx,
                                    MACROBLOCKD* const xd,
                                    BOOL_DECODER* const bc) {
  int i, eobtotal = 0;
  PLANE_TYPE type;
  const int segment_id = xd->mode_info_context->mbmi.segment_id;
  const int seg_eob = get_eob(xd, segment_id, 16);
  const int has_2nd_order = get_2nd_order_usage(xd);

  // 2nd order DC block
  if (has_2nd_order) {
    eobtotal += decode_coefs_4x4(dx, xd, bc, PLANE_TYPE_Y2, 24, seg_eob,
                                 DCT_DCT, vp9_default_zig_zag1d_4x4) - 16;
    type = PLANE_TYPE_Y_NO_DC;
  } else {
    xd->above_context->y2 = 0;
    xd->left_context->y2 = 0;
    xd->eobs[24] = 0;
    type = PLANE_TYPE_Y_WITH_DC;
  }

  // luma blocks
  for (i = 0; i < 16; ++i) {
    eobtotal += decode_coefs_4x4_y(dx, xd, bc, type, i, seg_eob);
  }

  // chroma blocks
  eobtotal += decode_mb_tokens_4x4_uv(dx, xd, bc, seg_eob);

  return eobtotal;
}

int vp9_decode_mb_tokens(VP9D_COMP* const dx,
                         MACROBLOCKD* const xd,
                         BOOL_DECODER* const bc) {
  const TX_SIZE tx_size = xd->mode_info_context->mbmi.txfm_size;
  int eobtotal;

  if (tx_size == TX_16X16) {
    eobtotal = vp9_decode_mb_tokens_16x16(dx, xd, bc);
  } else if (tx_size == TX_8X8) {
    eobtotal = vp9_decode_mb_tokens_8x8(dx, xd, bc);
  } else {
    assert(tx_size == TX_4X4);
    eobtotal = vp9_decode_mb_tokens_4x4(dx, xd, bc);
  }

  return eobtotal;
}
