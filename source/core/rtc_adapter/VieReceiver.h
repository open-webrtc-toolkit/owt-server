/*
 * Copyright (c) 2013, The WebRTC project authors. All rights reserved.
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

// Copyright (C) <2017> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef WEBRTC_VIDEO_ENGINE_VIE_RECEIVER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_RECEIVER_H_

#include <list>

#include "webrtc/base/criticalsection.h"
#include "webrtc/modules/rtp_rtcp/include/receive_statistics.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.h"

namespace webrtc {

#define kViEMaxMtu 1500

// Not enabling Flex-Fec unless all client SDK enable it.
class UlpfecReceiver;
class RemoteNtpTimeEstimator;
class ReceiveStatistics;
class RemoteBitrateEstimator;
class RtpHeaderParser;
class RTPPayloadRegistry;
class RtpReceiver;
class RtpRtcp;
class VideoCodingModule;

namespace vcm {
class VideoReceiver;
}

struct ReceiveBandwidthEstimatorStats;

class ViEReceiver : public RtpData {
 public:
/*
  ViEReceiver(VideoCodingModule* module_vcm,
              RemoteBitrateEstimator* remote_bitrate_estimator,
              RtpFeedback* rtp_feedback);
*/
  ViEReceiver(vcm::VideoReceiver* module_video_receiver,
              RemoteBitrateEstimator* remote_bitrate_estimator,
              RtpFeedback* rtp_feedback);
  ~ViEReceiver();

  bool SetReceiveCodec(const VideoCodec& video_codec);
  bool RegisterPayload(const VideoCodec& video_codec);

  void SetNackStatus(bool enable, int max_nack_reordering_threshold);
  void SetRtxPayloadType(int payload_type);
  void SetRtxSsrc(uint32_t ssrc);
  bool GetRtxSsrc(uint32_t* ssrc) const;

  bool IsFecEnabled() const;

  uint32_t GetRemoteSsrc() const;
  int GetCsrcs(uint32_t* csrcs) const;

  void SetRtpRtcpModule(RtpRtcp* module);

  RtpReceiver* GetRtpReceiver() const;

  void RegisterSimulcastRtpRtcpModules(const std::list<RtpRtcp*>& rtp_modules);

  bool SetReceiveTimestampOffsetStatus(bool enable, int id);
  bool SetReceiveAbsoluteSendTimeStatus(bool enable, int id);
  bool SetReceiveTransportSequenceNumberStatus(bool enable, int id);

  void StartReceive();
  void StopReceive();

  // Receives packets from external transport.
  int ReceivedRTPPacket(const void* rtp_packet, size_t rtp_packet_length,
                        const PacketTime& packet_time);
  int ReceivedRTCPPacket(const void* rtcp_packet, size_t rtcp_packet_length);

  // Implements RtpData.
  virtual int32_t OnReceivedPayloadData(
      const uint8_t* payload_data,
      size_t payload_size,
      const WebRtcRTPHeader* rtp_header) override;
  virtual bool OnRecoveredPacket(const uint8_t* packet,
                                 size_t packet_length) override;

  void GetReceiveBandwidthEstimatorStats(
      ReceiveBandwidthEstimatorStats* output) const;

  ReceiveStatistics* GetReceiveStatistics() const;

  void ReceivedBWEPacket(int64_t arrival_time_ms, size_t payload_size,
                         const RTPHeader& header);
 private:
  int InsertRTPPacket(const uint8_t* rtp_packet, size_t rtp_packet_length,
                      const PacketTime& packet_time);
  bool ReceivePacket(const uint8_t* packet,
                     size_t packet_length,
                     const RTPHeader& header,
                     bool in_order);
  // Parses and handles for instance RTX and RED headers.
  // This function assumes that it's being called from only one thread.
  bool ParseAndHandleEncapsulatingHeader(const uint8_t* packet,
                                         size_t packet_length,
                                         const RTPHeader& header);
  void NotifyReceiverOfFecPacket(const RTPHeader& header);
  int InsertRTCPPacket(const uint8_t* rtcp_packet, size_t rtcp_packet_length);
  bool IsPacketInOrder(const RTPHeader& header) const;
  bool IsPacketRetransmitted(const RTPHeader& header, bool in_order) const;
  void UpdateHistograms();

  rtc::CriticalSection receive_cs_;
  Clock* clock_;
  std::unique_ptr<RtpHeaderParser> rtp_header_parser_;
  std::unique_ptr<RTPPayloadRegistry> rtp_payload_registry_;
  std::unique_ptr<RtpReceiver> rtp_receiver_;
  std::unique_ptr<ReceiveStatistics> rtp_receive_statistics_;
  std::unique_ptr<UlpfecReceiver> fec_receiver_;
  RtpRtcp* rtp_rtcp_;
  std::list<RtpRtcp*> rtp_rtcp_simulcast_;
  //VideoCodingModule* vcm_;
  webrtc::vcm::VideoReceiver* video_receiver_;
  RemoteBitrateEstimator* remote_bitrate_estimator_;

  std::unique_ptr<RemoteNtpTimeEstimator> ntp_estimator_;

  bool receiving_;
  uint8_t restored_packet_[kViEMaxMtu];
  bool restored_packet_in_use_;
  bool receiving_ast_enabled_;
  int64_t last_packet_log_ms_;
};

}  // namespace webrt

#endif  // WEBRTC_VIDEO_ENGINE_VIE_RECEIVER_H_
