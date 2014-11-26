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
vp9_coeff_accum context_counters_4x4[BLOCK_TYPES];
vp9_coeff_accum context_counters_8x8[BLOCK_TYPES];
vp9_coeff_accum context_counters_16x16[BLOCK_TYPES];
vp9_coeff_accum context_counters_32x32[BLOCK_TYPES];

extern vp9_coeff_stats tree_update_hist_4x4[BLOCK_TYPES];
extern vp9_coeff_stats tree_update_hist_8x8[BLOCK_TYPES];
extern vp9_coeff_stats tree_update_hist_16x16[BLOCK_TYPES];
extern vp9_coeff_stats tree_update_hist_32x32[BLOCK_TYPES];
#endif  /* ENTROPY_STATS */

#if CONFIG_CODE_NONZEROCOUNT
#ifdef NZC_STATS
unsigned int nzc_counts_4x4[MAX_NZC_CONTEXTS][REF_TYPES][BLOCK_TYPES]
                           [NZC4X4_TOKENS];
unsigned int nzc_counts_8x8[MAX_NZC_CONTEXTS][REF_TYPES][BLOCK_TYPES]
                           [NZC8X8_TOKENS];
unsigned int nzc_counts_16x16[MAX_NZC_CONTEXTS][REF_TYPES][BLOCK_TYPES]
                             [NZC16X16_TOKENS];
unsigned int nzc_counts_32x32[MAX_NZC_CONTEXTS][REF_TYPES][BLOCK_TYPES]
                             [NZC32X32_TOKENS];
unsigned int nzc_pcat_counts[MAX_NZC_CONTEXTS][NZC_TOKENS_EXTRA]
                            [NZC_BITS_EXTRA][2];
#endif
#endif

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

extern const int *vp9_get_coef_neighbors_handle(const int *scan, int *pad);

