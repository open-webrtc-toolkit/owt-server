/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdio.h>

#include "./vpx_config.h"
#include "vp9_rtcd.h"
#include "vp9/common/vp9_reconintra.h"
#include "vpx_mem/vpx_mem.h"

// For skip_recon_mb(), add vp9_build_intra_predictors_mby_s(MACROBLOCKD *xd)
// and vp9_build_intra_predictors_mbuv_s(MACROBLOCKD *xd).

// Using multiplication and shifting instead of division in diagonal prediction.
// iscale table is calculated from ((1 << 16) + (i + 2) / 2) / (i+2) and used as
// ((A + B) * iscale[i] + (1 << 15)) >> 16;
// where A and B are weighted pixel values.
static const unsigned int iscale[64] = {
  32768, 21845, 16384, 13107, 10923,  9362,  8192,  7282,
   6554,  5958,  5461,  5041,  4681,  4369,  4096,  3855,
   3641,  3449,  3277,  3121,  2979,  2849,  2731,  2621,
   2521,  2427,  2341,  2260,  2185,  2114,  2048,  1986,
   1928,  1872,  1820,  1771,  1725,  1680,  1638,  1598,
   1560,  1524,  1489,  1456,  1425,  1394,  1365,  1337,
   1311,  1285,  1260,  1237,  1214,  1192,  1170,  1150,
   1130,  1111,  1092,  1074,  1057,  1040,  1024,  1008,
};

static INLINE int iscale_round(int value, int i) {
    return ROUND_POWER_OF_TWO(value * iscale[i], 16);
}

static void d27_predictor(uint8_t *ypred_ptr, int y_stride, int n,
                          uint8_t *yabove_row, uint8_t *yleft_col) {
  int r, c;

  r = 0;
  for (c = 0; c < n - 2; c++) {
    int a = c & 1 ? yleft_col[r + 1]
                  : ROUND_POWER_OF_TWO(yleft_col[r] + yleft_col[r + 1], 1);
    int b = yabove_row[c + 2];
    ypred_ptr[c] = iscale_round(2 * a + (c + 1) * b, 1 + c);
  }

  for (r = 1; r < n / 2 - 1; r++) {
    for (c = 0; c < n - 2 - 2 * r; c++) {
      int a = c & 1 ? yleft_col[r + 1]
                    : ROUND_POWER_OF_TWO(yleft_col[r] + yleft_col[r + 1], 1);
      int b = ypred_ptr[(r - 1) * y_stride + c + 2];
      ypred_ptr[r * y_stride + c] = iscale_round(2 * a + (c + 1) * b, 1 + c);
    }
  }

  for (; r < n - 1; r++) {
    for (c = 0; c < n; c++) {
      int v = c & 1 ? yleft_col[r + 1]
                    : ROUND_POWER_OF_TWO(yleft_col[r] + yleft_col[r + 1], 1);
      int h = r - c / 2;
      ypred_ptr[h * y_stride + c] = v;
    }
  }

  c = 0;
  r = n - 1;
  ypred_ptr[r * y_stride] = ROUND_POWER_OF_TWO(ypred_ptr[(r - 1) * y_stride] +
                                               yleft_col[r], 1);
  for (r = n - 2; r >= n / 2; --r) {
    int w = c + (n - 1 - r) * 2;
    ypred_ptr[r * y_stride + w] =
        ROUND_POWER_OF_TWO(ypred_ptr[(r - 1) * y_stride + w] +
                           ypred_ptr[r * y_stride + w - 1], 1);
  }

  for (c = 1; c < n; c++) {
    for (r = n - 1; r >= n / 2 + c / 2; --r) {
      int w = c + (n - 1 - r) * 2;
      ypred_ptr[r * y_stride + w] =
          ROUND_POWER_OF_TWO(ypred_ptr[(r - 1) * y_stride + w] +
                             ypred_ptr[r * y_stride + w - 1], 1);
    }
  }
}

