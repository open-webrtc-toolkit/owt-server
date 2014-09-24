/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <stdlib.h>
#include <string.h>
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"
#include "vpx/internal/vpx_codec_internal.h"
#include "vpx_version.h"
#include "decoder/vp9_onyxd.h"
#include "decoder/vp9_onyxd_int.h"

#define VP8_CAP_POSTPROC (CONFIG_POSTPROC ? VPX_CODEC_CAP_POSTPROC : 0)
typedef vpx_codec_stream_info_t  vp8_stream_info_t;

/* Structures for handling memory allocations */
typedef enum {
  VP8_SEG_ALG_PRIV     = 256,
  VP8_SEG_MAX
} mem_seg_id_t;
#define NELEMENTS(x) ((int)(sizeof(x)/sizeof(x[0])))

static unsigned long vp8_priv_sz(const vpx_codec_dec_cfg_t *si, vpx_codec_flags_t);

typedef struct {
  unsigned int   id;
  unsigned long  sz;
  unsigned int   align;
  unsigned int   flags;
  unsigned long(*calc_sz)(const vpx_codec_dec_cfg_t *, vpx_codec_flags_t);
} mem_req_t;

static const mem_req_t vp8_mem_req_segs[] = {
  {VP8_SEG_ALG_PRIV,    0, 8, VPX_CODEC_MEM_ZERO, vp8_priv_sz},
  {VP8_SEG_MAX, 0, 0, 0, NULL}
};

struct vpx_codec_alg_priv {
  vpx_codec_priv_t        base;
  vpx_codec_mmap_t        mmaps[NELEMENTS(vp8_mem_req_segs) - 1];
  vpx_codec_dec_cfg_t     cfg;
  vp8_stream_info_t       si;
  int                     defer_alloc;
  int                     decoder_init;
  VP9D_PTR                pbi;
  int                     postproc_cfg_set;
  vp8_postproc_cfg_t      postproc_cfg;
#if CONFIG_POSTPROC_VISUALIZER
  unsigned int            dbg_postproc_flag;
  int                     dbg_color_ref_frame_flag;
  int                     dbg_color_mb_modes_flag;
  int                     dbg_color_b_modes_flag;
  int                     dbg_display_mv_flag;
#endif
  vpx_image_t             img;
  int                     img_setup;
  int                     img_avail;
};

static unsigned long vp8_priv_sz(const vpx_codec_dec_cfg_t *si,
                                 vpx_codec_flags_t flags) {
  /* Although this declaration is constant, we can't use it in the requested
   * segments list because we want to define the requested segments list
   * before defining the private type (so that the number of memory maps is
   * known)
   */
  (void)si;
  return sizeof(vpx_codec_alg_priv_t);
}


static void vp8_mmap_dtor(vpx_codec_mmap_t *mmap) {
  free(mmap->priv);
}

static vpx_codec_err_t vp8_mmap_alloc(vpx_codec_mmap_t *mmap) {
  vpx_codec_err_t  res;
  unsigned int   align;

  align = mmap->align ? mmap->align - 1 : 0;

  if (mmap->flags & VPX_CODEC_MEM_ZERO)
    mmap->priv = calloc(1, mmap->sz + align);
  else
    mmap->priv = malloc(mmap->sz + align);

  res = (mmap->priv) ? VPX_CODEC_OK : VPX_CODEC_MEM_ERROR;
  mmap->base = (void *)((((uintptr_t)mmap->priv) + align) & ~(uintptr_t)align);
  mmap->dtor = vp8_mmap_dtor;
  return res;
}

static vpx_codec_err_t vp8_validate_mmaps(const vp8_stream_info_t *si,
                                          const vpx_codec_mmap_t *mmaps,
                                          vpx_codec_flags_t init_flags) {
  int i;
  vpx_codec_err_t res = VPX_CODEC_OK;

  for (i = 0; i < NELEMENTS(vp8_mem_req_segs) - 1; i++) {
    /* Ensure the segment has been allocated */
    if (!mmaps[i].base) {
      res = VPX_CODEC_MEM_ERROR;
      break;
    }

    /* Verify variable size segment is big enough for the current si. */
    if (vp8_mem_req_segs[i].calc_sz) {
      vpx_codec_dec_cfg_t cfg;

      cfg.w = si->w;
      cfg.h = si->h;

      if (mmaps[i].sz < vp8_mem_req_segs[i].calc_sz(&cfg, init_flags)) {
        res = VPX_CODEC_MEM_ERROR;
        break;
      }
    }
  }

  return res;
}

