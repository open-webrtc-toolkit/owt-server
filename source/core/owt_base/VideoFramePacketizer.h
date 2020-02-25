// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFramePacketizer_h
#define VideoFramePacketizer_h

#include "WebRTCTaskRunner.h"
#include "WebRTCTransport.h"
#include "MediaFramePipeline.h"
#include "SsrcGenerator.h"

#include <MediaDefinitions.h>
#include <MediaDefinitionExtra.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <modules/rtp_rtcp/include/rtp_rtcp.h>
#include <modules/rtp_rtcp/include/rtp_rtcp_defines.h>
#include <modules/rtp_rtcp/source/rtp_sender_video.h>
#include <api/transport/field_trial_based_config.h>

#include <rtc_base/random.h>
#include <rtc_base/time_utils.h>
#include <rtc_base/rate_limiter.h>

namespace owt_base {
/**
 * This is the class to accept the encoded frame with the given format,
 * packetize the frame and send them out via the given WebRTCTransport.
 * It also gives the feedback to the encoder based on the feedback from the remote.
 */
class VideoFramePacketizer : public FrameDestination,
                             public erizo::MediaSource,
                             public erizo::FeedbackSink,
                             public erizoExtra::RTPDataReceiver,
                             public webrtc::RtcpIntraFrameObserver {
    DECLARE_LOGGER();

public:
    VideoFramePacketizer(bool enableRed,
                         bool enableUlpfec,
                         bool enableTransportcc = true,
                         bool selfRequestKeyframe = false,
                         uint32_t transportccExt = 0);
    ~VideoFramePacketizer();

    void bindTransport(erizo::MediaSink* sink);
    void unbindTransport();
    void enable(bool enabled);
    uint32_t getSsrc() { return m_ssrc; }

    // Implements FrameDestination.
    void onFrame(const Frame&);
    void onVideoSourceChanged() override;

    // Implements erizo::MediaSource.
    int sendFirPacket();

    // Implements RTPDataReceiver.
    void receiveRtpData(char*, int len, erizoExtra::DataType, uint32_t channelId);

    // Implements webrtc::RtcpIntraFrameObserver.
    void OnReceivedIntraFrameRequest(uint32_t ssrc);
    void OnReceivedSLI(uint32_t ssrc, uint8_t picture_id) { }
    void OnReceivedRPSI(uint32_t ssrc, uint64_t picture_id) { }
    void OnLocalSsrcChanged(uint32_t old_ssrc, uint32_t new_ssrc) { }

private:
    bool init(bool enableRed, bool enableUlpfec, bool enableTransportcc, uint32_t transportccExt);
    void close();

    // Implement erizo::FeedbackSink
    int deliverFeedback_(std::shared_ptr<erizo::DataPacket> data_packet);
    // Implement erizo::MediaSource
    int sendPLI();

    bool m_enabled;
    bool m_enableDump;
    bool m_keyFrameArrived;
    bool m_selfRequestKeyframe;
    std::unique_ptr<webrtc::RateLimiter> m_retransmissionRateLimiter;
    // boost::scoped_ptr<webrtc::BitrateController> m_bitrateController;
    boost::scoped_ptr<webrtc::RtcpBandwidthObserver> m_bandwidthObserver;
    std::unique_ptr<webrtc::RtpRtcp> m_rtpRtcp;
    boost::shared_mutex m_rtpRtcpMutex;

    boost::shared_ptr<webrtc::Transport> m_videoTransport;
    boost::shared_ptr<WebRTCTaskRunner> m_taskRunner;
    FrameFormat m_frameFormat;
    uint16_t m_frameWidth;
    uint16_t m_frameHeight;
    webrtc::Random m_random;
    uint32_t m_ssrc;
    SsrcGenerator* const m_ssrcGenerator;

    boost::shared_mutex m_transportMutex;

    uint16_t m_sendFrameCount;
    webrtc::Clock *m_clock;
    int64_t m_timeStampOffset;

    std::unique_ptr<webrtc::RtcEventLog> m_eventLog;
    std::unique_ptr<webrtc::RTPSenderVideo> m_senderVideo;
    std::unique_ptr<webrtc::PlayoutDelayOracle> m_playoutDelayOracle;
    std::unique_ptr<webrtc::FieldTrialBasedConfig> m_fieldTrialConfig;
};

}
#endif /* EncodedVideoFrameSender_h */
