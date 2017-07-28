/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

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
