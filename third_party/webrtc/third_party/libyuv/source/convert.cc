/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/convert.h"

#include "libyuv/basic_types.h"
#include "libyuv/cpu_id.h"
#include "libyuv/format_conversion.h"
#ifdef HAVE_JPEG
#include "libyuv/mjpeg_decoder.h"
#endif
#include "libyuv/planar_functions.h"
#include "libyuv/rotate.h"
#include "libyuv/scale.h"  // For ScalePlane()
#include "libyuv/video_common.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Copy I420 with optional flipping
LIBYUV_API
int I420Copy(const uint8* src_y, int src_stride_y,
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
    CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  }
  CopyPlane(src_u, src_stride_u, dst_u, dst_stride_u, halfwidth, halfheight);
  CopyPlane(src_v, src_stride_v, dst_v, dst_stride_v, halfwidth, halfheight);
  return 0;
}

LIBYUV_API
int I422ToI420(const uint8* src_y, int src_stride_y,
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
  void (*HalfRow)(const uint8* src_uv, int src_uv_stride,
                  uint8* dst_uv, int pix) = HalfRow_C;
#if defined(HAS_HALFROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(halfwidth, 16) &&
      IS_ALIGNED(src_u, 16) && IS_ALIGNED(src_stride_u, 16) &&
      IS_ALIGNED(src_v, 16) && IS_ALIGNED(src_stride_v, 16) &&
      IS_ALIGNED(dst_u, 16) && IS_ALIGNED(dst_stride_u, 16) &&
      IS_ALIGNED(dst_v, 16) && IS_ALIGNED(dst_stride_v, 16)) {
    HalfRow = HalfRow_SSE2;
  }
#endif
#if defined(HAS_HALFROW_AVX2)
  bool clear = false;
  if (TestCpuFlag(kCpuHasAVX2) && IS_ALIGNED(halfwidth, 32)) {
    clear = true;
    HalfRow = HalfRow_AVX2;
  }
#endif
#if defined(HAS_HALFROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(halfwidth, 16)) {
    HalfRow = HalfRow_NEON;
  }
#endif

  // Copy Y plane
  if (dst_y) {
    CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  }

  // SubSample U plane.
  int y;
  for (y = 0; y < height - 1; y += 2) {
    HalfRow(src_u, src_stride_u, dst_u, halfwidth);
    src_u += src_stride_u * 2;
    dst_u += dst_stride_u;
  }
  if (height & 1) {
    HalfRow(src_u, 0, dst_u, halfwidth);
  }

  // SubSample V plane.
  for (y = 0; y < height - 1; y += 2) {
    HalfRow(src_v, src_stride_v, dst_v, halfwidth);
    src_v += src_stride_v * 2;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    HalfRow(src_v, 0, dst_v, halfwidth);
  }
#if defined(HAS_HALFROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif
  return 0;
}

// Blends 32x2 pixels to 16x1
// source in scale.cc
#if !defined(LIBYUV_DISABLE_NEON) && \
    (defined(__ARM_NEON__) || defined(LIBYUV_NEON))
#define HAS_SCALEROWDOWN2_NEON
void ScaleRowDown2Int_NEON(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst, int dst_width);
#elif !defined(LIBYUV_DISABLE_X86) && \
    (defined(_M_IX86) || defined(__x86_64__) || defined(__i386__))

void ScaleRowDown2Int_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst_ptr, int dst_width);
#endif
void ScaleRowDown2Int_C(const uint8* src_ptr, ptrdiff_t src_stride,
                        uint8* dst_ptr, int dst_width);

LIBYUV_API
int I444ToI420(const uint8* src_y, int src_stride_y,
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
  void (*ScaleRowDown2)(const uint8* src_ptr, ptrdiff_t src_stride,
                        uint8* dst_ptr, int dst_width) = ScaleRowDown2Int_C;
#if defined(HAS_SCALEROWDOWN2_NEON)
  if (TestCpuFlag(kCpuHasNEON) &&
      IS_ALIGNED(halfwidth, 16)) {
    ScaleRowDown2 = ScaleRowDown2Int_NEON;
  }
#elif defined(HAS_SCALEROWDOWN2_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      IS_ALIGNED(halfwidth, 16) &&
      IS_ALIGNED(src_u, 16) && IS_ALIGNED(src_stride_u, 16) &&
      IS_ALIGNED(src_v, 16) && IS_ALIGNED(src_stride_v, 16) &&
      IS_ALIGNED(dst_u, 16) && IS_ALIGNED(dst_stride_u, 16) &&
      IS_ALIGNED(dst_v, 16) && IS_ALIGNED(dst_stride_v, 16)) {
    ScaleRowDown2 = ScaleRowDown2Int_SSE2;
  }
#endif

  // Copy Y plane
  if (dst_y) {
    CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  }

  // SubSample U plane.
  int y;
  for (y = 0; y < height - 1; y += 2) {
    ScaleRowDown2(src_u, src_stride_u, dst_u, halfwidth);
    src_u += src_stride_u * 2;
    dst_u += dst_stride_u;
  }
  if (height & 1) {
    ScaleRowDown2(src_u, 0, dst_u, halfwidth);
  }

  // SubSample V plane.
  for (y = 0; y < height - 1; y += 2) {
    ScaleRowDown2(src_v, src_stride_v, dst_v, halfwidth);
    src_v += src_stride_v * 2;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ScaleRowDown2(src_v, 0, dst_v, halfwidth);
  }
  return 0;
}

// TODO(fbarchard): Enable bilinear when fast enough or specialized upsampler.
// 411 chroma is 1/4 width, 1x height
// 420 chroma is 1/2 width, 1/2 height
LIBYUV_API
int I411ToI420(const uint8* src_y, int src_stride_y,
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

  // Copy Y plane
  if (dst_y) {
    CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  }

  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;
  int quarterwidth = (width + 3) >> 2;

  // Resample U plane from 1/4 width, 1x height to 1/2 width, 1/2 height.
  ScalePlane(src_u, src_stride_u, quarterwidth, height,
             dst_u, dst_stride_u, halfwidth, halfheight,
             kFilterNone);

  // Resample V plane.
  ScalePlane(src_v, src_stride_v, quarterwidth, height,
             dst_v, dst_stride_v, halfwidth, halfheight,
             kFilterNone);
  return 0;
}

// I400 is greyscale typically used in MJPG
LIBYUV_API
int I400ToI420(const uint8* src_y, int src_stride_y,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (!src_y || !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_stride_y;
    src_stride_y = -src_stride_y;
  }
  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;
  CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  SetPlane(dst_u, dst_stride_u, halfwidth, halfheight, 128);
  SetPlane(dst_v, dst_stride_v, halfwidth, halfheight, 128);
  return 0;
}

static void CopyPlane2(const uint8* src, int src_stride_0, int src_stride_1,
                       uint8* dst, int dst_stride,
                       int width, int height) {
  void (*CopyRow)(const uint8* src, uint8* dst, int width) = CopyRow_C;
#if defined(HAS_COPYROW_X86)
  if (TestCpuFlag(kCpuHasX86) && IS_ALIGNED(width, 4)) {
    CopyRow = CopyRow_X86;
  }
#endif
#if defined(HAS_COPYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 32) &&
      IS_ALIGNED(src, 16) &&
      IS_ALIGNED(src_stride_0, 16) && IS_ALIGNED(src_stride_1, 16) &&
      IS_ALIGNED(dst, 16) && IS_ALIGNED(dst_stride, 16)) {
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
  for (int y = 0; y < height - 1; y += 2) {
    CopyRow(src, dst, width);
    CopyRow(src + src_stride_0, dst + dst_stride, width);
    src += src_stride_0 + src_stride_1;
    dst += dst_stride * 2;
  }
  if (height & 1) {
    CopyRow(src, dst, width);
  }
}

