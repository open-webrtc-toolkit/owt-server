// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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
