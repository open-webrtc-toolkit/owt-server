/*
 *  Copyright (c) 2013 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef VP9_VP9_IFACE_COMMON_H_
#define VP9_VP9_IFACE_COMMON_H_

static void yuvconfig2image(vpx_image_t               *img,
                            const YV12_BUFFER_CONFIG  *yv12,
                            void                      *user_priv) {
  /** vpx_img_wrap() doesn't allow specifying independent strides for
    * the Y, U, and V planes, nor other alignment adjustments that
    * might be representable by a YV12_BUFFER_CONFIG, so we just
    * initialize all the fields.*/
  img->fmt = yv12->clrtype == REG_YUV ?
             VPX_IMG_FMT_I420 : VPX_IMG_FMT_VPXI420;
  img->w = yv12->y_stride;
  img->h = (yv12->y_height + 2 * VP9BORDERINPIXELS + 15) & ~15;
  img->d_w = yv12->y_width;
  img->d_h = yv12->y_height;
  img->x_chroma_shift = 1;
  img->y_chroma_shift = 1;
  img->planes[VPX_PLANE_Y] = yv12->y_buffer;
  img->planes[VPX_PLANE_U] = yv12->u_buffer;
  img->planes[VPX_PLANE_V] = yv12->v_buffer;
  img->planes[VPX_PLANE_ALPHA] = NULL;
  img->stride[VPX_PLANE_Y] = yv12->y_stride;
  img->stride[VPX_PLANE_U] = yv12->uv_stride;
  img->stride[VPX_PLANE_V] = yv12->uv_stride;
  img->stride[VPX_PLANE_ALPHA] = yv12->y_stride;
  img->bps = 12;
  img->user_priv = user_priv;
  img->img_data = yv12->buffer_alloc;
  img->img_data_owner = 0;
  img->self_allocd = 0;
}

#endif
