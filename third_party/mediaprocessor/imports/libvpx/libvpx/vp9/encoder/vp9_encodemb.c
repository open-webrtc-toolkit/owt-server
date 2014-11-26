/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "./vpx_config.h"
#include "vp9/encoder/vp9_encodemb.h"
#include "vp9/common/vp9_reconinter.h"
#include "vp9/encoder/vp9_quantize.h"
#include "vp9/encoder/vp9_tokenize.h"
#include "vp9/common/vp9_invtrans.h"
#include "vp9/common/vp9_reconintra.h"
#include "vpx_mem/vpx_mem.h"
#include "vp9/encoder/vp9_rdopt.h"
#include "vp9/common/vp9_systemdependent.h"
#include "vp9_rtcd.h"

void vp9_subtract_b_c(BLOCK *be, BLOCKD *bd, int pitch) {
  uint8_t *src_ptr = (*(be->base_src) + be->src);
  int16_t *diff_ptr = be->src_diff;
  uint8_t *pred_ptr = bd->predictor;
  int src_stride = be->src_stride;

  int r, c;

  for (r = 0; r < 4; r++) {
    for (c = 0; c < 4; c++)
      diff_ptr[c] = src_ptr[c] - pred_ptr[c];

    diff_ptr += pitch;
    pred_ptr += pitch;
    src_ptr  += src_stride;
  }
}

void vp9_subtract_4b_c(BLOCK *be, BLOCKD *bd, int pitch) {
  uint8_t *src_ptr = (*(be->base_src) + be->src);
  int16_t *diff_ptr = be->src_diff;
  uint8_t *pred_ptr = bd->predictor;
  int src_stride = be->src_stride;
  int r, c;

  for (r = 0; r < 8; r++) {
    for (c = 0; c < 8; c++)
      diff_ptr[c] = src_ptr[c] - pred_ptr[c];

    diff_ptr += pitch;
    pred_ptr += pitch;
    src_ptr  += src_stride;
  }
}

void vp9_subtract_mbuv_s_c(int16_t *diff, const uint8_t *usrc,
                           const uint8_t *vsrc, int src_stride,
                           const uint8_t *upred,
                           const uint8_t *vpred, int dst_stride) {
  int16_t *udiff = diff + 256;
  int16_t *vdiff = diff + 320;
  int r, c;

  for (r = 0; r < 8; r++) {
    for (c = 0; c < 8; c++)
      udiff[c] = usrc[c] - upred[c];

    udiff += 8;
    upred += dst_stride;
    usrc  += src_stride;
  }

  for (r = 0; r < 8; r++) {
    for (c = 0; c < 8; c++) {
      vdiff[c] = vsrc[c] - vpred[c];
    }

    vdiff += 8;
    vpred += dst_stride;
    vsrc  += src_stride;
  }
}

void vp9_subtract_mbuv_c(int16_t *diff, uint8_t *usrc,
                         uint8_t *vsrc, uint8_t *pred, int stride) {
  uint8_t *upred = pred + 256;
  uint8_t *vpred = pred + 320;

  vp9_subtract_mbuv_s_c(diff, usrc, vsrc, stride, upred, vpred, 8);
}

void vp9_subtract_mby_s_c(int16_t *diff, const uint8_t *src, int src_stride,
                          const uint8_t *pred, int dst_stride) {
  int r, c;

  for (r = 0; r < 16; r++) {
    for (c = 0; c < 16; c++)
      diff[c] = src[c] - pred[c];

    diff += 16;
    pred += dst_stride;
    src  += src_stride;
  }
}

void vp9_subtract_sby_s_c(int16_t *diff, const uint8_t *src, int src_stride,
                          const uint8_t *pred, int dst_stride) {
  int r, c;

  for (r = 0; r < 32; r++) {
    for (c = 0; c < 32; c++)
      diff[c] = src[c] - pred[c];

    diff += 32;
    pred += dst_stride;
    src  += src_stride;
  }
}

void vp9_subtract_sbuv_s_c(int16_t *diff, const uint8_t *usrc,
                           const uint8_t *vsrc, int src_stride,
                           const uint8_t *upred,
                           const uint8_t *vpred, int dst_stride) {
  int16_t *udiff = diff + 1024;
  int16_t *vdiff = diff + 1024 + 256;
  int r, c;

  for (r = 0; r < 16; r++) {
    for (c = 0; c < 16; c++)
      udiff[c] = usrc[c] - upred[c];

    udiff += 16;
    upred += dst_stride;
    usrc  += src_stride;
  }

  for (r = 0; r < 16; r++) {
    for (c = 0; c < 16; c++)
      vdiff[c] = vsrc[c] - vpred[c];

    vdiff += 16;
    vpred += dst_stride;
    vsrc  += src_stride;
  }
}

void vp9_subtract_sb64y_s_c(int16_t *diff, const uint8_t *src, int src_stride,
                            const uint8_t *pred, int dst_stride) {
  int r, c;

  for (r = 0; r < 64; r++) {
    for (c = 0; c < 64; c++) {
      diff[c] = src[c] - pred[c];
    }

    diff += 64;
    pred += dst_stride;
    src  += src_stride;
  }
}

void vp9_subtract_sb64uv_s_c(int16_t *diff, const uint8_t *usrc,
                             const uint8_t *vsrc, int src_stride,
                             const uint8_t *upred,
                             const uint8_t *vpred, int dst_stride) {
  int16_t *udiff = diff + 4096;
  int16_t *vdiff = diff + 4096 + 1024;
  int r, c;

  for (r = 0; r < 32; r++) {
    for (c = 0; c < 32; c++) {
      udiff[c] = usrc[c] - upred[c];
    }

    udiff += 32;
    upred += dst_stride;
    usrc  += src_stride;
  }

  for (r = 0; r < 32; r++) {
    for (c = 0; c < 32; c++) {
      vdiff[c] = vsrc[c] - vpred[c];
    }

    vdiff += 32;
    vpred += dst_stride;
    vsrc  += src_stride;
  }
}

void vp9_subtract_mby_c(int16_t *diff, uint8_t *src,
                        uint8_t *pred, int stride) {
  vp9_subtract_mby_s_c(diff, src, stride, pred, 16);
}

static void subtract_mb(MACROBLOCK *x) {
  BLOCK *b = &x->block[0];

  vp9_subtract_mby(x->src_diff, *(b->base_src), x->e_mbd.predictor,
                   b->src_stride);
  vp9_subtract_mbuv(x->src_diff, x->src.u_buffer, x->src.v_buffer,
                    x->e_mbd.predictor, x->src.uv_stride);
}

void vp9_transform_mby_4x4(MACROBLOCK *x) {
  int i;
  MACROBLOCKD *xd = &x->e_mbd;

  for (i = 0; i < 16; i++) {
    BLOCK *b = &x->block[i];
    TX_TYPE tx_type = get_tx_type_4x4(xd, i);
    if (tx_type != DCT_DCT) {
      vp9_short_fht4x4(b->src_diff, b->coeff, 16, tx_type);
    } else if (!(i & 1) && get_tx_type_4x4(xd, i + 1) == DCT_DCT) {
      x->fwd_txm8x4(x->block[i].src_diff, x->block[i].coeff, 32);
      i++;
    } else {
      x->fwd_txm4x4(x->block[i].src_diff, x->block[i].coeff, 32);
    }
  }
}