static void d63_predictor(uint8_t *ypred_ptr, int y_stride, int n,
                          uint8_t *yabove_row, uint8_t *yleft_col) {
  int r, c;

  c = 0;
  for (r = 0; r < n - 2; r++) {
    int a = r & 1 ? yabove_row[c + 1]
                  : ROUND_POWER_OF_TWO(yabove_row[c] + yabove_row[c + 1], 1);
    int b = yleft_col[r + 2];
    ypred_ptr[r * y_stride] = iscale_round(2 * a + (r + 1) * b, 1 + r);
  }

  for (c = 1; c < n / 2 - 1; c++) {
    for (r = 0; r < n - 2 - 2 * c; r++) {
      int a = r & 1 ? yabove_row[c + 1]
                    : ROUND_POWER_OF_TWO(yabove_row[c] + yabove_row[c + 1], 1);
      int b = ypred_ptr[(r + 2) * y_stride + c - 1];
      ypred_ptr[r * y_stride + c] = iscale_round(2 * a + (c + 1) * b, 1 + c);
    }
  }

  for (; c < n - 1; ++c) {
    for (r = 0; r < n; r++) {
      int v = r & 1 ? yabove_row[c + 1]
                    : ROUND_POWER_OF_TWO(yabove_row[c] + yabove_row[c + 1], 1);
      int w = c - r / 2;
      ypred_ptr[r * y_stride + w] = v;
    }
  }

  r = 0;
  c = n - 1;
  ypred_ptr[c] = ROUND_POWER_OF_TWO(ypred_ptr[(c - 1)] + yabove_row[c], 1);
  for (c = n - 2; c >= n / 2; --c) {
    int h = r + (n - 1 - c) * 2;
    ypred_ptr[h * y_stride + c] =
         ROUND_POWER_OF_TWO(ypred_ptr[h * y_stride + c - 1] +
                            ypred_ptr[(h - 1) * y_stride + c], 1);
  }

  for (r = 1; r < n; r++) {
    for (c = n - 1; c >= n / 2 + r / 2; --c) {
      int h = r + (n - 1 - c) * 2;
      ypred_ptr[h * y_stride + c] =
          ROUND_POWER_OF_TWO(ypred_ptr[h * y_stride + c - 1] +
                             ypred_ptr[(h - 1) * y_stride + c], 1);
    }
  }
}

static void d45_predictor(uint8_t *ypred_ptr, int y_stride, int n,
                          uint8_t *yabove_row, uint8_t *yleft_col) {
  int r, c;

  for (r = 0; r < n - 1; ++r) {
    for (c = 0; c <= r; ++c) {
      ypred_ptr[(r - c) * y_stride + c] = iscale_round(
          yabove_row[r + 1] * (c + 1) + yleft_col[r + 1] * (r - c + 1), r);
    }
  }

  for (c = 0; c <= r; ++c) {
    int yabove_ext = yabove_row[r];  // clip_pixel(2 * yabove_row[r] -
                                     //            yabove_row[r - 1]);
    int yleft_ext = yleft_col[r];  // clip_pixel(2 * yleft_col[r] -
                                   //            yleft_col[r-1]);
    ypred_ptr[(r - c) * y_stride + c] =
         iscale_round(yabove_ext * (c + 1) + yleft_ext * (r - c + 1), r);
  }
  for (r = 1; r < n; ++r) {
    for (c = n - r; c < n; ++c) {
      const int yabove_ext = ypred_ptr[(r - 1) * y_stride + c];
      const int yleft_ext = ypred_ptr[r * y_stride + c - 1];
      ypred_ptr[r * y_stride + c] =
          ROUND_POWER_OF_TWO(yabove_ext + yleft_ext, 1);
    }
  }
}

