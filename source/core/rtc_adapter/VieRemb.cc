/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "VieRemb.h"

#include <assert.h>
#include <algorithm>

#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp.h"
#include "webrtc/modules/utility/include/process_thread.h"

// This implmentation is borrowed from webrtc project to work as
// remote bitrate observer to handle outbound REMB.

namespace webrtc {

const int kRembSendIntervalMs = 200;

// % threshold for if we should send a new REMB asap.
const uint32_t kSendThresholdPercent = 97;

VieRemb::VieRemb(Clock* clock)
    : clock_(clock),
      last_remb_time_(clock_->TimeInMilliseconds()),
      last_send_bitrate_(0),
      bitrate_(0) {}

VieRemb::~VieRemb() {}

void VieRemb::AddRembSender(RtpRtcp* rtp_rtcp) {
  assert(rtp_rtcp);

  rtc::CritScope lock(&list_crit_);

  // Verify this module hasn't been added earlier.
  if (std::find(rtcp_sender_.begin(), rtcp_sender_.end(), rtp_rtcp) !=
      rtcp_sender_.end())
    return;
  rtcp_sender_.push_back(rtp_rtcp);
}

void VieRemb::RemoveRembSender(RtpRtcp* rtp_rtcp) {
  assert(rtp_rtcp);

  rtc::CritScope lock(&list_crit_);
  for (RtpModules::iterator it = rtcp_sender_.begin();
       it != rtcp_sender_.end(); ++it) {
    if ((*it) == rtp_rtcp) {
      rtcp_sender_.erase(it);
      return;
    }
  }
}

bool VieRemb::InUse() const {
  rtc::CritScope lock(&list_crit_);
  return !rtcp_sender_.empty();
}

void VieRemb::OnReceiveBitrateChanged(const std::vector<uint32_t>& ssrcs,
                                      uint32_t bitrate) {
  RtpRtcp* sender = nullptr;
  {
    rtc::CritScope lock(&list_crit_);
    // If we already have an estimate, check if the new total estimate is below
    // kSendThresholdPercent of the previous estimate.
    if (last_send_bitrate_ > 0) {
      uint32_t new_remb_bitrate = last_send_bitrate_ - bitrate_ + bitrate;

      if (new_remb_bitrate < kSendThresholdPercent * last_send_bitrate_ / 100) {
        // The new bitrate estimate is less than kSendThresholdPercent % of the
        // last report. Send a REMB asap.
        last_remb_time_ = clock_->TimeInMilliseconds() - kRembSendIntervalMs;
      }
    }
    bitrate_ = bitrate;

    // Calculate total receive bitrate estimate.
    int64_t now = clock_->TimeInMilliseconds();

    if (now - last_remb_time_ < kRembSendIntervalMs) {
      return;
    }
    last_remb_time_ = now;

    if (ssrcs.empty()) {
      return;
    }

    // Send a REMB packet.
    if (!rtcp_sender_.empty()) {
      sender = rtcp_sender_.front();
    }
    last_send_bitrate_ = bitrate_;
  }

  if (sender) {
    // (TODO:jianlin) At present BWE keeps decreasing bitrate once overuse detected,
    // but never bounce back. For this reason we cannot set REMB data in the observer.
    // Need a better policy to handle REMB request.
    //sender->SetREMBData(bitrate_, ssrcs);
  }
}

}  // namespace webrtc