static void tokenize_b(VP9_COMP *cpi,
                       MACROBLOCKD *xd,
                       const int ib,
                       TOKENEXTRA **tp,
                       PLANE_TYPE type,
                       TX_SIZE tx_size,
                       int dry_run) {
  MB_MODE_INFO *mbmi = &xd->mode_info_context->mbmi;
  int pt; /* near block/prev token context index */
  int c = 0;
  const int eob = xd->eobs[ib];     /* one beyond last nonzero coeff */
  TOKENEXTRA *t = *tp;        /* store tokens starting here */
  int16_t *qcoeff_ptr = xd->qcoeff + 16 * ib;
  int seg_eob, default_eob, pad;
  const int segment_id = mbmi->segment_id;
  const BLOCK_SIZE_TYPE sb_type = mbmi->sb_type;
  const int *scan, *nb;
  vp9_coeff_count *counts;
  vp9_coeff_probs *probs;
  const int ref = mbmi->ref_frame != INTRA_FRAME;
  ENTROPY_CONTEXT *a, *l, *a1, *l1, *a2, *l2, *a3, *l3, a_ec, l_ec;
  uint8_t token_cache[1024];
#if CONFIG_CODE_NONZEROCOUNT
  int zerosleft, nzc = 0;
  if (eob == 0)
    assert(xd->nzcs[ib] == 0);
#endif

  if (sb_type == BLOCK_SIZE_SB64X64) {
    a = (ENTROPY_CONTEXT *)xd->above_context +
                                             vp9_block2above_sb64[tx_size][ib];
    l = (ENTROPY_CONTEXT *)xd->left_context + vp9_block2left_sb64[tx_size][ib];
    a1 = a + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l1 = l + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    a2 = a1 + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l2 = l1 + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    a3 = a2 + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l3 = l2 + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
  } else if (sb_type == BLOCK_SIZE_SB32X32) {
    a = (ENTROPY_CONTEXT *)xd->above_context + vp9_block2above_sb[tx_size][ib];
    l = (ENTROPY_CONTEXT *)xd->left_context + vp9_block2left_sb[tx_size][ib];
    a1 = a + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l1 = l + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    a2 = a3 = l2 = l3 = NULL;
  } else {
    a = (ENTROPY_CONTEXT *)xd->above_context + vp9_block2above[tx_size][ib];
    l = (ENTROPY_CONTEXT *)xd->left_context + vp9_block2left[tx_size][ib];
    a1 = l1 = a2 = l2 = a3 = l3 = NULL;
  }

  switch (tx_size) {
    default:
    case TX_4X4: {
      const TX_TYPE tx_type = (type == PLANE_TYPE_Y_WITH_DC) ?
                              get_tx_type_4x4(xd, ib) : DCT_DCT;
      a_ec = *a;
      l_ec = *l;
      seg_eob = 16;
      scan = vp9_default_zig_zag1d_4x4;
      if (tx_type != DCT_DCT) {
        if (tx_type == ADST_DCT) {
          scan = vp9_row_scan_4x4;
        } else if (tx_type == DCT_ADST) {
          scan = vp9_col_scan_4x4;
        }
      }
      counts = cpi->coef_counts_4x4;
      probs = cpi->common.fc.coef_probs_4x4;
      break;
    }
    case TX_8X8: {
      const int sz = 3 + sb_type, x = ib & ((1 << sz) - 1), y = ib - x;
      const TX_TYPE tx_type = (type == PLANE_TYPE_Y_WITH_DC) ?
                              get_tx_type_8x8(xd, y + (x >> 1)) : DCT_DCT;
      a_ec = (a[0] + a[1]) != 0;
      l_ec = (l[0] + l[1]) != 0;
      seg_eob = 64;
      scan = vp9_default_zig_zag1d_8x8;
      if (tx_type != DCT_DCT) {
        if (tx_type == ADST_DCT) {
          scan = vp9_row_scan_8x8;
        } else if (tx_type == DCT_ADST) {
          scan = vp9_col_scan_8x8;
        }
      }
      counts = cpi->coef_counts_8x8;
      probs = cpi->common.fc.coef_probs_8x8;
      break;
    }
    case TX_16X16: {
      const int sz = 4 + sb_type, x = ib & ((1 << sz) - 1), y = ib - x;
      const TX_TYPE tx_type = (type == PLANE_TYPE_Y_WITH_DC) ?
                              get_tx_type_16x16(xd, y + (x >> 2)) : DCT_DCT;
      if (type != PLANE_TYPE_UV) {
        a_ec = (a[0] + a[1] + a[2] + a[3]) != 0;
        l_ec = (l[0] + l[1] + l[2] + l[3]) != 0;
      } else {
        a_ec = (a[0] + a[1] + a1[0] + a1[1]) != 0;
        l_ec = (l[0] + l[1] + l1[0] + l1[1]) != 0;
      }
      seg_eob = 256;
      scan = vp9_default_zig_zag1d_16x16;
      if (tx_type != DCT_DCT) {
        if (tx_type == ADST_DCT) {
          scan = vp9_row_scan_16x16;
        } else if (tx_type == DCT_ADST) {
          scan = vp9_col_scan_16x16;
        }
      }
      counts = cpi->coef_counts_16x16;
      probs = cpi->common.fc.coef_probs_16x16;
      break;
    }
    case TX_32X32:
      if (type != PLANE_TYPE_UV) {
        a_ec = (a[0] + a[1] + a[2] + a[3] +
                a1[0] + a1[1] + a1[2] + a1[3]) != 0;
        l_ec = (l[0] + l[1] + l[2] + l[3] +
                l1[0] + l1[1] + l1[2] + l1[3]) != 0;
      } else {
        a_ec = (a[0] + a[1] + a1[0] + a1[1] +
                a2[0] + a2[1] + a3[0] + a3[1]) != 0;
        l_ec = (l[0] + l[1] + l1[0] + l1[1] +
                l2[0] + l2[1] + l3[0] + l3[1]) != 0;
      }
      seg_eob = 1024;
      scan = vp9_default_zig_zag1d_32x32;
      counts = cpi->coef_counts_32x32;
      probs = cpi->common.fc.coef_probs_32x32;
      break;
  }

  VP9_COMBINEENTROPYCONTEXTS(pt, a_ec, l_ec);
  nb = vp9_get_coef_neighbors_handle(scan, &pad);
  default_eob = seg_eob;

  if (vp9_segfeature_active(xd, segment_id, SEG_LVL_SKIP))
    seg_eob = 0;

  do {
    const int band = get_coef_band(scan, tx_size, c);
    int token;
    int v = 0;
#if CONFIG_CODE_NONZEROCOUNT
    zerosleft = seg_eob - xd->nzcs[ib] - c + nzc;
#endif
    if (c < eob) {
      const int rc = scan[c];
      v = qcoeff_ptr[rc];
      assert(-DCT_MAX_VALUE <= v  &&  v < DCT_MAX_VALUE);

      t->Extra = vp9_dct_value_tokens_ptr[v].Extra;
      token    = vp9_dct_value_tokens_ptr[v].Token;
    } else {
#if CONFIG_CODE_NONZEROCOUNT
      break;
#else
      token = DCT_EOB_TOKEN;
#endif
    }

    t->Token = token;
    t->context_tree = probs[type][ref][band][pt];
#if CONFIG_CODE_NONZEROCOUNT
    // Skip zero node if there are no zeros left
    t->skip_eob_node = 1 + (zerosleft == 0);
#else
    t->skip_eob_node = (c > 0) && (token_cache[c - 1] == 0);
#endif
    assert(vp9_coef_encodings[t->Token].Len - t->skip_eob_node > 0);
    if (!dry_run) {
      ++counts[type][ref][band][pt][token];
      if (!t->skip_eob_node)
        ++cpi->common.fc.eob_branch_counts[tx_size][type][ref][band][pt];
    }
#if CONFIG_CODE_NONZEROCOUNT
    nzc += (v != 0);
#endif
    token_cache[c] = token;

    pt = vp9_get_coef_context(scan, nb, pad, token_cache, c + 1, default_eob);
    ++t;
  } while (c < eob && ++c < seg_eob);
#if CONFIG_CODE_NONZEROCOUNT
  assert(nzc == xd->nzcs[ib]);
#endif

  *tp = t;
  a_ec = l_ec = (c > 0); /* 0 <-> all coeff data is zero */
  a[0] = a_ec;
  l[0] = l_ec;

  if (tx_size == TX_8X8) {
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
    if (type != PLANE_TYPE_UV) {
      a[1] = a[2] = a[3] = a_ec;
      l[1] = l[2] = l[3] = l_ec;
      a1[0] = a1[1] = a1[2] = a1[3] = a_ec;
      l1[0] = l1[1] = l1[2] = l1[3] = l_ec;
    } else {
      a[1] = a1[0] = a1[1] = a_ec;
      l[1] = l1[0] = l1[1] = l_ec;
      a2[0] = a2[1] = a3[0] = a3[1] = a_ec;
      l2[0] = l2[1] = l3[0] = l3[1] = l_ec;
    }
  }
}

