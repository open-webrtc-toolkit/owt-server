/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/encoder/vp9_treewriter.h"

static void cost(
  int *const C,
  vp9_tree T,
  const vp9_prob *const P,
  int i,
  int c
) {
  const vp9_prob p = P [i >> 1];

  do {
    const vp9_tree_index j = T[i];
    const int d = c + vp9_cost_bit(p, i & 1);

    if (j <= 0)
      C[-j] = d;
    else
      cost(C, T, P, j, d);
  } while (++i & 1);
}
void vp9_cost_tokens(int *c, const vp9_prob *p, vp9_tree t) {
  cost(c, t, p, 0, 0);
}

void vp9_cost_tokens_skip(int *c, const vp9_prob *p, vp9_tree t) {
  cost(c, t, p, 2, 0);
}
