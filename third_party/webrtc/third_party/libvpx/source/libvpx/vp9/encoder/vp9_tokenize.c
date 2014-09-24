/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "vp9/encoder/vp9_onyx_int.h"
#include "vp9/encoder/vp9_tokenize.h"
#include "vpx_mem/vpx_mem.h"

#include "vp9/common/vp9_pred_common.h"
#include "vp9/common/vp9_seg_common.h"
#include "vp9/common/vp9_entropy.h"

/* Global event counters used for accumulating statistics across several
   compressions, then generating vp9_context.c = initial stats. */

#ifdef ENTROPY_STATS
vp9_coeff_accum context_counters_4x4[BLOCK_TYPES_4X4];
vp9_coeff_accum hybrid_context_counters_4x4[BLOCK_TYPES_4X4];
vp9_coeff_accum context_counters_8x8[BLOCK_TYPES_8X8];
vp9_coeff_accum hybrid_context_counters_8x8[BLOCK_TYPES_8X8];
vp9_coeff_accum context_counters_16x16[BLOCK_TYPES_16X16];
vp9_coeff_accum hybrid_context_counters_16x16[BLOCK_TYPES_16X16];
vp9_coeff_accum context_counters_32x32[BLOCK_TYPES_32X32];

extern vp9_coeff_stats tree_update_hist_4x4[BLOCK_TYPES_4X4];
extern vp9_coeff_stats hybrid_tree_update_hist_4x4[BLOCK_TYPES_4X4];
extern vp9_coeff_stats tree_update_hist_8x8[BLOCK_TYPES_8X8];
extern vp9_coeff_stats hybrid_tree_update_hist_8x8[BLOCK_TYPES_8X8];
extern vp9_coeff_stats tree_update_hist_16x16[BLOCK_TYPES_16X16];
extern vp9_coeff_stats hybrid_tree_update_hist_16x16[BLOCK_TYPES_16X16];
extern vp9_coeff_stats tree_update_hist_32x32[BLOCK_TYPES_32X32];
#endif  /* ENTROPY_STATS */

static TOKENVALUE dct_value_tokens[DCT_MAX_VALUE * 2];
const TOKENVALUE *vp9_dct_value_tokens_ptr;
static int dct_value_cost[DCT_MAX_VALUE * 2];
const int *vp9_dct_value_cost_ptr;

static void fill_value_tokens() {

  TOKENVALUE *const t = dct_value_tokens + DCT_MAX_VALUE;
  vp9_extra_bit_struct *const e = vp9_extra_bits;

  int i = -DCT_MAX_VALUE;
  int sign = 1;

  do {
    if (!i)
      sign = 0;

    {
      const int a = sign ? -i : i;
      int eb = sign;

      if (a > 4) {
        int j = 4;

        while (++j < 11  &&  e[j].base_val <= a) {}

        t[i].Token = --j;
        eb |= (a - e[j].base_val) << 1;
      } else
        t[i].Token = a;

      t[i].Extra = eb;
    }

    // initialize the cost for extra bits for all possible coefficient value.
    {
      int cost = 0;
      vp9_extra_bit_struct *p = vp9_extra_bits + t[i].Token;

      if (p->base_val) {
        const int extra = t[i].Extra;
        const int Length = p->Len;

        if (Length)
          cost += treed_cost(p->tree, p->prob, extra >> 1, Length);

        cost += vp9_cost_bit(vp9_prob_half, extra & 1); /* sign */
        dct_value_cost[i + DCT_MAX_VALUE] = cost;
      }

    }

  } while (++i < DCT_MAX_VALUE);

  vp9_dct_value_tokens_ptr = dct_value_tokens + DCT_MAX_VALUE;
  vp9_dct_value_cost_ptr   = dct_value_cost + DCT_MAX_VALUE;
}

#if CONFIG_NEWCOEFCONTEXT
#define PT pn
#else
#define PT pt
#endif

