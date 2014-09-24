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
#include "vpx_mem/vpx_mem.h"

#include "vp9/encoder/vp9_onyx_int.h"
#include "vp9/encoder/vp9_quantize.h"
#include "vp9/common/vp9_quant_common.h"

#include "vp9/common/vp9_seg_common.h"

#ifdef ENC_DEBUG
extern int enc_debug;
#endif

void vp9_ht_quantize_b_4x4(BLOCK *b, BLOCKD *d, TX_TYPE tx_type) {
  int i, rc, eob;
  int zbin;
  int x, y, z, sz;
  int16_t *zbin_boost_ptr  = b->zrun_zbin_boost;
  int16_t *coeff_ptr       = b->coeff;
  int16_t *zbin_ptr        = b->zbin;
  int16_t *round_ptr       = b->round;
  int16_t *quant_ptr       = b->quant;
  uint8_t *quant_shift_ptr = b->quant_shift;
  int16_t *qcoeff_ptr      = d->qcoeff;
  int16_t *dqcoeff_ptr     = d->dqcoeff;
  int16_t *dequant_ptr     = d->dequant;
  int zbin_oq_value        = b->zbin_extra;

  int const *pt_scan ;

  switch (tx_type) {
    case ADST_DCT:
      pt_scan = vp9_row_scan_4x4;
      break;

    case DCT_ADST:
      pt_scan = vp9_col_scan_4x4;
      break;

    default:
      pt_scan = vp9_default_zig_zag1d_4x4;
      break;
  }

  vpx_memset(qcoeff_ptr, 0, 32);
  vpx_memset(dqcoeff_ptr, 0, 32);

  eob = -1;

  for (i = 0; i < b->eob_max_offset; i++) {
    rc   = pt_scan[i];
    z    = coeff_ptr[rc];

    zbin = zbin_ptr[rc] + *zbin_boost_ptr + zbin_oq_value;
    zbin_boost_ptr ++;

    sz = (z >> 31);                                 // sign of z
    x  = (z ^ sz) - sz;                             // x = abs(z)

    if (x >= zbin) {
      x += round_ptr[rc];
      y  = (((x * quant_ptr[rc]) >> 16) + x)
           >> quant_shift_ptr[rc];                // quantize (x)
      x  = (y ^ sz) - sz;                         // get the sign back
      qcoeff_ptr[rc]  = x;                        // write to destination
      dqcoeff_ptr[rc] = x * dequant_ptr[rc];      // dequantized value

      if (y) {
        eob = i;                                // last nonzero coeffs
        zbin_boost_ptr = b->zrun_zbin_boost;    // reset zero runlength
      }
    }
  }

  d->eob = eob + 1;
}

void vp9_regular_quantize_b_4x4(BLOCK *b, BLOCKD *d) {
  int i, rc, eob;
  int zbin;
  int x, y, z, sz;
  int16_t *zbin_boost_ptr  = b->zrun_zbin_boost;
  int16_t *coeff_ptr       = b->coeff;
  int16_t *zbin_ptr        = b->zbin;
  int16_t *round_ptr       = b->round;
  int16_t *quant_ptr       = b->quant;
  uint8_t *quant_shift_ptr = b->quant_shift;
  int16_t *qcoeff_ptr      = d->qcoeff;
  int16_t *dqcoeff_ptr     = d->dqcoeff;
  int16_t *dequant_ptr     = d->dequant;
  int zbin_oq_value        = b->zbin_extra;

  vpx_memset(qcoeff_ptr, 0, 32);
  vpx_memset(dqcoeff_ptr, 0, 32);

  eob = -1;

  for (i = 0; i < b->eob_max_offset; i++) {
    rc   = vp9_default_zig_zag1d_4x4[i];
    z    = coeff_ptr[rc];

    zbin = zbin_ptr[rc] + *zbin_boost_ptr + zbin_oq_value;
    zbin_boost_ptr ++;

    sz = (z >> 31);                                 // sign of z
    x  = (z ^ sz) - sz;                             // x = abs(z)

    if (x >= zbin) {
      x += round_ptr[rc];

      y  = (((x * quant_ptr[rc]) >> 16) + x)
           >> quant_shift_ptr[rc];                // quantize (x)
      x  = (y ^ sz) - sz;                         // get the sign back
      qcoeff_ptr[rc]  = x;                        // write to destination
      dqcoeff_ptr[rc] = x * dequant_ptr[rc];      // dequantized value

      if (y) {
        eob = i;                                // last nonzero coeffs
        zbin_boost_ptr = b->zrun_zbin_boost;    // reset zero runlength
      }
    }
  }

  d->eob = eob + 1;
}

