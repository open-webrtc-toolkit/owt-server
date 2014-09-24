/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/cpu_id.h"

#ifdef _MSC_VER
#include <intrin.h>  // For __cpuid()
#endif
#if !defined(__CLR_VER) && defined(_M_X64) && \
    defined(_MSC_VER) && (_MSC_FULL_VER >= 160040219)
#include <immintrin.h>  // For _xgetbv()
#endif

#include <stdlib.h>  // For getenv()

// For ArmCpuCaps() but unittested on all platforms
#include <stdio.h>
#include <string.h>

#include "libyuv/basic_types.h"  // For CPU_X86

// TODO(fbarchard): Use cpuid.h when gcc 4.4 is used on OSX and Linux.
#if (defined(__pic__) || defined(__APPLE__)) && defined(__i386__)
static __inline void __cpuid(int cpu_info[4], int info_type) {
  asm volatile (  // NOLINT
    "mov %%ebx, %%edi                          \n"
    "cpuid                                     \n"
    "xchg %%edi, %%ebx                         \n"
    : "=a"(cpu_info[0]), "=D"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type));
}
#elif defined(__i386__) || defined(__x86_64__)
static __inline void __cpuid(int cpu_info[4], int info_type) {
  asm volatile (  // NOLINT
    "cpuid                                     \n"
    : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type));
}
#endif

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Low level cpuid for X86. Returns zeros on other CPUs.
#if !defined(__CLR_VER) && (defined(_M_IX86) || defined(_M_X64) || \
    defined(__i386__) || defined(__x86_64__))
LIBYUV_API
void CpuId(int cpu_info[4], int info_type) {
  __cpuid(cpu_info, info_type);
}
#else
LIBYUV_API
void CpuId(int cpu_info[4], int) {
  cpu_info[0] = cpu_info[1] = cpu_info[2] = cpu_info[3] = 0;
}
#endif

// X86 CPUs have xgetbv to detect OS saves high parts of ymm registers.
#if !defined(__CLR_VER) && defined(_M_X64) && \
    defined(_MSC_VER) && (_MSC_FULL_VER >= 160040219)
#define HAS_XGETBV
static uint32 XGetBV(unsigned int xcr) {
  return static_cast<uint32>(_xgetbv(xcr));
}
#elif !defined(__CLR_VER) && defined(_M_IX86)
#define HAS_XGETBV
__declspec(naked) __declspec(align(16))
static uint32 XGetBV(unsigned int xcr) {
  __asm {
    mov        ecx, [esp + 4]    // xcr
    _asm _emit 0x0f _asm _emit 0x01 _asm _emit 0xd0  // xgetbv for vs2005.
    ret
  }
}
#elif defined(__i386__) || defined(__x86_64__)
#define HAS_XGETBV
static uint32 XGetBV(unsigned int xcr) {
  uint32 xcr_feature_mask;
  asm volatile (  // NOLINT
    ".byte 0x0f, 0x01, 0xd0\n"
    : "=a"(xcr_feature_mask)
    : "c"(xcr)
    : "memory", "cc", "edx");  // edx unused.
  return xcr_feature_mask;
}
#endif
#ifdef HAS_XGETBV
static const int kXCR_XFEATURE_ENABLED_MASK = 0;
#endif

// based on libvpx arm_cpudetect.c
// For Arm, but public to allow testing on any CPU
LIBYUV_API
int ArmCpuCaps(const char* cpuinfo_name) {
  FILE* f = fopen(cpuinfo_name, "r");
  if (f) {
    char buf[512];
    while (fgets(buf, 511, f)) {
      if (memcmp(buf, "Features", 8) == 0) {
        char* p = strstr(buf, " neon");
        if (p && (p[5] == ' ' || p[5] == '\n')) {
          fclose(f);
          return kCpuHasNEON;
        }
      }
    }
    fclose(f);
  }
  return 0;
}

#if defined(__mips__) && defined(__linux__)
static int MipsCpuCaps(const char* search_string) {
  const char* file_name = "/proc/cpuinfo";
  char cpuinfo_line[256];
  FILE* f = NULL;
  if ((f = fopen(file_name, "r")) != NULL) {
    while (fgets(cpuinfo_line, sizeof(cpuinfo_line), f) != NULL) {
      if (strstr(cpuinfo_line, search_string) != NULL) {
        fclose(f);
        return kCpuHasMIPS_DSP;
      }
    }
    fclose(f);
  }
  /* Did not find string in the proc file, or not Linux ELF. */
  return 0;
}
#endif

// CPU detect function for SIMD instruction sets.
LIBYUV_API
int cpu_info_ = kCpuInit;  // cpu_info is not initialized yet.