int vp9_mby_is_skippable_4x4(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 0; i < 16; i++)
    skip &= (!xd->eobs[i]);

  return skip;
}

int vp9_mbuv_is_skippable_4x4(MACROBLOCKD *xd) {
  int skip = 1;
  int i;

  for (i = 16; i < 24; i++)
    skip &= (!xd->eobs[i]);
  return skip;
}

static int mb_is_skippable_4x4(MACROBLOCKD *xd) {
  return (vp9_mby_is_skippable_4x4(xd) &
          vp9_mbuv_is_skippable_4x4(xd));
}

int vp9_mby_is_skippable_8x8(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 0; i < 16; i += 4)
    skip &= (!xd->eobs[i]);

  return skip;
}

int vp9_mbuv_is_skippable_8x8(MACROBLOCKD *xd) {
  return (!xd->eobs[16]) & (!xd->eobs[20]);
}

static int mb_is_skippable_8x8(MACROBLOCKD *xd) {
  return (vp9_mby_is_skippable_8x8(xd) &
          vp9_mbuv_is_skippable_8x8(xd));
}

static int mb_is_skippable_8x8_4x4uv(MACROBLOCKD *xd) {
  return (vp9_mby_is_skippable_8x8(xd) &
          vp9_mbuv_is_skippable_4x4(xd));
}

int vp9_mby_is_skippable_16x16(MACROBLOCKD *xd) {
  return (!xd->eobs[0]);
}

static int mb_is_skippable_16x16(MACROBLOCKD *xd) {
  return (vp9_mby_is_skippable_16x16(xd) & vp9_mbuv_is_skippable_8x8(xd));
}

int vp9_sby_is_skippable_32x32(MACROBLOCKD *xd) {
  return (!xd->eobs[0]);
}

int vp9_sbuv_is_skippable_16x16(MACROBLOCKD *xd) {
  return (!xd->eobs[64]) & (!xd->eobs[80]);
}

static int sb_is_skippable_32x32(MACROBLOCKD *xd) {
  return vp9_sby_is_skippable_32x32(xd) &&
         vp9_sbuv_is_skippable_16x16(xd);
}

int vp9_sby_is_skippable_16x16(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 0; i < 64; i += 16)
    skip &= (!xd->eobs[i]);

  return skip;
}

static int sb_is_skippable_16x16(MACROBLOCKD *xd) {
  return vp9_sby_is_skippable_16x16(xd) & vp9_sbuv_is_skippable_16x16(xd);
}

int vp9_sby_is_skippable_8x8(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 0; i < 64; i += 4)
    skip &= (!xd->eobs[i]);

  return skip;
}

int vp9_sbuv_is_skippable_8x8(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 64; i < 96; i += 4)
    skip &= (!xd->eobs[i]);

  return skip;
}

static int sb_is_skippable_8x8(MACROBLOCKD *xd) {
  return vp9_sby_is_skippable_8x8(xd) & vp9_sbuv_is_skippable_8x8(xd);
}

int vp9_sby_is_skippable_4x4(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 0; i < 64; i++)
    skip &= (!xd->eobs[i]);

  return skip;
}

int vp9_sbuv_is_skippable_4x4(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 64; i < 96; i++)
    skip &= (!xd->eobs[i]);

  return skip;
}

static int sb_is_skippable_4x4(MACROBLOCKD *xd) {
  return vp9_sby_is_skippable_4x4(xd) & vp9_sbuv_is_skippable_4x4(xd);
}

