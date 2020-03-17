// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_ADAPTER_VIDEO_SEND_ADAPTER_
#define RTC_ADAPTER_VIDEO_SEND_ADAPTER_

#include <AdapterInternalDefinitions.h>
#include <RtcAdapter.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "MediaFramePipeline.h"
#include "SsrcGenerator.h"
#include "WebRTCTaskRunner.h"

#include <api/transport/field_trial_based_config.h>
#include <modules/rtp_rtcp/include/rtp_rtcp.h>
#include <modules/rtp_rtcp/include/rtp_rtcp_defines.h>
#include <modules/rtp_rtcp/source/rtp_sender_video.h>
#include <api/transport/network_control.h>
#include <api/transport/field_trial_based_config.h>
#include <call/rtp_transport_controller_send_interface.h>

#include <rtc_base/random.h>
#include <rtc_base/rate_limiter.h>
#include <rtc_base/time_utils.h>

namespace rtc_adapter {

class VideoSendAdapterImpl : public VideoSendAdapter,
                             public webrtc::Transport,
                             public webrtc::RtcpIntraFrameObserver,
                             public webrtc::TargetTransferRateObserver {
public:
    VideoSendAdapterImpl(CallOwner* owner, const RtcAdapter::Config& config);
    ~VideoSendAdapterImpl();

    // Implement VideoSendAdapter
    void onFrame(const owt_base::Frame&) override;
    int onRtcpData(char* data, int len) override;
    void reset() override;

    uint32_t ssrc() { return m_ssrc; }

    // Implement webrtc::Transport
    bool SendRtp(const uint8_t* packet,
        size_t length,
        const webrtc::PacketOptions& options) override;
    bool SendRtcp(const uint8_t* packet, size_t length) override;

    // Implements webrtc::RtcpIntraFrameObserver.
    void OnReceivedIntraFrameRequest(uint32_t ssrc);
    void OnReceivedSLI(uint32_t ssrc, uint8_t picture_id) {}
    void OnReceivedRPSI(uint32_t ssrc, uint64_t picture_id) {}
    void OnLocalSsrcChanged(uint32_t old_ssrc, uint32_t new_ssrc) {}

    //Implements webrtc::TargetTransferRateObjserver
    void OnTargetTransferRate(webrtc::TargetTransferRate) override;

private:
    bool init();

    bool m_enableDump;
    RtcAdapter::Config m_config;

    bool m_keyFrameArrived;
    std::unique_ptr<webrtc::RateLimiter> m_retransmissionRateLimiter;
    // boost::scoped_ptr<webrtc::BitrateController> m_bitrateController;
    boost::scoped_ptr<webrtc::RtcpBandwidthObserver> m_bandwidthObserver;
    std::unique_ptr<webrtc::RtpRtcp> m_rtpRtcp;
    boost::shared_mutex m_rtpRtcpMutex;

    boost::shared_ptr<webrtc::Transport> m_videoTransport;
    boost::shared_ptr<owt_base::WebRTCTaskRunner> m_taskRunner;
    owt_base::FrameFormat m_frameFormat;
    uint16_t m_frameWidth;
    uint16_t m_frameHeight;
    webrtc::Random m_random;
    uint32_t m_ssrc;
    owt_base::SsrcGenerator* const m_ssrcGenerator;

    boost::shared_mutex m_transportMutex;

    webrtc::Clock* m_clock;
    int64_t m_timeStampOffset;

    std::unique_ptr<webrtc::RtcEventLog> m_eventLog;
    std::unique_ptr<webrtc::RTPSenderVideo> m_senderVideo;
    std::unique_ptr<webrtc::FieldTrialBasedConfig> m_fieldTrialConfig;
    std::unique_ptr<webrtc::TaskQueueFactory> m_taskQueueFactory;
    std::unique_ptr<webrtc::RtpTransportControllerSendInterface> m_transportControllerSend;

    // Listeners
    AdapterFeedbackListener* m_feedbackListener;
    AdapterDataListener* m_rtpListener;
    AdapterStatsListener* m_statsListener;
};
}

#endif /* RTC_ADAPTER_VIDEO_SEND_ADAPTER_ */
