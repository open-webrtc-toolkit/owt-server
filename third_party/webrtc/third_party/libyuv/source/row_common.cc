/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"

#include <string.h>  // For memcpy and memset.

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#ifdef LIBYUV_LITTLE_ENDIAN
#define WRITEWORD(p, v) *reinterpret_cast<uint32*>(p) = v
#else
static inline void WRITEWORD(uint8* p, uint32 v) {
  p[0] = (uint8)(v & 255);
  p[1] = (uint8)((v >> 8) & 255);
  p[2] = (uint8)((v >> 16) & 255);
  p[3] = (uint8)((v >> 24) & 255);
}
#endif

void BGRAToARGBRow_C(const uint8* src_bgra, uint8* dst_argb, int width) {
  for (int x = 0; x < width; ++x) {
    // To support in-place conversion.
    uint8 a = src_bgra[0];
    uint8 r = src_bgra[1];
    uint8 g = src_bgra[2];
    uint8 b = src_bgra[3];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = a;
    dst_argb += 4;
    src_bgra += 4;
  }
}

void ABGRToARGBRow_C(const uint8* src_abgr, uint8* dst_argb, int width) {
  for (int x = 0; x < width; ++x) {
    // To support in-place conversion.
    uint8 r = src_abgr[0];
    uint8 g = src_abgr[1];
    uint8 b = src_abgr[2];
    uint8 a = src_abgr[3];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = a;
    dst_argb += 4;
    src_abgr += 4;
  }
}

void RGBAToARGBRow_C(const uint8* src_abgr, uint8* dst_argb, int width) {
  for (int x = 0; x < width; ++x) {
    // To support in-place conversion.
    uint8 a = src_abgr[0];
    uint8 b = src_abgr[1];
    uint8 g = src_abgr[2];
    uint8 r = src_abgr[3];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = a;
    dst_argb += 4;
    src_abgr += 4;
  }
}

void RGB24ToARGBRow_C(const uint8* src_rgb24, uint8* dst_argb, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 b = src_rgb24[0];
    uint8 g = src_rgb24[1];
    uint8 r = src_rgb24[2];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_rgb24 += 3;
  }
}

void RAWToARGBRow_C(const uint8* src_raw, uint8* dst_argb, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 r = src_raw[0];
    uint8 g = src_raw[1];
    uint8 b = src_raw[2];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_raw += 3;
  }
}

void RGB565ToARGBRow_C(const uint8* src_rgb565, uint8* dst_argb, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 b = src_rgb565[0] & 0x1f;
    uint8 g = (src_rgb565[0] >> 5) | ((src_rgb565[1] & 0x07) << 3);
    uint8 r = src_rgb565[1] >> 3;
    dst_argb[0] = (b << 3) | (b >> 2);
    dst_argb[1] = (g << 2) | (g >> 4);
    dst_argb[2] = (r << 3) | (r >> 2);
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_rgb565 += 2;
  }
}

void ARGB1555ToARGBRow_C(const uint8* src_argb1555, uint8* dst_argb,
                         int width) {
  for (int x = 0; x < width; ++x) {
    uint8 b = src_argb1555[0] & 0x1f;
    uint8 g = (src_argb1555[0] >> 5) | ((src_argb1555[1] & 0x03) << 3);
    uint8 r = (src_argb1555[1] & 0x7c) >> 2;
    uint8 a = src_argb1555[1] >> 7;
    dst_argb[0] = (b << 3) | (b >> 2);
    dst_argb[1] = (g << 3) | (g >> 2);
    dst_argb[2] = (r << 3) | (r >> 2);
    dst_argb[3] = -a;
    dst_argb += 4;
    src_argb1555 += 2;
  }
}

void ARGB4444ToARGBRow_C(const uint8* src_argb4444, uint8* dst_argb,
                         int width) {
  for (int x = 0; x < width; ++x) {
    uint8 b = src_argb4444[0] & 0x0f;
    uint8 g = src_argb4444[0] >> 4;
    uint8 r = src_argb4444[1] & 0x0f;
    uint8 a = src_argb4444[1] >> 4;
    dst_argb[0] = (b << 4) | b;
    dst_argb[1] = (g << 4) | g;
    dst_argb[2] = (r << 4) | r;
    dst_argb[3] = (a << 4) | a;
    dst_argb += 4;
    src_argb4444 += 2;
  }
}

void ARGBToRGBARow_C(const uint8* src_argb, uint8* dst_rgb, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 b = src_argb[0];
    uint8 g = src_argb[1];
    uint8 r = src_argb[2];
    uint8 a = src_argb[3];
    dst_rgb[0] = a;
    dst_rgb[1] = b;
    dst_rgb[2] = g;
    dst_rgb[3] = r;
    dst_rgb += 4;
    src_argb += 4;
  }
}

void ARGBToRGB24Row_C(const uint8* src_argb, uint8* dst_rgb, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 b = src_argb[0];
    uint8 g = src_argb[1];
    uint8 r = src_argb[2];
    dst_rgb[0] = b;
    dst_rgb[1] = g;
    dst_rgb[2] = r;
    dst_rgb += 3;
    src_argb += 4;
  }
}

void ARGBToRAWRow_C(const uint8* src_argb, uint8* dst_rgb, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 b = src_argb[0];
    uint8 g = src_argb[1];
    uint8 r = src_argb[2];
    dst_rgb[0] = r;
    dst_rgb[1] = g;
    dst_rgb[2] = b;
    dst_rgb += 3;
    src_argb += 4;
  }
}

void ARGBToRGB565Row_C(const uint8* src_argb, uint8* dst_rgb, int width) {
  for (int x = 0; x < width - 1; x += 2) {
    uint8 b0 = src_argb[0] >> 3;
    uint8 g0 = src_argb[1] >> 2;
    uint8 r0 = src_argb[2] >> 3;
    uint8 b1 = src_argb[4] >> 3;
    uint8 g1 = src_argb[5] >> 2;
    uint8 r1 = src_argb[6] >> 3;
    WRITEWORD(dst_rgb, b0 | (g0 << 5) | (r0 << 11) |
              (b1 << 16) | (g1 << 21) | (r1 << 27));
    dst_rgb += 4;
    src_argb += 8;
  }
  if (width & 1) {
    uint8 b0 = src_argb[0] >> 3;
    uint8 g0 = src_argb[1] >> 2;
    uint8 r0 = src_argb[2] >> 3;
    *reinterpret_cast<uint16*>(dst_rgb) = b0 | (g0 << 5) | (r0 << 11);
  }
}

void ARGBToARGB1555Row_C(const uint8* src_argb, uint8* dst_rgb, int width) {
  for (int x = 0; x < width - 1; x += 2) {
    uint8 b0 = src_argb[0] >> 3;
    uint8 g0 = src_argb[1] >> 3;
    uint8 r0 = src_argb[2] >> 3;
    uint8 a0 = src_argb[3] >> 7;
    uint8 b1 = src_argb[4] >> 3;
    uint8 g1 = src_argb[5] >> 3;
    uint8 r1 = src_argb[6] >> 3;
    uint8 a1 = src_argb[7] >> 7;
    *reinterpret_cast<uint32*>(dst_rgb) =
        b0 | (g0 << 5) | (r0 << 10) | (a0 << 15) |
        (b1 << 16) | (g1 << 21) | (r1 << 26) | (a1 << 31);
    dst_rgb += 4;
    src_argb += 8;
  }
  if (width & 1) {
    uint8 b0 = src_argb[0] >> 3;
    uint8 g0 = src_argb[1] >> 3;
    uint8 r0 = src_argb[2] >> 3;
    uint8 a0 = src_argb[3] >> 7;
    *reinterpret_cast<uint16*>(dst_rgb) =
        b0 | (g0 << 5) | (r0 << 10) | (a0 << 15);
  }
}

void ARGBToARGB4444Row_C(const uint8* src_argb, uint8* dst_rgb, int width) {
  for (int x = 0; x < width - 1; x += 2) {
    uint8 b0 = src_argb[0] >> 4;
    uint8 g0 = src_argb[1] >> 4;
    uint8 r0 = src_argb[2] >> 4;
    uint8 a0 = src_argb[3] >> 4;
    uint8 b1 = src_argb[4] >> 4;
    uint8 g1 = src_argb[5] >> 4;
    uint8 r1 = src_argb[6] >> 4;
    uint8 a1 = src_argb[7] >> 4;
    *reinterpret_cast<uint32*>(dst_rgb) =
        b0 | (g0 << 4) | (r0 << 8) | (a0 << 12) |
        (b1 << 16) | (g1 << 20) | (r1 << 24) | (a1 << 28);
    dst_rgb += 4;
    src_argb += 8;
  }
  if (width & 1) {
    uint8 b0 = src_argb[0] >> 4;
    uint8 g0 = src_argb[1] >> 4;
    uint8 r0 = src_argb[2] >> 4;
    uint8 a0 = src_argb[3] >> 4;
    *reinterpret_cast<uint16*>(dst_rgb) =
        b0 | (g0 << 4) | (r0 << 8) | (a0 << 12);
  }
}

static __inline int RGBToY(uint8 r, uint8 g, uint8 b) {
  return (66 * r + 129 * g +  25 * b + 0x1080) >> 8;
}

