/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9/common/vp9_onyxc_int.h"

#define MAX_REGIONS 24000
#ifndef NULL
#define NULL 0
#endif

#define min_mbs_in_region 3

// this linked list structure holds equivalences for connected
// component labeling
struct list_el {
  int label;
  int seg_value;
  int count;
  struct list_el *next;
};
typedef struct list_el item;

// connected colorsegments
typedef struct {
  int min_x;
  int min_y;
  int max_x;
  int max_y;
  int64_t sum_x;
  int64_t sum_y;
  int pixels;
  int seg_value;
  int label;
} segment_info;


typedef enum {
  SEGMENT_MODE,
  SEGMENT_MV,
  SEGMENT_REFFRAME,
  SEGMENT_SKIPPED
} SEGMENT_TYPE;


// this merges the two equivalence lists and
// then makes sure that every label points to the same
// equivalence list
void merge(item *labels, int u, int v) {
  item *a = labels[u].next;
  item *b = labels[v].next;
  item c;
  item *it = &c;
  int count;

  // check if they are already merged
  if (u == v || a == b)
    return;

  count = a->count + b->count;

  // merge 2 sorted linked lists.
  while (a != NULL && b != NULL) {
    if (a->label < b->label) {
      it->next = a;
      a = a->next;
    } else {
      it->next = b;
      b = b->next;
    }

    it = it->next;
  }

  if (a == NULL)
    it->next = b;
  else
    it->next = a;

  it = c.next;

  // make sure every equivalence in the linked list points to this new ll
  while (it != NULL) {
    labels[it->label].next = c.next;
    it = it->next;
  }
  c.next->count = count;

}

void segment_via_mode_info(VP9_COMMON *oci, int how) {
  MODE_INFO *mi = oci->mi;
  int i, j;
  int mb_index = 0;

  int label = 1;
  int pitch = oci->mb_cols;

  // holds linked list equivalences
  // the max should probably be allocated at a higher level in oci
  item equivalences[MAX_REGIONS];
  int eq_ptr = 0;
  item labels[MAX_REGIONS];
  segment_info segments[MAX_REGIONS];
  int label_count = 1;
  int labeling[400 * 300];
  int *lp = labeling;

  label_count = 1;
  memset(labels, 0, sizeof(labels));
  memset(segments, 0, sizeof(segments));

  /* Go through each macroblock first pass labelling */
  for (i = 0; i < oci->mb_rows; i++, lp += pitch) {
    for (j = 0; j < oci->mb_cols; j++) {
      // int above seg_value, left seg_value, this seg_value...
      int a = -1, l = -1, n = -1;

      // above label, left label
      int al = -1, ll = -1;
      if (i) {
        al = lp[j - pitch];
        a = labels[al].next->seg_value;
      }
      if (j) {
        ll = lp[j - 1];
        l = labels[ll].next->seg_value;
      }

      // what setting are we going to do the implicit segmentation on
      switch (how) {
        case SEGMENT_MODE:
          n = mi[mb_index].mbmi.mode;
          break;
        case SEGMENT_MV:
          n = mi[mb_index].mbmi.mv[0].as_int;
          if (mi[mb_index].mbmi.ref_frame == INTRA_FRAME)
            n = -9999999;
          break;
        case SEGMENT_REFFRAME:
          n = mi[mb_index].mbmi.ref_frame;
          break;
        case SEGMENT_SKIPPED:
          n = mi[mb_index].mbmi.mb_skip_coeff;
          break;
      }

      // above and left both have the same seg_value
      if (n == a && n == l) {
        // pick the lowest label
        lp[j] = (al < ll ? al : ll);
        labels[lp[j]].next->count++;

        // merge the above and left equivalencies
        merge(labels, al, ll);
      }
      // this matches above seg_value
      else if (n == a) {
        // give it the same label as above
        lp[j] = al;
        labels[al].next->count++;
      }
      // this matches left seg_value
      else if (n == l) {
        // give it the same label as above
        lp[j] = ll;
        labels[ll].next->count++;
      } else {
        // new label doesn't match either
        item *e = &labels[label];
        item *nl = &equivalences[eq_ptr++];
        lp[j] = label;
        nl->label = label;
        nl->next = 0;
        nl->seg_value = n;
        nl->count = 1;
        e->next = nl;
        label++;
      }
      mb_index++;
    }
    mb_index++;
  }
  lp = labeling;

  // give new labels to regions
  for (i = 1; i < label; i++)
    if (labels[i].next->count > min_mbs_in_region  &&  labels[labels[i].next->label].label == 0) {
      segment_info *cs = &segments[label_count];
      cs->label = label_count;
      labels[labels[i].next->label].label = label_count++;
      labels[labels[i].next->label].seg_value  = labels[i].next->seg_value;
      cs->seg_value = labels[labels[i].next->label].seg_value;
      cs->min_x = oci->mb_cols;
      cs->min_y = oci->mb_rows;
      cs->max_x = 0;
      cs->max_y = 0;
      cs->sum_x = 0;
      cs->sum_y = 0;
      cs->pixels = 0;

    }
  lp = labeling;

  // this is just to gather stats...
  for (i = 0; i < oci->mb_rows; i++, lp += pitch) {
    for (j = 0; j < oci->mb_cols; j++) {
      segment_info *cs;
      int oldlab = labels[lp[j]].next->label;
      int lab = labels[oldlab].label;
      lp[j] = lab;

      cs = &segments[lab];

      cs->min_x = (j < cs->min_x ? j : cs->min_x);
      cs->max_x = (j > cs->max_x ? j : cs->max_x);
      cs->min_y = (i < cs->min_y ? i : cs->min_y);
      cs->max_y = (i > cs->max_y ? i : cs->max_y);
      cs->sum_x += j;
      cs->sum_y += i;
      cs->pixels++;

      lp[j] = lab;
      mb_index++;
    }
    mb_index++;
  }

  {
    lp = labeling;
    printf("labelling \n");
    mb_index = 0;
    for (i = 0; i < oci->mb_rows; i++, lp += pitch) {
      for (j = 0; j < oci->mb_cols; j++) {
        printf("%4d", lp[j]);
      }
      printf("            ");
      for (j = 0; j < oci->mb_cols; j++, mb_index++) {
        // printf("%3d",mi[mb_index].mbmi.mode );
        printf("%4d:%4d", mi[mb_index].mbmi.mv[0].as_mv.row,
            mi[mb_index].mbmi.mv[0].as_mv.col);
      }
      printf("\n");
      ++mb_index;
    }
    printf("\n");
  }
}

