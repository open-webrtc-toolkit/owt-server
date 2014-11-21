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
#include "vp9/common/vp9_sadmxn.h"
#include "./vpx_config.h"
#include "vpx/vpx_integer.h"
#include "./vp9_rtcd.h"

unsigned int vp9_sad64x64_c(const uint8_t *src_ptr,
                            int  src_stride,
                            const uint8_t *ref_ptr,
                            int  ref_stride,
                            unsigned int max_sad) {
  return sad_mx_n_c(src_ptr, src_stride, ref_ptr, ref_stride, 64, 64);
}

unsigned int vp9_sad32x32_c(const uint8_t *src_ptr,
                            int  src_stride,
                            const uint8_t *ref_ptr,
                            int  ref_stride,
                            unsigned int max_sad) {
  return sad_mx_n_c(src_ptr, src_stride, ref_ptr, ref_stride, 32, 32);
}

unsigned int vp9_sad16x16_c(const uint8_t *src_ptr,
                            int  src_stride,
                            const uint8_t *ref_ptr,
                            int  ref_stride,
                            unsigned int max_sad) {
  return sad_mx_n_c(src_ptr, src_stride, ref_ptr, ref_stride, 16, 16);
}

unsigned int vp9_sad8x8_c(const uint8_t *src_ptr,
                          int  src_stride,
                          const uint8_t *ref_ptr,
                          int  ref_stride,
                          unsigned int max_sad) {
  return sad_mx_n_c(src_ptr, src_stride, ref_ptr, ref_stride, 8, 8);
}


unsigned int vp9_sad16x8_c(const uint8_t *src_ptr,
                           int  src_stride,
                           const uint8_t *ref_ptr,
                           int  ref_stride,
                           unsigned int max_sad) {
  return sad_mx_n_c(src_ptr, src_stride, ref_ptr, ref_stride, 16, 8);
}

unsigned int vp9_sad8x16_c(const uint8_t *src_ptr,
                           int  src_stride,
                           const uint8_t *ref_ptr,
                           int  ref_stride,
                           unsigned int max_sad) {
  return sad_mx_n_c(src_ptr, src_stride, ref_ptr, ref_stride, 8, 16);
}


unsigned int vp9_sad4x4_c(const uint8_t *src_ptr,
                          int  src_stride,
                          const uint8_t *ref_ptr,
                          int  ref_stride,
                          unsigned int max_sad) {
  return sad_mx_n_c(src_ptr, src_stride, ref_ptr, ref_stride, 4, 4);
}

void vp9_sad64x64x3_c(const uint8_t *src_ptr,
                      int  src_stride,
                      const uint8_t *ref_ptr,
                      int  ref_stride,
                      unsigned int *sad_array) {
  sad_array[0] = vp9_sad64x64(src_ptr, src_stride, ref_ptr, ref_stride,
                              0x7fffffff);
  sad_array[1] = vp9_sad64x64(src_ptr, src_stride, ref_ptr + 1, ref_stride,
                              0x7fffffff);
  sad_array[2] = vp9_sad64x64(src_ptr, src_stride, ref_ptr + 2, ref_stride,
                              0x7fffffff);
}

void vp9_sad32x32x3_c(const uint8_t *src_ptr,
                      int  src_stride,
                      const uint8_t *ref_ptr,
                      int  ref_stride,
                      unsigned int *sad_array) {
  sad_array[0] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr, ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr + 1, ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr + 2, ref_stride, 0x7fffffff);
}

void vp9_sad64x64x8_c(const uint8_t *src_ptr,
                      int  src_stride,
                      const uint8_t *ref_ptr,
                      int  ref_stride,
                      unsigned int *sad_array) {
  sad_array[0] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr, ref_stride,
                              0x7fffffff);
  sad_array[1] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr + 1, ref_stride,
                              0x7fffffff);
  sad_array[2] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr + 2, ref_stride,
                              0x7fffffff);
  sad_array[3] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr + 3, ref_stride,
                              0x7fffffff);
  sad_array[4] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr + 4, ref_stride,
                              0x7fffffff);
  sad_array[5] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr + 5, ref_stride,
                              0x7fffffff);
  sad_array[6] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr + 6, ref_stride,
                              0x7fffffff);
  sad_array[7] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr + 7, ref_stride,
                              0x7fffffff);
}

void vp9_sad32x32x8_c(const uint8_t *src_ptr,
                      int  src_stride,
                      const uint8_t *ref_ptr,
                      int  ref_stride,
                      unsigned int *sad_array) {
  sad_array[0] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr, ref_stride,
                              0x7fffffff);
  sad_array[1] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr + 1, ref_stride,
                              0x7fffffff);
  sad_array[2] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr + 2, ref_stride,
                              0x7fffffff);
  sad_array[3] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr + 3, ref_stride,
                              0x7fffffff);
  sad_array[4] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr + 4, ref_stride,
                              0x7fffffff);
  sad_array[5] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr + 5, ref_stride,
                              0x7fffffff);
  sad_array[6] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr + 6, ref_stride,
                              0x7fffffff);
  sad_array[7] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr + 7, ref_stride,
                              0x7fffffff);
}

void vp9_sad16x16x3_c(const uint8_t *src_ptr,
                      int  src_stride,
                      const uint8_t *ref_ptr,
                      int  ref_stride,
                      unsigned int *sad_array) {
  sad_array[0] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr, ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr + 1, ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr + 2, ref_stride, 0x7fffffff);
}

void vp9_sad16x16x8_c(const uint8_t *src_ptr,
                      int  src_stride,
                      const uint8_t *ref_ptr,
                      int  ref_stride,
                      uint32_t *sad_array) {
  sad_array[0] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr, ref_stride,
                              0x7fffffff);
  sad_array[1] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr + 1, ref_stride,
                              0x7fffffff);
  sad_array[2] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr + 2, ref_stride,
                              0x7fffffff);
  sad_array[3] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr + 3, ref_stride,
                              0x7fffffff);
  sad_array[4] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr + 4, ref_stride,
                              0x7fffffff);
  sad_array[5] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr + 5, ref_stride,
                              0x7fffffff);
  sad_array[6] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr + 6, ref_stride,
                              0x7fffffff);
  sad_array[7] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr + 7, ref_stride,
                              0x7fffffff);
}

void vp9_sad16x8x3_c(const uint8_t *src_ptr,
                     int  src_stride,
                     const uint8_t *ref_ptr,
                     int  ref_stride,
                     unsigned int *sad_array) {
  sad_array[0] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr, ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr + 1, ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr + 2, ref_stride, 0x7fffffff);
}

void vp9_sad16x8x8_c(const uint8_t *src_ptr,
                     int  src_stride,
                     const uint8_t *ref_ptr,
                     int  ref_stride,
                     uint32_t *sad_array) {
  sad_array[0] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr, ref_stride,
                             0x7fffffff);
  sad_array[1] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr + 1, ref_stride,
                             0x7fffffff);
  sad_array[2] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr + 2, ref_stride,
                             0x7fffffff);
  sad_array[3] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr + 3, ref_stride,
                             0x7fffffff);
  sad_array[4] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr + 4, ref_stride,
                             0x7fffffff);
  sad_array[5] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr + 5, ref_stride,
                             0x7fffffff);
  sad_array[6] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr + 6, ref_stride,
                             0x7fffffff);
  sad_array[7] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr + 7, ref_stride,
                             0x7fffffff);
}

void vp9_sad8x8x3_c(const uint8_t *src_ptr,
                    int  src_stride,
                    const uint8_t *ref_ptr,
                    int  ref_stride,
                    unsigned int *sad_array) {
  sad_array[0] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr, ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr + 1, ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr + 2, ref_stride, 0x7fffffff);
}

void vp9_sad8x8x8_c(const uint8_t *src_ptr,
                    int  src_stride,
                    const uint8_t *ref_ptr,
                    int  ref_stride,
                    uint32_t *sad_array) {
  sad_array[0] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr, ref_stride,
                            0x7fffffff);
  sad_array[1] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr + 1, ref_stride,
                            0x7fffffff);
  sad_array[2] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr + 2, ref_stride,
                            0x7fffffff);
  sad_array[3] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr + 3, ref_stride,
                            0x7fffffff);
  sad_array[4] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr + 4, ref_stride,
                            0x7fffffff);
  sad_array[5] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr + 5, ref_stride,
                            0x7fffffff);
  sad_array[6] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr + 6, ref_stride,
                            0x7fffffff);
  sad_array[7] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr + 7, ref_stride,
                            0x7fffffff);
}

void vp9_sad8x16x3_c(const uint8_t *src_ptr,
                     int  src_stride,
                     const uint8_t *ref_ptr,
                     int  ref_stride,
                     unsigned int *sad_array) {
  sad_array[0] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr, ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr + 1, ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr + 2, ref_stride, 0x7fffffff);
}

void vp9_sad8x16x8_c(const uint8_t *src_ptr,
                     int  src_stride,
                     const uint8_t *ref_ptr,
                     int  ref_stride,
                     uint32_t *sad_array) {
  sad_array[0] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr, ref_stride,
                             0x7fffffff);
  sad_array[1] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr + 1, ref_stride,
                             0x7fffffff);
  sad_array[2] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr + 2, ref_stride,
                             0x7fffffff);
  sad_array[3] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr + 3, ref_stride,
                             0x7fffffff);
  sad_array[4] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr + 4, ref_stride,
                             0x7fffffff);
  sad_array[5] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr + 5, ref_stride,
                             0x7fffffff);
  sad_array[6] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr + 6, ref_stride,
                             0x7fffffff);
  sad_array[7] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr + 7, ref_stride,
                             0x7fffffff);
}