static __inline int RGBToU(uint8 r, uint8 g, uint8 b) {
  return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static __inline int RGBToV(uint8 r, uint8 g, uint8 b) {
  return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}

#define MAKEROWY(NAME, R, G, B, BPP) \
void NAME ## ToYRow_C(const uint8* src_argb0, uint8* dst_y, int width) {       \
  for (int x = 0; x < width; ++x) {                                            \
    dst_y[0] = RGBToY(src_argb0[R], src_argb0[G], src_argb0[B]);               \
    src_argb0 += BPP;                                                          \
    dst_y += 1;                                                                \
  }                                                                            \
}                                                                              \
void NAME ## ToUVRow_C(const uint8* src_rgb0, int src_stride_rgb,              \
                       uint8* dst_u, uint8* dst_v, int width) {                \
  const uint8* src_rgb1 = src_rgb0 + src_stride_rgb;                           \
  for (int x = 0; x < width - 1; x += 2) {                                     \
    uint8 ab = (src_rgb0[B] + src_rgb0[B + BPP] +                              \
               src_rgb1[B] + src_rgb1[B + BPP]) >> 2;                          \
    uint8 ag = (src_rgb0[G] + src_rgb0[G + BPP] +                              \
               src_rgb1[G] + src_rgb1[G + BPP]) >> 2;                          \
    uint8 ar = (src_rgb0[R] + src_rgb0[R + BPP] +                              \
               src_rgb1[R] + src_rgb1[R + BPP]) >> 2;                          \
    dst_u[0] = RGBToU(ar, ag, ab);                                             \
    dst_v[0] = RGBToV(ar, ag, ab);                                             \
    src_rgb0 += BPP * 2;                                                       \
    src_rgb1 += BPP * 2;                                                       \
    dst_u += 1;                                                                \
    dst_v += 1;                                                                \
  }                                                                            \
  if (width & 1) {                                                             \
    uint8 ab = (src_rgb0[B] + src_rgb1[B]) >> 1;                               \
    uint8 ag = (src_rgb0[G] + src_rgb1[G]) >> 1;                               \
    uint8 ar = (src_rgb0[R] + src_rgb1[R]) >> 1;                               \
    dst_u[0] = RGBToU(ar, ag, ab);                                             \
    dst_v[0] = RGBToV(ar, ag, ab);                                             \
  }                                                                            \
}

MAKEROWY(ARGB, 2, 1, 0, 4)
MAKEROWY(BGRA, 1, 2, 3, 4)
MAKEROWY(ABGR, 0, 1, 2, 4)
MAKEROWY(RGBA, 3, 2, 1, 4)
MAKEROWY(RGB24, 2, 1, 0, 3)
MAKEROWY(RAW, 0, 1, 2, 3)
#undef MAKEROWY

void RGB565ToYRow_C(const uint8* src_rgb565, uint8* dst_y, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 b = src_rgb565[0] & 0x1f;
    uint8 g = (src_rgb565[0] >> 5) | ((src_rgb565[1] & 0x07) << 3);
    uint8 r = src_rgb565[1] >> 3;
    b = (b << 3) | (b >> 2);
    g = (g << 2) | (g >> 4);
    r = (r << 3) | (r >> 2);
    dst_y[0] = RGBToY(r, g, b);
    src_rgb565 += 2;
    dst_y += 1;
  }
}

void ARGB1555ToYRow_C(const uint8* src_argb1555, uint8* dst_y, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 b = src_argb1555[0] & 0x1f;
    uint8 g = (src_argb1555[0] >> 5) | ((src_argb1555[1] & 0x03) << 3);
    uint8 r = (src_argb1555[1] & 0x7c) >> 2;
    b = (b << 3) | (b >> 2);
    g = (g << 3) | (g >> 2);
    r = (r << 3) | (r >> 2);
    dst_y[0] = RGBToY(r, g, b);
    src_argb1555 += 2;
    dst_y += 1;
  }
}

void ARGB4444ToYRow_C(const uint8* src_argb4444, uint8* dst_y, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 b = src_argb4444[0] & 0x0f;
    uint8 g = src_argb4444[0] >> 4;
    uint8 r = src_argb4444[1] & 0x0f;
    b = (b << 4) | b;
    g = (g << 4) | g;
    r = (r << 4) | r;
    dst_y[0] = RGBToY(r, g, b);
    src_argb4444 += 2;
    dst_y += 1;
  }
}

void RGB565ToUVRow_C(const uint8* src_rgb565, int src_stride_rgb565,
                     uint8* dst_u, uint8* dst_v, int width) {
  const uint8* next_rgb565 = src_rgb565 + src_stride_rgb565;
  for (int x = 0; x < width - 1; x += 2) {
    uint8 b0 = src_rgb565[0] & 0x1f;
    uint8 g0 = (src_rgb565[0] >> 5) | ((src_rgb565[1] & 0x07) << 3);
    uint8 r0 = src_rgb565[1] >> 3;
    uint8 b1 = src_rgb565[2] & 0x1f;
    uint8 g1 = (src_rgb565[2] >> 5) | ((src_rgb565[3] & 0x07) << 3);
    uint8 r1 = src_rgb565[3] >> 3;
    uint8 b2 = next_rgb565[0] & 0x1f;
    uint8 g2 = (next_rgb565[0] >> 5) | ((next_rgb565[1] & 0x07) << 3);
    uint8 r2 = next_rgb565[1] >> 3;
    uint8 b3 = next_rgb565[2] & 0x1f;
    uint8 g3 = (next_rgb565[2] >> 5) | ((next_rgb565[3] & 0x07) << 3);
    uint8 r3 = next_rgb565[3] >> 3;
    uint8 b = (b0 + b1 + b2 + b3);  // 565 * 4 = 787.
    uint8 g = (g0 + g1 + g2 + g3);
    uint8 r = (r0 + r1 + r2 + r3);
    b = (b << 1) | (b >> 6);  // 787 -> 888.
    r = (r << 1) | (r >> 6);
    dst_u[0] = RGBToU(r, g, b);
    dst_v[0] = RGBToV(r, g, b);
    src_rgb565 += 4;
    next_rgb565 += 4;
    dst_u += 1;
    dst_v += 1;
  }
  if (width & 1) {
    uint8 b0 = src_rgb565[0] & 0x1f;
    uint8 g0 = (src_rgb565[0] >> 5) | ((src_rgb565[1] & 0x07) << 3);
    uint8 r0 = src_rgb565[1] >> 3;
    uint8 b2 = next_rgb565[0] & 0x1f;
    uint8 g2 = (next_rgb565[0] >> 5) | ((next_rgb565[1] & 0x07) << 3);
    uint8 r2 = next_rgb565[1] >> 3;
    uint8 b = (b0 + b2);  // 565 * 2 = 676.
    uint8 g = (g0 + g2);
    uint8 r = (r0 + r2);
    b = (b << 2) | (b >> 4);  // 676 -> 888
    g = (g << 1) | (g >> 6);
    r = (r << 2) | (r >> 4);
    dst_u[0] = RGBToU(r, g, b);
    dst_v[0] = RGBToV(r, g, b);
  }
}

void ARGB1555ToUVRow_C(const uint8* src_argb1555, int src_stride_argb1555,
                       uint8* dst_u, uint8* dst_v, int width) {
  const uint8* next_argb1555 = src_argb1555 + src_stride_argb1555;
  for (int x = 0; x < width - 1; x += 2) {
    uint8 b0 = src_argb1555[0] & 0x1f;
    uint8 g0 = (src_argb1555[0] >> 5) | ((src_argb1555[1] & 0x03) << 3);
    uint8 r0 = (src_argb1555[1] & 0x7c) >> 2;
    uint8 b1 = src_argb1555[2] & 0x1f;
    uint8 g1 = (src_argb1555[2] >> 5) | ((src_argb1555[3] & 0x03) << 3);
    uint8 r1 = (src_argb1555[3] & 0x7c) >> 2;
    uint8 b2 = next_argb1555[0] & 0x1f;
    uint8 g2 = (next_argb1555[0] >> 5) | ((next_argb1555[1] & 0x03) << 3);
    uint8 r2 = (next_argb1555[1] & 0x7c) >> 2;
    uint8 b3 = next_argb1555[2] & 0x1f;
    uint8 g3 = (next_argb1555[2] >> 5) | ((next_argb1555[3] & 0x03) << 3);
    uint8 r3 = (next_argb1555[3] & 0x7c) >> 2;
    uint8 b = (b0 + b1 + b2 + b3);  // 555 * 4 = 777.
    uint8 g = (g0 + g1 + g2 + g3);
    uint8 r = (r0 + r1 + r2 + r3);
    b = (b << 1) | (b >> 6);  // 777 -> 888.
    g = (g << 1) | (g >> 6);
    r = (r << 1) | (r >> 6);
    dst_u[0] = RGBToU(r, g, b);
    dst_v[0] = RGBToV(r, g, b);
    src_argb1555 += 4;
    next_argb1555 += 4;
    dst_u += 1;
    dst_v += 1;
  }
  if (width & 1) {
    uint8 b0 = src_argb1555[0] & 0x1f;
    uint8 g0 = (src_argb1555[0] >> 5) | ((src_argb1555[1] & 0x03) << 3);
    uint8 r0 = (src_argb1555[1] & 0x7c) >> 2;
    uint8 b2 = next_argb1555[0] & 0x1f;
    uint8 g2 = (next_argb1555[0] >> 5) | ((next_argb1555[1] & 0x03) << 3);
    uint8 r2 = next_argb1555[1] >> 3;
    uint8 b = (b0 + b2);  // 555 * 2 = 666.
    uint8 g = (g0 + g2);
    uint8 r = (r0 + r2);
    b = (b << 2) | (b >> 4);  // 666 -> 888.
    g = (g << 2) | (g >> 4);
    r = (r << 2) | (r >> 4);
    dst_u[0] = RGBToU(r, g, b);
    dst_v[0] = RGBToV(r, g, b);
  }
}

