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
#include "vp9/encoder/vp9_encodemv.h"
#include "vp9/common/vp9_entropymode.h"
#include "vp9/common/vp9_systemdependent.h"

#include <math.h>

#ifdef ENTROPY_STATS
extern unsigned int active_section;
#endif

#ifdef NMV_STATS
nmv_context_counts tnmvcounts;
#endif

static void encode_nmv_component(vp9_writer* const bc,
                                 int v,
                                 int r,
                                 const nmv_component* const mvcomp) {
  int s, z, c, o, d;
  assert (v != 0);            /* should not be zero */
  s = v < 0;
  vp9_write(bc, s, mvcomp->sign);
  z = (s ? -v : v) - 1;       /* magnitude - 1 */

  c = vp9_get_mv_class(z, &o);

  write_token(bc, vp9_mv_class_tree, mvcomp->classes,
              vp9_mv_class_encodings + c);

  d = (o >> 3);               /* int mv data */

  if (c == MV_CLASS_0) {
    write_token(bc, vp9_mv_class0_tree, mvcomp->class0,
                vp9_mv_class0_encodings + d);
  } else {
    int i, b;
    b = c + CLASS0_BITS - 1;  /* number of bits */
    for (i = 0; i < b; ++i)
      vp9_write(bc, ((d >> i) & 1), mvcomp->bits[i]);
  }
}

static void encode_nmv_component_fp(vp9_writer *bc,
                                    int v,
                                    int r,
                                    const nmv_component* const mvcomp,
                                    int usehp) {
  int s, z, c, o, d, f, e;
  assert (v != 0);            /* should not be zero */
  s = v < 0;
  z = (s ? -v : v) - 1;       /* magnitude - 1 */

  c = vp9_get_mv_class(z, &o);

  d = (o >> 3);               /* int mv data */
  f = (o >> 1) & 3;           /* fractional pel mv data */
  e = (o & 1);                /* high precision mv data */

  /* Code the fractional pel bits */
  if (c == MV_CLASS_0) {
    write_token(bc, vp9_mv_fp_tree, mvcomp->class0_fp[d],
                vp9_mv_fp_encodings + f);
  } else {
    write_token(bc, vp9_mv_fp_tree, mvcomp->fp,
                vp9_mv_fp_encodings + f);
  }
  /* Code the high precision bit */
  if (usehp) {
    if (c == MV_CLASS_0) {
      vp9_write(bc, e, mvcomp->class0_hp);
    } else {
      vp9_write(bc, e, mvcomp->hp);
    }
  }
}

static void build_nmv_component_cost_table(int *mvcost,
                                           const nmv_component* const mvcomp,
                                           int usehp) {
  int i, v;
  int sign_cost[2], class_cost[MV_CLASSES], class0_cost[CLASS0_SIZE];
  int bits_cost[MV_OFFSET_BITS][2];
  int class0_fp_cost[CLASS0_SIZE][4], fp_cost[4];
  int class0_hp_cost[2], hp_cost[2];

  sign_cost[0] = vp9_cost_zero(mvcomp->sign);
  sign_cost[1] = vp9_cost_one(mvcomp->sign);
  vp9_cost_tokens(class_cost, mvcomp->classes, vp9_mv_class_tree);
  vp9_cost_tokens(class0_cost, mvcomp->class0, vp9_mv_class0_tree);
  for (i = 0; i < MV_OFFSET_BITS; ++i) {
    bits_cost[i][0] = vp9_cost_zero(mvcomp->bits[i]);
    bits_cost[i][1] = vp9_cost_one(mvcomp->bits[i]);
  }

  for (i = 0; i < CLASS0_SIZE; ++i)
    vp9_cost_tokens(class0_fp_cost[i], mvcomp->class0_fp[i], vp9_mv_fp_tree);
  vp9_cost_tokens(fp_cost, mvcomp->fp, vp9_mv_fp_tree);

  if (usehp) {
    class0_hp_cost[0] = vp9_cost_zero(mvcomp->class0_hp);
    class0_hp_cost[1] = vp9_cost_one(mvcomp->class0_hp);
    hp_cost[0] = vp9_cost_zero(mvcomp->hp);
    hp_cost[1] = vp9_cost_one(mvcomp->hp);
  }
  mvcost[0] = 0;
  for (v = 1; v <= MV_MAX; ++v) {
    int z, c, o, d, e, f, cost = 0;
    z = v - 1;
    c = vp9_get_mv_class(z, &o);
    cost += class_cost[c];
    d = (o >> 3);               /* int mv data */
    f = (o >> 1) & 3;           /* fractional pel mv data */
    e = (o & 1);                /* high precision mv data */
    if (c == MV_CLASS_0) {
      cost += class0_cost[d];
    } else {
      int i, b;
      b = c + CLASS0_BITS - 1;  /* number of bits */
      for (i = 0; i < b; ++i)
        cost += bits_cost[i][((d >> i) & 1)];
    }
    if (c == MV_CLASS_0) {
      cost += class0_fp_cost[d][f];
    } else {
      cost += fp_cost[f];
    }
    if (usehp) {
      if (c == MV_CLASS_0) {
        cost += class0_hp_cost[e];
      } else {
        cost += hp_cost[e];
      }
    }
    mvcost[v] = cost + sign_cost[0];
    mvcost[-v] = cost + sign_cost[1];
  }
}

