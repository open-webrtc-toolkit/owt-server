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
#include <assert.h>

#include "vp9/common/vp9_onyxc_int.h"
#if CONFIG_POSTPROC
#include "vp9/common/vp9_postproc.h"
#endif
#include "vp9/decoder/vp9_onyxd.h"
#include "vp9/decoder/vp9_onyxd_int.h"
#include "vpx_mem/vpx_mem.h"
#include "vp9/common/vp9_alloccommon.h"
#include "vp9/common/vp9_loopfilter.h"
#include "vp9/common/vp9_swapyv12buffer.h"

#include "vp9/common/vp9_quant_common.h"
#include "vpx_scale/vpx_scale.h"
#include "vp9/common/vp9_systemdependent.h"
#include "vpx_ports/vpx_timer.h"
#include "vp9/decoder/vp9_decodframe.h"
#include "vp9/decoder/vp9_detokenize.h"
#include "./vpx_scale_rtcd.h"

#define WRITE_RECON_BUFFER 0
#if WRITE_RECON_BUFFER == 1
static void recon_write_yuv_frame(const char *name,
                                  const YV12_BUFFER_CONFIG *s,
                                  int w, int _h) {
  FILE *yuv_file = fopen((char *)name, "ab");
  const uint8_t *src = s->y_buffer;
  int h = _h;

  do {
    fwrite(src, w, 1,  yuv_file);
    src += s->y_stride;
  } while (--h);

  src = s->u_buffer;
  h = (_h + 1) >> 1;
  w = (w + 1) >> 1;

  do {
    fwrite(src, w, 1,  yuv_file);
    src += s->uv_stride;
  } while (--h);

  src = s->v_buffer;
  h = (_h + 1) >> 1;

  do {
    fwrite(src, w, 1, yuv_file);
    src += s->uv_stride;
  } while (--h);

  fclose(yuv_file);
}
#endif
#if WRITE_RECON_BUFFER == 2
void write_dx_frame_to_file(YV12_BUFFER_CONFIG *frame, int this_frame) {

  // write the frame
  FILE *yframe;
  int i;
  char filename[255];

  sprintf(filename, "dx\\y%04d.raw", this_frame);
  yframe = fopen(filename, "wb");

  for (i = 0; i < frame->y_height; i++)
    fwrite(frame->y_buffer + i * frame->y_stride,
           frame->y_width, 1, yframe);

  fclose(yframe);
  sprintf(filename, "dx\\u%04d.raw", this_frame);
  yframe = fopen(filename, "wb");

  for (i = 0; i < frame->uv_height; i++)
    fwrite(frame->u_buffer + i * frame->uv_stride,
           frame->uv_width, 1, yframe);

  fclose(yframe);
  sprintf(filename, "dx\\v%04d.raw", this_frame);
  yframe = fopen(filename, "wb");

  for (i = 0; i < frame->uv_height; i++)
    fwrite(frame->v_buffer + i * frame->uv_stride,
           frame->uv_width, 1, yframe);

  fclose(yframe);
}
#endif

void vp9_initialize_dec() {
  static int init_done = 0;

  if (!init_done) {
    vp9_initialize_common();
    vp9_init_quant_tables();
    init_done = 1;
  }
}

VP9D_PTR vp9_create_decompressor(VP9D_CONFIG *oxcf) {
  VP9D_COMP *pbi = vpx_memalign(32, sizeof(VP9D_COMP));

  if (!pbi)
    return NULL;

  vpx_memset(pbi, 0, sizeof(VP9D_COMP));

  if (setjmp(pbi->common.error.jmp)) {
    pbi->common.error.setjmp = 0;
    vp9_remove_decompressor(pbi);
    return 0;
  }

  pbi->common.error.setjmp = 1;
  vp9_initialize_dec();

  vp9_create_common(&pbi->common);
  pbi->oxcf = *oxcf;

  pbi->common.current_video_frame = 0;
  pbi->ready_for_new_data = 1;

  /* vp9_init_de_quantizer() is first called here. Add check in
   * frame_init_dequantizer() to avoid unnecessary calling of
   * vp9_init_de_quantizer() for every frame.
   */
  vp9_init_de_quantizer(pbi);

  vp9_loop_filter_init(&pbi->common);

  pbi->common.error.setjmp = 0;

  pbi->decoded_key_frame = 0;

  return (VP9D_PTR) pbi;
}

void vp9_remove_decompressor(VP9D_PTR ptr) {
  VP9D_COMP *pbi = (VP9D_COMP *) ptr;

  if (!pbi)
    return;

  // Delete segmentation map
  if (pbi->common.last_frame_seg_map)
    vpx_free(pbi->common.last_frame_seg_map);

  vp9_remove_common(&pbi->common);
  vpx_free(pbi->mbc);
  vpx_free(pbi);
}

