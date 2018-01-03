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

#ifndef WebRTCTransport_h
#define WebRTCTransport_h

#include <MediaDefinitions.h>
#include <MediaDefinitionExtra.h>
#include <rtputils.h>
#include <webrtc/common_types.h>
#include <webrtc/api/call/transport.h>

namespace woogeen_base {

template<erizoExtra::DataType dataType>
class WebRTCTransport : public webrtc::Transport, public erizo::FeedbackSource {
public:
    WebRTCTransport(erizoExtra::RTPDataReceiver*, erizo::FeedbackSink*);
    virtual ~WebRTCTransport();

    virtual bool SendRtp(const uint8_t* data, size_t len, const webrtc::PacketOptions& options);
    virtual bool SendRtcp(const uint8_t* data, size_t len);
private:
    erizoExtra::RTPDataReceiver* m_rtpReceiver;
};

template<erizoExtra::DataType dataType>
WebRTCTransport<dataType>::WebRTCTransport(erizoExtra::RTPDataReceiver* rtpReceiver, erizo::FeedbackSink* feedbackSink)
    : m_rtpReceiver(rtpReceiver)
{
    fb_sink_ = feedbackSink;
}

template<erizoExtra::DataType dataType>
WebRTCTransport<dataType>::~WebRTCTransport()
{
}

template<erizoExtra::DataType dataType>
inline bool WebRTCTransport<dataType>::SendRtp(const uint8_t* data, size_t len, const webrtc::PacketOptions& options)
{
    //Ignore PacketOption here, and pass a fake channel id to receiver
    int ret = 0;
    if (m_rtpReceiver) {
        m_rtpReceiver->receiveRtpData(reinterpret_cast<char*>(const_cast<uint8_t*>(data)), len, dataType, 0);
        ret = len;
    }

    return ret;
}

template<erizoExtra::DataType dataType>
inline bool WebRTCTransport<dataType>::SendRtcp(const uint8_t* data, size_t len)
{
    // FIXME: We need add a new interface to the RTPDataReceiver for RTCP Sender Reports.
    const RTCPHeader* chead = reinterpret_cast<const RTCPHeader*>(data);
    uint8_t packetType = chead->getPacketType();
    if (packetType == RTCP_Sender_PT) {
        int ret = 0;
        if (m_rtpReceiver) {
            m_rtpReceiver->receiveRtpData(reinterpret_cast<char*>(const_cast<uint8_t*>(data)), len, dataType, 0);
            ret = len;
        }
        return ret;
    }

    erizo::packetType p = (dataType == erizoExtra::AUDIO) ? erizo::AUDIO_PACKET : erizo::VIDEO_PACKET;
    return fb_sink_ ? fb_sink_->deliverFeedback(std::make_shared<erizo::DataPacket>(0, reinterpret_cast<char*>(const_cast<uint8_t*>(data)), len, p)) : 0;
}

}
#endif /* WebRTCTransport_h */
