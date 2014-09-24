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
#include "vpx_mem/vpx_mem.h"
#include "vp9/common/vp9_reconintra.h"
#include "vp9_rtcd.h"

#if CONFIG_NEWBINTRAMODES
static int find_grad_measure(uint8_t *x, int stride, int n, int t,
                             int dx, int dy) {
  int i, j;
  int count = 0, gsum = 0, gdiv;
  /* TODO: Make this code more efficient by breaking up into two loops */
  for (i = -t; i < n; ++i)
    for (j = -t; j < n; ++j) {
      int g;
      if (i >= 0 && j >= 0) continue;
      if (i + dy >= 0 && j + dx >= 0) continue;
      if (i + dy < -t || i + dy >= n || j + dx < -t || j + dx >= n) continue;
      g = abs(x[(i + dy) * stride + j + dx] - x[i * stride + j]);
      gsum += g * g;
      count++;
    }
  gdiv = (dx * dx + dy * dy) * count;
  return ((gsum << 8) + (gdiv >> 1)) / gdiv;
}

#if CONTEXT_PRED_REPLACEMENTS == 6
B_PREDICTION_MODE vp9_find_dominant_direction(uint8_t *ptr,
                                              int stride, int n) {
  int g[8], i, imin, imax;
  g[1] = find_grad_measure(ptr, stride, n, 4,  2, 1);
  g[2] = find_grad_measure(ptr, stride, n, 4,  1, 1);
  g[3] = find_grad_measure(ptr, stride, n, 4,  1, 2);
  g[5] = find_grad_measure(ptr, stride, n, 4, -1, 2);
  g[6] = find_grad_measure(ptr, stride, n, 4, -1, 1);
  g[7] = find_grad_measure(ptr, stride, n, 4, -2, 1);
  imin = 1;
  for (i = 2; i < 8; i += 1 + (i == 3))
    imin = (g[i] < g[imin] ? i : imin);
  imax = 1;
  for (i = 2; i < 8; i += 1 + (i == 3))
    imax = (g[i] > g[imax] ? i : imax);
  /*
  printf("%d %d %d %d %d %d = %d %d\n",
         g[1], g[2], g[3], g[5], g[6], g[7], imin, imax);
         */
  switch (imin) {
    case 1:
      return B_HD_PRED;
    case 2:
      return B_RD_PRED;
    case 3:
      return B_VR_PRED;
    case 5:
      return B_VL_PRED;
    case 6:
      return B_LD_PRED;
    case 7:
      return B_HU_PRED;
    default:
      assert(0);
  }
}
#elif CONTEXT_PRED_REPLACEMENTS == 4
B_PREDICTION_MODE vp9_find_dominant_direction(uint8_t *ptr,
                                              int stride, int n) {
  int g[8], i, imin, imax;
  g[1] = find_grad_measure(ptr, stride, n, 4,  2, 1);
  g[3] = find_grad_measure(ptr, stride, n, 4,  1, 2);
  g[5] = find_grad_measure(ptr, stride, n, 4, -1, 2);
  g[7] = find_grad_measure(ptr, stride, n, 4, -2, 1);
  imin = 1;
  for (i = 3; i < 8; i+=2)
    imin = (g[i] < g[imin] ? i : imin);
  imax = 1;
  for (i = 3; i < 8; i+=2)
    imax = (g[i] > g[imax] ? i : imax);
  /*
  printf("%d %d %d %d = %d %d\n",
         g[1], g[3], g[5], g[7], imin, imax);
         */
  switch (imin) {
    case 1:
      return B_HD_PRED;
    case 3:
      return B_VR_PRED;
    case 5:
      return B_VL_PRED;
    case 7:
      return B_HU_PRED;
    default:
      assert(0);
  }
}
#elif CONTEXT_PRED_REPLACEMENTS == 0
B_PREDICTION_MODE vp9_find_dominant_direction(uint8_t *ptr,
                                              int stride, int n) {
  int g[8], i, imin, imax;
  g[0] = find_grad_measure(ptr, stride, n, 4,  1, 0);
  g[1] = find_grad_measure(ptr, stride, n, 4,  2, 1);
  g[2] = find_grad_measure(ptr, stride, n, 4,  1, 1);
  g[3] = find_grad_measure(ptr, stride, n, 4,  1, 2);
  g[4] = find_grad_measure(ptr, stride, n, 4,  0, 1);
  g[5] = find_grad_measure(ptr, stride, n, 4, -1, 2);
  g[6] = find_grad_measure(ptr, stride, n, 4, -1, 1);
  g[7] = find_grad_measure(ptr, stride, n, 4, -2, 1);
  imax = 0;
  for (i = 1; i < 8; i++)
    imax = (g[i] > g[imax] ? i : imax);
  imin = 0;
  for (i = 1; i < 8; i++)
    imin = (g[i] < g[imin] ? i : imin);

  switch (imin) {
    case 0:
      return B_HE_PRED;
    case 1:
      return B_HD_PRED;
    case 2:
      return B_RD_PRED;
    case 3:
      return B_VR_PRED;
    case 4:
      return B_VE_PRED;
    case 5:
      return B_VL_PRED;
    case 6:
      return B_LD_PRED;
    case 7:
      return B_HU_PRED;
    default:
      assert(0);
  }
}
#endif

