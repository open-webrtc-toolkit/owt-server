/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <string.h>
#include "arm.h"

static int arm_cpu_env_flags(int *flags) {
  char *env;
  env = getenv("VPX_SIMD_CAPS");
  if (env && *env) {
    *flags = (int)strtol(env, NULL, 0);
    return 0;
  }
  *flags = 0;
  return -1;
}

static int arm_cpu_env_mask(void) {
  char *env;
  env = getenv("VPX_SIMD_CAPS_MASK");
  return env && *env ? (int)strtol(env, NULL, 0) : ~0;
}

#if !CONFIG_RUNTIME_CPU_DETECT

int arm_cpu_caps(void) {
  /* This function should actually be a no-op. There is no way to adjust any of
   * these because the RTCD tables do not exist: the functions are called
   * statically */
  int flags;
  int mask;
  if (!arm_cpu_env_flags(&flags)) {
    return flags;
  }
  mask = arm_cpu_env_mask();
#if HAVE_EDSP
  flags |= HAS_EDSP;
#endif /* HAVE_EDSP */
#if HAVE_MEDIA
  flags |= HAS_MEDIA;
#endif /* HAVE_MEDIA */
#if HAVE_NEON
  flags |= HAS_NEON;
#endif /* HAVE_NEON */
  return flags & mask;
}

#elif defined(_MSC_VER) /* end !CONFIG_RUNTIME_CPU_DETECT */
/*For GetExceptionCode() and EXCEPTION_ILLEGAL_INSTRUCTION.*/
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

int arm_cpu_caps(void) {
  int flags;
  int mask;
  if (!arm_cpu_env_flags(&flags)) {
    return flags;
  }
  mask = arm_cpu_env_mask();
  /* MSVC has no inline __asm support for ARM, but it does let you __emit
   *  instructions via their assembled hex code.
   * All of these instructions should be essentially nops.
   */
#if HAVE_EDSP
  if (mask & HAS_EDSP) {
    __try {
      /*PLD [r13]*/
      __emit(0xF5DDF000);
      flags |= HAS_EDSP;
    } __except (GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
      /*Ignore exception.*/
    }
  }
#if HAVE_MEDIA
  if (mask & HAS_MEDIA)
    __try {
      /*SHADD8 r3,r3,r3*/
      __emit(0xE6333F93);
      flags |= HAS_MEDIA;
    } __except (GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
    /*Ignore exception.*/
  }
}
#if HAVE_NEON
if (mask &HAS_NEON) {
  __try {
    /*VORR q0,q0,q0*/
    __emit(0xF2200150);
    flags |= HAS_NEON;
  } __except (GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
    /*Ignore exception.*/
  }
}
#endif /* HAVE_NEON */
#endif /* HAVE_MEDIA */
#endif /* HAVE_EDSP */
return flags & mask;
}

#elif defined(__ANDROID__) /* end _MSC_VER */
#include <cpu-features.h>

int arm_cpu_caps(void) {
  int flags;
  int mask;
  uint64_t features;
  if (!arm_cpu_env_flags(&flags)) {
    return flags;
  }
  mask = arm_cpu_env_mask();
  features = android_getCpuFeatures();

#if HAVE_EDSP
  flags |= HAS_EDSP;
#endif /* HAVE_EDSP */
#if HAVE_MEDIA
  flags |= HAS_MEDIA;
#endif /* HAVE_MEDIA */
#if HAVE_NEON
  if (features & ANDROID_CPU_ARM_FEATURE_NEON)
    flags |= HAS_NEON;
#endif /* HAVE_NEON */
  return flags & mask;
}

#elif defined(__linux__) /* end __ANDROID__ */

#include <stdio.h>

int arm_cpu_caps(void) {
  FILE *fin;
  int flags;
  int mask;
  if (!arm_cpu_env_flags(&flags)) {
    return flags;
  }
  mask = arm_cpu_env_mask();
  /* Reading /proc/self/auxv would be easier, but that doesn't work reliably
   *  on Android.
   * This also means that detection will fail in Scratchbox.
   */
  fin = fopen("/proc/cpuinfo", "r");
  if (fin != NULL) {
    /* 512 should be enough for anybody (it's even enough for all the flags
     * that x86 has accumulated... so far).
     */
    char buf[512];
    while (fgets(buf, 511, fin) != NULL) {
#if HAVE_EDSP || HAVE_NEON
      if (memcmp(buf, "Features", 8) == 0) {
        char *p;
#if HAVE_EDSP
        p = strstr(buf, " edsp");
        if (p != NULL && (p[5] == ' ' || p[5] == '\n')) {
          flags |= HAS_EDSP;
        }
#if HAVE_NEON
        p = strstr(buf, " neon");
        if (p != NULL && (p[5] == ' ' || p[5] == '\n')) {
          flags |= HAS_NEON;
        }
#endif /* HAVE_NEON */
#endif /* HAVE_EDSP */
      }
#endif /* HAVE_EDSP || HAVE_NEON */
#if HAVE_MEDIA
      if (memcmp(buf, "CPU architecture:", 17) == 0) {
        int version;
        version = atoi(buf + 17);
        if (version >= 6) {
          flags |= HAS_MEDIA;
        }
      }
#endif /* HAVE_MEDIA */
    }
    fclose(fin);
  }
  return flags & mask;
}
#else /* end __linux__ */
#error "--enable-runtime-cpu-detect selected, but no CPU detection method " \
"available for your platform. Reconfigure with --disable-runtime-cpu-detect."
#endif
