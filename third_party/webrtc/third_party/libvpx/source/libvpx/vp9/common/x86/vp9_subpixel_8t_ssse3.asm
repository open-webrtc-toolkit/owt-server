;
;  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
;
;  Use of this source code is governed by a BSD-style license
;  that can be found in the LICENSE file in the root of the source
;  tree. An additional intellectual property rights grant can be found
;  in the file PATENTS.  All contributing project authors may
;  be found in the AUTHORS file in the root of the source tree.
;


%include "vpx_ports/x86_abi_support.asm"

;/************************************************************************************
; Notes: filter_block1d_h6 applies a 6 tap filter horizontally to the input pixels. The
; input pixel array has output_height rows. This routine assumes that output_height is an
; even number. This function handles 8 pixels in horizontal direction, calculating ONE
; rows each iteration to take advantage of the 128 bits operations.
;
; This is an implementation of some of the SSE optimizations first seen in ffvp8
;
;*************************************************************************************/

;void vp9_filter_block1d8_v8_ssse3
;(
;    unsigned char *src_ptr,
;    unsigned int   src_pitch,
;    unsigned char *output_ptr,
;    unsigned int   out_pitch,
;    unsigned int   output_height,
;    short *filter
;)
global sym(vp9_filter_block1d8_v8_ssse3) PRIVATE
sym(vp9_filter_block1d8_v8_ssse3):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    SAVE_XMM 7
    push        rsi
    push        rdi
    push        rbx
    ; end prolog

    ALIGN_STACK 16, rax
    sub         rsp, 16*5
    %define k0k1 [rsp + 16*0]
    %define k2k3 [rsp + 16*1]
    %define k4k5 [rsp + 16*2]
    %define k6k7 [rsp + 16*3]
    %define krd [rsp + 16*4]

    mov         rdx, arg(5)                 ;filter ptr
    mov         rsi, arg(0)                 ;src_ptr
    mov         rdi, arg(2)                 ;output_ptr
    mov         rcx, 0x0400040

    movdqa      xmm4, [rdx]                 ;load filters
    movd        xmm5, rcx
    packsswb    xmm4, xmm4
    pshuflw     xmm0, xmm4, 0b              ;k0_k1
    pshuflw     xmm1, xmm4, 01010101b       ;k2_k3
    pshuflw     xmm2, xmm4, 10101010b       ;k4_k5
    pshuflw     xmm3, xmm4, 11111111b       ;k6_k7

    punpcklqdq  xmm0, xmm0
    punpcklqdq  xmm1, xmm1
    punpcklqdq  xmm2, xmm2
    punpcklqdq  xmm3, xmm3

    movdqa      k0k1, xmm0
    movdqa      k2k3, xmm1
    pshufd      xmm5, xmm5, 0
    movdqa      k4k5, xmm2
    movdqa      k6k7, xmm3
    movdqa      krd, xmm5

    movsxd      rdx, DWORD PTR arg(1)       ;pixels_per_line

%if ABI_IS_32BIT=0
    movsxd      r8, DWORD PTR arg(3)        ;out_pitch
%endif
    mov         rax, rsi
    movsxd      rcx, DWORD PTR arg(4)       ;output_height
    add         rax, rdx

    lea         rbx, [rdx + rdx*4]
    add         rbx, rdx                    ;pitch * 6

.vp9_filter_block1d8_v8_ssse3_loop:
    movq        xmm0, [rsi]                 ;A
    movq        xmm1, [rsi + rdx]           ;B
    movq        xmm2, [rsi + rdx * 2]       ;C
    movq        xmm3, [rax + rdx * 2]       ;D
    movq        xmm4, [rsi + rdx * 4]       ;E
    movq        xmm5, [rax + rdx * 4]       ;F

    punpcklbw   xmm0, xmm1                  ;A B
    punpcklbw   xmm2, xmm3                  ;C D
    punpcklbw   xmm4, xmm5                  ;E F

    movq        xmm6, [rsi + rbx]           ;G
    movq        xmm7, [rax + rbx]           ;H

    pmaddubsw   xmm0, k0k1
    pmaddubsw   xmm2, k2k3
    punpcklbw   xmm6, xmm7                  ;G H
    pmaddubsw   xmm4, k4k5
    pmaddubsw   xmm6, k6k7

    paddsw      xmm0, xmm2
    paddsw      xmm0, krd
    paddsw      xmm4, xmm6
    paddsw      xmm0, xmm4

    psraw       xmm0, 7
    packuswb    xmm0, xmm0

    add         rsi,  rdx
    add         rax,  rdx

    movq        [rdi], xmm0

%if ABI_IS_32BIT
    add         rdi, DWORD PTR arg(3)       ;out_pitch
%else
    add         rdi, r8
%endif
    dec         rcx
    jnz         .vp9_filter_block1d8_v8_ssse3_loop

    add rsp, 16*5
    pop rsp
    pop rbx
    ; begin epilog
    pop rdi
    pop rsi
    RESTORE_XMM
    UNSHADOW_ARGS
    pop         rbp
    ret

;void vp9_filter_block1d16_v8_ssse3
;(
;    unsigned char *src_ptr,
;    unsigned int   src_pitch,
;    unsigned char *output_ptr,
;    unsigned int   out_pitch,
;    unsigned int   output_height,
;    short *filter
;)
global sym(vp9_filter_block1d16_v8_ssse3) PRIVATE
sym(vp9_filter_block1d16_v8_ssse3):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    SAVE_XMM 7
    push        rsi
    push        rdi
    push        rbx
    ; end prolog

    ALIGN_STACK 16, rax
    sub         rsp, 16*5
    %define k0k1 [rsp + 16*0]
    %define k2k3 [rsp + 16*1]
    %define k4k5 [rsp + 16*2]
    %define k6k7 [rsp + 16*3]
    %define krd [rsp + 16*4]

    mov         rdx, arg(5)                 ;filter ptr
    mov         rsi, arg(0)                 ;src_ptr
    mov         rdi, arg(2)                 ;output_ptr
    mov         rcx, 0x0400040

    movdqa      xmm4, [rdx]                 ;load filters
    movd        xmm5, rcx
    packsswb    xmm4, xmm4
    pshuflw     xmm0, xmm4, 0b              ;k0_k1
    pshuflw     xmm1, xmm4, 01010101b       ;k2_k3
    pshuflw     xmm2, xmm4, 10101010b       ;k4_k5
    pshuflw     xmm3, xmm4, 11111111b       ;k6_k7

    punpcklqdq  xmm0, xmm0
    punpcklqdq  xmm1, xmm1
    punpcklqdq  xmm2, xmm2
    punpcklqdq  xmm3, xmm3

    movdqa      k0k1, xmm0
    movdqa      k2k3, xmm1
    pshufd      xmm5, xmm5, 0
    movdqa      k4k5, xmm2
    movdqa      k6k7, xmm3
    movdqa      krd, xmm5

    movsxd      rdx, DWORD PTR arg(1)       ;pixels_per_line

%if ABI_IS_32BIT=0
    movsxd      r8, DWORD PTR arg(3)        ;out_pitch
%endif
    mov         rax, rsi
    movsxd      rcx, DWORD PTR arg(4)       ;output_height
    add         rax, rdx

    lea         rbx, [rdx + rdx*4]
    add         rbx, rdx                    ;pitch * 6

.vp9_filter_block1d16_v8_ssse3_loop:
    movq        xmm0, [rsi]                 ;A
    movq        xmm1, [rsi + rdx]           ;B
    movq        xmm2, [rsi + rdx * 2]       ;C
    movq        xmm3, [rax + rdx * 2]       ;D
    movq        xmm4, [rsi + rdx * 4]       ;E
    movq        xmm5, [rax + rdx * 4]       ;F

    punpcklbw   xmm0, xmm1                  ;A B
    punpcklbw   xmm2, xmm3                  ;C D
    punpcklbw   xmm4, xmm5                  ;E F

    movq        xmm6, [rsi + rbx]           ;G
    movq        xmm7, [rax + rbx]           ;H

    pmaddubsw   xmm0, k0k1
    pmaddubsw   xmm2, k2k3
    punpcklbw   xmm6, xmm7                  ;G H
    pmaddubsw   xmm4, k4k5
    pmaddubsw   xmm6, k6k7

    paddsw      xmm0, xmm2
    paddsw      xmm0, krd
    paddsw      xmm4, xmm6
    paddsw      xmm0, xmm4

    psraw       xmm0, 7
    packuswb    xmm0, xmm0

    movq        [rdi], xmm0

    movq        xmm0, [rsi + 8]             ;A
    movq        xmm1, [rsi + rdx + 8]       ;B
    movq        xmm2, [rsi + rdx * 2 + 8]   ;C
    movq        xmm3, [rax + rdx * 2 + 8]   ;D
    movq        xmm4, [rsi + rdx * 4 + 8]   ;E
    movq        xmm5, [rax + rdx * 4 + 8]   ;F

    punpcklbw   xmm0, xmm1                  ;A B
    punpcklbw   xmm2, xmm3                  ;C D
    punpcklbw   xmm4, xmm5                  ;E F


    movq        xmm6, [rsi + rbx + 8]       ;G
    movq        xmm7, [rax + rbx + 8]       ;H
    punpcklbw   xmm6, xmm7                  ;G H


    pmaddubsw   xmm0, k0k1
    pmaddubsw   xmm2, k2k3
    pmaddubsw   xmm4, k4k5
    pmaddubsw   xmm6, k6k7

    paddsw      xmm0, xmm2
    paddsw      xmm4, xmm6
    paddsw      xmm0, krd
    paddsw      xmm0, xmm4

    psraw       xmm0, 7
    packuswb    xmm0, xmm0

    add         rsi,  rdx
    add         rax,  rdx

    movq        [rdi+8], xmm0

%if ABI_IS_32BIT
    add         rdi, DWORD PTR arg(3)       ;out_pitch
%else
    add         rdi, r8
%endif
    dec         rcx
    jnz         .vp9_filter_block1d16_v8_ssse3_loop

    add rsp, 16*5
    pop rsp
    pop rbx
    ; begin epilog
    pop rdi
    pop rsi
    RESTORE_XMM
    UNSHADOW_ARGS
    pop         rbp
    ret

;void vp9_filter_block1d8_h8_ssse3
;(
;    unsigned char  *src_ptr,
;    unsigned int    src_pixels_per_line,
;    unsigned char  *output_ptr,
;    unsigned int    output_pitch,
;    unsigned int    output_height,
;    short *filter
;)
global sym(vp9_filter_block1d8_h8_ssse3) PRIVATE
sym(vp9_filter_block1d8_h8_ssse3):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    SAVE_XMM 7
    GET_GOT     rbx
    push        rsi
    push        rdi
    ; end prolog

    ALIGN_STACK 16, rax
    sub         rsp, 16*5
    %define k0k1 [rsp + 16*0]
    %define k2k3 [rsp + 16*1]
    %define k4k5 [rsp + 16*2]
    %define k6k7 [rsp + 16*3]
    %define krd [rsp + 16*4]

    mov         rdx, arg(5)                 ;filter ptr
    mov         rsi, arg(0)                 ;src_ptr
    mov         rdi, arg(2)                 ;output_ptr
    mov         rcx, 0x0400040

    movdqa      xmm4, [rdx]                 ;load filters
    movd        xmm5, rcx
    packsswb    xmm4, xmm4
    pshuflw     xmm0, xmm4, 0b              ;k0_k1
    pshuflw     xmm1, xmm4, 01010101b       ;k2_k3
    pshuflw     xmm2, xmm4, 10101010b       ;k4_k5
    pshuflw     xmm3, xmm4, 11111111b       ;k6_k7

    punpcklqdq  xmm0, xmm0
    punpcklqdq  xmm1, xmm1
    punpcklqdq  xmm2, xmm2
    punpcklqdq  xmm3, xmm3

    movdqa      k0k1, xmm0
    movdqa      k2k3, xmm1
    pshufd      xmm5, xmm5, 0
    movdqa      k4k5, xmm2
    movdqa      k6k7, xmm3
;    movdqa      krd, xmm5

    movsxd      rax, dword ptr arg(1)       ;src_pixels_per_line
    movsxd      rdx, dword ptr arg(3)       ;output_pitch
    movsxd      rcx, dword ptr arg(4)       ;output_height

.filter_block1d8_h8_rowloop_ssse3:
    movq        xmm0,   [rsi - 3]    ; -3 -2 -1  0  1  2  3  4

;    movq        xmm3,   [rsi + 4]    ; 4  5  6  7  8  9 10 11
    movq        xmm3,   [rsi + 5]    ; 5  6  7  8  9 10 11 12
;note: if we create a k0_k7 filter, we can save a pshufb
;    punpcklbw   xmm0,   xmm3         ; -3 4 -2 5 -1 6 0 7 1 8 2 9 3 10 4 11
    punpcklqdq  xmm0,   xmm3

    movdqa      xmm1,   xmm0
    pshufb      xmm0,   [GLOBAL(shuf_t0t1)]
    pmaddubsw   xmm0,   k0k1

    movdqa      xmm2,   xmm1
    pshufb      xmm1,   [GLOBAL(shuf_t2t3)]
    pmaddubsw   xmm1,   k2k3

    movdqa      xmm4,   xmm2
    pshufb      xmm2,   [GLOBAL(shuf_t4t5)]
    pmaddubsw   xmm2,   k4k5

    pshufb      xmm4,   [GLOBAL(shuf_t6t7)]
    pmaddubsw   xmm4,   k6k7

    paddsw      xmm0,   xmm1
    paddsw      xmm0,   xmm2
    paddsw      xmm0,   xmm5
    paddsw      xmm0,   xmm4
    psraw       xmm0,   7
    packuswb    xmm0,   xmm0

    lea         rsi,    [rsi + rax]
    movq        [rdi],  xmm0

    lea         rdi,    [rdi + rdx]
    dec         rcx
    jnz         .filter_block1d8_h8_rowloop_ssse3

    add rsp, 16*5
    pop rsp

    ; begin epilog
    pop rdi
    pop rsi
    RESTORE_GOT
    RESTORE_XMM
    UNSHADOW_ARGS
    pop         rbp
    ret

;void vp9_filter_block1d16_h8_ssse3
;(
;    unsigned char  *src_ptr,
;    unsigned int    src_pixels_per_line,
;    unsigned char  *output_ptr,
;    unsigned int    output_pitch,
;    unsigned int    output_height,
;    short *filter
;)
global sym(vp9_filter_block1d16_h8_ssse3) PRIVATE
sym(vp9_filter_block1d16_h8_ssse3):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    SAVE_XMM 7
    GET_GOT     rbx
    push        rsi
    push        rdi
    ; end prolog

    ALIGN_STACK 16, rax
    sub         rsp, 16*5
    %define k0k1 [rsp + 16*0]
    %define k2k3 [rsp + 16*1]
    %define k4k5 [rsp + 16*2]
    %define k6k7 [rsp + 16*3]
    %define krd [rsp + 16*4]

    mov         rdx, arg(5)                 ;filter ptr
    mov         rsi, arg(0)                 ;src_ptr
    mov         rdi, arg(2)                 ;output_ptr
    mov         rcx, 0x0400040

    movdqa      xmm4, [rdx]                 ;load filters
    movd        xmm5, rcx
    packsswb    xmm4, xmm4
    pshuflw     xmm0, xmm4, 0b              ;k0_k1
    pshuflw     xmm1, xmm4, 01010101b       ;k2_k3
    pshuflw     xmm2, xmm4, 10101010b       ;k4_k5
    pshuflw     xmm3, xmm4, 11111111b       ;k6_k7

    punpcklqdq  xmm0, xmm0
    punpcklqdq  xmm1, xmm1
    punpcklqdq  xmm2, xmm2
    punpcklqdq  xmm3, xmm3

    movdqa      k0k1, xmm0
    movdqa      k2k3, xmm1
    pshufd      xmm5, xmm5, 0
    movdqa      k4k5, xmm2
    movdqa      k6k7, xmm3
    movdqa      krd, xmm5

    movsxd      rax, dword ptr arg(1)       ;src_pixels_per_line
    movsxd      rdx, dword ptr arg(3)       ;output_pitch
    movsxd      rcx, dword ptr arg(4)       ;output_height

