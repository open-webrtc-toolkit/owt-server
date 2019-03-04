// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SsrcGenerator_h
#define SsrcGenerator_h

#include <set>

#include "webrtc/base/criticalsection.h"
#include "webrtc/base/random.h"
#include "webrtc/base/thread_annotations.h"
#include "webrtc/system_wrappers/include/static_instance.h"
#include "webrtc/typedefs.h"

namespace woogeen_base {

class SsrcGenerator {
 public:
  static SsrcGenerator* GetSsrcGenerator();
  static void ReturnSsrcGenerator();

  uint32_t CreateSsrc();
  void RegisterSsrc(uint32_t ssrc);
  void ReturnSsrc(uint32_t ssrc);

 protected:
  SsrcGenerator();
  ~SsrcGenerator();

  static SsrcGenerator* CreateInstance() { return new SsrcGenerator(); }

  // Friend function to allow the SSRC destructor to be accessed from the
  // template class.
  friend SsrcGenerator* webrtc::GetStaticInstance<SsrcGenerator>(
      CountOperation count_operation);

 private:
  rtc::CriticalSection crit_;
  webrtc::Random random_ GUARDED_BY(crit_);
  std::set<uint32_t> ssrcs_ GUARDED_BY(crit_);
};
}  // namespace woogeen_base

#endif  // SsrcGenerator_h