// Test environment variable for disabling CPU features. Any non-zero value
// to disable. Zero ignored to make it easy to set the variable on/off.
static bool TestEnv(const char* name) {
  const char* var = getenv(name);
  if (var) {
    if (var[0] != '0') {
      return true;
    }
  }
  return false;
}

LIBYUV_API
int InitCpuFlags(void) {
#if !defined(__CLR_VER) && defined(CPU_X86)
  int cpu_info[4] = { 0, 0, 0, 0 };
  __cpuid(cpu_info, 1);
  cpu_info_ = ((cpu_info[3] & 0x04000000) ? kCpuHasSSE2 : 0) |
              ((cpu_info[2] & 0x00000200) ? kCpuHasSSSE3 : 0) |
              ((cpu_info[2] & 0x00080000) ? kCpuHasSSE41 : 0) |
              ((cpu_info[2] & 0x00100000) ? kCpuHasSSE42 : 0) |
              ((cpu_info[2] & 0x00400000) ? kCpuHasMOVBE : 0) |
              (((cpu_info[2] & 0x18000000) == 0x18000000) ? kCpuHasAVX : 0) |
              kCpuHasX86;
#ifdef HAS_XGETBV
  if (cpu_info_ & kCpuHasAVX) {
    __cpuid(cpu_info, 7);
    if ((cpu_info[1] & 0x00000020) &&
        ((XGetBV(kXCR_XFEATURE_ENABLED_MASK) & 0x06) == 0x06)) {
      cpu_info_ |= kCpuHasAVX2;
    }
  }
#endif
  // Environment variable overrides for testing.
  if (TestEnv("LIBYUV_DISABLE_X86")) {
    cpu_info_ &= ~kCpuHasX86;
  }
  if (TestEnv("LIBYUV_DISABLE_SSE2")) {
    cpu_info_ &= ~kCpuHasSSE2;
  }
  if (TestEnv("LIBYUV_DISABLE_SSSE3")) {
    cpu_info_ &= ~kCpuHasSSSE3;
  }
  if (TestEnv("LIBYUV_DISABLE_SSE41")) {
    cpu_info_ &= ~kCpuHasSSE41;
  }
  if (TestEnv("LIBYUV_DISABLE_SSE42")) {
    cpu_info_ &= ~kCpuHasSSE42;
  }
  if (TestEnv("LIBYUV_DISABLE_MOVBE")) {
    cpu_info_ &= ~kCpuHasMOVBE;
  }
  if (TestEnv("LIBYUV_DISABLE_AVX")) {
    cpu_info_ &= ~kCpuHasAVX;
  }
  if (TestEnv("LIBYUV_DISABLE_AVX2")) {
    cpu_info_ &= ~kCpuHasAVX2;
  }
#elif defined(__mips__) && defined(__linux__)
  // Linux mips parse text file for dsp detect.
  cpu_info_ = MipsCpuCaps("dsp");  // set kCpuHasMIPS_DSP.
#if defined(__mips_dspr2)
  cpu_info_ |= kCpuHasMIPS_DSPR2;
#endif
  cpu_info_ |= kCpuHasMIPS;

  if (getenv("LIBYUV_DISABLE_MIPS")) {
    cpu_info_ &= ~kCpuHasMIPS;
  }
  if (getenv("LIBYUV_DISABLE_MIPS_DSP")) {
    cpu_info_ &= ~kCpuHasMIPS_DSP;
  }
  if (getenv("LIBYUV_DISABLE_MIPS_DSPR2")) {
    cpu_info_ &= ~kCpuHasMIPS_DSPR2;
  }
#elif defined(__arm__)
#if defined(__linux__) && (defined(__ARM_NEON__) || defined(LIBYUV_NEON))
  // Linux arm parse text file for neon detect.
  cpu_info_ = ArmCpuCaps("/proc/cpuinfo");
#elif defined(__ARM_NEON__)
  // gcc -mfpu=neon defines __ARM_NEON__
  // Enable Neon if you want support for Neon and Arm, and use MaskCpuFlags
  // to disable Neon on devices that do not have it.
  cpu_info_ = kCpuHasNEON;
#endif
  cpu_info_ |= kCpuHasARM;
  if (TestEnv("LIBYUV_DISABLE_NEON")) {
    cpu_info_ &= ~kCpuHasNEON;
  }
#endif  // __arm__
  if (TestEnv("LIBYUV_DISABLE_ASM")) {
    cpu_info_ = 0;
  }
  return cpu_info_;
}

LIBYUV_API
void MaskCpuFlags(int enable_flags) {
  cpu_info_ = InitCpuFlags() & enable_flags;
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif
