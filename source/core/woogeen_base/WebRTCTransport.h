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

#ifndef WebRTCTransport_h
#define WebRTCTransport_h

#include <MediaDefinitions.h>
#include <rtputils.h>
#include <webrtc/common_types.h>

namespace woogeen_base {

template<erizo::DataType dataType>
class WebRTCTransport : public webrtc::Transport {
public:
    WebRTCTransport(erizo::RTPDataReceiver*, erizo::FeedbackSink*);
    virtual ~WebRTCTransport();

    virtual int SendPacket(int channel, const void* data, size_t len);
    virtual int SendRTCPPacket(int channel, const void* data, size_t len);

    void setRTPReceiver(erizo::RTPDataReceiver*);
    void setFeedbackSink(erizo::FeedbackSink*);

private:
    erizo::RTPDataReceiver* m_rtpReceiver;
    erizo::FeedbackSink* m_feedbackSink;
};

template<erizo::DataType dataType>
WebRTCTransport<dataType>::WebRTCTransport(erizo::RTPDataReceiver* rtpReceiver, erizo::FeedbackSink* feedbackSink)
    : m_rtpReceiver(rtpReceiver)
    , m_feedbackSink(feedbackSink)
{
}

template<erizo::DataType dataType>
WebRTCTransport<dataType>::~WebRTCTransport()
{
}

template<erizo::DataType dataType>
inline void WebRTCTransport<dataType>::setRTPReceiver(erizo::RTPDataReceiver* rtpReceiver)
{
    m_rtpReceiver = rtpReceiver;
}

template<erizo::DataType dataType>
inline void WebRTCTransport<dataType>::setFeedbackSink(erizo::FeedbackSink* feedbackSink)
{
    m_feedbackSink = feedbackSink;
}

template<erizo::DataType dataType>
inline int WebRTCTransport<dataType>::SendPacket(int channel, const void* data, size_t len)
{
    int ret = 0;
    if (m_rtpReceiver) {
        m_rtpReceiver->receiveRtpData(reinterpret_cast<char*>(const_cast<void*>(data)), len, dataType, channel);
        ret = len;
    }

    return ret;
}

template<erizo::DataType dataType>
inline int WebRTCTransport<dataType>::SendRTCPPacket(int channel, const void* data, size_t len)
{
    // FIXME: We need add a new interface to the RTPDataReceiver for RTCP Sender Reports.
    const RTCPHeader* chead = reinterpret_cast<const RTCPHeader*>(data);
    uint8_t packetType = chead->getPacketType();
    if (packetType == RTCP_Sender_PT) {
        int ret = 0;
        if (m_rtpReceiver) {
            m_rtpReceiver->receiveRtpData(reinterpret_cast<char*>(const_cast<void*>(data)), len, dataType, channel);
            ret = len;
        }
        return ret;
    }

    return m_feedbackSink ? m_feedbackSink->deliverFeedback(reinterpret_cast<char*>(const_cast<void*>(data)), len) : 0;
}

}
#endif /* WebRTCTransport_h */
