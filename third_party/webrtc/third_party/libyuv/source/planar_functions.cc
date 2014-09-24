/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/planar_functions.h"

#include <string.h>  // for memset()

#include "libyuv/cpu_id.h"
#ifdef HAVE_JPEG
#include "libyuv/mjpeg_decoder.h"
#endif
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Copy a plane of data
LIBYUV_API
void CopyPlane(const uint8* src_y, int src_stride_y,
               uint8* dst_y, int dst_stride_y,
               int width, int height) {
  // Coalesce contiguous rows.
  if (src_stride_y == width && dst_stride_y == width) {
    CopyPlane(src_y, 0, dst_y, 0, width * height, 1);
    return;
  }

  void (*CopyRow)(const uint8* src, uint8* dst, int width) = CopyRow_C;
#if defined(HAS_COPYROW_X86)
  if (TestCpuFlag(kCpuHasX86) && IS_ALIGNED(width, 4)) {
    CopyRow = CopyRow_X86;
  }
#endif
#if defined(HAS_COPYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 32) &&
      IS_ALIGNED(src_y, 16) && IS_ALIGNED(src_stride_y, 16) &&
      IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
    CopyRow = CopyRow_SSE2;
  }
#endif
#if defined(HAS_COPYROW_AVX2)
  // TODO(fbarchard): Detect Fast String support.
  if (TestCpuFlag(kCpuHasAVX2)) {
    CopyRow = CopyRow_AVX2;
  }
#endif
#if defined(HAS_COPYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 32)) {
    CopyRow = CopyRow_NEON;
  }
#endif
#if defined(HAS_COPYROW_MIPS)
  if (TestCpuFlag(kCpuHasMIPS)) {
    CopyRow = CopyRow_MIPS;
  }
#endif

  // Copy plane
  for (int y = 0; y < height; ++y) {
    CopyRow(src_y, dst_y, width);
    src_y += src_stride_y;
    dst_y += dst_stride_y;
  }
}

// Copy I422.
LIBYUV_API
int I422Copy(const uint8* src_y, int src_stride_y,
             const uint8* src_u, int src_stride_u,
             const uint8* src_v, int src_stride_v,
             uint8* dst_y, int dst_stride_y,
             uint8* dst_u, int dst_stride_u,
             uint8* dst_v, int dst_stride_v,
             int width, int height) {
  if (!src_y || !src_u || !src_v ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_stride_y;
    src_u = src_u + (height - 1) * src_stride_u;
    src_v = src_v + (height - 1) * src_stride_v;
    src_stride_y = -src_stride_y;
    src_stride_u = -src_stride_u;
    src_stride_v = -src_stride_v;
  }

  int halfwidth = (width + 1) >> 1;
  CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  CopyPlane(src_u, src_stride_u, dst_u, dst_stride_u, halfwidth, height);
  CopyPlane(src_v, src_stride_v, dst_v, dst_stride_v, halfwidth, height);
  return 0;
}

// Copy I444.
LIBYUV_API
int I444Copy(const uint8* src_y, int src_stride_y,
             const uint8* src_u, int src_stride_u,
             const uint8* src_v, int src_stride_v,
             uint8* dst_y, int dst_stride_y,
             uint8* dst_u, int dst_stride_u,
             uint8* dst_v, int dst_stride_v,
             int width, int height) {
  if (!src_y || !src_u || !src_v ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_stride_y;
    src_u = src_u + (height - 1) * src_stride_u;
    src_v = src_v + (height - 1) * src_stride_v;
    src_stride_y = -src_stride_y;
    src_stride_u = -src_stride_u;
    src_stride_v = -src_stride_v;
  }

  CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  CopyPlane(src_u, src_stride_u, dst_u, dst_stride_u, width, height);
  CopyPlane(src_v, src_stride_v, dst_v, dst_stride_v, width, height);
  return 0;
}

// Copy I400.
LIBYUV_API
int I400ToI400(const uint8* src_y, int src_stride_y,
               uint8* dst_y, int dst_stride_y,
               int width, int height) {
  if (!src_y || !dst_y || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_stride_y;
    src_stride_y = -src_stride_y;
  }
  CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  return 0;
}

// Convert I420 to I400.
LIBYUV_API
int I420ToI400(const uint8* src_y, int src_stride_y,
               uint8*, int,  // src_u
               uint8*, int,  // src_v
               uint8* dst_y, int dst_stride_y,
               int width, int height) {
  if (!src_y || !dst_y || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_stride_y;
    src_stride_y = -src_stride_y;
  }
  CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  return 0;
}

// Mirror a plane of data
void MirrorPlane(const uint8* src_y, int src_stride_y,
                 uint8* dst_y, int dst_stride_y,
                 int width, int height) {
  void (*MirrorRow)(const uint8* src, uint8* dst, int width) = MirrorRow_C;
#if defined(HAS_MIRRORROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 16)) {
    MirrorRow = MirrorRow_NEON;
  }
#endif
#if defined(HAS_MIRRORROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 16)) {
    MirrorRow = MirrorRow_SSE2;
  }
#endif
#if defined(HAS_MIRRORROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && IS_ALIGNED(width, 16) &&
      IS_ALIGNED(src_y, 16) && IS_ALIGNED(src_stride_y, 16) &&
      IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
    MirrorRow = MirrorRow_SSSE3;
  }
#endif

  // Mirror plane
  for (int y = 0; y < height; ++y) {
    MirrorRow(src_y, dst_y, width);
    src_y += src_stride_y;
    dst_y += dst_stride_y;
  }
}

// Convert YUY2 to I422.
LIBYUV_API
int YUY2ToI422(const uint8* src_yuy2, int src_stride_yuy2,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_yuy2 = src_yuy2 + (height - 1) * src_stride_yuy2;
    src_stride_yuy2 = -src_stride_yuy2;
  }
  void (*YUY2ToUV422Row)(const uint8* src_yuy2,
                         uint8* dst_u, uint8* dst_v, int pix);
  void (*YUY2ToYRow)(const uint8* src_yuy2,
                     uint8* dst_y, int pix);
  YUY2ToYRow = YUY2ToYRow_C;
  YUY2ToUV422Row = YUY2ToUV422Row_C;