static void vp8_init_ctx(vpx_codec_ctx_t *ctx, const vpx_codec_mmap_t *mmap) {
  int i;

  ctx->priv = mmap->base;
  ctx->priv->sz = sizeof(*ctx->priv);
  ctx->priv->iface = ctx->iface;
  ctx->priv->alg_priv = mmap->base;

  for (i = 0; i < NELEMENTS(ctx->priv->alg_priv->mmaps); i++)
    ctx->priv->alg_priv->mmaps[i].id = vp8_mem_req_segs[i].id;

  ctx->priv->alg_priv->mmaps[0] = *mmap;
  ctx->priv->alg_priv->si.sz = sizeof(ctx->priv->alg_priv->si);
  ctx->priv->init_flags = ctx->init_flags;

  if (ctx->config.dec) {
    /* Update the reference to the config structure to an internal copy. */
    ctx->priv->alg_priv->cfg = *ctx->config.dec;
    ctx->config.dec = &ctx->priv->alg_priv->cfg;
  }
}

static void *mmap_lkup(vpx_codec_alg_priv_t *ctx, unsigned int id) {
  int i;

  for (i = 0; i < NELEMENTS(ctx->mmaps); i++)
    if (ctx->mmaps[i].id == id)
      return ctx->mmaps[i].base;

  return NULL;
}
static void vp8_finalize_mmaps(vpx_codec_alg_priv_t *ctx) {
  /* nothing to clean up */
}

static vpx_codec_err_t vp8_init(vpx_codec_ctx_t *ctx,
                                vpx_codec_priv_enc_mr_cfg_t *data) {
  vpx_codec_err_t        res = VPX_CODEC_OK;

  /* This function only allocates space for the vpx_codec_alg_priv_t
   * structure. More memory may be required at the time the stream
   * information becomes known.
   */
  if (!ctx->priv) {
    vpx_codec_mmap_t mmap;

    mmap.id = vp8_mem_req_segs[0].id;
    mmap.sz = sizeof(vpx_codec_alg_priv_t);
    mmap.align = vp8_mem_req_segs[0].align;
    mmap.flags = vp8_mem_req_segs[0].flags;

    res = vp8_mmap_alloc(&mmap);

    if (!res) {
      vp8_init_ctx(ctx, &mmap);

      ctx->priv->alg_priv->defer_alloc = 1;
      /*post processing level initialized to do nothing */
    }
  }

  return res;
}

static vpx_codec_err_t vp8_destroy(vpx_codec_alg_priv_t *ctx) {
  int i;

  vp9_remove_decompressor(ctx->pbi);

  for (i = NELEMENTS(ctx->mmaps) - 1; i >= 0; i--) {
    if (ctx->mmaps[i].dtor)
      ctx->mmaps[i].dtor(&ctx->mmaps[i]);
  }

  return VPX_CODEC_OK;
}

static vpx_codec_err_t vp8_peek_si(const uint8_t         *data,
                                   unsigned int           data_sz,
                                   vpx_codec_stream_info_t *si) {
  vpx_codec_err_t res = VPX_CODEC_OK;

  if (data + data_sz <= data)
    res = VPX_CODEC_INVALID_PARAM;
  else {
    /* Parse uncompresssed part of key frame header.
     * 3 bytes:- including version, frame type and an offset
     * 3 bytes:- sync code (0x9d, 0x01, 0x2a)
     * 4 bytes:- including image width and height in the lowest 14 bits
     *           of each 2-byte value.
     */
    si->is_kf = 0;

    if (data_sz >= 10 && !(data[0] & 0x01)) { /* I-Frame */
      const uint8_t *c = data + 3;
      si->is_kf = 1;

      /* vet via sync code */
      if (c[0] != 0x9d || c[1] != 0x01 || c[2] != 0x2a)
        res = VPX_CODEC_UNSUP_BITSTREAM;

      si->w = (c[3] | (c[4] << 8)) & 0x3fff;
      si->h = (c[5] | (c[6] << 8)) & 0x3fff;

      /*printf("w=%d, h=%d\n", si->w, si->h);*/
      if (!(si->h | si->w))
        res = VPX_CODEC_UNSUP_BITSTREAM;
    } else
      res = VPX_CODEC_UNSUP_BITSTREAM;
  }

  return res;

}