static void d117_predictor(uint8_t *ypred_ptr, int y_stride, int n,
                           uint8_t *yabove_row, uint8_t *yleft_col) {
  int r, c;
  for (c = 0; c < n; c++)
    ypred_ptr[c] = ROUND_POWER_OF_TWO(yabove_row[c - 1] + yabove_row[c], 1);
  ypred_ptr += y_stride;
  for (c = 0; c < n; c++)
    ypred_ptr[c] = yabove_row[c - 1];
  ypred_ptr += y_stride;
  for (r = 2; r < n; ++r) {
    ypred_ptr[0] = yleft_col[r - 2];
    for (c = 1; c < n; c++)
      ypred_ptr[c] = ypred_ptr[-2 * y_stride + c - 1];
    ypred_ptr += y_stride;
  }
}

static void d135_predictor(uint8_t *ypred_ptr, int y_stride, int n,
                           uint8_t *yabove_row, uint8_t *yleft_col) {
  int r, c;
  ypred_ptr[0] = yabove_row[-1];
  for (c = 1; c < n; c++)
    ypred_ptr[c] = yabove_row[c - 1];
  for (r = 1; r < n; ++r)
    ypred_ptr[r * y_stride] = yleft_col[r - 1];

  ypred_ptr += y_stride;
  for (r = 1; r < n; ++r) {
    for (c = 1; c < n; c++) {
      ypred_ptr[c] = ypred_ptr[-y_stride + c - 1];
    }
    ypred_ptr += y_stride;
  }
}

static void d153_predictor(uint8_t *ypred_ptr, int y_stride, int n,
                           uint8_t *yabove_row, uint8_t *yleft_col) {
  int r, c;
  ypred_ptr[0] = ROUND_POWER_OF_TWO(yabove_row[-1] + yleft_col[0], 1);
  for (r = 1; r < n; r++)
    ypred_ptr[r * y_stride] =
        ROUND_POWER_OF_TWO(yleft_col[r - 1] + yleft_col[r], 1);
  ypred_ptr++;
  ypred_ptr[0] = yabove_row[-1];
  for (r = 1; r < n; r++)
    ypred_ptr[r * y_stride] = yleft_col[r - 1];
  ypred_ptr++;

  for (c = 0; c < n - 2; c++)
    ypred_ptr[c] = yabove_row[c];
  ypred_ptr += y_stride;
  for (r = 1; r < n; ++r) {
    for (c = 0; c < n - 2; c++)
      ypred_ptr[c] = ypred_ptr[-y_stride + c - 2];
    ypred_ptr += y_stride;
  }
}

static void corner_predictor(uint8_t *ypred_ptr, int y_stride, int n,
                             uint8_t *yabove_row,
                             uint8_t *yleft_col) {
  int mh, mv, maxgradh, maxgradv, x, y, nx, ny;
  int i, j;
  int top_left = yabove_row[-1];
  mh = mv = 0;
  maxgradh = yabove_row[1] - top_left;
  maxgradv = yleft_col[1] - top_left;
  for (i = 2; i < n; ++i) {
    int gh = yabove_row[i] - yabove_row[i - 2];
    int gv = yleft_col[i] - yleft_col[i - 2];
    if (gh > maxgradh) {
      maxgradh = gh;
      mh = i - 1;
    }
    if (gv > maxgradv) {
      maxgradv = gv;
      mv = i - 1;
    }
  }
  nx = mh + mv + 3;
  ny = 2 * n + 1 - nx;

  x = top_left;
  for (i = 0; i <= mh; ++i) x += yabove_row[i];
  for (i = 0; i <= mv; ++i) x += yleft_col[i];
  x += (nx >> 1);
  x /= nx;
  y = 0;
  for (i = mh + 1; i < n; ++i) y += yabove_row[i];
  for (i = mv + 1; i < n; ++i) y += yleft_col[i];
  y += (ny >> 1);
  y /= ny;

  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j)
      ypred_ptr[j] = (i <= mh && j <= mv ? x : y);
    ypred_ptr += y_stride;
  }
}

