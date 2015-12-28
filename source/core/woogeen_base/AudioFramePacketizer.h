/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#ifndef AudioFramePacketizer_h
#define AudioFramePacketizer_h

#include "MediaFramePipeline.h"
#include "WebRTCTransport.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <MediaDefinitions.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>

namespace woogeen_base {

class WebRTCTaskRunner;

/**
 * This is the class to send out the audio frame with a given format.
 */
class AudioFramePacketizer : public FrameDestination,
                             public erizo::MediaSource,
                             public erizo::FeedbackSink,
                             public erizo::RTPDataReceiver {
public:
    AudioFramePacketizer(erizo::MediaSink* audioSink, erizo::FeedbackSource* fbSource);
    ~AudioFramePacketizer();

    // Implements FrameDestination.
    void onFrame(const Frame&);

    // Implements FeedbackSink.
    int deliverFeedback(char* buf, int len);

    // Implements erizo::MediaSource.
    int sendFirPacket() {return 0;}

    // Implements RTPDataReceiver.
    void receiveRtpData(char*, int len, erizo::DataType, uint32_t channelId);

private:
    bool init();
    bool setSendCodec(FrameFormat format);
    void close();

    boost::scoped_ptr<webrtc::RtpRtcp> m_rtpRtcp;

    boost::shared_ptr<webrtc::Transport> m_audioTransport;
    boost::shared_ptr<WebRTCTaskRunner> m_taskRunner;
    FrameFormat m_frameFormat;
};

}
#endif /* AudioFramePacketizer_h */