static vpx_codec_err_t vp8_get_si(vpx_codec_alg_priv_t    *ctx,
                                  vpx_codec_stream_info_t *si) {

  unsigned int sz;

  if (si->sz >= sizeof(vp8_stream_info_t))
    sz = sizeof(vp8_stream_info_t);
  else
    sz = sizeof(vpx_codec_stream_info_t);

  memcpy(si, &ctx->si, sz);
  si->sz = sz;

  return VPX_CODEC_OK;
}


static vpx_codec_err_t
update_error_state(vpx_codec_alg_priv_t                 *ctx,
                   const struct vpx_internal_error_info *error) {
  vpx_codec_err_t res;

  if ((res = error->error_code))
    ctx->base.err_detail = error->has_detail
                           ? error->detail
                           : NULL;

  return res;
}

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

static vpx_codec_err_t decode_one(vpx_codec_alg_priv_t  *ctx,
                                  const uint8_t        **data,
                                  unsigned int           data_sz,
                                  void                  *user_priv,
                                  long                   deadline) {
  vpx_codec_err_t res = VPX_CODEC_OK;

  ctx->img_avail = 0;

  /* Determine the stream parameters. Note that we rely on peek_si to
   * validate that we have a buffer that does not wrap around the top
   * of the heap.
   */
  if (!ctx->si.h)
    res = ctx->base.iface->dec.peek_si(*data, data_sz, &ctx->si);


  /* Perform deferred allocations, if required */
  if (!res && ctx->defer_alloc) {
    int i;

    for (i = 1; !res && i < NELEMENTS(ctx->mmaps); i++) {
      vpx_codec_dec_cfg_t cfg;

      cfg.w = ctx->si.w;
      cfg.h = ctx->si.h;
      ctx->mmaps[i].id = vp8_mem_req_segs[i].id;
      ctx->mmaps[i].sz = vp8_mem_req_segs[i].sz;
      ctx->mmaps[i].align = vp8_mem_req_segs[i].align;
      ctx->mmaps[i].flags = vp8_mem_req_segs[i].flags;

      if (!ctx->mmaps[i].sz)
        ctx->mmaps[i].sz = vp8_mem_req_segs[i].calc_sz(&cfg,
                                                       ctx->base.init_flags);

      res = vp8_mmap_alloc(&ctx->mmaps[i]);
    }

    if (!res)
      vp8_finalize_mmaps(ctx);

    ctx->defer_alloc = 0;
  }

  /* Initialize the decoder instance on the first frame*/
  if (!res && !ctx->decoder_init) {
    res = vp8_validate_mmaps(&ctx->si, ctx->mmaps, ctx->base.init_flags);

    if (!res) {
      VP9D_CONFIG oxcf;
      VP9D_PTR optr;

      vp9_initialize_dec();

      oxcf.Width = ctx->si.w;
      oxcf.Height = ctx->si.h;
      oxcf.Version = 9;
      oxcf.postprocess = 0;
      oxcf.max_threads = ctx->cfg.threads;
      optr = vp9_create_decompressor(&oxcf);

      /* If postprocessing was enabled by the application and a
       * configuration has not been provided, default it.
       */
      if (!ctx->postproc_cfg_set
          && (ctx->base.init_flags & VPX_CODEC_USE_POSTPROC)) {
        ctx->postproc_cfg.post_proc_flag =
          VP8_DEBLOCK | VP8_DEMACROBLOCK;
        ctx->postproc_cfg.deblocking_level = 4;
        ctx->postproc_cfg.noise_level = 0;
      }

      if (!optr)
        res = VPX_CODEC_ERROR;
      else
        ctx->pbi = optr;
    }

    ctx->decoder_init = 1;
  }

  if (!res && ctx->pbi) {
    YV12_BUFFER_CONFIG sd;
    int64_t time_stamp = 0, time_end_stamp = 0;
    vp9_ppflags_t flags = {0};

    if (ctx->base.init_flags & VPX_CODEC_USE_POSTPROC) {
      flags.post_proc_flag = ctx->postproc_cfg.post_proc_flag
#if CONFIG_POSTPROC_VISUALIZER

                             | ((ctx->dbg_color_ref_frame_flag != 0) ? VP9D_DEBUG_CLR_FRM_REF_BLKS : 0)
                             | ((ctx->dbg_color_mb_modes_flag != 0) ? VP9D_DEBUG_CLR_BLK_MODES : 0)
                             | ((ctx->dbg_color_b_modes_flag != 0) ? VP9D_DEBUG_CLR_BLK_MODES : 0)
                             | ((ctx->dbg_display_mv_flag != 0) ? VP9D_DEBUG_DRAW_MV : 0)
#endif
;
      flags.deblocking_level      = ctx->postproc_cfg.deblocking_level;
      flags.noise_level           = ctx->postproc_cfg.noise_level;
#if CONFIG_POSTPROC_VISUALIZER
      flags.display_ref_frame_flag = ctx->dbg_color_ref_frame_flag;
      flags.display_mb_modes_flag = ctx->dbg_color_mb_modes_flag;
      flags.display_b_modes_flag  = ctx->dbg_color_b_modes_flag;
      flags.display_mv_flag       = ctx->dbg_display_mv_flag;
#endif
    }

    if (vp9_receive_compressed_data(ctx->pbi, data_sz, data, deadline)) {
      VP9D_COMP *pbi = (VP9D_COMP *)ctx->pbi;
      res = update_error_state(ctx, &pbi->common.error);
    }

    if (!res && 0 == vp9_get_raw_frame(ctx->pbi, &sd, &time_stamp,
                                       &time_end_stamp, &flags)) {
      yuvconfig2image(&ctx->img, &sd, user_priv);
      ctx->img_avail = 1;
    }
  }

  return res;
}

