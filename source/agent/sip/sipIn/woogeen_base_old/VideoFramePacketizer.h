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

#ifndef VideoFramePacketizer_h
#define VideoFramePacketizer_h

#include "WebRTCTaskRunner.h"
#include "WebRTCTransport.h"
#include "MediaFramePipeline.h"
#include "SsrcGenerator.h"

#include <MediaDefinitions.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <webrtc/modules/bitrate_controller/include/bitrate_controller.h>
#include <webrtc/modules/rtp_rtcp/include/rtp_rtcp.h>
#include <webrtc/logging/rtc_event_log/rtc_event_log.h>
#include <webrtc/base/random.h>
#include <webrtc/base/timeutils.h>
#include <webrtc/base/rate_limiter.h>

namespace woogeen_base {
/**
 * This is the class to accept the encoded frame with the given format,
 * packetize the frame and send them out via the given WebRTCTransport.
 * It also gives the feedback to the encoder based on the feedback from the remote.
 */
class VideoFramePacketizer : public FrameDestination,
                             public erizo::MediaSource,
                             public erizo::FeedbackSink,
                             public erizo::RTPDataReceiver,
                             public webrtc::BitrateObserver,
                             public webrtc::RtcpIntraFrameObserver {
    DECLARE_LOGGER();

public:
    VideoFramePacketizer(bool enableRed, bool enableUlpfec);
    ~VideoFramePacketizer();

    void bindTransport(erizo::MediaSink* sink);
    void unbindTransport();
    void enable(bool enabled);

    // Implements FrameDestination.
    void onFrame(const Frame&);

    // Implements erizo::MediaSource.
    int sendFirPacket();

    // Implements FeedbackSink.
    int deliverFeedback(char* buf, int len);

    // Implements RTPDataReceiver.
    void receiveRtpData(char*, int len, erizo::DataType, uint32_t channelId);

    // Implements webrtc::RtcpIntraFrameObserver.
    void OnReceivedIntraFrameRequest(uint32_t ssrc);
    void OnReceivedSLI(uint32_t ssrc, uint8_t picture_id) { }
    void OnReceivedRPSI(uint32_t ssrc, uint64_t picture_id) { }
    void OnLocalSsrcChanged(uint32_t old_ssrc, uint32_t new_ssrc) { }

    // Implements webrtc::BitrateObserver.
    void OnNetworkChanged(const uint32_t target_bitrate, const uint8_t fraction_loss, const int64_t rtt);

private:
    bool init(bool enableRed, bool enableUlpfec);
    void close();
    bool setSendCodec(FrameFormat, unsigned int width, unsigned int height);

    bool m_enabled;
    bool m_enableDump;
    bool m_keyFrameArrived;
    std::unique_ptr<webrtc::RateLimiter> m_retransmissionRateLimiter;
    boost::scoped_ptr<webrtc::BitrateController> m_bitrateController;
    boost::scoped_ptr<webrtc::RtcpBandwidthObserver> m_bandwidthObserver;
    boost::scoped_ptr<webrtc::RtpRtcp> m_rtpRtcp;
    // Use dummy event logger
    webrtc::RtcEventLogNullImpl m_rtcEventLog;
    boost::shared_mutex m_rtpRtcpMutex;

    boost::shared_ptr<webrtc::Transport> m_videoTransport;
    boost::shared_ptr<WebRTCTaskRunner> m_taskRunner;
    FrameFormat m_frameFormat;
    uint16_t m_frameWidth;
    uint16_t m_frameHeight;
    webrtc::Random m_random;
    uint32_t m_ssrc;
    SsrcGenerator* const m_ssrc_generator;

    boost::shared_mutex m_transport_mutex;
};

}
#endif /* EncodedVideoFrameSender_h */
