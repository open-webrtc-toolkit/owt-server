##
##  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
##
##  Use of this source code is governed by a BSD-style license
##  that can be found in the LICENSE file in the root of the source
##  tree. An additional intellectual property rights grant can be found
##  in the file PATENTS.  All contributing project authors may
##  be found in the AUTHORS file in the root of the source tree.
##

VP9_COMMON_SRCS-yes += vp9_common.mk
VP9_COMMON_SRCS-yes += vp9_iface_common.h
VP9_COMMON_SRCS-yes += common/vp9_pragmas.h
VP9_COMMON_SRCS-yes += common/vp9_ppflags.h
VP9_COMMON_SRCS-yes += common/vp9_onyx.h
VP9_COMMON_SRCS-yes += common/vp9_alloccommon.c
VP9_COMMON_SRCS-yes += common/vp9_asm_com_offsets.c
VP9_COMMON_SRCS-yes += common/vp9_blockd.c
VP9_COMMON_SRCS-yes += common/vp9_coefupdateprobs.h
VP9_COMMON_SRCS-yes += common/vp9_convolve.c
VP9_COMMON_SRCS-yes += common/vp9_convolve.h
VP9_COMMON_SRCS-yes += common/vp9_debugmodes.c
VP9_COMMON_SRCS-yes += common/vp9_default_coef_probs.h
VP9_COMMON_SRCS-yes += common/vp9_entropy.c
VP9_COMMON_SRCS-yes += common/vp9_entropymode.c
VP9_COMMON_SRCS-yes += common/vp9_entropymv.c
VP9_COMMON_SRCS-yes += common/vp9_extend.c
VP9_COMMON_SRCS-yes += common/vp9_filter.c
VP9_COMMON_SRCS-yes += common/vp9_filter.h
VP9_COMMON_SRCS-yes += common/vp9_findnearmv.c
VP9_COMMON_SRCS-yes += common/generic/vp9_systemdependent.c
VP9_COMMON_SRCS-yes += common/vp9_idct.c
VP9_COMMON_SRCS-yes += common/vp9_alloccommon.h
VP9_COMMON_SRCS-yes += common/vp9_blockd.h
VP9_COMMON_SRCS-yes += common/vp9_common.h
VP9_COMMON_SRCS-yes += common/vp9_entropy.h
VP9_COMMON_SRCS-yes += common/vp9_entropymode.h
VP9_COMMON_SRCS-yes += common/vp9_entropymv.h
VP9_COMMON_SRCS-yes += common/vp9_extend.h
VP9_COMMON_SRCS-yes += common/vp9_findnearmv.h
VP9_COMMON_SRCS-yes += common/vp9_header.h
VP9_COMMON_SRCS-yes += common/vp9_idct.h
VP9_COMMON_SRCS-yes += common/vp9_invtrans.h
VP9_COMMON_SRCS-yes += common/vp9_loopfilter.h
VP9_COMMON_SRCS-yes += common/vp9_modecont.h
VP9_COMMON_SRCS-yes += common/vp9_mv.h
VP9_COMMON_SRCS-yes += common/vp9_onyxc_int.h
VP9_COMMON_SRCS-yes += common/vp9_pred_common.h
VP9_COMMON_SRCS-yes += common/vp9_pred_common.c
VP9_COMMON_SRCS-yes += common/vp9_quant_common.h
VP9_COMMON_SRCS-yes += common/vp9_reconinter.h
VP9_COMMON_SRCS-yes += common/vp9_reconintra.h
VP9_COMMON_SRCS-yes += common/vp9_rtcd.c
VP9_COMMON_SRCS-yes += common/vp9_rtcd_defs.sh
VP9_COMMON_SRCS-yes += common/vp9_sadmxn.h
VP9_COMMON_SRCS-yes += common/vp9_subpelvar.h
VP9_COMMON_SRCS-yes += common/vp9_seg_common.h
VP9_COMMON_SRCS-yes += common/vp9_seg_common.c
VP9_COMMON_SRCS-yes += common/vp9_setupintrarecon.h
VP9_COMMON_SRCS-yes += common/vp9_swapyv12buffer.h
VP9_COMMON_SRCS-yes += common/vp9_systemdependent.h
VP9_COMMON_SRCS-yes += common/vp9_textblit.h
VP9_COMMON_SRCS-yes += common/vp9_tile_common.h
VP9_COMMON_SRCS-yes += common/vp9_tile_common.c
VP9_COMMON_SRCS-yes += common/vp9_treecoder.h
VP9_COMMON_SRCS-yes += common/vp9_invtrans.c
VP9_COMMON_SRCS-yes += common/vp9_loopfilter.c
VP9_COMMON_SRCS-yes += common/vp9_loopfilter_filters.c
VP9_COMMON_SRCS-yes += common/vp9_mbpitch.c
VP9_COMMON_SRCS-yes += common/vp9_modecont.c
VP9_COMMON_SRCS-yes += common/vp9_modecontext.c
VP9_COMMON_SRCS-yes += common/vp9_mvref_common.c
VP9_COMMON_SRCS-yes += common/vp9_mvref_common.h
VP9_COMMON_SRCS-yes += common/vp9_quant_common.c
VP9_COMMON_SRCS-yes += common/vp9_recon.c
VP9_COMMON_SRCS-yes += common/vp9_reconinter.c
VP9_COMMON_SRCS-yes += common/vp9_reconintra.c
VP9_COMMON_SRCS-yes += common/vp9_reconintra4x4.c
VP9_COMMON_SRCS-yes += common/vp9_setupintrarecon.c
VP9_COMMON_SRCS-yes += common/vp9_swapyv12buffer.c
VP9_COMMON_SRCS-$(CONFIG_POSTPROC_VISUALIZER) += common/vp9_textblit.c
VP9_COMMON_SRCS-yes += common/vp9_treecoder.c
VP9_COMMON_SRCS-$(CONFIG_IMPLICIT_SEGMENTATION) += common/vp9_implicit_segmentation.c