static int update_nmv_savings(const unsigned int ct[2],
                              const vp9_prob cur_p,
                              const vp9_prob new_p,
                              const vp9_prob upd_p) {

#ifdef LOW_PRECISION_MV_UPDATE
  vp9_prob mod_p = new_p | 1;
#else
  vp9_prob mod_p = new_p;
#endif
  const int cur_b = cost_branch256(ct, cur_p);
  const int mod_b = cost_branch256(ct, mod_p);
  const int cost = 7 * 256 +
#ifndef LOW_PRECISION_MV_UPDATE
      256 +
#endif
      (vp9_cost_one(upd_p) - vp9_cost_zero(upd_p));
  if (cur_b - mod_b - cost > 0) {
    return cur_b - mod_b - cost;
  } else {
    return 0 - vp9_cost_zero(upd_p);
  }
}

static int update_nmv(
  vp9_writer *const bc,
  const unsigned int ct[2],
  vp9_prob *const cur_p,
  const vp9_prob new_p,
  const vp9_prob upd_p) {

#ifdef LOW_PRECISION_MV_UPDATE
  vp9_prob mod_p = new_p | 1;
#else
  vp9_prob mod_p = new_p;
#endif

  const int cur_b = cost_branch256(ct, *cur_p);
  const int mod_b = cost_branch256(ct, mod_p);
  const int cost = 7 * 256 +
#ifndef LOW_PRECISION_MV_UPDATE
      256 +
#endif
      (vp9_cost_one(upd_p) - vp9_cost_zero(upd_p));

  if (cur_b - mod_b > cost) {
    *cur_p = mod_p;
    vp9_write(bc, 1, upd_p);
#ifdef LOW_PRECISION_MV_UPDATE
    vp9_write_literal(bc, mod_p >> 1, 7);
#else
    vp9_write_literal(bc, mod_p, 8);
#endif
    return 1;
  } else {
    vp9_write(bc, 0, upd_p);
    return 0;
  }
}

