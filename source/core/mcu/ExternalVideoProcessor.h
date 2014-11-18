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

#ifndef ExternalVideoProcessor_h
#define ExternalVideoProcessor_h

#include "Config.h"
#include "VideoOutputProcessor.h"

#include <logger.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>

namespace mcu {

class TaskRunner;

/**
 * This is the class to accept the encoded frame from external, packetize the frame and
 * send them out via the given WoogeenTransport. It also gives the feedback
 * to the encoder based on the feedback from the remote.
 */
class ExternalVideoProcessor : public VideoOutputProcessor,
                               public erizo::FeedbackSink,
                               public ConfigListener,
                               public webrtc::RtcpIntraFrameObserver {
    DECLARE_LOGGER();

public:
    ExternalVideoProcessor(int id);
    ~ExternalVideoProcessor();

    // Implements VideoOutputProcessor.
    bool init(woogeen_base::WoogeenTransport<erizo::VIDEO>*, boost::shared_ptr<TaskRunner>);
    void close();

    bool setSendVideoCodec(const webrtc::VideoCodec&);
    void onRequestIFrame();
    uint32_t sendSSRC();
    int sendFrame(char* payload, int len);
    erizo::FeedbackSink* feedbackSink() { return this; }

    // Implements FeedbackSink.
    int deliverFeedback(char* buf, int len);

    // Implements ConfigListener.
    void onConfigChanged();

    // Implements webrtc::RtcpIntraFrameObserver.
    void OnReceivedIntraFrameRequest(uint32_t ssrc);
    void OnReceivedSLI(uint32_t ssrc, uint8_t picture_id) { }
    void OnReceivedRPSI(uint32_t ssrc, uint64_t picture_id) { }
    void OnLocalSsrcChanged(uint32_t old_ssrc, uint32_t new_ssrc) { }

private:
    boost::scoped_ptr<webrtc::RtpRtcp> m_rtpRtcp;

    boost::shared_ptr<webrtc::Transport> m_videoTransport;
    boost::shared_ptr<TaskRunner> m_taskRunner;
};

}
#endif /* ExternalVideoProcessor_h */