#if defined(HAS_YUY2TOYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 16) {
    YUY2ToUV422Row = YUY2ToUV422Row_Any_SSE2;
    YUY2ToYRow = YUY2ToYRow_Any_SSE2;
    if (IS_ALIGNED(width, 16)) {
      YUY2ToUV422Row = YUY2ToUV422Row_Unaligned_SSE2;
      YUY2ToYRow = YUY2ToYRow_Unaligned_SSE2;
      if (IS_ALIGNED(src_yuy2, 16) && IS_ALIGNED(src_stride_yuy2, 16)) {
        YUY2ToUV422Row = YUY2ToUV422Row_SSE2;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          YUY2ToYRow = YUY2ToYRow_SSE2;
        }
      }
    }
  }
#endif
#if defined(HAS_YUY2TOYROW_AVX2)
  bool clear = false;
  if (TestCpuFlag(kCpuHasAVX2) && width >= 32) {
    bool clear = true;
    YUY2ToUV422Row = YUY2ToUV422Row_Any_AVX2;
    YUY2ToYRow = YUY2ToYRow_Any_AVX2;
    if (IS_ALIGNED(width, 32)) {
      YUY2ToUV422Row = YUY2ToUV422Row_AVX2;
      YUY2ToYRow = YUY2ToYRow_AVX2;
    }
  }
#endif
#if defined(HAS_YUY2TOYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    YUY2ToYRow = YUY2ToYRow_Any_NEON;
    if (width >= 16) {
      YUY2ToUV422Row = YUY2ToUV422Row_Any_NEON;
    }
    if (IS_ALIGNED(width, 16)) {
      YUY2ToYRow = YUY2ToYRow_NEON;
      YUY2ToUV422Row = YUY2ToUV422Row_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    YUY2ToUV422Row(src_yuy2, dst_u, dst_v, width);
    YUY2ToYRow(src_yuy2, dst_y, width);
    src_yuy2 += src_stride_yuy2;
    dst_y += dst_stride_y;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }

#if defined(HAS_YUY2TOYROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif
  return 0;
}

// Convert UYVY to I422.
LIBYUV_API
int UYVYToI422(const uint8* src_uyvy, int src_stride_uyvy,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_uyvy = src_uyvy + (height - 1) * src_stride_uyvy;
    src_stride_uyvy = -src_stride_uyvy;
  }
  void (*UYVYToUV422Row)(const uint8* src_uyvy,
                         uint8* dst_u, uint8* dst_v, int pix);
  void (*UYVYToYRow)(const uint8* src_uyvy,
                     uint8* dst_y, int pix);
  UYVYToYRow = UYVYToYRow_C;
  UYVYToUV422Row = UYVYToUV422Row_C;
#if defined(HAS_UYVYTOYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 16) {
    UYVYToUV422Row = UYVYToUV422Row_Any_SSE2;
    UYVYToYRow = UYVYToYRow_Any_SSE2;
    if (IS_ALIGNED(width, 16)) {
      UYVYToUV422Row = UYVYToUV422Row_Unaligned_SSE2;
      UYVYToYRow = UYVYToYRow_Unaligned_SSE2;
      if (IS_ALIGNED(src_uyvy, 16) && IS_ALIGNED(src_stride_uyvy, 16)) {
        UYVYToUV422Row = UYVYToUV422Row_SSE2;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          UYVYToYRow = UYVYToYRow_SSE2;
        }
      }
    }
  }
#endif
#if defined(HAS_UYVYTOYROW_AVX2)
  bool clear = false;
  if (TestCpuFlag(kCpuHasAVX2) && width >= 32) {
    bool clear = true;
    UYVYToUV422Row = UYVYToUV422Row_Any_AVX2;
    UYVYToYRow = UYVYToYRow_Any_AVX2;
    if (IS_ALIGNED(width, 32)) {
      UYVYToUV422Row = UYVYToUV422Row_AVX2;
      UYVYToYRow = UYVYToYRow_AVX2;
    }
  }
#endif
#if defined(HAS_UYVYTOYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    UYVYToYRow = UYVYToYRow_Any_NEON;
    if (width >= 16) {
      UYVYToUV422Row = UYVYToUV422Row_Any_NEON;
    }
    if (IS_ALIGNED(width, 16)) {
      UYVYToYRow = UYVYToYRow_NEON;
      UYVYToUV422Row = UYVYToUV422Row_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    UYVYToUV422Row(src_uyvy, dst_u, dst_v, width);
    UYVYToYRow(src_uyvy, dst_y, width);
    src_uyvy += src_stride_uyvy;
    dst_y += dst_stride_y;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }

#if defined(HAS_UYVYTOYROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif
  return 0;
}

// Mirror I400 with optional flipping
LIBYUV_API
int I400Mirror(const uint8* src_y, int src_stride_y,
               uint8* dst_y, int dst_stride_y,
               int width, int height) {
  if (!src_y || !dst_y ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_stride_y;
    src_stride_y = -src_stride_y;
  }

  MirrorPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  return 0;
}

// Mirror I420 with optional flipping
LIBYUV_API
int I420Mirror(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    int halfheight = (height + 1) >> 1;
    src_y = src_y + (height - 1) * src_stride_y;
    src_u = src_u + (halfheight - 1) * src_stride_u;
    src_v = src_v + (halfheight - 1) * src_stride_v;
    src_stride_y = -src_stride_y;
    src_stride_u = -src_stride_u;
    src_stride_v = -src_stride_v;
  }

  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;
  if (dst_y) {
    MirrorPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  }
  MirrorPlane(src_u, src_stride_u, dst_u, dst_stride_u, halfwidth, halfheight);
  MirrorPlane(src_v, src_stride_v, dst_v, dst_stride_v, halfwidth, halfheight);
  return 0;
}

// ARGB mirror.
LIBYUV_API
int ARGBMirror(const uint8* src_argb, int src_stride_argb,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  if (!src_argb || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }

  void (*ARGBMirrorRow)(const uint8* src, uint8* dst, int width) =
      ARGBMirrorRow_C;
#if defined(HAS_ARGBMIRRORROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBMirrorRow = ARGBMirrorRow_SSSE3;
  }