void print_nmvcounts(nmv_context_counts tnmvcounts) {
  int i, j, k;
  printf("\nCounts =\n  { ");
  for (j = 0; j < MV_JOINTS; ++j)
    printf("%d, ", tnmvcounts.joints[j]);
  printf("},\n");
  for (i = 0; i < 2; ++i) {
    printf("  {\n");
    printf("    %d/%d,\n", tnmvcounts.comps[i].sign[0],
                           tnmvcounts.comps[i].sign[1]);
    printf("    { ");
    for (j = 0; j < MV_CLASSES; ++j)
      printf("%d, ", tnmvcounts.comps[i].classes[j]);
    printf("},\n");
    printf("    { ");
    for (j = 0; j < CLASS0_SIZE; ++j)
      printf("%d, ", tnmvcounts.comps[i].class0[j]);
    printf("},\n");
    printf("    { ");
    for (j = 0; j < MV_OFFSET_BITS; ++j)
      printf("%d/%d, ", tnmvcounts.comps[i].bits[j][0],
                        tnmvcounts.comps[i].bits[j][1]);
    printf("},\n");

    printf("    {");
    for (j = 0; j < CLASS0_SIZE; ++j) {
      printf("{");
      for (k = 0; k < 4; ++k)
        printf("%d, ", tnmvcounts.comps[i].class0_fp[j][k]);
      printf("}, ");
    }
    printf("},\n");

    printf("    { ");
    for (j = 0; j < 4; ++j)
      printf("%d, ", tnmvcounts.comps[i].fp[j]);
    printf("},\n");

    printf("    %d/%d,\n",
           tnmvcounts.comps[i].class0_hp[0],
           tnmvcounts.comps[i].class0_hp[1]);
    printf("    %d/%d,\n",
           tnmvcounts.comps[i].hp[0],
           tnmvcounts.comps[i].hp[1]);
    printf("  },\n");
  }
}

#ifdef NMV_STATS
void init_nmvstats() {
  vp9_zero(tnmvcounts);
}

void print_nmvstats() {
  nmv_context prob;
  unsigned int branch_ct_joint[MV_JOINTS - 1][2];
  unsigned int branch_ct_sign[2][2];
  unsigned int branch_ct_classes[2][MV_CLASSES - 1][2];
  unsigned int branch_ct_class0[2][CLASS0_SIZE - 1][2];
  unsigned int branch_ct_bits[2][MV_OFFSET_BITS][2];
  unsigned int branch_ct_class0_fp[2][CLASS0_SIZE][4 - 1][2];
  unsigned int branch_ct_fp[2][4 - 1][2];
  unsigned int branch_ct_class0_hp[2][2];
  unsigned int branch_ct_hp[2][2];
  int i, j, k;
  vp9_counts_to_nmv_context(&tnmvcounts, &prob, 1,
                            branch_ct_joint, branch_ct_sign, branch_ct_classes,
                            branch_ct_class0, branch_ct_bits,
                            branch_ct_class0_fp, branch_ct_fp,
                            branch_ct_class0_hp, branch_ct_hp);

  printf("\nCounts =\n  { ");
  for (j = 0; j < MV_JOINTS; ++j)
    printf("%d, ", tnmvcounts.joints[j]);
  printf("},\n");
  for (i = 0; i < 2; ++i) {
    printf("  {\n");
    printf("    %d/%d,\n", tnmvcounts.comps[i].sign[0],
                           tnmvcounts.comps[i].sign[1]);
    printf("    { ");
    for (j = 0; j < MV_CLASSES; ++j)
      printf("%d, ", tnmvcounts.comps[i].classes[j]);
    printf("},\n");
    printf("    { ");
    for (j = 0; j < CLASS0_SIZE; ++j)
      printf("%d, ", tnmvcounts.comps[i].class0[j]);
    printf("},\n");
    printf("    { ");
    for (j = 0; j < MV_OFFSET_BITS; ++j)
      printf("%d/%d, ", tnmvcounts.comps[i].bits[j][0],
                        tnmvcounts.comps[i].bits[j][1]);
    printf("},\n");

    printf("    {");
    for (j = 0; j < CLASS0_SIZE; ++j) {
      printf("{");
      for (k = 0; k < 4; ++k)
        printf("%d, ", tnmvcounts.comps[i].class0_fp[j][k]);
      printf("}, ");
    }
    printf("},\n");

    printf("    { ");
    for (j = 0; j < 4; ++j)
      printf("%d, ", tnmvcounts.comps[i].fp[j]);
    printf("},\n");

    printf("    %d/%d,\n",
           tnmvcounts.comps[i].class0_hp[0],
           tnmvcounts.comps[i].class0_hp[1]);
    printf("    %d/%d,\n",
           tnmvcounts.comps[i].hp[0],
           tnmvcounts.comps[i].hp[1]);
    printf("  },\n");
  }

  printf("\nProbs =\n  { ");
  for (j = 0; j < MV_JOINTS - 1; ++j)
    printf("%d, ", prob.joints[j]);
  printf("},\n");
  for (i=0; i< 2; ++i) {
    printf("  {\n");
    printf("    %d,\n", prob.comps[i].sign);
    printf("    { ");
    for (j = 0; j < MV_CLASSES - 1; ++j)
      printf("%d, ", prob.comps[i].classes[j]);
    printf("},\n");
    printf("    { ");
    for (j = 0; j < CLASS0_SIZE - 1; ++j)
      printf("%d, ", prob.comps[i].class0[j]);
    printf("},\n");
    printf("    { ");
    for (j = 0; j < MV_OFFSET_BITS; ++j)
      printf("%d, ", prob.comps[i].bits[j]);
    printf("},\n");
    printf("    { ");
    for (j = 0; j < CLASS0_SIZE; ++j) {
      printf("{");
      for (k = 0; k < 3; ++k)
        printf("%d, ", prob.comps[i].class0_fp[j][k]);
      printf("}, ");
    }
    printf("},\n");
    printf("    { ");
    for (j = 0; j < 3; ++j)
      printf("%d, ", prob.comps[i].fp[j]);
    printf("},\n");

    printf("    %d,\n", prob.comps[i].class0_hp);
    printf("    %d,\n", prob.comps[i].hp);
    printf("  },\n");
  }
}