void vp9_quantize_mby_4x4_c(MACROBLOCK *x) {
  int i;
  int has_2nd_order = get_2nd_order_usage(&x->e_mbd);

  for (i = 0; i < 16; i++) {
    TX_TYPE tx_type = get_tx_type_4x4(&x->e_mbd, &x->e_mbd.block[i]);
    if (tx_type != DCT_DCT) {
      assert(has_2nd_order == 0);
      vp9_ht_quantize_b_4x4(&x->block[i], &x->e_mbd.block[i], tx_type);
    } else {
      x->quantize_b_4x4(&x->block[i], &x->e_mbd.block[i]);
    }
  }
  if (has_2nd_order) {
    x->quantize_b_4x4(&x->block[24], &x->e_mbd.block[24]);
  } else {
    vpx_memset(x->e_mbd.block[24].qcoeff, 0,
               16 * sizeof(x->e_mbd.block[24].qcoeff[0]));
    vpx_memset(x->e_mbd.block[24].dqcoeff, 0,
               16 * sizeof(x->e_mbd.block[24].dqcoeff[0]));
    x->e_mbd.block[24].eob = 0;
  }
}

void vp9_quantize_mbuv_4x4_c(MACROBLOCK *x) {
  int i;

  for (i = 16; i < 24; i++)
    x->quantize_b_4x4(&x->block[i], &x->e_mbd.block[i]);
}

void vp9_quantize_mb_4x4_c(MACROBLOCK *x) {
  vp9_quantize_mby_4x4_c(x);
  vp9_quantize_mbuv_4x4_c(x);
}

void vp9_regular_quantize_b_2x2(BLOCK *b, BLOCKD *d) {
  int i, rc, eob;
  int zbin;
  int x, y, z, sz;
  int16_t *zbin_boost_ptr = b->zrun_zbin_boost;
  int zbin_zrun_index = 0;
  int16_t *coeff_ptr  = b->coeff;
  int16_t *zbin_ptr   = b->zbin;
  int16_t *round_ptr  = b->round;
  int16_t *quant_ptr  = b->quant;
  uint8_t *quant_shift_ptr = b->quant_shift;
  int16_t *qcoeff_ptr = d->qcoeff;
  int16_t *dqcoeff_ptr = d->dqcoeff;
  int16_t *dequant_ptr = d->dequant;
  int zbin_oq_value    = b->zbin_extra;
  // double q2nd = 4;
  vpx_memset(qcoeff_ptr, 0, 32);
  vpx_memset(dqcoeff_ptr, 0, 32);

  eob = -1;

  for (i = 0; i < b->eob_max_offset_8x8; i++) {
    rc   = vp9_default_zig_zag1d_4x4[i];
    z    = coeff_ptr[rc];

    zbin_boost_ptr = &b->zrun_zbin_boost[zbin_zrun_index];
    zbin_zrun_index += 4;
    zbin = (zbin_ptr[rc] + *zbin_boost_ptr + zbin_oq_value);

    sz = (z >> 31);                               // sign of z
    x  = (z ^ sz) - sz;                           // x = abs(z)

    if (x >= zbin) {
      x += (round_ptr[rc]);
      y  = ((int)((int)(x * quant_ptr[rc]) >> 16) + x)
           >> quant_shift_ptr[rc];                // quantize (x)
      x  = (y ^ sz) - sz;                         // get the sign back
      qcoeff_ptr[rc]  = x;                        // write to destination
      dqcoeff_ptr[rc] = x * dequant_ptr[rc];      // dequantized value

      if (y) {
        eob = i;                                  // last nonzero coeffs
        zbin_zrun_index = 0;
      }
    }
  }

  d->eob = eob + 1;
}