void vp9_transform_mbuv_4x4(MACROBLOCK *x) {
  int i;

  for (i = 16; i < 24; i += 2)
    x->fwd_txm8x4(x->block[i].src_diff, x->block[i].coeff, 16);
}

static void transform_mb_4x4(MACROBLOCK *x) {
  vp9_transform_mby_4x4(x);
  vp9_transform_mbuv_4x4(x);
}

void vp9_transform_mby_8x8(MACROBLOCK *x) {
  int i;
  MACROBLOCKD *xd = &x->e_mbd;
  TX_TYPE tx_type;

  for (i = 0; i < 9; i += 8) {
    BLOCK *b = &x->block[i];
    tx_type = get_tx_type_8x8(xd, i);
    if (tx_type != DCT_DCT) {
      vp9_short_fht8x8(b->src_diff, b->coeff, 16, tx_type);
    } else {
      x->fwd_txm8x8(x->block[i].src_diff, x->block[i].coeff, 32);
    }
  }
  for (i = 2; i < 11; i += 8) {
    BLOCK *b = &x->block[i];
    tx_type = get_tx_type_8x8(xd, i);
    if (tx_type != DCT_DCT) {
      vp9_short_fht8x8(b->src_diff, (b + 2)->coeff, 16, tx_type);
    } else {
      x->fwd_txm8x8(x->block[i].src_diff, x->block[i + 2].coeff, 32);
    }
  }
}

void vp9_transform_mbuv_8x8(MACROBLOCK *x) {
  int i;

  for (i = 16; i < 24; i += 4)
    x->fwd_txm8x8(x->block[i].src_diff, x->block[i].coeff, 16);
}

void vp9_transform_mb_8x8(MACROBLOCK *x) {
  vp9_transform_mby_8x8(x);
  vp9_transform_mbuv_8x8(x);
}

void vp9_transform_mby_16x16(MACROBLOCK *x) {
  MACROBLOCKD *xd = &x->e_mbd;
  BLOCK *b = &x->block[0];
  TX_TYPE tx_type = get_tx_type_16x16(xd, 0);
  vp9_clear_system_state();
  if (tx_type != DCT_DCT) {
    vp9_short_fht16x16(b->src_diff, b->coeff, 16, tx_type);
  } else {
    x->fwd_txm16x16(x->block[0].src_diff, x->block[0].coeff, 32);
  }
}

void vp9_transform_mb_16x16(MACROBLOCK *x) {
  vp9_transform_mby_16x16(x);
  vp9_transform_mbuv_8x8(x);
}

void vp9_transform_sby_32x32(MACROBLOCK *x) {
  vp9_short_fdct32x32(x->src_diff, x->coeff, 64);
}

void vp9_transform_sby_16x16(MACROBLOCK *x) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int n;

  for (n = 0; n < 4; n++) {
    const int x_idx = n & 1, y_idx = n >> 1;
    const TX_TYPE tx_type = get_tx_type_16x16(xd, (y_idx * 8 + x_idx) * 4);

    if (tx_type != DCT_DCT) {
      vp9_short_fht16x16(x->src_diff + y_idx * 32 * 16 + x_idx * 16,
                         x->coeff + n * 256, 32, tx_type);
    } else {
      x->fwd_txm16x16(x->src_diff + y_idx * 32 * 16 + x_idx * 16,
                      x->coeff + n * 256, 64);
    }
  }
}

void vp9_transform_sby_8x8(MACROBLOCK *x) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int n;

  for (n = 0; n < 16; n++) {
    const int x_idx = n & 3, y_idx = n >> 2;
    const TX_TYPE tx_type = get_tx_type_8x8(xd, (y_idx * 8 + x_idx) * 2);

    if (tx_type != DCT_DCT) {
      vp9_short_fht8x8(x->src_diff + y_idx * 32 * 8 + x_idx * 8,
                       x->coeff + n * 64, 32, tx_type);
    } else {
      x->fwd_txm8x8(x->src_diff + y_idx * 32 * 8 + x_idx * 8,
                    x->coeff + n * 64, 64);
    }
  }
}

void vp9_transform_sby_4x4(MACROBLOCK *x) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int n;

  for (n = 0; n < 64; n++) {
    const int x_idx = n & 7, y_idx = n >> 3;
    const TX_TYPE tx_type = get_tx_type_4x4(xd, y_idx * 8 + x_idx);

    if (tx_type != DCT_DCT) {
      vp9_short_fht4x4(x->src_diff + y_idx * 32 * 4 + x_idx * 4,
                       x->coeff + n * 16, 32, tx_type);
    } else {
      x->fwd_txm4x4(x->src_diff + y_idx * 32 * 4 + x_idx * 4,
                    x->coeff + n * 16, 64);
    }
  }
}

void vp9_transform_sbuv_16x16(MACROBLOCK *x) {
  vp9_clear_system_state();
  x->fwd_txm16x16(x->src_diff + 1024, x->coeff + 1024, 32);
  x->fwd_txm16x16(x->src_diff + 1280, x->coeff + 1280, 32);
}

void vp9_transform_sbuv_8x8(MACROBLOCK *x) {
  int n;

  vp9_clear_system_state();
  for (n = 0; n < 4; n++) {
    const int x_idx = n & 1, y_idx = n >> 1;

    x->fwd_txm8x8(x->src_diff + 1024 + y_idx * 16 * 8 + x_idx * 8,
                  x->coeff + 1024 + n * 64, 32);
    x->fwd_txm8x8(x->src_diff + 1280 + y_idx * 16 * 8 + x_idx * 8,
                  x->coeff + 1280 + n * 64, 32);
  }
}

void vp9_transform_sbuv_4x4(MACROBLOCK *x) {
  int n;

  vp9_clear_system_state();
  for (n = 0; n < 16; n++) {
    const int x_idx = n & 3, y_idx = n >> 2;

    x->fwd_txm4x4(x->src_diff + 1024 + y_idx * 16 * 4 + x_idx * 4,
                  x->coeff + 1024 + n * 16, 32);
    x->fwd_txm4x4(x->src_diff + 1280 + y_idx * 16 * 4 + x_idx * 4,
                  x->coeff + 1280 + n * 16, 32);
  }
}

void vp9_transform_sb64y_32x32(MACROBLOCK *x) {
  int n;

  for (n = 0; n < 4; n++) {
    const int x_idx = n & 1, y_idx = n >> 1;

    vp9_short_fdct32x32(x->src_diff + y_idx * 64 * 32 + x_idx * 32,
                        x->coeff + n * 1024, 128);
  }
}

void vp9_transform_sb64y_16x16(MACROBLOCK *x) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int n;

  for (n = 0; n < 16; n++) {
    const int x_idx = n & 3, y_idx = n >> 2;
    const TX_TYPE tx_type = get_tx_type_16x16(xd, (y_idx * 16 + x_idx) * 4);

    if (tx_type != DCT_DCT) {
      vp9_short_fht16x16(x->src_diff + y_idx * 64 * 16 + x_idx * 16,
                         x->coeff + n * 256, 64, tx_type);
    } else {
      x->fwd_txm16x16(x->src_diff + y_idx * 64 * 16 + x_idx * 16,
                      x->coeff + n * 256, 128);
    }
  }
}