static void add_nmvcount(nmv_context_counts* const dst,
                         const nmv_context_counts* const src) {
  int i, j, k;
  for (j = 0; j < MV_JOINTS; ++j) {
    dst->joints[j] += src->joints[j];
  }
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < MV_VALS; ++j) {
      dst->comps[i].mvcount[j] += src->comps[i].mvcount[j];
    }
    dst->comps[i].sign[0] += src->comps[i].sign[0];
    dst->comps[i].sign[1] += src->comps[i].sign[1];
    for (j = 0; j < MV_CLASSES; ++j) {
      dst->comps[i].classes[j] += src->comps[i].classes[j];
    }
    for (j = 0; j < CLASS0_SIZE; ++j) {
      dst->comps[i].class0[j] += src->comps[i].class0[j];
    }
    for (j = 0; j < MV_OFFSET_BITS; ++j) {
      dst->comps[i].bits[j][0] += src->comps[i].bits[j][0];
      dst->comps[i].bits[j][1] += src->comps[i].bits[j][1];
    }
  }
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < CLASS0_SIZE; ++j) {
      for (k = 0; k < 4; ++k) {
        dst->comps[i].class0_fp[j][k] += src->comps[i].class0_fp[j][k];
      }
    }
    for (j = 0; j < 4; ++j) {
      dst->comps[i].fp[j] += src->comps[i].fp[j];
    }
    dst->comps[i].class0_hp[0] += src->comps[i].class0_hp[0];
    dst->comps[i].class0_hp[1] += src->comps[i].class0_hp[1];
    dst->comps[i].hp[0] += src->comps[i].hp[0];
    dst->comps[i].hp[1] += src->comps[i].hp[1];
  }
}
#endif