void ARGB4444ToUVRow_C(const uint8* src_argb4444, int src_stride_argb4444,
                       uint8* dst_u, uint8* dst_v, int width) {
  const uint8* next_argb4444 = src_argb4444 + src_stride_argb4444;
  for (int x = 0; x < width - 1; x += 2) {
    uint8 b0 = src_argb4444[0] & 0x0f;
    uint8 g0 = src_argb4444[0] >> 4;
    uint8 r0 = src_argb4444[1] & 0x0f;
    uint8 b1 = src_argb4444[2] & 0x0f;
    uint8 g1 = src_argb4444[2] >> 4;
    uint8 r1 = src_argb4444[3] & 0x0f;
    uint8 b2 = next_argb4444[0] & 0x0f;
    uint8 g2 = next_argb4444[0] >> 4;
    uint8 r2 = next_argb4444[1] & 0x0f;
    uint8 b3 = next_argb4444[2] & 0x0f;
    uint8 g3 = next_argb4444[2] >> 4;
    uint8 r3 = next_argb4444[3] & 0x0f;
    uint8 b = (b0 + b1 + b2 + b3);  // 444 * 4 = 666.
    uint8 g = (g0 + g1 + g2 + g3);
    uint8 r = (r0 + r1 + r2 + r3);
    b = (b << 2) | (b >> 4);  // 666 -> 888.
    g = (g << 2) | (g >> 4);
    r = (r << 2) | (r >> 4);
    dst_u[0] = RGBToU(r, g, b);
    dst_v[0] = RGBToV(r, g, b);
    src_argb4444 += 4;
    next_argb4444 += 4;
    dst_u += 1;
    dst_v += 1;
  }
  if (width & 1) {
    uint8 b0 = src_argb4444[0] & 0x0f;
    uint8 g0 = src_argb4444[0] >> 4;
    uint8 r0 = src_argb4444[1] & 0x0f;
    uint8 b2 = next_argb4444[0] & 0x0f;
    uint8 g2 = next_argb4444[0] >> 4;
    uint8 r2 = next_argb4444[1] & 0x0f;
    uint8 b = (b0 + b2);  // 444 * 2 = 555.
    uint8 g = (g0 + g2);
    uint8 r = (r0 + r2);
    b = (b << 3) | (b >> 2);  // 555 -> 888.
    g = (g << 3) | (g >> 2);
    r = (r << 3) | (r >> 2);
    dst_u[0] = RGBToU(r, g, b);
    dst_v[0] = RGBToV(r, g, b);
  }
}

void ARGBToUV444Row_C(const uint8* src_argb,
                      uint8* dst_u, uint8* dst_v, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 ab = src_argb[0];
    uint8 ag = src_argb[1];
    uint8 ar = src_argb[2];
    dst_u[0] = RGBToU(ar, ag, ab);
    dst_v[0] = RGBToV(ar, ag, ab);
    src_argb += 4;
    dst_u += 1;
    dst_v += 1;
  }
}

void ARGBToUV422Row_C(const uint8* src_argb,
                      uint8* dst_u, uint8* dst_v, int width) {
  for (int x = 0; x < width - 1; x += 2) {
    uint8 ab = (src_argb[0] + src_argb[4]) >> 1;
    uint8 ag = (src_argb[1] + src_argb[5]) >> 1;
    uint8 ar = (src_argb[2] + src_argb[6]) >> 1;
    dst_u[0] = RGBToU(ar, ag, ab);
    dst_v[0] = RGBToV(ar, ag, ab);
    src_argb += 8;
    dst_u += 1;
    dst_v += 1;
  }
  if (width & 1) {
    uint8 ab = src_argb[0];
    uint8 ag = src_argb[1];
    uint8 ar = src_argb[2];
    dst_u[0] = RGBToU(ar, ag, ab);
    dst_v[0] = RGBToV(ar, ag, ab);
  }
}

void ARGBToUV411Row_C(const uint8* src_argb,
                      uint8* dst_u, uint8* dst_v, int width) {
  for (int x = 0; x < width - 3; x += 4) {
    uint8 ab = (src_argb[0] + src_argb[4] + src_argb[8] + src_argb[12]) >> 2;
    uint8 ag = (src_argb[1] + src_argb[5] + src_argb[9] + src_argb[13]) >> 2;
    uint8 ar = (src_argb[2] + src_argb[6] + src_argb[10] + src_argb[14]) >> 2;
    dst_u[0] = RGBToU(ar, ag, ab);
    dst_v[0] = RGBToV(ar, ag, ab);
    src_argb += 16;
    dst_u += 1;
    dst_v += 1;
  }
  if ((width & 3) == 3) {
    uint8 ab = (src_argb[0] + src_argb[4] + src_argb[8]) / 3;
    uint8 ag = (src_argb[1] + src_argb[5] + src_argb[9]) / 3;
    uint8 ar = (src_argb[2] + src_argb[6] + src_argb[10]) / 3;
    dst_u[0] = RGBToU(ar, ag, ab);
    dst_v[0] = RGBToV(ar, ag, ab);
  } else if ((width & 3) == 2) {
    uint8 ab = (src_argb[0] + src_argb[4]) >> 1;
    uint8 ag = (src_argb[1] + src_argb[5]) >> 1;
    uint8 ar = (src_argb[2] + src_argb[6]) >> 1;
    dst_u[0] = RGBToU(ar, ag, ab);
    dst_v[0] = RGBToV(ar, ag, ab);
  } else if ((width & 3) == 1) {
    uint8 ab = src_argb[0];
    uint8 ag = src_argb[1];
    uint8 ar = src_argb[2];
    dst_u[0] = RGBToU(ar, ag, ab);
    dst_v[0] = RGBToV(ar, ag, ab);
  }
}

// http://en.wikipedia.org/wiki/Grayscale.
// 0.11 * B + 0.59 * G + 0.30 * R
// Coefficients rounded to multiple of 2 for consistency with SSSE3 version.
static __inline int RGBToGray(uint8 r, uint8 g, uint8 b) {
  return (28 * b + 152 * g + 76 * r) >> 8;
}

void ARGBGrayRow_C(const uint8* src_argb, uint8* dst_argb, int width) {
  for (int x = 0; x < width; ++x) {
    uint8 y = RGBToGray(src_argb[2], src_argb[1], src_argb[0]);
    dst_argb[2] = dst_argb[1] = dst_argb[0] = y;
    dst_argb[3] = src_argb[3];
    dst_argb += 4;
    src_argb += 4;
  }
}

// Convert a row of image to Sepia tone.
void ARGBSepiaRow_C(uint8* dst_argb, int width) {
  for (int x = 0; x < width; ++x) {
    int b = dst_argb[0];
    int g = dst_argb[1];
    int r = dst_argb[2];
    int sb = (b * 17 + g * 68 + r * 35) >> 7;
    int sg = (b * 22 + g * 88 + r * 45) >> 7;
    int sr = (b * 24 + g * 98 + r * 50) >> 7;
    // b does not over flow. a is preserved from original.
    if (sg > 255) {
      sg = 255;
    }
    if (sr > 255) {
      sr = 255;
    }
    dst_argb[0] = sb;
    dst_argb[1] = sg;
    dst_argb[2] = sr;
    dst_argb += 4;
  }
}

// Apply color matrix to a row of image. Matrix is signed.
void ARGBColorMatrixRow_C(uint8* dst_argb, const int8* matrix_argb, int width) {
  for (int x = 0; x < width; ++x) {
    int b = dst_argb[0];
    int g = dst_argb[1];
    int r = dst_argb[2];
    int a = dst_argb[3];
    int sb = (b * matrix_argb[0] + g * matrix_argb[1] +
              r * matrix_argb[2] + a * matrix_argb[3]) >> 7;
    int sg = (b * matrix_argb[4] + g * matrix_argb[5] +
              r * matrix_argb[6] + a * matrix_argb[7]) >> 7;
    int sr = (b * matrix_argb[8] + g * matrix_argb[9] +
              r * matrix_argb[10] + a * matrix_argb[11]) >> 7;
    if (sb < 0) {
      sb = 0;
    }
    if (sb > 255) {
      sb = 255;
    }
    if (sg < 0) {
      sg = 0;
    }
    if (sg > 255) {
      sg = 255;
    }
    if (sr < 0) {
      sr = 0;
    }
    if (sr > 255) {
      sr = 255;
    }
    dst_argb[0] = sb;
    dst_argb[1] = sg;
    dst_argb[2] = sr;
    dst_argb += 4;
  }
}

// Apply color table to a row of image.
void ARGBColorTableRow_C(uint8* dst_argb, const uint8* table_argb, int width) {
  for (int x = 0; x < width; ++x) {
    int b = dst_argb[0];
    int g = dst_argb[1];
    int r = dst_argb[2];
    int a = dst_argb[3];
    dst_argb[0] = table_argb[b * 4 + 0];
    dst_argb[1] = table_argb[g * 4 + 1];
    dst_argb[2] = table_argb[r * 4 + 2];
    dst_argb[3] = table_argb[a * 4 + 3];
    dst_argb += 4;
  }
}