static void tokenize_b(VP9_COMP *cpi,
                       MACROBLOCKD *xd,
                       const int ib,
                       TOKENEXTRA **tp,
                       PLANE_TYPE type,
                       TX_SIZE tx_size,
                       int dry_run) {
  int pt; /* near block/prev token context index */
  int c = (type == PLANE_TYPE_Y_NO_DC) ? 1 : 0;
  const BLOCKD * const b = xd->block + ib;
  const int eob = b->eob;     /* one beyond last nonzero coeff */
  TOKENEXTRA *t = *tp;        /* store tokens starting here */
  int16_t *qcoeff_ptr = b->qcoeff;
  int seg_eob;
  const int segment_id = xd->mode_info_context->mbmi.segment_id;
  const int *bands, *scan;
  vp9_coeff_count *counts;
  vp9_coeff_probs *probs;
  const TX_TYPE tx_type = (type == PLANE_TYPE_Y_WITH_DC) ?
                          get_tx_type(xd, b) : DCT_DCT;
#if CONFIG_NEWCOEFCONTEXT
  const int *neighbors;
  int pn;
#endif

  ENTROPY_CONTEXT *const a = (ENTROPY_CONTEXT *)xd->above_context +
      vp9_block2above[tx_size][ib];
  ENTROPY_CONTEXT *const l = (ENTROPY_CONTEXT *)xd->left_context +
      vp9_block2left[tx_size][ib];
  ENTROPY_CONTEXT a_ec = *a, l_ec = *l;

  ENTROPY_CONTEXT *const a1 = (ENTROPY_CONTEXT *)(&xd->above_context[1]) +
      vp9_block2above[tx_size][ib];
  ENTROPY_CONTEXT *const l1 = (ENTROPY_CONTEXT *)(&xd->left_context[1]) +
      vp9_block2left[tx_size][ib];


  switch (tx_size) {
    default:
    case TX_4X4:
      seg_eob = 16;
      bands = vp9_coef_bands_4x4;
      scan = vp9_default_zig_zag1d_4x4;
      if (tx_type != DCT_DCT) {
        counts = cpi->hybrid_coef_counts_4x4;
        probs = cpi->common.fc.hybrid_coef_probs_4x4;
        if (tx_type == ADST_DCT) {
          scan = vp9_row_scan_4x4;
        } else if (tx_type == DCT_ADST) {
          scan = vp9_col_scan_4x4;
        }
      } else {
        counts = cpi->coef_counts_4x4;
        probs = cpi->common.fc.coef_probs_4x4;
      }
      break;
    case TX_8X8:
      if (type == PLANE_TYPE_Y2) {
        seg_eob = 4;
        bands = vp9_coef_bands_4x4;
        scan = vp9_default_zig_zag1d_4x4;
      } else {
#if CONFIG_CNVCONTEXT
        a_ec = (a[0] + a[1]) != 0;
        l_ec = (l[0] + l[1]) != 0;
#endif
        seg_eob = 64;
        bands = vp9_coef_bands_8x8;
        scan = vp9_default_zig_zag1d_8x8;
      }
      if (tx_type != DCT_DCT) {
        counts = cpi->hybrid_coef_counts_8x8;
        probs = cpi->common.fc.hybrid_coef_probs_8x8;
      } else {
        counts = cpi->coef_counts_8x8;
        probs = cpi->common.fc.coef_probs_8x8;
      }
      break;
    case TX_16X16:
#if CONFIG_CNVCONTEXT
      if (type != PLANE_TYPE_UV) {
        a_ec = (a[0] + a[1] + a[2] + a[3]) != 0;
        l_ec = (l[0] + l[1] + l[2] + l[3]) != 0;
      } else {
        a_ec = (a[0] + a[1] + a1[0] + a1[1]) != 0;
        l_ec = (l[0] + l[1] + l1[0] + l1[1]) != 0;
      }
#endif
      seg_eob = 256;
      bands = vp9_coef_bands_16x16;
      scan = vp9_default_zig_zag1d_16x16;
      if (tx_type != DCT_DCT) {
        counts = cpi->hybrid_coef_counts_16x16;
        probs = cpi->common.fc.hybrid_coef_probs_16x16;
      } else {
        counts = cpi->coef_counts_16x16;
        probs = cpi->common.fc.coef_probs_16x16;
      }
      if (type == PLANE_TYPE_UV) {
        int uv_idx = (ib - 16) >> 2;
        qcoeff_ptr = xd->sb_coeff_data.qcoeff + 1024 + 256 * uv_idx;
      }
      break;
    case TX_32X32:
#if CONFIG_CNVCONTEXT
      a_ec = a[0] + a[1] + a[2] + a[3] +
             a1[0] + a1[1] + a1[2] + a1[3];
      l_ec = l[0] + l[1] + l[2] + l[3] +
             l1[0] + l1[1] + l1[2] + l1[3];
      a_ec = a_ec != 0;
      l_ec = l_ec != 0;
#endif
      seg_eob = 1024;
      bands = vp9_coef_bands_32x32;
      scan = vp9_default_zig_zag1d_32x32;
      counts = cpi->coef_counts_32x32;
      probs = cpi->common.fc.coef_probs_32x32;
      qcoeff_ptr = xd->sb_coeff_data.qcoeff;
      break;
  }

  VP9_COMBINEENTROPYCONTEXTS(pt, a_ec, l_ec);
#if CONFIG_NEWCOEFCONTEXT
  neighbors = vp9_get_coef_neighbors_handle(scan);
  pn = pt;
#endif

  if (vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB))
    seg_eob = vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);

  do {
    const int band = bands[c];
    int token;

    if (c < eob) {
      const int rc = scan[c];
      const int v = qcoeff_ptr[rc];
      assert(-DCT_MAX_VALUE <= v  &&  v < DCT_MAX_VALUE);

      t->Extra = vp9_dct_value_tokens_ptr[v].Extra;
      token    = vp9_dct_value_tokens_ptr[v].Token;
    } else {
      token = DCT_EOB_TOKEN;
    }

    t->Token = token;
    t->context_tree = probs[type][band][PT];
    t->skip_eob_node = (pt == 0) && ((band > 0 && type != PLANE_TYPE_Y_NO_DC) ||
                                     (band > 1 && type == PLANE_TYPE_Y_NO_DC));
    assert(vp9_coef_encodings[t->Token].Len - t->skip_eob_node > 0);
    if (!dry_run) {
      ++counts[type][band][PT][token];
    }
    pt = vp9_prev_token_class[token];
#if CONFIG_NEWCOEFCONTEXT
    if (c < seg_eob - 1 && NEWCOEFCONTEXT_BAND_COND(bands[c + 1]))
      pn = vp9_get_coef_neighbor_context(
          qcoeff_ptr, (type == PLANE_TYPE_Y_NO_DC), neighbors, scan[c + 1]);
    else
      pn = pt;
#endif
    ++t;
  } while (c < eob && ++c < seg_eob);

  *tp = t;
  a_ec = l_ec = (c > !type); /* 0 <-> all coeff data is zero */
  a[0] = a_ec;
  l[0] = l_ec;

  if (tx_size == TX_8X8 && type != PLANE_TYPE_Y2) {
    a[1] = a_ec;
    l[1] = l_ec;
  } else if (tx_size == TX_16X16) {
    if (type != PLANE_TYPE_UV) {
      a[1] = a[2] = a[3] = a_ec;
      l[1] = l[2] = l[3] = l_ec;
    } else {
      a1[0] = a1[1] = a[1] = a_ec;
      l1[0] = l1[1] = l[1] = l_ec;
    }
  } else if (tx_size == TX_32X32) {
    a[1] = a[2] = a[3] = a_ec;
    l[1] = l[2] = l[3] = l_ec;
    a1[0] = a1[1] = a1[2] = a1[3] = a_ec;
    l1[0] = l1[1] = l1[2] = l1[3] = l_ec;
  }
}