void vp9_recon_intra_mbuv(MACROBLOCKD *xd) {
  int i;
  for (i = 16; i < 24; i += 2) {
    BLOCKD *b = &xd->block[i];
    vp9_recon2b(b->predictor, b->diff, *(b->base_dst) + b->dst, b->dst_stride);
  }
}

static INLINE int log2_minus_1(int n) {
  switch (n) {
    case 4: return 1;
    case 8: return 2;
    case 16: return 3;
    case 32: return 4;
    case 64: return 5;
    default:
      assert(0);
      return 0;
  }
}


void vp9_build_intra_predictors_internal(uint8_t *src, int src_stride,
                                         uint8_t *ypred_ptr,
                                         int y_stride, int mode, int bsize,
                                         int up_available, int left_available,
                                         int right_available) {
  int r, c, i;
  uint8_t yleft_col[64], yabove_data[65], ytop_left;
  uint8_t *yabove_row = yabove_data + 1;
  /*
   * 127 127 127 .. 127 127 127 127 127 127
   * 129  A   B  ..  Y   Z
   * 129  C   D  ..  W   X
   * 129  E   F  ..  U   V
   * 129  G   H  ..  S   T   T   T   T   T
   *  ..
   */

  if (left_available) {
    for (i = 0; i < bsize; i++)
      yleft_col[i] = src[i * src_stride - 1];
  } else {
    vpx_memset(yleft_col, 129, bsize);
  }

  if (up_available) {
    uint8_t *yabove_ptr = src - src_stride;
    vpx_memcpy(yabove_row, yabove_ptr, bsize);
    if (left_available) {
      ytop_left = yabove_ptr[-1];
    } else {
      ytop_left = 127;
    }
  } else {
    vpx_memset(yabove_row, 127, bsize);
    ytop_left = 127;
  }
  yabove_row[-1] = ytop_left;

  /* for Y */
  switch (mode) {
    case DC_PRED: {
      int expected_dc;
      int i;
      int shift;
      int average = 0;
      int log2_bsize_minus_1 = log2_minus_1(bsize);

      if (up_available || left_available) {
        if (up_available) {
          for (i = 0; i < bsize; i++) {
            average += yabove_row[i];
          }
        }

        if (left_available) {
          for (i = 0; i < bsize; i++) {
            average += yleft_col[i];
          }
        }
        shift = log2_bsize_minus_1 + up_available + left_available;
        expected_dc = ROUND_POWER_OF_TWO(average, shift);
      } else {
        expected_dc = 128;
      }

      for (r = 0; r < bsize; r++) {
        vpx_memset(ypred_ptr, expected_dc, bsize);
        ypred_ptr += y_stride;
      }
    }
    break;
    case V_PRED:
      for (r = 0; r < bsize; r++) {
        memcpy(ypred_ptr, yabove_row, bsize);
        ypred_ptr += y_stride;
      }
      break;
    case H_PRED:
      for (r = 0; r < bsize; r++) {
        vpx_memset(ypred_ptr, yleft_col[r], bsize);
        ypred_ptr += y_stride;
      }
      break;
    case TM_PRED:
      for (r = 0; r < bsize; r++) {
        for (c = 0; c < bsize; c++) {
          ypred_ptr[c] = clip_pixel(yleft_col[r] + yabove_row[c] - ytop_left);
        }

        ypred_ptr += y_stride;
      }
      break;
    case D45_PRED:
      d45_predictor(ypred_ptr, y_stride, bsize,  yabove_row, yleft_col);
      break;
    case D135_PRED:
      d135_predictor(ypred_ptr, y_stride, bsize,  yabove_row, yleft_col);
      break;
    case D117_PRED:
      d117_predictor(ypred_ptr, y_stride, bsize,  yabove_row, yleft_col);
      break;
    case D153_PRED:
      d153_predictor(ypred_ptr, y_stride, bsize,  yabove_row, yleft_col);
      break;
    case D27_PRED:
      d27_predictor(ypred_ptr, y_stride, bsize,  yabove_row, yleft_col);
      break;
    case D63_PRED:
      d63_predictor(ypred_ptr, y_stride, bsize,  yabove_row, yleft_col);
      break;
    case I8X8_PRED:
    case B_PRED:
    case NEARESTMV:
    case NEARMV:
    case ZEROMV:
    case NEWMV:
    case SPLITMV:
    case MB_MODE_COUNT:
      break;
  }
}

