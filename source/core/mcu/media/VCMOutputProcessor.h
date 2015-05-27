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

#ifndef VCMOutputProcessor_h
#define VCMOutputProcessor_h

#include "VideoFrameMixer.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <MediaRecording.h>
#include <VideoFrameSender.h>
#include <WebRTCTaskRunner.h>
#include <WebRTCTransport.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/system_wrappers/interface/scoped_refptr.h>
#include <webrtc/video_engine/payload_router.h>
#include <webrtc/video_engine/vie_encoder.h>

namespace mcu {

/**
 * This is the class to accept the composited raw frame and encode it.
 * It then sends out the encoded data via the given WebRTCTransport.
 * It also handles the feedback from the remote.
 */
class VCMOutputProcessor : public woogeen_base::VideoFrameSender, public erizo::FeedbackSink, public woogeen_base::IntraFrameCallback {
    DECLARE_LOGGER();

public:
    VCMOutputProcessor(int id, boost::shared_ptr<woogeen_base::VideoFrameProvider>, int targetKbps, woogeen_base::WebRTCTransport<erizo::VIDEO>*, boost::shared_ptr<woogeen_base::WebRTCTaskRunner>);
    ~VCMOutputProcessor();

    // Implements VideoFrameSender.
    void onFrame(woogeen_base::FrameFormat, unsigned char* payload, int len, unsigned int ts);
    bool setSendCodec(woogeen_base::FrameFormat, unsigned int width, unsigned int height);
    bool updateVideoSize(unsigned int width, unsigned int height);
    bool startSend(bool nack, bool fec);
    bool stopSend(bool nack, bool fec);
    uint32_t sendSSRC(bool nack, bool fec);
    woogeen_base::IntraFrameCallback* iFrameCallback() { return this; }
    erizo::FeedbackSink* feedbackSink() { return this; }

    void registerPreSendFrameCallback(woogeen_base::MediaFrameQueue& videoQueue);
    void deRegisterPreSendFrameCallback();

    // Implements FeedbackSink.
    int deliverFeedback(char* buf, int len);

    // Implements IntraFrameCallback.
    void handleIntraFrameRequest();

private:
    bool init(woogeen_base::WebRTCTransport<erizo::VIDEO>*, boost::shared_ptr<woogeen_base::WebRTCTaskRunner>);
    void close();

    woogeen_base::FrameFormat m_sendFormat;
    int m_targetKbps;

    boost::scoped_ptr<webrtc::BitrateController> m_bitrateController;
    boost::scoped_ptr<webrtc::RtcpBandwidthObserver> m_bandwidthObserver;
    webrtc::scoped_refptr<webrtc::PayloadRouter> m_payloadRouter;
    boost::scoped_ptr<webrtc::ViEEncoder> m_videoEncoder;
    std::list<webrtc::RtpRtcp*> m_rtpRtcps;

    boost::shared_ptr<webrtc::Transport> m_videoTransport;
    boost::shared_ptr<woogeen_base::WebRTCTaskRunner> m_taskRunner;
    boost::shared_ptr<woogeen_base::VideoFrameProvider> m_source;

    boost::scoped_ptr<woogeen_base::VideoEncodedFrameCallbackAdapter> m_encodedFrameCallback;
};

} /* namespace mcu */
#endif /* VCMOutputProcessor_h */