int vp9_mby_is_skippable_4x4(MACROBLOCKD *xd, int has_2nd_order) {
  int skip = 1;
  int i = 0;

  if (has_2nd_order) {
    for (i = 0; i < 16; i++)
      skip &= (xd->block[i].eob < 2);
    skip &= (!xd->block[24].eob);
  } else {
    for (i = 0; i < 16; i++)
      skip &= (!xd->block[i].eob);
  }
  return skip;
}

int vp9_mbuv_is_skippable_4x4(MACROBLOCKD *xd) {
  int skip = 1;
  int i;

  for (i = 16; i < 24; i++)
    skip &= (!xd->block[i].eob);
  return skip;
}

static int mb_is_skippable_4x4(MACROBLOCKD *xd, int has_2nd_order) {
  return (vp9_mby_is_skippable_4x4(xd, has_2nd_order) &
          vp9_mbuv_is_skippable_4x4(xd));
}

int vp9_mby_is_skippable_8x8(MACROBLOCKD *xd, int has_2nd_order) {
  int skip = 1;
  int i = 0;

  if (has_2nd_order) {
    for (i = 0; i < 16; i += 4)
      skip &= (xd->block[i].eob < 2);
    skip &= (!xd->block[24].eob);
  } else {
    for (i = 0; i < 16; i += 4)
      skip &= (!xd->block[i].eob);
  }
  return skip;
}

int vp9_mbuv_is_skippable_8x8(MACROBLOCKD *xd) {
  return (!xd->block[16].eob) & (!xd->block[20].eob);
}

static int mb_is_skippable_8x8(MACROBLOCKD *xd, int has_2nd_order) {
  return (vp9_mby_is_skippable_8x8(xd, has_2nd_order) &
          vp9_mbuv_is_skippable_8x8(xd));
}

static int mb_is_skippable_8x8_4x4uv(MACROBLOCKD *xd, int has_2nd_order) {
  return (vp9_mby_is_skippable_8x8(xd, has_2nd_order) &
          vp9_mbuv_is_skippable_4x4(xd));
}

int vp9_mby_is_skippable_16x16(MACROBLOCKD *xd) {
  int skip = 1;
  skip &= !xd->block[0].eob;
  return skip;
}

static int mb_is_skippable_16x16(MACROBLOCKD *xd) {
  return (vp9_mby_is_skippable_16x16(xd) & vp9_mbuv_is_skippable_8x8(xd));
}

int vp9_sby_is_skippable_32x32(MACROBLOCKD *xd) {
  int skip = 1;
  skip &= !xd->block[0].eob;
  return skip;
}

int vp9_sbuv_is_skippable_16x16(MACROBLOCKD *xd) {
  return (!xd->block[16].eob) & (!xd->block[20].eob);
}

static int sb_is_skippable_32x32(MACROBLOCKD *xd) {
  return vp9_sby_is_skippable_32x32(xd) &&
         vp9_sbuv_is_skippable_16x16(xd);
}

