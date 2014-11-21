/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_DECODER_VP9_ONYXD_INT_H_
#define VP9_DECODER_VP9_ONYXD_INT_H_
#include "./vpx_config.h"
#include "vp9/decoder/vp9_onyxd.h"
#include "vp9/decoder/vp9_treereader.h"
#include "vp9/common/vp9_onyxc_int.h"
#include "vp9/decoder/vp9_dequantize.h"

// #define DEC_DEBUG

typedef struct VP9Decompressor {
  DECLARE_ALIGNED(16, MACROBLOCKD, mb);

  DECLARE_ALIGNED(16, VP9_COMMON, common);

  VP9D_CONFIG oxcf;


  const unsigned char *Source;
  unsigned int   source_sz;

  vp9_reader *mbc;
  int64_t last_time_stamp;
  int   ready_for_new_data;

  int refresh_frame_flags;
  vp9_prob prob_skip_false;

  int decoded_key_frame;

  int initial_width;
  int initial_height;
} VP9D_COMP;

int vp9_decode_frame(VP9D_COMP *cpi, const unsigned char **p_data_end);


#if CONFIG_DEBUG
#define CHECK_MEM_ERROR(lval,expr) do {\
    lval = (expr); \
    if(!lval) \
      vpx_internal_error(&pbi->common.error, VPX_CODEC_MEM_ERROR,\
                         "Failed to allocate "#lval" at %s:%d", \
                         __FILE__,__LINE__);\
  } while(0)
#else
#define CHECK_MEM_ERROR(lval,expr) do {\
    lval = (expr); \
    if(!lval) \
      vpx_internal_error(&pbi->common.error, VPX_CODEC_MEM_ERROR,\
                         "Failed to allocate "#lval);\
  } while(0)
#endif

#endif  // VP9_DECODER_VP9_TREEREADER_H_