B_PREDICTION_MODE vp9_find_bpred_context(BLOCKD *x) {
  uint8_t *ptr = *(x->base_dst) + x->dst;
  int stride = x->dst_stride;
  return vp9_find_dominant_direction(ptr, stride, 4);
}
#endif

void vp9_intra4x4_predict(BLOCKD *x,
                          int b_mode,
                          uint8_t *predictor) {
  int i, r, c;

  uint8_t *above = *(x->base_dst) + x->dst - x->dst_stride;
  uint8_t left[4];
  uint8_t top_left = above[-1];

  left[0] = (*(x->base_dst))[x->dst - 1];
  left[1] = (*(x->base_dst))[x->dst - 1 + x->dst_stride];
  left[2] = (*(x->base_dst))[x->dst - 1 + 2 * x->dst_stride];
  left[3] = (*(x->base_dst))[x->dst - 1 + 3 * x->dst_stride];

#if CONFIG_NEWBINTRAMODES
  if (b_mode == B_CONTEXT_PRED)
    b_mode = x->bmi.as_mode.context;
#endif

  switch (b_mode) {
    case B_DC_PRED: {
      int expected_dc = 0;

      for (i = 0; i < 4; i++) {
        expected_dc += above[i];
        expected_dc += left[i];
      }

      expected_dc = (expected_dc + 4) >> 3;

      for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
          predictor[c] = expected_dc;
        }

        predictor += 16;
      }
    }
    break;
    case B_TM_PRED: {
      /* prediction similar to true_motion prediction */
      for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
          predictor[c] = clip_pixel(above[c] - top_left + left[r]);
        }

        predictor += 16;
      }
    }
    break;

    case B_VE_PRED: {
      unsigned int ap[4];

      ap[0] = above[0];
      ap[1] = above[1];
      ap[2] = above[2];
      ap[3] = above[3];

      for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
          predictor[c] = ap[c];
        }

        predictor += 16;
      }
    }
    break;

    case B_HE_PRED: {
      unsigned int lp[4];

      lp[0] = left[0];
      lp[1] = left[1];
      lp[2] = left[2];
      lp[3] = left[3];

      for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
          predictor[c] = lp[r];
        }

        predictor += 16;
      }
    }
    break;
    case B_LD_PRED: {
      uint8_t *ptr = above;

      predictor[0 * 16 + 0] = (ptr[0] + ptr[1] * 2 + ptr[2] + 2) >> 2;
      predictor[0 * 16 + 1] =
        predictor[1 * 16 + 0] = (ptr[1] + ptr[2] * 2 + ptr[3] + 2) >> 2;
      predictor[0 * 16 + 2] =
        predictor[1 * 16 + 1] =
          predictor[2 * 16 + 0] = (ptr[2] + ptr[3] * 2 + ptr[4] + 2) >> 2;
      predictor[0 * 16 + 3] =
        predictor[1 * 16 + 2] =
          predictor[2 * 16 + 1] =
            predictor[3 * 16 + 0] = (ptr[3] + ptr[4] * 2 + ptr[5] + 2) >> 2;
      predictor[1 * 16 + 3] =
        predictor[2 * 16 + 2] =
          predictor[3 * 16 + 1] = (ptr[4] + ptr[5] * 2 + ptr[6] + 2) >> 2;
      predictor[2 * 16 + 3] =
        predictor[3 * 16 + 2] = (ptr[5] + ptr[6] * 2 + ptr[7] + 2) >> 2;
      predictor[3 * 16 + 3] = (ptr[6] + ptr[7] * 2 + ptr[7] + 2) >> 2;

    }
    break;
    case B_RD_PRED: {
      uint8_t pp[9];

      pp[0] = left[3];
      pp[1] = left[2];
      pp[2] = left[1];
      pp[3] = left[0];
      pp[4] = top_left;
      pp[5] = above[0];
      pp[6] = above[1];
      pp[7] = above[2];
      pp[8] = above[3];

      predictor[3 * 16 + 0] = (pp[0] + pp[1] * 2 + pp[2] + 2) >> 2;
      predictor[3 * 16 + 1] =
        predictor[2 * 16 + 0] = (pp[1] + pp[2] * 2 + pp[3] + 2) >> 2;
      predictor[3 * 16 + 2] =
        predictor[2 * 16 + 1] =
          predictor[1 * 16 + 0] = (pp[2] + pp[3] * 2 + pp[4] + 2) >> 2;
      predictor[3 * 16 + 3] =
        predictor[2 * 16 + 2] =
          predictor[1 * 16 + 1] =
            predictor[0 * 16 + 0] = (pp[3] + pp[4] * 2 + pp[5] + 2) >> 2;
      predictor[2 * 16 + 3] =
        predictor[1 * 16 + 2] =
          predictor[0 * 16 + 1] = (pp[4] + pp[5] * 2 + pp[6] + 2) >> 2;
      predictor[1 * 16 + 3] =
        predictor[0 * 16 + 2] = (pp[5] + pp[6] * 2 + pp[7] + 2) >> 2;
      predictor[0 * 16 + 3] = (pp[6] + pp[7] * 2 + pp[8] + 2) >> 2;

    }
    break;
    case B_VR_PRED: {
      uint8_t pp[9];

      pp[0] = left[3];
      pp[1] = left[2];
      pp[2] = left[1];
      pp[3] = left[0];
      pp[4] = top_left;
      pp[5] = above[0];
      pp[6] = above[1];
      pp[7] = above[2];
      pp[8] = above[3];

      predictor[3 * 16 + 0] = (pp[1] + pp[2] * 2 + pp[3] + 2) >> 2;
      predictor[2 * 16 + 0] = (pp[2] + pp[3] * 2 + pp[4] + 2) >> 2;
      predictor[3 * 16 + 1] =
        predictor[1 * 16 + 0] = (pp[3] + pp[4] * 2 + pp[5] + 2) >> 2;
      predictor[2 * 16 + 1] =
        predictor[0 * 16 + 0] = (pp[4] + pp[5] + 1) >> 1;
      predictor[3 * 16 + 2] =
        predictor[1 * 16 + 1] = (pp[4] + pp[5] * 2 + pp[6] + 2) >> 2;
      predictor[2 * 16 + 2] =
        predictor[0 * 16 + 1] = (pp[5] + pp[6] + 1) >> 1;
      predictor[3 * 16 + 3] =
        predictor[1 * 16 + 2] = (pp[5] + pp[6] * 2 + pp[7] + 2) >> 2;
      predictor[2 * 16 + 3] =
        predictor[0 * 16 + 2] = (pp[6] + pp[7] + 1) >> 1;
      predictor[1 * 16 + 3] = (pp[6] + pp[7] * 2 + pp[8] + 2) >> 2;
      predictor[0 * 16 + 3] = (pp[7] + pp[8] + 1) >> 1;

    }
    break;
    case B_VL_PRED: {
      uint8_t *pp = above;

      predictor[0 * 16 + 0] = (pp[0] + pp[1] + 1) >> 1;
      predictor[1 * 16 + 0] = (pp[0] + pp[1] * 2 + pp[2] + 2) >> 2;
      predictor[2 * 16 + 0] =
        predictor[0 * 16 + 1] = (pp[1] + pp[2] + 1) >> 1;
      predictor[1 * 16 + 1] =
        predictor[3 * 16 + 0] = (pp[1] + pp[2] * 2 + pp[3] + 2) >> 2;
      predictor[2 * 16 + 1] =
        predictor[0 * 16 + 2] = (pp[2] + pp[3] + 1) >> 1;
      predictor[3 * 16 + 1] =
        predictor[1 * 16 + 2] = (pp[2] + pp[3] * 2 + pp[4] + 2) >> 2;
      predictor[0 * 16 + 3] =
        predictor[2 * 16 + 2] = (pp[3] + pp[4] + 1) >> 1;
      predictor[1 * 16 + 3] =
        predictor[3 * 16 + 2] = (pp[3] + pp[4] * 2 + pp[5] + 2) >> 2;
      predictor[2 * 16 + 3] = (pp[4] + pp[5] * 2 + pp[6] + 2) >> 2;
      predictor[3 * 16 + 3] = (pp[5] + pp[6] * 2 + pp[7] + 2) >> 2;
    }
    break;

    case B_HD_PRED: {
      uint8_t pp[9];

      pp[0] = left[3];
      pp[1] = left[2];
      pp[2] = left[1];
      pp[3] = left[0];
      pp[4] = top_left;
      pp[5] = above[0];
      pp[6] = above[1];
      pp[7] = above[2];
      pp[8] = above[3];


      predictor[3 * 16 + 0] = (pp[0] + pp[1] + 1) >> 1;
      predictor[3 * 16 + 1] = (pp[0] + pp[1] * 2 + pp[2] + 2) >> 2;
      predictor[2 * 16 + 0] =
        predictor[3 * 16 + 2] = (pp[1] + pp[2] + 1) >> 1;
      predictor[2 * 16 + 1] =
        predictor[3 * 16 + 3] = (pp[1] + pp[2] * 2 + pp[3] + 2) >> 2;
      predictor[2 * 16 + 2] =
        predictor[1 * 16 + 0] = (pp[2] + pp[3] + 1) >> 1;
      predictor[2 * 16 + 3] =
        predictor[1 * 16 + 1] = (pp[2] + pp[3] * 2 + pp[4] + 2) >> 2;
      predictor[1 * 16 + 2] =
        predictor[0 * 16 + 0] = (pp[3] + pp[4] + 1) >> 1;
      predictor[1 * 16 + 3] =
        predictor[0 * 16 + 1] = (pp[3] + pp[4] * 2 + pp[5] + 2) >> 2;
      predictor[0 * 16 + 2] = (pp[4] + pp[5] * 2 + pp[6] + 2) >> 2;
      predictor[0 * 16 + 3] = (pp[5] + pp[6] * 2 + pp[7] + 2) >> 2;
    }
    break;


    case B_HU_PRED: {
      uint8_t *pp = left;
      predictor[0 * 16 + 0] = (pp[0] + pp[1] + 1) >> 1;
      predictor[0 * 16 + 1] = (pp[0] + pp[1] * 2 + pp[2] + 2) >> 2;
      predictor[0 * 16 + 2] =
        predictor[1 * 16 + 0] = (pp[1] + pp[2] + 1) >> 1;
      predictor[0 * 16 + 3] =
        predictor[1 * 16 + 1] = (pp[1] + pp[2] * 2 + pp[3] + 2) >> 2;
      predictor[1 * 16 + 2] =
        predictor[2 * 16 + 0] = (pp[2] + pp[3] + 1) >> 1;
      predictor[1 * 16 + 3] =
        predictor[2 * 16 + 1] = (pp[2] + pp[3] * 2 + pp[3] + 2) >> 2;
      predictor[2 * 16 + 2] =
        predictor[2 * 16 + 3] =
          predictor[3 * 16 + 0] =
            predictor[3 * 16 + 1] =
              predictor[3 * 16 + 2] =
                predictor[3 * 16 + 3] = pp[3];
    }
    break;

