// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "InternalOut.h"

namespace owt_base {

InternalOut::InternalOut(const std::string& protocol, const std::string& dest_ip, unsigned int dest_port)
{
    if (protocol == "tcp")
        m_transport.reset(new owt_base::RawTransport<TCP>(this));
    else
        m_transport.reset(new owt_base::RawTransport<UDP>(this));

    m_transport->createConnection(dest_ip, dest_port);

    #ifdef BUILD_FOR_GST_ANALYTICS
    m_frameCount = 0;
    #endif
}

InternalOut::~InternalOut()
{
    m_transport->close();
}

void InternalOut::onFrame(const Frame& frame)
{
    char sendBuffer[sizeof(Frame) + 1];
    size_t header_len = sizeof(Frame);

    sendBuffer[0] = TDT_MEDIA_FRAME;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<Frame*>(&frame)), header_len);
    m_transport->sendData(sendBuffer, header_len + 1, reinterpret_cast<char*>(const_cast<uint8_t*>(frame.payload)), frame.length);
}

void InternalOut::onFrame(uint8_t *buffer, uint32_t length)
{
    Frame outFrame;
    memset(&outFrame, 0, sizeof(outFrame));
    printf("============Set key frame to true in 1\n");
    printf("============New buffer size is:%d\n", length);

    outFrame.format = FRAME_FORMAT_H264;
    outFrame.length = length;
    outFrame.additionalInfo.video.width = 1440;
    outFrame.additionalInfo.video.height = 810;
    if(m_frameCount == 0)
       outFrame.additionalInfo.video.isKeyFrame = true; 
    outFrame.timeStamp = (m_frameCount++) * 1000 / 30 * 90;
    printf("============Set key frame to true\n");

    outFrame.payload = buffer;
    
    
    char sendBuffer[sizeof(Frame) + 1];
    size_t header_len = sizeof(Frame);

    sendBuffer[0] = TDT_MEDIA_FRAME;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<Frame*>(&outFrame)), header_len);
    m_transport->sendData(sendBuffer, header_len + 1, reinterpret_cast<char*>(const_cast<uint8_t*>(outFrame.payload)), outFrame.length);
}

void InternalOut::onTransportData(char* buf, int len)
{
    switch (buf[0]) {
        case TDT_FEEDBACK_MSG:
            printf("============Got feedback message\n");
            // #ifdef BUILD_FOR_GST_ANALYTICS
            // FeedbackMsg* msg = reinterpret_cast<FeedbackMsg*>(buf + 1);
            // if(msg->type == VIDEO_FEEDBACK) {
            //     printf("============Got video feedback message\n");
            // }
            // #endif
            deliverFeedbackMsg(*(reinterpret_cast<FeedbackMsg*>(buf + 1)));
        default:
            break;
    }
}


} /* namespace owt_base */