static vpx_codec_err_t vp9_decode(vpx_codec_alg_priv_t  *ctx,
                                  const uint8_t         *data,
                                  unsigned int           data_sz,
                                  void                  *user_priv,
                                  long                   deadline) {
  const uint8_t *data_start = data;
  const uint8_t *data_end = data + data_sz;
  vpx_codec_err_t res;

  do {
    res = decode_one(ctx, &data_start, data_sz, user_priv, deadline);
    assert(data_start >= data);
    assert(data_start <= data_end);

    /* Early exit if there was a decode error */
    if (res)
      break;

    /* Account for suboptimal termination by the encoder. */
    while (data_start < data_end && *data_start == 0)
      data_start++;

    data_sz = data_end - data_start;
  } while (data_start < data_end);
  return res;
}

static vpx_image_t *vp8_get_frame(vpx_codec_alg_priv_t  *ctx,
                                  vpx_codec_iter_t      *iter) {
  vpx_image_t *img = NULL;

  if (ctx->img_avail) {
    /* iter acts as a flip flop, so an image is only returned on the first
     * call to get_frame.
     */
    if (!(*iter)) {
      img = &ctx->img;
      *iter = img;
    }
  }

  return img;
}


static
vpx_codec_err_t vp8_xma_get_mmap(const vpx_codec_ctx_t      *ctx,
                                 vpx_codec_mmap_t           *mmap,
                                 vpx_codec_iter_t           *iter) {
  vpx_codec_err_t     res;
  const mem_req_t  *seg_iter = *iter;

  /* Get address of next segment request */
  do {
    if (!seg_iter)
      seg_iter = vp8_mem_req_segs;
    else if (seg_iter->id != VP8_SEG_MAX)
      seg_iter++;

    *iter = (vpx_codec_iter_t)seg_iter;

    if (seg_iter->id != VP8_SEG_MAX) {
      mmap->id = seg_iter->id;
      mmap->sz = seg_iter->sz;
      mmap->align = seg_iter->align;
      mmap->flags = seg_iter->flags;

      if (!seg_iter->sz)
        mmap->sz = seg_iter->calc_sz(ctx->config.dec, ctx->init_flags);

      res = VPX_CODEC_OK;
    } else
      res = VPX_CODEC_LIST_END;
  } while (!mmap->sz && res != VPX_CODEC_LIST_END);

  return res;
}

static vpx_codec_err_t vp8_xma_set_mmap(vpx_codec_ctx_t         *ctx,
                                        const vpx_codec_mmap_t  *mmap) {
  vpx_codec_err_t res = VPX_CODEC_MEM_ERROR;
  int i, done;

  if (!ctx->priv) {
    if (mmap->id == VP8_SEG_ALG_PRIV) {
      if (!ctx->priv) {
        vp8_init_ctx(ctx, mmap);
        res = VPX_CODEC_OK;
      }
    }
  }

  done = 1;

  if (!res && ctx->priv->alg_priv) {
    for (i = 0; i < NELEMENTS(ctx->priv->alg_priv->mmaps); i++) {
      if (ctx->priv->alg_priv->mmaps[i].id == mmap->id)
        if (!ctx->priv->alg_priv->mmaps[i].base) {
          ctx->priv->alg_priv->mmaps[i] = *mmap;
          res = VPX_CODEC_OK;
        }

      done &= (ctx->priv->alg_priv->mmaps[i].base != NULL);
    }
  }

  if (done && !res) {
    vp8_finalize_mmaps(ctx->priv->alg_priv);
    res = ctx->iface->init(ctx, NULL);
  }

  return res;
}

static vpx_codec_err_t image2yuvconfig(const vpx_image_t   *img,
                                       YV12_BUFFER_CONFIG  *yv12) {
  vpx_codec_err_t        res = VPX_CODEC_OK;
  yv12->y_buffer = img->planes[VPX_PLANE_Y];
  yv12->u_buffer = img->planes[VPX_PLANE_U];
  yv12->v_buffer = img->planes[VPX_PLANE_V];

  yv12->y_width  = img->d_w;
  yv12->y_height = img->d_h;
  yv12->uv_width = yv12->y_width / 2;
  yv12->uv_height = yv12->y_height / 2;

  yv12->y_stride = img->stride[VPX_PLANE_Y];
  yv12->uv_stride = img->stride[VPX_PLANE_U];

  yv12->border  = (img->stride[VPX_PLANE_Y] - img->d_w) / 2;
  yv12->clrtype = (img->fmt == VPX_IMG_FMT_VPXI420 ||
                   img->fmt == VPX_IMG_FMT_VPXYV12);

  return res;
}


static vpx_codec_err_t vp9_set_reference(vpx_codec_alg_priv_t *ctx,
                                         int ctr_id,
                                         va_list args) {

  vpx_ref_frame_t *data = va_arg(args, vpx_ref_frame_t *);

  if (data) {
    vpx_ref_frame_t *frame = (vpx_ref_frame_t *)data;
    YV12_BUFFER_CONFIG sd;

    image2yuvconfig(&frame->img, &sd);

    return vp9_set_reference_dec(ctx->pbi,
                                 (VP9_REFFRAME)frame->frame_type, &sd);
  } else
    return VPX_CODEC_INVALID_PARAM;

}

static vpx_codec_err_t vp9_get_reference(vpx_codec_alg_priv_t *ctx,
                                         int ctr_id,
                                         va_list args) {

  vpx_ref_frame_t *data = va_arg(args, vpx_ref_frame_t *);

  if (data) {
    vpx_ref_frame_t *frame = (vpx_ref_frame_t *)data;
    YV12_BUFFER_CONFIG sd;

    image2yuvconfig(&frame->img, &sd);

    return vp9_get_reference_dec(ctx->pbi,
                                 (VP9_REFFRAME)frame->frame_type, &sd);
  } else
    return VPX_CODEC_INVALID_PARAM;

}

static vpx_codec_err_t vp8_set_postproc(vpx_codec_alg_priv_t *ctx,
                                        int ctr_id,
                                        va_list args) {
#if CONFIG_POSTPROC
  vp8_postproc_cfg_t *data = va_arg(args, vp8_postproc_cfg_t *);

  if (data) {
    ctx->postproc_cfg_set = 1;
    ctx->postproc_cfg = *((vp8_postproc_cfg_t *)data);
    return VPX_CODEC_OK;
  } else
    return VPX_CODEC_INVALID_PARAM;

#else
  return VPX_CODEC_INCAPABLE;
#endif
}

