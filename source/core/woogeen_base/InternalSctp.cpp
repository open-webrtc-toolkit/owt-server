// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "InternalSctp.h"

namespace woogeen_base {

InternalSctp::InternalSctp()
{
    m_transport.reset(new woogeen_base::SctpTransport(this, 1 << 16));
    m_transport->open();
}

InternalSctp::~InternalSctp()
{
    m_transport->close();
}

void InternalSctp::connect(const std::string &ip, unsigned int udpPort, unsigned int sctpPort)
{
    m_transport->connect(ip, udpPort, sctpPort);
}

void InternalSctp::onFrame(const Frame& frame)
{
    char sendBuffer[sizeof(Frame) + 1];
    size_t header_len = sizeof(Frame);

    sendBuffer[0] = TDT_MEDIA_FRAME;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<Frame*>(&frame)), header_len);
    m_transport->sendData(sendBuffer, header_len + 1, reinterpret_cast<char*>(const_cast<uint8_t*>(frame.payload)), frame.length);
}

void InternalSctp::onFeedback(const FeedbackMsg& msg)
{
    char sendBuffer[512];
    sendBuffer[0] = TDT_FEEDBACK_MSG;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<FeedbackMsg*>(&msg)), sizeof(FeedbackMsg));
    m_transport->sendData((char*)sendBuffer, sizeof(FeedbackMsg) + 1);
}

void InternalSctp::onTransportData(char* buf, int len)
{
    Frame* frame = nullptr;
    switch (buf[0]) {
        case TDT_MEDIA_FRAME:
            frame = reinterpret_cast<Frame*>(buf + 1);
            frame->payload = reinterpret_cast<uint8_t*>(buf + 1 + sizeof(Frame));
            deliverFrame(*frame);
            break;
        case TDT_FEEDBACK_MSG:
            deliverFeedbackMsg(*(reinterpret_cast<FeedbackMsg*>(buf + 1)));
        default:
            break;
    }
}

} /* namespace woogeen_base */