void vp9_tokenize_sb(VP9_COMP *cpi,
                     MACROBLOCKD *xd,
                     TOKENEXTRA **t,
                     int dry_run) {
  VP9_COMMON * const cm = &cpi->common;
  MB_MODE_INFO * const mbmi = &xd->mode_info_context->mbmi;
  TOKENEXTRA *t_backup = *t;
  const int mb_skip_context = vp9_get_pred_context(cm, xd, PRED_MBSKIP);
  const int segment_id = mbmi->segment_id;
  const int skip_inc = !vp9_segfeature_active(xd, segment_id, SEG_LVL_SKIP);
  int b;

  switch (mbmi->txfm_size) {
    case TX_32X32:
      mbmi->mb_skip_coeff = sb_is_skippable_32x32(xd);
      break;
    case TX_16X16:
      mbmi->mb_skip_coeff = sb_is_skippable_16x16(xd);
      break;
    case TX_8X8:
      mbmi->mb_skip_coeff = sb_is_skippable_8x8(xd);
      break;
    case TX_4X4:
      mbmi->mb_skip_coeff = sb_is_skippable_4x4(xd);
      break;
    default: assert(0);
  }

  if (mbmi->mb_skip_coeff) {
    if (!dry_run)
      cpi->skip_true_count[mb_skip_context] += skip_inc;
    if (!cm->mb_no_coeff_skip) {
      vp9_stuff_sb(cpi, xd, t, dry_run);
    } else {
      vp9_reset_sb_tokens_context(xd);
    }
    if (dry_run)
      *t = t_backup;
    return;
  }

  if (!dry_run)
    cpi->skip_false_count[mb_skip_context] += skip_inc;

  switch (mbmi->txfm_size) {
    case TX_32X32:
      tokenize_b(cpi, xd, 0, t, PLANE_TYPE_Y_WITH_DC,
                 TX_32X32, dry_run);
      for (b = 64; b < 96; b += 16)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV,
                   TX_16X16, dry_run);
      break;
    case TX_16X16:
      for (b = 0; b < 64; b += 16)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC,
                   TX_16X16, dry_run);
      for (b = 64; b < 96; b += 16)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV,
                   TX_16X16, dry_run);
      break;
    case TX_8X8:
      for (b = 0; b < 64; b += 4)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC,
                   TX_8X8, dry_run);
      for (b = 64; b < 96; b += 4)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV,
                   TX_8X8, dry_run);
      break;
    case TX_4X4:
      for (b = 0; b < 64; b++)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC,
                   TX_4X4, dry_run);
      for (b = 64; b < 96; b++)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV,
                   TX_4X4, dry_run);
      break;
    default: assert(0);
  }

  if (dry_run)
    *t = t_backup;
}

int vp9_sb64y_is_skippable_32x32(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 0; i < 256; i += 64)
    skip &= (!xd->eobs[i]);

  return skip;
}

int vp9_sb64uv_is_skippable_32x32(MACROBLOCKD *xd) {
  return (!xd->eobs[256]) & (!xd->eobs[320]);
}

static int sb64_is_skippable_32x32(MACROBLOCKD *xd) {
  return vp9_sb64y_is_skippable_32x32(xd) & vp9_sb64uv_is_skippable_32x32(xd);
}

int vp9_sb64y_is_skippable_16x16(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 0; i < 256; i += 16)
    skip &= (!xd->eobs[i]);

  return skip;
}

int vp9_sb64uv_is_skippable_16x16(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 256; i < 384; i += 16)
    skip &= (!xd->eobs[i]);

  return skip;
}

static int sb64_is_skippable_16x16(MACROBLOCKD *xd) {
  return vp9_sb64y_is_skippable_16x16(xd) & vp9_sb64uv_is_skippable_16x16(xd);
}

int vp9_sb64y_is_skippable_8x8(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 0; i < 256; i += 4)
    skip &= (!xd->eobs[i]);

  return skip;
}

int vp9_sb64uv_is_skippable_8x8(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 256; i < 384; i += 4)
    skip &= (!xd->eobs[i]);

  return skip;
}

static int sb64_is_skippable_8x8(MACROBLOCKD *xd) {
  return vp9_sb64y_is_skippable_8x8(xd) & vp9_sb64uv_is_skippable_8x8(xd);
}

int vp9_sb64y_is_skippable_4x4(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 0; i < 256; i++)
    skip &= (!xd->eobs[i]);

  return skip;
}

int vp9_sb64uv_is_skippable_4x4(MACROBLOCKD *xd) {
  int skip = 1;
  int i = 0;

  for (i = 256; i < 384; i++)
    skip &= (!xd->eobs[i]);

  return skip;
}

static int sb64_is_skippable_4x4(MACROBLOCKD *xd) {
  return vp9_sb64y_is_skippable_4x4(xd) & vp9_sb64uv_is_skippable_4x4(xd);
}