void vp9_transform_sb64y_8x8(MACROBLOCK *x) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int n;

  for (n = 0; n < 64; n++) {
    const int x_idx = n & 7, y_idx = n >> 3;
    const TX_TYPE tx_type = get_tx_type_8x8(xd, (y_idx * 16 + x_idx) * 2);

    if (tx_type != DCT_DCT) {
      vp9_short_fht8x8(x->src_diff + y_idx * 64 * 8 + x_idx * 8,
                         x->coeff + n * 64, 64, tx_type);
    } else {
      x->fwd_txm8x8(x->src_diff + y_idx * 64 * 8 + x_idx * 8,
                    x->coeff + n * 64, 128);
    }
  }
}

void vp9_transform_sb64y_4x4(MACROBLOCK *x) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int n;

  for (n = 0; n < 256; n++) {
    const int x_idx = n & 15, y_idx = n >> 4;
    const TX_TYPE tx_type = get_tx_type_4x4(xd, y_idx * 16 + x_idx);

    if (tx_type != DCT_DCT) {
      vp9_short_fht8x8(x->src_diff + y_idx * 64 * 4 + x_idx * 4,
                       x->coeff + n * 16, 64, tx_type);
    } else {
      x->fwd_txm4x4(x->src_diff + y_idx * 64 * 4 + x_idx * 4,
                    x->coeff + n * 16, 128);
    }
  }
}

void vp9_transform_sb64uv_32x32(MACROBLOCK *x) {
  vp9_clear_system_state();
  vp9_short_fdct32x32(x->src_diff + 4096,
                      x->coeff + 4096, 64);
  vp9_short_fdct32x32(x->src_diff + 4096 + 1024,
                      x->coeff + 4096 + 1024, 64);
}

void vp9_transform_sb64uv_16x16(MACROBLOCK *x) {
  int n;

  vp9_clear_system_state();
  for (n = 0; n < 4; n++) {
    const int x_idx = n & 1, y_idx = n >> 1;

    x->fwd_txm16x16(x->src_diff + 4096 + y_idx * 32 * 16 + x_idx * 16,
                    x->coeff + 4096 + n * 256, 64);
    x->fwd_txm16x16(x->src_diff + 4096 + 1024 + y_idx * 32 * 16 + x_idx * 16,
                    x->coeff + 4096 + 1024 + n * 256, 64);
  }
}

void vp9_transform_sb64uv_8x8(MACROBLOCK *x) {
  int n;

  vp9_clear_system_state();
  for (n = 0; n < 16; n++) {
    const int x_idx = n & 3, y_idx = n >> 2;

    x->fwd_txm8x8(x->src_diff + 4096 + y_idx * 32 * 8 + x_idx * 8,
                  x->coeff + 4096 + n * 64, 64);
    x->fwd_txm8x8(x->src_diff + 4096 + 1024 + y_idx * 32 * 8 + x_idx * 8,
                  x->coeff + 4096 + 1024 + n * 64, 64);
  }
}

void vp9_transform_sb64uv_4x4(MACROBLOCK *x) {
  int n;

  vp9_clear_system_state();
  for (n = 0; n < 64; n++) {
    const int x_idx = n & 7, y_idx = n >> 3;

    x->fwd_txm4x4(x->src_diff + 4096 + y_idx * 32 * 4 + x_idx * 4,
                  x->coeff + 4096 + n * 16, 64);
    x->fwd_txm4x4(x->src_diff + 4096 + 1024 + y_idx * 32 * 4 + x_idx * 4,
                  x->coeff + 4096 + 1024 + n * 16, 64);
  }
}

#define RDTRUNC(RM,DM,R,D) ( (128+(R)*(RM)) & 0xFF )
#define RDTRUNC_8x8(RM,DM,R,D) ( (128+(R)*(RM)) & 0xFF )
typedef struct vp9_token_state vp9_token_state;

struct vp9_token_state {
  int           rate;
  int           error;
  int           next;
  signed char   token;
  short         qc;
};

// TODO: experiments to find optimal multiple numbers
#define Y1_RD_MULT 4
#define UV_RD_MULT 2

static const int plane_rd_mult[4] = {
  Y1_RD_MULT,
  UV_RD_MULT,
};

#define UPDATE_RD_COST()\
{\
  rd_cost0 = RDCOST(rdmult, rddiv, rate0, error0);\
  rd_cost1 = RDCOST(rdmult, rddiv, rate1, error1);\
  if (rd_cost0 == rd_cost1) {\
    rd_cost0 = RDTRUNC(rdmult, rddiv, rate0, error0);\
    rd_cost1 = RDTRUNC(rdmult, rddiv, rate1, error1);\
  }\
}

// This function is a place holder for now but may ultimately need
// to scan previous tokens to work out the correct context.
static int trellis_get_coeff_context(const int *scan,
                                     const int *nb,
                                     int idx, int token,
                                     uint8_t *token_cache,
                                     int pad, int l) {
  int bak = token_cache[idx], pt;
  token_cache[idx] = token;
  pt = vp9_get_coef_context(scan, nb, pad, token_cache, idx + 1, l);
  token_cache[idx] = bak;
  return pt;
}