void vp9_regular_quantize_b_8x8(BLOCK *b, BLOCKD *d) {
  int i, rc, eob;
  int zbin;
  int x, y, z, sz;
  int16_t *zbin_boost_ptr = b->zrun_zbin_boost_8x8;
  int16_t *coeff_ptr  = b->coeff;
  int16_t *zbin_ptr   = b->zbin_8x8;
  int16_t *round_ptr  = b->round;
  int16_t *quant_ptr  = b->quant;
  uint8_t *quant_shift_ptr = b->quant_shift;
  int16_t *qcoeff_ptr = d->qcoeff;
  int16_t *dqcoeff_ptr = d->dqcoeff;
  int16_t *dequant_ptr = d->dequant;
  int zbin_oq_value = b->zbin_extra;

  vpx_memset(qcoeff_ptr, 0, 64 * sizeof(int16_t));
  vpx_memset(dqcoeff_ptr, 0, 64 * sizeof(int16_t));

  eob = -1;

  for (i = 0; i < b->eob_max_offset_8x8; i++) {
    rc   = vp9_default_zig_zag1d_8x8[i];
    z    = coeff_ptr[rc];

    zbin = (zbin_ptr[rc != 0] + *zbin_boost_ptr + zbin_oq_value);
    zbin_boost_ptr++;

    sz = (z >> 31);                               // sign of z
    x  = (z ^ sz) - sz;                           // x = abs(z)

    if (x >= zbin) {
      x += (round_ptr[rc != 0]);
      y  = ((int)(((int)(x * quant_ptr[rc != 0]) >> 16) + x))
           >> quant_shift_ptr[rc != 0];            // quantize (x)
      x  = (y ^ sz) - sz;                         // get the sign back
      qcoeff_ptr[rc]  = x;                        // write to destination
      dqcoeff_ptr[rc] = x * dequant_ptr[rc != 0]; // dequantized value

      if (y) {
        eob = i;                                  // last nonzero coeffs
        zbin_boost_ptr = b->zrun_zbin_boost_8x8;
      }
    }
  }

  d->eob = eob + 1;
}

void vp9_quantize_mby_8x8(MACROBLOCK *x) {
  int i;
  int has_2nd_order = get_2nd_order_usage(&x->e_mbd);

  for (i = 0; i < 16; i ++) {
    x->e_mbd.block[i].eob = 0;
  }
  x->e_mbd.block[24].eob = 0;
  for (i = 0; i < 16; i += 4) {
    int ib = (i & 8) + ((i & 4) >> 1);
    TX_TYPE tx_type = get_tx_type_8x8(&x->e_mbd, &x->e_mbd.block[ib]);
    if (tx_type != DCT_DCT)
      assert(has_2nd_order == 0);
    x->quantize_b_8x8(&x->block[i], &x->e_mbd.block[i]);
  }

  if (has_2nd_order) {
    x->quantize_b_2x2(&x->block[24], &x->e_mbd.block[24]);
  } else {
    vpx_memset(x->e_mbd.block[24].qcoeff, 0,
               16 * sizeof(x->e_mbd.block[24].qcoeff[0]));
    vpx_memset(x->e_mbd.block[24].dqcoeff, 0,
               16 * sizeof(x->e_mbd.block[24].dqcoeff[0]));
    x->e_mbd.block[24].eob = 0;
  }
}

void vp9_quantize_mbuv_8x8(MACROBLOCK *x) {
  int i;

  for (i = 16; i < 24; i ++)
    x->e_mbd.block[i].eob = 0;
  for (i = 16; i < 24; i += 4)
    x->quantize_b_8x8(&x->block[i], &x->e_mbd.block[i]);
}

void vp9_quantize_mb_8x8(MACROBLOCK *x) {
  vp9_quantize_mby_8x8(x);
  vp9_quantize_mbuv_8x8(x);
}

void vp9_quantize_mby_16x16(MACROBLOCK *x) {
  int i;

  for (i = 0; i < 16; i++)
    x->e_mbd.block[i].eob = 0;
  x->e_mbd.block[24].eob = 0;
  x->quantize_b_16x16(&x->block[0], &x->e_mbd.block[0]);
}

void vp9_quantize_mb_16x16(MACROBLOCK *x) {
  vp9_quantize_mby_16x16(x);
  vp9_quantize_mbuv_8x8(x);
}

static void quantize(int16_t *zbin_boost_orig_ptr,
                     int16_t *coeff_ptr, int n_coeffs, int max_coeffs,
                     int16_t *zbin_ptr, int16_t *round_ptr, int16_t *quant_ptr,
                     uint8_t *quant_shift_ptr,
                     int16_t *qcoeff_ptr, int16_t *dqcoeff_ptr,
                     int16_t *dequant_ptr, int zbin_oq_value,
                     int *eob_ptr, const int *scan, int mul) {
  int i, rc, eob;
  int zbin;
  int x, y, z, sz;
  int16_t *zbin_boost_ptr = zbin_boost_orig_ptr;

  vpx_memset(qcoeff_ptr, 0, n_coeffs*sizeof(int16_t));
  vpx_memset(dqcoeff_ptr, 0, n_coeffs*sizeof(int16_t));

  eob = -1;
  for (i = 0; i < max_coeffs; i++) {
    rc   = scan[i];
    z    = coeff_ptr[rc] * mul;

    zbin = (zbin_ptr[rc!=0] + *zbin_boost_ptr + zbin_oq_value);
    zbin_boost_ptr ++;

    sz = (z >> 31);                               // sign of z
    x  = (z ^ sz) - sz;                           // x = abs(z)

    if (x >= zbin) {
      x += (round_ptr[rc!=0]);
      y  = ((int)(((int)(x * quant_ptr[rc != 0]) >> 16) + x))
          >> quant_shift_ptr[rc!=0];              // quantize (x)
      x  = (y ^ sz) - sz;                         // get the sign back
      qcoeff_ptr[rc]  = x;                        // write to destination
      dqcoeff_ptr[rc] = x * dequant_ptr[rc != 0] / mul;  // dequantized value

      if (y) {
        eob = i;                                  // last nonzero coeffs
        zbin_boost_ptr = zbin_boost_orig_ptr;
      }
    }
  }

  *eob_ptr = eob + 1;
}