void ARGBQuantizeRow_C(uint8* dst_argb, int scale, int interval_size,
                       int interval_offset, int width) {
  for (int x = 0; x < width; ++x) {
    int b = dst_argb[0];
    int g = dst_argb[1];
    int r = dst_argb[2];
    dst_argb[0] = (b * scale >> 16) * interval_size + interval_offset;
    dst_argb[1] = (g * scale >> 16) * interval_size + interval_offset;
    dst_argb[2] = (r * scale >> 16) * interval_size + interval_offset;
    dst_argb += 4;
  }
}

#define REPEAT8(v) (v) | ((v) << 8)
#define SHADE(f, v) v * f >> 24

void ARGBShadeRow_C(const uint8* src_argb, uint8* dst_argb, int width,
                    uint32 value) {
  const uint32 b_scale = REPEAT8(value & 0xff);
  const uint32 g_scale = REPEAT8((value >> 8) & 0xff);
  const uint32 r_scale = REPEAT8((value >> 16) & 0xff);
  const uint32 a_scale = REPEAT8(value >> 24);

  for (int i = 0; i < width; ++i) {
    const uint32 b = REPEAT8(src_argb[0]);
    const uint32 g = REPEAT8(src_argb[1]);
    const uint32 r = REPEAT8(src_argb[2]);
    const uint32 a = REPEAT8(src_argb[3]);
    dst_argb[0] = SHADE(b, b_scale);
    dst_argb[1] = SHADE(g, g_scale);
    dst_argb[2] = SHADE(r, r_scale);
    dst_argb[3] = SHADE(a, a_scale);
    src_argb += 4;
    dst_argb += 4;
  }
}
#undef REPEAT8
#undef SHADE

#define REPEAT8(v) (v) | ((v) << 8)
#define SHADE(f, v) v * f >> 16

void ARGBMultiplyRow_C(const uint8* src_argb0, const uint8* src_argb1,
                       uint8* dst_argb, int width) {
  for (int i = 0; i < width; ++i) {
    const uint32 b = REPEAT8(src_argb0[0]);
    const uint32 g = REPEAT8(src_argb0[1]);
    const uint32 r = REPEAT8(src_argb0[2]);
    const uint32 a = REPEAT8(src_argb0[3]);
    const uint32 b_scale = src_argb1[0];
    const uint32 g_scale = src_argb1[1];
    const uint32 r_scale = src_argb1[2];
    const uint32 a_scale = src_argb1[3];
    dst_argb[0] = SHADE(b, b_scale);
    dst_argb[1] = SHADE(g, g_scale);
    dst_argb[2] = SHADE(r, r_scale);
    dst_argb[3] = SHADE(a, a_scale);
    src_argb0 += 4;
    src_argb1 += 4;
    dst_argb += 4;
  }
}
#undef REPEAT8
#undef SHADE

#define SHADE(f, v) ((v + f) > 255) ? 255 : (v + f)

void ARGBAddRow_C(const uint8* src_argb0, const uint8* src_argb1,
                  uint8* dst_argb, int width) {
  for (int i = 0; i < width; ++i) {
    const uint32 b = src_argb0[0];
    const uint32 g = src_argb0[1];
    const uint32 r = src_argb0[2];
    const uint32 a = src_argb0[3];
    const uint32 b_add = src_argb1[0];
    const uint32 g_add = src_argb1[1];
    const uint32 r_add = src_argb1[2];
    const uint32 a_add = src_argb1[3];
    dst_argb[0] = SHADE(b, b_add);
    dst_argb[1] = SHADE(g, g_add);
    dst_argb[2] = SHADE(r, r_add);
    dst_argb[3] = SHADE(a, a_add);
    src_argb0 += 4;
    src_argb1 += 4;
    dst_argb += 4;
  }
}
#undef SHADE

#define SHADE(f, v) ((f - v) > f) ? 0 : (f - v)

void ARGBSubtractRow_C(const uint8* src_argb0, const uint8* src_argb1,
                       uint8* dst_argb, int width) {
  for (int i = 0; i < width; ++i) {
    const uint32 b = src_argb0[0];
    const uint32 g = src_argb0[1];
    const uint32 r = src_argb0[2];
    const uint32 a = src_argb0[3];
    const uint32 b_sub = src_argb1[0];
    const uint32 g_sub = src_argb1[1];
    const uint32 r_sub = src_argb1[2];
    const uint32 a_sub = src_argb1[3];
    dst_argb[0] = SHADE(b, b_sub);
    dst_argb[1] = SHADE(g, g_sub);
    dst_argb[2] = SHADE(r, r_sub);
    dst_argb[3] = SHADE(a, a_sub);
    src_argb0 += 4;
    src_argb1 += 4;
    dst_argb += 4;
  }
}
#undef SHADE

void I400ToARGBRow_C(const uint8* src_y, uint8* dst_argb, int width) {
  // Copy a Y to RGB.
  for (int x = 0; x < width; ++x) {
    uint8 y = src_y[0];
    dst_argb[2] = dst_argb[1] = dst_argb[0] = y;
    dst_argb[3] = 255u;
    dst_argb += 4;
    ++src_y;
  }
}

// C reference code that mimics the YUV assembly.

#define YG 74 /* static_cast<int8>(1.164 * 64 + 0.5) */

#define UB 127 /* min(63,static_cast<int8>(2.018 * 64)) */
#define UG -25 /* static_cast<int8>(-0.391 * 64 - 0.5) */
#define UR 0

#define VB 0
#define VG -52 /* static_cast<int8>(-0.813 * 64 - 0.5) */
#define VR 102 /* static_cast<int8>(1.596 * 64 + 0.5) */

// Bias
#define BB UB * 128 + VB * 128
#define BG UG * 128 + VG * 128
#define BR UR * 128 + VR * 128

static __inline uint32 Clip(int32 val) {
  if (val < 0) {
    return static_cast<uint32>(0);
  } else if (val > 255) {
    return static_cast<uint32>(255);
  }
  return static_cast<uint32>(val);
}

static __inline void YuvPixel(uint8 y, uint8 u, uint8 v, uint8* rgb_buf,
                              int ashift, int rshift, int gshift, int bshift) {
  int32 y1 = (static_cast<int32>(y) - 16) * YG;
  uint32 b = Clip(static_cast<int32>((u * UB + v * VB) - (BB) + y1) >> 6);
  uint32 g = Clip(static_cast<int32>((u * UG + v * VG) - (BG) + y1) >> 6);
  uint32 r = Clip(static_cast<int32>((u * UR + v * VR) - (BR) + y1) >> 6);
  *reinterpret_cast<uint32*>(rgb_buf) = (b << bshift) |
                                        (g << gshift) |
                                        (r << rshift) |
                                        (255u << ashift);
}

static __inline void YuvPixel2(uint8 y, uint8 u, uint8 v,
                               uint8* b, uint8* g, uint8* r) {
  int32 y1 = (static_cast<int32>(y) - 16) * YG;
  *b = Clip(static_cast<int32>((u * UB + v * VB) - (BB) + y1) >> 6);
  *g = Clip(static_cast<int32>((u * UG + v * VG) - (BG) + y1) >> 6);
  *r = Clip(static_cast<int32>((u * UR + v * VR) - (BR) + y1) >> 6);
}

#if !defined(LIBYUV_DISABLE_NEON) && \
    (defined(__ARM_NEON__) || defined(LIBYUV_NEON))
// C mimic assembly.
// TODO(fbarchard): Remove subsampling from Neon.
void I444ToARGBRow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width - 1; x += 2) {
    uint8 u = (src_u[0] + src_u[1] + 1) >> 1;
    uint8 v = (src_v[0] + src_v[1] + 1) >> 1;
    YuvPixel(src_y[0], u, v, rgb_buf + 0, 24, 16, 8, 0);
    YuvPixel(src_y[1], u, v, rgb_buf + 4, 24, 16, 8, 0);
    src_y += 2;
    src_u += 2;
    src_v += 2;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 24, 16, 8, 0);
  }
}
#else
void I444ToARGBRow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width; ++x) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf, 24, 16, 8, 0);
    src_y += 1;
    src_u += 1;
    src_v += 1;
    rgb_buf += 4;  // Advance 1 pixel.
  }
}
#endif
// Also used for 420
void I422ToARGBRow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 24, 16, 8, 0);
    YuvPixel(src_y[1], src_u[0], src_v[0], rgb_buf + 4, 24, 16, 8, 0);
    src_y += 2;
    src_u += 1;
    src_v += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 24, 16, 8, 0);
  }
}

void I422ToRGB24Row_C(const uint8* src_y,
                      const uint8* src_u,
                      const uint8* src_v,
                      uint8* rgb_buf,
                      int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel2(src_y[0], src_u[0], src_v[0],
              rgb_buf + 0, rgb_buf + 1, rgb_buf + 2);
    YuvPixel2(src_y[1], src_u[0], src_v[0],
              rgb_buf + 3, rgb_buf + 4, rgb_buf + 5);
    src_y += 2;
    src_u += 1;
    src_v += 1;
    rgb_buf += 6;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel2(src_y[0], src_u[0], src_v[0],
              rgb_buf + 0, rgb_buf + 1, rgb_buf + 2);
  }
}