static int equal_dimensions(YV12_BUFFER_CONFIG *a, YV12_BUFFER_CONFIG *b) {
    return a->y_height == b->y_height && a->y_width == b->y_width &&
           a->uv_height == b->uv_height && a->uv_width == b->uv_width;
}

vpx_codec_err_t vp9_copy_reference_dec(VP9D_PTR ptr,
                                       VP9_REFFRAME ref_frame_flag,
                                       YV12_BUFFER_CONFIG *sd) {
  VP9D_COMP *pbi = (VP9D_COMP *) ptr;
  VP9_COMMON *cm = &pbi->common;
  int ref_fb_idx;

  /* TODO(jkoleszar): The decoder doesn't have any real knowledge of what the
   * encoder is using the frame buffers for. This is just a stub to keep the
   * vpxenc --test-decode functionality working, and will be replaced in a
   * later commit that adds VP9-specific controls for this functionality.
   */
  if (ref_frame_flag == VP9_LAST_FLAG) {
    ref_fb_idx = pbi->common.ref_frame_map[0];
  } else {
    vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR,
                       "Invalid reference frame");
    return pbi->common.error.error_code;
  }

  if (!equal_dimensions(&cm->yv12_fb[ref_fb_idx], sd)) {
    vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR,
                       "Incorrect buffer dimensions");
  } else {
    vp8_yv12_copy_frame(&cm->yv12_fb[ref_fb_idx], sd);
  }

  return pbi->common.error.error_code;
}


vpx_codec_err_t vp9_set_reference_dec(VP9D_PTR ptr, VP9_REFFRAME ref_frame_flag,
                                      YV12_BUFFER_CONFIG *sd) {
  VP9D_COMP *pbi = (VP9D_COMP *) ptr;
  VP9_COMMON *cm = &pbi->common;
  int *ref_fb_ptr = NULL;

  /* TODO(jkoleszar): The decoder doesn't have any real knowledge of what the
   * encoder is using the frame buffers for. This is just a stub to keep the
   * vpxenc --test-decode functionality working, and will be replaced in a
   * later commit that adds VP9-specific controls for this functionality.
   */
  if (ref_frame_flag == VP9_LAST_FLAG)
    ref_fb_ptr = &pbi->common.active_ref_idx[0];
  else if (ref_frame_flag == VP9_GOLD_FLAG)
    ref_fb_ptr = &pbi->common.active_ref_idx[1];
  else if (ref_frame_flag == VP9_ALT_FLAG)
    ref_fb_ptr = &pbi->common.active_ref_idx[2];
  else {
    vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR,
                       "Invalid reference frame");
    return pbi->common.error.error_code;
  }

  if (!equal_dimensions(&cm->yv12_fb[*ref_fb_ptr], sd)) {
    vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR,
                       "Incorrect buffer dimensions");
  } else {
    // Find an empty frame buffer.
    const int free_fb = get_free_fb(cm);
    // Decrease fb_idx_ref_cnt since it will be increased again in
    // ref_cnt_fb() below.
    cm->fb_idx_ref_cnt[free_fb]--;

    // Manage the reference counters and copy image.
    ref_cnt_fb(cm->fb_idx_ref_cnt, ref_fb_ptr, free_fb);
    vp8_yv12_copy_frame(sd, &cm->yv12_fb[*ref_fb_ptr]);
  }

  return pbi->common.error.error_code;
}


int vp9_get_reference_dec(VP9D_PTR ptr, int index, YV12_BUFFER_CONFIG **fb) {
  VP9D_COMP *pbi = (VP9D_COMP *) ptr;
  VP9_COMMON *cm = &pbi->common;

  if (index < 0 || index >= NUM_REF_FRAMES)
    return -1;

  *fb = &cm->yv12_fb[cm->ref_frame_map[index]];
  return 0;
}

/* If any buffer updating is signalled it should be done here. */
static void swap_frame_buffers(VP9D_COMP *pbi) {
  int ref_index = 0, mask;

  for (mask = pbi->refresh_frame_flags; mask; mask >>= 1) {
    if (mask & 1) {
      ref_cnt_fb(pbi->common.fb_idx_ref_cnt,
                 &pbi->common.ref_frame_map[ref_index],
                 pbi->common.new_fb_idx);
    }
    ++ref_index;
  }

  pbi->common.frame_to_show = &pbi->common.yv12_fb[pbi->common.new_fb_idx];
  pbi->common.fb_idx_ref_cnt[pbi->common.new_fb_idx]--;

  /* Invalidate these references until the next frame starts. */
  for (ref_index = 0; ref_index < 3; ref_index++)
    pbi->common.active_ref_idx[ref_index] = INT_MAX;
}

