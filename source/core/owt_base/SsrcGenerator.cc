// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "SsrcGenerator.h"
#include "rtc_base/time_utils.h"
#include "rtc_base/checks.h"

namespace owt_base {

SsrcGenerator* SsrcGenerator::GetSsrcGenerator() {
  static SsrcGenerator inst;
  return &inst;
}

uint32_t SsrcGenerator::CreateSsrc() {
  mutex_.Lock();

  while (true) {  // Try until get a new ssrc.
    uint32_t ssrc = random_.Rand(1u, 0xfffffffe);
    if (ssrcs_.insert(ssrc).second) {
      mutex_.Unlock();
      return ssrc;
    }
  }
  mutex_.Unlock();
}

void SsrcGenerator::RegisterSsrc(uint32_t ssrc) {
  mutex_.Lock();
  ssrcs_.insert(ssrc);
  mutex_.Unlock();

}

void SsrcGenerator::ReturnSsrc(uint32_t ssrc) {
  mutex_.Lock();
  ssrcs_.erase(ssrc);
  mutex_.Unlock();
}

SsrcGenerator::SsrcGenerator() : random_(rtc::TimeMicros()) {}

SsrcGenerator::~SsrcGenerator() {}

}  // namespace owt_base
