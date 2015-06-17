/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
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

#ifndef EncodedVideoFrameSender_h
#define EncodedVideoFrameSender_h

#include "VideoFrameSender.h"
#include "WebRTCTaskRunner.h"
#include "WebRTCTransport.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <webrtc/modules/bitrate_controller/include/bitrate_controller.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>

namespace woogeen_base {

/**
 * This is the class to accept the encoded frame with the given format,
 * packetize the frame and send them out via the given WebRTCTransport.
 * It also gives the feedback to the encoder based on the feedback from the remote.
 */
class EncodedVideoFrameSender : public VideoFrameSender,
                                public erizo::FeedbackSink,
                                public IntraFrameCallback,
                                public webrtc::BitrateObserver,
                                public webrtc::RtcpIntraFrameObserver {
    DECLARE_LOGGER();

public:
    EncodedVideoFrameSender(int id, boost::shared_ptr<VideoFrameProvider>, FrameFormat, int targetKbps, WebRTCTransport<erizo::VIDEO>*, boost::shared_ptr<WebRTCTaskRunner>);
    ~EncodedVideoFrameSender();

    // Implements VideoFrameSender.
    void onFrame(FrameFormat, unsigned char* payload, int len, unsigned int ts);
    bool setSendCodec(FrameFormat, unsigned int width, unsigned int height);
    bool updateVideoSize(unsigned int width, unsigned int height);
    bool startSend(bool nack, bool fec) { return true; }
    bool stopSend(bool nack, bool fec) { return true; }
    uint32_t sendSSRC(bool nack, bool fec);
    IntraFrameCallback* iFrameCallback() { return this; }
    erizo::FeedbackSink* feedbackSink() { return this; }
    bool acceptRawFrame() { return false; }
    void registerPreSendFrameCallback(FrameConsumer*);
    void deRegisterPreSendFrameCallback();

    // Implements FeedbackSink.
    int deliverFeedback(char* buf, int len);

    // Implements IntraFrameCallback.
    void handleIntraFrameRequest();

    // Implements webrtc::RtcpIntraFrameObserver.
    void OnReceivedIntraFrameRequest(uint32_t ssrc);
    void OnReceivedSLI(uint32_t ssrc, uint8_t picture_id) { }
    void OnReceivedRPSI(uint32_t ssrc, uint64_t picture_id) { }
    void OnLocalSsrcChanged(uint32_t old_ssrc, uint32_t new_ssrc) { }

    // Implements webrtc::BitrateObserver.
    void OnNetworkChanged(const uint32_t target_bitrate, const uint8_t fraction_loss, const int64_t rtt);

private:
    bool init(WebRTCTransport<erizo::VIDEO>*, boost::shared_ptr<WebRTCTaskRunner>);
    void close();

    boost::scoped_ptr<webrtc::BitrateController> m_bitrateController;
    boost::scoped_ptr<webrtc::RtcpBandwidthObserver> m_bandwidthObserver;
    boost::scoped_ptr<webrtc::RtpRtcp> m_rtpRtcp;

    boost::shared_ptr<webrtc::Transport> m_videoTransport;
    boost::shared_ptr<WebRTCTaskRunner> m_taskRunner;
    boost::shared_ptr<VideoFrameProvider> m_source;
    FrameFormat m_frameFormat;
    int m_targetKbps;

    FrameConsumer* m_frameConsumer;
};

}
#endif /* EncodedVideoFrameSender_h */