void vp9_tokenize_sb(VP9_COMP *cpi,
                     MACROBLOCKD *xd,
                     TOKENEXTRA **t,
                     int dry_run) {
  VP9_COMMON * const cm = &cpi->common;
  MB_MODE_INFO * const mbmi = &xd->mode_info_context->mbmi;
  TOKENEXTRA *t_backup = *t;
  ENTROPY_CONTEXT *A[2] = { (ENTROPY_CONTEXT *) (xd->above_context + 0),
                            (ENTROPY_CONTEXT *) (xd->above_context + 1), };
  ENTROPY_CONTEXT *L[2] = { (ENTROPY_CONTEXT *) (xd->left_context + 0),
                            (ENTROPY_CONTEXT *) (xd->left_context + 1), };
  const int mb_skip_context = vp9_get_pred_context(cm, xd, PRED_MBSKIP);
  const int segment_id = mbmi->segment_id;
  const int skip_inc =  !vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB) ||
                        (vp9_get_segdata(xd, segment_id, SEG_LVL_EOB) != 0);
  int b;

  mbmi->mb_skip_coeff = sb_is_skippable_32x32(xd);

  if (mbmi->mb_skip_coeff) {
    if (!dry_run)
      cpi->skip_true_count[mb_skip_context] += skip_inc;
    if (!cm->mb_no_coeff_skip) {
      vp9_stuff_sb(cpi, xd, t, dry_run);
    } else {
      vp9_fix_contexts_sb(xd);
    }
    if (dry_run)
      *t = t_backup;
    return;
  }

  if (!dry_run)
    cpi->skip_false_count[mb_skip_context] += skip_inc;

  tokenize_b(cpi, xd, 0, t, PLANE_TYPE_Y_WITH_DC,
             TX_32X32, dry_run);

  for (b = 16; b < 24; b += 4) {
    tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV,
               TX_16X16, dry_run);
  }
  A[0][8] = L[0][8] = A[1][8] = L[1][8] = 0;
  if (dry_run)
    *t = t_backup;
}

void vp9_tokenize_mb(VP9_COMP *cpi,
                     MACROBLOCKD *xd,
                     TOKENEXTRA **t,
                     int dry_run) {
  PLANE_TYPE plane_type;
  int has_2nd_order;
  int b;
  int tx_size = xd->mode_info_context->mbmi.txfm_size;
  int mb_skip_context = vp9_get_pred_context(&cpi->common, xd, PRED_MBSKIP);
  TOKENEXTRA *t_backup = *t;

  // If the MB is going to be skipped because of a segment level flag
  // exclude this from the skip count stats used to calculate the
  // transmitted skip probability;
  int skip_inc;
  int segment_id = xd->mode_info_context->mbmi.segment_id;

  if (!vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB) ||
      (vp9_get_segdata(xd, segment_id, SEG_LVL_EOB) != 0)) {
    skip_inc = 1;
  } else
    skip_inc = 0;

  has_2nd_order = get_2nd_order_usage(xd);

  switch (tx_size) {
    case TX_16X16:

      xd->mode_info_context->mbmi.mb_skip_coeff = mb_is_skippable_16x16(xd);
      break;
    case TX_8X8:
      if (xd->mode_info_context->mbmi.mode == I8X8_PRED ||
          xd->mode_info_context->mbmi.mode == SPLITMV)
        xd->mode_info_context->mbmi.mb_skip_coeff =
            mb_is_skippable_8x8_4x4uv(xd, 0);
      else
        xd->mode_info_context->mbmi.mb_skip_coeff =
            mb_is_skippable_8x8(xd, has_2nd_order);
      break;

    default:
      xd->mode_info_context->mbmi.mb_skip_coeff =
          mb_is_skippable_4x4(xd, has_2nd_order);
      break;
  }

  if (xd->mode_info_context->mbmi.mb_skip_coeff) {
    if (!dry_run)
      cpi->skip_true_count[mb_skip_context] += skip_inc;
    if (!cpi->common.mb_no_coeff_skip) {
      vp9_stuff_mb(cpi, xd, t, dry_run);
    } else {
      vp9_reset_mb_tokens_context(xd);
    }

    if (dry_run)
      *t = t_backup;
    return;
  }

  if (!dry_run)
    cpi->skip_false_count[mb_skip_context] += skip_inc;

  if (has_2nd_order) {
    tokenize_b(cpi, xd, 24, t, PLANE_TYPE_Y2, tx_size, dry_run);
    plane_type = PLANE_TYPE_Y_NO_DC;
  } else {
    xd->above_context->y2 = 0;
    xd->left_context->y2 = 0;
    plane_type = PLANE_TYPE_Y_WITH_DC;
  }

  if (tx_size == TX_16X16) {
    tokenize_b(cpi, xd, 0, t, PLANE_TYPE_Y_WITH_DC, TX_16X16, dry_run);
    for (b = 16; b < 24; b += 4) {
      tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_8X8, dry_run);
    }
  } else if (tx_size == TX_8X8) {
    for (b = 0; b < 16; b += 4) {
      tokenize_b(cpi, xd, b, t, plane_type, TX_8X8, dry_run);
    }
    if (xd->mode_info_context->mbmi.mode == I8X8_PRED ||
        xd->mode_info_context->mbmi.mode == SPLITMV) {
      for (b = 16; b < 24; b++) {
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_4X4, dry_run);
      }
    } else {
      for (b = 16; b < 24; b += 4) {
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_8X8, dry_run);
      }
    }
  } else {
    for (b = 0; b < 24; b++) {
      if (b >= 16)
        plane_type = PLANE_TYPE_UV;
      tokenize_b(cpi, xd, b, t, plane_type, TX_4X4, dry_run);
    }
  }
  if (dry_run)
    *t = t_backup;
}