void I422ToRAWRow_C(const uint8* src_y,
                    const uint8* src_u,
                    const uint8* src_v,
                    uint8* rgb_buf,
                    int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel2(src_y[0], src_u[0], src_v[0],
              rgb_buf + 2, rgb_buf + 1, rgb_buf + 0);
    YuvPixel2(src_y[1], src_u[0], src_v[0],
              rgb_buf + 5, rgb_buf + 4, rgb_buf + 3);
    src_y += 2;
    src_u += 1;
    src_v += 1;
    rgb_buf += 6;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel2(src_y[0], src_u[0], src_v[0],
              rgb_buf + 2, rgb_buf + 1, rgb_buf + 0);
  }
}

void I422ToARGB4444Row_C(const uint8* src_y,
                         const uint8* src_u,
                         const uint8* src_v,
                         uint8* dst_argb4444,
                         int width) {
  uint8 b0;
  uint8 g0;
  uint8 r0;
  uint8 b1;
  uint8 g1;
  uint8 r1;
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel2(src_y[0], src_u[0], src_v[0], &b0, &g0, &r0);
    YuvPixel2(src_y[1], src_u[0], src_v[0], &b1, &g1, &r1);
    b0 = b0 >> 4;
    g0 = g0 >> 4;
    r0 = r0 >> 4;
    b1 = b1 >> 4;
    g1 = g1 >> 4;
    r1 = r1 >> 4;
    *reinterpret_cast<uint32*>(dst_argb4444) = b0 | (g0 << 4) | (r0 << 8) |
        (b1 << 16) | (g1 << 20) | (r1 << 24) | 0xf000f000;
    src_y += 2;
    src_u += 1;
    src_v += 1;
    dst_argb4444 += 4;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel2(src_y[0], src_u[0], src_v[0], &b0, &g0, &r0);
    b0 = b0 >> 4;
    g0 = g0 >> 4;
    r0 = r0 >> 4;
    *reinterpret_cast<uint16*>(dst_argb4444) = b0 | (g0 << 4) | (r0 << 8) |
        0xf000;
  }
}

void I422ToARGB1555Row_C(const uint8* src_y,
                         const uint8* src_u,
                         const uint8* src_v,
                         uint8* dst_argb1555,
                         int width) {
  uint8 b0;
  uint8 g0;
  uint8 r0;
  uint8 b1;
  uint8 g1;
  uint8 r1;
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel2(src_y[0], src_u[0], src_v[0], &b0, &g0, &r0);
    YuvPixel2(src_y[1], src_u[0], src_v[0], &b1, &g1, &r1);
    b0 = b0 >> 3;
    g0 = g0 >> 3;
    r0 = r0 >> 3;
    b1 = b1 >> 3;
    g1 = g1 >> 3;
    r1 = r1 >> 3;
    *reinterpret_cast<uint32*>(dst_argb1555) = b0 | (g0 << 5) | (r0 << 10) |
        (b1 << 16) | (g1 << 21) | (r1 << 26) | 0x80008000;
    src_y += 2;
    src_u += 1;
    src_v += 1;
    dst_argb1555 += 4;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel2(src_y[0], src_u[0], src_v[0], &b0, &g0, &r0);
    b0 = b0 >> 3;
    g0 = g0 >> 3;
    r0 = r0 >> 3;
    *reinterpret_cast<uint16*>(dst_argb1555) = b0 | (g0 << 5) | (r0 << 10) |
        0x8000;
  }
}

void I422ToRGB565Row_C(const uint8* src_y,
                       const uint8* src_u,
                       const uint8* src_v,
                       uint8* dst_rgb565,
                       int width) {
  uint8 b0;
  uint8 g0;
  uint8 r0;
  uint8 b1;
  uint8 g1;
  uint8 r1;
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel2(src_y[0], src_u[0], src_v[0], &b0, &g0, &r0);
    YuvPixel2(src_y[1], src_u[0], src_v[0], &b1, &g1, &r1);
    b0 = b0 >> 3;
    g0 = g0 >> 2;
    r0 = r0 >> 3;
    b1 = b1 >> 3;
    g1 = g1 >> 2;
    r1 = r1 >> 3;
    *reinterpret_cast<uint32*>(dst_rgb565) = b0 | (g0 << 5) | (r0 << 11) |
        (b1 << 16) | (g1 << 21) | (r1 << 27);
    src_y += 2;
    src_u += 1;
    src_v += 1;
    dst_rgb565 += 4;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel2(src_y[0], src_u[0], src_v[0], &b0, &g0, &r0);
    b0 = b0 >> 3;
    g0 = g0 >> 2;
    r0 = r0 >> 3;
    *reinterpret_cast<uint16*>(dst_rgb565) = b0 | (g0 << 5) | (r0 << 11);
  }
}

void I411ToARGBRow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width - 3; x += 4) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 24, 16, 8, 0);
    YuvPixel(src_y[1], src_u[0], src_v[0], rgb_buf + 4, 24, 16, 8, 0);
    YuvPixel(src_y[2], src_u[0], src_v[0], rgb_buf + 8, 24, 16, 8, 0);
    YuvPixel(src_y[3], src_u[0], src_v[0], rgb_buf + 12, 24, 16, 8, 0);
    src_y += 4;
    src_u += 1;
    src_v += 1;
    rgb_buf += 16;  // Advance 4 pixels.
  }
  if (width & 2) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 24, 16, 8, 0);
    YuvPixel(src_y[1], src_u[0], src_v[0], rgb_buf + 4, 24, 16, 8, 0);
    src_y += 2;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 24, 16, 8, 0);
  }
}

void NV12ToARGBRow_C(const uint8* src_y,
                     const uint8* usrc_v,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(src_y[0], usrc_v[0], usrc_v[1], rgb_buf + 0, 24, 16, 8, 0);
    YuvPixel(src_y[1], usrc_v[0], usrc_v[1], rgb_buf + 4, 24, 16, 8, 0);
    src_y += 2;
    usrc_v += 2;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], usrc_v[0], usrc_v[1], rgb_buf + 0, 24, 16, 8, 0);
  }
}

void NV21ToARGBRow_C(const uint8* src_y,
                     const uint8* src_vu,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(src_y[0], src_vu[1], src_vu[0], rgb_buf + 0, 24, 16, 8, 0);
    YuvPixel(src_y[1], src_vu[1], src_vu[0], rgb_buf + 4, 24, 16, 8, 0);
    src_y += 2;
    src_vu += 2;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], src_vu[1], src_vu[0], rgb_buf + 0, 24, 16, 8, 0);
  }
}

void NV12ToRGB565Row_C(const uint8* src_y,
                       const uint8* usrc_v,
                       uint8* dst_rgb565,
                       int width) {
  uint8 b0;
  uint8 g0;
  uint8 r0;
  uint8 b1;
  uint8 g1;
  uint8 r1;
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel2(src_y[0], usrc_v[0], usrc_v[1], &b0, &g0, &r0);
    YuvPixel2(src_y[1], usrc_v[0], usrc_v[1], &b1, &g1, &r1);
    b0 = b0 >> 3;
    g0 = g0 >> 2;
    r0 = r0 >> 3;
    b1 = b1 >> 3;
    g1 = g1 >> 2;
    r1 = r1 >> 3;
    *reinterpret_cast<uint32*>(dst_rgb565) = b0 | (g0 << 5) | (r0 << 11) |
        (b1 << 16) | (g1 << 21) | (r1 << 27);
    src_y += 2;
    usrc_v += 2;
    dst_rgb565 += 4;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel2(src_y[0], usrc_v[0], usrc_v[1], &b0, &g0, &r0);
    b0 = b0 >> 3;
    g0 = g0 >> 2;
    r0 = r0 >> 3;
    *reinterpret_cast<uint16*>(dst_rgb565) = b0 | (g0 << 5) | (r0 << 11);
  }
}

void NV21ToRGB565Row_C(const uint8* src_y,
                       const uint8* vsrc_u,
                       uint8* dst_rgb565,
                       int width) {
  uint8 b0;
  uint8 g0;
  uint8 r0;
  uint8 b1;
  uint8 g1;
  uint8 r1;
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel2(src_y[0], vsrc_u[1], vsrc_u[0], &b0, &g0, &r0);
    YuvPixel2(src_y[1], vsrc_u[1], vsrc_u[0], &b1, &g1, &r1);
    b0 = b0 >> 3;
    g0 = g0 >> 2;
    r0 = r0 >> 3;
    b1 = b1 >> 3;
    g1 = g1 >> 2;
    r1 = r1 >> 3;
    *reinterpret_cast<uint32*>(dst_rgb565) = b0 | (g0 << 5) | (r0 << 11) |
        (b1 << 16) | (g1 << 21) | (r1 << 27);
    src_y += 2;
    vsrc_u += 2;
    dst_rgb565 += 4;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel2(src_y[0], vsrc_u[1], vsrc_u[0], &b0, &g0, &r0);
    b0 = b0 >> 3;
    g0 = g0 >> 2;
    r0 = r0 >> 3;
    *reinterpret_cast<uint16*>(dst_rgb565) = b0 | (g0 << 5) | (r0 << 11);
  }
}

void YUY2ToARGBRow_C(const uint8* src_yuy2,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(src_yuy2[0], src_yuy2[1], src_yuy2[3], rgb_buf + 0, 24, 16, 8, 0);
    YuvPixel(src_yuy2[2], src_yuy2[1], src_yuy2[3], rgb_buf + 4, 24, 16, 8, 0);
    src_yuy2 += 4;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_yuy2[0], src_yuy2[1], src_yuy2[3], rgb_buf + 0, 24, 16, 8, 0);
  }
}