void vp9_tokenize_sb64(VP9_COMP *cpi,
                       MACROBLOCKD *xd,
                       TOKENEXTRA **t,
                       int dry_run) {
  VP9_COMMON * const cm = &cpi->common;
  MB_MODE_INFO * const mbmi = &xd->mode_info_context->mbmi;
  TOKENEXTRA *t_backup = *t;
  const int mb_skip_context = vp9_get_pred_context(cm, xd, PRED_MBSKIP);
  const int segment_id = mbmi->segment_id;
  const int skip_inc = !vp9_segfeature_active(xd, segment_id, SEG_LVL_SKIP);
  int b;

  switch (mbmi->txfm_size) {
    case TX_32X32:
      mbmi->mb_skip_coeff = sb64_is_skippable_32x32(xd);
      break;
    case TX_16X16:
      mbmi->mb_skip_coeff = sb64_is_skippable_16x16(xd);
      break;
    case TX_8X8:
      mbmi->mb_skip_coeff = sb64_is_skippable_8x8(xd);
      break;
    case TX_4X4:
      mbmi->mb_skip_coeff = sb64_is_skippable_4x4(xd);
      break;
    default: assert(0);
  }

  if (mbmi->mb_skip_coeff) {
    if (!dry_run)
      cpi->skip_true_count[mb_skip_context] += skip_inc;
    if (!cm->mb_no_coeff_skip) {
      vp9_stuff_sb64(cpi, xd, t, dry_run);
    } else {
      vp9_reset_sb64_tokens_context(xd);
    }
    if (dry_run)
      *t = t_backup;
    return;
  }

  if (!dry_run)
    cpi->skip_false_count[mb_skip_context] += skip_inc;

  switch (mbmi->txfm_size) {
    case TX_32X32:
      for (b = 0; b < 256; b += 64)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC,
                   TX_32X32, dry_run);
      for (b = 256; b < 384; b += 64)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV,
                   TX_32X32, dry_run);
      break;
    case TX_16X16:
      for (b = 0; b < 256; b += 16)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC,
                   TX_16X16, dry_run);
      for (b = 256; b < 384; b += 16)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV,
                   TX_16X16, dry_run);
      break;
    case TX_8X8:
      for (b = 0; b < 256; b += 4)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC,
                   TX_8X8, dry_run);
      for (b = 256; b < 384; b += 4)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV,
                   TX_8X8, dry_run);
      break;
    case TX_4X4:
      for (b = 0; b < 256; b++)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC,
                   TX_4X4, dry_run);
      for (b = 256; b < 384; b++)
        tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV,
                   TX_4X4, dry_run);
      break;
    default: assert(0);
  }

  if (dry_run)
    *t = t_backup;
}

void vp9_tokenize_mb(VP9_COMP *cpi,
                     MACROBLOCKD *xd,
                     TOKENEXTRA **t,
                     int dry_run) {
  int b;
  int tx_size = xd->mode_info_context->mbmi.txfm_size;
  int mb_skip_context = vp9_get_pred_context(&cpi->common, xd, PRED_MBSKIP);
  TOKENEXTRA *t_backup = *t;

  // If the MB is going to be skipped because of a segment level flag
  // exclude this from the skip count stats used to calculate the
  // transmitted skip probability;
  int skip_inc;
  int segment_id = xd->mode_info_context->mbmi.segment_id;

  if (!vp9_segfeature_active(xd, segment_id, SEG_LVL_SKIP)) {
    skip_inc = 1;
  } else
    skip_inc = 0;

  switch (tx_size) {
    case TX_16X16:

      xd->mode_info_context->mbmi.mb_skip_coeff = mb_is_skippable_16x16(xd);
      break;
    case TX_8X8:
      if (xd->mode_info_context->mbmi.mode == I8X8_PRED ||
          xd->mode_info_context->mbmi.mode == SPLITMV)
        xd->mode_info_context->mbmi.mb_skip_coeff =
            mb_is_skippable_8x8_4x4uv(xd);
      else
        xd->mode_info_context->mbmi.mb_skip_coeff =
            mb_is_skippable_8x8(xd);
      break;

    default:
      xd->mode_info_context->mbmi.mb_skip_coeff =
          mb_is_skippable_4x4(xd);
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

  if (tx_size == TX_16X16) {
    tokenize_b(cpi, xd, 0, t, PLANE_TYPE_Y_WITH_DC, TX_16X16, dry_run);
    for (b = 16; b < 24; b += 4) {
      tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_8X8, dry_run);
    }
  } else if (tx_size == TX_8X8) {
    for (b = 0; b < 16; b += 4) {
      tokenize_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_8X8, dry_run);
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
    for (b = 0; b < 16; b++)
      tokenize_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_4X4, dry_run);
    for (b = 16; b < 24; b++)
      tokenize_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_4X4, dry_run);
  }
  if (dry_run)
    *t = t_backup;
}

#ifdef ENTROPY_STATS
void init_context_counters(void) {
  FILE *f = fopen("context.bin", "rb");
  if (!f) {
    vpx_memset(context_counters_4x4, 0, sizeof(context_counters_4x4));
    vpx_memset(context_counters_8x8, 0, sizeof(context_counters_8x8));
    vpx_memset(context_counters_16x16, 0, sizeof(context_counters_16x16));
    vpx_memset(context_counters_32x32, 0, sizeof(context_counters_32x32));
  } else {
    fread(context_counters_4x4, sizeof(context_counters_4x4), 1, f);
    fread(context_counters_8x8, sizeof(context_counters_8x8), 1, f);
    fread(context_counters_16x16, sizeof(context_counters_16x16), 1, f);
    fread(context_counters_32x32, sizeof(context_counters_32x32), 1, f);
    fclose(f);
  }

  f = fopen("treeupdate.bin", "rb");
  if (!f) {
    vpx_memset(tree_update_hist_4x4, 0, sizeof(tree_update_hist_4x4));
    vpx_memset(tree_update_hist_8x8, 0, sizeof(tree_update_hist_8x8));
    vpx_memset(tree_update_hist_16x16, 0, sizeof(tree_update_hist_16x16));
    vpx_memset(tree_update_hist_32x32, 0, sizeof(tree_update_hist_32x32));
  } else {
    fread(tree_update_hist_4x4, sizeof(tree_update_hist_4x4), 1, f);
    fread(tree_update_hist_8x8, sizeof(tree_update_hist_8x8), 1, f);
    fread(tree_update_hist_16x16, sizeof(tree_update_hist_16x16), 1, f);
    fread(tree_update_hist_32x32, sizeof(tree_update_hist_32x32), 1, f);
    fclose(f);
  }
}

