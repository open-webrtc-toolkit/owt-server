/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9/common/vp9_loopfilter.h"

prototype_loopfilter(vp9_loop_filter_vertical_edge_mmx);
prototype_loopfilter(vp9_loop_filter_horizontal_edge_mmx);

/* Horizontal MB filtering */
void vp9_loop_filter_mbh_mmx(unsigned char *y_ptr,
                             unsigned char *u_ptr, unsigned char *v_ptr,
                             int y_stride, int uv_stride,
                             struct loop_filter_info *lfi) {
}

/* Vertical MB Filtering */
void vp9_loop_filter_mbv_mmx(unsigned char *y_ptr,
                             unsigned char *u_ptr, unsigned char *v_ptr,
                             int y_stride, int uv_stride,
                             struct loop_filter_info *lfi) {
}

/* Horizontal B Filtering */
void vp9_loop_filter_bh_mmx(unsigned char *y_ptr,
                            unsigned char *u_ptr, unsigned char *v_ptr,
                            int y_stride, int uv_stride,
                            struct loop_filter_info *lfi) {

}

void vp9_loop_filter_bhs_mmx(unsigned char *y_ptr, int y_stride,
                             const unsigned char *blimit) {
  vp9_loop_filter_simple_horizontal_edge_mmx(y_ptr + 4 * y_stride,
                                             y_stride, blimit);
  vp9_loop_filter_simple_horizontal_edge_mmx(y_ptr + 8 * y_stride,
                                             y_stride, blimit);
  vp9_loop_filter_simple_horizontal_edge_mmx(y_ptr + 12 * y_stride,
                                             y_stride, blimit);
}

/* Vertical B Filtering */
void vp9_loop_filter_bv_mmx(unsigned char *y_ptr,
                            unsigned char *u_ptr, unsigned char *v_ptr,
                            int y_stride, int uv_stride,
                            struct loop_filter_info *lfi) {
  vp9_loop_filter_vertical_edge_mmx(y_ptr + 4, y_stride,
                                    lfi->blim, lfi->lim, lfi->hev_thr, 2);
  vp9_loop_filter_vertical_edge_mmx(y_ptr + 8, y_stride,
                                    lfi->blim, lfi->lim, lfi->hev_thr, 2);
  vp9_loop_filter_vertical_edge_mmx(y_ptr + 12, y_stride,
                                    lfi->blim, lfi->lim, lfi->hev_thr, 2);

  if (u_ptr)
    vp9_loop_filter_vertical_edge_mmx(u_ptr + 4, uv_stride,
                                      lfi->blim, lfi->lim, lfi->hev_thr, 1);

  if (v_ptr)
    vp9_loop_filter_vertical_edge_mmx(v_ptr + 4, uv_stride,
                                      lfi->blim, lfi->lim, lfi->hev_thr, 1);
}

void vp9_loop_filter_bvs_mmx(unsigned char *y_ptr, int y_stride,
                             const unsigned char *blimit) {
  vp9_loop_filter_simple_vertical_edge_mmx(y_ptr + 4, y_stride, blimit);
  vp9_loop_filter_simple_vertical_edge_mmx(y_ptr + 8, y_stride, blimit);
  vp9_loop_filter_simple_vertical_edge_mmx(y_ptr + 12, y_stride, blimit);
}