#if CONFIG_COMP_INTERINTRA_PRED
static void combine_interintra(MB_PREDICTION_MODE mode,
                               uint8_t *interpred,
                               int interstride,
                               uint8_t *intrapred,
                               int intrastride,
                               int size) {
  // TODO(debargha): Explore different ways of combining predictors
  //                 or designing the tables below
  static const int scale_bits = 8;
  static const int scale_max = 256;     // 1 << scale_bits;
  static const int scale_round = 127;   // (1 << (scale_bits - 1));
  // This table is a function A + B*exp(-kx), where x is hor. index
  static const int weights1d[64] = {
    128, 125, 122, 119, 116, 114, 111, 109,
    107, 105, 103, 101,  99,  97,  96,  94,
     93,  91,  90,  89,  88,  86,  85,  84,
     83,  82,  81,  81,  80,  79,  78,  78,
     77,  76,  76,  75,  75,  74,  74,  73,
     73,  72,  72,  71,  71,  71,  70,  70,
     70,  70,  69,  69,  69,  69,  68,  68,
     68,  68,  68,  67,  67,  67,  67,  67,
  };

  int size_scale = (size >= 64 ? 1:
                    size == 32 ? 2 :
                    size == 16 ? 4 :
                    size == 8  ? 8 : 16);
  int i, j;
  switch (mode) {
    case V_PRED:
      for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
          int k = i * interstride + j;
          int scale = weights1d[i * size_scale];
          interpred[k] =
              ((scale_max - scale) * interpred[k] +
               scale * intrapred[i * intrastride + j] + scale_round)
              >> scale_bits;
        }
      }
      break;

    case H_PRED:
      for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
          int k = i * interstride + j;
          int scale = weights1d[j * size_scale];
          interpred[k] =
              ((scale_max - scale) * interpred[k] +
               scale * intrapred[i * intrastride + j] + scale_round)
              >> scale_bits;
        }
      }
      break;

    case D63_PRED:
    case D117_PRED:
      for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
          int k = i * interstride + j;
          int scale = (weights1d[i * size_scale] * 3 +
                       weights1d[j * size_scale]) >> 2;
          interpred[k] =
              ((scale_max - scale) * interpred[k] +
               scale * intrapred[i * intrastride + j] + scale_round)
              >> scale_bits;
        }
      }
      break;

    case D27_PRED:
    case D153_PRED:
      for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
          int k = i * interstride + j;
          int scale = (weights1d[j * size_scale] * 3 +
                       weights1d[i * size_scale]) >> 2;
          interpred[k] =
              ((scale_max - scale) * interpred[k] +
               scale * intrapred[i * intrastride + j] + scale_round)
              >> scale_bits;
        }
      }
      break;

    case D135_PRED:
      for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
          int k = i * interstride + j;
          int scale = weights1d[(i < j ? i : j) * size_scale];
          interpred[k] =
              ((scale_max - scale) * interpred[k] +
               scale * intrapred[i * intrastride + j] + scale_round)
              >> scale_bits;
        }
      }
      break;

    case D45_PRED:
      for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
          int k = i * interstride + j;
          int scale = (weights1d[i * size_scale] +
                       weights1d[j * size_scale]) >> 1;
          interpred[k] =
              ((scale_max - scale) * interpred[k] +
               scale * intrapred[i * intrastride + j] + scale_round)
              >> scale_bits;
        }
      }
      break;

    case TM_PRED:
    case DC_PRED:
    default:
      // simple average
      for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
          int k = i * interstride + j;
          interpred[k] = (interpred[k] + intrapred[i * intrastride + j]) >> 1;
        }
      }
      break;
  }
}

void vp9_build_interintra_16x16_predictors_mb(MACROBLOCKD *xd,
                                              uint8_t *ypred,
                                              uint8_t *upred,
                                              uint8_t *vpred,
                                              int ystride, int uvstride) {
  vp9_build_interintra_16x16_predictors_mby(xd, ypred, ystride);
  vp9_build_interintra_16x16_predictors_mbuv(xd, upred, vpred, uvstride);
}

void vp9_build_interintra_16x16_predictors_mby(MACROBLOCKD *xd,
                                               uint8_t *ypred,
                                               int ystride) {
  uint8_t intrapredictor[256];
  vp9_build_intra_predictors_internal(
      xd->dst.y_buffer, xd->dst.y_stride,
      intrapredictor, 16,
      xd->mode_info_context->mbmi.interintra_mode, 16,
      xd->up_available, xd->left_available, xd->right_available);
  combine_interintra(xd->mode_info_context->mbmi.interintra_mode,
                     ypred, ystride, intrapredictor, 16, 16);
}

void vp9_build_interintra_16x16_predictors_mbuv(MACROBLOCKD *xd,
                                                uint8_t *upred,
                                                uint8_t *vpred,
                                                int uvstride) {
  uint8_t uintrapredictor[64];
  uint8_t vintrapredictor[64];
  vp9_build_intra_predictors_internal(
      xd->dst.u_buffer, xd->dst.uv_stride,
      uintrapredictor, 8,
      xd->mode_info_context->mbmi.interintra_uv_mode, 8,
      xd->up_available, xd->left_available, xd->right_available);
  vp9_build_intra_predictors_internal(
      xd->dst.v_buffer, xd->dst.uv_stride,
      vintrapredictor, 8,
      xd->mode_info_context->mbmi.interintra_uv_mode, 8,
      xd->up_available, xd->left_available, xd->right_available);
  combine_interintra(xd->mode_info_context->mbmi.interintra_uv_mode,
                     upred, uvstride, uintrapredictor, 8, 8);
  combine_interintra(xd->mode_info_context->mbmi.interintra_uv_mode,
                     vpred, uvstride, vintrapredictor, 8, 8);
}

void vp9_build_interintra_32x32_predictors_sby(MACROBLOCKD *xd,
                                               uint8_t *ypred,
                                               int ystride) {
  uint8_t intrapredictor[1024];
  vp9_build_intra_predictors_internal(
      xd->dst.y_buffer, xd->dst.y_stride,
      intrapredictor, 32,
      xd->mode_info_context->mbmi.interintra_mode, 32,
      xd->up_available, xd->left_available, xd->right_available);
  combine_interintra(xd->mode_info_context->mbmi.interintra_mode,
                     ypred, ystride, intrapredictor, 32, 32);
}

void vp9_build_interintra_32x32_predictors_sbuv(MACROBLOCKD *xd,
                                                uint8_t *upred,
                                                uint8_t *vpred,
                                                int uvstride) {
  uint8_t uintrapredictor[256];
  uint8_t vintrapredictor[256];
  vp9_build_intra_predictors_internal(
      xd->dst.u_buffer, xd->dst.uv_stride,
      uintrapredictor, 16,
      xd->mode_info_context->mbmi.interintra_uv_mode, 16,
      xd->up_available, xd->left_available, xd->right_available);
  vp9_build_intra_predictors_internal(
      xd->dst.v_buffer, xd->dst.uv_stride,
      vintrapredictor, 16,
      xd->mode_info_context->mbmi.interintra_uv_mode, 16,
      xd->up_available, xd->left_available, xd->right_available);
  combine_interintra(xd->mode_info_context->mbmi.interintra_uv_mode,
                     upred, uvstride, uintrapredictor, 16, 16);
  combine_interintra(xd->mode_info_context->mbmi.interintra_uv_mode,
                     vpred, uvstride, vintrapredictor, 16, 16);
}

