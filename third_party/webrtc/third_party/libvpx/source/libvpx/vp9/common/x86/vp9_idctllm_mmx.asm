;
;  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
;
;  Use of this source code is governed by a BSD-style license
;  that can be found in the LICENSE file in the root of the source
;  tree. An additional intellectual property rights grant can be found
;  in the file PATENTS.  All contributing project authors may
;  be found in the AUTHORS file in the root of the source tree.
;

%include "third_party/x86inc/x86inc.asm"

SECTION_RODATA
align 16
x_s1sqr2:      times 4 dw 0x8A8C
align 16
x_c1sqr2less1: times 4 dw 0x4E7B
align 16
pw_16:         times 4 dw 16

SECTION .text


; /****************************************************************************
; * Notes:
; *
; * This implementation makes use of 16 bit fixed point version of two multiply
; * constants:
; *        1.   sqrt(2) * cos (pi/8)
; *        2.   sqrt(2) * sin (pi/8)
; * Because the first constant is bigger than 1, to maintain the same 16 bit
; * fixed point precision as the second one, we use a trick of
; *        x * a = x + x*(a-1)
; * so
; *        x * sqrt(2) * cos (pi/8) = x + x * (sqrt(2) *cos(pi/8)-1).
; *
; * For the second constant, because of the 16bit version is 35468, which
; * is bigger than 32768, in signed 16 bit multiply, it becomes a negative
; * number.
; *        (x * (unsigned)35468 >> 16) = x * (signed)35468 >> 16 + x
; *
; **************************************************************************/

INIT_MMX

;void short_idct4x4llm_mmx(short *input, short *output, int pitch)
cglobal short_idct4x4llm_mmx, 3,3,0, inp, out, pit
    mova            m0,     [inpq +0]
    mova            m1,     [inpq +8]

    mova            m2,     [inpq+16]
    mova            m3,     [inpq+24]

    psubw           m0,      m2             ; b1= 0-2
    paddw           m2,      m2             ;

    mova            m5,      m1
    paddw           m2,      m0             ; a1 =0+2

    pmulhw          m5,     [x_s1sqr2]       ;
    paddw           m5,      m1             ; ip1 * sin(pi/8) * sqrt(2)

    mova            m7,      m3             ;
    pmulhw          m7,     [x_c1sqr2less1]   ;

    paddw           m7,      m3             ; ip3 * cos(pi/8) * sqrt(2)
    psubw           m7,      m5             ; c1

    mova            m5,      m1
    mova            m4,      m3

    pmulhw          m5,     [x_c1sqr2less1]
    paddw           m5,      m1

    pmulhw          m3,     [x_s1sqr2]
    paddw           m3,      m4

    paddw           m3,      m5             ; d1
    mova            m6,      m2             ; a1

    mova            m4,      m0             ; b1
    paddw           m2,      m3             ;0

    paddw           m4,      m7             ;1
    psubw           m0,      m7             ;2

    psubw           m6,      m3             ;3

    mova            m1,      m2             ; 03 02 01 00
    mova            m3,      m4             ; 23 22 21 20

    punpcklwd       m1,      m0             ; 11 01 10 00
    punpckhwd       m2,      m0             ; 13 03 12 02

    punpcklwd       m3,      m6             ; 31 21 30 20
    punpckhwd       m4,      m6             ; 33 23 32 22

    mova            m0,      m1             ; 11 01 10 00
    mova            m5,      m2             ; 13 03 12 02

    punpckldq       m0,      m3             ; 30 20 10 00
    punpckhdq       m1,      m3             ; 31 21 11 01

    punpckldq       m2,      m4             ; 32 22 12 02
    punpckhdq       m5,      m4             ; 33 23 13 03

    mova            m3,      m5             ; 33 23 13 03

    psubw           m0,      m2             ; b1= 0-2
    paddw           m2,      m2             ;

    mova            m5,      m1
    paddw           m2,      m0             ; a1 =0+2

    pmulhw          m5,     [x_s1sqr2]        ;
    paddw           m5,      m1             ; ip1 * sin(pi/8) * sqrt(2)

    mova            m7,      m3             ;
    pmulhw          m7,     [x_c1sqr2less1]   ;

    paddw           m7,      m3             ; ip3 * cos(pi/8) * sqrt(2)
    psubw           m7,      m5             ; c1

    mova            m5,      m1
    mova            m4,      m3

    pmulhw          m5,     [x_c1sqr2less1]
    paddw           m5,      m1

    pmulhw          m3,     [x_s1sqr2]
    paddw           m3,      m4

    paddw           m3,      m5             ; d1
    paddw           m0,     [pw_16]

    paddw           m2,     [pw_16]
    mova            m6,      m2             ; a1

    mova            m4,      m0             ; b1
    paddw           m2,      m3             ;0

    paddw           m4,      m7             ;1
    psubw           m0,      m7             ;2

    psubw           m6,      m3             ;3
    psraw           m2,      5

    psraw           m0,      5
    psraw           m4,      5

    psraw           m6,      5

    mova            m1,      m2             ; 03 02 01 00
    mova            m3,      m4             ; 23 22 21 20

    punpcklwd       m1,      m0             ; 11 01 10 00
    punpckhwd       m2,      m0             ; 13 03 12 02

    punpcklwd       m3,      m6             ; 31 21 30 20
    punpckhwd       m4,      m6             ; 33 23 32 22

    mova            m0,      m1             ; 11 01 10 00
    mova            m5,      m2             ; 13 03 12 02

    punpckldq       m0,      m3             ; 30 20 10 00
    punpckhdq       m1,      m3             ; 31 21 11 01

    punpckldq       m2,      m4             ; 32 22 12 02
    punpckhdq       m5,      m4             ; 33 23 13 03

    mova        [outq],      m0

    mova     [outq+r2],      m1
    mova [outq+pitq*2],      m2

    add           outq,      pitq
    mova [outq+pitq*2],      m5
    RET