int vp9_receive_compressed_data(VP9D_PTR ptr, unsigned long size,
                                const unsigned char **psource,
                                int64_t time_stamp) {
  VP9D_COMP *pbi = (VP9D_COMP *) ptr;
  VP9_COMMON *cm = &pbi->common;
  const unsigned char *source = *psource;
  int retcode = 0;

  /*if(pbi->ready_for_new_data == 0)
      return -1;*/

  if (ptr == 0) {
    return -1;
  }

  pbi->common.error.error_code = VPX_CODEC_OK;

  pbi->Source = source;
  pbi->source_sz = size;

  if (pbi->source_sz == 0) {
    /* This is used to signal that we are missing frames.
     * We do not know if the missing frame(s) was supposed to update
     * any of the reference buffers, but we act conservative and
     * mark only the last buffer as corrupted.
     *
     * TODO(jkoleszar): Error concealment is undefined and non-normative
     * at this point, but if it becomes so, [0] may not always be the correct
     * thing to do here.
     */
    if (cm->active_ref_idx[0] != INT_MAX)
      cm->yv12_fb[cm->active_ref_idx[0]].corrupted = 1;
  }

  cm->new_fb_idx = get_free_fb(cm);

  if (setjmp(pbi->common.error.jmp)) {
    pbi->common.error.setjmp = 0;

    /* We do not know if the missing frame(s) was supposed to update
     * any of the reference buffers, but we act conservative and
     * mark only the last buffer as corrupted.
     *
     * TODO(jkoleszar): Error concealment is undefined and non-normative
     * at this point, but if it becomes so, [0] may not always be the correct
     * thing to do here.
     */
    if (cm->active_ref_idx[0] != INT_MAX)
      cm->yv12_fb[cm->active_ref_idx[0]].corrupted = 1;

    if (cm->fb_idx_ref_cnt[cm->new_fb_idx] > 0)
      cm->fb_idx_ref_cnt[cm->new_fb_idx]--;
    return -1;
  }

  pbi->common.error.setjmp = 1;

  retcode = vp9_decode_frame(pbi, psource);

  if (retcode < 0) {
    pbi->common.error.error_code = VPX_CODEC_ERROR;
    pbi->common.error.setjmp = 0;
    if (cm->fb_idx_ref_cnt[cm->new_fb_idx] > 0)
      cm->fb_idx_ref_cnt[cm->new_fb_idx]--;
    return retcode;
  }

  {
    swap_frame_buffers(pbi);

#if WRITE_RECON_BUFFER == 2
    if (cm->show_frame)
      write_dx_frame_to_file(cm->frame_to_show,
                             cm->current_video_frame);
    else
      write_dx_frame_to_file(cm->frame_to_show,
                             cm->current_video_frame + 1000);
#endif

    if (cm->filter_level) {
      /* Apply the loop filter if appropriate. */
      vp9_loop_filter_frame(cm, &pbi->mb, cm->filter_level, 0,
                            cm->dering_enabled);
    }
    vp8_yv12_extend_frame_borders(cm->frame_to_show);
  }

#if WRITE_RECON_BUFFER == 1
  if (cm->show_frame)
    recon_write_yuv_frame("recon.yuv", cm->frame_to_show,
                          cm->width, cm->height);
#endif

  vp9_clear_system_state();

  if (cm->show_frame) {
    vpx_memcpy(cm->prev_mip, cm->mip,
               (cm->mb_cols + 1) * (cm->mb_rows + 1)* sizeof(MODE_INFO));
  } else {
    vpx_memset(cm->prev_mip, 0,
               (cm->mb_cols + 1) * (cm->mb_rows + 1)* sizeof(MODE_INFO));
  }

  /*vp9_print_modes_and_motion_vectors(cm->mi, cm->mb_rows,cm->mb_cols,
                                       cm->current_video_frame);*/

  if (cm->show_frame)
    cm->current_video_frame++;

  pbi->ready_for_new_data = 0;
  pbi->last_time_stamp = time_stamp;
  pbi->source_sz = 0;

  pbi->common.error.setjmp = 0;
  return retcode;
}

int vp9_get_raw_frame(VP9D_PTR ptr, YV12_BUFFER_CONFIG *sd,
                      int64_t *time_stamp, int64_t *time_end_stamp,
                      vp9_ppflags_t *flags) {
  int ret = -1;
  VP9D_COMP *pbi = (VP9D_COMP *) ptr;

  if (pbi->ready_for_new_data == 1)
    return ret;

  /* ie no raw frame to show!!! */
  if (pbi->common.show_frame == 0)
    return ret;

  pbi->ready_for_new_data = 1;
  *time_stamp = pbi->last_time_stamp;
  *time_end_stamp = 0;

  sd->clrtype = pbi->common.clr_type;
#if CONFIG_POSTPROC
  ret = vp9_post_proc_frame(&pbi->common, sd, flags);
#else

  if (pbi->common.frame_to_show) {
    *sd = *pbi->common.frame_to_show;
    sd->y_width = pbi->common.width;
    sd->y_height = pbi->common.height;
    sd->uv_height = pbi->common.height / 2;
    ret = 0;
  } else {
    ret = -1;
  }

#endif /*!CONFIG_POSTPROC*/
  vp9_clear_system_state();
  return ret;
}
