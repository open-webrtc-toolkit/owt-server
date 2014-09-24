/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vp9/common/vp9_onyxc_int.h"
#include "vp9/common/vp9_entropymv.h"

//#define MV_COUNT_TESTING

#define MV_COUNT_SAT 16
#define MV_MAX_UPDATE_FACTOR 160

#if CONFIG_NEW_MVREF
/* Integer pel reference mv threshold for use of high-precision 1/8 mv */
#define COMPANDED_MVREF_THRESH    1000000
#else
/* Integer pel reference mv threshold for use of high-precision 1/8 mv */
#define COMPANDED_MVREF_THRESH    8
#endif

/* Smooth or bias the mv-counts before prob computation */
/* #define SMOOTH_MV_COUNTS */

const vp9_tree_index vp9_mv_joint_tree[2 * MV_JOINTS - 2] = {
  -MV_JOINT_ZERO, 2,
  -MV_JOINT_HNZVZ, 4,
  -MV_JOINT_HZVNZ, -MV_JOINT_HNZVNZ
};
struct vp9_token_struct vp9_mv_joint_encodings[MV_JOINTS];

const vp9_tree_index vp9_mv_class_tree[2 * MV_CLASSES - 2] = {
  -MV_CLASS_0, 2,
  -MV_CLASS_1, 4,
  6, 8,
  -MV_CLASS_2, -MV_CLASS_3,
  10, 12,
  -MV_CLASS_4, -MV_CLASS_5,
  -MV_CLASS_6, -MV_CLASS_7,
};
struct vp9_token_struct vp9_mv_class_encodings[MV_CLASSES];

const vp9_tree_index vp9_mv_class0_tree [2 * CLASS0_SIZE - 2] = {
  -0, -1,
};
struct vp9_token_struct vp9_mv_class0_encodings[CLASS0_SIZE];

const vp9_tree_index vp9_mv_fp_tree [2 * 4 - 2] = {
  -0, 2,
  -1, 4,
  -2, -3
};
struct vp9_token_struct vp9_mv_fp_encodings[4];

const nmv_context vp9_default_nmv_context = {
  {32, 64, 96},
  {
    { /* vert component */
      128,                                             /* sign */
      {224, 144, 192, 168, 192, 176, 192},             /* class */
      {216},                                           /* class0 */
      {136, 140, 148, 160, 176, 192, 224},             /* bits */
      {{128, 128, 64}, {96, 112, 64}},                 /* class0_fp */
      {64, 96, 64},                                    /* fp */
      160,                                             /* class0_hp bit */
      128,                                             /* hp */
    },
    { /* hor component */
      128,                                             /* sign */
      {216, 128, 176, 160, 176, 176, 192},             /* class */
      {208},                                           /* class0 */
      {136, 140, 148, 160, 176, 192, 224},             /* bits */
      {{128, 128, 64}, {96, 112, 64}},                 /* class0_fp */
      {64, 96, 64},                                    /* fp */
      160,                                             /* class0_hp bit */
      128,                                             /* hp */
    }
  },
};

MV_JOINT_TYPE vp9_get_mv_joint(MV mv) {
  if (mv.row == 0 && mv.col == 0) return MV_JOINT_ZERO;
  else if (mv.row == 0 && mv.col != 0) return MV_JOINT_HNZVZ;
  else if (mv.row != 0 && mv.col == 0) return MV_JOINT_HZVNZ;
  else return MV_JOINT_HNZVNZ;
}

#define mv_class_base(c) ((c) ? (CLASS0_SIZE << (c + 2)) : 0)