#elif defined(HAS_ARGBMIRRORROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 4)) {
    ARGBMirrorRow = ARGBMirrorRow_NEON;
  }
#endif

  // Mirror plane
  for (int y = 0; y < height; ++y) {
    ARGBMirrorRow(src_argb, dst_argb, width);
    src_argb += src_stride_argb;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Get a blender that optimized for the CPU, alignment and pixel count.
// As there are 6 blenders to choose from, the caller should try to use
// the same blend function for all pixels if possible.
LIBYUV_API
ARGBBlendRow GetARGBBlend() {
  void (*ARGBBlendRow)(const uint8* src_argb, const uint8* src_argb1,
                       uint8* dst_argb, int width) = ARGBBlendRow_C;
#if defined(HAS_ARGBBLENDROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    ARGBBlendRow = ARGBBlendRow_SSSE3;
    return ARGBBlendRow;
  }
#endif
#if defined(HAS_ARGBBLENDROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2)) {
    ARGBBlendRow = ARGBBlendRow_SSE2;
  }
#endif
#if defined(HAS_ARGBBLENDROW_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    ARGBBlendRow = ARGBBlendRow_NEON;
  }
#endif
  return ARGBBlendRow;
}

// Alpha Blend 2 ARGB images and store to destination.
LIBYUV_API
int ARGBBlend(const uint8* src_argb0, int src_stride_argb0,
              const uint8* src_argb1, int src_stride_argb1,
              uint8* dst_argb, int dst_stride_argb,
              int width, int height) {
  if (!src_argb0 || !src_argb1 || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  void (*ARGBBlendRow)(const uint8* src_argb, const uint8* src_argb1,
                       uint8* dst_argb, int width) = GetARGBBlend();

  for (int y = 0; y < height; ++y) {
    ARGBBlendRow(src_argb0, src_argb1, dst_argb, width);
    src_argb0 += src_stride_argb0;
    src_argb1 += src_stride_argb1;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Multiply 2 ARGB images and store to destination.
LIBYUV_API
int ARGBMultiply(const uint8* src_argb0, int src_stride_argb0,
                 const uint8* src_argb1, int src_stride_argb1,
                 uint8* dst_argb, int dst_stride_argb,
                 int width, int height) {
  if (!src_argb0 || !src_argb1 || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  // Coalesce contiguous rows.
  if (src_stride_argb0 == width * 4 &&
      src_stride_argb1 == width * 4 &&
      dst_stride_argb == width * 4) {
    return ARGBMultiply(src_argb0, 0,
                        src_argb1, 0,
                        dst_argb, 0,
                        width * height, 1);
  }

  void (*ARGBMultiplyRow)(const uint8* src0, const uint8* src1, uint8* dst,
                          int width) = ARGBMultiplyRow_C;
#if defined(HAS_ARGBMULTIPLYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 4 &&
      IS_ALIGNED(src_argb0, 16) && IS_ALIGNED(src_stride_argb0, 16) &&
      IS_ALIGNED(src_argb1, 16) && IS_ALIGNED(src_stride_argb1, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBMultiplyRow = ARGBMultiplyRow_Any_SSE2;
    if (IS_ALIGNED(width, 4)) {
      ARGBMultiplyRow = ARGBMultiplyRow_SSE2;
    }
  }
#endif
#if defined(HAS_ARGBMULTIPLYROW_AVX2)
  bool clear = false;
  if (TestCpuFlag(kCpuHasAVX2) && width >= 8) {
    clear = true;
    ARGBMultiplyRow = ARGBMultiplyRow_Any_AVX2;
    if (IS_ALIGNED(width, 8)) {
      ARGBMultiplyRow = ARGBMultiplyRow_AVX2;
    }
  }
#endif
#if defined(HAS_ARGBMULTIPLYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    ARGBMultiplyRow = ARGBMultiplyRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGBMultiplyRow = ARGBMultiplyRow_NEON;
    }
  }
#endif

  // Multiply plane
  for (int y = 0; y < height; ++y) {
    ARGBMultiplyRow(src_argb0, src_argb1, dst_argb, width);
    src_argb0 += src_stride_argb0;
    src_argb1 += src_stride_argb1;
    dst_argb += dst_stride_argb;
  }

#if defined(HAS_ARGBMULTIPLYROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif
  return 0;
}

// Add 2 ARGB images and store to destination.
LIBYUV_API
int ARGBAdd(const uint8* src_argb0, int src_stride_argb0,
            const uint8* src_argb1, int src_stride_argb1,
            uint8* dst_argb, int dst_stride_argb,
            int width, int height) {
  if (!src_argb0 || !src_argb1 || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  // Coalesce contiguous rows.
  if (src_stride_argb0 == width * 4 &&
      src_stride_argb1 == width * 4 &&
      dst_stride_argb == width * 4) {
    return ARGBAdd(src_argb0, 0,
                   src_argb1, 0,
                   dst_argb, 0,
                   width * height, 1);
  }

  void (*ARGBAddRow)(const uint8* src0, const uint8* src1, uint8* dst,
                     int width) = ARGBAddRow_C;
#if defined(HAS_ARGBADDROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 4 &&
      IS_ALIGNED(src_argb0, 16) && IS_ALIGNED(src_stride_argb0, 16) &&
      IS_ALIGNED(src_argb1, 16) && IS_ALIGNED(src_stride_argb1, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBAddRow = ARGBAddRow_Any_SSE2;
    if (IS_ALIGNED(width, 4)) {
      ARGBAddRow = ARGBAddRow_SSE2;
    }
  }
#endif
#if defined(HAS_ARGBADDROW_AVX2)
  bool clear = false;
  if (TestCpuFlag(kCpuHasAVX2) && width >= 8) {
    clear = true;
    ARGBAddRow = ARGBAddRow_Any_AVX2;
    if (IS_ALIGNED(width, 8)) {
      ARGBAddRow = ARGBAddRow_AVX2;
    }
  }
#endif
#if defined(HAS_ARGBADDROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    ARGBAddRow = ARGBAddRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGBAddRow = ARGBAddRow_NEON;
    }
  }
#endif

  // Add plane
  for (int y = 0; y < height; ++y) {
    ARGBAddRow(src_argb0, src_argb1, dst_argb, width);
    src_argb0 += src_stride_argb0;
    src_argb1 += src_stride_argb1;
    dst_argb += dst_stride_argb;
  }

#if defined(HAS_ARGBADDROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif
  return 0;
}

// Subtract 2 ARGB images and store to destination.
LIBYUV_API
int ARGBSubtract(const uint8* src_argb0, int src_stride_argb0,
                 const uint8* src_argb1, int src_stride_argb1,
                 uint8* dst_argb, int dst_stride_argb,
                 int width, int height) {
  if (!src_argb0 || !src_argb1 || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  // Coalesce contiguous rows.
  if (src_stride_argb0 == width * 4 &&
      src_stride_argb1 == width * 4 &&
      dst_stride_argb == width * 4) {
    return ARGBSubtract(src_argb0, 0,
                        src_argb1, 0,
                        dst_argb, 0,
                        width * height, 1);
  }

  void (*ARGBSubtractRow)(const uint8* src0, const uint8* src1, uint8* dst,
                          int width) = ARGBSubtractRow_C;
#if defined(HAS_ARGBSUBTRACTROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 4 &&
      IS_ALIGNED(src_argb0, 16) && IS_ALIGNED(src_stride_argb0, 16) &&
      IS_ALIGNED(src_argb1, 16) && IS_ALIGNED(src_stride_argb1, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBSubtractRow = ARGBSubtractRow_Any_SSE2;
    if (IS_ALIGNED(width, 4)) {
      ARGBSubtractRow = ARGBSubtractRow_SSE2;
    }
  }
#endif
#if defined(HAS_ARGBSUBTRACTROW_AVX2)
  bool clear = false;
  if (TestCpuFlag(kCpuHasAVX2) && width >= 8) {
    clear = true;
    ARGBSubtractRow = ARGBSubtractRow_Any_AVX2;
    if (IS_ALIGNED(width, 8)) {
      ARGBSubtractRow = ARGBSubtractRow_AVX2;
    }
  }
#endif
#if defined(HAS_ARGBSUBTRACTROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    ARGBSubtractRow = ARGBSubtractRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGBSubtractRow = ARGBSubtractRow_NEON;
    }
  }
#endif

  // Subtract plane
  for (int y = 0; y < height; ++y) {
    ARGBSubtractRow(src_argb0, src_argb1, dst_argb, width);
    src_argb0 += src_stride_argb0;
    src_argb1 += src_stride_argb1;
    dst_argb += dst_stride_argb;
  }

#if defined(HAS_ARGBSUBTRACTROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif
  return 0;
}

