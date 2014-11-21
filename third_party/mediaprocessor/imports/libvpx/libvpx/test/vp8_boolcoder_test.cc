/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

extern "C" {
#include "vp8/encoder/boolhuff.h"
#include "vp8/decoder/dboolhuff.h"
}

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "test/acm_random.h"
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "vpx/vpx_integer.h"

namespace {
const int num_tests = 10;

void encrypt_buffer(uint8_t *buffer, int size, const uint8_t *key) {
  for (int i = 0; i < size; ++i) {
    buffer[i] ^= key[i % 32];
  }
}

const uint8_t secret_key[32] = {
  234,  32,   2,  3,  4, 230,   6,  11,
    0, 132,  22, 23, 45,  21, 124, 255,
    0,  43,  52,  3, 23,  63,  99,   7,
  120,   8, 252, 84,  4,  83,   6,  13
};

}  // namespace

using libvpx_test::ACMRandom;

TEST(VP8, TestBitIO) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  for (int n = 0; n < num_tests; ++n) {
    for (int method = 0; method <= 7; ++method) {   // we generate various proba
      const int bits_to_test = 1000;
      uint8_t probas[bits_to_test];

      for (int i = 0; i < bits_to_test; ++i) {
        const int parity = i & 1;
        probas[i] =
            (method == 0) ? 0 : (method == 1) ? 255 :
            (method == 2) ? 128 :
            (method == 3) ? rnd.Rand8() :
            (method == 4) ? (parity ? 0 : 255) :
            // alternate between low and high proba:
            (method == 5) ? (parity ? rnd(128) : 255 - rnd(128)) :
            (method == 6) ?
                (parity ? rnd(64) : 255 - rnd(64)) :
                (parity ? rnd(32) : 255 - rnd(32));
      }
      for (int bit_method = 0; bit_method <= 3; ++bit_method) {
        const int random_seed = 6432;
        const int buffer_size = 10000;
        ACMRandom bit_rnd(random_seed);
        BOOL_CODER bw;
        uint8_t bw_buffer[buffer_size];
        vp8_start_encode(&bw, bw_buffer, bw_buffer + buffer_size);

        int bit = (bit_method == 0) ? 0 : (bit_method == 1) ? 1 : 0;
        for (int i = 0; i < bits_to_test; ++i) {
          if (bit_method == 2) {
            bit = (i & 1);
          } else if (bit_method == 3) {
            bit = bit_rnd(2);
          }
          vp8_encode_bool(&bw, bit, static_cast<int>(probas[i]));
        }

        vp8_stop_encode(&bw);

        BOOL_DECODER br;

#if CONFIG_DECRYPT
        encrypt_buffer(bw_buffer, buffer_size, secret_key);
#endif

        vp8dx_start_decode(&br, bw_buffer, buffer_size, bw_buffer, secret_key);
        bit_rnd.Reset(random_seed);
        for (int i = 0; i < bits_to_test; ++i) {
          if (bit_method == 2) {
            bit = (i & 1);
          } else if (bit_method == 3) {
            bit = bit_rnd(2);
          }
          GTEST_ASSERT_EQ(vp8dx_decode_bool(&br, probas[i]), bit)
              << "pos: "<< i << " / " << bits_to_test
              << " bit_method: " << bit_method
              << " method: " << method;
        }
      }
    }
  }
}