// Support converting from FOURCC_M420
// Useful for bandwidth constrained transports like USB 1.0 and 2.0 and for
// easy conversion to I420.
// M420 format description:
// M420 is row biplanar 420: 2 rows of Y and 1 row of UV.
// Chroma is half width / half height. (420)
// src_stride_m420 is row planar. Normally this will be the width in pixels.
//   The UV plane is half width, but 2 values, so src_stride_m420 applies to
//   this as well as the two Y planes.
static int X420ToI420(const uint8* src_y,
                      int src_stride_y0, int src_stride_y1,
                      const uint8* src_uv, int src_stride_uv,
                      uint8* dst_y, int dst_stride_y,
                      uint8* dst_u, int dst_stride_u,
                      uint8* dst_v, int dst_stride_v,
                      int width, int height) {
  if (!src_y || !src_uv ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    int halfheight = (height + 1) >> 1;
    dst_y = dst_y + (height - 1) * dst_stride_y;
    dst_u = dst_u + (halfheight - 1) * dst_stride_u;
    dst_v = dst_v + (halfheight - 1) * dst_stride_v;
    dst_stride_y = -dst_stride_y;
    dst_stride_u = -dst_stride_u;
    dst_stride_v = -dst_stride_v;
  }

  int halfwidth = (width + 1) >> 1;
  void (*SplitUVRow)(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix) =
      SplitUVRow_C;
#if defined(HAS_SPLITUVROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && halfwidth >= 16) {
    SplitUVRow = SplitUVRow_Any_SSE2;
    if (IS_ALIGNED(halfwidth, 16)) {
      SplitUVRow = SplitUVRow_Unaligned_SSE2;
      if (IS_ALIGNED(src_uv, 16) && IS_ALIGNED(src_stride_uv, 16) &&
          IS_ALIGNED(dst_u, 16) && IS_ALIGNED(dst_stride_u, 16) &&
          IS_ALIGNED(dst_v, 16) && IS_ALIGNED(dst_stride_v, 16)) {
        SplitUVRow = SplitUVRow_SSE2;
      }
    }
  }
#endif
#if defined(HAS_SPLITUVROW_AVX2)
  if (TestCpuFlag(kCpuHasAVX2) && halfwidth >= 32) {
    SplitUVRow = SplitUVRow_Any_AVX2;
    if (IS_ALIGNED(halfwidth, 32)) {
      SplitUVRow = SplitUVRow_AVX2;
    }
  }
#endif
#if defined(HAS_SPLITUVROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && halfwidth >= 16) {
    SplitUVRow = SplitUVRow_Any_NEON;
    if (IS_ALIGNED(halfwidth, 16)) {
      SplitUVRow = SplitUVRow_NEON;
    }
  }
#endif
#if defined(HAS_SPLITUVROW_MIPS_DSPR2)
  if (TestCpuFlag(kCpuHasMIPS_DSPR2) && halfwidth >= 16) {
    SplitUVRow = SplitUVRow_Any_MIPS_DSPR2;
    if (IS_ALIGNED(halfwidth, 16)) {
      SplitUVRow = SplitUVRow_Unaligned_MIPS_DSPR2;
      if (IS_ALIGNED(src_uv, 4) && IS_ALIGNED(src_stride_uv, 4) &&
          IS_ALIGNED(dst_u, 4) && IS_ALIGNED(dst_stride_u, 4) &&
          IS_ALIGNED(dst_v, 4) && IS_ALIGNED(dst_stride_v, 4)) {
        SplitUVRow = SplitUVRow_MIPS_DSPR2;
      }
    }
  }
#endif

  if (dst_y) {
    if (src_stride_y0 == src_stride_y1) {
      CopyPlane(src_y, src_stride_y0, dst_y, dst_stride_y, width, height);
    } else {
      CopyPlane2(src_y, src_stride_y0, src_stride_y1, dst_y, dst_stride_y,
                 width, height);
    }
  }

  int halfheight = (height + 1) >> 1;
  for (int y = 0; y < halfheight; ++y) {
    // Copy a row of UV.
    SplitUVRow(src_uv, dst_u, dst_v, halfwidth);
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
    src_uv += src_stride_uv;
  }
  return 0;
}

// Convert NV12 to I420.
LIBYUV_API
int NV12ToI420(const uint8* src_y, int src_stride_y,
               const uint8* src_uv, int src_stride_uv,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  return X420ToI420(src_y, src_stride_y, src_stride_y,
                    src_uv, src_stride_uv,
                    dst_y, dst_stride_y,
                    dst_u, dst_stride_u,
                    dst_v, dst_stride_v,
                    width, height);
}

// Convert NV21 to I420.  Same as NV12 but u and v pointers swapped.
LIBYUV_API
int NV21ToI420(const uint8* src_y, int src_stride_y,
               const uint8* src_vu, int src_stride_vu,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  return X420ToI420(src_y, src_stride_y, src_stride_y,
                    src_vu, src_stride_vu,
                    dst_y, dst_stride_y,
                    dst_v, dst_stride_v,
                    dst_u, dst_stride_u,
                    width, height);
}

// Convert M420 to I420.
LIBYUV_API
int M420ToI420(const uint8* src_m420, int src_stride_m420,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  return X420ToI420(src_m420, src_stride_m420, src_stride_m420 * 2,
                    src_m420 + src_stride_m420 * 2, src_stride_m420 * 3,
                    dst_y, dst_stride_y,
                    dst_u, dst_stride_u,
                    dst_v, dst_stride_v,
                    width, height);
}

// Convert Q420 to I420.
// Format is rows of YY/YUYV
LIBYUV_API
int Q420ToI420(const uint8* src_y, int src_stride_y,
               const uint8* src_yuy2, int src_stride_yuy2,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (!src_y || !src_yuy2 ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    int halfheight = (height + 1) >> 1;
    dst_y = dst_y + (height - 1) * dst_stride_y;
    dst_u = dst_u + (halfheight - 1) * dst_stride_u;
    dst_v = dst_v + (halfheight - 1) * dst_stride_v;
    dst_stride_y = -dst_stride_y;
    dst_stride_u = -dst_stride_u;
    dst_stride_v = -dst_stride_v;
  }
  // CopyRow for rows of just Y in Q420 copied to Y plane of I420.
  void (*CopyRow)(const uint8* src, uint8* dst, int width) = CopyRow_C;
#if defined(HAS_COPYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 32)) {
    CopyRow = CopyRow_NEON;
  }
#endif
#if defined(HAS_COPYROW_X86)
  if (IS_ALIGNED(width, 4)) {
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
  if (TestCpuFlag(kCpuHasAVX2)) {
    CopyRow = CopyRow_AVX2;
  }
#endif
#if defined(HAS_COPYROW_MIPS)
  if (TestCpuFlag(kCpuHasMIPS)) {
    CopyRow = CopyRow_MIPS;
  }
#endif

  void (*YUY2ToUV422Row)(const uint8* src_yuy2, uint8* dst_u, uint8* dst_v,
      int pix) = YUY2ToUV422Row_C;
  void (*YUY2ToYRow)(const uint8* src_yuy2, uint8* dst_y, int pix) =
      YUY2ToYRow_C;
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
    clear = true;
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

  for (int y = 0; y < height - 1; y += 2) {
    CopyRow(src_y, dst_y, width);
    src_y += src_stride_y;
    dst_y += dst_stride_y;

    YUY2ToUV422Row(src_yuy2, dst_u, dst_v, width);
    YUY2ToYRow(src_yuy2, dst_y, width);
    src_yuy2 += src_stride_yuy2;
    dst_y += dst_stride_y;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    CopyRow(src_y, dst_y, width);
    YUY2ToUV422Row(src_yuy2, dst_u, dst_v, width);
  }
#if defined(HAS_YUY2TOYROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif
  return 0;
}