void UYVYToARGBRow_C(const uint8* src_uyvy,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(src_uyvy[1], src_uyvy[0], src_uyvy[2], rgb_buf + 0, 24, 16, 8, 0);
    YuvPixel(src_uyvy[3], src_uyvy[0], src_uyvy[2], rgb_buf + 4, 24, 16, 8, 0);
    src_uyvy += 4;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_uyvy[1], src_uyvy[0], src_uyvy[2], rgb_buf + 0, 24, 16, 8, 0);
  }
}

void I422ToBGRARow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 0, 8, 16, 24);
    YuvPixel(src_y[1], src_u[0], src_v[0], rgb_buf + 4, 0, 8, 16, 24);
    src_y += 2;
    src_u += 1;
    src_v += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf, 0, 8, 16, 24);
  }
}

void I422ToABGRRow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 24, 0, 8, 16);
    YuvPixel(src_y[1], src_u[0], src_v[0], rgb_buf + 4, 24, 0, 8, 16);
    src_y += 2;
    src_u += 1;
    src_v += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 24, 0, 8, 16);
  }
}

void I422ToRGBARow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* rgb_buf,
                     int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 0, 24, 16, 8);
    YuvPixel(src_y[1], src_u[0], src_v[0], rgb_buf + 4, 0, 24, 16, 8);
    src_y += 2;
    src_u += 1;
    src_v += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], src_u[0], src_v[0], rgb_buf + 0, 0, 24, 16, 8);
  }
}

void YToARGBRow_C(const uint8* src_y, uint8* rgb_buf, int width) {
  for (int x = 0; x < width; ++x) {
    YuvPixel(src_y[0], 128, 128, rgb_buf, 24, 16, 8, 0);
    src_y += 1;
    rgb_buf += 4;  // Advance 1 pixel.
  }
}

void MirrorRow_C(const uint8* src, uint8* dst, int width) {
  src += width - 1;
  for (int x = 0; x < width - 1; x += 2) {
    dst[x] = src[0];
    dst[x + 1] = src[-1];
    src -= 2;
  }
  if (width & 1) {
    dst[width - 1] = src[0];
  }
}

void MirrorUVRow_C(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int width) {
  src_uv += (width - 1) << 1;
  for (int x = 0; x < width - 1; x += 2) {
    dst_u[x] = src_uv[0];
    dst_u[x + 1] = src_uv[-2];
    dst_v[x] = src_uv[1];
    dst_v[x + 1] = src_uv[-2 + 1];
    src_uv -= 4;
  }
  if (width & 1) {
    dst_u[width - 1] = src_uv[0];
    dst_v[width - 1] = src_uv[1];
  }
}

void ARGBMirrorRow_C(const uint8* src, uint8* dst, int width) {
  const uint32* src32 = reinterpret_cast<const uint32*>(src);
  uint32* dst32 = reinterpret_cast<uint32*>(dst);
  src32 += width - 1;
  for (int x = 0; x < width - 1; x += 2) {
    dst32[x] = src32[0];
    dst32[x + 1] = src32[-1];
    src32 -= 2;
  }
  if (width & 1) {
    dst32[width - 1] = src32[0];
  }
}

void SplitUVRow_C(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int width) {
  for (int x = 0; x < width - 1; x += 2) {
    dst_u[x] = src_uv[0];
    dst_u[x + 1] = src_uv[2];
    dst_v[x] = src_uv[1];
    dst_v[x + 1] = src_uv[3];
    src_uv += 4;
  }
  if (width & 1) {
    dst_u[width - 1] = src_uv[0];
    dst_v[width - 1] = src_uv[1];
  }
}

void MergeUVRow_C(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
                  int width) {
  for (int x = 0; x < width - 1; x += 2) {
    dst_uv[0] = src_u[x];
    dst_uv[1] = src_v[x];
    dst_uv[2] = src_u[x + 1];
    dst_uv[3] = src_v[x + 1];
    dst_uv += 4;
  }
  if (width & 1) {
    dst_uv[0] = src_u[width - 1];
    dst_uv[1] = src_v[width - 1];
  }
}

void CopyRow_C(const uint8* src, uint8* dst, int count) {
  memcpy(dst, src, count);
}

void SetRow_C(uint8* dst, uint32 v8, int count) {
#ifdef _MSC_VER
  // VC will generate rep stosb.
  for (int x = 0; x < count; ++x) {
    dst[x] = v8;
  }
#else
  memset(dst, v8, count);
#endif
}

void ARGBSetRows_C(uint8* dst, uint32 v32, int width,
                 int dst_stride, int height) {
  for (int y = 0; y < height; ++y) {
    uint32* d = reinterpret_cast<uint32*>(dst);
    for (int x = 0; x < width; ++x) {
      d[x] = v32;
    }
    dst += dst_stride;
  }
}

// Filter 2 rows of YUY2 UV's (422) into U and V (420).
void YUY2ToUVRow_C(const uint8* src_yuy2, int src_stride_yuy2,
                   uint8* dst_u, uint8* dst_v, int width) {
  // Output a row of UV values, filtering 2 rows of YUY2.
  for (int x = 0; x < width; x += 2) {
    dst_u[0] = (src_yuy2[1] + src_yuy2[src_stride_yuy2 + 1] + 1) >> 1;
    dst_v[0] = (src_yuy2[3] + src_yuy2[src_stride_yuy2 + 3] + 1) >> 1;
    src_yuy2 += 4;
    dst_u += 1;
    dst_v += 1;
  }
}

// Copy row of YUY2 UV's (422) into U and V (422).
void YUY2ToUV422Row_C(const uint8* src_yuy2,
                      uint8* dst_u, uint8* dst_v, int width) {
  // Output a row of UV values.
  for (int x = 0; x < width; x += 2) {
    dst_u[0] = src_yuy2[1];
    dst_v[0] = src_yuy2[3];
    src_yuy2 += 4;
    dst_u += 1;
    dst_v += 1;
  }
}

// Copy row of YUY2 Y's (422) into Y (420/422).
void YUY2ToYRow_C(const uint8* src_yuy2, uint8* dst_y, int width) {
  // Output a row of Y values.
  for (int x = 0; x < width - 1; x += 2) {
    dst_y[x] = src_yuy2[0];
    dst_y[x + 1] = src_yuy2[2];
    src_yuy2 += 4;
  }
  if (width & 1) {
    dst_y[width - 1] = src_yuy2[0];
  }
}

// Filter 2 rows of UYVY UV's (422) into U and V (420).
void UYVYToUVRow_C(const uint8* src_uyvy, int src_stride_uyvy,
                   uint8* dst_u, uint8* dst_v, int width) {
  // Output a row of UV values.
  for (int x = 0; x < width; x += 2) {
    dst_u[0] = (src_uyvy[0] + src_uyvy[src_stride_uyvy + 0] + 1) >> 1;
    dst_v[0] = (src_uyvy[2] + src_uyvy[src_stride_uyvy + 2] + 1) >> 1;
    src_uyvy += 4;
    dst_u += 1;
    dst_v += 1;
  }
}

// Copy row of UYVY UV's (422) into U and V (422).
void UYVYToUV422Row_C(const uint8* src_uyvy,
                      uint8* dst_u, uint8* dst_v, int width) {
  // Output a row of UV values.
  for (int x = 0; x < width; x += 2) {
    dst_u[0] = src_uyvy[0];
    dst_v[0] = src_uyvy[2];
    src_uyvy += 4;
    dst_u += 1;
    dst_v += 1;
  }
}

// Copy row of UYVY Y's (422) into Y (420/422).
void UYVYToYRow_C(const uint8* src_uyvy, uint8* dst_y, int width) {
  // Output a row of Y values.
  for (int x = 0; x < width - 1; x += 2) {
    dst_y[x] = src_uyvy[1];
    dst_y[x + 1] = src_uyvy[3];
    src_uyvy += 4;
  }
  if (width & 1) {
    dst_y[width - 1] = src_uyvy[1];
  }
}

#define BLEND(f, b, a) (((256 - a) * b) >> 8) + f

// Blend src_argb0 over src_argb1 and store to dst_argb.
// dst_argb may be src_argb0 or src_argb1.
// This code mimics the SSSE3 version for better testability.
void ARGBBlendRow_C(const uint8* src_argb0, const uint8* src_argb1,
                    uint8* dst_argb, int width) {
  for (int x = 0; x < width - 1; x += 2) {
    uint32 fb = src_argb0[0];
    uint32 fg = src_argb0[1];
    uint32 fr = src_argb0[2];
    uint32 a = src_argb0[3];
    uint32 bb = src_argb1[0];
    uint32 bg = src_argb1[1];
    uint32 br = src_argb1[2];
    dst_argb[0] = BLEND(fb, bb, a);
    dst_argb[1] = BLEND(fg, bg, a);
    dst_argb[2] = BLEND(fr, br, a);
    dst_argb[3] = 255u;

    fb = src_argb0[4 + 0];
    fg = src_argb0[4 + 1];
    fr = src_argb0[4 + 2];
    a = src_argb0[4 + 3];
    bb = src_argb1[4 + 0];
    bg = src_argb1[4 + 1];
    br = src_argb1[4 + 2];
    dst_argb[4 + 0] = BLEND(fb, bb, a);
    dst_argb[4 + 1] = BLEND(fg, bg, a);
    dst_argb[4 + 2] = BLEND(fr, br, a);
    dst_argb[4 + 3] = 255u;
    src_argb0 += 8;
    src_argb1 += 8;
    dst_argb += 8;
  }

  if (width & 1) {
    uint32 fb = src_argb0[0];
    uint32 fg = src_argb0[1];
    uint32 fr = src_argb0[2];
    uint32 a = src_argb0[3];
    uint32 bb = src_argb1[0];
    uint32 bg = src_argb1[1];
    uint32 br = src_argb1[2];
    dst_argb[0] = BLEND(fb, bb, a);
    dst_argb[1] = BLEND(fg, bg, a);
    dst_argb[2] = BLEND(fr, br, a);
    dst_argb[3] = 255u;
  }
}
#undef BLEND
#define ATTENUATE(f, a) (a | (a << 8)) * (f | (f << 8)) >> 24