void vp9_write_nmv_probs(VP9_COMP* const cpi, int usehp, vp9_writer* const bc) {
  int i, j;
  nmv_context prob;
  unsigned int branch_ct_joint[MV_JOINTS - 1][2];
  unsigned int branch_ct_sign[2][2];
  unsigned int branch_ct_classes[2][MV_CLASSES - 1][2];
  unsigned int branch_ct_class0[2][CLASS0_SIZE - 1][2];
  unsigned int branch_ct_bits[2][MV_OFFSET_BITS][2];
  unsigned int branch_ct_class0_fp[2][CLASS0_SIZE][4 - 1][2];
  unsigned int branch_ct_fp[2][4 - 1][2];
  unsigned int branch_ct_class0_hp[2][2];
  unsigned int branch_ct_hp[2][2];
#ifdef MV_GROUP_UPDATE
  int savings = 0;
#endif

#ifdef NMV_STATS
  if (!cpi->dummy_packing)
    add_nmvcount(&tnmvcounts, &cpi->NMVcount);
#endif
  vp9_counts_to_nmv_context(&cpi->NMVcount, &prob, usehp,
                            branch_ct_joint, branch_ct_sign, branch_ct_classes,
                            branch_ct_class0, branch_ct_bits,
                            branch_ct_class0_fp, branch_ct_fp,
                            branch_ct_class0_hp, branch_ct_hp);
  /* write updates if they help */
#ifdef MV_GROUP_UPDATE
  for (j = 0; j < MV_JOINTS - 1; ++j) {
    savings += update_nmv_savings(branch_ct_joint[j],
                                  cpi->common.fc.nmvc.joints[j],
                                  prob.joints[j],
                                  VP9_NMV_UPDATE_PROB);
  }
  for (i = 0; i < 2; ++i) {
    savings += update_nmv_savings(branch_ct_sign[i],
                                  cpi->common.fc.nmvc.comps[i].sign,
                                  prob.comps[i].sign,
                                  VP9_NMV_UPDATE_PROB);
    for (j = 0; j < MV_CLASSES - 1; ++j) {
      savings += update_nmv_savings(branch_ct_classes[i][j],
                                    cpi->common.fc.nmvc.comps[i].classes[j],
                                    prob.comps[i].classes[j],
                                    VP9_NMV_UPDATE_PROB);
    }
    for (j = 0; j < CLASS0_SIZE - 1; ++j) {
      savings += update_nmv_savings(branch_ct_class0[i][j],
                                    cpi->common.fc.nmvc.comps[i].class0[j],
                                    prob.comps[i].class0[j],
                                    VP9_NMV_UPDATE_PROB);
    }
    for (j = 0; j < MV_OFFSET_BITS; ++j) {
      savings += update_nmv_savings(branch_ct_bits[i][j],
                                    cpi->common.fc.nmvc.comps[i].bits[j],
                                    prob.comps[i].bits[j],
                                    VP9_NMV_UPDATE_PROB);
    }
  }
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < CLASS0_SIZE; ++j) {
      int k;
      for (k = 0; k < 3; ++k) {
        savings += update_nmv_savings(branch_ct_class0_fp[i][j][k],
                                      cpi->common.fc.nmvc.comps[i].class0_fp[j][k],
                                      prob.comps[i].class0_fp[j][k],
                                      VP9_NMV_UPDATE_PROB);
      }
    }
    for (j = 0; j < 3; ++j) {
      savings += update_nmv_savings(branch_ct_fp[i][j],
                                    cpi->common.fc.nmvc.comps[i].fp[j],
                                    prob.comps[i].fp[j],
                                    VP9_NMV_UPDATE_PROB);
    }
  }
  if (usehp) {
    for (i = 0; i < 2; ++i) {
      savings += update_nmv_savings(branch_ct_class0_hp[i],
                                    cpi->common.fc.nmvc.comps[i].class0_hp,
                                    prob.comps[i].class0_hp,
                                    VP9_NMV_UPDATE_PROB);
      savings += update_nmv_savings(branch_ct_hp[i],
                                    cpi->common.fc.nmvc.comps[i].hp,
                                    prob.comps[i].hp,
                                    VP9_NMV_UPDATE_PROB);
    }
  }
  if (savings <= 0) {
    vp9_write_bit(bc, 0);
    return;
  }
  vp9_write_bit(bc, 1);
