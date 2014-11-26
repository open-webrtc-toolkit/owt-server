;
;  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
;
;  Use of this source code is governed by a BSD-style license and patent
;  grant that can be found in the LICENSE file in the root of the source
;  tree. All contributing project authors may be found in the AUTHORS
;  file in the root of the source tree.
;


    EXPORT  |idct_dequant_0_2x_neon|
    ARM
    REQUIRE8
    PRESERVE8

    AREA ||.text||, CODE, READONLY, ALIGN=2
;void idct_dequant_0_2x_neon(short *q, short dq,
;                            unsigned char *dst, int stride);
; r0   *q
; r1   dq
; r2   *dst
; r3   stride
|idct_dequant_0_2x_neon| PROC
    push            {r4, r5}

    add             r12, r2, #4
    vld1.32         {d2[0]}, [r2], r3
    vld1.32         {d8[0]}, [r12], r3
    vld1.32         {d2[1]}, [r2], r3
    vld1.32         {d8[1]}, [r12], r3
    vld1.32         {d4[0]}, [r2], r3
    vld1.32         {d10[0]}, [r12], r3
    vld1.32         {d4[1]}, [r2], r3
    vld1.32         {d10[1]}, [r12], r3

    ldrh            r12, [r0]               ; lo q
    ldrh            r4, [r0, #32]           ; hi q
    mov             r5, #0
    strh            r5, [r0]
    strh            r5, [r0, #32]

    sxth            r12, r12                ; lo
    mul             r0, r12, r1
    add             r0, r0, #4
    asr             r0, r0, #3
    vdup.16         q0, r0
    sxth            r4, r4                  ; hi
    mul             r0, r4, r1
    add             r0, r0, #4
    asr             r0, r0, #3
    vdup.16         q3, r0

    vaddw.u8        q1, q0, d2              ; lo
    vaddw.u8        q2, q0, d4
    vaddw.u8        q4, q3, d8              ; hi
    vaddw.u8        q5, q3, d10

    sub             r2, r2, r3, lsl #2      ; dst - 4*stride
    add             r0, r2, #4

    vqmovun.s16     d2, q1                  ; lo
    vqmovun.s16     d4, q2
    vqmovun.s16     d8, q4                  ; hi
    vqmovun.s16     d10, q5

    vst1.32         {d2[0]}, [r2], r3       ; lo
    vst1.32         {d8[0]}, [r0], r3       ; hi
    vst1.32         {d2[1]}, [r2], r3
    vst1.32         {d8[1]}, [r0], r3
    vst1.32         {d4[0]}, [r2], r3
    vst1.32         {d10[0]}, [r0], r3
    vst1.32         {d4[1]}, [r2]
    vst1.32         {d10[1]}, [r0]

    pop             {r4, r5}
    bx              lr

    ENDP            ; |idct_dequant_0_2x_neon|
    END
