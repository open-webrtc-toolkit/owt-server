/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_ENTROPYMODE_H_
#define VP9_COMMON_VP9_ENTROPYMODE_H_

#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_treecoder.h"

#define SUBMVREF_COUNT 5
#define VP9_NUMMBSPLITS 4

#if CONFIG_COMP_INTERINTRA_PRED
#define VP9_DEF_INTERINTRA_PROB 248
#define VP9_UPD_INTERINTRA_PROB 192
// whether to use a separate uv mode (1) or use the same as the y mode (0)
#define SEPARATE_INTERINTRA_UV  0
#endif

typedef const int vp9_mbsplit[16];

extern vp9_mbsplit vp9_mbsplits[VP9_NUMMBSPLITS];

extern const int vp9_mbsplit_count[VP9_NUMMBSPLITS];    /* # of subsets */

extern const vp9_prob vp9_mbsplit_probs[VP9_NUMMBSPLITS - 1];

extern int vp9_mv_cont(const int_mv *l, const int_mv *a);

extern const vp9_prob vp9_sub_mv_ref_prob2[SUBMVREF_COUNT][VP9_SUBMVREFS - 1];

extern const unsigned int vp9_kf_default_bmode_counts[VP9_KF_BINTRAMODES]
                                                     [VP9_KF_BINTRAMODES]
                                                     [VP9_KF_BINTRAMODES];

extern const vp9_tree_index vp9_bmode_tree[];
extern const vp9_tree_index vp9_kf_bmode_tree[];

extern const vp9_tree_index  vp9_ymode_tree[];
extern const vp9_tree_index  vp9_kf_ymode_tree[];
extern const vp9_tree_index  vp9_uv_mode_tree[];
#define vp9_sb_ymode_tree vp9_uv_mode_tree
#define vp9_sb_kf_ymode_tree vp9_uv_mode_tree
extern const vp9_tree_index  vp9_i8x8_mode_tree[];
extern const vp9_tree_index  vp9_mbsplit_tree[];
extern const vp9_tree_index  vp9_mv_ref_tree[];
extern const vp9_tree_index  vp9_sb_mv_ref_tree[];
extern const vp9_tree_index  vp9_sub_mv_ref_tree[];

extern struct vp9_token_struct vp9_bmode_encodings[VP9_NKF_BINTRAMODES];
extern struct vp9_token_struct vp9_kf_bmode_encodings[VP9_KF_BINTRAMODES];
extern struct vp9_token_struct vp9_ymode_encodings[VP9_YMODES];
extern struct vp9_token_struct vp9_sb_ymode_encodings[VP9_I32X32_MODES];
extern struct vp9_token_struct vp9_sb_kf_ymode_encodings[VP9_I32X32_MODES];
extern struct vp9_token_struct vp9_kf_ymode_encodings[VP9_YMODES];
extern struct vp9_token_struct vp9_i8x8_mode_encodings[VP9_I8X8_MODES];
extern struct vp9_token_struct vp9_uv_mode_encodings[VP9_UV_MODES];
extern struct vp9_token_struct vp9_mbsplit_encodings[VP9_NUMMBSPLITS];

/* Inter mode values do not start at zero */

extern struct vp9_token_struct vp9_mv_ref_encoding_array[VP9_MVREFS];
extern struct vp9_token_struct vp9_sb_mv_ref_encoding_array[VP9_MVREFS];
extern struct vp9_token_struct vp9_sub_mv_ref_encoding_array[VP9_SUBMVREFS];

void vp9_entropy_mode_init(void);

struct VP9Common;

/* sets up common features to forget past dependence */
void vp9_setup_past_independence(struct VP9Common *cm, MACROBLOCKD *xd);

void vp9_init_mbmode_probs(struct VP9Common *x);

extern void vp9_init_mode_contexts(struct VP9Common *pc);

extern void vp9_adapt_mode_context(struct VP9Common *pc);

extern void vp9_accum_mv_refs(struct VP9Common *pc,
                              MB_PREDICTION_MODE m,
                              const int context);

void vp9_default_bmode_probs(vp9_prob dest[VP9_NKF_BINTRAMODES - 1]);

void vp9_kf_default_bmode_probs(vp9_prob dest[VP9_KF_BINTRAMODES]
                                             [VP9_KF_BINTRAMODES]
                                             [VP9_KF_BINTRAMODES - 1]);

void vp9_adapt_mode_probs(struct VP9Common *);

#define VP9_SWITCHABLE_FILTERS 3 /* number of switchable filters */

extern const  INTERPOLATIONFILTERTYPE vp9_switchable_interp
                  [VP9_SWITCHABLE_FILTERS];

extern const  int vp9_switchable_interp_map[SWITCHABLE + 1];

extern const  int vp9_is_interpolating_filter[SWITCHABLE + 1];

extern const  vp9_tree_index vp9_switchable_interp_tree
                  [2 * (VP9_SWITCHABLE_FILTERS - 1)];

extern struct vp9_token_struct vp9_switchable_interp_encodings
                  [VP9_SWITCHABLE_FILTERS];

extern const  vp9_prob vp9_switchable_interp_prob[VP9_SWITCHABLE_FILTERS + 1]
                                                 [VP9_SWITCHABLE_FILTERS - 1];

#endif  // VP9_COMMON_VP9_ENTROPYMODE_H_
