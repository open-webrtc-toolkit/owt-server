// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "SsrcGenerator.h"
#include "webrtc/base/timeutils.h"
#include "webrtc/base/checks.h"

namespace owt_base {

SsrcGenerator* SsrcGenerator::GetSsrcGenerator() {
  return webrtc::GetStaticInstance<SsrcGenerator>(webrtc::kAddRef);
}

void SsrcGenerator::ReturnSsrcGenerator() {
  webrtc::GetStaticInstance<SsrcGenerator>(webrtc::kRelease);
}

uint32_t SsrcGenerator::CreateSsrc() {
  rtc::CritScope lock(&crit_);

  while (true) {  // Try until get a new ssrc.
    uint32_t ssrc = random_.Rand(1u, 0xfffffffe);
    if (ssrcs_.insert(ssrc).second) {
      return ssrc;
    }
  }
}

void SsrcGenerator::RegisterSsrc(uint32_t ssrc) {
  rtc::CritScope lock(&crit_);
  ssrcs_.insert(ssrc);
}

void SsrcGenerator::ReturnSsrc(uint32_t ssrc) {
  rtc::CritScope lock(&crit_);
  ssrcs_.erase(ssrc);
}

SsrcGenerator::SsrcGenerator() : random_(rtc::TimeMicros()) {}

SsrcGenerator::~SsrcGenerator() {}

}  // namespace owt_base