void vp9_build_interintra_32x32_predictors_sb(MACROBLOCKD *xd,
                                              uint8_t *ypred,
                                              uint8_t *upred,
                                              uint8_t *vpred,
                                              int ystride,
                                              int uvstride) {
  vp9_build_interintra_32x32_predictors_sby(xd, ypred, ystride);
  vp9_build_interintra_32x32_predictors_sbuv(xd, upred, vpred, uvstride);
}

void vp9_build_interintra_64x64_predictors_sby(MACROBLOCKD *xd,
                                               uint8_t *ypred,
                                               int ystride) {
  uint8_t intrapredictor[4096];
  const int mode = xd->mode_info_context->mbmi.interintra_mode;
  vp9_build_intra_predictors_internal(xd->dst.y_buffer, xd->dst.y_stride,
                                      intrapredictor, 64, mode, 64,
                                      xd->up_available, xd->left_available,
                                      xd->right_available);
  combine_interintra(xd->mode_info_context->mbmi.interintra_mode,
                     ypred, ystride, intrapredictor, 64, 64);
}

void vp9_build_interintra_64x64_predictors_sbuv(MACROBLOCKD *xd,
                                                uint8_t *upred,
                                                uint8_t *vpred,
                                                int uvstride) {
  uint8_t uintrapredictor[1024];
  uint8_t vintrapredictor[1024];
  const int mode = xd->mode_info_context->mbmi.interintra_uv_mode;
  vp9_build_intra_predictors_internal(xd->dst.u_buffer, xd->dst.uv_stride,
                                      uintrapredictor, 32, mode, 32,
                                      xd->up_available, xd->left_available,
                                      xd->right_available);
  vp9_build_intra_predictors_internal(xd->dst.v_buffer, xd->dst.uv_stride,
                                      vintrapredictor, 32, mode, 32,
                                      xd->up_available, xd->left_available,
                                      xd->right_available);
  combine_interintra(xd->mode_info_context->mbmi.interintra_uv_mode,
                     upred, uvstride, uintrapredictor, 32, 32);
  combine_interintra(xd->mode_info_context->mbmi.interintra_uv_mode,
                     vpred, uvstride, vintrapredictor, 32, 32);
}

void vp9_build_interintra_64x64_predictors_sb(MACROBLOCKD *xd,
                                              uint8_t *ypred,
                                              uint8_t *upred,
                                              uint8_t *vpred,
                                              int ystride,
                                              int uvstride) {
  vp9_build_interintra_64x64_predictors_sby(xd, ypred, ystride);
  vp9_build_interintra_64x64_predictors_sbuv(xd, upred, vpred, uvstride);
}
#endif  // CONFIG_COMP_INTERINTRA_PRED

void vp9_build_intra_predictors_mby(MACROBLOCKD *xd) {
  vp9_build_intra_predictors_internal(xd->dst.y_buffer, xd->dst.y_stride,
                                      xd->predictor, 16,
                                      xd->mode_info_context->mbmi.mode, 16,
                                      xd->up_available, xd->left_available,
                                      xd->right_available);
}

void vp9_build_intra_predictors_mby_s(MACROBLOCKD *xd) {
  vp9_build_intra_predictors_internal(xd->dst.y_buffer, xd->dst.y_stride,
                                      xd->dst.y_buffer, xd->dst.y_stride,
                                      xd->mode_info_context->mbmi.mode, 16,
                                      xd->up_available, xd->left_available,
                                      xd->right_available);
}

void vp9_build_intra_predictors_sby_s(MACROBLOCKD *xd) {
  vp9_build_intra_predictors_internal(xd->dst.y_buffer, xd->dst.y_stride,
                                      xd->dst.y_buffer, xd->dst.y_stride,
                                      xd->mode_info_context->mbmi.mode, 32,
                                      xd->up_available, xd->left_available,
                                      xd->right_available);
}

