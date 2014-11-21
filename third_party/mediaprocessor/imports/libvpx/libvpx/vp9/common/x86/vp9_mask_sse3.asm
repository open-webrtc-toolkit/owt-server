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

;void int vp8_makemask_sse3(
;    unsigned char *y,
;    unsigned char *u,
;    unsigned char *v,
;    unsigned char *ym,
;    unsigned char *uvm,
;    int yp,
;    int uvp,
;    int ys,
;    int us,
;    int vs,
;    int yt,
;    int ut,
;    int vt)
global sym(vp8_makemask_sse3) PRIVATE
sym(vp8_makemask_sse3):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 14
    push        rsi
    push        rdi
    ; end prolog

        mov             rsi,        arg(0) ;y
        mov             rdi,        arg(1) ;u
        mov             rcx,        arg(2) ;v
        mov             rax,        arg(3) ;ym
        movsxd          rbx,        dword arg(4) ;yp
        movsxd          rdx,        dword arg(5) ;uvp

        pxor            xmm0,xmm0

        ;make 16 copies of the center y value
        movd            xmm1, arg(6)
        pshufb          xmm1, xmm0

        ; make 16 copies of the center u value
        movd            xmm2, arg(7)
        pshufb          xmm2, xmm0

        ; make 16 copies of the center v value
        movd            xmm3, arg(8)
        pshufb          xmm3, xmm0
        unpcklpd        xmm2, xmm3

        ;make 16 copies of the y tolerance
        movd            xmm3, arg(9)
        pshufb          xmm3, xmm0

        ;make 16 copies of the u tolerance
        movd            xmm4, arg(10)
        pshufb          xmm4, xmm0

        ;make 16 copies of the v tolerance
        movd            xmm5, arg(11)
        pshufb          xmm5, xmm0
        unpckhpd        xmm4, xmm5

        mov             r8,8

NextPairOfRows:

        ;grab the y source values
        movdqu          xmm0, [rsi]

        ;compute abs difference between source and y target
        movdqa          xmm6, xmm1
        movdqa          xmm7, xmm0
        psubusb         xmm0, xmm1
        psubusb         xmm6, xmm7
        por             xmm0, xmm6

        ;compute abs difference between
        movdqa          xmm6, xmm3
        pcmpgtb         xmm6, xmm0

        ;grab the y source values
        add             rsi, rbx
        movdqu          xmm0, [rsi]

        ;compute abs difference between source and y target
        movdqa          xmm11, xmm1
        movdqa          xmm7, xmm0
        psubusb         xmm0, xmm1
        psubusb         xmm11, xmm7
        por             xmm0, xmm11

        ;compute abs difference between
        movdqa          xmm11, xmm3
        pcmpgtb         xmm11, xmm0


        ;grab the u and v source values
        movdqu          xmm7, [rdi]
        movdqu          xmm8, [rcx]
        unpcklpd        xmm7, xmm8

        ;compute abs difference between source and uv targets
        movdqa          xmm9, xmm2
        movdqa          xmm10, xmm7
        psubusb         xmm7, xmm2
        psubusb         xmm9, xmm10
        por             xmm7, xmm9

        ;check whether the number is < tolerance
        movdqa          xmm0, xmm4
        pcmpgtb         xmm0, xmm7

        ;double  u and v masks
        movdqa          xmm8, xmm0
        punpckhbw       xmm0, xmm0
        punpcklbw       xmm8, xmm8

        ;mask row 0 and output
        pand            xmm6, xmm8
        pand            xmm6, xmm0
        movdqa          [rax],xmm6

        ;mask row 1 and output
        pand            xmm11, xmm8
        pand            xmm11, xmm0
        movdqa          [rax+16],xmm11


        ; to the next row or set of rows
        add             rsi, rbx
        add             rdi, rdx
        add             rcx, rdx
        add             rax,32
        dec r8
        jnz NextPairOfRows


    ; begin epilog
    pop         rdi
    pop         rsi
    UNSHADOW_ARGS
    pop         rbp
    ret

;GROW_HORIZ (register for result, source register or mem local)
; takes source and shifts left and ors with source
; then shifts right and ors with source
%macro GROW_HORIZ 2
    movdqa          %1, %2
    movdqa          xmm14, %1
    movdqa          xmm15, %1
    pslldq          xmm14, 1
    psrldq          xmm15, 1
    por             %1,xmm14
    por             %1,xmm15
%endmacro
;GROW_VERT (result, center row, above row, below row)
%macro GROW_VERT 4
    movdqa          %1,%2
    por             %1,%3
    por             %1,%4
%endmacro

