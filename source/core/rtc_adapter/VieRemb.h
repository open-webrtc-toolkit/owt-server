/*
 * Copyright (c) 2011, The WebRTC project authors. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 *   * Neither the name of Google nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WEBRTC_VIDEO_VIE_REMB_H_
#define WEBRTC_VIDEO_VIE_REMB_H_

#include <list>
#include <utility>
#include <vector>

#include "webrtc/base/criticalsection.h"
#include "webrtc/modules/include/module.h"
#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.h"

namespace webrtc {

class ProcessThread;
class RtpRtcp;

class VieRemb : public RemoteBitrateObserver {
 public:
  explicit VieRemb(Clock* clock);
  ~VieRemb();

  // Called to add a module that can generate and send REMB RTCP.
  void AddRembSender(RtpRtcp* rtp_rtcp);

  // Removes a REMB RTCP sender.
  void RemoveRembSender(RtpRtcp* rtp_rtcp);

  // Returns true if the instance is in use, false otherwise.
  bool InUse() const;

  // Called every time there is a new bitrate estimate for a receive channel
  // group. This call will trigger a new RTCP REMB packet if the bitrate
  // estimate has decreased or if no RTCP REMB packet has been sent for
  // a certain time interval.
  // Implements RtpReceiveBitrateUpdate.
  virtual void OnReceiveBitrateChanged(const std::vector<uint32_t>& ssrcs,
                                       uint32_t bitrate);

 private:
  typedef std::list<RtpRtcp*> RtpModules;

  Clock* const clock_;
  rtc::CriticalSection list_crit_;

  // The last time a REMB was sent.
  int64_t last_remb_time_;
  uint32_t last_send_bitrate_;

  // All modules that can send REMB RTCP.
  RtpModules rtcp_sender_;

  // The last bitrate update.
  uint32_t bitrate_;
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_VIE_REMB_H_