#ifdef ENTROPY_STATS
void init_context_counters(void) {
  FILE *f = fopen("context.bin", "rb");
  if (!f) {
    vpx_memset(context_counters_4x4, 0, sizeof(context_counters_4x4));
    vpx_memset(hybrid_context_counters_4x4, 0,
               sizeof(hybrid_context_counters_4x4));
    vpx_memset(context_counters_8x8, 0, sizeof(context_counters_8x8));
    vpx_memset(hybrid_context_counters_8x8, 0,
               sizeof(hybrid_context_counters_8x8));
    vpx_memset(context_counters_16x16, 0, sizeof(context_counters_16x16));
    vpx_memset(hybrid_context_counters_16x16, 0,
               sizeof(hybrid_context_counters_16x16));
    vpx_memset(context_counters_32x32, 0, sizeof(context_counters_32x32));
  } else {
    fread(context_counters_4x4, sizeof(context_counters_4x4), 1, f);
    fread(hybrid_context_counters_4x4,
          sizeof(hybrid_context_counters_4x4), 1, f);
    fread(context_counters_8x8, sizeof(context_counters_8x8), 1, f);
    fread(hybrid_context_counters_8x8,
          sizeof(hybrid_context_counters_8x8), 1, f);
    fread(context_counters_16x16, sizeof(context_counters_16x16), 1, f);
    fread(hybrid_context_counters_16x16,
          sizeof(hybrid_context_counters_16x16), 1, f);
    fread(context_counters_32x32, sizeof(context_counters_32x32), 1, f);
    fclose(f);
  }

  f = fopen("treeupdate.bin", "rb");
  if (!f) {
    vpx_memset(tree_update_hist_4x4, 0, sizeof(tree_update_hist_4x4));
    vpx_memset(hybrid_tree_update_hist_4x4, 0,
               sizeof(hybrid_tree_update_hist_4x4));
    vpx_memset(tree_update_hist_8x8, 0, sizeof(tree_update_hist_8x8));
    vpx_memset(hybrid_tree_update_hist_8x8, 0,
               sizeof(hybrid_tree_update_hist_8x8));
    vpx_memset(tree_update_hist_16x16, 0, sizeof(tree_update_hist_16x16));
    vpx_memset(hybrid_tree_update_hist_16x16, 0,
               sizeof(hybrid_tree_update_hist_16x16));
    vpx_memset(tree_update_hist_32x32, 0, sizeof(tree_update_hist_32x32));
  } else {
    fread(tree_update_hist_4x4, sizeof(tree_update_hist_4x4), 1, f);
    fread(hybrid_tree_update_hist_4x4,
          sizeof(hybrid_tree_update_hist_4x4), 1, f);
    fread(tree_update_hist_8x8, sizeof(tree_update_hist_8x8), 1, f);
    fread(hybrid_tree_update_hist_8x8,
          sizeof(hybrid_tree_update_hist_8x8), 1, f);
    fread(tree_update_hist_16x16, sizeof(tree_update_hist_16x16), 1, f);
    fread(hybrid_tree_update_hist_16x16,
          sizeof(hybrid_tree_update_hist_16x16), 1, f);
    fread(tree_update_hist_32x32, sizeof(tree_update_hist_32x32), 1, f);
    fclose(f);
  }
}

static void print_counter(FILE *f, vp9_coeff_accum *context_counters,
                          int block_types, const char *header) {
  int type, band, pt, t;

  fprintf(f, "static const vp9_coeff_count %s = {\n", header);

#define Comma(X) (X ? "," : "")
  type = 0;
  do {
    fprintf(f, "%s\n  { /* block Type %d */", Comma(type), type);
    band = 0;
    do {
      fprintf(f, "%s\n    { /* Coeff Band %d */", Comma(band), band);
      pt = 0;
      do {
        fprintf(f, "%s\n      {", Comma(pt));

        t = 0;
        do {
          const int64_t x = context_counters[type][band][pt][t];
          const int y = (int) x;

          assert(x == (int64_t) y);  /* no overflow handling yet */
          fprintf(f, "%s %d", Comma(t), y);
        } while (++t < MAX_ENTROPY_TOKENS);
        fprintf(f, "}");
      } while (++pt < PREV_COEF_CONTEXTS);
      fprintf(f, "\n    }");
    } while (++band < COEF_BANDS);
    fprintf(f, "\n  }");
  } while (++type < block_types);
  fprintf(f, "\n};\n");
}

