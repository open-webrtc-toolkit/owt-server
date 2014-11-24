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

; Use of pmaxub instead of psubusb to compute filter mask was seen
; in ffvp8

%macro LFH_FILTER_AND_HEV_MASK 1
%if %1
        movdqa      xmm2,                   [rdi+2*rax]       ; q3
        movdqa      xmm1,                   [rsi+2*rax]       ; q2
        movdqa      xmm4,                   [rsi+rax]         ; q1
        movdqa      xmm5,                   [rsi]             ; q0
        neg         rax                     ; negate pitch to deal with above border
%else
        movlps      xmm2,                   [rsi + rcx*2]     ; q3
        movlps      xmm1,                   [rsi + rcx]       ; q2
        movlps      xmm4,                   [rsi]             ; q1
        movlps      xmm5,                   [rsi + rax]       ; q0

        movhps      xmm2,                   [rdi + rcx*2]
        movhps      xmm1,                   [rdi + rcx]
        movhps      xmm4,                   [rdi]
        movhps      xmm5,                   [rdi + rax]

        lea         rsi,                    [rsi + rax*4]
        lea         rdi,                    [rdi + rax*4]

        movdqa      XMMWORD PTR [rsp],      xmm1              ; store q2
        movdqa      XMMWORD PTR [rsp + 16], xmm4              ; store q1
%endif

        movdqa      xmm6,                   xmm1              ; q2
        movdqa      xmm3,                   xmm4              ; q1

        psubusb     xmm1,                   xmm2              ; q2-=q3
        psubusb     xmm2,                   xmm6              ; q3-=q2

        psubusb     xmm4,                   xmm6              ; q1-=q2
        psubusb     xmm6,                   xmm3              ; q2-=q1

        por         xmm4,                   xmm6              ; abs(q2-q1)
        por         xmm1,                   xmm2              ; abs(q3-q2)

        movdqa      xmm0,                   xmm5              ; q0
        pmaxub      xmm1,                   xmm4

        psubusb     xmm5,                   xmm3              ; q0-=q1
        psubusb     xmm3,                   xmm0              ; q1-=q0

        por         xmm5,                   xmm3              ; abs(q0-q1)
        movdqa      t0,                     xmm5              ; save to t0

        pmaxub      xmm1,                   xmm5

%if %1
        movdqa      xmm2,                   [rsi+4*rax]       ; p3
        movdqa      xmm4,                   [rdi+4*rax]       ; p2
        movdqa      xmm6,                   [rsi+2*rax]       ; p1
%else
        movlps      xmm2,                   [rsi + rax]       ; p3
        movlps      xmm4,                   [rsi]             ; p2
        movlps      xmm6,                   [rsi + rcx]       ; p1

        movhps      xmm2,                   [rdi + rax]
        movhps      xmm4,                   [rdi]
        movhps      xmm6,                   [rdi + rcx]

        movdqa      XMMWORD PTR [rsp + 32], xmm4              ; store p2
        movdqa      XMMWORD PTR [rsp + 48], xmm6              ; store p1
%endif

        movdqa      xmm5,                   xmm4              ; p2
        movdqa      xmm3,                   xmm6              ; p1

        psubusb     xmm4,                   xmm2              ; p2-=p3
        psubusb     xmm2,                   xmm5              ; p3-=p2

        psubusb     xmm3,                   xmm5              ; p1-=p2
        pmaxub      xmm1,                   xmm4              ; abs(p3 - p2)

        psubusb     xmm5,                   xmm6              ; p2-=p1
        pmaxub      xmm1,                   xmm2              ; abs(p3 - p2)

        pmaxub      xmm1,                   xmm5              ; abs(p2 - p1)
        movdqa      xmm2,                   xmm6              ; p1

        pmaxub      xmm1,                   xmm3              ; abs(p2 - p1)
%if %1
        movdqa      xmm4,                   [rsi+rax]         ; p0
        movdqa      xmm3,                   [rdi]             ; q1
%else
        movlps      xmm4,                   [rsi + rcx*2]     ; p0
        movhps      xmm4,                   [rdi + rcx*2]
        movdqa      xmm3,                   q1                ; q1
%endif

        movdqa      xmm5,                   xmm4              ; p0
        psubusb     xmm4,                   xmm6              ; p0-=p1

        psubusb     xmm6,                   xmm5              ; p1-=p0

        por         xmm6,                   xmm4              ; abs(p1 - p0)
        mov         rdx,                    arg(2)            ; get blimit

        movdqa        t1,                   xmm6              ; save to t1

        movdqa      xmm4,                   xmm3              ; q1
        pmaxub      xmm1,                   xmm6

        psubusb     xmm3,                   xmm2              ; q1-=p1
        psubusb     xmm2,                   xmm4              ; p1-=q1

        psubusb     xmm1,                   xmm7
        por         xmm2,                   xmm3              ; abs(p1-q1)

        movdqa      xmm7,                   XMMWORD PTR [rdx] ; blimit

        movdqa      xmm3,                   xmm0              ; q0
        pand        xmm2,                   [GLOBAL(tfe)]     ; set lsb of each byte to zero

        mov         rdx,                    arg(4)            ; hev get thresh

        movdqa      xmm6,                   xmm5              ; p0
        psrlw       xmm2,                   1                 ; abs(p1-q1)/2

        psubusb     xmm5,                   xmm3              ; p0-=q0

        psubusb     xmm3,                   xmm6              ; q0-=p0
        por         xmm5,                   xmm3              ; abs(p0 - q0)

        paddusb     xmm5,                   xmm5              ; abs(p0-q0)*2

        movdqa      xmm4,                   t0                ; hev get abs (q1 - q0)

        movdqa      xmm3,                   t1                ; get abs (p1 - p0)

        paddusb     xmm5,                   xmm2              ; abs (p0 - q0) *2 + abs(p1-q1)/2

        movdqa      xmm2,                   XMMWORD PTR [rdx] ; hev

        psubusb     xmm5,                   xmm7              ; abs (p0 - q0) *2 + abs(p1-q1)/2  > blimit
        psubusb     xmm4,                   xmm2              ; hev

        psubusb     xmm3,                   xmm2              ; hev
        por         xmm1,                   xmm5

        pxor        xmm7,                   xmm7
        paddb       xmm4,                   xmm3              ; hev abs(q1 - q0) > thresh || abs(p1 - p0) > thresh

        pcmpeqb     xmm4,                   xmm5              ; hev
        pcmpeqb     xmm3,                   xmm3              ; hev

        pcmpeqb     xmm1,                   xmm7              ; mask xmm1
        pxor        xmm4,                   xmm3              ; hev
%endmacro

%macro B_FILTER 1
%if %1 == 0
        movdqa      xmm2,                   p1                ; p1
        movdqa      xmm7,                   q1                ; q1
%elif %1 == 1
        movdqa      xmm2,                   [rsi+2*rax]       ; p1
        movdqa      xmm7,                   [rdi]             ; q1
%elif %1 == 2
        lea         rdx,                    srct

        movdqa      xmm2,                   [rdx]             ; p1
        movdqa      xmm7,                   [rdx+48]          ; q1
        movdqa      xmm6,                   [rdx+16]          ; p0
        movdqa      xmm0,                   [rdx+32]          ; q0