MV_CLASS_TYPE vp9_get_mv_class(int z, int *offset) {
  MV_CLASS_TYPE c;
  if      (z < CLASS0_SIZE * 8)    c = MV_CLASS_0;
  else if (z < CLASS0_SIZE * 16)   c = MV_CLASS_1;
  else if (z < CLASS0_SIZE * 32)   c = MV_CLASS_2;
  else if (z < CLASS0_SIZE * 64)   c = MV_CLASS_3;
  else if (z < CLASS0_SIZE * 128)  c = MV_CLASS_4;
  else if (z < CLASS0_SIZE * 256)  c = MV_CLASS_5;
  else if (z < CLASS0_SIZE * 512)  c = MV_CLASS_6;
  else if (z < CLASS0_SIZE * 1024) c = MV_CLASS_7;
  else assert(0);
  if (offset)
    *offset = z - mv_class_base(c);
  return c;
}

int vp9_use_nmv_hp(const MV *ref) {
  if ((abs(ref->row) >> 3) < COMPANDED_MVREF_THRESH &&
      (abs(ref->col) >> 3) < COMPANDED_MVREF_THRESH)
    return 1;
  else
    return 0;
}

int vp9_get_mv_mag(MV_CLASS_TYPE c, int offset) {
  return mv_class_base(c) + offset;
}

static void increment_nmv_component_count(int v,
                                          nmv_component_counts *mvcomp,
                                          int incr,
                                          int usehp) {
  assert (v != 0);            /* should not be zero */
  mvcomp->mvcount[MV_MAX + v] += incr;
}

static void increment_nmv_component(int v,
                                    nmv_component_counts *mvcomp,
                                    int incr,
                                    int usehp) {
  int s, z, c, o, d, e, f;
  assert (v != 0);            /* should not be zero */
  s = v < 0;
  mvcomp->sign[s] += incr;
  z = (s ? -v : v) - 1;       /* magnitude - 1 */

  c = vp9_get_mv_class(z, &o);
  mvcomp->classes[c] += incr;

  d = (o >> 3);               /* int mv data */
  f = (o >> 1) & 3;           /* fractional pel mv data */
  e = (o & 1);                /* high precision mv data */
  if (c == MV_CLASS_0) {
    mvcomp->class0[d] += incr;
  } else {
    int i, b;
    b = c + CLASS0_BITS - 1;  /* number of bits */
    for (i = 0; i < b; ++i)
      mvcomp->bits[i][((d >> i) & 1)] += incr;
  }

  /* Code the fractional pel bits */
  if (c == MV_CLASS_0) {
    mvcomp->class0_fp[d][f] += incr;
  } else {
    mvcomp->fp[f] += incr;
  }

  /* Code the high precision bit */
  if (usehp) {
    if (c == MV_CLASS_0) {
      mvcomp->class0_hp[e] += incr;
    } else {
      mvcomp->hp[e] += incr;
    }
  }
}

#ifdef SMOOTH_MV_COUNTS
static void smooth_counts(nmv_component_counts *mvcomp) {
  static const int flen = 3;  // (filter_length + 1) / 2
  static const int fval[] = {8, 3, 1};
  static const int fvalbits = 4;
  int i;
  unsigned int smvcount[MV_VALS];
  vpx_memcpy(smvcount, mvcomp->mvcount, sizeof(smvcount));
  smvcount[MV_MAX] = (smvcount[MV_MAX - 1] + smvcount[MV_MAX + 1]) >> 1;
  for (i = flen - 1; i <= MV_VALS - flen; ++i) {
    int j, s = smvcount[i] * fval[0];
    for (j = 1; j < flen; ++j)
      s += (smvcount[i - j] + smvcount[i + j]) * fval[j];
    mvcomp->mvcount[i] = (s + (1 << (fvalbits - 1))) >> fvalbits;
  }
}
#endif

static void counts_to_context(nmv_component_counts *mvcomp, int usehp) {
  int v;
  vpx_memset(mvcomp->sign, 0, sizeof(nmv_component_counts) - sizeof(mvcomp->mvcount));
  for (v = 1; v <= MV_MAX; v++) {
    increment_nmv_component(-v, mvcomp, mvcomp->mvcount[MV_MAX - v], usehp);
    increment_nmv_component( v, mvcomp, mvcomp->mvcount[MV_MAX + v], usehp);
  }
}