void vp9_build_intra_predictors_sb64y_s(MACROBLOCKD *xd) {
  vp9_build_intra_predictors_internal(xd->dst.y_buffer, xd->dst.y_stride,
                                      xd->dst.y_buffer, xd->dst.y_stride,
                                      xd->mode_info_context->mbmi.mode, 64,
                                      xd->up_available, xd->left_available,
                                      xd->right_available);
}

void vp9_build_intra_predictors_mbuv_internal(MACROBLOCKD *xd,
                                              uint8_t *upred_ptr,
                                              uint8_t *vpred_ptr,
                                              int uv_stride,
                                              int mode, int bsize) {
  vp9_build_intra_predictors_internal(xd->dst.u_buffer, xd->dst.uv_stride,
                                      upred_ptr, uv_stride, mode, bsize,
                                      xd->up_available, xd->left_available,
                                      xd->right_available);
  vp9_build_intra_predictors_internal(xd->dst.v_buffer, xd->dst.uv_stride,
                                      vpred_ptr, uv_stride, mode, bsize,
                                      xd->up_available, xd->left_available,
                                      xd->right_available);
}

void vp9_build_intra_predictors_mbuv(MACROBLOCKD *xd) {
  vp9_build_intra_predictors_mbuv_internal(xd, &xd->predictor[256],
                                           &xd->predictor[320], 8,
                                           xd->mode_info_context->mbmi.uv_mode,
                                           8);
}

void vp9_build_intra_predictors_mbuv_s(MACROBLOCKD *xd) {
  vp9_build_intra_predictors_mbuv_internal(xd, xd->dst.u_buffer,
                                           xd->dst.v_buffer,
                                           xd->dst.uv_stride,
                                           xd->mode_info_context->mbmi.uv_mode,
                                           8);
}

void vp9_build_intra_predictors_sbuv_s(MACROBLOCKD *xd) {
  vp9_build_intra_predictors_mbuv_internal(xd, xd->dst.u_buffer,
                                           xd->dst.v_buffer, xd->dst.uv_stride,
                                           xd->mode_info_context->mbmi.uv_mode,
                                           16);
}

void vp9_build_intra_predictors_sb64uv_s(MACROBLOCKD *xd) {
  vp9_build_intra_predictors_mbuv_internal(xd, xd->dst.u_buffer,
                                           xd->dst.v_buffer, xd->dst.uv_stride,
                                           xd->mode_info_context->mbmi.uv_mode,
                                           32);
}

void vp9_intra8x8_predict(MACROBLOCKD *xd,
                          BLOCKD *b,
                          int mode,
                          uint8_t *predictor) {
  const int block4x4_idx = (b - xd->block);
  const int block_idx = (block4x4_idx >> 2) | !!(block4x4_idx & 2);
  const int have_top = (block_idx >> 1) || xd->up_available;
  const int have_left = (block_idx & 1)  || xd->left_available;
  const int have_right = !(block_idx & 1) || xd->right_available;

  vp9_build_intra_predictors_internal(*(b->base_dst) + b->dst,
                                      b->dst_stride, predictor, 16,
                                      mode, 8, have_top, have_left,
                                      have_right);
}

void vp9_intra_uv4x4_predict(MACROBLOCKD *xd,
                             BLOCKD *b,
                             int mode,
                             uint8_t *predictor) {
  const int block_idx = (b - xd->block) & 3;
  const int have_top = (block_idx >> 1) || xd->up_available;
  const int have_left = (block_idx & 1)  || xd->left_available;
  const int have_right = !(block_idx & 1) || xd->right_available;

  vp9_build_intra_predictors_internal(*(b->base_dst) + b->dst,
                                      b->dst_stride, predictor, 8,
                                      mode, 4, have_top, have_left,
                                      have_right);
}

/* TODO: try different ways of use Y-UV mode correlation
   Current code assumes that a uv 4x4 block use same mode
   as corresponding Y 8x8 area
   */