static void print_probs(FILE *f, vp9_coeff_accum *context_counters,
                        int block_types, const char *header) {
  int type, band, pt, t;

  fprintf(f, "static const vp9_coeff_probs %s = {", header);

  type = 0;
#define Newline(x, spaces) (x ? " " : "\n" spaces)
  do {
    fprintf(f, "%s%s{ /* block Type %d */",
            Comma(type), Newline(type, "  "), type);
    band = 0;
    do {
      fprintf(f, "%s%s{ /* Coeff Band %d */",
              Comma(band), Newline(band, "    "), band);
      pt = 0;
      do {
        unsigned int branch_ct[ENTROPY_NODES][2];
        unsigned int coef_counts[MAX_ENTROPY_TOKENS];
        vp9_prob coef_probs[ENTROPY_NODES];

        for (t = 0; t < MAX_ENTROPY_TOKENS; ++t)
          coef_counts[t] = context_counters[type][band][pt][t];
        vp9_tree_probs_from_distribution(MAX_ENTROPY_TOKENS,
                                         vp9_coef_encodings, vp9_coef_tree,
                                         coef_probs, branch_ct, coef_counts);
        fprintf(f, "%s\n      {", Comma(pt));

        t = 0;
        do {
          fprintf(f, "%s %3d", Comma(t), coef_probs[t]);
        } while (++t < ENTROPY_NODES);

        fprintf(f, " }");
      } while (++pt < PREV_COEF_CONTEXTS);
      fprintf(f, "\n    }");
    } while (++band < COEF_BANDS);
    fprintf(f, "\n  }");
  } while (++type < block_types);
  fprintf(f, "\n};\n");
}

void print_context_counters() {
  FILE *f = fopen("vp9_context.c", "w");

  fprintf(f, "#include \"vp9_entropy.h\"\n");
  fprintf(f, "\n/* *** GENERATED FILE: DO NOT EDIT *** */\n\n");

  /* print counts */
  print_counter(f, context_counters_4x4, BLOCK_TYPES_4X4,
                "vp9_default_coef_counts_4x4[BLOCK_TYPES_4X4]");
  print_counter(f, hybrid_context_counters_4x4, BLOCK_TYPES_4X4,
                "vp9_default_hybrid_coef_counts_4x4[BLOCK_TYPES_4X4]");
  print_counter(f, context_counters_8x8, BLOCK_TYPES_8X8,
                "vp9_default_coef_counts_8x8[BLOCK_TYPES_8X8]");
  print_counter(f, hybrid_context_counters_8x8, BLOCK_TYPES_8X8,
                "vp9_default_hybrid_coef_counts_8x8[BLOCK_TYPES_8X8]");
  print_counter(f, context_counters_16x16, BLOCK_TYPES_16X16,
                "vp9_default_coef_counts_16x16[BLOCK_TYPES_16X16]");
  print_counter(f, hybrid_context_counters_16x16, BLOCK_TYPES_16X16,
                "vp9_default_hybrid_coef_counts_16x16[BLOCK_TYPES_16X16]");
  print_counter(f, context_counters_32x32, BLOCK_TYPES_32X32,
                "vp9_default_coef_counts_32x32[BLOCK_TYPES_32X32]");

  /* print coefficient probabilities */
  print_probs(f, context_counters_4x4, BLOCK_TYPES_4X4,
              "default_coef_probs_4x4[BLOCK_TYPES_4X4]");
  print_probs(f, hybrid_context_counters_4x4, BLOCK_TYPES_4X4,
              "default_hybrid_coef_probs_4x4[BLOCK_TYPES_4X4]");
  print_probs(f, context_counters_8x8, BLOCK_TYPES_8X8,
              "default_coef_probs_8x8[BLOCK_TYPES_8X8]");
  print_probs(f, hybrid_context_counters_8x8, BLOCK_TYPES_8X8,
              "default_hybrid_coef_probs_8x8[BLOCK_TYPES_8X8]");
  print_probs(f, context_counters_16x16, BLOCK_TYPES_16X16,
              "default_coef_probs_16x16[BLOCK_TYPES_16X16]");
  print_probs(f, hybrid_context_counters_16x16, BLOCK_TYPES_16X16,
              "default_hybrid_coef_probs_16x16[BLOCK_TYPES_16X16]");
  print_probs(f, context_counters_32x32, BLOCK_TYPES_32X32,
              "default_coef_probs_32x32[BLOCK_TYPES_32X32]");

  fclose(f);

  f = fopen("context.bin", "wb");
  fwrite(context_counters_4x4, sizeof(context_counters_4x4), 1, f);
  fwrite(hybrid_context_counters_4x4,
         sizeof(hybrid_context_counters_4x4), 1, f);
  fwrite(context_counters_8x8, sizeof(context_counters_8x8), 1, f);
  fwrite(hybrid_context_counters_8x8,
         sizeof(hybrid_context_counters_8x8), 1, f);
  fwrite(context_counters_16x16, sizeof(context_counters_16x16), 1, f);
  fwrite(hybrid_context_counters_16x16,
         sizeof(hybrid_context_counters_16x16), 1, f);
  fwrite(context_counters_32x32, sizeof(context_counters_32x32), 1, f);
  fclose(f);
}
#endif

void vp9_tokenize_initialize() {
  fill_value_tokens();
}

static __inline void stuff_b(VP9_COMP *cpi,
                             MACROBLOCKD *xd,
                             const int ib,
                             TOKENEXTRA **tp,
                             PLANE_TYPE type,
                             TX_SIZE tx_size,
                             int dry_run) {
  const BLOCKD * const b = xd->block + ib;
  const int *bands;
  vp9_coeff_count *counts;
  vp9_coeff_probs *probs;
  int pt, band;
  TOKENEXTRA *t = *tp;
  const TX_TYPE tx_type = (type == PLANE_TYPE_Y_WITH_DC) ?
                          get_tx_type(xd, b) : DCT_DCT;
  ENTROPY_CONTEXT *const a = (ENTROPY_CONTEXT *)xd->above_context +
      vp9_block2above[tx_size][ib];
  ENTROPY_CONTEXT *const l = (ENTROPY_CONTEXT *)xd->left_context +
      vp9_block2left[tx_size][ib];
  ENTROPY_CONTEXT a_ec = *a, l_ec = *l;
  ENTROPY_CONTEXT *const a1 = (ENTROPY_CONTEXT *)(&xd->above_context[1]) +
      vp9_block2above[tx_size][ib];
  ENTROPY_CONTEXT *const l1 = (ENTROPY_CONTEXT *)(&xd->left_context[1]) +
      vp9_block2left[tx_size][ib];

  switch (tx_size) {
    default:
    case TX_4X4:
      bands = vp9_coef_bands_4x4;
      if (tx_type != DCT_DCT) {
        counts = cpi->hybrid_coef_counts_4x4;
        probs = cpi->common.fc.hybrid_coef_probs_4x4;
      } else {
        counts = cpi->coef_counts_4x4;
        probs = cpi->common.fc.coef_probs_4x4;
      }
      break;
    case TX_8X8:
#if CONFIG_CNVCONTEXT
      if (type != PLANE_TYPE_Y2) {
        a_ec = (a[0] + a[1]) != 0;
        l_ec = (l[0] + l[1]) != 0;
      }
#endif
      bands = vp9_coef_bands_8x8;
      if (tx_type != DCT_DCT) {
        counts = cpi->hybrid_coef_counts_8x8;
        probs = cpi->common.fc.hybrid_coef_probs_8x8;
      } else {
        counts = cpi->coef_counts_8x8;
        probs = cpi->common.fc.coef_probs_8x8;
      }
      break;
    case TX_16X16:
#if CONFIG_CNVCONTEXT
      if (type != PLANE_TYPE_UV) {
        a_ec = (a[0] + a[1] + a[2] + a[3]) != 0;
        l_ec = (l[0] + l[1] + l[2] + l[3]) != 0;
      } else {
        a_ec = (a[0] + a[1] + a1[0] + a1[1]) != 0;
        l_ec = (l[0] + l[1] + l1[0] + l1[1]) != 0;
      }
#endif
      bands = vp9_coef_bands_16x16;
      if (tx_type != DCT_DCT) {
        counts = cpi->hybrid_coef_counts_16x16;
        probs = cpi->common.fc.hybrid_coef_probs_16x16;
      } else {
        counts = cpi->coef_counts_16x16;
        probs = cpi->common.fc.coef_probs_16x16;
      }
      break;
    case TX_32X32:
#if CONFIG_CNVCONTEXT
      a_ec = a[0] + a[1] + a[2] + a[3] +
             a1[0] + a1[1] + a1[2] + a1[3];
      l_ec = l[0] + l[1] + l[2] + l[3] +
             l1[0] + l1[1] + l1[2] + l1[3];
      a_ec = a_ec != 0;
      l_ec = l_ec != 0;
#endif
      bands = vp9_coef_bands_32x32;
      counts = cpi->coef_counts_32x32;
      probs = cpi->common.fc.coef_probs_32x32;
      break;
  }

  VP9_COMBINEENTROPYCONTEXTS(pt, a_ec, l_ec);

  band = bands[(type == PLANE_TYPE_Y_NO_DC) ? 1 : 0];
  t->Token = DCT_EOB_TOKEN;
  t->context_tree = probs[type][band][pt];
  t->skip_eob_node = 0;
  ++t;
  *tp = t;
  *a = *l = 0;
  if (tx_size == TX_8X8 && type != PLANE_TYPE_Y2) {
    a[1] = 0;
    l[1] = 0;
  } else if (tx_size == TX_16X16) {
    if (type != PLANE_TYPE_UV) {
      a[1] = a[2] = a[3] = 0;
      l[1] = l[2] = l[3] = 0;
    } else {
      a1[0] = a1[1] = a[1] = a_ec;
      l1[0] = l1[1] = l[1] = l_ec;
    }
  } else if (tx_size == TX_32X32) {
    a[1] = a[2] = a[3] = a_ec;
    l[1] = l[2] = l[3] = l_ec;
    a1[0] = a1[1] = a1[2] = a1[3] = a_ec;
    l1[0] = l1[1] = l1[2] = l1[3] = l_ec;
  }

  if (!dry_run) {
    ++counts[type][band][pt][DCT_EOB_TOKEN];
  }
}

