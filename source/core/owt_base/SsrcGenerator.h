// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SsrcGenerator_h
#define SsrcGenerator_h

#include <set>

#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/thread_annotations.h"
#include "rtc_base/random.h"

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
  mutable webrtc::Mutex mutex_;
  webrtc::Random random_ RTC_GUARDED_BY(mutex_);
  std::set<uint32_t> ssrcs_ RTC_GUARDED_BY(mutex_);
};
}  // namespace owt_base

#endif  // SsrcGenerator_h
