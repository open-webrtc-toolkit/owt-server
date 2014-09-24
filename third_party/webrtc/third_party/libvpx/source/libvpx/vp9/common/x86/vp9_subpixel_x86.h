/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_COMMON_X86_VP9_SUBPIXEL_X86_H_
#define VP9_COMMON_X86_VP9_SUBPIXEL_X86_H_

/* Note:
 *
 * This platform is commonly built for runtime CPU detection. If you modify
 * any of the function mappings present in this file, be sure to also update
 * them in the function pointer initialization code
 */

#if HAVE_MMX
extern prototype_subpixel_predict(vp9_sixtap_predict16x16_mmx);
extern prototype_subpixel_predict(vp9_sixtap_predict8x8_mmx);
extern prototype_subpixel_predict(vp9_sixtap_predict8x4_mmx);
extern prototype_subpixel_predict(vp9_sixtap_predict4x4_mmx);
extern prototype_subpixel_predict(vp9_bilinear_predict16x16_mmx);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp9_subpix_sixtap16x16
#define vp9_subpix_sixtap16x16 vp9_sixtap_predict16x16_mmx

#undef  vp9_subpix_sixtap8x8
#define vp9_subpix_sixtap8x8 vp9_sixtap_predict8x8_mmx

#undef  vp9_subpix_sixtap8x4
#define vp9_subpix_sixtap8x4 vp9_sixtap_predict8x4_mmx

#undef  vp9_subpix_sixtap4x4
#define vp9_subpix_sixtap4x4 vp9_sixtap_predict4x4_mmx

#undef  vp9_subpix_bilinear16x16
#define vp9_subpix_bilinear16x16 vp9_bilinear_predict16x16_mmx

#endif
#endif


#if HAVE_SSE2
extern prototype_subpixel_predict(vp9_sixtap_predict16x16_sse2);
extern prototype_subpixel_predict(vp9_sixtap_predict8x8_sse2);
extern prototype_subpixel_predict(vp9_sixtap_predict8x4_sse2);
extern prototype_subpixel_predict(vp9_bilinear_predict16x16_sse2);
extern prototype_subpixel_predict(vp9_bilinear_predict8x8_sse2);


#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp9_subpix_sixtap16x16
#define vp9_subpix_sixtap16x16 vp9_sixtap_predict16x16_sse2

#undef  vp9_subpix_sixtap8x8
#define vp9_subpix_sixtap8x8 vp9_sixtap_predict8x8_sse2

#undef  vp9_subpix_sixtap8x4
#define vp9_subpix_sixtap8x4 vp9_sixtap_predict8x4_sse2

#undef  vp9_subpix_bilinear16x16
#define vp9_subpix_bilinear16x16 vp9_bilinear_predict16x16_sse2

#undef  vp9_subpix_bilinear8x8
#define vp9_subpix_bilinear8x8 vp9_bilinear_predict8x8_sse2

#endif
#endif

#if HAVE_SSSE3
extern prototype_subpixel_predict(vp9_sixtap_predict16x16_ssse3);
extern prototype_subpixel_predict(vp9_sixtap_predict8x8_ssse3);
extern prototype_subpixel_predict(vp9_sixtap_predict8x4_ssse3);
extern prototype_subpixel_predict(vp9_sixtap_predict4x4_ssse3);
extern prototype_subpixel_predict(vp9_bilinear_predict16x16_ssse3);
extern prototype_subpixel_predict(vp9_bilinear_predict8x8_ssse3);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp9_subpix_sixtap16x16
#define vp9_subpix_sixtap16x16 vp9_sixtap_predict16x16_ssse3

#undef  vp9_subpix_sixtap8x8
#define vp9_subpix_sixtap8x8 vp9_sixtap_predict8x8_ssse3

#undef  vp9_subpix_sixtap8x4
#define vp9_subpix_sixtap8x4 vp9_sixtap_predict8x4_ssse3

#undef  vp9_subpix_sixtap4x4
#define vp9_subpix_sixtap4x4 vp9_sixtap_predict4x4_ssse3


#undef  vp9_subpix_bilinear16x16
#define vp9_subpix_bilinear16x16 vp9_bilinear_predict16x16_ssse3

#undef  vp9_subpix_bilinear8x8
#define vp9_subpix_bilinear8x8 vp9_bilinear_predict8x8_ssse3

#endif
#endif



#endif