void vp9_increment_nmv(const MV *mv, const MV *ref, nmv_context_counts *mvctx,
                       int usehp) {
  MV_JOINT_TYPE j = vp9_get_mv_joint(*mv);
  mvctx->joints[j]++;
  usehp = usehp && vp9_use_nmv_hp(ref);
  if (j == MV_JOINT_HZVNZ || j == MV_JOINT_HNZVNZ) {
    increment_nmv_component_count(mv->row, &mvctx->comps[0], 1, usehp);
  }
  if (j == MV_JOINT_HNZVZ || j == MV_JOINT_HNZVNZ) {
    increment_nmv_component_count(mv->col, &mvctx->comps[1], 1, usehp);
  }
}

static void adapt_prob(vp9_prob *dest, vp9_prob prep, vp9_prob newp,
                       unsigned int ct[2]) {
  int count = ct[0] + ct[1];

  if (count) {
    count = count > MV_COUNT_SAT ? MV_COUNT_SAT : count;
    *dest = weighted_prob(prep, newp,
                          MV_MAX_UPDATE_FACTOR * count / MV_COUNT_SAT);
  }
}

void vp9_counts_process(nmv_context_counts *NMVcount, int usehp) {
  counts_to_context(&NMVcount->comps[0], usehp);
  counts_to_context(&NMVcount->comps[1], usehp);
}

void vp9_counts_to_nmv_context(
    nmv_context_counts *NMVcount,
    nmv_context *prob,
    int usehp,
    unsigned int (*branch_ct_joint)[2],
    unsigned int (*branch_ct_sign)[2],
    unsigned int (*branch_ct_classes)[MV_CLASSES - 1][2],
    unsigned int (*branch_ct_class0)[CLASS0_SIZE - 1][2],
    unsigned int (*branch_ct_bits)[MV_OFFSET_BITS][2],
    unsigned int (*branch_ct_class0_fp)[CLASS0_SIZE][4 - 1][2],
    unsigned int (*branch_ct_fp)[4 - 1][2],
    unsigned int (*branch_ct_class0_hp)[2],
    unsigned int (*branch_ct_hp)[2]) {
  int i, j, k;
  vp9_counts_process(NMVcount, usehp);
  vp9_tree_probs_from_distribution(MV_JOINTS,
                                   vp9_mv_joint_encodings,
                                   vp9_mv_joint_tree,
                                   prob->joints,
                                   branch_ct_joint,
                                   NMVcount->joints);
  for (i = 0; i < 2; ++i) {
    prob->comps[i].sign = get_binary_prob(NMVcount->comps[i].sign[0],
                                          NMVcount->comps[i].sign[1]);
    branch_ct_sign[i][0] = NMVcount->comps[i].sign[0];
    branch_ct_sign[i][1] = NMVcount->comps[i].sign[1];
    vp9_tree_probs_from_distribution(MV_CLASSES,
                                     vp9_mv_class_encodings,
                                     vp9_mv_class_tree,
                                     prob->comps[i].classes,
                                     branch_ct_classes[i],
                                     NMVcount->comps[i].classes);
    vp9_tree_probs_from_distribution(CLASS0_SIZE,
                                     vp9_mv_class0_encodings,
                                     vp9_mv_class0_tree,
                                     prob->comps[i].class0,
                                     branch_ct_class0[i],
                                     NMVcount->comps[i].class0);
    for (j = 0; j < MV_OFFSET_BITS; ++j) {
      prob->comps[i].bits[j] = get_binary_prob(NMVcount->comps[i].bits[j][0],
                                               NMVcount->comps[i].bits[j][1]);
      branch_ct_bits[i][j][0] = NMVcount->comps[i].bits[j][0];
      branch_ct_bits[i][j][1] = NMVcount->comps[i].bits[j][1];
    }
  }
  for (i = 0; i < 2; ++i) {
    for (k = 0; k < CLASS0_SIZE; ++k) {
      vp9_tree_probs_from_distribution(4,
                                       vp9_mv_fp_encodings,
                                       vp9_mv_fp_tree,
                                       prob->comps[i].class0_fp[k],
                                       branch_ct_class0_fp[i][k],
                                       NMVcount->comps[i].class0_fp[k]);
    }
    vp9_tree_probs_from_distribution(4,
                                     vp9_mv_fp_encodings,
                                     vp9_mv_fp_tree,
                                     prob->comps[i].fp,
                                     branch_ct_fp[i],
                                     NMVcount->comps[i].fp);
  }
  if (usehp) {
    for (i = 0; i < 2; ++i) {
      prob->comps[i].class0_hp =
          get_binary_prob(NMVcount->comps[i].class0_hp[0],
                          NMVcount->comps[i].class0_hp[1]);
      branch_ct_class0_hp[i][0] = NMVcount->comps[i].class0_hp[0];
      branch_ct_class0_hp[i][1] = NMVcount->comps[i].class0_hp[1];

      prob->comps[i].hp = get_binary_prob(NMVcount->comps[i].hp[0],
                                          NMVcount->comps[i].hp[1]);
      branch_ct_hp[i][0] = NMVcount->comps[i].hp[0];
      branch_ct_hp[i][1] = NMVcount->comps[i].hp[1];
    }
  }
}

void vp9_adapt_nmv_probs(VP9_COMMON *cm, int usehp) {
  int i, j, k;
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
#ifdef MV_COUNT_TESTING
  printf("joints count: ");
  for (j = 0; j < MV_JOINTS; ++j) printf("%d ", cm->fc.NMVcount.joints[j]);
  printf("\n"); fflush(stdout);
  printf("signs count:\n");
  for (i = 0; i < 2; ++i)
    printf("%d/%d ", cm->fc.NMVcount.comps[i].sign[0], cm->fc.NMVcount.comps[i].sign[1]);
  printf("\n"); fflush(stdout);
  printf("classes count:\n");
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < MV_CLASSES; ++j)
      printf("%d ", cm->fc.NMVcount.comps[i].classes[j]);
    printf("\n"); fflush(stdout);
  }
  printf("class0 count:\n");
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < CLASS0_SIZE; ++j)
      printf("%d ", cm->fc.NMVcount.comps[i].class0[j]);
    printf("\n"); fflush(stdout);
  }
  printf("bits count:\n");
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < MV_OFFSET_BITS; ++j)
      printf("%d/%d ", cm->fc.NMVcount.comps[i].bits[j][0],
                       cm->fc.NMVcount.comps[i].bits[j][1]);
    printf("\n"); fflush(stdout);
  }
  printf("class0_fp count:\n");
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < CLASS0_SIZE; ++j) {
      printf("{");
      for (k = 0; k < 4; ++k)
        printf("%d ", cm->fc.NMVcount.comps[i].class0_fp[j][k]);
      printf("}, ");
    }
    printf("\n"); fflush(stdout);
  }
  printf("fp count:\n");
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < 4; ++j)
      printf("%d ", cm->fc.NMVcount.comps[i].fp[j]);
    printf("\n"); fflush(stdout);
  }
  if (usehp) {
    printf("class0_hp count:\n");
    for (i = 0; i < 2; ++i)
      printf("%d/%d ", cm->fc.NMVcount.comps[i].class0_hp[0],
                       cm->fc.NMVcount.comps[i].class0_hp[1]);
    printf("\n"); fflush(stdout);
    printf("hp count:\n");
    for (i = 0; i < 2; ++i)
      printf("%d/%d ", cm->fc.NMVcount.comps[i].hp[0],
                       cm->fc.NMVcount.comps[i].hp[1]);
    printf("\n"); fflush(stdout);
  }