VP9_COMMON_SRCS-$(ARCH_X86)$(ARCH_X86_64) += common/x86/vp9_loopfilter_x86.h
VP9_COMMON_SRCS-$(ARCH_X86)$(ARCH_X86_64) += common/x86/vp9_postproc_x86.h
VP9_COMMON_SRCS-$(ARCH_X86)$(ARCH_X86_64) += common/x86/vp9_asm_stubs.c
VP9_COMMON_SRCS-$(ARCH_X86)$(ARCH_X86_64) += common/x86/vp9_loopfilter_intrin_mmx.c
VP9_COMMON_SRCS-$(ARCH_X86)$(ARCH_X86_64) += common/x86/vp9_loopfilter_intrin_sse2.c
VP9_COMMON_SRCS-$(CONFIG_POSTPROC) += common/vp9_postproc.h
VP9_COMMON_SRCS-$(CONFIG_POSTPROC) += common/vp9_postproc.c
VP9_COMMON_SRCS-$(HAVE_MMX) += common/x86/vp9_iwalsh_mmx.asm
VP9_COMMON_SRCS-$(HAVE_MMX) += common/x86/vp9_recon_mmx.asm
VP9_COMMON_SRCS-$(HAVE_MMX) += common/x86/vp9_loopfilter_mmx.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_idct_sse2.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_iwalsh_sse2.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_loopfilter_sse2.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_recon_sse2.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_recon_wrapper_sse2.c
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_subpel_variance_impl_sse2.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_subpixel_variance_sse2.c
VP9_COMMON_SRCS-$(HAVE_SSSE3) += common/x86/vp9_subpixel_8t_ssse3.asm
ifeq ($(CONFIG_POSTPROC),yes)
VP9_COMMON_SRCS-$(HAVE_MMX) += common/x86/vp9_postproc_mmx.asm
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_postproc_sse2.asm
endif

# common (c)
ifeq ($(CONFIG_CSM),yes)
VP9_COMMON_SRCS-yes += common/vp9_maskingmv.c
VP9_COMMON_SRCS-$(HAVE_SSE3) += common/x86/vp9_mask_sse3.asm
endif

VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_idct_intrin_sse2.c
VP9_COMMON_SRCS-$(HAVE_SSE2) += common/x86/vp9_sadmxn_sse2.c
ifeq ($(HAVE_SSE2),yes)
vp9/common/x86/vp9_idct_intrin_sse2.c.o: CFLAGS += -msse2
vp9/common/x86/vp9_loopfilter_intrin_sse2.c.o: CFLAGS += -msse2
vp9/common/x86/vp9_sadmxn_sse2.c.o: CFLAGS += -msse2
vp9/common/x86/vp9_idct_intrin_sse2.c.d: CFLAGS += -msse2
vp9/common/x86/vp9_loopfilter_intrin_sse2.c.d: CFLAGS += -msse2
vp9/common/x86/vp9_sadmxn_sse2.c.d: CFLAGS += -msse2
endif

$(eval $(call asm_offsets_template,\
         vp9_asm_com_offsets.asm, $(VP9_PREFIX)common/vp9_asm_com_offsets.c))

$(eval $(call rtcd_h_template,vp9_rtcd,vp9/common/vp9_rtcd_defs.sh))
