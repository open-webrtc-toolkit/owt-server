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

#include "SsrcGenerator.h"
#include "webrtc/base/timeutils.h"
#include "webrtc/base/checks.h"

namespace woogeen_base {

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

}  // namespace woogeen_base
