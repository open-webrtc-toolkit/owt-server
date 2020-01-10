// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SsrcGenerator_h
#define SsrcGenerator_h

#include <set>

#include "rtc_base/critical_section.h"
#include "rtc_base/random.h"
#include "base/thread_annotations.h"

namespace owt_base {

class SsrcGenerator {
 public:
  static SsrcGenerator* GetSsrcGenerator();

  uint32_t CreateSsrc();
  void RegisterSsrc(uint32_t ssrc);
  void ReturnSsrc(uint32_t ssrc);

 protected:
  SsrcGenerator();
  ~SsrcGenerator();

 private:
  rtc::CriticalSection crit_;
  webrtc::Random random_ GUARDED_BY(crit_);
  std::set<uint32_t> ssrcs_ GUARDED_BY(crit_);
};
}  // namespace owt_base

#endif  // SsrcGenerator_h