#if CONFIG_NEWBINTRAMODES
    case B_CONTEXT_PRED:
    break;
    /*
    case B_CORNER_PRED:
    corner_predictor(predictor, 16, 4, above, left);
    break;
    */
#endif
  }
}

/* copy 4 bytes from the above right down so that the 4x4 prediction modes using pixels above and
 * to the right prediction have filled in pixels to use.
 */
void vp9_intra_prediction_down_copy(MACROBLOCKD *xd) {
  int extend_edge = xd->mb_to_right_edge == 0 && xd->mb_index < 2;
  uint8_t *above_right = *(xd->block[0].base_dst) + xd->block[0].dst -
                               xd->block[0].dst_stride + 16;
  uint32_t *dst_ptr0 = (uint32_t *)above_right;
  uint32_t *dst_ptr1 =
    (uint32_t *)(above_right + 4 * xd->block[0].dst_stride);
  uint32_t *dst_ptr2 =
    (uint32_t *)(above_right + 8 * xd->block[0].dst_stride);
  uint32_t *dst_ptr3 =
    (uint32_t *)(above_right + 12 * xd->block[0].dst_stride);

  uint32_t *src_ptr = (uint32_t *) above_right;

  if ((xd->sb_index >= 2 && xd->mb_to_right_edge == 0) ||
      (xd->sb_index == 3 && xd->mb_index & 1))
    src_ptr = (uint32_t *) (((uint8_t *) src_ptr) - 32 *
                                                    xd->block[0].dst_stride);
  if (xd->mb_index == 3 ||
      (xd->mb_to_right_edge == 0 && xd->mb_index == 2))
    src_ptr = (uint32_t *) (((uint8_t *) src_ptr) - 16 *
                                                    xd->block[0].dst_stride);

  if (extend_edge) {
    *src_ptr = ((uint8_t *) src_ptr)[-1] * 0x01010101U;
  }

  *dst_ptr0 = *src_ptr;
  *dst_ptr1 = *src_ptr;
  *dst_ptr2 = *src_ptr;
  *dst_ptr3 = *src_ptr;
}