void vp9_sad4x4x3_c(const uint8_t *src_ptr,
                    int  src_stride,
                    const uint8_t *ref_ptr,
                    int  ref_stride,
                    unsigned int *sad_array) {
  sad_array[0] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr, ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr + 1, ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr + 2, ref_stride, 0x7fffffff);
}

void vp9_sad4x4x8_c(const uint8_t *src_ptr,
                    int  src_stride,
                    const uint8_t *ref_ptr,
                    int  ref_stride,
                    uint32_t *sad_array) {
  sad_array[0] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr, ref_stride,
                            0x7fffffff);
  sad_array[1] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr + 1, ref_stride,
                            0x7fffffff);
  sad_array[2] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr + 2, ref_stride,
                            0x7fffffff);
  sad_array[3] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr + 3, ref_stride,
                            0x7fffffff);
  sad_array[4] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr + 4, ref_stride,
                            0x7fffffff);
  sad_array[5] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr + 5, ref_stride,
                            0x7fffffff);
  sad_array[6] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr + 6, ref_stride,
                            0x7fffffff);
  sad_array[7] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr + 7, ref_stride,
                            0x7fffffff);
}

void vp9_sad64x64x4d_c(const uint8_t *src_ptr,
                       int  src_stride,
                       const uint8_t* const ref_ptr[],
                       int  ref_stride,
                       unsigned int *sad_array) {
  sad_array[0] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr[0], ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr[1], ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr[2], ref_stride, 0x7fffffff);
  sad_array[3] = vp9_sad64x64(src_ptr, src_stride,
                              ref_ptr[3], ref_stride, 0x7fffffff);
}

void vp9_sad32x32x4d_c(const uint8_t *src_ptr,
                       int  src_stride,
                       const uint8_t* const ref_ptr[],
                       int  ref_stride,
                       unsigned int *sad_array) {
  sad_array[0] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr[0], ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr[1], ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr[2], ref_stride, 0x7fffffff);
  sad_array[3] = vp9_sad32x32(src_ptr, src_stride,
                              ref_ptr[3], ref_stride, 0x7fffffff);
}

void vp9_sad16x16x4d_c(const uint8_t *src_ptr,
                       int  src_stride,
                       const uint8_t* const ref_ptr[],
                       int  ref_stride,
                       unsigned int *sad_array) {
  sad_array[0] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr[0], ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr[1], ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr[2], ref_stride, 0x7fffffff);
  sad_array[3] = vp9_sad16x16(src_ptr, src_stride,
                              ref_ptr[3], ref_stride, 0x7fffffff);
}

void vp9_sad16x8x4d_c(const uint8_t *src_ptr,
                      int  src_stride,
                      const uint8_t* const ref_ptr[],
                      int  ref_stride,
                      unsigned int *sad_array) {
  sad_array[0] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr[0], ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr[1], ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr[2], ref_stride, 0x7fffffff);
  sad_array[3] = vp9_sad16x8(src_ptr, src_stride,
                             ref_ptr[3], ref_stride, 0x7fffffff);
}

void vp9_sad8x8x4d_c(const uint8_t *src_ptr,
                     int  src_stride,
                     const uint8_t* const ref_ptr[],
                     int  ref_stride,
                     unsigned int *sad_array) {
  sad_array[0] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr[0], ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr[1], ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr[2], ref_stride, 0x7fffffff);
  sad_array[3] = vp9_sad8x8(src_ptr, src_stride,
                            ref_ptr[3], ref_stride, 0x7fffffff);
}

void vp9_sad8x16x4d_c(const uint8_t *src_ptr,
                      int  src_stride,
                      const uint8_t* const ref_ptr[],
                      int  ref_stride,
                      unsigned int *sad_array) {
  sad_array[0] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr[0], ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr[1], ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr[2], ref_stride, 0x7fffffff);
  sad_array[3] = vp9_sad8x16(src_ptr, src_stride,
                             ref_ptr[3], ref_stride, 0x7fffffff);
}

void vp9_sad4x4x4d_c(const uint8_t *src_ptr,
                     int  src_stride,
                     const uint8_t* const ref_ptr[],
                     int  ref_stride,
                     unsigned int *sad_array) {
  sad_array[0] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr[0], ref_stride, 0x7fffffff);
  sad_array[1] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr[1], ref_stride, 0x7fffffff);
  sad_array[2] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr[2], ref_stride, 0x7fffffff);
  sad_array[3] = vp9_sad4x4(src_ptr, src_stride,
                            ref_ptr[3], ref_stride, 0x7fffffff);
}