static void print_counter(FILE *f, vp9_coeff_accum *context_counters,
                          int block_types, const char *header) {
  int type, ref, band, pt, t;

  fprintf(f, "static const vp9_coeff_count %s = {\n", header);

#define Comma(X) (X ? "," : "")
  type = 0;
  do {
    ref = 0;
    fprintf(f, "%s\n  { /* block Type %d */", Comma(type), type);
    do {
      fprintf(f, "%s\n    { /* %s */", Comma(type), ref ? "Inter" : "Intra");
      band = 0;
      do {
        fprintf(f, "%s\n      { /* Coeff Band %d */", Comma(band), band);
        pt = 0;
        do {
          fprintf(f, "%s\n        {", Comma(pt));

          t = 0;
          do {
            const int64_t x = context_counters[type][ref][band][pt][t];
            const int y = (int) x;

            assert(x == (int64_t) y);  /* no overflow handling yet */
            fprintf(f, "%s %d", Comma(t), y);
          } while (++t < 1 + MAX_ENTROPY_TOKENS);
          fprintf(f, "}");
        } while (++pt < PREV_COEF_CONTEXTS);
        fprintf(f, "\n      }");
      } while (++band < COEF_BANDS);
      fprintf(f, "\n    }");
    } while (++ref < REF_TYPES);
    fprintf(f, "\n  }");
  } while (++type < block_types);
  fprintf(f, "\n};\n");
}

static void print_probs(FILE *f, vp9_coeff_accum *context_counters,
                        int block_types, const char *header) {
  int type, ref, band, pt, t;

  fprintf(f, "static const vp9_coeff_probs %s = {", header);

  type = 0;
#define Newline(x, spaces) (x ? " " : "\n" spaces)
  do {
    fprintf(f, "%s%s{ /* block Type %d */",
            Comma(type), Newline(type, "  "), type);
    ref = 0;
    do {
      fprintf(f, "%s%s{ /* %s */",
              Comma(band), Newline(band, "    "), ref ? "Inter" : "Intra");
      band = 0;
      do {
        fprintf(f, "%s%s{ /* Coeff Band %d */",
                Comma(band), Newline(band, "      "), band);
        pt = 0;
        do {
          unsigned int branch_ct[ENTROPY_NODES][2];
          unsigned int coef_counts[MAX_ENTROPY_TOKENS + 1];
          vp9_prob coef_probs[ENTROPY_NODES];

          if (pt >= 3 && band == 0)
            break;
          for (t = 0; t < MAX_ENTROPY_TOKENS + 1; ++t)
            coef_counts[t] = context_counters[type][ref][band][pt][t];
          vp9_tree_probs_from_distribution(vp9_coef_tree, coef_probs,
                                           branch_ct, coef_counts, 0);
          branch_ct[0][1] = coef_counts[MAX_ENTROPY_TOKENS] - branch_ct[0][0];
          coef_probs[0] = get_binary_prob(branch_ct[0][0], branch_ct[0][1]);
          fprintf(f, "%s\n      {", Comma(pt));

          t = 0;
          do {
            fprintf(f, "%s %3d", Comma(t), coef_probs[t]);
          } while (++t < ENTROPY_NODES);

          fprintf(f, " }");
        } while (++pt < PREV_COEF_CONTEXTS);
        fprintf(f, "\n      }");
      } while (++band < COEF_BANDS);
      fprintf(f, "\n    }");
    } while (++ref < REF_TYPES);
    fprintf(f, "\n  }");
  } while (++type < block_types);
  fprintf(f, "\n};\n");
}

void print_context_counters() {
  FILE *f = fopen("vp9_context.c", "w");

  fprintf(f, "#include \"vp9_entropy.h\"\n");
  fprintf(f, "\n/* *** GENERATED FILE: DO NOT EDIT *** */\n\n");

  /* print counts */
  print_counter(f, context_counters_4x4, BLOCK_TYPES,
                "vp9_default_coef_counts_4x4[BLOCK_TYPES]");
  print_counter(f, context_counters_8x8, BLOCK_TYPES,
                "vp9_default_coef_counts_8x8[BLOCK_TYPES]");
  print_counter(f, context_counters_16x16, BLOCK_TYPES,
                "vp9_default_coef_counts_16x16[BLOCK_TYPES]");
  print_counter(f, context_counters_32x32, BLOCK_TYPES,
                "vp9_default_coef_counts_32x32[BLOCK_TYPES]");

  /* print coefficient probabilities */
  print_probs(f, context_counters_4x4, BLOCK_TYPES,
              "default_coef_probs_4x4[BLOCK_TYPES]");
  print_probs(f, context_counters_8x8, BLOCK_TYPES,
              "default_coef_probs_8x8[BLOCK_TYPES]");
  print_probs(f, context_counters_16x16, BLOCK_TYPES,
              "default_coef_probs_16x16[BLOCK_TYPES]");
  print_probs(f, context_counters_32x32, BLOCK_TYPES,
              "default_coef_probs_32x32[BLOCK_TYPES]");

  fclose(f);

  f = fopen("context.bin", "wb");
  fwrite(context_counters_4x4, sizeof(context_counters_4x4), 1, f);
  fwrite(context_counters_8x8, sizeof(context_counters_8x8), 1, f);
  fwrite(context_counters_16x16, sizeof(context_counters_16x16), 1, f);
  fwrite(context_counters_32x32, sizeof(context_counters_32x32), 1, f);
  fclose(f);
}
#endif

