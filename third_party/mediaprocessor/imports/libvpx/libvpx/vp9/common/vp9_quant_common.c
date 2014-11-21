/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_quant_common.h"

static int dc_qlookup[QINDEX_RANGE];
static int ac_qlookup[QINDEX_RANGE];

#define ACDC_MIN 4

void vp9_init_quant_tables() {
  int i;
  int current_val = 4;
  int last_val = 4;
  int ac_val;

  for (i = 0; i < QINDEX_RANGE; i++) {
    ac_qlookup[i] = current_val;
    current_val = (int)(current_val * 1.02);
    if (current_val == last_val)
      current_val++;
    last_val = current_val;

    ac_val = ac_qlookup[i];
    dc_qlookup[i] = (int)((0.000000305 * ac_val * ac_val * ac_val) +
                          (-0.00065 * ac_val * ac_val) +
                          (0.9 * ac_val) + 0.5);
    if (dc_qlookup[i] < ACDC_MIN)
      dc_qlookup[i] = ACDC_MIN;
  }
}

int vp9_dc_quant(int qindex, int delta) {
  return dc_qlookup[clamp(qindex + delta, 0, MAXQ)];
}

int vp9_dc_uv_quant(int qindex, int delta) {
  return dc_qlookup[clamp(qindex + delta, 0, MAXQ)];
}

int vp9_ac_yquant(int qindex) {
  return ac_qlookup[clamp(qindex, 0, MAXQ)];
}

int vp9_ac_uv_quant(int qindex, int delta) {
  return ac_qlookup[clamp(qindex + delta, 0, MAXQ)];
}