static vpx_codec_err_t vp8_set_dbg_options(vpx_codec_alg_priv_t *ctx,
                                           int ctrl_id,
                                           va_list args) {
#if CONFIG_POSTPROC_VISUALIZER && CONFIG_POSTPROC
  int data = va_arg(args, int);

#define MAP(id, var) case id: var = data; break;

  switch (ctrl_id) {
      MAP(VP8_SET_DBG_COLOR_REF_FRAME,   ctx->dbg_color_ref_frame_flag);
      MAP(VP8_SET_DBG_COLOR_MB_MODES,    ctx->dbg_color_mb_modes_flag);
      MAP(VP8_SET_DBG_COLOR_B_MODES,     ctx->dbg_color_b_modes_flag);
      MAP(VP8_SET_DBG_DISPLAY_MV,        ctx->dbg_display_mv_flag);
  }

  return VPX_CODEC_OK;
#else
  return VPX_CODEC_INCAPABLE;
#endif
}

static vpx_codec_err_t vp8_get_last_ref_updates(vpx_codec_alg_priv_t *ctx,
                                                int ctrl_id,
                                                va_list args) {
  int *update_info = va_arg(args, int *);
  VP9D_COMP *pbi = (VP9D_COMP *)ctx->pbi;

  if (update_info) {
    *update_info = pbi->common.refresh_alt_ref_frame * (int) VP8_ALTR_FRAME
                   + pbi->common.refresh_golden_frame * (int) VP8_GOLD_FRAME
                   + pbi->common.refresh_last_frame * (int) VP8_LAST_FRAME;

    return VPX_CODEC_OK;
  } else
    return VPX_CODEC_INVALID_PARAM;
}


static vpx_codec_err_t vp8_get_frame_corrupted(vpx_codec_alg_priv_t *ctx,
                                               int ctrl_id,
                                               va_list args) {

  int *corrupted = va_arg(args, int *);

  if (corrupted) {
    VP9D_COMP *pbi = (VP9D_COMP *)ctx->pbi;
    *corrupted = pbi->common.frame_to_show->corrupted;

    return VPX_CODEC_OK;
  } else
    return VPX_CODEC_INVALID_PARAM;

}

static vpx_codec_ctrl_fn_map_t ctf_maps[] = {
  {VP8_SET_REFERENCE,             vp9_set_reference},
  {VP8_COPY_REFERENCE,            vp9_get_reference},
  {VP8_SET_POSTPROC,              vp8_set_postproc},
  {VP8_SET_DBG_COLOR_REF_FRAME,   vp8_set_dbg_options},
  {VP8_SET_DBG_COLOR_MB_MODES,    vp8_set_dbg_options},
  {VP8_SET_DBG_COLOR_B_MODES,     vp8_set_dbg_options},
  {VP8_SET_DBG_DISPLAY_MV,        vp8_set_dbg_options},
  {VP8D_GET_LAST_REF_UPDATES,     vp8_get_last_ref_updates},
  {VP8D_GET_FRAME_CORRUPTED,      vp8_get_frame_corrupted},
  { -1, NULL},
};


#ifndef VERSION_STRING
#define VERSION_STRING
#endif
CODEC_INTERFACE(vpx_codec_vp9_dx) = {
  "WebM Project VP9 Decoder" VERSION_STRING,
  VPX_CODEC_INTERNAL_ABI_VERSION,
  VPX_CODEC_CAP_DECODER | VP8_CAP_POSTPROC,
  /* vpx_codec_caps_t          caps; */
  vp8_init,         /* vpx_codec_init_fn_t       init; */
  vp8_destroy,      /* vpx_codec_destroy_fn_t    destroy; */
  ctf_maps,         /* vpx_codec_ctrl_fn_map_t  *ctrl_maps; */
  vp8_xma_get_mmap, /* vpx_codec_get_mmap_fn_t   get_mmap; */
  vp8_xma_set_mmap, /* vpx_codec_set_mmap_fn_t   set_mmap; */
  {
    vp8_peek_si,      /* vpx_codec_peek_si_fn_t    peek_si; */
    vp8_get_si,       /* vpx_codec_get_si_fn_t     get_si; */
    vp9_decode,       /* vpx_codec_decode_fn_t     decode; */
    vp8_get_frame,    /* vpx_codec_frame_get_fn_t  frame_get; */
  },
  {
    /* encoder functions */
    NOT_IMPLEMENTED,
    NOT_IMPLEMENTED,
    NOT_IMPLEMENTED,
    NOT_IMPLEMENTED,
    NOT_IMPLEMENTED,
    NOT_IMPLEMENTED
  }
};