static void optimize_b(VP9_COMMON *const cm,
                       MACROBLOCK *mb, int ib, PLANE_TYPE type,
                       const int16_t *dequant_ptr,
                       ENTROPY_CONTEXT *a, ENTROPY_CONTEXT *l,
                       int tx_size) {
  const int ref = mb->e_mbd.mode_info_context->mbmi.ref_frame != INTRA_FRAME;
  MACROBLOCKD *const xd = &mb->e_mbd;
  vp9_token_state tokens[1025][2];
  unsigned best_index[1025][2];
  const int16_t *coeff_ptr = mb->coeff + ib * 16;
  int16_t *qcoeff_ptr = xd->qcoeff + ib * 16;
  int16_t *dqcoeff_ptr = xd->dqcoeff + ib * 16;
  int eob = xd->eobs[ib], final_eob, sz = 0;
  const int i0 = 0;
  int rc, x, next, i;
  int64_t rdmult, rddiv, rd_cost0, rd_cost1;
  int rate0, rate1, error0, error1, t0, t1;
  int best, band, pt;
  int err_mult = plane_rd_mult[type];
  int default_eob, pad;
  int const *scan, *nb;
  const int mul = 1 + (tx_size == TX_32X32);
  uint8_t token_cache[1024];
#if CONFIG_CODE_NONZEROCOUNT
  // TODO(debargha): the dynamic programming approach used in this function
  // is not compatible with the true rate cost when nzcs are used. Note
  // the total rate is the sum of the nzc rate and the indicvidual token
  // rates. The latter part can be optimized in this function, but because
  // the nzc rate is a function of all the other tokens without a Markov
  // relationship this rate cannot be considered correctly.
  // The current implementation uses a suboptimal approach to account for
  // the nzc rates somewhat, but in reality the optimization approach needs
  // to change substantially.
  uint16_t nzc = xd->nzcs[ib];
  uint16_t nzc0, nzc1;
  uint16_t final_nzc = 0, final_nzc_exp;
  int nzc_context = vp9_get_nzc_context(cm, xd, ib);
  unsigned int *nzc_cost;
  nzc0 = nzc1 = nzc;
#endif

  switch (tx_size) {
    default:
    case TX_4X4: {
      const TX_TYPE tx_type = get_tx_type_4x4(xd, ib);
      default_eob = 16;
#if CONFIG_CODE_NONZEROCOUNT
      nzc_cost = mb->nzc_costs_4x4[nzc_context][ref][type];
#endif
      if (tx_type == DCT_ADST) {
        scan = vp9_col_scan_4x4;
      } else if (tx_type == ADST_DCT) {
        scan = vp9_row_scan_4x4;
      } else {
        scan = vp9_default_zig_zag1d_4x4;
      }
      break;
    }
    case TX_8X8: {
      const BLOCK_SIZE_TYPE sb_type = xd->mode_info_context->mbmi.sb_type;
      const int sz = 3 + sb_type, x = ib & ((1 << sz) - 1), y = ib - x;
      const TX_TYPE tx_type = get_tx_type_8x8(xd, y + (x >> 1));
      if (tx_type == DCT_ADST) {
        scan = vp9_col_scan_8x8;
      } else if (tx_type == ADST_DCT) {
        scan = vp9_row_scan_8x8;
      } else {
        scan = vp9_default_zig_zag1d_8x8;
      }
      default_eob = 64;
#if CONFIG_CODE_NONZEROCOUNT
      nzc_cost = mb->nzc_costs_8x8[nzc_context][ref][type];
#endif
      break;
    }
    case TX_16X16: {
      const BLOCK_SIZE_TYPE sb_type = xd->mode_info_context->mbmi.sb_type;
      const int sz = 4 + sb_type, x = ib & ((1 << sz) - 1), y = ib - x;
      const TX_TYPE tx_type = get_tx_type_16x16(xd, y + (x >> 2));
      if (tx_type == DCT_ADST) {
        scan = vp9_col_scan_16x16;
      } else if (tx_type == ADST_DCT) {
        scan = vp9_row_scan_16x16;
      } else {
        scan = vp9_default_zig_zag1d_16x16;
      }
      default_eob = 256;
#if CONFIG_CODE_NONZEROCOUNT
      nzc_cost = mb->nzc_costs_16x16[nzc_context][ref][type];
#endif
      break;
    }
    case TX_32X32:
      scan = vp9_default_zig_zag1d_32x32;
      default_eob = 1024;
#if CONFIG_CODE_NONZEROCOUNT
      nzc_cost = mb->nzc_costs_32x32[nzc_context][ref][type];
#endif
      break;
  }

  /* Now set up a Viterbi trellis to evaluate alternative roundings. */
  rdmult = mb->rdmult * err_mult;
  if (mb->e_mbd.mode_info_context->mbmi.ref_frame == INTRA_FRAME)
    rdmult = (rdmult * 9) >> 4;
  rddiv = mb->rddiv;
  memset(best_index, 0, sizeof(best_index));
  /* Initialize the sentinel node of the trellis. */
#if CONFIG_CODE_NONZEROCOUNT
  tokens[eob][0].rate = nzc_cost[nzc];
#else
  tokens[eob][0].rate = 0;
#endif
  tokens[eob][0].error = 0;
  tokens[eob][0].next = default_eob;
  tokens[eob][0].token = DCT_EOB_TOKEN;
  tokens[eob][0].qc = 0;
  *(tokens[eob] + 1) = *(tokens[eob] + 0);
  next = eob;
  for (i = 0; i < eob; i++)
    token_cache[i] = vp9_dct_value_tokens_ptr[qcoeff_ptr[scan[i]]].Token;
  nb = vp9_get_coef_neighbors_handle(scan, &pad);

  for (i = eob; i-- > i0;) {
    int base_bits, d2, dx;
#if CONFIG_CODE_NONZEROCOUNT
    int new_nzc0, new_nzc1;
#endif

    rc = scan[i];
    x = qcoeff_ptr[rc];
    /* Only add a trellis state for non-zero coefficients. */
    if (x) {
      int shortcut = 0;
      error0 = tokens[next][0].error;
      error1 = tokens[next][1].error;
      /* Evaluate the first possibility for this state. */
      rate0 = tokens[next][0].rate;
      rate1 = tokens[next][1].rate;
      t0 = (vp9_dct_value_tokens_ptr + x)->Token;
      /* Consider both possible successor states. */
      if (next < default_eob) {
        band = get_coef_band(scan, tx_size, i + 1);
        pt = trellis_get_coeff_context(scan, nb, i, t0, token_cache,
                                       pad, default_eob);
        rate0 +=
          mb->token_costs[tx_size][type][ref][band][pt][tokens[next][0].token];
        rate1 +=
          mb->token_costs[tx_size][type][ref][band][pt][tokens[next][1].token];
      }
      UPDATE_RD_COST();
      /* And pick the best. */
      best = rd_cost1 < rd_cost0;
      base_bits = *(vp9_dct_value_cost_ptr + x);
      dx = mul * (dqcoeff_ptr[rc] - coeff_ptr[rc]);
      d2 = dx * dx;
      tokens[i][0].rate = base_bits + (best ? rate1 : rate0);
      tokens[i][0].error = d2 + (best ? error1 : error0);
      tokens[i][0].next = next;
      tokens[i][0].token = t0;
      tokens[i][0].qc = x;
      best_index[i][0] = best;
#if CONFIG_CODE_NONZEROCOUNT
      new_nzc0 = (best ? nzc1 : nzc0);
#endif

      /* Evaluate the second possibility for this state. */
      rate0 = tokens[next][0].rate;
      rate1 = tokens[next][1].rate;

      if ((abs(x)*dequant_ptr[rc != 0] > abs(coeff_ptr[rc]) * mul) &&
          (abs(x)*dequant_ptr[rc != 0] < abs(coeff_ptr[rc]) * mul +
                                         dequant_ptr[rc != 0]))
        shortcut = 1;
      else
        shortcut = 0;

      if (shortcut) {
        sz = -(x < 0);
        x -= 2 * sz + 1;
      }

      /* Consider both possible successor states. */
      if (!x) {
        /* If we reduced this coefficient to zero, check to see if
         *  we need to move the EOB back here.
         */
        t0 = tokens[next][0].token == DCT_EOB_TOKEN ?
             DCT_EOB_TOKEN : ZERO_TOKEN;
        t1 = tokens[next][1].token == DCT_EOB_TOKEN ?
             DCT_EOB_TOKEN : ZERO_TOKEN;
#if CONFIG_CODE_NONZEROCOUNT
        // Account for rate drop because of the nzc change.
        // TODO(debargha): Find a better solution
        rate0 -= nzc_cost[nzc0] - nzc_cost[nzc0 - 1];
        rate1 -= nzc_cost[nzc1] - nzc_cost[nzc1 - 1];
#endif
      } else {
        t0 = t1 = (vp9_dct_value_tokens_ptr + x)->Token;
      }
      if (next < default_eob) {
        band = get_coef_band(scan, tx_size, i + 1);
        if (t0 != DCT_EOB_TOKEN) {
          pt = trellis_get_coeff_context(scan, nb, i, t0, token_cache,
                                         pad, default_eob);
          rate0 += mb->token_costs[tx_size][type][ref][band][pt][
              tokens[next][0].token];
        }
        if (t1 != DCT_EOB_TOKEN) {
          pt = trellis_get_coeff_context(scan, nb, i, t1, token_cache,
                                         pad, default_eob);
          rate1 += mb->token_costs[tx_size][type][ref][band][pt][
              tokens[next][1].token];
        }
      }

      UPDATE_RD_COST();
      /* And pick the best. */
      best = rd_cost1 < rd_cost0;
      base_bits = *(vp9_dct_value_cost_ptr + x);

      if (shortcut) {
        dx -= (dequant_ptr[rc != 0] + sz) ^ sz;
        d2 = dx * dx;
      }
      tokens[i][1].rate = base_bits + (best ? rate1 : rate0);
      tokens[i][1].error = d2 + (best ? error1 : error0);
      tokens[i][1].next = next;
      tokens[i][1].token = best ? t1 : t0;
      tokens[i][1].qc = x;
      best_index[i][1] = best;
#if CONFIG_CODE_NONZEROCOUNT
      new_nzc1 = (best ? nzc1 : nzc0) - (!x);
      nzc0 = new_nzc0;
      nzc1 = new_nzc1;
#endif
      /* Finally, make this the new head of the trellis. */
      next = i;
    }
    /* There's no choice to make for a zero coefficient, so we don't
     *  add a new trellis node, but we do need to update the costs.
     */
    else {
      band = get_coef_band(scan, tx_size, i + 1);
      t0 = tokens[next][0].token;
      t1 = tokens[next][1].token;
      /* Update the cost of each path if we're past the EOB token. */
      if (t0 != DCT_EOB_TOKEN) {
        tokens[next][0].rate +=
            mb->token_costs[tx_size][type][ref][band][0][t0];
        tokens[next][0].token = ZERO_TOKEN;
      }
      if (t1 != DCT_EOB_TOKEN) {
        tokens[next][1].rate +=
            mb->token_costs[tx_size][type][ref][band][0][t1];
        tokens[next][1].token = ZERO_TOKEN;
      }
      /* Don't update next, because we didn't add a new node. */
    }
  }

  /* Now pick the best path through the whole trellis. */
  band = get_coef_band(scan, tx_size, i + 1);
  VP9_COMBINEENTROPYCONTEXTS(pt, *a, *l);
  rate0 = tokens[next][0].rate;
  rate1 = tokens[next][1].rate;
  error0 = tokens[next][0].error;
  error1 = tokens[next][1].error;
  t0 = tokens[next][0].token;
  t1 = tokens[next][1].token;
  rate0 += mb->token_costs[tx_size][type][ref][band][pt][t0];
  rate1 += mb->token_costs[tx_size][type][ref][band][pt][t1];
  UPDATE_RD_COST();
  best = rd_cost1 < rd_cost0;
#if CONFIG_CODE_NONZEROCOUNT
  final_nzc_exp = (best ? nzc1 : nzc0);
#endif
  final_eob = i0 - 1;
  for (i = next; i < eob; i = next) {
    x = tokens[i][best].qc;
    if (x) {
      final_eob = i;
#if CONFIG_CODE_NONZEROCOUNT
      ++final_nzc;
#endif
    }
    rc = scan[i];
    qcoeff_ptr[rc] = x;
    dqcoeff_ptr[rc] = (x * dequant_ptr[rc != 0]) / mul;

    next = tokens[i][best].next;
    best = best_index[i][best];
  }
  final_eob++;

  xd->eobs[ib] = final_eob;
  *a = *l = (final_eob > 0);
#if CONFIG_CODE_NONZEROCOUNT
  assert(final_nzc == final_nzc_exp);
  xd->nzcs[ib] = final_nzc;
#endif
}