#endif
#ifdef SMOOTH_MV_COUNTS
  smooth_counts(&cm->fc.NMVcount.comps[0]);
  smooth_counts(&cm->fc.NMVcount.comps[1]);
#endif
  vp9_counts_to_nmv_context(&cm->fc.NMVcount,
                            &prob,
                            usehp,
                            branch_ct_joint,
                            branch_ct_sign,
                            branch_ct_classes,
                            branch_ct_class0,
                            branch_ct_bits,
                            branch_ct_class0_fp,
                            branch_ct_fp,
                            branch_ct_class0_hp,
                            branch_ct_hp);

  for (j = 0; j < MV_JOINTS - 1; ++j) {
    adapt_prob(&cm->fc.nmvc.joints[j],
               cm->fc.pre_nmvc.joints[j],
               prob.joints[j],
               branch_ct_joint[j]);
  }
  for (i = 0; i < 2; ++i) {
    adapt_prob(&cm->fc.nmvc.comps[i].sign,
               cm->fc.pre_nmvc.comps[i].sign,
               prob.comps[i].sign,
               branch_ct_sign[i]);
    for (j = 0; j < MV_CLASSES - 1; ++j) {
      adapt_prob(&cm->fc.nmvc.comps[i].classes[j],
                 cm->fc.pre_nmvc.comps[i].classes[j],
                 prob.comps[i].classes[j],
                 branch_ct_classes[i][j]);
    }
    for (j = 0; j < CLASS0_SIZE - 1; ++j) {
      adapt_prob(&cm->fc.nmvc.comps[i].class0[j],
                 cm->fc.pre_nmvc.comps[i].class0[j],
                 prob.comps[i].class0[j],
                 branch_ct_class0[i][j]);
    }
    for (j = 0; j < MV_OFFSET_BITS; ++j) {
      adapt_prob(&cm->fc.nmvc.comps[i].bits[j],
                 cm->fc.pre_nmvc.comps[i].bits[j],
                 prob.comps[i].bits[j],
                 branch_ct_bits[i][j]);
    }
  }
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < CLASS0_SIZE; ++j) {
      for (k = 0; k < 3; ++k) {
        adapt_prob(&cm->fc.nmvc.comps[i].class0_fp[j][k],
                   cm->fc.pre_nmvc.comps[i].class0_fp[j][k],
                   prob.comps[i].class0_fp[j][k],
                   branch_ct_class0_fp[i][j][k]);
      }
    }
    for (j = 0; j < 3; ++j) {
      adapt_prob(&cm->fc.nmvc.comps[i].fp[j],
                 cm->fc.pre_nmvc.comps[i].fp[j],
                 prob.comps[i].fp[j],
                 branch_ct_fp[i][j]);
    }
  }
  if (usehp) {
    for (i = 0; i < 2; ++i) {
      adapt_prob(&cm->fc.nmvc.comps[i].class0_hp,
                 cm->fc.pre_nmvc.comps[i].class0_hp,
                 prob.comps[i].class0_hp,
                 branch_ct_class0_hp[i]);
      adapt_prob(&cm->fc.nmvc.comps[i].hp,
                 cm->fc.pre_nmvc.comps[i].hp,
                 prob.comps[i].hp,
                 branch_ct_hp[i]);
    }
  }
}

void vp9_entropy_mv_init() {
  vp9_tokens_from_tree(vp9_mv_joint_encodings, vp9_mv_joint_tree);
  vp9_tokens_from_tree(vp9_mv_class_encodings, vp9_mv_class_tree);
  vp9_tokens_from_tree(vp9_mv_class0_encodings, vp9_mv_class0_tree);
  vp9_tokens_from_tree(vp9_mv_fp_encodings, vp9_mv_fp_tree);
}

void vp9_init_mv_probs(VP9_COMMON *cm) {
  vpx_memcpy(&cm->fc.nmvc, &vp9_default_nmv_context, sizeof(nmv_context));
}