%endif

        pxor        xmm2,                   [GLOBAL(t80)]     ; p1 offset to convert to signed values
        pxor        xmm7,                   [GLOBAL(t80)]     ; q1 offset to convert to signed values

        psubsb      xmm2,                   xmm7              ; p1 - q1
        pxor        xmm6,                   [GLOBAL(t80)]     ; offset to convert to signed values

        pand        xmm2,                   xmm4              ; high var mask (hvm)(p1 - q1)
        pxor        xmm0,                   [GLOBAL(t80)]     ; offset to convert to signed values

        movdqa      xmm3,                   xmm0              ; q0
        psubsb      xmm0,                   xmm6              ; q0 - p0

        paddsb      xmm2,                   xmm0              ; 1 * (q0 - p0) + hvm(p1 - q1)

        paddsb      xmm2,                   xmm0              ; 2 * (q0 - p0) + hvm(p1 - q1)

        paddsb      xmm2,                   xmm0              ; 3 * (q0 - p0) + hvm(p1 - q1)

        pand        xmm1,                   xmm2              ; mask filter values we don't care about

        movdqa      xmm2,                   xmm1

        paddsb      xmm1,                   [GLOBAL(t4)]      ; 3* (q0 - p0) + hvm(p1 - q1) + 4
        paddsb      xmm2,                   [GLOBAL(t3)]      ; 3* (q0 - p0) + hvm(p1 - q1) + 3

        punpckhbw   xmm5,                   xmm2              ; axbxcxdx
        punpcklbw   xmm2,                   xmm2              ; exfxgxhx

        punpcklbw   xmm0,                   xmm1              ; exfxgxhx
        psraw       xmm5,                   11                ; sign extended shift right by 3

        punpckhbw   xmm1,                   xmm1              ; axbxcxdx
        psraw       xmm2,                   11                ; sign extended shift right by 3

        packsswb    xmm2,                   xmm5              ; (3* (q0 - p0) + hvm(p1 - q1) + 3) >> 3;
        psraw       xmm0,                   11                ; sign extended shift right by 3

        psraw       xmm1,                   11                ; sign extended shift right by 3
        movdqa      xmm5,                   xmm0              ; save results

        packsswb    xmm0,                   xmm1              ; (3* (q0 - p0) + hvm(p1 - q1) + 4) >>3
        paddsw      xmm5,                   [GLOBAL(ones)]

        paddsw      xmm1,                   [GLOBAL(ones)]
        psraw       xmm5,                   1                 ; partial shifted one more time for 2nd tap

        psraw       xmm1,                   1                 ; partial shifted one more time for 2nd tap

        paddsb      xmm6,                   xmm2              ; p0+= p0 add
        packsswb    xmm5,                   xmm1              ; (3* (q0 - p0) + hvm(p1 - q1) + 4) >>4

%if %1 == 0
        movdqa      xmm1,                   p1                ; p1
%elif %1 == 1
        movdqa      xmm1,                   [rsi+2*rax]       ; p1
%elif %1 == 2
        movdqa      xmm1,                   [rdx]             ; p1
%endif
        pandn       xmm4,                   xmm5              ; high edge variance additive
        pxor        xmm6,                   [GLOBAL(t80)]     ; unoffset

        pxor        xmm1,                   [GLOBAL(t80)]     ; reoffset
        psubsb      xmm3,                   xmm0              ; q0-= q0 add

        paddsb      xmm1,                   xmm4              ; p1+= p1 add
        pxor        xmm3,                   [GLOBAL(t80)]     ; unoffset

        pxor        xmm1,                   [GLOBAL(t80)]     ; unoffset
        psubsb      xmm7,                   xmm4              ; q1-= q1 add

        pxor        xmm7,                   [GLOBAL(t80)]     ; unoffset
%if %1 == 0
        lea         rsi,                    [rsi + rcx*2]
        lea         rdi,                    [rdi + rcx*2]
        movq        MMWORD PTR [rsi],       xmm6              ; p0
        movhps      MMWORD PTR [rdi],       xmm6
        movq        MMWORD PTR [rsi + rax], xmm1              ; p1
        movhps      MMWORD PTR [rdi + rax], xmm1
        movq        MMWORD PTR [rsi + rcx], xmm3              ; q0
        movhps      MMWORD PTR [rdi + rcx], xmm3
        movq        MMWORD PTR [rsi + rcx*2],xmm7             ; q1
        movhps      MMWORD PTR [rdi + rcx*2],xmm7
%elif %1 == 1
        movdqa      [rsi+rax],              xmm6              ; write back
        movdqa      [rsi+2*rax],            xmm1              ; write back
        movdqa      [rsi],                  xmm3              ; write back
        movdqa      [rdi],                  xmm7              ; write back
%endif

%endmacro


;void vp9_loop_filter_horizontal_edge_sse2
;(
;    unsigned char *src_ptr,
;    int            src_pixel_step,
;    const char    *blimit,
;    const char    *limit,
;    const char    *thresh,
;    int            count
;)
global sym(vp9_loop_filter_horizontal_edge_sse2) PRIVATE
sym(vp9_loop_filter_horizontal_edge_sse2):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    SAVE_XMM 7
    GET_GOT     rbx
    push        rsi
    push        rdi
    ; end prolog

    ALIGN_STACK 16, rax
    sub         rsp, 32     ; reserve 32 bytes
    %define t0 [rsp + 0]    ;__declspec(align(16)) char t0[16];
    %define t1 [rsp + 16]   ;__declspec(align(16)) char t1[16];

        mov         rsi,                    arg(0)           ;src_ptr
        movsxd      rax,                    dword ptr arg(1) ;src_pixel_step

        mov         rdx,                    arg(3)           ;limit
        movdqa      xmm7,                   XMMWORD PTR [rdx]

        lea         rdi,                    [rsi+rax]        ; rdi points to row +1 for indirect addressing

        ; calculate breakout conditions and high edge variance
        LFH_FILTER_AND_HEV_MASK 1
        ; filter and write back the result
        B_FILTER 1

    add rsp, 32
    pop rsp
    ; begin epilog
    pop rdi
    pop rsi
    RESTORE_GOT
    RESTORE_XMM
    UNSHADOW_ARGS
    pop         rbp
    ret


;void vp9_loop_filter_horizontal_edge_uv_sse2
;(
;    unsigned char *src_ptr,
;    int            src_pixel_step,
;    const char    *blimit,
;    const char    *limit,
;    const char    *thresh,
;    int            count
;)
global sym(vp9_loop_filter_horizontal_edge_uv_sse2) PRIVATE
sym(vp9_loop_filter_horizontal_edge_uv_sse2):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    SAVE_XMM 7
    GET_GOT     rbx
    push        rsi
    push        rdi
    ; end prolog

    ALIGN_STACK 16, rax
    sub         rsp, 96       ; reserve 96 bytes
    %define q2  [rsp + 0]     ;__declspec(align(16)) char q2[16];
    %define q1  [rsp + 16]    ;__declspec(align(16)) char q1[16];
    %define p2  [rsp + 32]    ;__declspec(align(16)) char p2[16];
    %define p1  [rsp + 48]    ;__declspec(align(16)) char p1[16];
    %define t0  [rsp + 64]    ;__declspec(align(16)) char t0[16];
    %define t1  [rsp + 80]    ;__declspec(align(16)) char t1[16];

        mov         rsi,                    arg(0)             ; u
        mov         rdi,                    arg(5)             ; v
        movsxd      rax,                    dword ptr arg(1)   ; src_pixel_step
        mov         rcx,                    rax
        neg         rax                     ; negate pitch to deal with above border

        mov         rdx,                    arg(3)             ;limit
        movdqa      xmm7,                   XMMWORD PTR [rdx]

        lea         rsi,                    [rsi + rcx]
        lea         rdi,                    [rdi + rcx]

        ; calculate breakout conditions and high edge variance
        LFH_FILTER_AND_HEV_MASK 0
        ; filter and write back the result
        B_FILTER 0

    add rsp, 96
    pop rsp
    ; begin epilog
    pop rdi
    pop rsi
    RESTORE_GOT
    RESTORE_XMM
    UNSHADOW_ARGS
    pop         rbp
    ret


%macro TRANSPOSE_16X8 2
        movq        xmm4,               QWORD PTR [rsi]        ; xx xx xx xx xx xx xx xx 07 06 05 04 03 02 01 00
        movq        xmm1,               QWORD PTR [rdi]        ; xx xx xx xx xx xx xx xx 17 16 15 14 13 12 11 10
        movq        xmm0,               QWORD PTR [rsi+2*rax]  ; xx xx xx xx xx xx xx xx 27 26 25 24 23 22 21 20
        movq        xmm7,               QWORD PTR [rdi+2*rax]  ; xx xx xx xx xx xx xx xx 37 36 35 34 33 32 31 30
        movq        xmm5,               QWORD PTR [rsi+4*rax]  ; xx xx xx xx xx xx xx xx 47 46 45 44 43 42 41 40
        movq        xmm2,               QWORD PTR [rdi+4*rax]  ; xx xx xx xx xx xx xx xx 57 56 55 54 53 52 51 50

        punpcklbw   xmm4,               xmm1            ; 17 07 16 06 15 05 14 04 13 03 12 02 11 01 10 00

        movq        xmm1,               QWORD PTR [rdi+2*rcx]  ; xx xx xx xx xx xx xx xx 77 76 75 74 73 72 71 70

        movdqa      xmm3,               xmm4            ; 17 07 16 06 15 05 14 04 13 03 12 02 11 01 10 00
        punpcklbw   xmm0,               xmm7            ; 37 27 36 36 35 25 34 24 33 23 32 22 31 21 30 20

        movq        xmm7,               QWORD PTR [rsi+2*rcx]  ; xx xx xx xx xx xx xx xx 67 66 65 64 63 62 61 60

        punpcklbw   xmm5,               xmm2            ; 57 47 56 46 55 45 54 44 53 43 52 42 51 41 50 40
%if %1
        lea         rsi,                [rsi+rax*8]
%else
        mov         rsi,                arg(5)          ; v_ptr
%endif

        movdqa      xmm6,               xmm5            ; 57 47 56 46 55 45 54 44 53 43 52 42 51 41 50 40
        punpcklbw   xmm7,               xmm1            ; 77 67 76 66 75 65 74 64 73 63 72 62 71 61 70 60

        punpcklwd   xmm5,               xmm7            ; 73 63 53 43 72 62 52 42 71 61 51 41 70 60 50 40

        punpckhwd   xmm6,               xmm7            ; 77 67 57 47 76 66 56 46 75 65 55 45 74 64 54 44
%if %1
        lea         rdi,                [rdi+rax*8]
%else
        lea         rsi,                [rsi - 4]
%endif

        punpcklwd   xmm3,               xmm0            ; 33 23 13 03 32 22 12 02 31 21 11 01 30 20 10 00
%if %1
        lea         rdx,                srct
%else
        lea         rdi,                [rsi + rax]     ; rdi points to row +1 for indirect addressing
%endif

        movdqa      xmm2,               xmm3            ; 33 23 13 03 32 22 12 02 31 21 11 01 30 20 10 00
        punpckhwd   xmm4,               xmm0            ; 37 27 17 07 36 26 16 06 35 25 15 05 34 24 14 04

        movdqa      xmm7,               xmm4            ; 37 27 17 07 36 26 16 06 35 25 15 05 34 24 14 04
        punpckhdq   xmm3,               xmm5            ; 73 63 53 43 33 23 13 03 72 62 52 42 32 22 12 02

        punpckhdq   xmm7,               xmm6            ; 77 67 57 47 37 27 17 07 76 66 56 46 36 26 16 06

        punpckldq   xmm4,               xmm6            ; 75 65 55 45 35 25 15 05 74 64 54 44 34 24 14 04

        punpckldq   xmm2,               xmm5            ; 71 61 51 41 31 21 11 01 70 60 50 40 30 20 10 00

        movdqa      t0,                 xmm2            ; save to free XMM2
        movq        xmm2,               QWORD PTR [rsi]       ; xx xx xx xx xx xx xx xx 87 86 85 84 83 82 81 80
        movq        xmm6,               QWORD PTR [rdi]       ; xx xx xx xx xx xx xx xx 97 96 95 94 93 92 91 90
        movq        xmm0,               QWORD PTR [rsi+2*rax] ; xx xx xx xx xx xx xx xx a7 a6 a5 a4 a3 a2 a1 a0
        movq        xmm5,               QWORD PTR [rdi+2*rax] ; xx xx xx xx xx xx xx xx b7 b6 b5 b4 b3 b2 b1 b0
        movq        xmm1,               QWORD PTR [rsi+4*rax] ; xx xx xx xx xx xx xx xx c7 c6 c5 c4 c3 c2 c1 c0

        punpcklbw   xmm2,               xmm6            ; 97 87 96 86 95 85 94 84 93 83 92 82 91 81 90 80

        movq        xmm6,               QWORD PTR [rdi+4*rax] ; xx xx xx xx xx xx xx xx d7 d6 d5 d4 d3 d2 d1 d0

        punpcklbw   xmm0,               xmm5                  ; b7 a7 b6 a6 b5 a5 b4 a4 b3 a3 b2 a2 b1 a1 b0 a0

        movq        xmm5,               QWORD PTR [rsi+2*rcx] ; xx xx xx xx xx xx xx xx e7 e6 e5 e4 e3 e2 e1 e0

        punpcklbw   xmm1,               xmm6            ; d7 c7 d6 c6 d5 c5 d4 c4 d3 c3 d2 c2 d1 e1 d0 c0

        movq        xmm6,               QWORD PTR [rdi+2*rcx] ; xx xx xx xx xx xx xx xx f7 f6 f5 f4 f3 f2 f1 f0

        punpcklbw   xmm5,               xmm6            ; f7 e7 f6 e6 f5 e5 f4 e4 f3 e3 f2 e2 f1 e1 f0 e0

        movdqa      xmm6,               xmm1            ;
        punpckhwd   xmm6,               xmm5            ; f7 e7 d7 c7 f6 e6 d6 c6 f5 e5 d5 c5 f4 e4 d4 c4

        punpcklwd   xmm1,               xmm5            ; f3 e3 d3 c3 f2 e2 d2 c2 f1 e1 d1 c1 f0 e0 d0 c0
        movdqa      xmm5,               xmm2            ; 97 87 96 86 95 85 94 84 93 83 92 82 91 81 90 80

        punpcklwd   xmm5,               xmm0            ; b3 a3 93 83 b2 a2 92 82 b1 a1 91 81 b0 a0 90 80

        punpckhwd   xmm2,               xmm0            ; b7 a7 97 87 b6 a6 96 86 b5 a5 95 85 b4 a4 94 84

        movdqa      xmm0,               xmm5
        punpckldq   xmm0,               xmm1            ; f1 e1 d1 c1 b1 a1 91 81 f0 e0 d0 c0 b0 a0 90 80

        punpckhdq   xmm5,               xmm1            ; f3 e3 d3 c3 b3 a3 93 83 f2 e2 d2 c2 b2 a2 92 82
        movdqa      xmm1,               xmm2            ; b7 a7 97 87 b6 a6 96 86 b5 a5 95 85 b4 a4 94 84

        punpckldq   xmm1,               xmm6            ; f5 e5 d5 c5 b5 a5 95 85 f4 e4 d4 c4 b4 a4 94 84

        punpckhdq   xmm2,               xmm6            ; f7 e7 d7 c7 b7 a7 97 87 f6 e6 d6 c6 b6 a6 96 86
        movdqa      xmm6,               xmm7            ; 77 67 57 47 37 27 17 07 76 66 56 46 36 26 16 06

        punpcklqdq  xmm6,               xmm2            ; f6 e6 d6 c6 b6 a6 96 86 76 66 56 46 36 26 16 06

        punpckhqdq  xmm7,               xmm2            ; f7 e7 d7 c7 b7 a7 97 87 77 67 57 47 37 27 17 07
%if %2
        movdqa      xmm2,               xmm3            ; 73 63 53 43 33 23 13 03 72 62 52 42 32 22 12 02
        punpcklqdq  xmm2,               xmm5            ; f2 e2 d2 c2 b2 a2 92 82 72 62 52 42 32 22 12 02

        punpckhqdq  xmm3,               xmm5            ; f3 e3 d3 c3 b3 a3 93 83 73 63 53 43 33 23 13 03

        movdqa      [rdx],              xmm2            ; save 2

        movdqa      xmm5,               xmm4            ; 75 65 55 45 35 25 15 05 74 64 54 44 34 24 14 04
        punpcklqdq  xmm4,               xmm1            ; f4 e4 d4 c4 b4 a4 94 84 74 64 54 44 34 24 14 04

        movdqa      [rdx+16],           xmm3            ; save 3

        punpckhqdq  xmm5,               xmm1            ; f5 e5 d5 c5 b5 a5 95 85 75 65 55 45 35 25 15 05

        movdqa      [rdx+32],           xmm4            ; save 4
        movdqa      [rdx+48],           xmm5            ; save 5
        movdqa      xmm1,               t0              ; get

        movdqa      xmm2,               xmm1            ;
        punpckhqdq  xmm1,               xmm0            ; f1 e1 d1 c1 b1 a1 91 81 71 61 51 41 31 21 11 01

        punpcklqdq  xmm2,               xmm0            ; f0 e0 d0 c0 b0 a0 90 80 70 60 50 40 30 20 10 00
%else
        movdqa      [rdx+112],          xmm7            ; save 7

        movdqa      [rdx+96],           xmm6            ; save 6

        movdqa      xmm2,               xmm3            ; 73 63 53 43 33 23 13 03 72 62 52 42 32 22 12 02
        punpckhqdq  xmm3,               xmm5            ; f3 e3 d3 c3 b3 a3 93 83 73 63 53 43 33 23 13 03

        punpcklqdq  xmm2,               xmm5            ; f2 e2 d2 c2 b2 a2 92 82 72 62 52 42 32 22 12 02

        movdqa      [rdx+32],           xmm2            ; save 2

        movdqa      xmm5,               xmm4            ; 75 65 55 45 35 25 15 05 74 64 54 44 34 24 14 04
        punpcklqdq  xmm4,               xmm1            ; f4 e4 d4 c4 b4 a4 94 84 74 64 54 44 34 24 14 04

        movdqa      [rdx+48],           xmm3            ; save 3

        punpckhqdq  xmm5,               xmm1            ; f5 e5 d5 c5 b5 a5 95 85 75 65 55 45 35 25 15 05

        movdqa      [rdx+64],           xmm4            ; save 4
        movdqa      [rdx+80],           xmm5            ; save 5
        movdqa      xmm1,               t0              ; get

        movdqa      xmm2,               xmm1
        punpckhqdq  xmm1,               xmm0            ; f1 e1 d1 c1 b1 a1 91 81 71 61 51 41 31 21 11 01

        punpcklqdq  xmm2,               xmm0            ; f0 e0 d0 c0 b0 a0 90 80 70 60 50 40 30 20 10 00

        movdqa      [rdx+16],           xmm1

        movdqa      [rdx],              xmm2
%endif
%endmacro

%macro LFV_FILTER_MASK_HEV_MASK 1
        movdqa      xmm0,               xmm6            ; q2
        psubusb     xmm0,               xmm7            ; q2-q3

        psubusb     xmm7,               xmm6            ; q3-q2
        movdqa      xmm4,               xmm5            ; q1

        por         xmm7,               xmm0            ; abs (q3-q2)
        psubusb     xmm4,               xmm6            ; q1-q2

        movdqa      xmm0,               xmm1
        psubusb     xmm6,               xmm5            ; q2-q1

        por         xmm6,               xmm4            ; abs (q2-q1)
        psubusb     xmm0,               xmm2            ; p2 - p3;

        psubusb     xmm2,               xmm1            ; p3 - p2;
        por         xmm0,               xmm2            ; abs(p2-p3)
%if %1
        movdqa      xmm2,               [rdx]           ; p1
%else
        movdqa      xmm2,               [rdx+32]        ; p1
%endif
        movdqa      xmm5,               xmm2            ; p1
        pmaxub      xmm0,               xmm7

        psubusb     xmm5,               xmm1            ; p1-p2
        psubusb     xmm1,               xmm2            ; p2-p1

        movdqa      xmm7,               xmm3            ; p0
        psubusb     xmm7,               xmm2            ; p0-p1

        por         xmm1,               xmm5            ; abs(p2-p1)
        pmaxub      xmm0,               xmm6

        pmaxub      xmm0,               xmm1
        movdqa      xmm1,               xmm2            ; p1

        psubusb     xmm2,               xmm3            ; p1-p0
        lea         rdx,                srct

        por         xmm2,               xmm7            ; abs(p1-p0)

        movdqa      t0,                 xmm2            ; save abs(p1-p0)

        pmaxub      xmm0,               xmm2

%if %1
        movdqa      xmm5,               [rdx+32]        ; q0
        movdqa      xmm7,               [rdx+48]        ; q1
%else
        movdqa      xmm5,               [rdx+64]        ; q0
        movdqa      xmm7,               [rdx+80]        ; q1
%endif
        mov         rdx,                arg(3)          ; limit

        movdqa      xmm6,               xmm5            ; q0
        movdqa      xmm2,               xmm7            ; q1

        psubusb     xmm5,               xmm7            ; q0-q1
        psubusb     xmm7,               xmm6            ; q1-q0

        por         xmm7,               xmm5            ; abs(q1-q0)

        movdqa      t1,                 xmm7            ; save abs(q1-q0)

        movdqa      xmm4,               XMMWORD PTR [rdx]; limit

        pmaxub      xmm0,               xmm7
        mov         rdx,                arg(2)          ; blimit

        psubusb     xmm0,               xmm4
        movdqa      xmm5,               xmm2            ; q1

        psubusb     xmm5,               xmm1            ; q1-=p1
        psubusb     xmm1,               xmm2            ; p1-=q1

        por         xmm5,               xmm1            ; abs(p1-q1)
        movdqa      xmm1,               xmm3            ; p0

        pand        xmm5,               [GLOBAL(tfe)]   ; set lsb of each byte to zero
        psubusb     xmm1,               xmm6            ; p0-q0

        psrlw       xmm5,               1               ; abs(p1-q1)/2
        psubusb     xmm6,               xmm3            ; q0-p0

        movdqa      xmm4,               XMMWORD PTR [rdx]; blimit

        mov         rdx,                arg(4)          ; get thresh

        por         xmm1,               xmm6            ; abs(q0-p0)

        movdqa      xmm6,               t0              ; get abs (q1 - q0)

        paddusb     xmm1,               xmm1            ; abs(q0-p0)*2

        movdqa      xmm3,               t1              ; get abs (p1 - p0)

        movdqa      xmm7,               XMMWORD PTR [rdx]

        paddusb     xmm1,               xmm5            ; abs (p0 - q0) *2 + abs(p1-q1)/2
        psubusb     xmm6,               xmm7            ; abs(q1 - q0) > thresh

        psubusb     xmm3,               xmm7            ; abs(p1 - p0)> thresh

        psubusb     xmm1,               xmm4            ; abs (p0 - q0) *2 + abs(p1-q1)/2  > blimit
        por         xmm6,               xmm3            ; abs(q1 - q0) > thresh || abs(p1 - p0) > thresh

        por         xmm1,               xmm0            ; mask
        pcmpeqb     xmm6,               xmm0

        pxor        xmm0,               xmm0
        pcmpeqb     xmm4,               xmm4

        pcmpeqb     xmm1,               xmm0
        pxor        xmm4,               xmm6
%endmacro

%macro BV_TRANSPOSE 0
        ; xmm1 =    f2 e2 d2 c2 b2 a2 92 82 72 62 52 42 32 22 12 02
        ; xmm6 =    f3 e3 d3 c3 b3 a3 93 83 73 63 53 43 33 23 13 03
        ; xmm3 =    f4 e4 d4 c4 b4 a4 94 84 74 64 54 44 34 24 14 04
        ; xmm7 =    f5 e5 d5 c5 b5 a5 95 85 75 65 55 45 35 25 15 05
        movdqa      xmm2,               xmm1            ; f2 e2 d2 c2 b2 a2 92 82 72 62 52 42 32 22 12 02
        punpcklbw   xmm2,               xmm6            ; 73 72 63 62 53 52 43 42 33 32 23 22 13 12 03 02

        movdqa      xmm4,               xmm3            ; f4 e4 d4 c4 b4 a4 94 84 74 64 54 44 34 24 14 04
        punpckhbw   xmm1,               xmm6            ; f3 f2 e3 e2 d3 d2 c3 c2 b3 b2 a3 a2 93 92 83 82

        punpcklbw   xmm4,               xmm7            ; 75 74 65 64 55 54 45 44 35 34 25 24 15 14 05 04

        punpckhbw   xmm3,               xmm7            ; f5 f4 e5 e4 d5 d4 c5 c4 b5 b4 a5 a4 95 94 85 84

        movdqa      xmm6,               xmm2            ; 73 72 63 62 53 52 43 42 33 32 23 22 13 12 03 02
        punpcklwd   xmm2,               xmm4            ; 35 34 33 32 25 24 23 22 15 14 13 12 05 04 03 02

        punpckhwd   xmm6,               xmm4            ; 75 74 73 72 65 64 63 62 55 54 53 52 45 44 43 42
        movdqa      xmm5,               xmm1            ; f3 f2 e3 e2 d3 d2 c3 c2 b3 b2 a3 a2 93 92 83 82

        punpcklwd   xmm1,               xmm3            ; b5 b4 b3 b2 a5 a4 a3 a2 95 94 93 92 85 84 83 82

        punpckhwd   xmm5,               xmm3            ; f5 f4 f3 f2 e5 e4 e3 e2 d5 d4 d3 d2 c5 c4 c3 c2
        ; xmm2 = 35 34 33 32 25 24 23 22 15 14 13 12 05 04 03 02
        ; xmm6 = 75 74 73 72 65 64 63 62 55 54 53 52 45 44 43 42
        ; xmm1 = b5 b4 b3 b2 a5 a4 a3 a2 95 94 93 92 85 84 83 82
        ; xmm5 = f5 f4 f3 f2 e5 e4 e3 e2 d5 d4 d3 d2 c5 c4 c3 c2
%endmacro

%macro BV_WRITEBACK 2
        movd        [rsi+2],            %1
        psrldq      %1,                 4

        movd        [rdi+2],            %1
        psrldq      %1,                 4

        movd        [rsi+2*rax+2],      %1
        psrldq      %1,                 4

        movd        [rdi+2*rax+2],      %1

        movd        [rsi+4*rax+2],      %2
        psrldq      %2,                 4

        movd        [rdi+4*rax+2],      %2
        psrldq      %2,                 4

        movd        [rsi+2*rcx+2],      %2
        psrldq      %2,                 4

        movd        [rdi+2*rcx+2],      %2
%endmacro


;void vp9_loop_filter_vertical_edge_sse2
;(
;    unsigned char *src_ptr,
;    int            src_pixel_step,
;    const char    *blimit,
;    const char    *limit,
;    const char    *thresh,
;    int            count
;)
global sym(vp9_loop_filter_vertical_edge_sse2) PRIVATE
sym(vp9_loop_filter_vertical_edge_sse2):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    SAVE_XMM 7
    GET_GOT     rbx
    push        rsi
    push        rdi
    ; end prolog

    ALIGN_STACK 16, rax
    sub             rsp, 96      ; reserve 96 bytes
    %define t0      [rsp + 0]    ;__declspec(align(16)) char t0[16];
    %define t1      [rsp + 16]   ;__declspec(align(16)) char t1[16];
    %define srct    [rsp + 32]   ;__declspec(align(16)) char srct[64];

        mov         rsi,        arg(0)                  ; src_ptr
        movsxd      rax,        dword ptr arg(1)        ; src_pixel_step

        lea         rsi,        [rsi - 4]
        lea         rdi,        [rsi + rax]             ; rdi points to row +1 for indirect addressing
        lea         rcx,        [rax*2+rax]

        ;transpose 16x8 to 8x16, and store the 8-line result on stack.
        TRANSPOSE_16X8 1, 1

        ; calculate filter mask and high edge variance
        LFV_FILTER_MASK_HEV_MASK 1

        ; start work on filters
        B_FILTER 2

        ; tranpose and write back - only work on q1, q0, p0, p1
        BV_TRANSPOSE
        ; store 16-line result

        lea         rdx,        [rax]
        neg         rdx

        BV_WRITEBACK xmm1, xmm5

        lea         rsi,        [rsi+rdx*8]
        lea         rdi,        [rdi+rdx*8]
        BV_WRITEBACK xmm2, xmm6

    add rsp, 96
    pop rsp
    ; begin epilog
    pop rdi
    pop rsi
    RESTORE_GOT
    RESTORE_XMM
    UNSHADOW_ARGS
    pop         rbp
    ret


;void vp9_loop_filter_vertical_edge_uv_sse2
;(
;    unsigned char *u,
;    int            src_pixel_step,
;    const char    *blimit,
;    const char    *limit,
;    const char    *thresh,
;    unsigned char *v
;)
global sym(vp9_loop_filter_vertical_edge_uv_sse2) PRIVATE
sym(vp9_loop_filter_vertical_edge_uv_sse2):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 6
    SAVE_XMM 7
    GET_GOT     rbx
    push        rsi
    push        rdi
    ; end prolog

    ALIGN_STACK 16, rax
    sub             rsp, 96      ; reserve 96 bytes
    %define t0      [rsp + 0]    ;__declspec(align(16)) char t0[16];
    %define t1      [rsp + 16]   ;__declspec(align(16)) char t1[16];
    %define srct    [rsp + 32]   ;__declspec(align(16)) char srct[64];

        mov         rsi,        arg(0)                  ; u_ptr
        movsxd      rax,        dword ptr arg(1)        ; src_pixel_step

        lea         rsi,        [rsi - 4]
        lea         rdi,        [rsi + rax]             ; rdi points to row +1 for indirect addressing
        lea         rcx,        [rax+2*rax]

        lea         rdx,        srct

        ;transpose 16x8 to 8x16, and store the 8-line result on stack.
        TRANSPOSE_16X8 0, 1

        ; calculate filter mask and high edge variance
        LFV_FILTER_MASK_HEV_MASK 1

        ; start work on filters
        B_FILTER 2

        ; tranpose and write back - only work on q1, q0, p0, p1
        BV_TRANSPOSE

        lea         rdi,        [rsi + rax]             ; rdi points to row +1 for indirect addressing

        ; store 16-line result
        BV_WRITEBACK xmm1, xmm5

        mov         rsi,        arg(0)                  ; u_ptr
        lea         rsi,        [rsi - 4]
        lea         rdi,        [rsi + rax]             ; rdi points to row +1 for indirect addressing
        BV_WRITEBACK xmm2, xmm6

    add rsp, 96
    pop rsp
    ; begin epilog
    pop rdi
    pop rsi
    RESTORE_GOT
    RESTORE_XMM
    UNSHADOW_ARGS
    pop         rbp
    ret

;void vp9_loop_filter_simple_horizontal_edge_sse2
;(
;    unsigned char *src_ptr,
;    int  src_pixel_step,
;    const char *blimit,
;)
global sym(vp9_loop_filter_simple_horizontal_edge_sse2) PRIVATE
sym(vp9_loop_filter_simple_horizontal_edge_sse2):
    push        rbp
    mov         rbp, rsp
    SHADOW_ARGS_TO_STACK 3
    SAVE_XMM 7
    GET_GOT     rbx
    push        rsi
    push        rdi
    ; end prolog

        mov         rsi, arg(0)             ;src_ptr
        movsxd      rax, dword ptr arg(1)   ;src_pixel_step     ; destination pitch?
        mov         rdx, arg(2)             ;blimit
        movdqa      xmm3, XMMWORD PTR [rdx]

        mov         rdi, rsi                ; rdi points to row +1 for indirect addressing
        add         rdi, rax
        neg         rax

        ; calculate mask
        movdqa      xmm1, [rsi+2*rax]       ; p1
        movdqa      xmm0, [rdi]             ; q1
        movdqa      xmm2, xmm1
        movdqa      xmm7, xmm0
        movdqa      xmm4, xmm0
        psubusb     xmm0, xmm1              ; q1-=p1
        psubusb     xmm1, xmm4              ; p1-=q1
        por         xmm1, xmm0              ; abs(p1-q1)
        pand        xmm1, [GLOBAL(tfe)]     ; set lsb of each byte to zero
        psrlw       xmm1, 1                 ; abs(p1-q1)/2

        movdqa      xmm5, [rsi+rax]         ; p0
        movdqa      xmm4, [rsi]             ; q0
        movdqa      xmm0, xmm4              ; q0
        movdqa      xmm6, xmm5              ; p0
        psubusb     xmm5, xmm4              ; p0-=q0
        psubusb     xmm4, xmm6              ; q0-=p0
        por         xmm5, xmm4              ; abs(p0 - q0)
        paddusb     xmm5, xmm5              ; abs(p0-q0)*2
        paddusb     xmm5, xmm1              ; abs (p0 - q0) *2 + abs(p1-q1)/2

        psubusb     xmm5, xmm3              ; abs(p0 - q0) *2 + abs(p1-q1)/2  > blimit
        pxor        xmm3, xmm3
        pcmpeqb     xmm5, xmm3

        ; start work on filters
        pxor        xmm2, [GLOBAL(t80)]     ; p1 offset to convert to signed values
        pxor        xmm7, [GLOBAL(t80)]     ; q1 offset to convert to signed values
        psubsb      xmm2, xmm7              ; p1 - q1

        pxor        xmm6, [GLOBAL(t80)]     ; offset to convert to signed values
        pxor        xmm0, [GLOBAL(t80)]     ; offset to convert to signed values
        movdqa      xmm3, xmm0              ; q0
        psubsb      xmm0, xmm6              ; q0 - p0
        paddsb      xmm2, xmm0              ; p1 - q1 + 1 * (q0 - p0)
        paddsb      xmm2, xmm0              ; p1 - q1 + 2 * (q0 - p0)
        paddsb      xmm2, xmm0              ; p1 - q1 + 3 * (q0 - p0)
        pand        xmm5, xmm2              ; mask filter values we don't care about

        ; do + 4 side
        paddsb      xmm5, [GLOBAL(t4)]      ; 3* (q0 - p0) + (p1 - q1) + 4

        movdqa      xmm0, xmm5              ; get a copy of filters
        psllw       xmm0, 8                 ; shift left 8
        psraw       xmm0, 3                 ; arithmetic shift right 11
        psrlw       xmm0, 8
        movdqa      xmm1, xmm5              ; get a copy of filters
        psraw       xmm1, 11                ; arithmetic shift right 11
        psllw       xmm1, 8                 ; shift left 8 to put it back

        por         xmm0, xmm1              ; put the two together to get result

        psubsb      xmm3, xmm0              ; q0-= q0 add
        pxor        xmm3, [GLOBAL(t80)]     ; unoffset
        movdqa      [rsi], xmm3             ; write back

        ; now do +3 side
        psubsb      xmm5, [GLOBAL(t1s)]     ; +3 instead of +4

        movdqa      xmm0, xmm5              ; get a copy of filters
        psllw       xmm0, 8                 ; shift left 8
        psraw       xmm0, 3                 ; arithmetic shift right 11
        psrlw       xmm0, 8
        psraw       xmm5, 11                ; arithmetic shift right 11
        psllw       xmm5, 8                 ; shift left 8 to put it back
        por         xmm0, xmm5              ; put the two together to get result


        paddsb      xmm6, xmm0              ; p0+= p0 add
        pxor        xmm6, [GLOBAL(t80)]     ; unoffset
        movdqa      [rsi+rax], xmm6         ; write back

    ; begin epilog
    pop rdi
    pop rsi
    RESTORE_GOT
    RESTORE_XMM
    UNSHADOW_ARGS
    pop         rbp
    ret


;void vp9_loop_filter_simple_vertical_edge_sse2
;(
;    unsigned char *src_ptr,
;    int  src_pixel_step,
;    const char *blimit,
;)
global sym(vp9_loop_filter_simple_vertical_edge_sse2) PRIVATE
sym(vp9_loop_filter_simple_vertical_edge_sse2):
    push        rbp         ; save old base pointer value.
    mov         rbp, rsp    ; set new base pointer value.
    SHADOW_ARGS_TO_STACK 3
    SAVE_XMM 7
    GET_GOT     rbx         ; save callee-saved reg
    push        rsi
    push        rdi
    ; end prolog

    ALIGN_STACK 16, rax
    sub         rsp, 32                         ; reserve 32 bytes
    %define t0  [rsp + 0]    ;__declspec(align(16)) char t0[16];
    %define t1  [rsp + 16]   ;__declspec(align(16)) char t1[16];

        mov         rsi, arg(0) ;src_ptr
        movsxd      rax, dword ptr arg(1) ;src_pixel_step     ; destination pitch?

        lea         rsi,        [rsi - 2 ]
        lea         rdi,        [rsi + rax]
        lea         rdx,        [rsi + rax*4]
        lea         rcx,        [rdx + rax]

        movd        xmm0,       [rsi]                   ; (high 96 bits unused) 03 02 01 00
        movd        xmm1,       [rdx]                   ; (high 96 bits unused) 43 42 41 40
        movd        xmm2,       [rdi]                   ; 13 12 11 10
        movd        xmm3,       [rcx]                   ; 53 52 51 50
        punpckldq   xmm0,       xmm1                    ; (high 64 bits unused) 43 42 41 40 03 02 01 00
        punpckldq   xmm2,       xmm3                    ; 53 52 51 50 13 12 11 10

        movd        xmm4,       [rsi + rax*2]           ; 23 22 21 20
        movd        xmm5,       [rdx + rax*2]           ; 63 62 61 60
        movd        xmm6,       [rdi + rax*2]           ; 33 32 31 30
        movd        xmm7,       [rcx + rax*2]           ; 73 72 71 70
        punpckldq   xmm4,       xmm5                    ; 63 62 61 60 23 22 21 20
        punpckldq   xmm6,       xmm7                    ; 73 72 71 70 33 32 31 30

        punpcklbw   xmm0,       xmm2                    ; 53 43 52 42 51 41 50 40 13 03 12 02 11 01 10 00
        punpcklbw   xmm4,       xmm6                    ; 73 63 72 62 71 61 70 60 33 23 32 22 31 21 30 20

        movdqa      xmm1,       xmm0
        punpcklwd   xmm0,       xmm4                    ; 33 23 13 03 32 22 12 02 31 21 11 01 30 20 10 00
        punpckhwd   xmm1,       xmm4                    ; 73 63 53 43 72 62 52 42 71 61 51 41 70 60 50 40

        movdqa      xmm2,       xmm0
        punpckldq   xmm0,       xmm1                    ; 71 61 51 41 31 21 11 01 70 60 50 40 30 20 10 00
        punpckhdq   xmm2,       xmm1                    ; 73 63 53 43 33 23 13 03 72 62 52 42 32 22 12 02

        movdqa      t0,         xmm0                    ; save to t0
        movdqa      t1,         xmm2                    ; save to t1

        lea         rsi,        [rsi + rax*8]
        lea         rdi,        [rsi + rax]
        lea         rdx,        [rsi + rax*4]
        lea         rcx,        [rdx + rax]

        movd        xmm4,       [rsi]                   ; 83 82 81 80
        movd        xmm1,       [rdx]                   ; c3 c2 c1 c0
        movd        xmm6,       [rdi]                   ; 93 92 91 90
        movd        xmm3,       [rcx]                   ; d3 d2 d1 d0
        punpckldq   xmm4,       xmm1                    ; c3 c2 c1 c0 83 82 81 80
        punpckldq   xmm6,       xmm3                    ; d3 d2 d1 d0 93 92 91 90

        movd        xmm0,       [rsi + rax*2]           ; a3 a2 a1 a0
        movd        xmm5,       [rdx + rax*2]           ; e3 e2 e1 e0
        movd        xmm2,       [rdi + rax*2]           ; b3 b2 b1 b0
        movd        xmm7,       [rcx + rax*2]           ; f3 f2 f1 f0
        punpckldq   xmm0,       xmm5                    ; e3 e2 e1 e0 a3 a2 a1 a0
        punpckldq   xmm2,       xmm7                    ; f3 f2 f1 f0 b3 b2 b1 b0

        punpcklbw   xmm4,       xmm6                    ; d3 c3 d2 c2 d1 c1 d0 c0 93 83 92 82 91 81 90 80
        punpcklbw   xmm0,       xmm2                    ; f3 e3 f2 e2 f1 e1 f0 e0 b3 a3 b2 a2 b1 a1 b0 a0

        movdqa      xmm1,       xmm4
        punpcklwd   xmm4,       xmm0                    ; b3 a3 93 83 b2 a2 92 82 b1 a1 91 81 b0 a0 90 80
        punpckhwd   xmm1,       xmm0                    ; f3 e3 d3 c3 f2 e2 d2 c2 f1 e1 d1 c1 f0 e0 d0 c0

        movdqa      xmm6,       xmm4
        punpckldq   xmm4,       xmm1                    ; f1 e1 d1 c1 b1 a1 91 81 f0 e0 d0 c0 b0 a0 90 80
        punpckhdq   xmm6,       xmm1                    ; f3 e3 d3 c3 b3 a3 93 83 f2 e2 d2 c2 b2 a2 92 82

        movdqa      xmm0,       t0                      ; 71 61 51 41 31 21 11 01 70 60 50 40 30 20 10 00
        movdqa      xmm2,       t1                      ; 73 63 53 43 33 23 13 03 72 62 52 42 32 22 12 02
        movdqa      xmm1,       xmm0
        movdqa      xmm3,       xmm2

        punpcklqdq  xmm0,       xmm4                    ; p1  f0 e0 d0 c0 b0 a0 90 80 70 60 50 40 30 20 10 00
        punpckhqdq  xmm1,       xmm4                    ; p0  f1 e1 d1 c1 b1 a1 91 81 71 61 51 41 31 21 11 01
        punpcklqdq  xmm2,       xmm6                    ; q0  f2 e2 d2 c2 b2 a2 92 82 72 62 52 42 32 22 12 02
        punpckhqdq  xmm3,       xmm6                    ; q1  f3 e3 d3 c3 b3 a3 93 83 73 63 53 43 33 23 13 03

        ; calculate mask
        movdqa      xmm6,       xmm0                            ; p1
        movdqa      xmm7,       xmm3                            ; q1
        psubusb     xmm7,       xmm0                            ; q1-=p1
        psubusb     xmm6,       xmm3                            ; p1-=q1
        por         xmm6,       xmm7                            ; abs(p1-q1)
        pand        xmm6,       [GLOBAL(tfe)]                   ; set lsb of each byte to zero
        psrlw       xmm6,       1                               ; abs(p1-q1)/2

        movdqa      xmm5,       xmm1                            ; p0
        movdqa      xmm4,       xmm2                            ; q0
        psubusb     xmm5,       xmm2                            ; p0-=q0
        psubusb     xmm4,       xmm1                            ; q0-=p0
        por         xmm5,       xmm4                            ; abs(p0 - q0)
        paddusb     xmm5,       xmm5                            ; abs(p0-q0)*2
        paddusb     xmm5,       xmm6                            ; abs (p0 - q0) *2 + abs(p1-q1)/2

        mov         rdx,        arg(2)                          ;blimit
        movdqa      xmm7, XMMWORD PTR [rdx]

        psubusb     xmm5,        xmm7                           ; abs(p0 - q0) *2 + abs(p1-q1)/2  > blimit
        pxor        xmm7,        xmm7
        pcmpeqb     xmm5,        xmm7                           ; mm5 = mask

        ; start work on filters
        movdqa        t0,        xmm0
        movdqa        t1,        xmm3

        pxor        xmm0,        [GLOBAL(t80)]                  ; p1 offset to convert to signed values
        pxor        xmm3,        [GLOBAL(t80)]                  ; q1 offset to convert to signed values

        psubsb      xmm0,        xmm3                           ; p1 - q1
        movdqa      xmm6,        xmm1                           ; p0

        movdqa      xmm7,        xmm2                           ; q0
        pxor        xmm6,        [GLOBAL(t80)]                  ; offset to convert to signed values

        pxor        xmm7,        [GLOBAL(t80)]                  ; offset to convert to signed values
        movdqa      xmm3,        xmm7                           ; offseted ; q0

        psubsb      xmm7,        xmm6                           ; q0 - p0
        paddsb      xmm0,        xmm7                           ; p1 - q1 + 1 * (q0 - p0)

        paddsb      xmm0,        xmm7                           ; p1 - q1 + 2 * (q0 - p0)
        paddsb      xmm0,        xmm7                           ; p1 - q1 + 3 * (q0 - p0)

        pand        xmm5,        xmm0                           ; mask filter values we don't care about


        paddsb      xmm5,        [GLOBAL(t4)]                   ;  3* (q0 - p0) + (p1 - q1) + 4

        movdqa      xmm0,        xmm5                           ; get a copy of filters
        psllw       xmm0,        8                              ; shift left 8

        psraw       xmm0,        3                              ; arithmetic shift right 11
        psrlw       xmm0,        8

        movdqa      xmm7,        xmm5                           ; get a copy of filters
        psraw       xmm7,        11                             ; arithmetic shift right 11

        psllw       xmm7,        8                              ; shift left 8 to put it back
        por         xmm0,        xmm7                           ; put the two together to get result

        psubsb      xmm3,        xmm0                           ; q0-= q0sz add
        pxor        xmm3,        [GLOBAL(t80)]                  ; unoffset   q0

        ; now do +3 side
        psubsb      xmm5,        [GLOBAL(t1s)]                  ; +3 instead of +4
        movdqa      xmm0,        xmm5                           ; get a copy of filters

        psllw       xmm0,        8                              ; shift left 8
        psraw       xmm0,        3                              ; arithmetic shift right 11

        psrlw       xmm0,        8
        psraw       xmm5,        11                             ; arithmetic shift right 11

        psllw       xmm5,        8                              ; shift left 8 to put it back
        por         xmm0,        xmm5                           ; put the two together to get result

        paddsb      xmm6,        xmm0                           ; p0+= p0 add
        pxor        xmm6,        [GLOBAL(t80)]                  ; unoffset   p0

        movdqa      xmm0,        t0                             ; p1
        movdqa      xmm4,        t1                             ; q1

        ; transpose back to write out
        ; p1  f0 e0 d0 c0 b0 a0 90 80 70 60 50 40 30 20 10 00
        ; p0  f1 e1 d1 c1 b1 a1 91 81 71 61 51 41 31 21 11 01
        ; q0  f2 e2 d2 c2 b2 a2 92 82 72 62 52 42 32 22 12 02
        ; q1  f3 e3 d3 c3 b3 a3 93 83 73 63 53 43 33 23 13 03
        movdqa      xmm1,       xmm0
        punpcklbw   xmm0,       xmm6                               ; 71 70 61 60 51 50 41 40 31 30 21 20 11 10 01 00
        punpckhbw   xmm1,       xmm6                               ; f1 f0 e1 e0 d1 d0 c1 c0 b1 b0 a1 a0 91 90 81 80

        movdqa      xmm5,       xmm3
        punpcklbw   xmm3,       xmm4                               ; 73 72 63 62 53 52 43 42 33 32 23 22 13 12 03 02
        punpckhbw   xmm5,       xmm4                               ; f3 f2 e3 e2 d3 d2 c3 c2 b3 b2 a3 a2 93 92 83 82

        movdqa      xmm2,       xmm0
        punpcklwd   xmm0,       xmm3                               ; 33 32 31 30 23 22 21 20 13 12 11 10 03 02 01 00
        punpckhwd   xmm2,       xmm3                               ; 73 72 71 70 63 62 61 60 53 52 51 50 43 42 41 40

        movdqa      xmm3,       xmm1
        punpcklwd   xmm1,       xmm5                               ; b3 b2 b1 b0 a3 a2 a1 a0 93 92 91 90 83 82 81 80
        punpckhwd   xmm3,       xmm5                               ; f3 f2 f1 f0 e3 e2 e1 e0 d3 d2 d1 d0 c3 c2 c1 c0

        ; write out order: xmm0 xmm2 xmm1 xmm3
        lea         rdx,        [rsi + rax*4]

        movd        [rsi],      xmm1                               ; write the second 8-line result
        psrldq      xmm1,       4
        movd        [rdi],      xmm1
        psrldq      xmm1,       4
        movd        [rsi + rax*2], xmm1
        psrldq      xmm1,       4
        movd        [rdi + rax*2], xmm1

        movd        [rdx],      xmm3
        psrldq      xmm3,       4
        movd        [rcx],      xmm3
        psrldq      xmm3,       4
        movd        [rdx + rax*2], xmm3
        psrldq      xmm3,       4
        movd        [rcx + rax*2], xmm3

        neg         rax
        lea         rsi,        [rsi + rax*8]
        neg         rax
        lea         rdi,        [rsi + rax]
        lea         rdx,        [rsi + rax*4]
        lea         rcx,        [rdx + rax]

        movd        [rsi],      xmm0                                ; write the first 8-line result
        psrldq      xmm0,       4
        movd        [rdi],      xmm0
        psrldq      xmm0,       4
        movd        [rsi + rax*2], xmm0
        psrldq      xmm0,       4
        movd        [rdi + rax*2], xmm0

        movd        [rdx],      xmm2
        psrldq      xmm2,       4
        movd        [rcx],      xmm2
        psrldq      xmm2,       4
        movd        [rdx + rax*2], xmm2
        psrldq      xmm2,       4
        movd        [rcx + rax*2], xmm2

    add rsp, 32
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
tfe:
    times 16 db 0xfe
align 16
t80:
    times 16 db 0x80
align 16
t1s:
    times 16 db 0x01
align 16
t3:
    times 16 db 0x03
align 16
t4:
    times 16 db 0x04
align 16
ones:
    times 8 dw 0x0001
align 16
s9:
    times 8 dw 0x0900
align 16
s63:
    times 8 dw 0x003f