void vp9_optimize_mby_4x4(VP9_COMMON *const cm, MACROBLOCK *x) {
  int b;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta;
  ENTROPY_CONTEXT *tl;

  if (!x->e_mbd.above_context || !x->e_mbd.left_context)
    return;

  vpx_memcpy(&t_above, x->e_mbd.above_context, sizeof(ENTROPY_CONTEXT_PLANES));
  vpx_memcpy(&t_left, x->e_mbd.left_context, sizeof(ENTROPY_CONTEXT_PLANES));

  ta = (ENTROPY_CONTEXT *)&t_above;
  tl = (ENTROPY_CONTEXT *)&t_left;

  for (b = 0; b < 16; b++) {
    optimize_b(cm, x, b, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[b].dequant,
               ta + vp9_block2above[TX_4X4][b],
               tl + vp9_block2left[TX_4X4][b], TX_4X4);
  }
}

void vp9_optimize_mbuv_4x4(VP9_COMMON *const cm, MACROBLOCK *x) {
  int b;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta;
  ENTROPY_CONTEXT *tl;

  if (!x->e_mbd.above_context || !x->e_mbd.left_context)
    return;

  vpx_memcpy(&t_above, x->e_mbd.above_context, sizeof(ENTROPY_CONTEXT_PLANES));
  vpx_memcpy(&t_left, x->e_mbd.left_context, sizeof(ENTROPY_CONTEXT_PLANES));

  ta = (ENTROPY_CONTEXT *)&t_above;
  tl = (ENTROPY_CONTEXT *)&t_left;

  for (b = 16; b < 24; b++) {
    optimize_b(cm, x, b, PLANE_TYPE_UV, x->e_mbd.block[b].dequant,
               ta + vp9_block2above[TX_4X4][b],
               tl + vp9_block2left[TX_4X4][b], TX_4X4);
  }
}

static void optimize_mb_4x4(VP9_COMMON *const cm, MACROBLOCK *x) {
  vp9_optimize_mby_4x4(cm, x);
  vp9_optimize_mbuv_4x4(cm, x);
}

void vp9_optimize_mby_8x8(VP9_COMMON *const cm, MACROBLOCK *x) {
  int b;
  ENTROPY_CONTEXT_PLANES t_above, t_left;
  ENTROPY_CONTEXT *ta;
  ENTROPY_CONTEXT *tl;

  if (!x->e_mbd.above_context || !x->e_mbd.left_context)
    return;

  vpx_memcpy(&t_above, x->e_mbd.above_context, sizeof(ENTROPY_CONTEXT_PLANES));
  vpx_memcpy(&t_left, x->e_mbd.left_context, sizeof(ENTROPY_CONTEXT_PLANES));

  ta = (ENTROPY_CONTEXT *)&t_above;
  tl = (ENTROPY_CONTEXT *)&t_left;
  for (b = 0; b < 16; b += 4) {
    ENTROPY_CONTEXT *const a = ta + vp9_block2above[TX_8X8][b];
    ENTROPY_CONTEXT *const l = tl + vp9_block2left[TX_8X8][b];
    ENTROPY_CONTEXT above_ec = (a[0] + a[1]) != 0;
    ENTROPY_CONTEXT left_ec = (l[0] + l[1]) != 0;
    optimize_b(cm, x, b, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[b].dequant,
               &above_ec, &left_ec, TX_8X8);
    a[1] = a[0] = above_ec;
    l[1] = l[0] = left_ec;
  }
}

void vp9_optimize_mbuv_8x8(VP9_COMMON *const cm, MACROBLOCK *x) {
  int b;
  ENTROPY_CONTEXT *const ta = (ENTROPY_CONTEXT *)x->e_mbd.above_context;
  ENTROPY_CONTEXT *const tl = (ENTROPY_CONTEXT *)x->e_mbd.left_context;

  if (!ta || !tl)
    return;

  for (b = 16; b < 24; b += 4) {
    ENTROPY_CONTEXT *const a = ta + vp9_block2above[TX_8X8][b];
    ENTROPY_CONTEXT *const l = tl + vp9_block2left[TX_8X8][b];
    ENTROPY_CONTEXT above_ec = (a[0] + a[1]) != 0;
    ENTROPY_CONTEXT left_ec = (l[0] + l[1]) != 0;
    optimize_b(cm, x, b, PLANE_TYPE_UV, x->e_mbd.block[b].dequant,
               &above_ec, &left_ec, TX_8X8);
  }
}

