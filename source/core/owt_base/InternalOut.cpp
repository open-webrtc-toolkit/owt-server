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

void InternalOut::onMetaData(const MetaData& metadata)
{
    char sendBuffer[sizeof(MetaData) + 1];
    size_t header_len = sizeof(MetaData);

    sendBuffer[0] = TDT_MEDIA_METADATA;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<MetaData*>(&metadata)), header_len);
    m_transport->sendData(sendBuffer, header_len + 1, reinterpret_cast<char*>(const_cast<uint8_t*>(metadata.payload)), metadata.length);
}

void InternalOut::onTransportData(char* buf, int len)
{
    switch (buf[0]) {
        case TDT_FEEDBACK_MSG:
            deliverFeedbackMsg(*(reinterpret_cast<FeedbackMsg*>(buf + 1)));
        default:
            break;
    }
}


} /* namespace owt_base */