;GROW_NEXTLINE (new line to grow, new source, line to write)
%macro GROW_NEXTLINE 3
    GROW_HORIZ %1, %2
    GROW_VERT xmm3, xmm0, xmm1, xmm2
    movdqa %3,xmm3
%endmacro


;void int vp8_growmaskmb_sse3(
;    unsigned char *om,
;    unsigned char *nm,
global sym(vp8_growmaskmb_sse3) PRIVATE
sym(vp8_growmaskmb_sse3):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 2
    push        rsi
    push        rdi
    ; end prolog

    mov             rsi,        arg(0) ;src
    mov             rdi,        arg(1) ;rst

    GROW_HORIZ xmm0, [rsi]
    GROW_HORIZ xmm1, [rsi+16]
    GROW_HORIZ xmm2, [rsi+32]

    GROW_VERT xmm3, xmm0, xmm1, xmm2
    por xmm0,xmm1
    movdqa [rdi], xmm0
    movdqa [rdi+16],xmm3

    GROW_NEXTLINE xmm0,[rsi+48],[rdi+32]
    GROW_NEXTLINE xmm1,[rsi+64],[rdi+48]
    GROW_NEXTLINE xmm2,[rsi+80],[rdi+64]
    GROW_NEXTLINE xmm0,[rsi+96],[rdi+80]
    GROW_NEXTLINE xmm1,[rsi+112],[rdi+96]
    GROW_NEXTLINE xmm2,[rsi+128],[rdi+112]
    GROW_NEXTLINE xmm0,[rsi+144],[rdi+128]
    GROW_NEXTLINE xmm1,[rsi+160],[rdi+144]
    GROW_NEXTLINE xmm2,[rsi+176],[rdi+160]
    GROW_NEXTLINE xmm0,[rsi+192],[rdi+176]
    GROW_NEXTLINE xmm1,[rsi+208],[rdi+192]
    GROW_NEXTLINE xmm2,[rsi+224],[rdi+208]
    GROW_NEXTLINE xmm0,[rsi+240],[rdi+224]

    por xmm0,xmm2
    movdqa [rdi+240], xmm0

    ; begin epilog
    pop         rdi
    pop         rsi
    UNSHADOW_ARGS
    pop         rbp
    ret



;unsigned int vp8_sad16x16_masked_wmt(
;    unsigned char *src_ptr,
;    int  src_stride,
;    unsigned char *ref_ptr,
;    int  ref_stride,
;    unsigned char *mask)
global sym(vp8_sad16x16_masked_wmt) PRIVATE
sym(vp8_sad16x16_masked_wmt):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 5
    push        rsi
    push        rdi
    ; end prolog
    mov             rsi,        arg(0) ;src_ptr
    mov             rdi,        arg(2) ;ref_ptr

    mov             rbx,        arg(4) ;mask
    movsxd          rax,        dword ptr arg(1) ;src_stride
    movsxd          rdx,        dword ptr arg(3) ;ref_stride

    mov             rcx,        16

    pxor            xmm3,       xmm3

NextSadRow:
    movdqu          xmm0,       [rsi]
    movdqu          xmm1,       [rdi]
    movdqu          xmm2,       [rbx]
    pand            xmm0,       xmm2
    pand            xmm1,       xmm2

    psadbw          xmm0,       xmm1
    paddw           xmm3,       xmm0

    add             rsi, rax
    add             rdi, rdx
    add             rbx,  16

    dec rcx
    jnz NextSadRow

    movdqa          xmm4 ,     xmm3
    psrldq          xmm4,       8
    paddw           xmm3,      xmm4
    movq            rax,       xmm3
    ; begin epilog
    pop rdi
    pop rsi
    UNSHADOW_ARGS
    pop         rbp
    ret


;unsigned int vp8_sad16x16_unmasked_wmt(
;    unsigned char *src_ptr,
;    int  src_stride,
;    unsigned char *ref_ptr,
;    int  ref_stride,
;    unsigned char *mask)
global sym(vp8_sad16x16_unmasked_wmt) PRIVATE
sym(vp8_sad16x16_unmasked_wmt):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 5
    push        rsi
    push        rdi
    ; end prolog
    mov             rsi,        arg(0) ;src_ptr
    mov             rdi,        arg(2) ;ref_ptr

    mov             rbx,        arg(4) ;mask
    movsxd          rax,        dword ptr arg(1) ;src_stride
    movsxd          rdx,        dword ptr arg(3) ;ref_stride

    mov             rcx,        16

    pxor            xmm3,       xmm3

next_vp8_sad16x16_unmasked_wmt:
    movdqu          xmm0,       [rsi]
    movdqu          xmm1,       [rdi]
    movdqu          xmm2,       [rbx]
    por             xmm0,       xmm2
    por             xmm1,       xmm2

    psadbw          xmm0,       xmm1
    paddw           xmm3,       xmm0

    add             rsi, rax
    add             rdi, rdx
    add             rbx,  16

    dec rcx
    jnz next_vp8_sad16x16_unmasked_wmt

    movdqa          xmm4 ,     xmm3
    psrldq          xmm4,       8
    paddw           xmm3,      xmm4
    movq            rax,        xmm3
    ; begin epilog
    pop rdi
    pop rsi
    UNSHADOW_ARGS
    pop         rbp
    ret


;unsigned int vp8_masked_predictor_wmt(
;    unsigned char *masked,
;    unsigned char *unmasked,
;    int  src_stride,
;    unsigned char *dst_ptr,
;    int  dst_stride,
;    unsigned char *mask)
global sym(vp8_masked_predictor_wmt) PRIVATE
sym(vp8_masked_predictor_wmt):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    push        rsi
    push        rdi
    ; end prolog
    mov             rsi,        arg(0) ;src_ptr
    mov             rdi,        arg(1) ;ref_ptr

    mov             rbx,        arg(5) ;mask
    movsxd          rax,        dword ptr arg(2) ;src_stride
    mov             r11,        arg(3) ; destination
    movsxd          rdx,        dword ptr arg(4) ;dst_stride

    mov             rcx,        16

    pxor            xmm3,       xmm3

next_vp8_masked_predictor_wmt:
    movdqu          xmm0,       [rsi]
    movdqu          xmm1,       [rdi]
    movdqu          xmm2,       [rbx]

    pand            xmm0,       xmm2
    pandn           xmm2,       xmm1
    por             xmm0,       xmm2
    movdqu          [r11],      xmm0

    add             r11, rdx
    add             rsi, rax
    add             rdi, rdx
    add             rbx,  16

    dec rcx
    jnz next_vp8_masked_predictor_wmt

    ; begin epilog
    pop rdi
    pop rsi
    UNSHADOW_ARGS
    pop         rbp
    ret

;unsigned int vp8_masked_predictor_uv_wmt(
;    unsigned char *masked,
;    unsigned char *unmasked,
;    int  src_stride,
;    unsigned char *dst_ptr,
;    int  dst_stride,
;    unsigned char *mask)
global sym(vp8_masked_predictor_uv_wmt) PRIVATE
sym(vp8_masked_predictor_uv_wmt):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    push        rsi
    push        rdi
    ; end prolog
    mov             rsi,        arg(0) ;src_ptr
    mov             rdi,        arg(1) ;ref_ptr

    mov             rbx,        arg(5) ;mask
    movsxd          rax,        dword ptr arg(2) ;src_stride
    mov             r11,        arg(3) ; destination
    movsxd          rdx,        dword ptr arg(4) ;dst_stride

    mov             rcx,        8

    pxor            xmm3,       xmm3

next_vp8_masked_predictor_uv_wmt:
    movq            xmm0,       [rsi]
    movq            xmm1,       [rdi]
    movq            xmm2,       [rbx]

    pand            xmm0,       xmm2
    pandn           xmm2,       xmm1
    por             xmm0,       xmm2
    movq            [r11],      xmm0

    add             r11, rdx
    add             rsi, rax
    add             rdi, rax
    add             rbx,  8

    dec rcx
    jnz next_vp8_masked_predictor_uv_wmt

    ; begin epilog
    pop rdi
    pop rsi
    UNSHADOW_ARGS
    pop         rbp
    ret


;unsigned int vp8_uv_from_y_mask(
;    unsigned char *ymask,
;    unsigned char *uvmask)
global sym(vp8_uv_from_y_mask) PRIVATE
sym(vp8_uv_from_y_mask):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    push        rsi
    push        rdi
    ; end prolog
    mov             rsi,        arg(0) ;src_ptr
    mov             rdi,        arg(1) ;dst_ptr


    mov             rcx,        8

    pxor            xmm3,       xmm3

next_p8_uv_from_y_mask:
    movdqu          xmm0,       [rsi]
    pshufb          xmm0, [shuf1b] ;[GLOBAL(shuf1b)]
    movq            [rdi],xmm0
    add             rdi, 8
    add             rsi,32

    dec rcx
    jnz next_p8_uv_from_y_mask

    ; begin epilog
    pop rdi
    pop rsi
    UNSHADOW_ARGS
    pop         rbp
    ret

SECTION_RODATA
align 16
shuf1b:
    db 0, 2, 4, 6, 8, 10, 12, 14, 0, 0, 0, 0, 0, 0, 0, 0