// Convert YUY2 to I420.
LIBYUV_API
int YUY2ToI420(const uint8* src_yuy2, int src_stride_yuy2,
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
  void (*YUY2ToUVRow)(const uint8* src_yuy2, int src_stride_yuy2,
                      uint8* dst_u, uint8* dst_v, int pix);
  void (*YUY2ToYRow)(const uint8* src_yuy2,
                     uint8* dst_y, int pix);
  YUY2ToYRow = YUY2ToYRow_C;
  YUY2ToUVRow = YUY2ToUVRow_C;
#if defined(HAS_YUY2TOYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 16) {
    YUY2ToUVRow = YUY2ToUVRow_Any_SSE2;
    YUY2ToYRow = YUY2ToYRow_Any_SSE2;
    if (IS_ALIGNED(width, 16)) {
      YUY2ToUVRow = YUY2ToUVRow_Unaligned_SSE2;
      YUY2ToYRow = YUY2ToYRow_Unaligned_SSE2;
      if (IS_ALIGNED(src_yuy2, 16) && IS_ALIGNED(src_stride_yuy2, 16)) {
        YUY2ToUVRow = YUY2ToUVRow_SSE2;
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
    YUY2ToUVRow = YUY2ToUVRow_Any_AVX2;
    YUY2ToYRow = YUY2ToYRow_Any_AVX2;
    if (IS_ALIGNED(width, 32)) {
      YUY2ToUVRow = YUY2ToUVRow_AVX2;
      YUY2ToYRow = YUY2ToYRow_AVX2;
    }
  }
#endif
#if defined(HAS_YUY2TOYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    YUY2ToYRow = YUY2ToYRow_Any_NEON;
    if (width >= 16) {
      YUY2ToUVRow = YUY2ToUVRow_Any_NEON;
    }
    if (IS_ALIGNED(width, 16)) {
      YUY2ToYRow = YUY2ToYRow_NEON;
      YUY2ToUVRow = YUY2ToUVRow_NEON;
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    YUY2ToUVRow(src_yuy2, src_stride_yuy2, dst_u, dst_v, width);
    YUY2ToYRow(src_yuy2, dst_y, width);
    YUY2ToYRow(src_yuy2 + src_stride_yuy2, dst_y + dst_stride_y, width);
    src_yuy2 += src_stride_yuy2 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    YUY2ToUVRow(src_yuy2, 0, dst_u, dst_v, width);
    YUY2ToYRow(src_yuy2, dst_y, width);
  }

#if defined(HAS_YUY2TOYROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif
  return 0;
}

// Convert UYVY to I420.
LIBYUV_API
int UYVYToI420(const uint8* src_uyvy, int src_stride_uyvy,
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
  void (*UYVYToUVRow)(const uint8* src_uyvy, int src_stride_uyvy,
                      uint8* dst_u, uint8* dst_v, int pix);
  void (*UYVYToYRow)(const uint8* src_uyvy,
                     uint8* dst_y, int pix);
  UYVYToYRow = UYVYToYRow_C;
  UYVYToUVRow = UYVYToUVRow_C;
#if defined(HAS_UYVYTOYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 16) {
    UYVYToUVRow = UYVYToUVRow_Any_SSE2;
    UYVYToYRow = UYVYToYRow_Any_SSE2;
    if (IS_ALIGNED(width, 16)) {
      UYVYToUVRow = UYVYToUVRow_Unaligned_SSE2;
      UYVYToYRow = UYVYToYRow_Unaligned_SSE2;
      if (IS_ALIGNED(src_uyvy, 16) && IS_ALIGNED(src_stride_uyvy, 16)) {
        UYVYToUVRow = UYVYToUVRow_SSE2;
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
    UYVYToUVRow = UYVYToUVRow_Any_AVX2;
    UYVYToYRow = UYVYToYRow_Any_AVX2;
    if (IS_ALIGNED(width, 32)) {
      UYVYToUVRow = UYVYToUVRow_AVX2;
      UYVYToYRow = UYVYToYRow_AVX2;
    }
  }
#endif
#if defined(HAS_UYVYTOYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    UYVYToYRow = UYVYToYRow_Any_NEON;
    if (width >= 16) {
      UYVYToUVRow = UYVYToUVRow_Any_NEON;
    }
    if (IS_ALIGNED(width, 16)) {
      UYVYToYRow = UYVYToYRow_NEON;
      UYVYToUVRow = UYVYToUVRow_NEON;
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    UYVYToUVRow(src_uyvy, src_stride_uyvy, dst_u, dst_v, width);
    UYVYToYRow(src_uyvy, dst_y, width);
    UYVYToYRow(src_uyvy + src_stride_uyvy, dst_y + dst_stride_y, width);
    src_uyvy += src_stride_uyvy * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    UYVYToUVRow(src_uyvy, 0, dst_u, dst_v, width);
    UYVYToYRow(src_uyvy, dst_y, width);
  }

#if defined(HAS_UYVYTOYROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif
  return 0;
}

// Convert ARGB to I420.
LIBYUV_API
int ARGBToI420(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (!src_argb ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
#if defined(HAS_ARGBTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_Unaligned_SSSE3;
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16)) {
        ARGBToUVRow = ARGBToUVRow_SSSE3;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          ARGBToYRow = ARGBToYRow_SSSE3;
        }
      }
    }
  }
#endif
#if defined(HAS_ARGBTOYROW_AVX2)
  bool clear = false;
  if (TestCpuFlag(kCpuHasAVX2) && width >= 32) {
    clear = true;
    ARGBToUVRow = ARGBToUVRow_Any_AVX2;
    ARGBToYRow = ARGBToYRow_Any_AVX2;
    if (IS_ALIGNED(width, 32)) {
      ARGBToUVRow = ARGBToUVRow_AVX2;
      ARGBToYRow = ARGBToYRow_AVX2;
    }
  }
#endif
#if defined(HAS_ARGBTOYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    ARGBToYRow = ARGBToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGBToYRow = ARGBToYRow_NEON;
    }
    if (width >= 16) {
      ARGBToUVRow = ARGBToUVRow_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        ARGBToUVRow = ARGBToUVRow_NEON;
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    ARGBToUVRow(src_argb, src_stride_argb, dst_u, dst_v, width);
    ARGBToYRow(src_argb, dst_y, width);
    ARGBToYRow(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
    src_argb += src_stride_argb * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGBToUVRow(src_argb, 0, dst_u, dst_v, width);
    ARGBToYRow(src_argb, dst_y, width);
  }

#if defined(HAS_ARGBTOYROW_AVX2)
  if (clear) {
    __asm vzeroupper;
  }
#endif
  return 0;
}

// Convert BGRA to I420.
LIBYUV_API
int BGRAToI420(const uint8* src_bgra, int src_stride_bgra,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (!src_bgra ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_bgra = src_bgra + (height - 1) * src_stride_bgra;
    src_stride_bgra = -src_stride_bgra;
  }
  void (*BGRAToUVRow)(const uint8* src_bgra0, int src_stride_bgra,
                      uint8* dst_u, uint8* dst_v, int width) = BGRAToUVRow_C;
  void (*BGRAToYRow)(const uint8* src_bgra, uint8* dst_y, int pix) =
      BGRAToYRow_C;
#if defined(HAS_BGRATOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    BGRAToUVRow = BGRAToUVRow_Any_SSSE3;
    BGRAToYRow = BGRAToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      BGRAToUVRow = BGRAToUVRow_Unaligned_SSSE3;
      BGRAToYRow = BGRAToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(src_bgra, 16) && IS_ALIGNED(src_stride_bgra, 16)) {
        BGRAToUVRow = BGRAToUVRow_SSSE3;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          BGRAToYRow = BGRAToYRow_SSSE3;
        }
      }
    }
  }
#elif defined(HAS_BGRATOYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    BGRAToYRow = BGRAToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      BGRAToYRow = BGRAToYRow_NEON;
    }
    if (width >= 16) {
      BGRAToUVRow = BGRAToUVRow_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        BGRAToUVRow = BGRAToUVRow_NEON;
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    BGRAToUVRow(src_bgra, src_stride_bgra, dst_u, dst_v, width);
    BGRAToYRow(src_bgra, dst_y, width);
    BGRAToYRow(src_bgra + src_stride_bgra, dst_y + dst_stride_y, width);
    src_bgra += src_stride_bgra * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    BGRAToUVRow(src_bgra, 0, dst_u, dst_v, width);
    BGRAToYRow(src_bgra, dst_y, width);
  }
  return 0;
}