void vp9_tokenize_initialize() {
  fill_value_tokens();
}

static void stuff_b(VP9_COMP *cpi,
                    MACROBLOCKD *xd,
                    const int ib,
                    TOKENEXTRA **tp,
                    PLANE_TYPE type,
                    TX_SIZE tx_size,
                    int dry_run) {
  MB_MODE_INFO *mbmi = &xd->mode_info_context->mbmi;
  const BLOCK_SIZE_TYPE sb_type = mbmi->sb_type;
#if CONFIG_CODE_NONZEROCOUNT == 0
  vp9_coeff_count *counts;
  vp9_coeff_probs *probs;
  int pt, band;
  TOKENEXTRA *t = *tp;
  const int ref = mbmi->ref_frame != INTRA_FRAME;
#endif
  ENTROPY_CONTEXT *a, *l, *a1, *l1, *a2, *l2, *a3, *l3, a_ec, l_ec;

  if (sb_type == BLOCK_SIZE_SB32X32) {
    a = (ENTROPY_CONTEXT *)xd->above_context +
                                             vp9_block2above_sb64[tx_size][ib];
    l = (ENTROPY_CONTEXT *)xd->left_context + vp9_block2left_sb64[tx_size][ib];
    a1 = a + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l1 = l + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    a2 = a1 + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l2 = l1 + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    a3 = a2 + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l3 = l2 + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
  } else if (sb_type == BLOCK_SIZE_SB32X32) {
    a = (ENTROPY_CONTEXT *)xd->above_context + vp9_block2above_sb[tx_size][ib];
    l = (ENTROPY_CONTEXT *)xd->left_context + vp9_block2left_sb[tx_size][ib];
    a1 = a + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l1 = l + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    a2 = l2 = a3 = l3 = NULL;
  } else {
    a = (ENTROPY_CONTEXT *)xd->above_context + vp9_block2above[tx_size][ib];
    l = (ENTROPY_CONTEXT *)xd->left_context + vp9_block2left[tx_size][ib];
    a1 = l1 = a2 = l2 = a3 = l3 = NULL;
  }

  switch (tx_size) {
    default:
    case TX_4X4:
      a_ec = a[0];
      l_ec = l[0];
#if CONFIG_CODE_NONZEROCOUNT == 0
      counts = cpi->coef_counts_4x4;
      probs = cpi->common.fc.coef_probs_4x4;
#endif
      break;
    case TX_8X8:
      a_ec = (a[0] + a[1]) != 0;
      l_ec = (l[0] + l[1]) != 0;
#if CONFIG_CODE_NONZEROCOUNT == 0
      counts = cpi->coef_counts_8x8;
      probs = cpi->common.fc.coef_probs_8x8;
#endif
      break;
    case TX_16X16:
      if (type != PLANE_TYPE_UV) {
        a_ec = (a[0] + a[1] + a[2] + a[3]) != 0;
        l_ec = (l[0] + l[1] + l[2] + l[3]) != 0;
      } else {
        a_ec = (a[0] + a[1] + a1[0] + a1[1]) != 0;
        l_ec = (l[0] + l[1] + l1[0] + l1[1]) != 0;
      }
#if CONFIG_CODE_NONZEROCOUNT == 0
      counts = cpi->coef_counts_16x16;
      probs = cpi->common.fc.coef_probs_16x16;
#endif
      break;
    case TX_32X32:
      if (type != PLANE_TYPE_UV) {
        a_ec = (a[0] + a[1] + a[2] + a[3] +
                a1[0] + a1[1] + a1[2] + a1[3]) != 0;
        l_ec = (l[0] + l[1] + l[2] + l[3] +
                l1[0] + l1[1] + l1[2] + l1[3]) != 0;
      } else {
        a_ec = (a[0] + a[1] + a1[0] + a1[1] +
                a2[0] + a2[1] + a3[0] + a3[1]) != 0;
        l_ec = (l[0] + l[1] + l1[0] + l1[1] +
                l2[0] + l2[1] + l3[0] + l3[1]) != 0;
      }
#if CONFIG_CODE_NONZEROCOUNT == 0
      counts = cpi->coef_counts_32x32;
      probs = cpi->common.fc.coef_probs_32x32;
#endif
      break;
  }

#if CONFIG_CODE_NONZEROCOUNT == 0
  VP9_COMBINEENTROPYCONTEXTS(pt, a_ec, l_ec);
  band = 0;
  t->Token = DCT_EOB_TOKEN;
  t->context_tree = probs[type][ref][band][pt];
  t->skip_eob_node = 0;
  ++t;
  *tp = t;
  if (!dry_run) {
    ++counts[type][ref][band][pt][DCT_EOB_TOKEN];
  }
#endif
  *a = *l = 0;
  if (tx_size == TX_8X8) {
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
    if (type != PLANE_TYPE_Y_WITH_DC) {
      a[1] = a[2] = a[3] = a_ec;
      l[1] = l[2] = l[3] = l_ec;
      a1[0] = a1[1] = a1[2] = a1[3] = a_ec;
      l1[0] = l1[1] = l1[2] = l1[3] = l_ec;
    } else {
      a[1] = a1[0] = a1[1] = a_ec;
      l[1] = l1[0] = l1[1] = l_ec;
      a2[0] = a2[1] = a3[0] = a3[1] = a_ec;
      l2[0] = l2[1] = l3[0] = l3[1] = l_ec;
    }
  }
}

static void stuff_mb_8x8(VP9_COMP *cpi, MACROBLOCKD *xd,
                         TOKENEXTRA **t, int dry_run) {
  int b;

  for (b = 0; b < 16; b += 4)
    stuff_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_8X8, dry_run);
  for (b = 16; b < 24; b += 4)
    stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_8X8, dry_run);
}

static void stuff_mb_16x16(VP9_COMP *cpi, MACROBLOCKD *xd,
                           TOKENEXTRA **t, int dry_run) {
  int b;
  stuff_b(cpi, xd, 0, t, PLANE_TYPE_Y_WITH_DC, TX_16X16, dry_run);

  for (b = 16; b < 24; b += 4) {
    stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_8X8, dry_run);
  }
}

static void stuff_mb_4x4(VP9_COMP *cpi, MACROBLOCKD *xd,
                         TOKENEXTRA **t, int dry_run) {
  int b;

  for (b = 0; b < 16; b++)
    stuff_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_4X4, dry_run);
  for (b = 16; b < 24; b++)
    stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_4X4, dry_run);
}

static void stuff_mb_8x8_4x4uv(VP9_COMP *cpi, MACROBLOCKD *xd,
                               TOKENEXTRA **t, int dry_run) {
  int b;

  for (b = 0; b < 16; b += 4)
    stuff_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_8X8, dry_run);
  for (b = 16; b < 24; b++)
    stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_4X4, dry_run);
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

void vp9_stuff_sb(VP9_COMP *cpi, MACROBLOCKD *xd, TOKENEXTRA **t, int dry_run) {
  TOKENEXTRA * const t_backup = *t;
  int b;

  switch (xd->mode_info_context->mbmi.txfm_size) {
    case TX_32X32:
      stuff_b(cpi, xd, 0, t, PLANE_TYPE_Y_WITH_DC, TX_32X32, dry_run);
      for (b = 64; b < 96; b += 16)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_16X16, dry_run);
      break;
    case TX_16X16:
      for (b = 0; b < 64; b += 16)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_16X16, dry_run);
      for (b = 64; b < 96; b += 16)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_16X16, dry_run);
      break;
    case TX_8X8:
      for (b = 0; b < 64; b += 4)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_8X8, dry_run);
      for (b = 64; b < 96; b += 4)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_8X8, dry_run);
      break;
    case TX_4X4:
      for (b = 0; b < 64; b++)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_4X4, dry_run);
      for (b = 64; b < 96; b++)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_4X4, dry_run);
      break;
    default: assert(0);
  }

  if (dry_run) {
    *t = t_backup;
  }
}

void vp9_stuff_sb64(VP9_COMP *cpi, MACROBLOCKD *xd,
                    TOKENEXTRA **t, int dry_run) {
  TOKENEXTRA * const t_backup = *t;
  int b;

  switch (xd->mode_info_context->mbmi.txfm_size) {
    case TX_32X32:
      for (b = 0; b < 256; b += 64)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_32X32, dry_run);
      for (b = 256; b < 384; b += 64)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_32X32, dry_run);
      break;
    case TX_16X16:
      for (b = 0; b < 256; b += 16)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_16X16, dry_run);
      for (b = 256; b < 384; b += 16)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_16X16, dry_run);
      break;
    case TX_8X8:
      for (b = 0; b < 256; b += 4)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_8X8, dry_run);
      for (b = 256; b < 384; b += 4)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_8X8, dry_run);
      break;
    case TX_4X4:
      for (b = 0; b < 256; b++)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_Y_WITH_DC, TX_4X4, dry_run);
      for (b = 256; b < 384; b++)
        stuff_b(cpi, xd, b, t, PLANE_TYPE_UV, TX_4X4, dry_run);
      break;
    default: assert(0);
  }

  if (dry_run) {
    *t = t_backup;
  }
}
