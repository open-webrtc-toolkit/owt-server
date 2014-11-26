##
##  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
##
##  Use of this source code is governed by a BSD-style license
##  that can be found in the LICENSE file in the root of the source
##  tree. An additional intellectual property rights grant can be found
##  in the file PATENTS.  All contributing project authors may
##  be found in the AUTHORS file in the root of the source tree.
##

VP9_CX_EXPORTS += exports_enc

VP9_CX_SRCS-yes += $(VP9_COMMON_SRCS-yes)
VP9_CX_SRCS-no  += $(VP9_COMMON_SRCS-no)
VP9_CX_SRCS_REMOVE-yes += $(VP9_COMMON_SRCS_REMOVE-yes)
VP9_CX_SRCS_REMOVE-no  += $(VP9_COMMON_SRCS_REMOVE-no)

VP9_CX_SRCS-yes += vp9_cx_iface.c

# encoder
#INCLUDES += algo/vpx_common/vpx_mem/include
#INCLUDES += common
#INCLUDES += common
#INCLUDES += common
#INCLUDES += algo/vpx_ref/cpu_id/include
#INCLUDES += common
#INCLUDES += encoder

VP9_CX_SRCS-yes += encoder/vp9_asm_enc_offsets.c
VP9_CX_SRCS-yes += encoder/vp9_bitstream.c
VP9_CX_SRCS-yes += encoder/vp9_boolhuff.c
VP9_CX_SRCS-yes += encoder/vp9_dct.c
VP9_CX_SRCS-yes += encoder/vp9_encodeframe.c
VP9_CX_SRCS-yes += encoder/vp9_encodeframe.h
VP9_CX_SRCS-yes += encoder/vp9_encodeintra.c
VP9_CX_SRCS-yes += encoder/vp9_encodemb.c
VP9_CX_SRCS-yes += encoder/vp9_encodemv.c
VP9_CX_SRCS-yes += encoder/vp9_firstpass.c
VP9_CX_SRCS-yes += encoder/vp9_block.h
VP9_CX_SRCS-yes += encoder/vp9_boolhuff.h
VP9_CX_SRCS-yes += encoder/vp9_bitstream.h
VP9_CX_SRCS-yes += encoder/vp9_encodeintra.h
VP9_CX_SRCS-yes += encoder/vp9_encodemb.h
VP9_CX_SRCS-yes += encoder/vp9_encodemv.h
VP9_CX_SRCS-yes += encoder/vp9_firstpass.h
VP9_CX_SRCS-yes += encoder/vp9_lookahead.c
VP9_CX_SRCS-yes += encoder/vp9_lookahead.h
VP9_CX_SRCS-yes += encoder/vp9_mcomp.h
VP9_CX_SRCS-yes += encoder/vp9_modecosts.h
VP9_CX_SRCS-yes += encoder/vp9_onyx_int.h
VP9_CX_SRCS-yes += encoder/vp9_psnr.h
VP9_CX_SRCS-yes += encoder/vp9_quantize.h
VP9_CX_SRCS-yes += encoder/vp9_ratectrl.h
VP9_CX_SRCS-yes += encoder/vp9_rdopt.h
VP9_CX_SRCS-yes += encoder/vp9_tokenize.h
VP9_CX_SRCS-yes += encoder/vp9_treewriter.h
VP9_CX_SRCS-yes += encoder/vp9_variance.h
VP9_CX_SRCS-yes += encoder/vp9_mcomp.c
VP9_CX_SRCS-yes += encoder/vp9_modecosts.c
VP9_CX_SRCS-yes += encoder/vp9_onyx_if.c
VP9_CX_SRCS-yes += encoder/vp9_picklpf.c
VP9_CX_SRCS-yes += encoder/vp9_picklpf.h
VP9_CX_SRCS-yes += encoder/vp9_psnr.c
VP9_CX_SRCS-yes += encoder/vp9_quantize.c
VP9_CX_SRCS-yes += encoder/vp9_ratectrl.c
VP9_CX_SRCS-yes += encoder/vp9_rdopt.c
VP9_CX_SRCS-yes += encoder/vp9_sad_c.c
VP9_CX_SRCS-yes += encoder/vp9_segmentation.c
VP9_CX_SRCS-yes += encoder/vp9_segmentation.h
VP9_CX_SRCS-$(CONFIG_INTERNAL_STATS) += encoder/vp9_ssim.c
VP9_CX_SRCS-yes += encoder/vp9_tokenize.c
VP9_CX_SRCS-yes += encoder/vp9_treewriter.c
VP9_CX_SRCS-yes += encoder/vp9_variance_c.c
ifeq ($(CONFIG_POSTPROC),yes)
VP9_CX_SRCS-$(CONFIG_INTERNAL_STATS) += common/vp9_postproc.h
VP9_CX_SRCS-$(CONFIG_INTERNAL_STATS) += common/vp9_postproc.c
endif
VP9_CX_SRCS-yes += encoder/vp9_temporal_filter.c
VP9_CX_SRCS-yes += encoder/vp9_temporal_filter.h
VP9_CX_SRCS-yes += encoder/vp9_mbgraph.c
VP9_CX_SRCS-yes += encoder/vp9_mbgraph.h


VP9_CX_SRCS-$(ARCH_X86)$(ARCH_X86_64) += encoder/x86/vp9_mcomp_x86.h
VP9_CX_SRCS-$(ARCH_X86)$(ARCH_X86_64) += encoder/x86/vp9_quantize_x86.h
VP9_CX_SRCS-$(ARCH_X86)$(ARCH_X86_64) += encoder/x86/vp9_x86_csystemdependent.c
VP9_CX_SRCS-$(HAVE_MMX) += encoder/x86/vp9_variance_mmx.c
VP9_CX_SRCS-$(HAVE_MMX) += encoder/x86/vp9_variance_impl_mmx.asm
VP9_CX_SRCS-$(HAVE_MMX) += encoder/x86/vp9_sad_mmx.asm
VP9_CX_SRCS-$(HAVE_MMX) += encoder/x86/vp9_dct_mmx.asm
VP9_CX_SRCS-$(HAVE_MMX) += encoder/x86/vp9_dct_mmx.h
VP9_CX_SRCS-$(HAVE_MMX) += encoder/x86/vp9_subtract_mmx.asm
VP9_CX_SRCS-$(HAVE_SSE2) += encoder/x86/vp9_variance_sse2.c
VP9_CX_SRCS-$(HAVE_SSE2) += encoder/x86/vp9_variance_impl_sse2.asm
VP9_CX_SRCS-$(HAVE_SSE2) += encoder/x86/vp9_sad_sse2.asm
VP9_CX_SRCS-$(HAVE_SSE2) += encoder/x86/vp9_sad4d_sse2.asm
VP9_CX_SRCS-$(HAVE_SSE2) += encoder/x86/vp9_fwalsh_sse2.asm
#VP9_CX_SRCS-$(HAVE_SSE2) += encoder/x86/vp9_quantize_sse2.asm
VP9_CX_SRCS-$(HAVE_SSE2) += encoder/x86/vp9_subtract_sse2.asm
VP9_CX_SRCS-$(HAVE_SSE2) += encoder/x86/vp9_temporal_filter_apply_sse2.asm
VP9_CX_SRCS-$(HAVE_SSE3) += encoder/x86/vp9_sad_sse3.asm
VP9_CX_SRCS-$(HAVE_SSSE3) += encoder/x86/vp9_sad_ssse3.asm
VP9_CX_SRCS-$(HAVE_SSSE3) += encoder/x86/vp9_variance_ssse3.c
VP9_CX_SRCS-$(HAVE_SSSE3) += encoder/x86/vp9_variance_impl_ssse3.asm
#VP9_CX_SRCS-$(HAVE_SSSE3) += encoder/x86/vp9_quantize_ssse3.asm
VP9_CX_SRCS-$(HAVE_SSE4_1) += encoder/x86/vp9_sad_sse4.asm
#VP9_CX_SRCS-$(HAVE_SSE4_1) += encoder/x86/vp9_quantize_sse4.asm
VP9_CX_SRCS-$(ARCH_X86)$(ARCH_X86_64) += encoder/x86/vp9_quantize_mmx.asm
VP9_CX_SRCS-$(ARCH_X86)$(ARCH_X86_64) += encoder/x86/vp9_encodeopt.asm
VP9_CX_SRCS-$(ARCH_X86_64) += encoder/x86/vp9_ssim_opt.asm

VP9_CX_SRCS-$(HAVE_SSE2) += encoder/x86/vp9_dct_sse2.c
ifeq ($(HAVE_SSE2),yes)
vp9/encoder/x86/vp9_dct_sse2.c.d: CFLAGS += -msse2
vp9/encoder/x86/vp9_dct_sse2.c.o: CFLAGS += -msse2
endif


VP9_CX_SRCS-yes := $(filter-out $(VP9_CX_SRCS_REMOVE-yes),$(VP9_CX_SRCS-yes))

$(eval $(call asm_offsets_template,\
         vp9_asm_enc_offsets.asm, $(VP9_PREFIX)encoder/vp9_asm_enc_offsets.c))
