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

#include "third_party/googletest/src/include/gtest/gtest.h"

#include "vpx/vpx_integer.h"

namespace libvpx_test {

class ACMRandom {
 public:
  ACMRandom() : random_(DeterministicSeed()) {}

  explicit ACMRandom(int seed) : random_(seed) {}

  void Reset(int seed) {
    random_.Reseed(seed);
  }

  uint8_t Rand8(void) {
    const uint32_t value =
        random_.Generate(testing::internal::Random::kMaxRange);
    // There's a bit more entropy in the upper bits of this implementation.
    return (value >> 24) & 0xff;
  }

  int PseudoUniform(int range) {
    return random_.Generate(range);
  }

  int operator()(int n) {
    return PseudoUniform(n);
  }

  static int DeterministicSeed(void) {
    return 0xbaba;
  }

 private:
  testing::internal::Random random_;
};

}  // namespace libvpx_test

#endif  // LIBVPX_TEST_ACM_RANDOM_H_