#endif

  for (j = 0; j < MV_JOINTS - 1; ++j) {
    update_nmv(bc, branch_ct_joint[j],
               &cpi->common.fc.nmvc.joints[j],
               prob.joints[j],
               VP9_NMV_UPDATE_PROB);
  }
  for (i = 0; i < 2; ++i) {
    update_nmv(bc, branch_ct_sign[i],
               &cpi->common.fc.nmvc.comps[i].sign,
               prob.comps[i].sign,
               VP9_NMV_UPDATE_PROB);
    for (j = 0; j < MV_CLASSES - 1; ++j) {
      update_nmv(bc, branch_ct_classes[i][j],
                 &cpi->common.fc.nmvc.comps[i].classes[j],
                 prob.comps[i].classes[j],
                 VP9_NMV_UPDATE_PROB);
    }
    for (j = 0; j < CLASS0_SIZE - 1; ++j) {
      update_nmv(bc, branch_ct_class0[i][j],
                 &cpi->common.fc.nmvc.comps[i].class0[j],
                 prob.comps[i].class0[j],
                 VP9_NMV_UPDATE_PROB);
    }
    for (j = 0; j < MV_OFFSET_BITS; ++j) {
      update_nmv(bc, branch_ct_bits[i][j],
                 &cpi->common.fc.nmvc.comps[i].bits[j],
                 prob.comps[i].bits[j],
                 VP9_NMV_UPDATE_PROB);
    }
  }
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < CLASS0_SIZE; ++j) {
      int k;
      for (k = 0; k < 3; ++k) {
        update_nmv(bc, branch_ct_class0_fp[i][j][k],
                   &cpi->common.fc.nmvc.comps[i].class0_fp[j][k],
                   prob.comps[i].class0_fp[j][k],
                   VP9_NMV_UPDATE_PROB);
      }
    }
    for (j = 0; j < 3; ++j) {
      update_nmv(bc, branch_ct_fp[i][j],
                 &cpi->common.fc.nmvc.comps[i].fp[j],
                 prob.comps[i].fp[j],
                 VP9_NMV_UPDATE_PROB);
    }
  }
  if (usehp) {
    for (i = 0; i < 2; ++i) {
      update_nmv(bc, branch_ct_class0_hp[i],
                 &cpi->common.fc.nmvc.comps[i].class0_hp,
                 prob.comps[i].class0_hp,
                 VP9_NMV_UPDATE_PROB);
      update_nmv(bc, branch_ct_hp[i],
                 &cpi->common.fc.nmvc.comps[i].hp,
                 prob.comps[i].hp,
                 VP9_NMV_UPDATE_PROB);
    }
  }
}

void vp9_encode_nmv(vp9_writer* const bc, const MV* const mv,
                    const MV* const ref, const nmv_context* const mvctx) {
  MV_JOINT_TYPE j = vp9_get_mv_joint(*mv);
  write_token(bc, vp9_mv_joint_tree, mvctx->joints,
              vp9_mv_joint_encodings + j);
  if (j == MV_JOINT_HZVNZ || j == MV_JOINT_HNZVNZ) {
    encode_nmv_component(bc, mv->row, ref->col, &mvctx->comps[0]);
  }
  if (j == MV_JOINT_HNZVZ || j == MV_JOINT_HNZVNZ) {
    encode_nmv_component(bc, mv->col, ref->col, &mvctx->comps[1]);
  }
}

void vp9_encode_nmv_fp(vp9_writer* const bc, const MV* const mv,
                       const MV* const ref, const nmv_context* const mvctx,
                       int usehp) {
  MV_JOINT_TYPE j = vp9_get_mv_joint(*mv);
  usehp = usehp && vp9_use_nmv_hp(ref);
  if (j == MV_JOINT_HZVNZ || j == MV_JOINT_HNZVNZ) {
    encode_nmv_component_fp(bc, mv->row, ref->row, &mvctx->comps[0], usehp);
  }
  if (j == MV_JOINT_HNZVZ || j == MV_JOINT_HNZVNZ) {
    encode_nmv_component_fp(bc, mv->col, ref->col, &mvctx->comps[1], usehp);
  }
}

void vp9_build_nmv_cost_table(int *mvjoint,
                              int *mvcost[2],
                              const nmv_context* const mvctx,
                              int usehp,
                              int mvc_flag_v,
                              int mvc_flag_h) {
  vp9_clear_system_state();
  vp9_cost_tokens(mvjoint, mvctx->joints, vp9_mv_joint_tree);
  if (mvc_flag_v)
    build_nmv_component_cost_table(mvcost[0], &mvctx->comps[0], usehp);
  if (mvc_flag_h)
    build_nmv_component_cost_table(mvcost[1], &mvctx->comps[1], usehp);
}