void vp9_regular_quantize_b_16x16(BLOCK *b, BLOCKD *d) {
  quantize(b->zrun_zbin_boost_16x16,
           b->coeff,
           256, b->eob_max_offset_16x16,
           b->zbin_16x16, b->round, b->quant, b->quant_shift,
           d->qcoeff,
           d->dqcoeff,
           d->dequant,
           b->zbin_extra,
           &d->eob, vp9_default_zig_zag1d_16x16, 1);
}

void vp9_quantize_sby_32x32(MACROBLOCK *x) {
  x->e_mbd.block[0].eob = 0;
  quantize(x->block[0].zrun_zbin_boost_32x32,
           x->sb_coeff_data.coeff,
           1024, x->block[0].eob_max_offset_32x32,
           x->block[0].zbin_32x32,
           x->block[0].round, x->block[0].quant, x->block[0].quant_shift,
           x->e_mbd.sb_coeff_data.qcoeff,
           x->e_mbd.sb_coeff_data.dqcoeff,
           x->e_mbd.block[0].dequant,
           x->block[0].zbin_extra,
           &x->e_mbd.block[0].eob,
           vp9_default_zig_zag1d_32x32, 2);
}

void vp9_quantize_sbuv_16x16(MACROBLOCK *x) {
  int i;

  x->e_mbd.block[16].eob = 0;
  x->e_mbd.block[20].eob = 0;
  for (i = 16; i < 24; i += 4)
    quantize(x->block[i].zrun_zbin_boost_16x16,
             x->sb_coeff_data.coeff + 1024 + (i - 16) * 64,
             256, x->block[i].eob_max_offset_16x16,
             x->block[i].zbin_16x16,
             x->block[i].round, x->block[0].quant, x->block[i].quant_shift,
             x->e_mbd.sb_coeff_data.qcoeff + 1024 + (i - 16) * 64,
             x->e_mbd.sb_coeff_data.dqcoeff + 1024 + (i - 16) * 64,
             x->e_mbd.block[i].dequant,
             x->block[i].zbin_extra,
             &x->e_mbd.block[i].eob,
             vp9_default_zig_zag1d_16x16, 1);
}

/* quantize_b_pair function pointer in MACROBLOCK structure is set to one of
 * these two C functions if corresponding optimized routine is not available.
 * NEON optimized version implements currently the fast quantization for pair
 * of blocks. */
void vp9_regular_quantize_b_4x4_pair(BLOCK *b1, BLOCK *b2,
                                     BLOCKD *d1, BLOCKD *d2) {
  vp9_regular_quantize_b_4x4(b1, d1);
  vp9_regular_quantize_b_4x4(b2, d2);
}

static void invert_quant(int16_t *quant,
                         uint8_t *shift, int d) {
  unsigned t;
  int l;
  t = d;
  for (l = 0; t > 1; l++)
    t >>= 1;
  t = 1 + (1 << (16 + l)) / d;
  *quant = (int16_t)(t - (1 << 16));
  *shift = l;
}