// Convert ABGR to I420.
LIBYUV_API
int ABGRToI420(const uint8* src_abgr, int src_stride_abgr,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (!src_abgr ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_abgr = src_abgr + (height - 1) * src_stride_abgr;
    src_stride_abgr = -src_stride_abgr;
  }
  void (*ABGRToUVRow)(const uint8* src_abgr0, int src_stride_abgr,
                      uint8* dst_u, uint8* dst_v, int width) = ABGRToUVRow_C;
  void (*ABGRToYRow)(const uint8* src_abgr, uint8* dst_y, int pix) =
      ABGRToYRow_C;
#if defined(HAS_ABGRTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ABGRToUVRow = ABGRToUVRow_Any_SSSE3;
    ABGRToYRow = ABGRToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ABGRToUVRow = ABGRToUVRow_Unaligned_SSSE3;
      ABGRToYRow = ABGRToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(src_abgr, 16) && IS_ALIGNED(src_stride_abgr, 16)) {
        ABGRToUVRow = ABGRToUVRow_SSSE3;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          ABGRToYRow = ABGRToYRow_SSSE3;
        }
      }
    }
  }
#elif defined(HAS_ABGRTOYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    ABGRToYRow = ABGRToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ABGRToYRow = ABGRToYRow_NEON;
    }
    if (width >= 16) {
      ABGRToUVRow = ABGRToUVRow_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        ABGRToUVRow = ABGRToUVRow_NEON;
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    ABGRToUVRow(src_abgr, src_stride_abgr, dst_u, dst_v, width);
    ABGRToYRow(src_abgr, dst_y, width);
    ABGRToYRow(src_abgr + src_stride_abgr, dst_y + dst_stride_y, width);
    src_abgr += src_stride_abgr * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ABGRToUVRow(src_abgr, 0, dst_u, dst_v, width);
    ABGRToYRow(src_abgr, dst_y, width);
  }
  return 0;
}

// Convert RGBA to I420.
LIBYUV_API
int RGBAToI420(const uint8* src_rgba, int src_stride_rgba,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (!src_rgba ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_rgba = src_rgba + (height - 1) * src_stride_rgba;
    src_stride_rgba = -src_stride_rgba;
  }
  void (*RGBAToUVRow)(const uint8* src_rgba0, int src_stride_rgba,
                      uint8* dst_u, uint8* dst_v, int width) = RGBAToUVRow_C;
  void (*RGBAToYRow)(const uint8* src_rgba, uint8* dst_y, int pix) =
      RGBAToYRow_C;
#if defined(HAS_RGBATOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    RGBAToUVRow = RGBAToUVRow_Any_SSSE3;
    RGBAToYRow = RGBAToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      RGBAToUVRow = RGBAToUVRow_Unaligned_SSSE3;
      RGBAToYRow = RGBAToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(src_rgba, 16) && IS_ALIGNED(src_stride_rgba, 16)) {
        RGBAToUVRow = RGBAToUVRow_SSSE3;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          RGBAToYRow = RGBAToYRow_SSSE3;
        }
      }
    }
  }
#elif defined(HAS_RGBATOYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    RGBAToYRow = RGBAToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      RGBAToYRow = RGBAToYRow_NEON;
    }
    if (width >= 16) {
      RGBAToUVRow = RGBAToUVRow_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        RGBAToUVRow = RGBAToUVRow_NEON;
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    RGBAToUVRow(src_rgba, src_stride_rgba, dst_u, dst_v, width);
    RGBAToYRow(src_rgba, dst_y, width);
    RGBAToYRow(src_rgba + src_stride_rgba, dst_y + dst_stride_y, width);
    src_rgba += src_stride_rgba * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    RGBAToUVRow(src_rgba, 0, dst_u, dst_v, width);
    RGBAToYRow(src_rgba, dst_y, width);
  }
  return 0;
}

// Convert RGB24 to I420.
LIBYUV_API
int RGB24ToI420(const uint8* src_rgb24, int src_stride_rgb24,
                uint8* dst_y, int dst_stride_y,
                uint8* dst_u, int dst_stride_u,
                uint8* dst_v, int dst_stride_v,
                int width, int height) {
  if (!src_rgb24 || !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0 ||
      width * 4 > kMaxStride) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_rgb24 = src_rgb24 + (height - 1) * src_stride_rgb24;
    src_stride_rgb24 = -src_stride_rgb24;
  }

#if defined(HAS_RGB24TOYROW_NEON)
  void (*RGB24ToUVRow)(const uint8* src_rgb24, int src_stride_rgb24,
      uint8* dst_u, uint8* dst_v, int width) = RGB24ToUVRow_C;
  void (*RGB24ToYRow)(const uint8* src_rgb24, uint8* dst_y, int pix) =
      RGB24ToYRow_C;
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    RGB24ToYRow = RGB24ToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      RGB24ToYRow = RGB24ToYRow_NEON;
    }
    if (width >= 16) {
      RGB24ToUVRow = RGB24ToUVRow_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        RGB24ToUVRow = RGB24ToUVRow_NEON;
      }
    }
  }
#else  // HAS_RGB24TOYROW_NEON
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*RGB24ToARGBRow)(const uint8* src_rgb, uint8* dst_argb, int pix) =
      RGB24ToARGBRow_C;
#if defined(HAS_RGB24TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    RGB24ToARGBRow = RGB24ToARGBRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      RGB24ToARGBRow = RGB24ToARGBRow_SSSE3;
    }
  }
#endif
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_SSSE3;
    }
  }
#endif
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
        ARGBToYRow = ARGBToYRow_SSSE3;
      }
    }
  }
#endif  // HAS_ARGBTOUVROW_SSSE3
#endif  // HAS_RGB24TOYROW_NEON

  for (int y = 0; y < height - 1; y += 2) {
#if defined(HAS_RGB24TOYROW_NEON)
    RGB24ToUVRow(src_rgb24, src_stride_rgb24, dst_u, dst_v, width);
    RGB24ToYRow(src_rgb24, dst_y, width);
    RGB24ToYRow(src_rgb24 + src_stride_rgb24, dst_y + dst_stride_y, width);
#else
    RGB24ToARGBRow(src_rgb24, row, width);
    RGB24ToARGBRow(src_rgb24 + src_stride_rgb24, row + kMaxStride, width);
    ARGBToUVRow(row, kMaxStride, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
    ARGBToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
#endif
    src_rgb24 += src_stride_rgb24 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
#if defined(HAS_RGB24TOYROW_NEON)
    RGB24ToUVRow(src_rgb24, 0, dst_u, dst_v, width);
    RGB24ToYRow(src_rgb24, dst_y, width);
#else
    RGB24ToARGBRow(src_rgb24, row, width);
    ARGBToUVRow(row, 0, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
#endif
  }
  return 0;
}

// Convert RAW to I420.
LIBYUV_API
int RAWToI420(const uint8* src_raw, int src_stride_raw,
              uint8* dst_y, int dst_stride_y,
              uint8* dst_u, int dst_stride_u,
              uint8* dst_v, int dst_stride_v,
              int width, int height) {
  if (!src_raw || !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0 ||
      width * 4 > kMaxStride) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_raw = src_raw + (height - 1) * src_stride_raw;
    src_stride_raw = -src_stride_raw;
  }

#if defined(HAS_RAWTOYROW_NEON)
  void (*RAWToUVRow)(const uint8* src_raw, int src_stride_raw,
      uint8* dst_u, uint8* dst_v, int width) = RAWToUVRow_C;
  void (*RAWToYRow)(const uint8* src_raw, uint8* dst_y, int pix) =
      RAWToYRow_C;
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    RAWToYRow = RAWToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      RAWToYRow = RAWToYRow_NEON;
    }
    if (width >= 16) {
      RAWToUVRow = RAWToUVRow_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        RAWToUVRow = RAWToUVRow_NEON;
      }
    }
  }