.filter_block1d16_h8_rowloop_ssse3:
    movq        xmm0,   [rsi - 3]    ; -3 -2 -1  0  1  2  3  4

;    movq        xmm3,   [rsi + 4]    ; 4  5  6  7  8  9 10 11
    movq        xmm3,   [rsi + 5]    ; 5  6  7  8  9 10 11 12
;note: if we create a k0_k7 filter, we can save a pshufb
;    punpcklbw   xmm0,   xmm3         ; -3 4 -2 5 -1 6 0 7 1 8 2 9 3 10 4 11
    punpcklqdq  xmm0,   xmm3

    movdqa      xmm1,   xmm0
    pshufb      xmm0,   [GLOBAL(shuf_t0t1)]
    pmaddubsw   xmm0,   k0k1

    movdqa      xmm2,   xmm1
    pshufb      xmm1,   [GLOBAL(shuf_t2t3)]
    pmaddubsw   xmm1,   k2k3

    movdqa      xmm4,   xmm2
    pshufb      xmm2,   [GLOBAL(shuf_t4t5)]
    pmaddubsw   xmm2,   k4k5

    pshufb      xmm4,   [GLOBAL(shuf_t6t7)]
    pmaddubsw   xmm4,   k6k7

    paddsw      xmm0,   xmm1
    paddsw      xmm0,   xmm4
    paddsw      xmm0,   xmm2
    paddsw      xmm0,   krd
    psraw       xmm0,   7
    packuswb    xmm0,   xmm0


    movq        xmm3,   [rsi +  5]
;    movq        xmm7,   [rsi + 12]
    movq        xmm7,   [rsi + 13]
;note: same as above
;    punpcklbw   xmm3,   xmm7
    punpcklqdq  xmm3,   xmm7

    movdqa      xmm1,   xmm3
    pshufb      xmm3,   [GLOBAL(shuf_t0t1)]
    pmaddubsw   xmm3,   k0k1

    movdqa      xmm2,   xmm1
    pshufb      xmm1,   [GLOBAL(shuf_t2t3)]
    pmaddubsw   xmm1,   k2k3

    movdqa      xmm4,   xmm2
    pshufb      xmm2,   [GLOBAL(shuf_t4t5)]
    pmaddubsw   xmm2,   k4k5

    pshufb      xmm4,   [GLOBAL(shuf_t6t7)]
    pmaddubsw   xmm4,   k6k7

    paddsw      xmm3,   xmm1
    paddsw      xmm3,   xmm2
    paddsw      xmm3,   krd
    paddsw      xmm3,   xmm4
    psraw       xmm3,   7
    packuswb    xmm3,   xmm3
    punpcklqdq  xmm0,   xmm3

    lea         rsi,    [rsi + rax]
    movdqa      [rdi],  xmm0

    lea         rdi,    [rdi + rdx]
    dec         rcx
    jnz         .filter_block1d16_h8_rowloop_ssse3

    add rsp, 16*5
    pop rsp

    ; begin epilog
    pop rdi
    pop rsi
    RESTORE_GOT
    RESTORE_XMM
    UNSHADOW_ARGS
    pop         rbp
    ret


SECTION_RODATA
align 16
shuf_t0t1:
    db  0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8
align 16
shuf_t2t3:
    db  2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10
align 16
shuf_t4t5:
    db  4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12
align 16
shuf_t6t7:
    db  6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14
