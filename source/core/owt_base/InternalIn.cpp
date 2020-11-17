// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "InternalIn.h"

namespace owt_base {

InternalIn::InternalIn(const std::string& protocol, unsigned int minPort, unsigned int maxPort)
{
    if (protocol == "tcp")
        m_transport.reset(new owt_base::RawTransport<TCP>(this));
    else
        m_transport.reset(new owt_base::RawTransport<UDP>(this, 64 * 1024));

    if (minPort > 0 && minPort <= maxPort) {
        m_transport->listenTo(minPort, maxPort);
    } else {
        m_transport->listenTo(0);
    }
}

InternalIn::~InternalIn()
{
    m_transport->close();
}

unsigned int InternalIn::getListeningPort()
{
    return m_transport->getListeningPort();
}

void InternalIn::onFeedback(const FeedbackMsg& msg)
{
    char sendBuffer[512];
    sendBuffer[0] = TDT_FEEDBACK_MSG;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<FeedbackMsg*>(&msg)), sizeof(FeedbackMsg));
    m_transport->sendData((char*)sendBuffer, sizeof(FeedbackMsg) + 1);
}

void InternalIn::onTransportData(char* buf, int len)
{
    Frame* frame = nullptr;
    MetaData* metadata = nullptr;
    switch (buf[0]) {
        case TDT_MEDIA_FRAME:
            frame = reinterpret_cast<Frame*>(buf + 1);
            frame->payload = reinterpret_cast<uint8_t*>(buf + 1 + sizeof(Frame));
            deliverFrame(*frame);
            break;
        case TDT_MEDIA_METADATA:
            metadata = reinterpret_cast<MetaData*>(buf + 1);
            metadata->payload = reinterpret_cast<uint8_t*>(buf + 1 + sizeof(MetaData));
            deliverMetaData(*metadata);
            break;
        default:
            break;
    }
}

} /* namespace owt_base */