static void optimize_mb_8x8(VP9_COMMON *const cm, MACROBLOCK *x) {
  vp9_optimize_mby_8x8(cm, x);
  vp9_optimize_mbuv_8x8(cm, x);
}

void vp9_optimize_mby_16x16(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT_PLANES *const t_above = x->e_mbd.above_context;
  ENTROPY_CONTEXT_PLANES *const t_left = x->e_mbd.left_context;
  ENTROPY_CONTEXT ta, tl;

  if (!t_above || !t_left)
    return;

  ta = (t_above->y1[0] + t_above->y1[1] + t_above->y1[2] + t_above->y1[3]) != 0;
  tl = (t_left->y1[0] + t_left->y1[1] + t_left->y1[2] + t_left->y1[3]) != 0;
  optimize_b(cm, x, 0, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[0].dequant,
             &ta, &tl, TX_16X16);
}

static void optimize_mb_16x16(VP9_COMMON *const cm, MACROBLOCK *x) {
  vp9_optimize_mby_16x16(cm, x);
  vp9_optimize_mbuv_8x8(cm, x);
}

void vp9_optimize_sby_32x32(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT *a = (ENTROPY_CONTEXT *) x->e_mbd.above_context;
  ENTROPY_CONTEXT *a1 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 1);
  ENTROPY_CONTEXT *l = (ENTROPY_CONTEXT *) x->e_mbd.left_context;
  ENTROPY_CONTEXT *l1 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 1);
  ENTROPY_CONTEXT ta, tl;

  ta = (a[0] + a[1] + a[2] + a[3] + a1[0] + a1[1] + a1[2] + a1[3]) != 0;
  tl = (l[0] + l[1] + l[2] + l[3] + l1[0] + l1[1] + l1[2] + l1[3]) != 0;
  optimize_b(cm, x, 0, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[0].dequant,
             &ta, &tl, TX_32X32);
}

void vp9_optimize_sby_16x16(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT *a = (ENTROPY_CONTEXT *) x->e_mbd.above_context;
  ENTROPY_CONTEXT *a1 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 1);
  ENTROPY_CONTEXT *l = (ENTROPY_CONTEXT *) x->e_mbd.left_context;
  ENTROPY_CONTEXT *l1 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 1);
  ENTROPY_CONTEXT ta[2], tl[2];
  int n;

  ta[0] = (a[0] + a[1] + a[2] + a[3]) != 0;
  ta[1] = (a1[0] + a1[1] + a1[2] + a1[3]) != 0;
  tl[0] = (l[0] + l[1] + l[2] + l[3]) != 0;
  tl[1] = (l1[0] + l1[1] + l1[2] + l1[3]) != 0;
  for (n = 0; n < 4; n++) {
    const int x_idx = n & 1, y_idx = n >> 1;

    optimize_b(cm, x, n * 16, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[0].dequant,
               ta + x_idx, tl + y_idx, TX_16X16);
  }
}

void vp9_optimize_sby_8x8(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT *a = (ENTROPY_CONTEXT *) x->e_mbd.above_context;
  ENTROPY_CONTEXT *a1 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 1);
  ENTROPY_CONTEXT *l = (ENTROPY_CONTEXT *) x->e_mbd.left_context;
  ENTROPY_CONTEXT *l1 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 1);
  ENTROPY_CONTEXT ta[4], tl[4];
  int n;

  ta[0] = (a[0] + a[1]) != 0;
  ta[1] = (a[2] + a[3]) != 0;
  ta[2] = (a1[0] + a1[1]) != 0;
  ta[3] = (a1[2] + a1[3]) != 0;
  tl[0] = (l[0] + l[1]) != 0;
  tl[1] = (l[2] + l[3]) != 0;
  tl[2] = (l1[0] + l1[1]) != 0;
  tl[3] = (l1[2] + l1[3]) != 0;
  for (n = 0; n < 16; n++) {
    const int x_idx = n & 3, y_idx = n >> 2;

    optimize_b(cm, x, n * 4, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[0].dequant,
               ta + x_idx, tl + y_idx, TX_8X8);
  }
}

void vp9_optimize_sby_4x4(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT ta[8], tl[8];
  int n;

  vpx_memcpy(ta, x->e_mbd.above_context, 4 * sizeof(ENTROPY_CONTEXT));
  vpx_memcpy(ta + 4, x->e_mbd.above_context + 1, 4 * sizeof(ENTROPY_CONTEXT));
  vpx_memcpy(tl, x->e_mbd.left_context, 4 * sizeof(ENTROPY_CONTEXT));
  vpx_memcpy(tl + 4, x->e_mbd.left_context + 1, 4 * sizeof(ENTROPY_CONTEXT));
  for (n = 0; n < 64; n++) {
    const int x_idx = n & 7, y_idx = n >> 3;

    optimize_b(cm, x, n, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[0].dequant,
               ta + x_idx, tl + y_idx, TX_4X4);
  }
}

void vp9_optimize_sbuv_16x16(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT *ta = (ENTROPY_CONTEXT *) x->e_mbd.above_context;
  ENTROPY_CONTEXT *tl = (ENTROPY_CONTEXT *) x->e_mbd.left_context;
  ENTROPY_CONTEXT *a, *l, *a1, *l1, above_ec, left_ec;
  int b;

  for (b = 64; b < 96; b += 16) {
    const int cidx = b >= 80 ? 20 : 16;
    a = ta + vp9_block2above_sb[TX_16X16][b];
    l = tl + vp9_block2left_sb[TX_16X16][b];
    a1 = a + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l1 = l + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    above_ec = (a[0] + a[1] + a1[0] + a1[1]) != 0;
    left_ec = (l[0] + l[1] + l1[0] + l1[1]) != 0;
    optimize_b(cm, x, b, PLANE_TYPE_UV, x->e_mbd.block[cidx].dequant,
               &above_ec, &left_ec, TX_16X16);
  }
}

void vp9_optimize_sbuv_8x8(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT_PLANES t_above[2], t_left[2];
  ENTROPY_CONTEXT *ta = (ENTROPY_CONTEXT *) t_above;
  ENTROPY_CONTEXT *tl = (ENTROPY_CONTEXT *) t_left;
  ENTROPY_CONTEXT *a, *l, above_ec, left_ec;
  int b;

  vpx_memcpy(t_above, x->e_mbd.above_context, sizeof(t_above));
  vpx_memcpy(t_left, x->e_mbd.left_context, sizeof(t_left));
  for (b = 64; b < 96; b += 4) {
    const int cidx = b >= 80 ? 20 : 16;
    a = ta + vp9_block2above_sb[TX_8X8][b];
    l = tl + vp9_block2left_sb[TX_8X8][b];
    above_ec = (a[0] + a[1]) != 0;
    left_ec = (l[0] + l[1]) != 0;
    optimize_b(cm, x, b, PLANE_TYPE_UV, x->e_mbd.block[cidx].dequant,
               &above_ec, &left_ec, TX_8X8);
    a[0] = a[1] = above_ec;
    l[0] = l[1] = left_ec;
  }
}

