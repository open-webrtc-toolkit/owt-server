// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef QoSUtility_h
#define QoSUtility_h

#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <webrtc/modules/bitrate_controller/include/bitrate_controller.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>

namespace owt_base {

class ProtectedRTPSender;

/*
 * Observer class for the encoders, each encoder should implement this class
 * to get the target bitrate. It also get the fraction loss and rtt to
 * optimize its settings for this type of network. |target_bitrate| is the
 * target media/payload bitrate excluding packet headers, measured in bits
 * per second.
 */
// Callback class used for telling the user about how to configure the FEC,
// and the rates sent the last second is returned to the VCM.
class VideoFeedbackReactor : public webrtc::BitrateObserver, public webrtc::VCMProtectionCallback {
    DECLARE_LOGGER();
public:
    VideoFeedbackReactor(uint32_t id, boost::shared_ptr<ProtectedRTPSender>);
    ~VideoFeedbackReactor();

    virtual void OnNetworkChanged(
        const uint32_t target_bitrate,
        const uint8_t fraction_loss,
        const int64_t rtt);

    virtual int ProtectionRequest(
        const webrtc::FecProtectionParams* delta_params,
        const webrtc::FecProtectionParams* key_params,
        uint32_t* sent_video_rate_bps,
        uint32_t* sent_nack_rate_bps,
        uint32_t* sent_fec_rate_bps);

private:
    boost::shared_ptr<ProtectedRTPSender> m_rtpSender;
    webrtc::VideoCodingModule* m_vcm;
};

class WebRTCRtpData : public webrtc::RtpData {
public:
    WebRTCRtpData(uint32_t streamId, erizo::RTPDataReceiver*);
    virtual ~WebRTCRtpData();

    virtual int32_t OnReceivedPayloadData(
        const uint8_t* payloadData,
        const size_t payloadSize,
        const webrtc::WebRtcRTPHeader*);

    virtual bool OnRecoveredPacket(const uint8_t* packet, size_t packet_length);

private:
    uint32_t m_streamId;
    erizo::RTPDataReceiver* m_rtpReceiver;
};

} // namespace owt_base
#endif /* QoSUtility_h */
