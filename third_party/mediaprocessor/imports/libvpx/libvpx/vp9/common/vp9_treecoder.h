/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_TREECODER_H_
#define VP9_COMMON_VP9_TREECODER_H_

#include "./vpx_config.h"
#include "vpx/vpx_integer.h"

typedef uint8_t vp9_prob;

#define vp9_prob_half ((vp9_prob) 128)

typedef int8_t vp9_tree_index;

#define vp9_complement(x) (255 - x)

/* We build coding trees compactly in arrays.
   Each node of the tree is a pair of vp9_tree_indices.
   Array index often references a corresponding probability table.
   Index <= 0 means done encoding/decoding and value = -Index,
   Index > 0 means need another bit, specification at index.
   Nonnegative indices are always even;  processing begins at node 0. */

typedef const vp9_tree_index vp9_tree[], *vp9_tree_p;

typedef const struct vp9_token_struct {
  int value;
  int Len;
} vp9_token;

/* Construct encoding array from tree. */

void vp9_tokens_from_tree(struct vp9_token_struct *, vp9_tree);
void vp9_tokens_from_tree_offset(struct vp9_token_struct *, vp9_tree,
                                 int offset);

/* Convert array of token occurrence counts into a table of probabilities
   for the associated binary encoding tree.  Also writes count of branches
   taken for each node on the tree; this facilitiates decisions as to
   probability updates. */

void vp9_tree_probs_from_distribution(vp9_tree tree,
                                      vp9_prob probs[ /* n - 1 */ ],
                                      unsigned int branch_ct[ /* n - 1 */ ][2],
                                      const unsigned int num_events[ /* n */ ],
                                      unsigned int tok0_offset);

static INLINE vp9_prob clip_prob(int p) {
  return (p > 255) ? 255u : (p < 1) ? 1u : p;
}

// int64 is not needed for normal frame level calculations.
// However when outputing entropy stats accumulated over many frames
// or even clips we can overflow int math.
#ifdef ENTROPY_STATS
static INLINE vp9_prob get_prob(int num, int den) {
  return (den == 0) ? 128u : clip_prob(((int64_t)num * 256 + (den >> 1)) / den);
}
#else
static INLINE vp9_prob get_prob(int num, int den) {
  return (den == 0) ? 128u : clip_prob((num * 256 + (den >> 1)) / den);
}
#endif

static INLINE vp9_prob get_binary_prob(int n0, int n1) {
  return get_prob(n0, n0 + n1);
}

/* this function assumes prob1 and prob2 are already within [1,255] range */
static INLINE vp9_prob weighted_prob(int prob1, int prob2, int factor) {
  return (prob1 * (256 - factor) + prob2 * factor + 128) >> 8;
}

#endif  // VP9_COMMON_VP9_TREECODER_H_
