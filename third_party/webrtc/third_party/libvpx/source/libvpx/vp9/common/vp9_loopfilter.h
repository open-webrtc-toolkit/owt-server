/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_LOOPFILTER_H_
#define VP9_COMMON_VP9_LOOPFILTER_H_

#include "vpx_ports/mem.h"
#include "vpx_config.h"
#include "vp9/common/vp9_blockd.h"

#define MAX_LOOP_FILTER 63

typedef enum {
  NORMAL_LOOPFILTER = 0,
  SIMPLE_LOOPFILTER = 1
} LOOPFILTERTYPE;

#define SIMD_WIDTH 16

/* Need to align this structure so when it is declared and
 * passed it can be loaded into vector registers.
 */
typedef struct {
  DECLARE_ALIGNED(SIMD_WIDTH, unsigned char,
                  mblim[MAX_LOOP_FILTER + 1][SIMD_WIDTH]);
  DECLARE_ALIGNED(SIMD_WIDTH, unsigned char,
                  blim[MAX_LOOP_FILTER + 1][SIMD_WIDTH]);
  DECLARE_ALIGNED(SIMD_WIDTH, unsigned char,
                  lim[MAX_LOOP_FILTER + 1][SIMD_WIDTH]);
  DECLARE_ALIGNED(SIMD_WIDTH, unsigned char,
                  hev_thr[4][SIMD_WIDTH]);
  unsigned char lvl[4][4][4];
  unsigned char hev_thr_lut[2][MAX_LOOP_FILTER + 1];
  unsigned char mode_lf_lut[MB_MODE_COUNT];
} loop_filter_info_n;

struct loop_filter_info {
  const unsigned char *mblim;
  const unsigned char *blim;
  const unsigned char *lim;
  const unsigned char *hev_thr;
};

#define prototype_loopfilter(sym) \
  void sym(uint8_t *src, int pitch, const unsigned char *blimit, \
           const unsigned char *limit, const unsigned char *thresh, int count)

#define prototype_loopfilter_block(sym) \
  void sym(uint8_t *y, uint8_t *u, uint8_t *v, \
           int ystride, int uv_stride, struct loop_filter_info *lfi)

#define prototype_simple_loopfilter(sym) \
  void sym(uint8_t *y, int ystride, const unsigned char *blimit)

#if ARCH_X86 || ARCH_X86_64
#include "x86/vp9_loopfilter_x86.h"
#endif

typedef void loop_filter_uvfunction(uint8_t *u,   /* source pointer */
                                    int p,              /* pitch */
                                    const unsigned char *blimit,
                                    const unsigned char *limit,
                                    const unsigned char *thresh,
                                    uint8_t *v);

/* assorted loopfilter functions which get used elsewhere */
struct VP9Common;
struct macroblockd;

void vp9_loop_filter_init(struct VP9Common *cm);

void vp9_loop_filter_frame_init(struct VP9Common *cm,
                                struct macroblockd *mbd,
                                int default_filt_lvl);

void vp9_loop_filter_frame(struct VP9Common *cm,
                           struct macroblockd *mbd,
                           int filter_level,
                           int y_only);

void vp9_loop_filter_partial_frame(struct VP9Common *cm,
                                   struct macroblockd *mbd,
                                   int default_filt_lvl);

void vp9_loop_filter_update_sharpness(loop_filter_info_n *lfi,
                                      int sharpness_lvl);

void vp9_mb_lpf_horizontal_edge_w(unsigned char *s, int p,
                                  const unsigned char *blimit,
                                  const unsigned char *limit,
                                  const unsigned char *thresh,
                                  int count);

void vp9_mb_lpf_vertical_edge_w(unsigned char *s, int p,
                                const unsigned char *blimit,
                                const unsigned char *limit,
                                const unsigned char *thresh,
                                int count);
#endif  // VP9_COMMON_VP9_LOOPFILTER_H_