// Convert I422 to BGRA.
LIBYUV_API
int I422ToBGRA(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_bgra, int dst_stride_bgra,
               int width, int height) {
  if (!src_y || !src_u || !src_v ||
      !dst_bgra ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_bgra = dst_bgra + (height - 1) * dst_stride_bgra;
    dst_stride_bgra = -dst_stride_bgra;
  }
  void (*I422ToBGRARow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToBGRARow_C;
#if defined(HAS_I422TOBGRAROW_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    I422ToBGRARow = I422ToBGRARow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToBGRARow = I422ToBGRARow_NEON;
    }
  }
#elif defined(HAS_I422TOBGRAROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToBGRARow = I422ToBGRARow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToBGRARow = I422ToBGRARow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_bgra, 16) && IS_ALIGNED(dst_stride_bgra, 16)) {
        I422ToBGRARow = I422ToBGRARow_SSSE3;
      }
    }
  }
#elif defined(HAS_I422TOBGRAROW_MIPS_DSPR2)
  if (TestCpuFlag(kCpuHasMIPS_DSPR2) && IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_y, 4) && IS_ALIGNED(src_stride_y, 4) &&
      IS_ALIGNED(src_u, 2) && IS_ALIGNED(src_stride_u, 2) &&
      IS_ALIGNED(src_v, 2) && IS_ALIGNED(src_stride_v, 2) &&
      IS_ALIGNED(dst_bgra, 4) && IS_ALIGNED(dst_stride_bgra, 4)) {
    I422ToBGRARow = I422ToBGRARow_MIPS_DSPR2;
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToBGRARow(src_y, src_u, src_v, dst_bgra, width);
    dst_bgra += dst_stride_bgra;
    src_y += src_stride_y;
    src_u += src_stride_u;
    src_v += src_stride_v;
  }
  return 0;
}

// Convert I422 to ABGR.
LIBYUV_API
int I422ToABGR(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_abgr, int dst_stride_abgr,
               int width, int height) {
  if (!src_y || !src_u || !src_v ||
      !dst_abgr ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_abgr = dst_abgr + (height - 1) * dst_stride_abgr;
    dst_stride_abgr = -dst_stride_abgr;
  }
  void (*I422ToABGRRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToABGRRow_C;
#if defined(HAS_I422TOABGRROW_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    I422ToABGRRow = I422ToABGRRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToABGRRow = I422ToABGRRow_NEON;
    }
  }
#elif defined(HAS_I422TOABGRROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToABGRRow = I422ToABGRRow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToABGRRow = I422ToABGRRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_abgr, 16) && IS_ALIGNED(dst_stride_abgr, 16)) {
        I422ToABGRRow = I422ToABGRRow_SSSE3;
      }
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToABGRRow(src_y, src_u, src_v, dst_abgr, width);
    dst_abgr += dst_stride_abgr;
    src_y += src_stride_y;
    src_u += src_stride_u;
    src_v += src_stride_v;
  }
  return 0;
}

// Convert I422 to RGBA.
LIBYUV_API
int I422ToRGBA(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_rgba, int dst_stride_rgba,
               int width, int height) {
  if (!src_y || !src_u || !src_v ||
      !dst_rgba ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_rgba = dst_rgba + (height - 1) * dst_stride_rgba;
    dst_stride_rgba = -dst_stride_rgba;
  }
  void (*I422ToRGBARow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToRGBARow_C;
#if defined(HAS_I422TORGBAROW_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    I422ToRGBARow = I422ToRGBARow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToRGBARow = I422ToRGBARow_NEON;
    }
  }
#elif defined(HAS_I422TORGBAROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToRGBARow = I422ToRGBARow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToRGBARow = I422ToRGBARow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_rgba, 16) && IS_ALIGNED(dst_stride_rgba, 16)) {
        I422ToRGBARow = I422ToRGBARow_SSSE3;
      }
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToRGBARow(src_y, src_u, src_v, dst_rgba, width);
    dst_rgba += dst_stride_rgba;
    src_y += src_stride_y;
    src_u += src_stride_u;
    src_v += src_stride_v;
  }
  return 0;
}

// Convert NV12 to RGB565.
LIBYUV_API
int NV12ToRGB565(const uint8* src_y, int src_stride_y,
                 const uint8* src_uv, int src_stride_uv,
                 uint8* dst_rgb565, int dst_stride_rgb565,
                 int width, int height) {
  if (!src_y || !src_uv || !dst_rgb565 ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_rgb565 = dst_rgb565 + (height - 1) * dst_stride_rgb565;
    dst_stride_rgb565 = -dst_stride_rgb565;
  }
  void (*NV12ToRGB565Row)(const uint8* y_buf,
                          const uint8* uv_buf,
                          uint8* rgb_buf,
                          int width) = NV12ToRGB565Row_C;
#if defined(HAS_NV12TORGB565ROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8 && width <= kMaxStride * 4) {
    NV12ToRGB565Row = NV12ToRGB565Row_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      NV12ToRGB565Row = NV12ToRGB565Row_SSSE3;
    }
  }
#elif defined(HAS_NV12TORGB565ROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    NV12ToRGB565Row = NV12ToRGB565Row_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      NV12ToRGB565Row = NV12ToRGB565Row_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    NV12ToRGB565Row(src_y, src_uv, dst_rgb565, width);
    dst_rgb565 += dst_stride_rgb565;
    src_y += src_stride_y;
    if (y & 1) {
      src_uv += src_stride_uv;
    }
  }
  return 0;
}

// Convert NV21 to RGB565.
LIBYUV_API
int NV21ToRGB565(const uint8* src_y, int src_stride_y,
                 const uint8* src_vu, int src_stride_vu,
                 uint8* dst_rgb565, int dst_stride_rgb565,
                 int width, int height) {
  if (!src_y || !src_vu || !dst_rgb565 ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_rgb565 = dst_rgb565 + (height - 1) * dst_stride_rgb565;
    dst_stride_rgb565 = -dst_stride_rgb565;
  }
  void (*NV21ToRGB565Row)(const uint8* y_buf,
                          const uint8* src_vu,
                          uint8* rgb_buf,
                          int width) = NV21ToRGB565Row_C;
#if defined(HAS_NV21TORGB565ROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8 && width <= kMaxStride * 4) {
    NV21ToRGB565Row = NV21ToRGB565Row_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      NV21ToRGB565Row = NV21ToRGB565Row_SSSE3;
    }
  }
#elif defined(HAS_NV21TORGB565ROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    NV21ToRGB565Row = NV21ToRGB565Row_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      NV21ToRGB565Row = NV21ToRGB565Row_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    NV21ToRGB565Row(src_y, src_vu, dst_rgb565, width);
    dst_rgb565 += dst_stride_rgb565;
    src_y += src_stride_y;
    if (y & 1) {
      src_vu += src_stride_vu;
    }
  }
  return 0;
}

LIBYUV_API
void SetPlane(uint8* dst_y, int dst_stride_y,
              int width, int height,
              uint32 value) {
  void (*SetRow)(uint8* dst, uint32 value, int pix) = SetRow_C;
#if defined(HAS_SETROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) &&
      IS_ALIGNED(width, 16) &&
      IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
    SetRow = SetRow_NEON;
  }
#endif
#if defined(HAS_SETROW_X86)
  if (TestCpuFlag(kCpuHasX86) && IS_ALIGNED(width, 4)) {
    SetRow = SetRow_X86;
  }
#endif

  uint32 v32 = value | (value << 8) | (value << 16) | (value << 24);
  // Set plane
  for (int y = 0; y < height; ++y) {
    SetRow(dst_y, v32, width);
    dst_y += dst_stride_y;
  }
}

// Draw a rectangle into I420
LIBYUV_API
int I420Rect(uint8* dst_y, int dst_stride_y,
             uint8* dst_u, int dst_stride_u,
             uint8* dst_v, int dst_stride_v,
             int x, int y,
             int width, int height,
             int value_y, int value_u, int value_v) {
  if (!dst_y || !dst_u || !dst_v ||
      width <= 0 || height <= 0 ||
      x < 0 || y < 0 ||
      value_y < 0 || value_y > 255 ||
      value_u < 0 || value_u > 255 ||
      value_v < 0 || value_v > 255) {
    return -1;
  }
  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;
  uint8* start_y = dst_y + y * dst_stride_y + x;
  uint8* start_u = dst_u + (y / 2) * dst_stride_u + (x / 2);
  uint8* start_v = dst_v + (y / 2) * dst_stride_v + (x / 2);

  SetPlane(start_y, dst_stride_y, width, height, value_y);
  SetPlane(start_u, dst_stride_u, halfwidth, halfheight, value_u);
  SetPlane(start_v, dst_stride_v, halfwidth, halfheight, value_v);
  return 0;
}

// Draw a rectangle into ARGB
LIBYUV_API
int ARGBRect(uint8* dst_argb, int dst_stride_argb,
             int dst_x, int dst_y,
             int width, int height,
             uint32 value) {
  if (!dst_argb ||
      width <= 0 || height <= 0 ||
      dst_x < 0 || dst_y < 0) {
    return -1;
  }
  uint8* dst = dst_argb + dst_y * dst_stride_argb + dst_x * 4;
#if defined(HAS_SETROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 16) &&
      IS_ALIGNED(dst, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBSetRows_NEON(dst, value, width, dst_stride_argb, height);
    return 0;
  }
#endif
#if defined(HAS_SETROW_X86)
  if (TestCpuFlag(kCpuHasX86)) {
    ARGBSetRows_X86(dst, value, width, dst_stride_argb, height);
    return 0;
  }
#endif
  ARGBSetRows_C(dst, value, width, dst_stride_argb, height);
  return 0;
}

