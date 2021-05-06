// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "InternalClient.h"
#include "RawTransport.h"

namespace owt_base {

DEFINE_LOGGER(InternalClient, "owt.InternalClient");

InternalClient::InternalClient(
    const std::string& streamId,
    const std::string& protocol,
    Listener* listener)
    : m_client(new TransportClient(this))
    , m_streamId(streamId)
    , m_ready(false)
    , m_listener(listener)
{
}

InternalClient::InternalClient(
    const std::string& streamId,
    const std::string& protocol,
    const std::string& ip,
    unsigned int port,
    Listener* listener)
    : InternalClient(streamId, protocol, listener)
{
    m_client->createConnection(ip, port);
}

InternalClient::~InternalClient()
{
    m_client->close();
    m_client.reset();
}

void InternalClient::connect(const std::string& ip, unsigned int port)
{
    m_client->createConnection(ip, port);
}

void InternalClient::onFeedback(const FeedbackMsg& msg)
{
    if (!m_ready && msg.cmd != INIT_STREAM_ID) {
        return;
    }
    ELOG_DEBUG("onFeedback ");

    char sendBuffer[512];
    sendBuffer[0] = TDT_FEEDBACK_MSG;
    memcpy(&sendBuffer[1],
           reinterpret_cast<char*>(const_cast<FeedbackMsg*>(&msg)),
           sizeof(FeedbackMsg));
    m_client->sendData((char*)sendBuffer, sizeof(FeedbackMsg) + 1);
}

void InternalClient::onConnected()
{
    ELOG_DEBUG("On Connected %s", m_streamId.c_str());
    if (m_listener) {
        m_listener->onConnected();
    }
    if (!m_streamId.empty()) {
        FeedbackMsg msg(VIDEO_FEEDBACK, INIT_STREAM_ID);
        if (m_streamId.length() > 128) {
            ELOG_WARN("Too long streamId:%s, will be resized",
                      m_streamId.c_str());
            m_streamId.resize(128);
        }
        memcpy(&msg.buffer.data[0], m_streamId.c_str(), m_streamId.length());
        msg.buffer.len = m_streamId.length();
        onFeedback(msg);
    }
    m_ready = true;
}

void InternalClient::onData(char* buf, int len)
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

void InternalClient::onDisconnected()
{
    if (m_listener) {
        m_listener->onDisconnected();
    }
}


} /* namespace owt_base */