// Multiply source RGB by alpha and store to destination.
// This code mimics the SSSE3 version for better testability.
void ARGBAttenuateRow_C(const uint8* src_argb, uint8* dst_argb, int width) {
  for (int i = 0; i < width - 1; i += 2) {
    uint32 b = src_argb[0];
    uint32 g = src_argb[1];
    uint32 r = src_argb[2];
    uint32 a = src_argb[3];
    dst_argb[0] = ATTENUATE(b, a);
    dst_argb[1] = ATTENUATE(g, a);
    dst_argb[2] = ATTENUATE(r, a);
    dst_argb[3] = a;
    b = src_argb[4];
    g = src_argb[5];
    r = src_argb[6];
    a = src_argb[7];
    dst_argb[4] = ATTENUATE(b, a);
    dst_argb[5] = ATTENUATE(g, a);
    dst_argb[6] = ATTENUATE(r, a);
    dst_argb[7] = a;
    src_argb += 8;
    dst_argb += 8;
  }

  if (width & 1) {
    const uint32 b = src_argb[0];
    const uint32 g = src_argb[1];
    const uint32 r = src_argb[2];
    const uint32 a = src_argb[3];
    dst_argb[0] = ATTENUATE(b, a);
    dst_argb[1] = ATTENUATE(g, a);
    dst_argb[2] = ATTENUATE(r, a);
    dst_argb[3] = a;
  }
}
#undef ATTENUATE

// Divide source RGB by alpha and store to destination.
// b = (b * 255 + (a / 2)) / a;
// g = (g * 255 + (a / 2)) / a;
// r = (r * 255 + (a / 2)) / a;
// Reciprocal method is off by 1 on some values. ie 125
// 8.8 fixed point inverse table with 1.0 in upper short and 1 / a in lower.
#define T(a) 0x01000000 + (0x10000 / a)
uint32 fixed_invtbl8[256] = {
  0x01000000, 0x0100ffff, T(0x02), T(0x03), T(0x04), T(0x05), T(0x06), T(0x07),
  T(0x08), T(0x09), T(0x0a), T(0x0b), T(0x0c), T(0x0d), T(0x0e), T(0x0f),
  T(0x10), T(0x11), T(0x12), T(0x13), T(0x14), T(0x15), T(0x16), T(0x17),
  T(0x18), T(0x19), T(0x1a), T(0x1b), T(0x1c), T(0x1d), T(0x1e), T(0x1f),
  T(0x20), T(0x21), T(0x22), T(0x23), T(0x24), T(0x25), T(0x26), T(0x27),
  T(0x28), T(0x29), T(0x2a), T(0x2b), T(0x2c), T(0x2d), T(0x2e), T(0x2f),
  T(0x30), T(0x31), T(0x32), T(0x33), T(0x34), T(0x35), T(0x36), T(0x37),
  T(0x38), T(0x39), T(0x3a), T(0x3b), T(0x3c), T(0x3d), T(0x3e), T(0x3f),
  T(0x40), T(0x41), T(0x42), T(0x43), T(0x44), T(0x45), T(0x46), T(0x47),
  T(0x48), T(0x49), T(0x4a), T(0x4b), T(0x4c), T(0x4d), T(0x4e), T(0x4f),
  T(0x50), T(0x51), T(0x52), T(0x53), T(0x54), T(0x55), T(0x56), T(0x57),
  T(0x58), T(0x59), T(0x5a), T(0x5b), T(0x5c), T(0x5d), T(0x5e), T(0x5f),
  T(0x60), T(0x61), T(0x62), T(0x63), T(0x64), T(0x65), T(0x66), T(0x67),
  T(0x68), T(0x69), T(0x6a), T(0x6b), T(0x6c), T(0x6d), T(0x6e), T(0x6f),
  T(0x70), T(0x71), T(0x72), T(0x73), T(0x74), T(0x75), T(0x76), T(0x77),
  T(0x78), T(0x79), T(0x7a), T(0x7b), T(0x7c), T(0x7d), T(0x7e), T(0x7f),
  T(0x80), T(0x81), T(0x82), T(0x83), T(0x84), T(0x85), T(0x86), T(0x87),
  T(0x88), T(0x89), T(0x8a), T(0x8b), T(0x8c), T(0x8d), T(0x8e), T(0x8f),
  T(0x90), T(0x91), T(0x92), T(0x93), T(0x94), T(0x95), T(0x96), T(0x97),
  T(0x98), T(0x99), T(0x9a), T(0x9b), T(0x9c), T(0x9d), T(0x9e), T(0x9f),
  T(0xa0), T(0xa1), T(0xa2), T(0xa3), T(0xa4), T(0xa5), T(0xa6), T(0xa7),
  T(0xa8), T(0xa9), T(0xaa), T(0xab), T(0xac), T(0xad), T(0xae), T(0xaf),
  T(0xb0), T(0xb1), T(0xb2), T(0xb3), T(0xb4), T(0xb5), T(0xb6), T(0xb7),
  T(0xb8), T(0xb9), T(0xba), T(0xbb), T(0xbc), T(0xbd), T(0xbe), T(0xbf),
  T(0xc0), T(0xc1), T(0xc2), T(0xc3), T(0xc4), T(0xc5), T(0xc6), T(0xc7),
  T(0xc8), T(0xc9), T(0xca), T(0xcb), T(0xcc), T(0xcd), T(0xce), T(0xcf),
  T(0xd0), T(0xd1), T(0xd2), T(0xd3), T(0xd4), T(0xd5), T(0xd6), T(0xd7),
  T(0xd8), T(0xd9), T(0xda), T(0xdb), T(0xdc), T(0xdd), T(0xde), T(0xdf),
  T(0xe0), T(0xe1), T(0xe2), T(0xe3), T(0xe4), T(0xe5), T(0xe6), T(0xe7),
  T(0xe8), T(0xe9), T(0xea), T(0xeb), T(0xec), T(0xed), T(0xee), T(0xef),
  T(0xf0), T(0xf1), T(0xf2), T(0xf3), T(0xf4), T(0xf5), T(0xf6), T(0xf7),
  T(0xf8), T(0xf9), T(0xfa), T(0xfb), T(0xfc), T(0xfd), T(0xfe), 0x01000100 };
#undef T

void ARGBUnattenuateRow_C(const uint8* src_argb, uint8* dst_argb, int width) {
  for (int i = 0; i < width; ++i) {
    uint32 b = src_argb[0];
    uint32 g = src_argb[1];
    uint32 r = src_argb[2];
    const uint32 a = src_argb[3];
    const uint32 ia = fixed_invtbl8[a] & 0xffff;  // 8.16 fixed point
    b = (b * ia) >> 8;
    g = (g * ia) >> 8;
    r = (r * ia) >> 8;
    // Clamping should not be necessary but is free in assembly.
    if (b > 255) {
      b = 255;
    }
    if (g > 255) {
      g = 255;
    }
    if (r > 255) {
      r = 255;
    }
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = a;
    src_argb += 4;
    dst_argb += 4;
  }
}

void ComputeCumulativeSumRow_C(const uint8* row, int32* cumsum,
                               const int32* previous_cumsum, int width) {
  int32 row_sum[4] = {0, 0, 0, 0};
  for (int x = 0; x < width; ++x) {
    row_sum[0] += row[x * 4 + 0];
    row_sum[1] += row[x * 4 + 1];
    row_sum[2] += row[x * 4 + 2];
    row_sum[3] += row[x * 4 + 3];
    cumsum[x * 4 + 0] = row_sum[0]  + previous_cumsum[x * 4 + 0];
    cumsum[x * 4 + 1] = row_sum[1]  + previous_cumsum[x * 4 + 1];
    cumsum[x * 4 + 2] = row_sum[2]  + previous_cumsum[x * 4 + 2];
    cumsum[x * 4 + 3] = row_sum[3]  + previous_cumsum[x * 4 + 3];
  }
}

void CumulativeSumToAverageRow_C(const int32* tl, const int32* bl,
                                int w, int area, uint8* dst, int count) {
  float ooa = 1.0f / area;
  for (int i = 0; i < count; ++i) {
    dst[0] = static_cast<uint8>((bl[w + 0] + tl[0] - bl[0] - tl[w + 0]) * ooa);
    dst[1] = static_cast<uint8>((bl[w + 1] + tl[1] - bl[1] - tl[w + 1]) * ooa);
    dst[2] = static_cast<uint8>((bl[w + 2] + tl[2] - bl[2] - tl[w + 2]) * ooa);
    dst[3] = static_cast<uint8>((bl[w + 3] + tl[3] - bl[3] - tl[w + 3]) * ooa);
    dst += 4;
    tl += 4;
    bl += 4;
  }
}

