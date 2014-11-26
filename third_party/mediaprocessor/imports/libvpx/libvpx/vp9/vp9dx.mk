##
##  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
##
##  Use of this source code is governed by a BSD-style license
##  that can be found in the LICENSE file in the root of the source
##  tree. An additional intellectual property rights grant can be found
##  in the file PATENTS.  All contributing project authors may
##  be found in the AUTHORS file in the root of the source tree.
##

VP9_DX_EXPORTS += exports_dec

VP9_DX_SRCS-yes += $(VP9_COMMON_SRCS-yes)
VP9_DX_SRCS-no  += $(VP9_COMMON_SRCS-no)
VP9_DX_SRCS_REMOVE-yes += $(VP9_COMMON_SRCS_REMOVE-yes)
VP9_DX_SRCS_REMOVE-no  += $(VP9_COMMON_SRCS_REMOVE-no)

VP9_DX_SRCS-yes += vp9_dx_iface.c

VP9_DX_SRCS-yes += decoder/vp9_asm_dec_offsets.c
VP9_DX_SRCS-yes += decoder/vp9_dboolhuff.c
VP9_DX_SRCS-yes += decoder/vp9_decodemv.c
VP9_DX_SRCS-yes += decoder/vp9_decodframe.c
VP9_DX_SRCS-yes += decoder/vp9_decodframe.h
VP9_DX_SRCS-yes += decoder/vp9_dequantize.c
VP9_DX_SRCS-yes += decoder/vp9_detokenize.c
VP9_DX_SRCS-yes += decoder/vp9_dboolhuff.h
VP9_DX_SRCS-yes += decoder/vp9_decodemv.h
VP9_DX_SRCS-yes += decoder/vp9_dequantize.h
VP9_DX_SRCS-yes += decoder/vp9_detokenize.h
VP9_DX_SRCS-yes += decoder/vp9_onyxd.h
VP9_DX_SRCS-yes += decoder/vp9_onyxd_int.h
VP9_DX_SRCS-yes += decoder/vp9_treereader.h
VP9_DX_SRCS-yes += decoder/vp9_onyxd_if.c
VP9_DX_SRCS-yes += decoder/vp9_idct_blk.c

VP9_DX_SRCS-yes := $(filter-out $(VP9_DX_SRCS_REMOVE-yes),$(VP9_DX_SRCS-yes))

VP9_DX_SRCS-$(HAVE_SSE2) += decoder/x86/vp9_idct_blk_sse2.c

VP9_DX_SRCS-$(HAVE_SSE2) += decoder/x86/vp9_dequantize_sse2.c
ifeq ($(HAVE_SSE2),yes)
vp9/decoder/x86/vp9_dequantize_sse2.c.o: CFLAGS += -msse2
vp9/decoder/x86/vp9_dequantize_sse2.c.d: CFLAGS += -msse2
endif

$(eval $(call asm_offsets_template,\
         vp9_asm_dec_offsets.asm, $(VP9_PREFIX)decoder/vp9_asm_dec_offsets.c))
