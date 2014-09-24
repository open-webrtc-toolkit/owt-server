/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_COMMON_X86_VP9_IDCT_X86_H_
#define VP9_COMMON_X86_VP9_IDCT_X86_H_

/* Note:
 *
 * This platform is commonly built for runtime CPU detection. If you modify
 * any of the function mappings present in this file, be sure to also update
 * them in the function pointer initialization code
 */

#if HAVE_MMX
extern prototype_idct(vp9_short_idct4x4llm_1_mmx);
extern prototype_idct(vp9_short_idct4x4llm_mmx);
extern prototype_idct_scalar_add(vp9_dc_only_idct_add_mmx);

extern prototype_second_order(vp9_short_inv_walsh4x4_mmx);
extern prototype_second_order(vp9_short_inv_walsh4x4_1_mmx);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp9_idct_idct1
#define vp9_idct_idct1 vp9_short_idct4x4llm_1_mmx

#undef  vp9_idct_idct16
#define vp9_idct_idct16 vp9_short_idct4x4llm_mmx

#undef  vp9_idct_idct1_scalar_add
#define vp9_idct_idct1_scalar_add vp9_dc_only_idct_add_mmx

#undef vp9_idct_iwalsh16
#define vp9_idct_iwalsh16 vp9_short_inv_walsh4x4_mmx

#undef vp9_idct_iwalsh1
#define vp9_idct_iwalsh1 vp9_short_inv_walsh4x4_1_mmx

#endif
#endif

#if HAVE_SSE2

extern prototype_second_order(vp9_short_inv_walsh4x4_sse2);

#if !CONFIG_RUNTIME_CPU_DETECT

#undef vp9_idct_iwalsh16
#define vp9_idct_iwalsh16 vp9_short_inv_walsh4x4_sse2

#endif

#endif



#endif
