/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9/common/vp9_extend.h"
#include "vpx_mem/vpx_mem.h"

static void copy_and_extend_plane(const uint8_t *src, int src_pitch,
                                  uint8_t *dst, int dst_pitch,
                                  int w, int h,
                                  int extend_top, int extend_left,
                                  int extend_bottom, int extend_right) {
  int i, linesize;

  // copy the left and right most columns out
  const uint8_t *src_ptr1 = src;
  const uint8_t *src_ptr2 = src + w - 1;
  uint8_t *dst_ptr1 = dst - extend_left;
  uint8_t *dst_ptr2 = dst + w;

  for (i = 0; i < h; i++) {
    vpx_memset(dst_ptr1, src_ptr1[0], extend_left);
    vpx_memcpy(dst_ptr1 + extend_left, src_ptr1, w);
    vpx_memset(dst_ptr2, src_ptr2[0], extend_right);
    src_ptr1 += src_pitch;
    src_ptr2 += src_pitch;
    dst_ptr1 += dst_pitch;
    dst_ptr2 += dst_pitch;
  }

  // Now copy the top and bottom lines into each line of the respective
  // borders
  src_ptr1 = dst - extend_left;
  src_ptr2 = dst + dst_pitch * (h - 1) - extend_left;
  dst_ptr1 = dst + dst_pitch * (-extend_top) - extend_left;
  dst_ptr2 = dst + dst_pitch * (h) - extend_left;
  linesize = extend_left + extend_right + w;

  for (i = 0; i < extend_top; i++) {
    vpx_memcpy(dst_ptr1, src_ptr1, linesize);
    dst_ptr1 += dst_pitch;
  }

  for (i = 0; i < extend_bottom; i++) {
    vpx_memcpy(dst_ptr2, src_ptr2, linesize);
    dst_ptr2 += dst_pitch;
  }
}

void vp9_copy_and_extend_frame(const YV12_BUFFER_CONFIG *src,
                               YV12_BUFFER_CONFIG *dst) {
  const int et_y = dst->border;
  const int el_y = dst->border;
  const int eb_y = dst->border + dst->y_height - src->y_height;
  const int er_y = dst->border + dst->y_width - src->y_width;

  const int et_uv = dst->border >> 1;
  const int el_uv = dst->border >> 1;
  const int eb_uv = (dst->border >> 1) + dst->uv_height - src->uv_height;
  const int er_uv = (dst->border >> 1) + dst->uv_width - src->uv_width;

  copy_and_extend_plane(src->y_buffer, src->y_stride,
                        dst->y_buffer, dst->y_stride,
                        src->y_width, src->y_height,
                        et_y, el_y, eb_y, er_y);

  copy_and_extend_plane(src->u_buffer, src->uv_stride,
                        dst->u_buffer, dst->uv_stride,
                        src->uv_width, src->uv_height,
                        et_uv, el_uv, eb_uv, er_uv);

  copy_and_extend_plane(src->v_buffer, src->uv_stride,
                        dst->v_buffer, dst->uv_stride,
                        src->uv_width, src->uv_height,
                        et_y, el_y, eb_uv, er_uv);
}

void vp9_copy_and_extend_frame_with_rect(const YV12_BUFFER_CONFIG *src,
                                         YV12_BUFFER_CONFIG *dst,
                                         int srcy, int srcx,
                                         int srch, int srcw) {
  // If the side is not touching the bounder then don't extend.
  const int et_y = srcy ? 0 : dst->border;
  const int el_y = srcx ? 0 : dst->border;
  const int eb_y = srcy + srch != src->y_height ? 0 :
                      dst->border + dst->y_height - src->y_height;
  const int er_y = srcx + srcw != src->y_width ? 0 :
                      dst->border + dst->y_width - src->y_width;
  const int src_y_offset = srcy * src->y_stride + srcx;
  const int dst_y_offset = srcy * dst->y_stride + srcx;

  const int et_uv = (et_y + 1) >> 1;
  const int el_uv = (el_y + 1) >> 1;
  const int eb_uv = (eb_y + 1) >> 1;
  const int er_uv = (er_y + 1) >> 1;
  const int src_uv_offset = ((srcy * src->uv_stride) >> 1) + (srcx >> 1);
  const int dst_uv_offset = ((srcy * dst->uv_stride) >> 1) + (srcx >> 1);
  const int srch_uv = (srch + 1) >> 1;
  const int srcw_uv = (srcw + 1) >> 1;

  copy_and_extend_plane(src->y_buffer + src_y_offset, src->y_stride,
                        dst->y_buffer + dst_y_offset, dst->y_stride,
                        srcw, srch,
                        et_y, el_y, eb_y, er_y);

  copy_and_extend_plane(src->u_buffer + src_uv_offset, src->uv_stride,
                        dst->u_buffer + dst_uv_offset, dst->uv_stride,
                        srcw_uv, srch_uv,
                        et_uv, el_uv, eb_uv, er_uv);

  copy_and_extend_plane(src->v_buffer + src_uv_offset, src->uv_stride,
                        dst->v_buffer + dst_uv_offset, dst->uv_stride,
                        srcw_uv, srch_uv,
                        et_uv, el_uv, eb_uv, er_uv);
}

// note the extension is only for the last row, for intra prediction purpose
void vp9_extend_mb_row(YV12_BUFFER_CONFIG *buf,
                       uint8_t *y, uint8_t *u, uint8_t *v) {
  int i;

  y += buf->y_stride * 14;
  u += buf->uv_stride * 6;
  v += buf->uv_stride * 6;

  for (i = 0; i < 4; i++) {
    y[i] = y[-1];
    u[i] = u[-1];
    v[i] = v[-1];
  }

  y += buf->y_stride;
  u += buf->uv_stride;
  v += buf->uv_stride;

  for (i = 0; i < 4; i++) {
    y[i] = y[-1];
    u[i] = u[-1];
    v[i] = v[-1];
  }
}