void vp9_optimize_sbuv_4x4(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT_PLANES t_above[2], t_left[2];
  ENTROPY_CONTEXT *ta = (ENTROPY_CONTEXT *) t_above;
  ENTROPY_CONTEXT *tl = (ENTROPY_CONTEXT *) t_left;
  ENTROPY_CONTEXT *a, *l;
  int b;

  vpx_memcpy(t_above, x->e_mbd.above_context, sizeof(t_above));
  vpx_memcpy(t_left, x->e_mbd.left_context, sizeof(t_left));
  for (b = 64; b < 96; b++) {
    const int cidx = b >= 80 ? 20 : 16;
    a = ta + vp9_block2above_sb[TX_4X4][b];
    l = tl + vp9_block2left_sb[TX_4X4][b];
    optimize_b(cm, x, b, PLANE_TYPE_UV, x->e_mbd.block[cidx].dequant,
               a, l, TX_4X4);
  }
}

void vp9_optimize_sb64y_32x32(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT *a = (ENTROPY_CONTEXT *) x->e_mbd.above_context;
  ENTROPY_CONTEXT *a1 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 1);
  ENTROPY_CONTEXT *a2 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 2);
  ENTROPY_CONTEXT *a3 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 3);
  ENTROPY_CONTEXT *l = (ENTROPY_CONTEXT *) x->e_mbd.left_context;
  ENTROPY_CONTEXT *l1 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 1);
  ENTROPY_CONTEXT *l2 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 2);
  ENTROPY_CONTEXT *l3 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 3);
  ENTROPY_CONTEXT ta[2], tl[2];
  int n;

  ta[0] = (a[0] + a[1] + a[2] + a[3] + a1[0] + a1[1] + a1[2] + a1[3]) != 0;
  ta[1] = (a2[0] + a2[1] + a2[2] + a2[3] + a3[0] + a3[1] + a3[2] + a3[3]) != 0;
  tl[0] = (l[0] + l[1] + l[2] + l[3] + l1[0] + l1[1] + l1[2] + l1[3]) != 0;
  tl[1] = (l2[0] + l2[1] + l2[2] + l2[3] + l3[0] + l3[1] + l3[2] + l3[3]) != 0;
  for (n = 0; n < 4; n++) {
    const int x_idx = n & 1, y_idx = n >> 1;

    optimize_b(cm, x, n * 64, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[0].dequant,
               ta + x_idx, tl + y_idx, TX_32X32);
  }
}

void vp9_optimize_sb64y_16x16(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT *a = (ENTROPY_CONTEXT *) x->e_mbd.above_context;
  ENTROPY_CONTEXT *a1 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 1);
  ENTROPY_CONTEXT *a2 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 2);
  ENTROPY_CONTEXT *a3 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 3);
  ENTROPY_CONTEXT *l = (ENTROPY_CONTEXT *) x->e_mbd.left_context;
  ENTROPY_CONTEXT *l1 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 1);
  ENTROPY_CONTEXT *l2 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 2);
  ENTROPY_CONTEXT *l3 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 3);
  ENTROPY_CONTEXT ta[4], tl[4];
  int n;

  ta[0] = (a[0] + a[1] + a[2] + a[3]) != 0;
  ta[1] = (a1[0] + a1[1] + a1[2] + a1[3]) != 0;
  ta[2] = (a2[0] + a2[1] + a2[2] + a2[3]) != 0;
  ta[3] = (a3[0] + a3[1] + a3[2] + a3[3]) != 0;
  tl[0] = (l[0] + l[1] + l[2] + l[3]) != 0;
  tl[1] = (l1[0] + l1[1] + l1[2] + l1[3]) != 0;
  tl[2] = (l2[0] + l2[1] + l2[2] + l2[3]) != 0;
  tl[3] = (l3[0] + l3[1] + l3[2] + l3[3]) != 0;
  for (n = 0; n < 16; n++) {
    const int x_idx = n & 3, y_idx = n >> 2;

    optimize_b(cm, x, n * 16, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[0].dequant,
               ta + x_idx, tl + y_idx, TX_16X16);
  }
}

void vp9_optimize_sb64y_8x8(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT *a = (ENTROPY_CONTEXT *) x->e_mbd.above_context;
  ENTROPY_CONTEXT *a1 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 1);
  ENTROPY_CONTEXT *a2 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 2);
  ENTROPY_CONTEXT *a3 = (ENTROPY_CONTEXT *) (x->e_mbd.above_context + 3);
  ENTROPY_CONTEXT *l = (ENTROPY_CONTEXT *) x->e_mbd.left_context;
  ENTROPY_CONTEXT *l1 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 1);
  ENTROPY_CONTEXT *l2 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 2);
  ENTROPY_CONTEXT *l3 = (ENTROPY_CONTEXT *) (x->e_mbd.left_context + 3);
  ENTROPY_CONTEXT ta[8], tl[8];
  int n;

  ta[0] = (a[0] + a[1]) != 0;
  ta[1] = (a[2] + a[3]) != 0;
  ta[2] = (a1[0] + a1[1]) != 0;
  ta[3] = (a1[2] + a1[3]) != 0;
  ta[4] = (a2[0] + a2[1]) != 0;
  ta[5] = (a2[2] + a2[3]) != 0;
  ta[6] = (a3[0] + a3[1]) != 0;
  ta[7] = (a3[2] + a3[3]) != 0;
  tl[0] = (l[0] + l[1]) != 0;
  tl[1] = (l[2] + l[3]) != 0;
  tl[2] = (l1[0] + l1[1]) != 0;
  tl[3] = (l1[2] + l1[3]) != 0;
  tl[4] = (l2[0] + l2[1]) != 0;
  tl[5] = (l2[2] + l2[3]) != 0;
  tl[6] = (l3[0] + l3[1]) != 0;
  tl[7] = (l3[2] + l3[3]) != 0;
  for (n = 0; n < 64; n++) {
    const int x_idx = n & 7, y_idx = n >> 3;

    optimize_b(cm, x, n * 4, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[0].dequant,
               ta + x_idx, tl + y_idx, TX_8X8);
  }
}

void vp9_optimize_sb64y_4x4(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT ta[16], tl[16];
  int n;

  vpx_memcpy(ta, x->e_mbd.above_context, 4 * sizeof(ENTROPY_CONTEXT));
  vpx_memcpy(ta + 4, x->e_mbd.above_context + 1, 4 * sizeof(ENTROPY_CONTEXT));
  vpx_memcpy(ta + 8, x->e_mbd.above_context + 2, 4 * sizeof(ENTROPY_CONTEXT));
  vpx_memcpy(ta + 12, x->e_mbd.above_context + 3, 4 * sizeof(ENTROPY_CONTEXT));
  vpx_memcpy(tl, x->e_mbd.left_context, 4 * sizeof(ENTROPY_CONTEXT));
  vpx_memcpy(tl + 4, x->e_mbd.left_context + 1, 4 * sizeof(ENTROPY_CONTEXT));
  vpx_memcpy(tl + 8, x->e_mbd.left_context + 2, 4 * sizeof(ENTROPY_CONTEXT));
  vpx_memcpy(tl + 12, x->e_mbd.left_context + 3, 4 * sizeof(ENTROPY_CONTEXT));
  for (n = 0; n < 256; n++) {
    const int x_idx = n & 15, y_idx = n >> 4;

    optimize_b(cm, x, n, PLANE_TYPE_Y_WITH_DC, x->e_mbd.block[0].dequant,
               ta + x_idx, tl + y_idx, TX_4X4);
  }
}