// Convert unattentuated ARGB to preattenuated ARGB.
// An unattenutated ARGB alpha blend uses the formula
// p = a * f + (1 - a) * b
// where
//   p is output pixel
//   f is foreground pixel
//   b is background pixel
//   a is alpha value from foreground pixel
// An preattenutated ARGB alpha blend uses the formula
// p = f + (1 - a) * b
// where
//   f is foreground pixel premultiplied by alpha

LIBYUV_API
int ARGBAttenuate(const uint8* src_argb, int src_stride_argb,
                  uint8* dst_argb, int dst_stride_argb,
                  int width, int height) {
  if (!src_argb || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBAttenuateRow)(const uint8* src_argb, uint8* dst_argb,
                           int width) = ARGBAttenuateRow_C;
#if defined(HAS_ARGBATTENUATEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 4 &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBAttenuateRow = ARGBAttenuateRow_Any_SSE2;
    if (IS_ALIGNED(width, 4)) {
      ARGBAttenuateRow = ARGBAttenuateRow_SSE2;
    }
  }
#endif
#if defined(HAS_ARGBATTENUATEROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 4 &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBAttenuateRow = ARGBAttenuateRow_Any_SSSE3;
    if (IS_ALIGNED(width, 4)) {
      ARGBAttenuateRow = ARGBAttenuateRow_SSSE3;
    }
  }
#endif
#if defined(HAS_ARGBATTENUATEROW_AVX2)
  bool clear = false;
  if (TestCpuFlag(kCpuHasAVX2) && width >= 8) {
    clear = true;
    ARGBAttenuateRow = ARGBAttenuateRow_Any_AVX2;
    if (IS_ALIGNED(width, 8)) {
      ARGBAttenuateRow = ARGBAttenuateRow_AVX2;
    }
  }
#endif
#if defined(HAS_ARGBATTENUATEROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    ARGBAttenuateRow = ARGBAttenuateRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGBAttenuateRow = ARGBAttenuateRow_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    ARGBAttenuateRow(src_argb, dst_argb, width);
    src_argb += src_stride_argb;
    dst_argb += dst_stride_argb;
  }

#if defined(HAS_ARGBATTENUATEROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif

  return 0;
}

// Convert preattentuated ARGB to unattenuated ARGB.
LIBYUV_API
int ARGBUnattenuate(const uint8* src_argb, int src_stride_argb,
                    uint8* dst_argb, int dst_stride_argb,
                    int width, int height) {
  if (!src_argb || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBUnattenuateRow)(const uint8* src_argb, uint8* dst_argb,
                           int width) = ARGBUnattenuateRow_C;
#if defined(HAS_ARGBUNATTENUATEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 4 &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBUnattenuateRow = ARGBUnattenuateRow_Any_SSE2;
    if (IS_ALIGNED(width, 4)) {
      ARGBUnattenuateRow = ARGBUnattenuateRow_SSE2;
    }
  }
#endif
#if defined(HAS_ARGBUNATTENUATEROW_AVX2)
  bool clear = false;
  if (TestCpuFlag(kCpuHasAVX2) && width >= 8) {
    clear = true;
    ARGBUnattenuateRow = ARGBUnattenuateRow_Any_AVX2;
    if (IS_ALIGNED(width, 8)) {
      ARGBUnattenuateRow = ARGBUnattenuateRow_AVX2;
    }
  }
#endif
// TODO(fbarchard): Neon version.

  for (int y = 0; y < height; ++y) {
    ARGBUnattenuateRow(src_argb, dst_argb, width);
    src_argb += src_stride_argb;
    dst_argb += dst_stride_argb;
  }

#if defined(HAS_ARGBUNATTENUATEROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif

  return 0;
}

// Convert ARGB to Grayed ARGB.
LIBYUV_API
int ARGBGrayTo(const uint8* src_argb, int src_stride_argb,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  if (!src_argb || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBGrayRow)(const uint8* src_argb, uint8* dst_argb,
                      int width) = ARGBGrayRow_C;
#if defined(HAS_ARGBGRAYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && IS_ALIGNED(width, 8) &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBGrayRow = ARGBGrayRow_SSSE3;
  }
#elif defined(HAS_ARGBGRAYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 8)) {
    ARGBGrayRow = ARGBGrayRow_NEON;
  }
#endif

  for (int y = 0; y < height; ++y) {
    ARGBGrayRow(src_argb, dst_argb, width);
    src_argb += src_stride_argb;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Make a rectangle of ARGB gray scale.
LIBYUV_API
int ARGBGray(uint8* dst_argb, int dst_stride_argb,
             int dst_x, int dst_y,
             int width, int height) {
  if (!dst_argb || width <= 0 || height <= 0 || dst_x < 0 || dst_y < 0) {
    return -1;
  }
  void (*ARGBGrayRow)(const uint8* src_argb, uint8* dst_argb,
                      int width) = ARGBGrayRow_C;
#if defined(HAS_ARGBGRAYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && IS_ALIGNED(width, 8) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBGrayRow = ARGBGrayRow_SSSE3;
  }
#elif defined(HAS_ARGBGRAYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 8)) {
    ARGBGrayRow = ARGBGrayRow_NEON;
  }
#endif
  uint8* dst = dst_argb + dst_y * dst_stride_argb + dst_x * 4;
  for (int y = 0; y < height; ++y) {
    ARGBGrayRow(dst, dst, width);
    dst += dst_stride_argb;
  }
  return 0;
}

// Make a rectangle of ARGB Sepia tone.
LIBYUV_API
int ARGBSepia(uint8* dst_argb, int dst_stride_argb,
              int dst_x, int dst_y, int width, int height) {
  if (!dst_argb || width <= 0 || height <= 0 || dst_x < 0 || dst_y < 0) {
    return -1;
  }
  void (*ARGBSepiaRow)(uint8* dst_argb, int width) = ARGBSepiaRow_C;
#if defined(HAS_ARGBSEPIAROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && IS_ALIGNED(width, 8) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBSepiaRow = ARGBSepiaRow_SSSE3;
  }