void vp9_init_quantizer(VP9_COMP *cpi) {
  int i;
  int quant_val;
  int Q;
  static const int zbin_boost[16] = {  0,  0,  8, 10, 12, 14, 16, 20,
                                      24, 28, 32, 36, 40, 44, 44, 44
                                    };

  static const int zbin_boost_8x8[64] = {  0,  0,  0,  8,  8,  8, 10, 12,
                                          14, 16, 18, 20, 22, 24, 26, 28,
                                          30, 32, 34, 36, 38, 40, 42, 44,
                                          46, 48, 48, 48, 48, 48, 48, 48,
                                          48, 48, 48, 48, 48, 48, 48, 48,
                                          48, 48, 48, 48, 48, 48, 48, 48,
                                          48, 48, 48, 48, 48, 48, 48, 48,
                                          48, 48, 48, 48, 48, 48, 48, 48
                                        };
  static const int zbin_boost_16x16[256] = {
     0,  0,  0,  8,  8,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
    30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
  };
  static const int zbin_boost_32x32[1024] = {
    0,  0,  0,  8,  8,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
    30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
  };
  int qrounding_factor = 48;


  for (Q = 0; Q < QINDEX_RANGE; Q++) {
    int qzbin_factor = (vp9_dc_quant(Q, 0) < 148) ? 84 : 80;

#if CONFIG_LOSSLESS
    if (cpi->oxcf.lossless) {
      if (Q == 0) {
        qzbin_factor = 64;
        qrounding_factor = 64;
      }
    }
#endif

    // dc values
    quant_val = vp9_dc_quant(Q, cpi->common.y1dc_delta_q);
    invert_quant(cpi->Y1quant[Q] + 0,
                 cpi->Y1quant_shift[Q] + 0, quant_val);
    cpi->Y1zbin[Q][0] = ((qzbin_factor * quant_val) + 64) >> 7;
    cpi->Y1zbin_8x8[Q][0] = ((qzbin_factor * quant_val) + 64) >> 7;
    cpi->Y1zbin_16x16[Q][0] = ((qzbin_factor * quant_val) + 64) >> 7;
    cpi->Y1round[Q][0] = (qrounding_factor * quant_val) >> 7;
    cpi->common.Y1dequant[Q][0] = quant_val;
    cpi->zrun_zbin_boost_y1[Q][0] = (quant_val * zbin_boost[0]) >> 7;
    cpi->zrun_zbin_boost_y1_8x8[Q][0] =
      ((quant_val * zbin_boost_8x8[0]) + 64) >> 7;
    cpi->zrun_zbin_boost_y1_16x16[Q][0] =
      ((quant_val * zbin_boost_16x16[0]) + 64) >> 7;
    cpi->Y1zbin_32x32[Q][0] = ((qzbin_factor * quant_val) + 64) >> 7;
    cpi->zrun_zbin_boost_y1_32x32[Q][0] =
     ((quant_val * zbin_boost_32x32[0]) + 64) >> 7;


    quant_val = vp9_dc2quant(Q, cpi->common.y2dc_delta_q);
    invert_quant(cpi->Y2quant[Q] + 0,
                 cpi->Y2quant_shift[Q] + 0, quant_val);
    cpi->Y2zbin[Q][0] = ((qzbin_factor * quant_val) + 64) >> 7;
    cpi->Y2zbin_8x8[Q][0] = ((qzbin_factor * quant_val) + 64) >> 7;
    cpi->Y2zbin_16x16[Q][0] = ((qzbin_factor * quant_val) + 64) >> 7;
    cpi->Y2round[Q][0] = (qrounding_factor * quant_val) >> 7;
    cpi->common.Y2dequant[Q][0] = quant_val;
    cpi->zrun_zbin_boost_y2[Q][0] = (quant_val * zbin_boost[0]) >> 7;
    cpi->zrun_zbin_boost_y2_8x8[Q][0] =
      ((quant_val * zbin_boost_8x8[0]) + 64) >> 7;
    cpi->zrun_zbin_boost_y2_16x16[Q][0] =
      ((quant_val * zbin_boost_16x16[0]) + 64) >> 7;

    quant_val = vp9_dc_uv_quant(Q, cpi->common.uvdc_delta_q);
    invert_quant(cpi->UVquant[Q] + 0,
                 cpi->UVquant_shift[Q] + 0, quant_val);
    cpi->UVzbin[Q][0] = ((qzbin_factor * quant_val) + 64) >> 7;
    cpi->UVzbin_8x8[Q][0] = ((qzbin_factor * quant_val) + 64) >> 7;
    cpi->UVzbin_16x16[Q][0] = ((qzbin_factor * quant_val) + 64) >> 7;
    cpi->UVround[Q][0] = (qrounding_factor * quant_val) >> 7;
    cpi->common.UVdequant[Q][0] = quant_val;
    cpi->zrun_zbin_boost_uv[Q][0] = (quant_val * zbin_boost[0]) >> 7;
    cpi->zrun_zbin_boost_uv_8x8[Q][0] =
      ((quant_val * zbin_boost_8x8[0]) + 64) >> 7;
    cpi->zrun_zbin_boost_uv_16x16[Q][0] =
      ((quant_val * zbin_boost_16x16[0]) + 64) >> 7;

    // all the 4x4 ac values =;
    for (i = 1; i < 16; i++) {
      int rc = vp9_default_zig_zag1d_4x4[i];

      quant_val = vp9_ac_yquant(Q);
      invert_quant(cpi->Y1quant[Q] + rc,
                   cpi->Y1quant_shift[Q] + rc, quant_val);
      cpi->Y1zbin[Q][rc] = ((qzbin_factor * quant_val) + 64) >> 7;
      cpi->Y1round[Q][rc] = (qrounding_factor * quant_val) >> 7;
      cpi->common.Y1dequant[Q][rc] = quant_val;
      cpi->zrun_zbin_boost_y1[Q][i] =
        ((quant_val * zbin_boost[i]) + 64) >> 7;

      quant_val = vp9_ac2quant(Q, cpi->common.y2ac_delta_q);
      invert_quant(cpi->Y2quant[Q] + rc,
                   cpi->Y2quant_shift[Q] + rc, quant_val);
      cpi->Y2zbin[Q][rc] = ((qzbin_factor * quant_val) + 64) >> 7;
      cpi->Y2round[Q][rc] = (qrounding_factor * quant_val) >> 7;
      cpi->common.Y2dequant[Q][rc] = quant_val;
      cpi->zrun_zbin_boost_y2[Q][i] =
        ((quant_val * zbin_boost[i]) + 64) >> 7;

      quant_val = vp9_ac_uv_quant(Q, cpi->common.uvac_delta_q);
      invert_quant(cpi->UVquant[Q] + rc,
                   cpi->UVquant_shift[Q] + rc, quant_val);
      cpi->UVzbin[Q][rc] = ((qzbin_factor * quant_val) + 64) >> 7;
      cpi->UVround[Q][rc] = (qrounding_factor * quant_val) >> 7;
      cpi->common.UVdequant[Q][rc] = quant_val;
      cpi->zrun_zbin_boost_uv[Q][i] =
        ((quant_val * zbin_boost[i]) + 64) >> 7;
    }

    // 8x8 structures... only zbin seperated out for now
    // This needs cleaning up for 8x8 especially if we are to add
    // support for non flat Q matices
    for (i = 1; i < 64; i++) {
      int rc = vp9_default_zig_zag1d_8x8[i];

      quant_val = vp9_ac_yquant(Q);
      cpi->Y1zbin_8x8[Q][rc] = ((qzbin_factor * quant_val) + 64) >> 7;
      cpi->zrun_zbin_boost_y1_8x8[Q][i] =
        ((quant_val * zbin_boost_8x8[i]) + 64) >> 7;

      quant_val = vp9_ac2quant(Q, cpi->common.y2ac_delta_q);
      cpi->Y2zbin_8x8[Q][rc] = ((qzbin_factor * quant_val) + 64) >> 7;
      cpi->zrun_zbin_boost_y2_8x8[Q][i] =
        ((quant_val * zbin_boost_8x8[i]) + 64) >> 7;

      quant_val = vp9_ac_uv_quant(Q, cpi->common.uvac_delta_q);
      cpi->UVzbin_8x8[Q][rc] = ((qzbin_factor * quant_val) + 64) >> 7;
      cpi->zrun_zbin_boost_uv_8x8[Q][i] =
        ((quant_val * zbin_boost_8x8[i]) + 64) >> 7;
    }

    // 16x16 structures. Same comment above applies.
    for (i = 1; i < 256; i++) {
      int rc = vp9_default_zig_zag1d_16x16[i];

      quant_val = vp9_ac_yquant(Q);
      cpi->Y1zbin_16x16[Q][rc] = ((qzbin_factor * quant_val) + 64) >> 7;
      cpi->zrun_zbin_boost_y1_16x16[Q][i] =
        ((quant_val * zbin_boost_16x16[i]) + 64) >> 7;

      quant_val = vp9_ac2quant(Q, cpi->common.y2ac_delta_q);
      cpi->Y2zbin_16x16[Q][rc] = ((qzbin_factor * quant_val) + 64) >> 7;
      cpi->zrun_zbin_boost_y2_16x16[Q][i] =
        ((quant_val * zbin_boost_16x16[i]) + 64) >> 7;

      quant_val = vp9_ac_uv_quant(Q, cpi->common.uvac_delta_q);
      cpi->UVzbin_16x16[Q][rc] = ((qzbin_factor * quant_val) + 64) >> 7;
      cpi->zrun_zbin_boost_uv_16x16[Q][i] =
        ((quant_val * zbin_boost_16x16[i]) + 64) >> 7;
    }
    // 32x32 structures. Same comment above applies.
    for (i = 1; i < 1024; i++) {
      int rc = vp9_default_zig_zag1d_32x32[i];

      quant_val = vp9_ac_yquant(Q);
      cpi->Y1zbin_32x32[Q][rc] = ((qzbin_factor * quant_val) + 64) >> 7;
      cpi->zrun_zbin_boost_y1_32x32[Q][i] =
        ((quant_val * zbin_boost_32x32[i]) + 64) >> 7;
    }
  }
}