static void stuff_mb_8x8(VP9_COMP *cpi, MACROBLOCKD *xd,
                         TOKENEXTRA **t, int dry_run) {
  PLANE_TYPE plane_type;
  int b;
  int has_2nd_order = get_2nd_order_usage(xd);

  if (has_2nd_order) {
    stuff_b(cpi, xd, 24, t, PLANE_TYPE_Y2, TX_8X8, dry_run);
    plane_type = PLANE_TYPE_Y_NO_DC;
  } else {
#if CONFIG_CNVCONTEXT
    xd->above_context->y2 = 0;
    xd->left_context->y2 = 0;
#endif
    plane_type = PLANE_TYPE_Y_WITH_DC;
  }

  for (b = 0; b < 24; b += 4) {
    if (b >= 16)
      plane_type = PLANE_TYPE_UV;
    stuff_b(cpi, xd, b, t, plane_type, TX_8X8, dry_run);
  }
}

static void stuff_mb_16x16(VP9_COMP *cpi, MACROBLOCKD *xd,
                           TOKENEXTRA **t, int dry_run) {
  int b;
  stuff_b(cpi, xd, 0, t, PLANE_TYPE_Y_WITH_DC, TX_16X16, dry_run);

  for (b = 16; b < 24; b += 4) {
    stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_8X8, dry_run);
  }
#if CONFIG_CNVCONTEXT
  xd->above_context->y2 = 0;
  xd->left_context->y2 = 0;
#endif
}

static void stuff_mb_4x4(VP9_COMP *cpi, MACROBLOCKD *xd,
                         TOKENEXTRA **t, int dry_run) {
  int b;
  PLANE_TYPE plane_type;
  int has_2nd_order = get_2nd_order_usage(xd);

  if (has_2nd_order) {
    stuff_b(cpi, xd, 24, t, PLANE_TYPE_Y2, TX_4X4, dry_run);
    plane_type = PLANE_TYPE_Y_NO_DC;
  } else {
    xd->above_context->y2 = 0;
    xd->left_context->y2 = 0;
    plane_type = PLANE_TYPE_Y_WITH_DC;
  }

  for (b = 0; b < 24; b++) {
    if (b >= 16)
      plane_type = PLANE_TYPE_UV;
    stuff_b(cpi, xd, b, t, plane_type, TX_4X4, dry_run);
  }
}

static void stuff_mb_8x8_4x4uv(VP9_COMP *cpi, MACROBLOCKD *xd,
                               TOKENEXTRA **t, int dry_run) {
  PLANE_TYPE plane_type;
  int b;

  int has_2nd_order = get_2nd_order_usage(xd);
  if (has_2nd_order) {
    stuff_b(cpi, xd, 24, t, PLANE_TYPE_Y2, TX_8X8, dry_run);
    plane_type = PLANE_TYPE_Y_NO_DC;
  } else {
    xd->above_context->y2 = 0;
    xd->left_context->y2 = 0;
    plane_type = PLANE_TYPE_Y_WITH_DC;
  }

  for (b = 0; b < 16; b += 4) {
    stuff_b(cpi, xd, b, t, plane_type, TX_8X8, dry_run);
  }

  for (b = 16; b < 24; b++) {
    stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_4X4, dry_run);
  }
}

void vp9_stuff_mb(VP9_COMP *cpi, MACROBLOCKD *xd, TOKENEXTRA **t, int dry_run) {
  TX_SIZE tx_size = xd->mode_info_context->mbmi.txfm_size;
  TOKENEXTRA * const t_backup = *t;

  if (tx_size == TX_16X16) {
    stuff_mb_16x16(cpi, xd, t, dry_run);
  } else if (tx_size == TX_8X8) {
    if (xd->mode_info_context->mbmi.mode == I8X8_PRED ||
        xd->mode_info_context->mbmi.mode == SPLITMV) {
      stuff_mb_8x8_4x4uv(cpi, xd, t, dry_run);
    } else {
      stuff_mb_8x8(cpi, xd, t, dry_run);
    }
  } else {
    stuff_mb_4x4(cpi, xd, t, dry_run);
  }

  if (dry_run) {
    *t = t_backup;
  }
}

static void stuff_sb_32x32(VP9_COMP *cpi, MACROBLOCKD *xd,
                               TOKENEXTRA **t, int dry_run) {
  int b;

  stuff_b(cpi, xd, 0, t, PLANE_TYPE_Y_WITH_DC, TX_32X32, dry_run);
  for (b = 16; b < 24; b += 4) {
    stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_16X16, dry_run);
  }
}

void vp9_stuff_sb(VP9_COMP *cpi, MACROBLOCKD *xd, TOKENEXTRA **t, int dry_run) {
  TOKENEXTRA * const t_backup = *t;

  stuff_sb_32x32(cpi, xd, t, dry_run);

  if (dry_run) {
    *t = t_backup;
  }
}

void vp9_fix_contexts_sb(MACROBLOCKD *xd) {
  vpx_memset(xd->above_context, 0, sizeof(ENTROPY_CONTEXT_PLANES) * 2);
  vpx_memset(xd->left_context, 0, sizeof(ENTROPY_CONTEXT_PLANES) * 2);
}
