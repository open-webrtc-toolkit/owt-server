/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>
#include "vpx_scale/yv12config.h"
#include "vpx_mem/vpx_mem.h"
#include "vpx_scale/vpx_scale.h"

/****************************************************************************
*  Exports
****************************************************************************/

/****************************************************************************
 *
 ****************************************************************************/
static void extend_plane(uint8_t *s,       /* source */
                         int sp,           /* source pitch */
                         int w,            /* width */
                         int h,            /* height */
                         int et,           /* extend top border */
                         int el,           /* extend left border */
                         int eb,           /* extend bottom border */
                         int er) {         /* extend right border */
  int i;
  uint8_t *src_ptr1, *src_ptr2;
  uint8_t *dest_ptr1, *dest_ptr2;
  int linesize;

  /* copy the left and right most columns out */
  src_ptr1 = s;
  src_ptr2 = s + w - 1;
  dest_ptr1 = s - el;
  dest_ptr2 = s + w;

  for (i = 0; i < h; i++) {
    vpx_memset(dest_ptr1, src_ptr1[0], el);
    vpx_memset(dest_ptr2, src_ptr2[0], er);
    src_ptr1  += sp;
    src_ptr2  += sp;
    dest_ptr1 += sp;
    dest_ptr2 += sp;
  }

  /* Now copy the top and bottom lines into each line of the respective
   * borders
   */
  src_ptr1 = s - el;
  src_ptr2 = s + sp * (h - 1) - el;
  dest_ptr1 = s + sp * (-et) - el;
  dest_ptr2 = s + sp * (h) - el;
  linesize = el + er + w;

  for (i = 0; i < et; i++) {
    vpx_memcpy(dest_ptr1, src_ptr1, linesize);
    dest_ptr1 += sp;
  }

  for (i = 0; i < eb; i++) {
    vpx_memcpy(dest_ptr2, src_ptr2, linesize);
    dest_ptr2 += sp;
  }
}

void
vp8_yv12_extend_frame_borders_c(YV12_BUFFER_CONFIG *ybf) {
  assert(ybf->y_height - ybf->y_crop_height < 16);
  assert(ybf->y_width - ybf->y_crop_width < 16);
  assert(ybf->y_height - ybf->y_crop_height >= 0);
  assert(ybf->y_width - ybf->y_crop_width >= 0);

  extend_plane(ybf->y_buffer, ybf->y_stride,
               ybf->y_crop_width, ybf->y_crop_height,
               ybf->border, ybf->border,
               ybf->border + ybf->y_height - ybf->y_crop_height,
               ybf->border + ybf->y_width - ybf->y_crop_width);

  extend_plane(ybf->u_buffer, ybf->uv_stride,
               (ybf->y_crop_width + 1) / 2, (ybf->y_crop_height + 1) / 2,
               ybf->border / 2, ybf->border / 2,
               (ybf->border + ybf->y_height - ybf->y_crop_height + 1) / 2,
               (ybf->border + ybf->y_width - ybf->y_crop_width + 1) / 2);

  extend_plane(ybf->v_buffer, ybf->uv_stride,
               (ybf->y_crop_width + 1) / 2, (ybf->y_crop_height + 1) / 2,
               ybf->border / 2, ybf->border / 2,
               (ybf->border + ybf->y_height - ybf->y_crop_height + 1) / 2,
               (ybf->border + ybf->y_width - ybf->y_crop_width + 1) / 2);
}


/****************************************************************************
 *
 *  ROUTINE       : vp8_yv12_copy_frame
 *
 *  INPUTS        :
 *
 *  OUTPUTS       : None.
 *
 *  RETURNS       : void
 *
 *  FUNCTION      : Copies the source image into the destination image and
 *                  updates the destination's UMV borders.
 *
 *  SPECIAL NOTES : The frames are assumed to be identical in size.
 *
 ****************************************************************************/
void
vp8_yv12_copy_frame_c(YV12_BUFFER_CONFIG *src_ybc,
                      YV12_BUFFER_CONFIG *dst_ybc) {
  int row;
  unsigned char *source, *dest;

#if 0
  /* These assertions are valid in the codec, but the libvpx-tester uses
   * this code slightly differently.
   */
  assert(src_ybc->y_width == dst_ybc->y_width);
  assert(src_ybc->y_height == dst_ybc->y_height);
#endif

  source = src_ybc->y_buffer;
  dest = dst_ybc->y_buffer;

  for (row = 0; row < src_ybc->y_height; row++) {
    vpx_memcpy(dest, source, src_ybc->y_width);
    source += src_ybc->y_stride;
    dest   += dst_ybc->y_stride;
  }

  source = src_ybc->u_buffer;
  dest = dst_ybc->u_buffer;

  for (row = 0; row < src_ybc->uv_height; row++) {
    vpx_memcpy(dest, source, src_ybc->uv_width);
    source += src_ybc->uv_stride;
    dest   += dst_ybc->uv_stride;
  }

  source = src_ybc->v_buffer;
  dest = dst_ybc->v_buffer;

  for (row = 0; row < src_ybc->uv_height; row++) {
    vpx_memcpy(dest, source, src_ybc->uv_width);
    source += src_ybc->uv_stride;
    dest   += dst_ybc->uv_stride;
  }

  vp8_yv12_extend_frame_borders_c(dst_ybc);
}

void vp8_yv12_copy_y_c(YV12_BUFFER_CONFIG *src_ybc,
                       YV12_BUFFER_CONFIG *dst_ybc) {
  int row;
  unsigned char *source, *dest;


  source = src_ybc->y_buffer;
  dest = dst_ybc->y_buffer;

  for (row = 0; row < src_ybc->y_height; row++) {
    vpx_memcpy(dest, source, src_ybc->y_width);
    source += src_ybc->y_stride;
    dest   += dst_ybc->y_stride;
  }
}
