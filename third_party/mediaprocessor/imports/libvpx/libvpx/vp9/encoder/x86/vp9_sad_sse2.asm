;
;  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
;
;  Use of this source code is governed by a BSD-style license
;  that can be found in the LICENSE file in the root of the source
;  tree. An additional intellectual property rights grant can be found
;  in the file PATENTS.  All contributing project authors may
;  be found in the AUTHORS file in the root of the source tree.
;

%include "third_party/x86inc/x86inc.asm"

SECTION .text

; unsigned int vp9_sad64x64_sse2(uint8_t *src, int src_stride,
;                                uint8_t *ref, int ref_stride);
INIT_XMM sse2
cglobal sad64x64, 4, 5, 5, src, src_stride, ref, ref_stride, n_rows
  movsxdifnidn src_strideq, src_strided
  movsxdifnidn ref_strideq, ref_strided
  mov              n_rowsd, 64
  pxor                  m0, m0
.loop:
  movu                  m1, [refq]
  movu                  m2, [refq+16]
  movu                  m3, [refq+32]
  movu                  m4, [refq+48]
  psadbw                m1, [srcq]
  psadbw                m2, [srcq+16]
  psadbw                m3, [srcq+32]
  psadbw                m4, [srcq+48]
  paddd                 m1, m2
  paddd                 m3, m4
  add                 refq, ref_strideq
  paddd                 m0, m1
  add                 srcq, src_strideq
  paddd                 m0, m3
  dec              n_rowsd
  jg .loop

  movhlps               m1, m0
  paddd                 m0, m1
  movd                 eax, m0
  RET

; unsigned int vp9_sad32x32_sse2(uint8_t *src, int src_stride,
;                                uint8_t *ref, int ref_stride);
INIT_XMM sse2
cglobal sad32x32, 4, 5, 5, src, src_stride, ref, ref_stride, n_rows
  movsxdifnidn src_strideq, src_strided
  movsxdifnidn ref_strideq, ref_strided
  mov              n_rowsd, 16
  pxor                  m0, m0

.loop:
  movu                  m1, [refq]
  movu                  m2, [refq+16]
  movu                  m3, [refq+ref_strideq]
  movu                  m4, [refq+ref_strideq+16]
  psadbw                m1, [srcq]
  psadbw                m2, [srcq+16]
  psadbw                m3, [srcq+src_strideq]
  psadbw                m4, [srcq+src_strideq+16]
  paddd                 m1, m2
  paddd                 m3, m4
  lea                 refq, [refq+ref_strideq*2]
  paddd                 m0, m1
  lea                 srcq, [srcq+src_strideq*2]
  paddd                 m0, m3
  dec              n_rowsd
  jg .loop

  movhlps               m1, m0
  paddd                 m0, m1
  movd                 eax, m0
  RET

; unsigned int vp9_sad16x{8,16}_sse2(uint8_t *src, int src_stride,
;                                    uint8_t *ref, int ref_stride);
%macro SAD16XN 1
cglobal sad16x%1, 4, 7, 5, src, src_stride, ref, ref_stride, \
                           src_stride3, ref_stride3, n_rows
  movsxdifnidn src_strideq, src_strided
  movsxdifnidn ref_strideq, ref_strided
  lea         src_stride3q, [src_strideq*3]
  lea         ref_stride3q, [ref_strideq*3]
  mov              n_rowsd, %1/4
  pxor                  m0, m0

.loop:
  movu                  m1, [refq]
  movu                  m2, [refq+ref_strideq]
  movu                  m3, [refq+ref_strideq*2]
  movu                  m4, [refq+ref_stride3q]
  psadbw                m1, [srcq]
  psadbw                m2, [srcq+src_strideq]
  psadbw                m3, [srcq+src_strideq*2]
  psadbw                m4, [srcq+src_stride3q]
  paddd                 m1, m2
  paddd                 m3, m4
  lea                 refq, [refq+ref_strideq*4]
  paddd                 m0, m1
  lea                 srcq, [srcq+src_strideq*4]
  paddd                 m0, m3
  dec              n_rowsd
  jg .loop

  movhlps               m1, m0
  paddd                 m0, m1
  movd                 eax, m0
  RET
%endmacro

INIT_XMM sse2
SAD16XN 16 ; sad16x16_sse2
SAD16XN  8 ; sad16x8_sse2

; unsigned int vp9_sad8x{8,16}_sse2(uint8_t *src, int src_stride,
;                                   uint8_t *ref, int ref_stride);
%macro SAD8XN 1
cglobal sad8x%1, 4, 7, 5, src, src_stride, ref, ref_stride, \
                          src_stride3, ref_stride3, n_rows
  movsxdifnidn src_strideq, src_strided
  movsxdifnidn ref_strideq, ref_strided
  lea         src_stride3q, [src_strideq*3]
  lea         ref_stride3q, [ref_strideq*3]
  mov              n_rowsd, %1/4
  pxor                  m0, m0

.loop:
  movh                  m1, [refq]
  movhps                m1, [refq+ref_strideq]
  movh                  m2, [refq+ref_strideq*2]
  movhps                m2, [refq+ref_stride3q]
  movh                  m3, [srcq]
  movhps                m3, [srcq+src_strideq]
  movh                  m4, [srcq+src_strideq*2]
  movhps                m4, [srcq+src_stride3q]
  psadbw                m1, m3
  psadbw                m2, m4
  lea                 refq, [refq+ref_strideq*4]
  paddd                 m0, m1
  lea                 srcq, [srcq+src_strideq*4]
  paddd                 m0, m2
  dec              n_rowsd
  jg .loop

  movhlps               m1, m0
  paddd                 m0, m1
  movd                 eax, m0
  RET
%endmacro

INIT_XMM sse2
SAD8XN 16 ; sad8x16_sse2
SAD8XN  8 ; sad8x8_sse2

; unsigned int vp9_sad4x4_sse(uint8_t *src, int src_stride,
;                             uint8_t *ref, int ref_stride);
INIT_MMX sse
cglobal sad4x4, 4, 4, 8, src, src_stride, ref, ref_stride
  movsxdifnidn src_strideq, src_strided
  movsxdifnidn ref_strideq, ref_strided
  movd                  m0, [refq]
  movd                  m1, [refq+ref_strideq]
  movd                  m2, [srcq]
  movd                  m3, [srcq+src_strideq]
  lea                 refq, [refq+ref_strideq*2]
  lea                 srcq, [srcq+src_strideq*2]
  movd                  m4, [refq]
  movd                  m5, [refq+ref_strideq]
  movd                  m6, [srcq]
  movd                  m7, [srcq+src_strideq]
  punpckldq             m0, m1
  punpckldq             m2, m3
  punpckldq             m4, m5
  punpckldq             m6, m7
  psadbw                m0, m2
  psadbw                m4, m6
  paddd                 m0, m4
  movd                 eax, m0
  RET