#else  // HAS_RAWTOYROW_NEON
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*RAWToARGBRow)(const uint8* src_rgb, uint8* dst_argb, int pix) =
      RAWToARGBRow_C;
#if defined(HAS_RAWTOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    RAWToARGBRow = RAWToARGBRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      RAWToARGBRow = RAWToARGBRow_SSSE3;
    }
  }
#endif
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_SSSE3;
    }
  }
#endif
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
        ARGBToYRow = ARGBToYRow_SSSE3;
      }
    }
  }
#endif  // HAS_ARGBTOUVROW_SSSE3
#endif  // HAS_RAWTOYROW_NEON

  for (int y = 0; y < height - 1; y += 2) {
#if defined(HAS_RAWTOYROW_NEON)
    RAWToUVRow(src_raw, src_stride_raw, dst_u, dst_v, width);
    RAWToYRow(src_raw, dst_y, width);
    RAWToYRow(src_raw + src_stride_raw, dst_y + dst_stride_y, width);
#else
    RAWToARGBRow(src_raw, row, width);
    RAWToARGBRow(src_raw + src_stride_raw, row + kMaxStride, width);
    ARGBToUVRow(row, kMaxStride, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
    ARGBToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
#endif
    src_raw += src_stride_raw * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
#if defined(HAS_RAWTOYROW_NEON)
    RAWToUVRow(src_raw, 0, dst_u, dst_v, width);
    RAWToYRow(src_raw, dst_y, width);
#else
    RAWToARGBRow(src_raw, row, width);
    ARGBToUVRow(row, 0, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
#endif
  }
  return 0;
}

// Convert RGB565 to I420.
LIBYUV_API
int RGB565ToI420(const uint8* src_rgb565, int src_stride_rgb565,
                uint8* dst_y, int dst_stride_y,
                uint8* dst_u, int dst_stride_u,
                uint8* dst_v, int dst_stride_v,
                int width, int height) {
  if (!src_rgb565 || !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0 ||
      width * 4 > kMaxStride) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_rgb565 = src_rgb565 + (height - 1) * src_stride_rgb565;
    src_stride_rgb565 = -src_stride_rgb565;
  }

#if defined(HAS_RGB565TOYROW_NEON)
  void (*RGB565ToUVRow)(const uint8* src_rgb565, int src_stride_rgb565,
      uint8* dst_u, uint8* dst_v, int width) = RGB565ToUVRow_C;
  void (*RGB565ToYRow)(const uint8* src_rgb565, uint8* dst_y, int pix) =
      RGB565ToYRow_C;
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    RGB565ToYRow = RGB565ToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      RGB565ToYRow = RGB565ToYRow_NEON;
    }
    if (width >= 16) {
      RGB565ToUVRow = RGB565ToUVRow_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        RGB565ToUVRow = RGB565ToUVRow_NEON;
      }
    }
  }
#else  // HAS_RGB565TOYROW_NEON
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*RGB565ToARGBRow)(const uint8* src_rgb, uint8* dst_argb, int pix) =
      RGB565ToARGBRow_C;
#if defined(HAS_RGB565TOARGBROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 8) {
    RGB565ToARGBRow = RGB565ToARGBRow_Any_SSE2;
    if (IS_ALIGNED(width, 8)) {
      RGB565ToARGBRow = RGB565ToARGBRow_SSE2;
    }
  }
#endif
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_SSSE3;
    }
  }
#endif
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
        ARGBToYRow = ARGBToYRow_SSSE3;
      }
    }
  }
#endif  // HAS_ARGBTOUVROW_SSSE3
#endif  // HAS_RGB565TOYROW_NEON

  for (int y = 0; y < height - 1; y += 2) {
#if defined(HAS_RGB565TOYROW_NEON)
    RGB565ToUVRow(src_rgb565, src_stride_rgb565, dst_u, dst_v, width);
    RGB565ToYRow(src_rgb565, dst_y, width);
    RGB565ToYRow(src_rgb565 + src_stride_rgb565, dst_y + dst_stride_y, width);
#else
    RGB565ToARGBRow(src_rgb565, row, width);
    RGB565ToARGBRow(src_rgb565 + src_stride_rgb565, row + kMaxStride, width);
    ARGBToUVRow(row, kMaxStride, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
    ARGBToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
#endif
    src_rgb565 += src_stride_rgb565 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
#if defined(HAS_RGB565TOYROW_NEON)
    RGB565ToUVRow(src_rgb565, 0, dst_u, dst_v, width);
    RGB565ToYRow(src_rgb565, dst_y, width);
#else
    RGB565ToARGBRow(src_rgb565, row, width);
    ARGBToUVRow(row, 0, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
#endif
  }
  return 0;
}

// Convert ARGB1555 to I420.
LIBYUV_API
int ARGB1555ToI420(const uint8* src_argb1555, int src_stride_argb1555,
                   uint8* dst_y, int dst_stride_y,
                   uint8* dst_u, int dst_stride_u,
                   uint8* dst_v, int dst_stride_v,
                   int width, int height) {
  if (!src_argb1555 || !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0 ||
      width * 4 > kMaxStride) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_argb1555 = src_argb1555 + (height - 1) * src_stride_argb1555;
    src_stride_argb1555 = -src_stride_argb1555;
  }

#if defined(HAS_ARGB1555TOYROW_NEON)
  void (*ARGB1555ToUVRow)(const uint8* src_argb1555, int src_stride_argb1555,
      uint8* dst_u, uint8* dst_v, int width) = ARGB1555ToUVRow_C;
  void (*ARGB1555ToYRow)(const uint8* src_argb1555, uint8* dst_y, int pix) =
      ARGB1555ToYRow_C;
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    ARGB1555ToYRow = ARGB1555ToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGB1555ToYRow = ARGB1555ToYRow_NEON;
    }
    if (width >= 16) {
      ARGB1555ToUVRow = ARGB1555ToUVRow_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        ARGB1555ToUVRow = ARGB1555ToUVRow_NEON;
      }
    }
  }
#else  // HAS_ARGB1555TOYROW_NEON
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*ARGB1555ToARGBRow)(const uint8* src_rgb, uint8* dst_argb, int pix) =
      ARGB1555ToARGBRow_C;
#if defined(HAS_ARGB1555TOARGBROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 8) {
    ARGB1555ToARGBRow = ARGB1555ToARGBRow_Any_SSE2;
    if (IS_ALIGNED(width, 8)) {
      ARGB1555ToARGBRow = ARGB1555ToARGBRow_SSE2;
    }
  }
#endif
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_SSSE3;
    }
  }
#endif
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
        ARGBToYRow = ARGBToYRow_SSSE3;
      }
    }
  }