// Copy pixels from rotated source to destination row with a slope.
LIBYUV_API
void ARGBAffineRow_C(const uint8* src_argb, int src_argb_stride,
                     uint8* dst_argb, const float* uv_dudv, int width) {
  // Render a row of pixels from source into a buffer.
  float uv[2];
  uv[0] = uv_dudv[0];
  uv[1] = uv_dudv[1];
  for (int i = 0; i < width; ++i) {
    int x = static_cast<int>(uv[0]);
    int y = static_cast<int>(uv[1]);
    *reinterpret_cast<uint32*>(dst_argb) =
        *reinterpret_cast<const uint32*>(src_argb + y * src_argb_stride +
                                         x * 4);
    dst_argb += 4;
    uv[0] += uv_dudv[2];
    uv[1] += uv_dudv[3];
  }
}

// C version 2x2 -> 2x1.
void ARGBInterpolateRow_C(uint8* dst_ptr, const uint8* src_ptr,
                          ptrdiff_t src_stride,
                          int dst_width, int source_y_fraction) {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint8* src_ptr1 = src_ptr + src_stride;
  uint8* end = dst_ptr + (dst_width << 2);
  do {
    dst_ptr[0] = (src_ptr[0] * y0_fraction + src_ptr1[0] * y1_fraction) >> 8;
    dst_ptr[1] = (src_ptr[1] * y0_fraction + src_ptr1[1] * y1_fraction) >> 8;
    dst_ptr[2] = (src_ptr[2] * y0_fraction + src_ptr1[2] * y1_fraction) >> 8;
    dst_ptr[3] = (src_ptr[3] * y0_fraction + src_ptr1[3] * y1_fraction) >> 8;
    dst_ptr[4] = (src_ptr[4] * y0_fraction + src_ptr1[4] * y1_fraction) >> 8;
    dst_ptr[5] = (src_ptr[5] * y0_fraction + src_ptr1[5] * y1_fraction) >> 8;
    dst_ptr[6] = (src_ptr[6] * y0_fraction + src_ptr1[6] * y1_fraction) >> 8;
    dst_ptr[7] = (src_ptr[7] * y0_fraction + src_ptr1[7] * y1_fraction) >> 8;
    src_ptr += 8;
    src_ptr1 += 8;
    dst_ptr += 8;
  } while (dst_ptr < end);
}

// Blend 2 rows into 1 for conversions such as I422ToI420.
void HalfRow_C(const uint8* src_uv, int src_uv_stride,
               uint8* dst_uv, int pix) {
  for (int x = 0; x < pix; ++x) {
    dst_uv[x] = (src_uv[x] + src_uv[src_uv_stride + x] + 1) >> 1;
  }
}

// Select 2 channels from ARGB on alternating pixels.  e.g.  BGBGBGBG
void ARGBToBayerRow_C(const uint8* src_argb,
                      uint8* dst_bayer, uint32 selector, int pix) {
  int index0 = selector & 0xff;
  int index1 = (selector >> 8) & 0xff;
  // Copy a row of Bayer.
  for (int x = 0; x < pix - 1; x += 2) {
    dst_bayer[0] = src_argb[index0];
    dst_bayer[1] = src_argb[index1];
    src_argb += 8;
    dst_bayer += 2;
  }
  if (pix & 1) {
    dst_bayer[0] = src_argb[index0];
  }
}

void I422ToYUY2Row_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* dst_frame, int width) {
    for (int x = 0; x < width - 1; x += 2) {
      dst_frame[0] = src_y[0];
      dst_frame[1] = src_u[0];
      dst_frame[2] = src_y[1];
      dst_frame[3] = src_v[0];
      dst_frame += 4;
      src_y += 2;
      src_u += 1;
      src_v += 1;
    }
    if (width & 1) {
      dst_frame[0] = src_y[0];
      dst_frame[1] = src_u[0];
      dst_frame[2] = src_y[0];  // duplicate last y
      dst_frame[3] = src_v[0];
    }
}

void I422ToUYVYRow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* dst_frame, int width) {
    for (int x = 0; x < width - 1; x += 2) {
      dst_frame[0] = src_u[0];
      dst_frame[1] = src_y[0];
      dst_frame[2] = src_v[0];
      dst_frame[3] = src_y[1];
      dst_frame += 4;
      src_y += 2;
      src_u += 1;
      src_v += 1;
    }
    if (width & 1) {
      dst_frame[0] = src_u[0];
      dst_frame[1] = src_y[0];
      dst_frame[2] = src_v[0];
      dst_frame[3] = src_y[0];  // duplicate last y
    }
}

#if !defined(LIBYUV_DISABLE_X86)
// row_win.cc has asm version, but GCC uses 2 step wrapper.  5% slower.
// TODO(fbarchard): Handle width > kMaxStride here instead of calling code.
#if defined(__x86_64__) || defined(__i386__)
void I422ToRGB565Row_SSSE3(const uint8* src_y,
                           const uint8* src_u,
                           const uint8* src_v,
                           uint8* rgb_buf,
                           int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  I422ToARGBRow_SSSE3(src_y, src_u, src_v, row, width);
  ARGBToRGB565Row_SSE2(row, rgb_buf, width);
}
#endif  // defined(__x86_64__) || defined(__i386__)

#if defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
void I422ToARGB1555Row_SSSE3(const uint8* src_y,
                             const uint8* src_u,
                             const uint8* src_v,
                             uint8* rgb_buf,
                             int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  I422ToARGBRow_SSSE3(src_y, src_u, src_v, row, width);
  ARGBToARGB1555Row_SSE2(row, rgb_buf, width);
}

void I422ToARGB4444Row_SSSE3(const uint8* src_y,
                             const uint8* src_u,
                             const uint8* src_v,
                             uint8* rgb_buf,
                             int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  I422ToARGBRow_SSSE3(src_y, src_u, src_v, row, width);
  ARGBToARGB4444Row_SSE2(row, rgb_buf, width);
}

void NV12ToRGB565Row_SSSE3(const uint8* src_y,
                           const uint8* src_uv,
                           uint8* dst_rgb565,
                           int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  NV12ToARGBRow_SSSE3(src_y, src_uv, row, width);
  ARGBToRGB565Row_SSE2(row, dst_rgb565, width);
}

void NV21ToRGB565Row_SSSE3(const uint8* src_y,
                           const uint8* src_vu,
                           uint8* dst_rgb565,
                           int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  NV21ToARGBRow_SSSE3(src_y, src_vu, row, width);
  ARGBToRGB565Row_SSE2(row, dst_rgb565, width);
}

void YUY2ToARGBRow_SSSE3(const uint8* src_yuy2,
                         uint8* dst_argb,
                         int width) {
  SIMD_ALIGNED(uint8 row_y[kMaxStride]);
  SIMD_ALIGNED(uint8 row_u[kMaxStride / 2]);
  SIMD_ALIGNED(uint8 row_v[kMaxStride / 2]);
  YUY2ToUV422Row_SSE2(src_yuy2, row_u, row_v, width);
  YUY2ToYRow_SSE2(src_yuy2, row_y, width);
  I422ToARGBRow_SSSE3(row_y, row_u, row_v, dst_argb, width);
}

void YUY2ToARGBRow_Unaligned_SSSE3(const uint8* src_yuy2,
                                   uint8* dst_argb,
                                   int width) {
  SIMD_ALIGNED(uint8 row_y[kMaxStride]);
  SIMD_ALIGNED(uint8 row_u[kMaxStride / 2]);
  SIMD_ALIGNED(uint8 row_v[kMaxStride / 2]);
  YUY2ToUV422Row_Unaligned_SSE2(src_yuy2, row_u, row_v, width);
  YUY2ToYRow_Unaligned_SSE2(src_yuy2, row_y, width);
  I422ToARGBRow_Unaligned_SSSE3(row_y, row_u, row_v, dst_argb, width);
}

void UYVYToARGBRow_SSSE3(const uint8* src_uyvy,
                         uint8* dst_argb,
                         int width) {
  SIMD_ALIGNED(uint8 row_y[kMaxStride]);
  SIMD_ALIGNED(uint8 row_u[kMaxStride / 2]);
  SIMD_ALIGNED(uint8 row_v[kMaxStride / 2]);
  UYVYToUV422Row_SSE2(src_uyvy, row_u, row_v, width);
  UYVYToYRow_SSE2(src_uyvy, row_y, width);
  I422ToARGBRow_SSSE3(row_y, row_u, row_v, dst_argb, width);
}

void UYVYToARGBRow_Unaligned_SSSE3(const uint8* src_uyvy,
                                   uint8* dst_argb,
                                   int width) {
  SIMD_ALIGNED(uint8 row_y[kMaxStride]);
  SIMD_ALIGNED(uint8 row_u[kMaxStride / 2]);
  SIMD_ALIGNED(uint8 row_v[kMaxStride / 2]);
  UYVYToUV422Row_Unaligned_SSE2(src_uyvy, row_u, row_v, width);
  UYVYToYRow_Unaligned_SSE2(src_uyvy, row_y, width);
  I422ToARGBRow_Unaligned_SSSE3(row_y, row_u, row_v, dst_argb, width);
}

#endif  // defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
#endif  // !defined(LIBYUV_DISABLE_X86)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif
