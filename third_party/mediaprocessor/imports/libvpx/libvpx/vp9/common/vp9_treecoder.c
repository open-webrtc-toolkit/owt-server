/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vpx_config.h"

#if defined(CONFIG_DEBUG) && CONFIG_DEBUG
#include <assert.h>
#endif
#include <stdio.h>

#include "vp9/common/vp9_treecoder.h"

static void tree2tok(
  struct vp9_token_struct *const p,
  vp9_tree t,
  int i,
  int v,
  int L
) {
  v += v;
  ++L;

  do {
    const vp9_tree_index j = t[i++];

    if (j <= 0) {
      p[-j].value = v;
      p[-j].Len = L;
    } else
      tree2tok(p, t, j, v, L);
  } while (++v & 1);
}

void vp9_tokens_from_tree(struct vp9_token_struct *p, vp9_tree t) {
  tree2tok(p, t, 0, 0, 0);
}

void vp9_tokens_from_tree_offset(struct vp9_token_struct *p, vp9_tree t,
                                 int offset) {
  tree2tok(p - offset, t, 0, 0, 0);
}

static unsigned int convert_distribution(unsigned int i,
                                         vp9_tree tree,
                                         vp9_prob probs[],
                                         unsigned int branch_ct[][2],
                                         const unsigned int num_events[],
                                         unsigned int tok0_offset) {
  unsigned int left, right;

  if (tree[i] <= 0) {
    left = num_events[-tree[i] - tok0_offset];
  } else {
    left = convert_distribution(tree[i], tree, probs, branch_ct,
                                num_events, tok0_offset);
  }
  if (tree[i + 1] <= 0) {
    right = num_events[-tree[i + 1] - tok0_offset];
  } else {
    right = convert_distribution(tree[i + 1], tree, probs, branch_ct,
                                num_events, tok0_offset);
  }
  probs[i>>1] = get_binary_prob(left, right);
  branch_ct[i>>1][0] = left;
  branch_ct[i>>1][1] = right;
  return left + right;
}

void vp9_tree_probs_from_distribution(
  vp9_tree tree,
  vp9_prob probs          [ /* n-1 */ ],
  unsigned int branch_ct       [ /* n-1 */ ] [2],
  const unsigned int num_events[ /* n */ ],
  unsigned int tok0_offset) {
  convert_distribution(0, tree, probs, branch_ct, num_events, tok0_offset);
}
