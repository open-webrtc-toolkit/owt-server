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

INIT_MMX


;void dequantize_b_impl_mmx(short *sq, short *dq, short *q)
cglobal dequantize_b_impl_mmx, 3,3,0,sq,dq,arg3
    mova       m1, [sqq]
    pmullw     m1, [arg3q+0]            ; mm4 *= kernel 0 modifiers.
    mova [dqq+ 0], m1

    mova       m1, [sqq+8]
    pmullw     m1, [arg3q+8]            ; mm4 *= kernel 0 modifiers.
    mova [dqq+ 8], m1

    mova       m1, [sqq+16]
    pmullw     m1, [arg3q+16]            ; mm4 *= kernel 0 modifiers.
    mova [dqq+16], m1

    mova       m1, [sqq+24]
    pmullw     m1, [arg3q+24]            ; mm4 *= kernel 0 modifiers.
    mova [dqq+24], m1
    RET


;void dequant_idct_add_mmx(short *input, short *dq, unsigned char *pred, unsigned char *dest, int pitch, int stride)
cglobal dequant_idct_add_mmx, 4,6,0,inp,dq,pred,dest,pit,stride

%if ARCH_X86_64
    movsxd              strideq,  dword stridem
    movsxd              pitq,     dword pitm
%else
    mov                 strideq,  stridem
    mov                 pitq,     pitm
%endif

    mova                m0,       [inpq+ 0]
    pmullw              m0,       [dqq]

    mova                m1,       [inpq+ 8]
    pmullw              m1,       [dqq+ 8]

    mova                m2,       [inpq+16]
    pmullw              m2,       [dqq+16]

    mova                m3,       [inpq+24]
    pmullw              m3,       [dqq+24]

    pxor                m7,        m7
    mova            [inpq],        m7
    mova          [inpq+8],        m7
    mova         [inpq+16],        m7
    mova         [inpq+24],        m7


    psubw               m0,        m2             ; b1= 0-2
    paddw               m2,        m2             ;

    mova                m5,        m1
    paddw               m2,        m0             ; a1 =0+2

    pmulhw              m5,       [x_s1sqr2];
    paddw               m5,        m1             ; ip1 * sin(pi/8) * sqrt(2)

    mova                m7,        m3             ;
    pmulhw              m7,       [x_c1sqr2less1];

    paddw               m7,        m3             ; ip3 * cos(pi/8) * sqrt(2)
    psubw               m7,        m5             ; c1

    mova                m5,        m1
    mova                m4,        m3

    pmulhw              m5,       [x_c1sqr2less1]
    paddw               m5,        m1

    pmulhw              m3,       [x_s1sqr2]
    paddw               m3,        m4

    paddw               m3,        m5             ; d1
    mova                m6,        m2             ; a1

    mova                m4,        m0             ; b1
    paddw               m2,        m3             ;0

    paddw               m4,        m7             ;1
    psubw               m0,        m7             ;2

    psubw               m6,        m3             ;3

    mova                m1,        m2             ; 03 02 01 00
    mova                m3,        m4             ; 23 22 21 20

    punpcklwd           m1,        m0             ; 11 01 10 00
    punpckhwd           m2,        m0             ; 13 03 12 02

    punpcklwd           m3,        m6             ; 31 21 30 20
    punpckhwd           m4,        m6             ; 33 23 32 22

    mova                m0,        m1             ; 11 01 10 00
    mova                m5,        m2             ; 13 03 12 02

    punpckldq           m0,        m3             ; 30 20 10 00
    punpckhdq           m1,        m3             ; 31 21 11 01

    punpckldq           m2,        m4             ; 32 22 12 02
    punpckhdq           m5,        m4             ; 33 23 13 03

    mova                m3,        m5             ; 33 23 13 03

    psubw               m0,        m2             ; b1= 0-2
    paddw               m2,        m2             ;

    mova                m5,        m1
    paddw               m2,        m0             ; a1 =0+2

    pmulhw              m5,       [x_s1sqr2];
    paddw               m5,        m1             ; ip1 * sin(pi/8) * sqrt(2)

    mova                m7,        m3             ;
    pmulhw              m7,       [x_c1sqr2less1];

    paddw               m7,        m3             ; ip3 * cos(pi/8) * sqrt(2)
    psubw               m7,        m5             ; c1

    mova                m5,        m1
    mova                m4,        m3

    pmulhw              m5,       [x_c1sqr2less1]
    paddw               m5,        m1

    pmulhw              m3,       [x_s1sqr2]
    paddw               m3,        m4

    paddw               m3,        m5             ; d1
    paddw               m0,       [pw_16]

    paddw               m2,       [pw_16]
    mova                m6,        m2             ; a1

    mova                m4,        m0             ; b1
    paddw               m2,        m3             ;0

    paddw               m4,        m7             ;1
    psubw               m0,        m7             ;2

    psubw               m6,        m3             ;3
    psraw               m2,        5

    psraw               m0,        5
    psraw               m4,        5

    psraw               m6,        5

    mova                m1,        m2             ; 03 02 01 00
    mova                m3,        m4             ; 23 22 21 20

    punpcklwd           m1,        m0             ; 11 01 10 00
    punpckhwd           m2,        m0             ; 13 03 12 02

    punpcklwd           m3,        m6             ; 31 21 30 20
    punpckhwd           m4,        m6             ; 33 23 32 22

    mova                m0,        m1             ; 11 01 10 00
    mova                m5,        m2             ; 13 03 12 02

    punpckldq           m0,        m3             ; 30 20 10 00
    punpckhdq           m1,        m3             ; 31 21 11 01

    punpckldq           m2,        m4             ; 32 22 12 02
    punpckhdq           m5,        m4             ; 33 23 13 03

    pxor                m7,        m7

    movh                m4,       [predq]
    punpcklbw           m4,        m7
    paddsw              m0,        m4
    packuswb            m0,        m7
    movh           [destq],      m0

    movh                m4,       [predq+pitq]
    punpcklbw           m4,        m7
    paddsw              m1,        m4
    packuswb            m1,        m7
    movh   [destq+strideq],        m1

    movh                m4,       [predq+2*pitq]
    punpcklbw           m4,        m7
    paddsw              m2,        m4
    packuswb            m2,        m7
    movh [destq+strideq*2],        m2

    add              destq,        strideq
    add              predq,        pitq

    movh                m4,       [predq+2*pitq]
    punpcklbw           m4,        m7
    paddsw              m5,        m4
    packuswb            m5,        m7
    movh [destq+strideq*2],        m5
    RET


;void dequant_dc_idct_add_mmx(short *input, short *dq, unsigned char *pred, unsigned char *dest, int pitch, int stride, int Dc)
cglobal dequant_dc_idct_add_mmx, 4,7,0,inp,dq,pred,dest,pit,stride,Dc

%if ARCH_X86_64
    movsxd              strideq,   dword stridem
    movsxd              pitq,      dword pitm
%else
    mov                 strideq,   stridem
    mov                 pitq,      pitm
%endif

    mov                 Dcq, Dcm
    mova                m0,       [inpq+ 0]
    pmullw              m0,       [dqq+ 0]

    mova                m1,       [inpq+ 8]
    pmullw              m1,       [dqq+ 8]

    mova                m2,       [inpq+16]
    pmullw              m2,       [dqq+16]

    mova                m3,       [inpq+24]
    pmullw              m3,       [dqq+24]

    pxor                m7,        m7
    mova         [inpq+ 0],        m7
    mova         [inpq+ 8],        m7
    mova         [inpq+16],        m7
    mova         [inpq+24],        m7

    ; move lower word of Dc to lower word of m0
    psrlq               m0,        16
    psllq               m0,        16
    and                Dcq,        0xFFFF         ; If Dc < 0, we don't want the full dword precision.
    movh                m7,        Dcq
    por                 m0,        m7
    psubw               m0,        m2             ; b1= 0-2
    paddw               m2,        m2             ;

    mova                m5,        m1
    paddw               m2,        m0             ; a1 =0+2

    pmulhw              m5,       [x_s1sqr2];
    paddw               m5,        m1             ; ip1 * sin(pi/8) * sqrt(2)

    mova                m7,        m3             ;
    pmulhw              m7,       [x_c1sqr2less1];

    paddw               m7,        m3             ; ip3 * cos(pi/8) * sqrt(2)
    psubw               m7,        m5             ; c1

    mova                m5,        m1
    mova                m4,        m3

    pmulhw              m5,       [x_c1sqr2less1]
    paddw               m5,        m1

    pmulhw              m3,       [x_s1sqr2]
    paddw               m3,        m4

    paddw               m3,        m5             ; d1
    mova                m6,        m2             ; a1

    mova                m4,        m0             ; b1
    paddw               m2,        m3             ;0

    paddw               m4,        m7             ;1
    psubw               m0,        m7             ;2

    psubw               m6,        m3             ;3

    mova                m1,        m2             ; 03 02 01 00
    mova                m3,        m4             ; 23 22 21 20

    punpcklwd           m1,        m0             ; 11 01 10 00
    punpckhwd           m2,        m0             ; 13 03 12 02

    punpcklwd           m3,        m6             ; 31 21 30 20
    punpckhwd           m4,        m6             ; 33 23 32 22

    mova                m0,        m1             ; 11 01 10 00
    mova                m5,        m2             ; 13 03 12 02

    punpckldq           m0,        m3             ; 30 20 10 00
    punpckhdq           m1,        m3             ; 31 21 11 01

    punpckldq           m2,        m4             ; 32 22 12 02
    punpckhdq           m5,        m4             ; 33 23 13 03

    mova                m3,        m5             ; 33 23 13 03

    psubw               m0,        m2             ; b1= 0-2
    paddw               m2,        m2             ;

    mova                m5,        m1
    paddw               m2,        m0             ; a1 =0+2

    pmulhw              m5,       [x_s1sqr2];
    paddw               m5,        m1             ; ip1 * sin(pi/8) * sqrt(2)

    mova                m7,        m3             ;
    pmulhw              m7,       [x_c1sqr2less1];

    paddw               m7,        m3             ; ip3 * cos(pi/8) * sqrt(2)
    psubw               m7,        m5             ; c1

    mova                m5,        m1
    mova                m4,        m3

    pmulhw              m5,       [x_c1sqr2less1]
    paddw               m5,        m1

    pmulhw              m3,       [x_s1sqr2]
    paddw               m3,        m4

    paddw               m3,        m5             ; d1
    paddw               m0,       [pw_16]

    paddw               m2,       [pw_16]
    mova                m6,        m2             ; a1

    mova                m4,        m0             ; b1
    paddw               m2,        m3             ;0

    paddw               m4,        m7             ;1
    psubw               m0,        m7             ;2

    psubw               m6,        m3             ;3
    psraw               m2,        5

    psraw               m0,        5
    psraw               m4,        5

    psraw               m6,        5

    mova                m1,        m2             ; 03 02 01 00
    mova                m3,        m4             ; 23 22 21 20

    punpcklwd           m1,        m0             ; 11 01 10 00
    punpckhwd           m2,        m0             ; 13 03 12 02

    punpcklwd           m3,        m6             ; 31 21 30 20
    punpckhwd           m4,        m6             ; 33 23 32 22

    mova                m0,        m1             ; 11 01 10 00
    mova                m5,        m2             ; 13 03 12 02

    punpckldq           m0,        m3             ; 30 20 10 00
    punpckhdq           m1,        m3             ; 31 21 11 01

    punpckldq           m2,        m4             ; 32 22 12 02
    punpckhdq           m5,        m4             ; 33 23 13 03

    pxor                m7,        m7

    movh                m4,       [predq]
    punpcklbw           m4,        m7
    paddsw              m0,        m4
    packuswb            m0,        m7
    movh           [destq],        m0

    movh                m4,       [predq+pitq]
    punpcklbw           m4,        m7
    paddsw              m1,        m4
    packuswb            m1,        m7
    movh   [destq+strideq],        m1

    movh                m4,       [predq+2*pitq]
    punpcklbw           m4,        m7
    paddsw              m2,        m4
    packuswb            m2,        m7
    movh [destq+strideq*2],        m2

    add              destq,        strideq
    add              predq,        pitq

    movh                m4,       [predq+2*pitq]
    punpcklbw           m4,        m7
    paddsw              m5,        m4
    packuswb            m5,        m7
    movh [destq+strideq*2],        m5
    RET