void vp9_mb_init_quantizer(VP9_COMP *cpi, MACROBLOCK *x) {
  int i;
  int QIndex;
  MACROBLOCKD *xd = &x->e_mbd;
  int zbin_extra;
  int segment_id = xd->mode_info_context->mbmi.segment_id;

  // Select the baseline MB Q index allowing for any segment level change.
  if (vp9_segfeature_active(xd, segment_id, SEG_LVL_ALT_Q)) {
    // Abs Value
    if (xd->mb_segment_abs_delta == SEGMENT_ABSDATA)
      QIndex = vp9_get_segdata(xd, segment_id, SEG_LVL_ALT_Q);

    // Delta Value
    else {
      QIndex = cpi->common.base_qindex +
               vp9_get_segdata(xd, segment_id, SEG_LVL_ALT_Q);

      // Clamp to valid range
      QIndex = (QIndex >= 0) ? ((QIndex <= MAXQ) ? QIndex : MAXQ) : 0;
    }
  } else
    QIndex = cpi->common.base_qindex;

  // Y
  zbin_extra = (cpi->common.Y1dequant[QIndex][1] *
                (cpi->zbin_over_quant +
                 cpi->zbin_mode_boost +
                 x->act_zbin_adj)) >> 7;

  for (i = 0; i < 16; i++) {
    x->block[i].quant = cpi->Y1quant[QIndex];
    x->block[i].quant_shift = cpi->Y1quant_shift[QIndex];
    x->block[i].zbin = cpi->Y1zbin[QIndex];
    x->block[i].zbin_8x8 = cpi->Y1zbin_8x8[QIndex];
    x->block[i].zbin_16x16 = cpi->Y1zbin_16x16[QIndex];
    x->block[i].zbin_32x32 = cpi->Y1zbin_32x32[QIndex];
    x->block[i].round = cpi->Y1round[QIndex];
    x->e_mbd.block[i].dequant = cpi->common.Y1dequant[QIndex];
    x->block[i].zrun_zbin_boost = cpi->zrun_zbin_boost_y1[QIndex];
    x->block[i].zrun_zbin_boost_8x8 = cpi->zrun_zbin_boost_y1_8x8[QIndex];
    x->block[i].zrun_zbin_boost_16x16 = cpi->zrun_zbin_boost_y1_16x16[QIndex];
    x->block[i].zrun_zbin_boost_32x32 = cpi->zrun_zbin_boost_y1_32x32[QIndex];
    x->block[i].zbin_extra = (int16_t)zbin_extra;

    // Segment max eob offset feature.
    if (vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB)) {
      x->block[i].eob_max_offset =
        vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);
      x->block[i].eob_max_offset_8x8 =
        vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);
      x->block[i].eob_max_offset_16x16 =
        vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);
      x->block[i].eob_max_offset_32x32 =
      vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);
    } else {
      x->block[i].eob_max_offset = 16;
      x->block[i].eob_max_offset_8x8 = 64;
      x->block[i].eob_max_offset_16x16 = 256;
      x->block[i].eob_max_offset_32x32 = 1024;
    }
  }

  // UV
  zbin_extra = (cpi->common.UVdequant[QIndex][1] *
                (cpi->zbin_over_quant +
                 cpi->zbin_mode_boost +
                 x->act_zbin_adj)) >> 7;

  for (i = 16; i < 24; i++) {
    x->block[i].quant = cpi->UVquant[QIndex];
    x->block[i].quant_shift = cpi->UVquant_shift[QIndex];
    x->block[i].zbin = cpi->UVzbin[QIndex];
    x->block[i].zbin_8x8 = cpi->UVzbin_8x8[QIndex];
    x->block[i].zbin_16x16 = cpi->UVzbin_16x16[QIndex];
    x->block[i].round = cpi->UVround[QIndex];
    x->e_mbd.block[i].dequant = cpi->common.UVdequant[QIndex];
    x->block[i].zrun_zbin_boost = cpi->zrun_zbin_boost_uv[QIndex];
    x->block[i].zrun_zbin_boost_8x8 = cpi->zrun_zbin_boost_uv_8x8[QIndex];
    x->block[i].zrun_zbin_boost_16x16 = cpi->zrun_zbin_boost_uv_16x16[QIndex];

    x->block[i].zbin_extra = (int16_t)zbin_extra;

    // Segment max eob offset feature.
    if (vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB)) {
      x->block[i].eob_max_offset =
        vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);
      x->block[i].eob_max_offset_8x8 =
        vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);
      x->block[i].eob_max_offset_16x16 =
      vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);
    } else {
      x->block[i].eob_max_offset = 16;
      x->block[i].eob_max_offset_8x8 = 64;
      x->block[i].eob_max_offset_16x16 = 256;
    }
  }

  // Y2
  zbin_extra = (cpi->common.Y2dequant[QIndex][1] *
                ((cpi->zbin_over_quant / 2) +
                 cpi->zbin_mode_boost +
                 x->act_zbin_adj)) >> 7;

  x->block[24].quant = cpi->Y2quant[QIndex];
  x->block[24].quant_shift = cpi->Y2quant_shift[QIndex];
  x->block[24].zbin = cpi->Y2zbin[QIndex];
  x->block[24].zbin_8x8 = cpi->Y2zbin_8x8[QIndex];
  x->block[24].zbin_16x16 = cpi->Y2zbin_16x16[QIndex];
  x->block[24].round = cpi->Y2round[QIndex];
  x->e_mbd.block[24].dequant = cpi->common.Y2dequant[QIndex];
  x->block[24].zrun_zbin_boost = cpi->zrun_zbin_boost_y2[QIndex];
  x->block[24].zrun_zbin_boost_8x8 = cpi->zrun_zbin_boost_y2_8x8[QIndex];
  x->block[24].zrun_zbin_boost_16x16 = cpi->zrun_zbin_boost_y2_16x16[QIndex];
  x->block[24].zbin_extra = (int16_t)zbin_extra;

  // TBD perhaps not use for Y2
  // Segment max eob offset feature.
  if (vp9_segfeature_active(xd, segment_id, SEG_LVL_EOB)) {
    x->block[24].eob_max_offset =
      vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);
    x->block[24].eob_max_offset_8x8 =
      vp9_get_segdata(xd, segment_id, SEG_LVL_EOB);
  } else {
    x->block[24].eob_max_offset = 16;
    x->block[24].eob_max_offset_8x8 = 4;
  }

  /* save this macroblock QIndex for vp9_update_zbin_extra() */
  x->e_mbd.q_index = QIndex;
}