#endif  // HAS_ARGBTOUVROW_SSSE3
#endif  // HAS_ARGB1555TOYROW_NEON

  for (int y = 0; y < height - 1; y += 2) {
#if defined(HAS_ARGB1555TOYROW_NEON)
    ARGB1555ToUVRow(src_argb1555, src_stride_argb1555, dst_u, dst_v, width);
    ARGB1555ToYRow(src_argb1555, dst_y, width);
    ARGB1555ToYRow(src_argb1555 + src_stride_argb1555, dst_y + dst_stride_y,
                   width);
#else
    ARGB1555ToARGBRow(src_argb1555, row, width);
    ARGB1555ToARGBRow(src_argb1555 + src_stride_argb1555, row + kMaxStride,
                      width);
    ARGBToUVRow(row, kMaxStride, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
    ARGBToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
#endif
    src_argb1555 += src_stride_argb1555 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
#if defined(HAS_ARGB1555TOYROW_NEON)
    ARGB1555ToUVRow(src_argb1555, 0, dst_u, dst_v, width);
    ARGB1555ToYRow(src_argb1555, dst_y, width);
#else
    ARGB1555ToARGBRow(src_argb1555, row, width);
    ARGBToUVRow(row, 0, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
#endif
  }
  return 0;
}

// Convert ARGB4444 to I420.
LIBYUV_API
int ARGB4444ToI420(const uint8* src_argb4444, int src_stride_argb4444,
                   uint8* dst_y, int dst_stride_y,
                   uint8* dst_u, int dst_stride_u,
                   uint8* dst_v, int dst_stride_v,
                   int width, int height) {
  if (!src_argb4444 || !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0 ||
      width * 4 > kMaxStride) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_argb4444 = src_argb4444 + (height - 1) * src_stride_argb4444;
    src_stride_argb4444 = -src_stride_argb4444;
  }

#if defined(HAS_ARGB4444TOYROW_NEON)
  void (*ARGB4444ToUVRow)(const uint8* src_argb4444, int src_stride_argb4444,
      uint8* dst_u, uint8* dst_v, int width) = ARGB4444ToUVRow_C;
  void (*ARGB4444ToYRow)(const uint8* src_argb4444, uint8* dst_y, int pix) =
      ARGB4444ToYRow_C;
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    ARGB4444ToYRow = ARGB4444ToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGB4444ToYRow = ARGB4444ToYRow_NEON;
    }
    if (width >= 16) {
      ARGB4444ToUVRow = ARGB4444ToUVRow_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        ARGB4444ToUVRow = ARGB4444ToUVRow_NEON;
      }
    }
  }
#else  // HAS_ARGB4444TOYROW_NEON
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*ARGB4444ToARGBRow)(const uint8* src_rgb, uint8* dst_argb, int pix) =
      ARGB4444ToARGBRow_C;
#if defined(HAS_ARGB4444TOARGBROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 8) {
    ARGB4444ToARGBRow = ARGB4444ToARGBRow_Any_SSE2;
    if (IS_ALIGNED(width, 8)) {
      ARGB4444ToARGBRow = ARGB4444ToARGBRow_SSE2;
    }
  }
#endif
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_SSSE3;
    }
  }
#endif
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 16) {
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
        ARGBToYRow = ARGBToYRow_SSSE3;
      }
    }
  }