void vp9_optimize_sb64uv_32x32(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT *ta = (ENTROPY_CONTEXT *) x->e_mbd.above_context;
  ENTROPY_CONTEXT *tl = (ENTROPY_CONTEXT *) x->e_mbd.left_context;
  ENTROPY_CONTEXT *a, *l, *a1, *l1, *a2, *l2, *a3, *l3, a_ec, l_ec;
  int b;

  for (b = 256; b < 384; b += 64) {
    const int cidx = b >= 320 ? 20 : 16;
    a = ta + vp9_block2above_sb64[TX_32X32][b];
    l = tl + vp9_block2left_sb64[TX_32X32][b];
    a1 = a + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l1 = l + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    a2 = a + 2 * sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l2 = l + 2 * sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    a3 = a + 3 * sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l3 = l + 3 * sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    a_ec = (a[0] + a[1] + a1[0] + a1[1] + a2[0] + a2[1] + a3[0] + a3[1]) != 0;
    l_ec = (l[0] + l[1] + l1[0] + l1[1] + l2[0] + l2[1] + l3[0] + l3[1]) != 0;
    optimize_b(cm, x, b, PLANE_TYPE_UV, x->e_mbd.block[cidx].dequant,
               &a_ec, &l_ec, TX_32X32);
  }
}

void vp9_optimize_sb64uv_16x16(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT_PLANES t_above[4], t_left[4];
  ENTROPY_CONTEXT *ta = (ENTROPY_CONTEXT *) t_above;
  ENTROPY_CONTEXT *tl = (ENTROPY_CONTEXT *) t_left;
  ENTROPY_CONTEXT *a, *l, *a1, *l1, above_ec, left_ec;
  int b;

  vpx_memcpy(t_above, x->e_mbd.above_context, sizeof(t_above));
  vpx_memcpy(t_left, x->e_mbd.left_context, sizeof(t_left));
  for (b = 256; b < 384; b += 16) {
    const int cidx = b >= 320 ? 20 : 16;
    a = ta + vp9_block2above_sb64[TX_16X16][b];
    l = tl + vp9_block2left_sb64[TX_16X16][b];
    a1 = a + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    l1 = l + sizeof(ENTROPY_CONTEXT_PLANES) / sizeof(ENTROPY_CONTEXT);
    above_ec = (a[0] + a[1] + a1[0] + a1[1]) != 0;
    left_ec = (l[0] + l[1] + l1[0] + l1[1]) != 0;
    optimize_b(cm, x, b, PLANE_TYPE_UV, x->e_mbd.block[cidx].dequant,
               &above_ec, &left_ec, TX_16X16);
    a[0] = a[1] = a1[0] = a1[1] = above_ec;
    l[0] = l[1] = l1[0] = l1[1] = left_ec;
  }
}

void vp9_optimize_sb64uv_8x8(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT_PLANES t_above[4], t_left[4];
  ENTROPY_CONTEXT *ta = (ENTROPY_CONTEXT *) t_above;
  ENTROPY_CONTEXT *tl = (ENTROPY_CONTEXT *) t_left;
  ENTROPY_CONTEXT *a, *l, above_ec, left_ec;
  int b;

  vpx_memcpy(t_above, x->e_mbd.above_context, sizeof(t_above));
  vpx_memcpy(t_left, x->e_mbd.left_context, sizeof(t_left));
  for (b = 256; b < 384; b += 4) {
    const int cidx = b >= 320 ? 20 : 16;
    a = ta + vp9_block2above_sb64[TX_8X8][b];
    l = tl + vp9_block2left_sb64[TX_8X8][b];
    above_ec = (a[0] + a[1]) != 0;
    left_ec = (l[0] + l[1]) != 0;
    optimize_b(cm, x, b, PLANE_TYPE_UV, x->e_mbd.block[cidx].dequant,
               &above_ec, &left_ec, TX_8X8);
    a[0] = a[1] = above_ec;
    l[0] = l[1] = left_ec;
  }
}

void vp9_optimize_sb64uv_4x4(VP9_COMMON *const cm, MACROBLOCK *x) {
  ENTROPY_CONTEXT_PLANES t_above[4], t_left[4];
  ENTROPY_CONTEXT *ta = (ENTROPY_CONTEXT *) t_above;
  ENTROPY_CONTEXT *tl = (ENTROPY_CONTEXT *) t_left;
  ENTROPY_CONTEXT *a, *l;
  int b;

  vpx_memcpy(t_above, x->e_mbd.above_context, sizeof(t_above));
  vpx_memcpy(t_left, x->e_mbd.left_context, sizeof(t_left));
  for (b = 256; b < 384; b++) {
    const int cidx = b >= 320 ? 20 : 16;
    a = ta + vp9_block2above_sb64[TX_4X4][b];
    l = tl + vp9_block2left_sb64[TX_4X4][b];
    optimize_b(cm, x, b, PLANE_TYPE_UV, x->e_mbd.block[cidx].dequant,
               a, l, TX_4X4);
  }
}

void vp9_fidct_mb(VP9_COMMON *const cm, MACROBLOCK *x) {
  MACROBLOCKD *const xd = &x->e_mbd;
  TX_SIZE tx_size = xd->mode_info_context->mbmi.txfm_size;

  if (tx_size == TX_16X16) {
    vp9_transform_mb_16x16(x);
    vp9_quantize_mb_16x16(x);
    if (x->optimize)
      optimize_mb_16x16(cm, x);
    vp9_inverse_transform_mb_16x16(xd);
  } else if (tx_size == TX_8X8) {
    if (xd->mode_info_context->mbmi.mode == SPLITMV) {
      assert(xd->mode_info_context->mbmi.partitioning != PARTITIONING_4X4);
      vp9_transform_mby_8x8(x);
      vp9_transform_mbuv_4x4(x);
      vp9_quantize_mby_8x8(x);
      vp9_quantize_mbuv_4x4(x);
      if (x->optimize) {
        vp9_optimize_mby_8x8(cm, x);
        vp9_optimize_mbuv_4x4(cm, x);
      }
      vp9_inverse_transform_mby_8x8(xd);
      vp9_inverse_transform_mbuv_4x4(xd);
    } else {
      vp9_transform_mb_8x8(x);
      vp9_quantize_mb_8x8(x);
      if (x->optimize)
        optimize_mb_8x8(cm, x);
      vp9_inverse_transform_mb_8x8(xd);
    }
  } else {
    transform_mb_4x4(x);
    vp9_quantize_mb_4x4(x);
    if (x->optimize)
      optimize_mb_4x4(cm, x);
    vp9_inverse_transform_mb_4x4(xd);
  }
}

void vp9_encode_inter16x16(VP9_COMMON *const cm, MACROBLOCK *x,
                           int mb_row, int mb_col) {
  MACROBLOCKD *const xd = &x->e_mbd;

  vp9_build_inter_predictors_mb(xd, mb_row, mb_col);
  subtract_mb(x);
  vp9_fidct_mb(cm, x);
  vp9_recon_mb(xd);
}

/* this function is used by first pass only */
void vp9_encode_inter16x16y(MACROBLOCK *x, int mb_row, int mb_col) {
  MACROBLOCKD *xd = &x->e_mbd;
  BLOCK *b = &x->block[0];

  vp9_build_inter16x16_predictors_mby(xd, xd->predictor, 16, mb_row, mb_col);

  vp9_subtract_mby(x->src_diff, *(b->base_src), xd->predictor, b->src_stride);

  vp9_transform_mby_4x4(x);
  vp9_quantize_mby_4x4(x);
  vp9_inverse_transform_mby_4x4(xd);

  vp9_recon_mby(xd);
}