void vp9_update_zbin_extra(VP9_COMP *cpi, MACROBLOCK *x) {
  int i;
  int QIndex = x->e_mbd.q_index;
  int zbin_extra;

  // Y
  zbin_extra = (cpi->common.Y1dequant[QIndex][1] *
                (cpi->zbin_over_quant +
                 cpi->zbin_mode_boost +
                 x->act_zbin_adj)) >> 7;
  for (i = 0; i < 16; i++) {
    x->block[i].zbin_extra = (int16_t)zbin_extra;
  }

  // UV
  zbin_extra = (cpi->common.UVdequant[QIndex][1] *
                (cpi->zbin_over_quant +
                 cpi->zbin_mode_boost +
                 x->act_zbin_adj)) >> 7;

  for (i = 16; i < 24; i++) {
    x->block[i].zbin_extra = (int16_t)zbin_extra;
  }

  // Y2
  zbin_extra = (cpi->common.Y2dequant[QIndex][1] *
                ((cpi->zbin_over_quant / 2) +
                 cpi->zbin_mode_boost +
                 x->act_zbin_adj)) >> 7;

  x->block[24].zbin_extra = (int16_t)zbin_extra;
}

void vp9_frame_init_quantizer(VP9_COMP *cpi) {
  // Clear Zbin mode boost for default case
  cpi->zbin_mode_boost = 0;

  // MB level quantizer setup
  vp9_mb_init_quantizer(cpi, &cpi->mb);
}

void vp9_set_quantizer(struct VP9_COMP *cpi, int Q) {
  VP9_COMMON *cm = &cpi->common;

  cm->base_qindex = Q;

  // if any of the delta_q values are changing update flag will
  // have to be set.
  cm->y1dc_delta_q = 0;
  cm->y2ac_delta_q = 0;
  cm->uvdc_delta_q = 0;
  cm->uvac_delta_q = 0;
  cm->y2dc_delta_q = 0;

  // quantizer has to be reinitialized if any delta_q changes.
  // As there are not any here for now this is inactive code.
  // if(update)
  //    vp9_init_quantizer(cpi);
}
