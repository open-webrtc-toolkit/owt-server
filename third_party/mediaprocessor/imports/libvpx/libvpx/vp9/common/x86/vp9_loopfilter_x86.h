/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_X86_VP9_LOOPFILTER_X86_H_
#define VP9_COMMON_X86_VP9_LOOPFILTER_X86_H_

/* Note:
 *
 * This platform is commonly built for runtime CPU detection. If you modify
 * any of the function mappings present in this file, be sure to also update
 * them in the function pointer initialization code
 */

#if HAVE_MMX
extern prototype_loopfilter_block(vp9_loop_filter_mbv_mmx);
extern prototype_loopfilter_block(vp9_loop_filter_bv_mmx);
extern prototype_loopfilter_block(vp9_loop_filter_mbh_mmx);
extern prototype_loopfilter_block(vp9_loop_filter_bh_mmx);
extern prototype_simple_loopfilter(vp9_loop_filter_simple_vertical_edge_mmx);
extern prototype_simple_loopfilter(vp9_loop_filter_bvs_mmx);
extern prototype_simple_loopfilter(vp9_loop_filter_simple_horizontal_edge_mmx);
extern prototype_simple_loopfilter(vp9_loop_filter_bhs_mmx);
#endif

#if HAVE_SSE2
extern prototype_loopfilter_block(vp9_loop_filter_mbv_sse2);
extern prototype_loopfilter_block(vp9_loop_filter_bv_sse2);
extern prototype_loopfilter_block(vp9_loop_filter_mbh_sse2);
extern prototype_loopfilter_block(vp9_loop_filter_bh_sse2);
extern prototype_simple_loopfilter(vp9_loop_filter_simple_vertical_edge_sse2);
extern prototype_simple_loopfilter(vp9_loop_filter_bvs_sse2);
extern prototype_simple_loopfilter(vp9_loop_filter_simple_horizontal_edge_sse2);
extern prototype_simple_loopfilter(vp9_loop_filter_bhs_sse2);
#endif

#endif  // LOOPFILTER_X86_H
