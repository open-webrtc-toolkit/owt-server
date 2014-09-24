/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_ENCODER_VP9_TREEWRITER_H_
#define VP9_ENCODER_VP9_TREEWRITER_H_

/* Trees map alphabets into huffman-like codes suitable for an arithmetic
   bit coder.  Timothy S Murphy  11 October 2004 */

#include "vp9/common/vp9_treecoder.h"

#include "vp9/encoder/vp9_boolhuff.h"       /* for now */

typedef BOOL_CODER vp9_writer;

#define vp9_write encode_bool
#define vp9_write_literal vp9_encode_value
#define vp9_write_bit(W, V) vp9_write(W, V, vp9_prob_half)

/* Approximate length of an encoded bool in 256ths of a bit at given prob */

#define vp9_cost_zero(x) (vp9_prob_cost[x])
#define vp9_cost_one(x) vp9_cost_zero(vp9_complement(x))

#define vp9_cost_bit(x, b) vp9_cost_zero((b) ? vp9_complement(x) : (x))

/* VP8BC version is scaled by 2^20 rather than 2^8; see bool_coder.h */


/* Both of these return bits, not scaled bits. */

static __inline unsigned int cost_branch(const unsigned int ct[2],
                                         vp9_prob p) {
  /* Imitate existing calculation */
  return ((ct[0] * vp9_cost_zero(p))
          + (ct[1] * vp9_cost_one(p))) >> 8;
}

static __inline unsigned int cost_branch256(const unsigned int ct[2],
                                            vp9_prob p) {
  /* Imitate existing calculation */
  return ((ct[0] * vp9_cost_zero(p))
          + (ct[1] * vp9_cost_one(p)));
}

/* Small functions to write explicit values and tokens, as well as
   estimate their lengths. */

static __inline void treed_write(vp9_writer *const w,
                                 vp9_tree t,
                                 const vp9_prob *const p,
                                 int v,
                                 /* number of bits in v, assumed nonzero */
                                 int n) {
  vp9_tree_index i = 0;

  do {
    const int b = (v >> --n) & 1;
    vp9_write(w, b, p[i >> 1]);
    i = t[i + b];
  } while (n);
}

static __inline void write_token(vp9_writer *const w,
                                 vp9_tree t,
                                 const vp9_prob *const p,
                                 vp9_token *const x) {
  treed_write(w, t, p, x->value, x->Len);
}

static __inline int treed_cost(vp9_tree t,
                               const vp9_prob *const p,
                               int v,
                               /* number of bits in v, assumed nonzero */
                               int n) {
  int c = 0;
  vp9_tree_index i = 0;

  do {
    const int b = (v >> --n) & 1;
    c += vp9_cost_bit(p[i >> 1], b);
    i = t[i + b];
  } while (n);

  return c;
}

static __inline int cost_token(vp9_tree t,
                               const vp9_prob *const p,
                               vp9_token *const x) {
  return treed_cost(t, p, x->value, x->Len);
}

/* Fill array of costs for all possible token values. */

void vp9_cost_tokens(int *Costs, const vp9_prob *, vp9_tree);

void vp9_cost_tokens_skip(int *c, const vp9_prob *p, vp9_tree t);

#endif  // VP9_ENCODER_VP9_TREEWRITER_H_
