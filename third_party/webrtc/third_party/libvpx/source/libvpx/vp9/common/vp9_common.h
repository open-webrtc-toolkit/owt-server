/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_COMMON_H_
#define VP9_COMMON_VP9_COMMON_H_

#include <assert.h>
#include "vpx_config.h"
/* Interface header for common constant data structures and lookup tables */

#include "vpx_mem/vpx_mem.h"
#include "vpx/vpx_integer.h"

#define TRUE    1
#define FALSE   0

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

/* Only need this for fixed-size arrays, for structs just assign. */

#define vp9_copy(Dest, Src) { \
    assert(sizeof(Dest) == sizeof(Src)); \
    vpx_memcpy(Dest, Src, sizeof(Src)); \
  }

/* Use this for variably-sized arrays. */

#define vp9_copy_array(Dest, Src, N) { \
    assert(sizeof(*Dest) == sizeof(*Src)); \
    vpx_memcpy(Dest, Src, N * sizeof(*Src)); \
  }

#define vp9_zero(Dest) vpx_memset(&Dest, 0, sizeof(Dest));

#define vp9_zero_array(Dest, N) vpx_memset(Dest, 0, N * sizeof(*Dest));

static __inline uint8_t clip_pixel(int val) {
  return (val > 255) ? 255u : (val < 0) ? 0u : val;
}

#endif  // VP9_COMMON_VP9_COMMON_H_
