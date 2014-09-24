/*
 *  Copyright (c) 2011 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vpx_config.h"
#include "vpx/vpx_codec.h"
#include "vpx_ports/asm_offsets.h"
#include "vp8/common/blockd.h"

#if CONFIG_POSTPROC
#include "postproc.h"
#endif /* CONFIG_POSTPROC */

BEGIN

#if CONFIG_POSTPROC
/* mfqe.c / filter_by_weight */
DEFINE(MFQE_PRECISION_VAL,                      MFQE_PRECISION);
#endif /* CONFIG_POSTPROC */

END

/* add asserts for any offset that is not supported by assembly code */
/* add asserts for any size that is not supported by assembly code */

#if HAVE_MEDIA
/* switch case in vp8_intra4x4_predict_armv6 is based on these enumerated values */
ct_assert(B_DC_PRED, B_DC_PRED == 0);
ct_assert(B_TM_PRED, B_TM_PRED == 1);
ct_assert(B_VE_PRED, B_VE_PRED == 2);
ct_assert(B_HE_PRED, B_HE_PRED == 3);
ct_assert(B_LD_PRED, B_LD_PRED == 4);
ct_assert(B_RD_PRED, B_RD_PRED == 5);
ct_assert(B_VR_PRED, B_VR_PRED == 6);
ct_assert(B_VL_PRED, B_VL_PRED == 7);
ct_assert(B_HD_PRED, B_HD_PRED == 8);
ct_assert(B_HU_PRED, B_HU_PRED == 9);
#endif

#if HAVE_SSE2
#if CONFIG_POSTPROC
/* vp8_filter_by_weight16x16 and 8x8 */
ct_assert(MFQE_PRECISION_VAL, MFQE_PRECISION == 4)
#endif /* CONFIG_POSTPROC */
#endif /* HAVE_SSE2 */
