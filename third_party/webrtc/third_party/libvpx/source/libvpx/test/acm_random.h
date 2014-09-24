/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef LIBVPX_TEST_ACM_RANDOM_H_
#define LIBVPX_TEST_ACM_RANDOM_H_

#include <stdlib.h>

#include "vpx/vpx_integer.h"

namespace libvpx_test {

class ACMRandom {
 public:
  ACMRandom() {
    Reset(DeterministicSeed());
  }

  explicit ACMRandom(int seed) {
    Reset(seed);
  }

  void Reset(int seed) {
    srand(seed);
  }

  uint8_t Rand8(void) {
    return (rand() >> 8) & 0xff;
  }

  int PseudoUniform(int range) {
    return (rand() >> 8) % range;
  }

  int operator()(int n) {
    return PseudoUniform(n);
  }

  static int DeterministicSeed(void) {
    return 0xbaba;
  }
};

}  // namespace libvpx_test

#endif  // LIBVPX_TEST_ACM_RANDOM_H_
