/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/common/vp9_blockd.h"
#include "vp9/encoder/vp9_onyx_int.h"
#include "vp9/encoder/vp9_treewriter.h"
#include "vp9/common/vp9_entropymode.h"


void vp9_init_mode_costs(VP9_COMP *c) {
  VP9_COMMON *x = &c->common;
  const vp9_tree_p T = vp9_bmode_tree;
  const vp9_tree_p KT = vp9_kf_bmode_tree;
  int i, j;

  for (i = 0; i < VP9_KF_BINTRAMODES; i++) {
    for (j = 0; j < VP9_KF_BINTRAMODES; j++) {
      vp9_cost_tokens((int *)c->mb.bmode_costs[i][j],
                      x->kf_bmode_prob[i][j], KT);
    }
  }

  vp9_cost_tokens((int *)c->mb.inter_bmode_costs, x->fc.bmode_prob, T);
  vp9_cost_tokens((int *)c->mb.inter_bmode_costs,
                  x->fc.sub_mv_ref_prob[0], vp9_sub_mv_ref_tree);

  // TODO(rbultje) separate tables for superblock costing?
  vp9_cost_tokens(c->mb.mbmode_cost[1], x->fc.ymode_prob, vp9_ymode_tree);
  vp9_cost_tokens(c->mb.mbmode_cost[0],
                  x->kf_ymode_prob[c->common.kf_ymode_probs_index],
                  vp9_kf_ymode_tree);
  vp9_cost_tokens(c->mb.intra_uv_mode_cost[1],
                  x->fc.uv_mode_prob[VP9_YMODES - 1], vp9_uv_mode_tree);
  vp9_cost_tokens(c->mb.intra_uv_mode_cost[0],
                  x->kf_uv_mode_prob[VP9_YMODES - 1], vp9_uv_mode_tree);
  vp9_cost_tokens(c->mb.i8x8_mode_costs,
                  x->fc.i8x8_mode_prob, vp9_i8x8_mode_tree);

  for (i = 0; i <= VP9_SWITCHABLE_FILTERS; ++i)
    vp9_cost_tokens((int *)c->mb.switchable_interp_costs[i],
                    x->fc.switchable_interp_prob[i],
                    vp9_switchable_interp_tree);
}