#elif defined(HAS_ARGBSEPIAROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 8)) {
    ARGBSepiaRow = ARGBSepiaRow_NEON;
  }
#endif
  uint8* dst = dst_argb + dst_y * dst_stride_argb + dst_x * 4;
  for (int y = 0; y < height; ++y) {
    ARGBSepiaRow(dst, width);
    dst += dst_stride_argb;
  }
  return 0;
}

// Apply a 4x3 matrix rotation to each ARGB pixel.
LIBYUV_API
int ARGBColorMatrix(uint8* dst_argb, int dst_stride_argb,
                    const int8* matrix_argb,
                    int dst_x, int dst_y, int width, int height) {
  if (!dst_argb || !matrix_argb || width <= 0 || height <= 0 ||
      dst_x < 0 || dst_y < 0) {
    return -1;
  }
  void (*ARGBColorMatrixRow)(uint8* dst_argb, const int8* matrix_argb,
                             int width) = ARGBColorMatrixRow_C;
#if defined(HAS_ARGBCOLORMATRIXROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && IS_ALIGNED(width, 8) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBColorMatrixRow = ARGBColorMatrixRow_SSSE3;
  }
#elif defined(HAS_ARGBCOLORMATRIXROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 8)) {
    ARGBColorMatrixRow = ARGBColorMatrixRow_NEON;
  }
#endif
  uint8* dst = dst_argb + dst_y * dst_stride_argb + dst_x * 4;
  for (int y = 0; y < height; ++y) {
    ARGBColorMatrixRow(dst, matrix_argb, width);
    dst += dst_stride_argb;
  }
  return 0;
}

// Apply a color table each ARGB pixel.
// Table contains 256 ARGB values.
LIBYUV_API
int ARGBColorTable(uint8* dst_argb, int dst_stride_argb,
                   const uint8* table_argb,
                   int dst_x, int dst_y, int width, int height) {
  if (!dst_argb || !table_argb || width <= 0 || height <= 0 ||
      dst_x < 0 || dst_y < 0) {
    return -1;
  }
  void (*ARGBColorTableRow)(uint8* dst_argb, const uint8* table_argb,
                            int width) = ARGBColorTableRow_C;
#if defined(HAS_ARGBCOLORTABLEROW_X86)
  if (TestCpuFlag(kCpuHasX86)) {
    ARGBColorTableRow = ARGBColorTableRow_X86;
  }
#endif
  uint8* dst = dst_argb + dst_y * dst_stride_argb + dst_x * 4;
  for (int y = 0; y < height; ++y) {
    ARGBColorTableRow(dst, table_argb, width);
    dst += dst_stride_argb;
  }
  return 0;
}

// ARGBQuantize is used to posterize art.
// e.g. rgb / qvalue * qvalue + qvalue / 2
// But the low levels implement efficiently with 3 parameters, and could be
// used for other high level operations.
// dst_argb[0] = (b * scale >> 16) * interval_size + interval_offset;
// where scale is 1 / interval_size as a fixed point value.
// The divide is replaces with a multiply by reciprocal fixed point multiply.
// Caveat - although SSE2 saturates, the C function does not and should be used
// with care if doing anything but quantization.
LIBYUV_API
int ARGBQuantize(uint8* dst_argb, int dst_stride_argb,
                 int scale, int interval_size, int interval_offset,
                 int dst_x, int dst_y, int width, int height) {
  if (!dst_argb || width <= 0 || height <= 0 || dst_x < 0 || dst_y < 0 ||
      interval_size < 1 || interval_size > 255) {
    return -1;
  }
  void (*ARGBQuantizeRow)(uint8* dst_argb, int scale, int interval_size,
                          int interval_offset, int width) = ARGBQuantizeRow_C;
#if defined(HAS_ARGBQUANTIZEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 4) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBQuantizeRow = ARGBQuantizeRow_SSE2;
  }
#elif defined(HAS_ARGBQUANTIZEROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 8)) {
    ARGBQuantizeRow = ARGBQuantizeRow_NEON;
  }
#endif
  uint8* dst = dst_argb + dst_y * dst_stride_argb + dst_x * 4;
  for (int y = 0; y < height; ++y) {
    ARGBQuantizeRow(dst, scale, interval_size, interval_offset, width);
    dst += dst_stride_argb;
  }
  return 0;
}

// Computes table of cumulative sum for image where the value is the sum
// of all values above and to the left of the entry. Used by ARGBBlur.
LIBYUV_API
int ARGBComputeCumulativeSum(const uint8* src_argb, int src_stride_argb,
                             int32* dst_cumsum, int dst_stride32_cumsum,
                             int width, int height) {
  if (!dst_cumsum || !src_argb || width <= 0 || height <= 0) {
    return -1;
  }
  void (*ComputeCumulativeSumRow)(const uint8* row, int32* cumsum,
      const int32* previous_cumsum, int width) = ComputeCumulativeSumRow_C;
#if defined(HAS_CUMULATIVESUMTOAVERAGEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2)) {
    ComputeCumulativeSumRow = ComputeCumulativeSumRow_SSE2;
  }
#endif
  memset(dst_cumsum, 0, width * sizeof(dst_cumsum[0]) * 4);  // 4 int per pixel.
  int32* previous_cumsum = dst_cumsum;
  for (int y = 0; y < height; ++y) {
    ComputeCumulativeSumRow(src_argb, dst_cumsum, previous_cumsum, width);
    previous_cumsum = dst_cumsum;
    dst_cumsum += dst_stride32_cumsum;
    src_argb += src_stride_argb;
  }
  return 0;
}