;void short_idct4x4llm_1_mmx(short *input, short *output, int pitch)
cglobal short_idct4x4llm_1_mmx,3,3,0,inp,out,pit
    movh            m0,     [inpq]
    paddw           m0,     [pw_16]
    psraw           m0,      5
    punpcklwd       m0,      m0
    punpckldq       m0,      m0

    mova        [outq],      m0
    mova   [outq+pitq],      m0

    mova [outq+pitq*2],      m0
    add             r1,      r2

    mova [outq+pitq*2],      m0
    RET


;void dc_only_idct_add_mmx(short input_dc, unsigned char *pred_ptr, unsigned char *dst_ptr, int pitch, int stride)
cglobal dc_only_idct_add_mmx, 4,5,0,in_dc,pred,dst,pit,stride
%if ARCH_X86_64
    movsxd         strideq,      dword stridem
%else
    mov            strideq,      stridem
%endif
    pxor                m0,      m0

    movh                m5,      in_dcq ; dc
    paddw               m5,     [pw_16]

    psraw               m5,      5

    punpcklwd           m5,      m5
    punpckldq           m5,      m5

    movh                m1,     [predq]
    punpcklbw           m1,      m0
    paddsw              m1,      m5
    packuswb            m1,      m0              ; pack and unpack to saturate
    movh            [dstq],      m1

    movh                m2,     [predq+pitq]
    punpcklbw           m2,      m0
    paddsw              m2,      m5
    packuswb            m2,      m0              ; pack and unpack to saturate
    movh    [dstq+strideq],      m2

    movh                m3,     [predq+2*pitq]
    punpcklbw           m3,      m0
    paddsw              m3,      m5
    packuswb            m3,      m0              ; pack and unpack to saturate
    movh  [dstq+2*strideq],      m3

    add               dstq,      strideq
    add              predq,      pitq
    movh                m4,     [predq+2*pitq]
    punpcklbw           m4,      m0
    paddsw              m4,      m5
    packuswb            m4,      m0              ; pack and unpack to saturate
    movh  [dstq+2*strideq],      m4
    RET