void vp9_update_nmv_count(VP9_COMP *cpi, MACROBLOCK *x,
                         int_mv *best_ref_mv, int_mv *second_best_ref_mv) {
  MB_MODE_INFO * mbmi = &x->e_mbd.mode_info_context->mbmi;
  MV mv;

  if (mbmi->mode == SPLITMV) {
    int i;

    for (i = 0; i < x->partition_info->count; i++) {
      if (x->partition_info->bmi[i].mode == NEW4X4) {
        if (x->e_mbd.allow_high_precision_mv) {
          mv.row = (x->partition_info->bmi[i].mv.as_mv.row
                    - best_ref_mv->as_mv.row);
          mv.col = (x->partition_info->bmi[i].mv.as_mv.col
                    - best_ref_mv->as_mv.col);
          vp9_increment_nmv(&mv, &best_ref_mv->as_mv, &cpi->NMVcount, 1);
          if (x->e_mbd.mode_info_context->mbmi.second_ref_frame > 0) {
            mv.row = (x->partition_info->bmi[i].second_mv.as_mv.row
                      - second_best_ref_mv->as_mv.row);
            mv.col = (x->partition_info->bmi[i].second_mv.as_mv.col
                      - second_best_ref_mv->as_mv.col);
            vp9_increment_nmv(&mv, &second_best_ref_mv->as_mv,
                              &cpi->NMVcount, 1);
          }
        } else {
          mv.row = (x->partition_info->bmi[i].mv.as_mv.row
                    - best_ref_mv->as_mv.row);
          mv.col = (x->partition_info->bmi[i].mv.as_mv.col
                    - best_ref_mv->as_mv.col);
          vp9_increment_nmv(&mv, &best_ref_mv->as_mv, &cpi->NMVcount, 0);
          if (x->e_mbd.mode_info_context->mbmi.second_ref_frame > 0) {
            mv.row = (x->partition_info->bmi[i].second_mv.as_mv.row
                      - second_best_ref_mv->as_mv.row);
            mv.col = (x->partition_info->bmi[i].second_mv.as_mv.col
                      - second_best_ref_mv->as_mv.col);
            vp9_increment_nmv(&mv, &second_best_ref_mv->as_mv,
                              &cpi->NMVcount, 0);
          }
        }
      }
    }
  } else if (mbmi->mode == NEWMV) {
    if (x->e_mbd.allow_high_precision_mv) {
      mv.row = (mbmi->mv[0].as_mv.row - best_ref_mv->as_mv.row);
      mv.col = (mbmi->mv[0].as_mv.col - best_ref_mv->as_mv.col);
      vp9_increment_nmv(&mv, &best_ref_mv->as_mv, &cpi->NMVcount, 1);
      if (mbmi->second_ref_frame > 0) {
        mv.row = (mbmi->mv[1].as_mv.row - second_best_ref_mv->as_mv.row);
        mv.col = (mbmi->mv[1].as_mv.col - second_best_ref_mv->as_mv.col);
        vp9_increment_nmv(&mv, &second_best_ref_mv->as_mv, &cpi->NMVcount, 1);
      }
    } else {
      mv.row = (mbmi->mv[0].as_mv.row - best_ref_mv->as_mv.row);
      mv.col = (mbmi->mv[0].as_mv.col - best_ref_mv->as_mv.col);
      vp9_increment_nmv(&mv, &best_ref_mv->as_mv, &cpi->NMVcount, 0);
      if (mbmi->second_ref_frame > 0) {
        mv.row = (mbmi->mv[1].as_mv.row - second_best_ref_mv->as_mv.row);
        mv.col = (mbmi->mv[1].as_mv.col - second_best_ref_mv->as_mv.col);
        vp9_increment_nmv(&mv, &second_best_ref_mv->as_mv, &cpi->NMVcount, 0);
      }
    }
  }
}
