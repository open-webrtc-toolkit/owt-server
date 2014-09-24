/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <string>
#include "vpx_config.h"
extern "C" {
#if ARCH_X86 || ARCH_X86_64
#include "vpx_ports/x86.h"
#endif
#if CONFIG_VP8
extern void vp8_rtcd();
#endif
#if CONFIG_VP9
extern void vp9_rtcd();
#endif
}
#include "third_party/googletest/src/include/gtest/gtest.h"

static void append_gtest_filter(const char *str) {
  std::string filter = ::testing::FLAGS_gtest_filter;
  filter += str;
  ::testing::FLAGS_gtest_filter = filter;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

#if ARCH_X86 || ARCH_X86_64
  const int simd_caps = x86_simd_caps();
  if (!(simd_caps & HAS_MMX))
    append_gtest_filter(":-MMX/*");
  if (!(simd_caps & HAS_SSE))
    append_gtest_filter(":-SSE/*");
  if (!(simd_caps & HAS_SSE2))
    append_gtest_filter(":-SSE2/*");
  if (!(simd_caps & HAS_SSE3))
    append_gtest_filter(":-SSE3/*");
  if (!(simd_caps & HAS_SSSE3))
    append_gtest_filter(":-SSSE3/*");
  if (!(simd_caps & HAS_SSE4_1))
    append_gtest_filter(":-SSE4_1/*");
#endif

#if !CONFIG_SHARED
  /* Shared library builds don't support whitebox tests that exercise internal symbols. */
#if CONFIG_VP8
  vp8_rtcd();
#endif
#if CONFIG_VP9
  vp9_rtcd();
#endif
#endif

  return RUN_ALL_TESTS();
}