// Blur ARGB image.
// Caller should allocate CumulativeSum table of width * height * 16 bytes
// aligned to 16 byte boundary. height can be radius * 2 + 2 to save memory
// as the buffer is treated as circular.
LIBYUV_API
int ARGBBlur(const uint8* src_argb, int src_stride_argb,
             uint8* dst_argb, int dst_stride_argb,
             int32* dst_cumsum, int dst_stride32_cumsum,
             int width, int height, int radius) {
  if (!src_argb || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  void (*ComputeCumulativeSumRow)(const uint8* row, int32* cumsum,
      const int32* previous_cumsum, int width) = ComputeCumulativeSumRow_C;
  void (*CUMULATIVESUMTOAVERAGEROW)(const int32* topleft, const int32* botleft,
      int width, int area, uint8* dst, int count) = CumulativeSumToAverageRow_C;
#if defined(HAS_CUMULATIVESUMTOAVERAGEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2)) {
    ComputeCumulativeSumRow = ComputeCumulativeSumRow_SSE2;
    CUMULATIVESUMTOAVERAGEROW = CumulativeSumToAverageRow_SSE2;
  }
#endif
  // Compute enough CumulativeSum for first row to be blurred. After this
  // one row of CumulativeSum is updated at a time.
  ARGBComputeCumulativeSum(src_argb, src_stride_argb,
                           dst_cumsum, dst_stride32_cumsum,
                           width, radius);

  src_argb = src_argb + radius * src_stride_argb;
  int32* cumsum_bot_row = &dst_cumsum[(radius - 1) * dst_stride32_cumsum];

  const int32* max_cumsum_bot_row =
      &dst_cumsum[(radius * 2 + 2) * dst_stride32_cumsum];
  const int32* cumsum_top_row = &dst_cumsum[0];

  for (int y = 0; y < height; ++y) {
    int top_y = ((y - radius - 1) >= 0) ? (y - radius - 1) : 0;
    int bot_y = ((y + radius) < height) ? (y + radius) : (height - 1);
    int area = radius * (bot_y - top_y);

    // Increment cumsum_top_row pointer with circular buffer wrap around.
    if (top_y) {
      cumsum_top_row += dst_stride32_cumsum;
      if (cumsum_top_row >= max_cumsum_bot_row) {
        cumsum_top_row = dst_cumsum;
      }
    }
    // Increment cumsum_bot_row pointer with circular buffer wrap around and
    // then fill in a row of CumulativeSum.
    if ((y + radius) < height) {
      const int32* prev_cumsum_bot_row = cumsum_bot_row;
      cumsum_bot_row += dst_stride32_cumsum;
      if (cumsum_bot_row >= max_cumsum_bot_row) {
        cumsum_bot_row = dst_cumsum;
      }
      ComputeCumulativeSumRow(src_argb, cumsum_bot_row, prev_cumsum_bot_row,
                              width);
      src_argb += src_stride_argb;
    }

    // Left clipped.
    int boxwidth = radius * 4;
    int x;
    for (x = 0; x < radius + 1; ++x) {
      CUMULATIVESUMTOAVERAGEROW(cumsum_top_row, cumsum_bot_row,
                              boxwidth, area, &dst_argb[x * 4], 1);
      area += (bot_y - top_y);
      boxwidth += 4;
    }

    // Middle unclipped.
    int n = (width - 1) - radius - x + 1;
    CUMULATIVESUMTOAVERAGEROW(cumsum_top_row, cumsum_bot_row,
                           boxwidth, area, &dst_argb[x * 4], n);

    // Right clipped.
    for (x += n; x <= width - 1; ++x) {
      area -= (bot_y - top_y);
      boxwidth -= 4;
      CUMULATIVESUMTOAVERAGEROW(cumsum_top_row + (x - radius - 1) * 4,
                             cumsum_bot_row + (x - radius - 1) * 4,
                             boxwidth, area, &dst_argb[x * 4], 1);
    }
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Multiply ARGB image by a specified ARGB value.
LIBYUV_API
int ARGBShade(const uint8* src_argb, int src_stride_argb,
              uint8* dst_argb, int dst_stride_argb,
              int width, int height, uint32 value) {
  if (!src_argb || !dst_argb || width <= 0 || height == 0 || value == 0u) {
    return -1;
  }
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBShadeRow)(const uint8* src_argb, uint8* dst_argb,
                       int width, uint32 value) = ARGBShadeRow_C;
#if defined(HAS_ARGBSHADEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBShadeRow = ARGBShadeRow_SSE2;
  }
#elif defined(HAS_ARGBSHADEROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 8)) {
    ARGBShadeRow = ARGBShadeRow_NEON;
  }
#endif

  for (int y = 0; y < height; ++y) {
    ARGBShadeRow(src_argb, dst_argb, width, value);
    src_argb += src_stride_argb;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Interpolate 2 ARGB images by specified amount (0 to 255).
// TODO(fbarchard): Consider selecting a specialization for interpolation so
//     row function doesn't need to check interpolation on each row.
LIBYUV_API
int ARGBInterpolate(const uint8* src_argb0, int src_stride_argb0,
                    const uint8* src_argb1, int src_stride_argb1,
                    uint8* dst_argb, int dst_stride_argb,
                    int width, int height, int interpolation) {
  if (!src_argb0 || !src_argb1 || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  void (*ARGBInterpolateRow)(uint8* dst_ptr, const uint8* src_ptr,
                             ptrdiff_t src_stride, int dst_width,
                             int source_y_fraction) = ARGBInterpolateRow_C;
#if defined(HAS_ARGBINTERPOLATEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_argb0, 16) && IS_ALIGNED(src_stride_argb0, 16) &&
      IS_ALIGNED(src_argb1, 16) && IS_ALIGNED(src_stride_argb1, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBInterpolateRow = ARGBInterpolateRow_SSE2;
  }
#endif
#if defined(HAS_ARGBINTERPOLATEROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_argb0, 16) && IS_ALIGNED(src_stride_argb0, 16) &&
      IS_ALIGNED(src_argb1, 16) && IS_ALIGNED(src_stride_argb1, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBInterpolateRow = ARGBInterpolateRow_SSSE3;
  }
#elif defined(HAS_ARGBINTERPOLATEROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 4)) {
    ARGBInterpolateRow = ARGBInterpolateRow_NEON;
  }
#endif
  for (int y = 0; y < height; ++y) {
    ARGBInterpolateRow(dst_argb, src_argb0, src_argb1 - src_argb0,
                       width, interpolation);
    src_argb0 += src_stride_argb0;
    src_argb1 += src_stride_argb1;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif
