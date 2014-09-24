/*
 *  Copyright (c) 2011 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vpx_ports/asm_offsets.h"
#include "vpx_config.h"
#include "vp9/encoder/vp9_block.h"
#include "vp9/common/vp9_blockd.h"
#include "vp9/encoder/vp9_onyx_int.h"
#include "vp9/encoder/vp9_treewriter.h"
#include "vp9/encoder/vp9_tokenize.h"

BEGIN

/* regular quantize */
DEFINE(vp9_block_coeff,                         offsetof(BLOCK, coeff));
DEFINE(vp9_block_zbin,                          offsetof(BLOCK, zbin));
DEFINE(vp9_block_round,                         offsetof(BLOCK, round));
DEFINE(vp9_block_quant,                         offsetof(BLOCK, quant));
DEFINE(vp9_block_quant_fast,                    offsetof(BLOCK, quant_fast));
DEFINE(vp9_block_zbin_extra,                    offsetof(BLOCK, zbin_extra));
DEFINE(vp9_block_zrun_zbin_boost,               offsetof(BLOCK, zrun_zbin_boost));
DEFINE(vp9_block_quant_shift,                   offsetof(BLOCK, quant_shift));

DEFINE(vp9_blockd_qcoeff,                       offsetof(BLOCKD, qcoeff));
DEFINE(vp9_blockd_dequant,                      offsetof(BLOCKD, dequant));
DEFINE(vp9_blockd_dqcoeff,                      offsetof(BLOCKD, dqcoeff));
DEFINE(vp9_blockd_eob,                          offsetof(BLOCKD, eob));

END

/* add asserts for any offset that is not supported by assembly code
 * add asserts for any size that is not supported by assembly code
 */