#endif  // HAS_ARGBTOUVROW_SSSE3
#endif  // HAS_ARGB4444TOYROW_NEON

  for (int y = 0; y < height - 1; y += 2) {
#if defined(HAS_ARGB4444TOYROW_NEON)
    ARGB4444ToUVRow(src_argb4444, src_stride_argb4444, dst_u, dst_v, width);
    ARGB4444ToYRow(src_argb4444, dst_y, width);
    ARGB4444ToYRow(src_argb4444 + src_stride_argb4444, dst_y + dst_stride_y,
                   width);
#else
    ARGB4444ToARGBRow(src_argb4444, row, width);
    ARGB4444ToARGBRow(src_argb4444 + src_stride_argb4444, row + kMaxStride,
                      width);
    ARGBToUVRow(row, kMaxStride, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
    ARGBToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
#endif
    src_argb4444 += src_stride_argb4444 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
#if defined(HAS_ARGB4444TOYROW_NEON)
    ARGB4444ToUVRow(src_argb4444, 0, dst_u, dst_v, width);
    ARGB4444ToYRow(src_argb4444, dst_y, width);
#else
    ARGB4444ToARGBRow(src_argb4444, row, width);
    ARGBToUVRow(row, 0, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
#endif
  }
  return 0;
}

#ifdef HAVE_JPEG
struct I420Buffers {
  uint8* y;
  int y_stride;
  uint8* u;
  int u_stride;
  uint8* v;
  int v_stride;
  int w;
  int h;
};

static void JpegCopyI420(void* opaque,
                         const uint8* const* data,
                         const int* strides,
                         int rows) {
  I420Buffers* dest = static_cast<I420Buffers*>(opaque);
  I420Copy(data[0], strides[0],
           data[1], strides[1],
           data[2], strides[2],
           dest->y, dest->y_stride,
           dest->u, dest->u_stride,
           dest->v, dest->v_stride,
           dest->w, rows);
  dest->y += rows * dest->y_stride;
  dest->u += ((rows + 1) >> 1) * dest->u_stride;
  dest->v += ((rows + 1) >> 1) * dest->v_stride;
  dest->h -= rows;
}

static void JpegI422ToI420(void* opaque,
                           const uint8* const* data,
                           const int* strides,
                           int rows) {
  I420Buffers* dest = static_cast<I420Buffers*>(opaque);
  I422ToI420(data[0], strides[0],
             data[1], strides[1],
             data[2], strides[2],
             dest->y, dest->y_stride,
             dest->u, dest->u_stride,
             dest->v, dest->v_stride,
             dest->w, rows);
  dest->y += rows * dest->y_stride;
  dest->u += ((rows + 1) >> 1) * dest->u_stride;
  dest->v += ((rows + 1) >> 1) * dest->v_stride;
  dest->h -= rows;
}

static void JpegI444ToI420(void* opaque,
                           const uint8* const* data,
                           const int* strides,
                           int rows) {
  I420Buffers* dest = static_cast<I420Buffers*>(opaque);
  I444ToI420(data[0], strides[0],
             data[1], strides[1],
             data[2], strides[2],
             dest->y, dest->y_stride,
             dest->u, dest->u_stride,
             dest->v, dest->v_stride,
             dest->w, rows);
  dest->y += rows * dest->y_stride;
  dest->u += ((rows + 1) >> 1) * dest->u_stride;
  dest->v += ((rows + 1) >> 1) * dest->v_stride;
  dest->h -= rows;
}

static void JpegI411ToI420(void* opaque,
                           const uint8* const* data,
                           const int* strides,
                           int rows) {
  I420Buffers* dest = static_cast<I420Buffers*>(opaque);
  I411ToI420(data[0], strides[0],
             data[1], strides[1],
             data[2], strides[2],
             dest->y, dest->y_stride,
             dest->u, dest->u_stride,
             dest->v, dest->v_stride,
             dest->w, rows);
  dest->y += rows * dest->y_stride;
  dest->u += ((rows + 1) >> 1) * dest->u_stride;
  dest->v += ((rows + 1) >> 1) * dest->v_stride;
  dest->h -= rows;
}

static void JpegI400ToI420(void* opaque,
                           const uint8* const* data,
                           const int* strides,
                           int rows) {
  I420Buffers* dest = static_cast<I420Buffers*>(opaque);
  I400ToI420(data[0], strides[0],
             dest->y, dest->y_stride,
             dest->u, dest->u_stride,
             dest->v, dest->v_stride,
             dest->w, rows);
  dest->y += rows * dest->y_stride;
  dest->u += ((rows + 1) >> 1) * dest->u_stride;
  dest->v += ((rows + 1) >> 1) * dest->v_stride;
  dest->h -= rows;
}

// Query size of MJPG in pixels.
LIBYUV_API
int MJPGSize(const uint8* sample, size_t sample_size,
             int* width, int* height) {
  MJpegDecoder mjpeg_decoder;
  bool ret = mjpeg_decoder.LoadFrame(sample, sample_size);
  if (ret) {
    *width = mjpeg_decoder.GetWidth();
    *height = mjpeg_decoder.GetHeight();
  }
  mjpeg_decoder.UnloadFrame();
  return ret ? 0 : -1;  // -1 for runtime failure.
}

// MJPG (Motion JPeg) to I420
// TODO(fbarchard): review w and h requirement. dw and dh may be enough.
LIBYUV_API
int MJPGToI420(const uint8* sample,
               size_t sample_size,
               uint8* y, int y_stride,
               uint8* u, int u_stride,
               uint8* v, int v_stride,
               int w, int h,
               int dw, int dh) {
  if (sample_size == kUnknownDataSize) {
    // ERROR: MJPEG frame size unknown
    return -1;
  }

  // TODO(fbarchard): Port MJpeg to C.
  MJpegDecoder mjpeg_decoder;
  bool ret = mjpeg_decoder.LoadFrame(sample, sample_size);
  if (ret && (mjpeg_decoder.GetWidth() != w ||
              mjpeg_decoder.GetHeight() != h)) {
    // ERROR: MJPEG frame has unexpected dimensions
    mjpeg_decoder.UnloadFrame();
    return 1;  // runtime failure
  }
  if (ret) {
    I420Buffers bufs = { y, y_stride, u, u_stride, v, v_stride, dw, dh };
    // YUV420
    if (mjpeg_decoder.GetColorSpace() ==
            MJpegDecoder::kColorSpaceYCbCr &&
        mjpeg_decoder.GetNumComponents() == 3 &&
        mjpeg_decoder.GetVertSampFactor(0) == 2 &&
        mjpeg_decoder.GetHorizSampFactor(0) == 2 &&
        mjpeg_decoder.GetVertSampFactor(1) == 1 &&
        mjpeg_decoder.GetHorizSampFactor(1) == 1 &&
        mjpeg_decoder.GetVertSampFactor(2) == 1 &&
        mjpeg_decoder.GetHorizSampFactor(2) == 1) {
      ret = mjpeg_decoder.DecodeToCallback(&JpegCopyI420, &bufs, dw, dh);
    // YUV422
    } else if (mjpeg_decoder.GetColorSpace() ==
                   MJpegDecoder::kColorSpaceYCbCr &&
               mjpeg_decoder.GetNumComponents() == 3 &&
               mjpeg_decoder.GetVertSampFactor(0) == 1 &&
               mjpeg_decoder.GetHorizSampFactor(0) == 2 &&
               mjpeg_decoder.GetVertSampFactor(1) == 1 &&
               mjpeg_decoder.GetHorizSampFactor(1) == 1 &&
               mjpeg_decoder.GetVertSampFactor(2) == 1 &&
               mjpeg_decoder.GetHorizSampFactor(2) == 1) {
      ret = mjpeg_decoder.DecodeToCallback(&JpegI422ToI420, &bufs, dw, dh);
    // YUV444
    } else if (mjpeg_decoder.GetColorSpace() ==
                   MJpegDecoder::kColorSpaceYCbCr &&
               mjpeg_decoder.GetNumComponents() == 3 &&
               mjpeg_decoder.GetVertSampFactor(0) == 1 &&
               mjpeg_decoder.GetHorizSampFactor(0) == 1 &&
               mjpeg_decoder.GetVertSampFactor(1) == 1 &&
               mjpeg_decoder.GetHorizSampFactor(1) == 1 &&
               mjpeg_decoder.GetVertSampFactor(2) == 1 &&
               mjpeg_decoder.GetHorizSampFactor(2) == 1) {
      ret = mjpeg_decoder.DecodeToCallback(&JpegI444ToI420, &bufs, dw, dh);
    // YUV411
    } else if (mjpeg_decoder.GetColorSpace() ==
                   MJpegDecoder::kColorSpaceYCbCr &&
               mjpeg_decoder.GetNumComponents() == 3 &&
               mjpeg_decoder.GetVertSampFactor(0) == 1 &&
               mjpeg_decoder.GetHorizSampFactor(0) == 4 &&
               mjpeg_decoder.GetVertSampFactor(1) == 1 &&
               mjpeg_decoder.GetHorizSampFactor(1) == 1 &&
               mjpeg_decoder.GetVertSampFactor(2) == 1 &&
               mjpeg_decoder.GetHorizSampFactor(2) == 1) {
      ret = mjpeg_decoder.DecodeToCallback(&JpegI411ToI420, &bufs, dw, dh);
    // YUV400
    } else if (mjpeg_decoder.GetColorSpace() ==
                   MJpegDecoder::kColorSpaceGrayscale &&
               mjpeg_decoder.GetNumComponents() == 1 &&
               mjpeg_decoder.GetVertSampFactor(0) == 1 &&
               mjpeg_decoder.GetHorizSampFactor(0) == 1) {
      ret = mjpeg_decoder.DecodeToCallback(&JpegI400ToI420, &bufs, dw, dh);
    } else {
      // TODO(fbarchard): Implement conversion for any other colorspace/sample
      // factors that occur in practice. 411 is supported by libjpeg
      // ERROR: Unable to convert MJPEG frame because format is not supported
      mjpeg_decoder.UnloadFrame();
      return 1;
    }
  }
  return 0;
}
#endif

// Convert camera sample to I420 with cropping, rotation and vertical flip.
// src_width is used for source stride computation
// src_height is used to compute location of planes, and indicate inversion
// sample_size is measured in bytes and is the size of the frame.
//   With MJPEG it is the compressed size of the frame.
LIBYUV_API
int ConvertToI420(const uint8* sample,
#ifdef HAVE_JPEG
                  size_t sample_size,
#else
                  size_t /* sample_size */,
#endif
                  uint8* y, int y_stride,
                  uint8* u, int u_stride,
                  uint8* v, int v_stride,
                  int crop_x, int crop_y,
                  int src_width, int src_height,
                  int dst_width, int dst_height,
                  RotationMode rotation,
                  uint32 fourcc) {
  uint32 format = CanonicalFourCC(fourcc);
  if (!y || !u || !v || !sample ||
      src_width <= 0 || dst_width <= 0  ||
      src_height == 0 || dst_height == 0) {
    return -1;
  }
  int aligned_src_width = (src_width + 1) & ~1;
  const uint8* src;
  const uint8* src_uv;
  int abs_src_height = (src_height < 0) ? -src_height : src_height;
  int inv_dst_height = (dst_height < 0) ? -dst_height : dst_height;
  if (src_height < 0) {
    inv_dst_height = -inv_dst_height;
  }
  int r = 0;

  // One pass rotation is available for some formats. For the rest, convert
  // to I420 (with optional vertical flipping) into a temporary I420 buffer,
  // and then rotate the I420 to the final destination buffer.
  // For in-place conversion, if destination y is same as source sample,
  // also enable temporary buffer.
  bool need_buf = (rotation && format != FOURCC_I420 &&
      format != FOURCC_NV12 && format != FOURCC_NV21 &&
      format != FOURCC_YU12 && format != FOURCC_YV12) || y == sample;
  uint8* tmp_y = y;
  uint8* tmp_u = u;
  uint8* tmp_v = v;
  int tmp_y_stride = y_stride;
  int tmp_u_stride = u_stride;
  int tmp_v_stride = v_stride;
  uint8* buf = NULL;
  int abs_dst_height = (dst_height < 0) ? -dst_height : dst_height;
  if (need_buf) {
    int y_size = dst_width * abs_dst_height;
    int uv_size = ((dst_width + 1) / 2) * ((abs_dst_height + 1) / 2);
    buf = new uint8[y_size + uv_size * 2];
    if (!buf) {
      return 1;  // Out of memory runtime error.
    }
    y = buf;
    u = y + y_size;
    v = u + uv_size;
    y_stride = dst_width;
    u_stride = v_stride = ((dst_width + 1) / 2);
  }

  switch (format) {
    // Single plane formats
    case FOURCC_YUY2:
      src = sample + (aligned_src_width * crop_y + crop_x) * 2;
      r = YUY2ToI420(src, aligned_src_width * 2,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    case FOURCC_UYVY:
      src = sample + (aligned_src_width * crop_y + crop_x) * 2;
      r = UYVYToI420(src, aligned_src_width * 2,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    case FOURCC_RGBP:
      src = sample + (src_width * crop_y + crop_x) * 2;
      r = RGB565ToI420(src, src_width * 2,
                       y, y_stride,
                       u, u_stride,
                       v, v_stride,
                       dst_width, inv_dst_height);
      break;
    case FOURCC_RGBO:
      src = sample + (src_width * crop_y + crop_x) * 2;
      r = ARGB1555ToI420(src, src_width * 2,
                         y, y_stride,
                         u, u_stride,
                         v, v_stride,
                         dst_width, inv_dst_height);
      break;
    case FOURCC_R444:
      src = sample + (src_width * crop_y + crop_x) * 2;
      r = ARGB4444ToI420(src, src_width * 2,
                         y, y_stride,
                         u, u_stride,
                         v, v_stride,
                         dst_width, inv_dst_height);
      break;
    case FOURCC_24BG:
      src = sample + (src_width * crop_y + crop_x) * 3;
      r = RGB24ToI420(src, src_width * 3,
                      y, y_stride,
                      u, u_stride,
                      v, v_stride,
                      dst_width, inv_dst_height);
      break;
    case FOURCC_RAW:
      src = sample + (src_width * crop_y + crop_x) * 3;
      r = RAWToI420(src, src_width * 3,
                    y, y_stride,
                    u, u_stride,
                    v, v_stride,
                    dst_width, inv_dst_height);
      break;
    case FOURCC_ARGB:
      src = sample + (src_width * crop_y + crop_x) * 4;
      r = ARGBToI420(src, src_width * 4,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    case FOURCC_BGRA:
      src = sample + (src_width * crop_y + crop_x) * 4;
      r = BGRAToI420(src, src_width * 4,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    case FOURCC_ABGR:
      src = sample + (src_width * crop_y + crop_x) * 4;
      r = ABGRToI420(src, src_width * 4,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    case FOURCC_RGBA:
      src = sample + (src_width * crop_y + crop_x) * 4;
      r = RGBAToI420(src, src_width * 4,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    // TODO(fbarchard): Support cropping Bayer by odd numbers
    // by adjusting fourcc.
    case FOURCC_BGGR:
      src = sample + (src_width * crop_y + crop_x);
      r = BayerBGGRToI420(src, src_width,
                          y, y_stride,
                          u, u_stride,
                          v, v_stride,
                          dst_width, inv_dst_height);
      break;
    case FOURCC_GBRG:
      src = sample + (src_width * crop_y + crop_x);
      r = BayerGBRGToI420(src, src_width,
                          y, y_stride,
                          u, u_stride,
                          v, v_stride,
                          dst_width, inv_dst_height);
      break;
    case FOURCC_GRBG:
      src = sample + (src_width * crop_y + crop_x);
      r = BayerGRBGToI420(src, src_width,
                          y, y_stride,
                          u, u_stride,
                          v, v_stride,
                          dst_width, inv_dst_height);
      break;
    case FOURCC_RGGB:
      src = sample + (src_width * crop_y + crop_x);
      r = BayerRGGBToI420(src, src_width,
                          y, y_stride,
                          u, u_stride,
                          v, v_stride,
                          dst_width, inv_dst_height);
      break;
    case FOURCC_I400:
      src = sample + src_width * crop_y + crop_x;
      r = I400ToI420(src, src_width,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    // Biplanar formats
    case FOURCC_NV12:
      src = sample + (src_width * crop_y + crop_x);
      src_uv = sample + aligned_src_width * (src_height + crop_y / 2) + crop_x;
      r = NV12ToI420Rotate(src, src_width,
                           src_uv, aligned_src_width,
                           y, y_stride,
                           u, u_stride,
                           v, v_stride,
                           dst_width, inv_dst_height, rotation);
      break;
    case FOURCC_NV21:
      src = sample + (src_width * crop_y + crop_x);
      src_uv = sample + aligned_src_width * (src_height + crop_y / 2) + crop_x;
      // Call NV12 but with u and v parameters swapped.
      r = NV12ToI420Rotate(src, src_width,
                           src_uv, aligned_src_width,
                           y, y_stride,
                           v, v_stride,
                           u, u_stride,
                           dst_width, inv_dst_height, rotation);
      break;
    case FOURCC_M420:
      src = sample + (src_width * crop_y) * 12 / 8 + crop_x;
      r = M420ToI420(src, src_width,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    case FOURCC_Q420:
      src = sample + (src_width + aligned_src_width * 2) * crop_y + crop_x;
      src_uv = sample + (src_width + aligned_src_width * 2) * crop_y +
               src_width + crop_x * 2;
      r = Q420ToI420(src, src_width * 3,
                    src_uv, src_width * 3,
                    y, y_stride,
                    u, u_stride,
                    v, v_stride,
                    dst_width, inv_dst_height);
      break;
    // Triplanar formats
    case FOURCC_I420:
    case FOURCC_YU12:
    case FOURCC_YV12: {
      const uint8* src_y = sample + (src_width * crop_y + crop_x);
      const uint8* src_u;
      const uint8* src_v;
      int halfwidth = (src_width + 1) / 2;
      int halfheight = (abs_src_height + 1) / 2;
      if (format == FOURCC_YV12) {
        src_v = sample + src_width * abs_src_height +
            (halfwidth * crop_y + crop_x) / 2;
        src_u = sample + src_width * abs_src_height +
            halfwidth * (halfheight + crop_y / 2) + crop_x / 2;
      } else {
        src_u = sample + src_width * abs_src_height +
            (halfwidth * crop_y + crop_x) / 2;
        src_v = sample + src_width * abs_src_height +
            halfwidth * (halfheight + crop_y / 2) + crop_x / 2;
      }
      r = I420Rotate(src_y, src_width,
                     src_u, halfwidth,
                     src_v, halfwidth,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height, rotation);
      break;
    }
    case FOURCC_I422:
    case FOURCC_YV16: {
      const uint8* src_y = sample + src_width * crop_y + crop_x;
      const uint8* src_u;
      const uint8* src_v;
      int halfwidth = (src_width + 1) / 2;
      if (format == FOURCC_YV16) {
        src_v = sample + src_width * abs_src_height +
            halfwidth * crop_y + crop_x / 2;
        src_u = sample + src_width * abs_src_height +
            halfwidth * (abs_src_height + crop_y) + crop_x / 2;
      } else {
        src_u = sample + src_width * abs_src_height +
            halfwidth * crop_y + crop_x / 2;
        src_v = sample + src_width * abs_src_height +
            halfwidth * (abs_src_height + crop_y) + crop_x / 2;
      }
      r = I422ToI420(src_y, src_width,
                     src_u, halfwidth,
                     src_v, halfwidth,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    }
    case FOURCC_I444:
    case FOURCC_YV24: {
      const uint8* src_y = sample + src_width * crop_y + crop_x;
      const uint8* src_u;
      const uint8* src_v;
      if (format == FOURCC_YV24) {
        src_v = sample + src_width * (abs_src_height + crop_y) + crop_x;
        src_u = sample + src_width * (abs_src_height * 2 + crop_y) + crop_x;
      } else {
        src_u = sample + src_width * (abs_src_height + crop_y) + crop_x;
        src_v = sample + src_width * (abs_src_height * 2 + crop_y) + crop_x;
      }
      r = I444ToI420(src_y, src_width,
                     src_u, src_width,
                     src_v, src_width,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    }
    case FOURCC_I411: {
      int quarterwidth = (src_width + 3) / 4;
      const uint8* src_y = sample + src_width * crop_y + crop_x;
      const uint8* src_u = sample + src_width * abs_src_height +
          quarterwidth * crop_y + crop_x / 4;
      const uint8* src_v = sample + src_width * abs_src_height +
          quarterwidth * (abs_src_height + crop_y) + crop_x / 4;
      r = I411ToI420(src_y, src_width,
                     src_u, quarterwidth,
                     src_v, quarterwidth,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    }
#ifdef HAVE_JPEG
    case FOURCC_MJPG:
      r = MJPGToI420(sample, sample_size,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     src_width, abs_src_height, dst_width, inv_dst_height);
      break;
#endif
    default:
      r = -1;  // unknown fourcc - return failure code.
  }

  if (need_buf) {
    if (!r) {
      r = I420Rotate(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     tmp_y, tmp_y_stride,
                     tmp_u, tmp_u_stride,
                     tmp_v, tmp_v_stride,
                     dst_width, abs_dst_height, rotation);
    }
    delete buf;
  }

  return r;
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif
