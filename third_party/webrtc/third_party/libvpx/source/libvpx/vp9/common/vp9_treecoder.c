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

static void branch_counts(
  int n,                      /* n = size of alphabet */
  vp9_token tok               [ /* n */ ],
  vp9_tree tree,
  unsigned int branch_ct       [ /* n-1 */ ] [2],
  const unsigned int num_events[ /* n */ ]
) {
  const int tree_len = n - 1;
  int t = 0;

#if CONFIG_DEBUG
  assert(tree_len);
#endif

  do {
    branch_ct[t][0] = branch_ct[t][1] = 0;
  } while (++t < tree_len);

  t = 0;

  do {
    int L = tok[t].Len;
    const int enc = tok[t].value;
    const unsigned int ct = num_events[t];

    vp9_tree_index i = 0;

    do {
      const int b = (enc >> --L) & 1;
      const int j = i >> 1;
#if CONFIG_DEBUG
      assert(j < tree_len  &&  0 <= L);
#endif

      branch_ct [j] [b] += ct;
      i = tree[ i + b];
    } while (i > 0);

#if CONFIG_DEBUG
    assert(!L);
#endif
  } while (++t < n);

}


void vp9_tree_probs_from_distribution(
  int n,                      /* n = size of alphabet */
  vp9_token tok               [ /* n */ ],
  vp9_tree tree,
  vp9_prob probs          [ /* n-1 */ ],
  unsigned int branch_ct       [ /* n-1 */ ] [2],
  const unsigned int num_events[ /* n */ ]
) {
  const int tree_len = n - 1;
  int t = 0;

  branch_counts(n, tok, tree, branch_ct, num_events);

  do {
    probs[t] = get_binary_prob(branch_ct[t][0], branch_ct[t][1]);
  } while (++t < tree_len);
}
