// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "QuicTransport.h"
#include <thread>
#include <chrono>
#include <iostream>

using namespace net;
using namespace owt_base;

const char TDT_FEEDBACK_MSG = 0x5A;
const char TDT_MEDIA_FRAME = 0x8F;
const size_t INIT_BUFF_SIZE = 80000;

// QUIC Incomming
QuicIn::QuicIn(unsigned int port, const std::string& cert_file, const std::string& key_file, EventRegistry *handle)
        : server_(RQuicFactory::createQuicServer(cert_file.c_str(), key_file.c_str()))
        , m_bufferSize(INIT_BUFF_SIZE)
        , m_receivedBytes(0)
        , m_asyncHandle(handle) {
  m_receiveData.buffer.reset(new char[m_bufferSize]);
  server_->setListener(this);
  server_->listen(port);
}

QuicIn::~QuicIn() {
    server_->stop();
    server_.reset();
}

unsigned int QuicIn::getListeningPort() {
    unsigned int port = server_->getServerPort();
    while (port == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        port = server_->getServerPort();
    }

    return port;
}


void QuicIn::send(uint32_t session_id, uint32_t stream_id, const std::string& data) {
    server_->send(session_id, stream_id, data);
}

/*void QuicIn::onFeedback(const FeedbackMsg& msg) {
    char sendBuffer[512];
    sendBuffer[0] = TDT_FEEDBACK_MSG;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<FeedbackMsg*>(&msg)), sizeof(FeedbackMsg));
    server_->send((char*)sendBuffer, sizeof(FeedbackMsg) + 1);
}*/


bool QuicIn::notifyAsyncEvent(const std::string& event, const std::string& data)
{
    if (m_asyncHandle) {
        return m_asyncHandle->notifyAsyncEvent(event, data);
    } else {
        return false;
    }
}

bool QuicIn::notifyAsyncEventInEmergency(const std::string& event, const std::string& data)
{
    if (m_asyncHandle) {
            return m_asyncHandle->notifyAsyncEventInEmergency(event, data);
        } else {
            return false;
        }
}

void QuicIn::onReady(uint32_t session_id, uint32_t stream_id) {}

void QuicIn::onData(uint32_t session_id, uint32_t stream_id, char* buf, uint32_t len) {
    // std::cout << "onData len:" << len << std::endl;
    std::string sessionId = std::to_string(session_id);
    std::string streamId = std::to_string(stream_id);
    std::string sId = sessionId + streamId;

    if (m_receivedBytes + len >= m_bufferSize) {
        m_bufferSize += (m_receivedBytes + len);
        std::cout << "new_bufferSize: " << m_bufferSize << std::endl;
        boost::shared_array<char> new_buffer;
        new_buffer.reset(new char[m_bufferSize]);
        if (new_buffer.get()) {
            memcpy(new_buffer.get(), m_receiveData.buffer.get(), m_receivedBytes);
            m_receiveData.buffer.reset();
            m_receiveData.buffer = new_buffer;
        } else {
            return;
        }
    }
    memcpy(m_receiveData.buffer.get() + m_receivedBytes, buf, len);
    m_receivedBytes += len;
    while (m_receivedBytes >= 4) {
        uint32_t payloadlen = 0;
        payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_receiveData.buffer.get())));
        uint32_t expectedLen = payloadlen + 4;
        if (expectedLen > m_receivedBytes) {
            // continue
            break;
        } else {
            // std::cout << "receive: " << expectedLen << std::endl;
            m_receivedBytes -= expectedLen;
            char* dpos = m_receiveData.buffer.get() + 4;

            if (hasStream_.count(sId) == 0) {
                hasStream_[sId] = true;
                std::string data("{\"sessionId\":sessionId, \"streamId\":streamId, \"room\":dpos}");
                notifyAsyncEvent("newstream", data.c_str());
            } else {
                notifyAsyncEvent("roomevents", dpos);
            }

            if (m_receivedBytes > 0) {
                std::cout << "not zero m_receiveBytes" << std::endl;
                memcpy(m_receiveData.buffer.get(), m_receiveData.buffer.get() + expectedLen, m_receivedBytes);
            }
        }
    }
}

// QUIC Outgoing
QuicOut::QuicOut(const std::string& dest_ip, unsigned int dest_port, EventRegistry *handle)
        : client_(RQuicFactory::createQuicClient())
        , m_bufferSize(INIT_BUFF_SIZE)
        , m_receivedBytes(0)
        , m_asyncHandle(handle) {
    client_->setListener(this);
    client_->start(dest_ip.c_str(), dest_port);
}

QuicOut::~QuicOut() {
    client_->stop();
    client_.reset();
}

void QuicOut::send(uint32_t session_id, uint32_t stream_id, const std::string& data) {
    client_->send(session_id, stream_id, data);
}


void QuicOut::onData(uint32_t session_id, uint32_t stream_id, char* buf, uint32_t len) {
    // std::cout << "onData len:" << len << std::endl;

    if (m_receivedBytes + len >= m_bufferSize) {
        m_bufferSize += (m_receivedBytes + len);
        std::cout << "new_bufferSize: " << m_bufferSize << std::endl;
        boost::shared_array<char> new_buffer;
        new_buffer.reset(new char[m_bufferSize]);
        if (new_buffer.get()) {
            memcpy(new_buffer.get(), m_receiveData.buffer.get(), m_receivedBytes);
            m_receiveData.buffer.reset();
            m_receiveData.buffer = new_buffer;
        } else {
            return;
        }
    }
    memcpy(m_receiveData.buffer.get() + m_receivedBytes, buf, len);
    m_receivedBytes += len;
    while (m_receivedBytes >= 4) {
        uint32_t payloadlen = 0;
        payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_receiveData.buffer.get())));
        uint32_t expectedLen = payloadlen + 4;
        if (expectedLen > m_receivedBytes) {
            // continue
            break;
        } else {
            // std::cout << "receive: " << expectedLen << std::endl;
            m_receivedBytes -= expectedLen;
            char* dpos = m_receiveData.buffer.get() + 4;
            notifyAsyncEvent("roomevents", dpos);

            if (m_receivedBytes > 0) {
                std::cout << "not zero m_receiveBytes" << std::endl;
                memcpy(m_receiveData.buffer.get(), m_receiveData.buffer.get() + expectedLen, m_receivedBytes);
            }
        }
    }
}

void QuicOut::onReady(uint32_t session_id, uint32_t stream_id) {
    std::string data("{\"sessionId\":sessionId, \"streamId\":streamId}");
    notifyAsyncEvent("newstream", data.c_str());
}

bool QuicOut::notifyAsyncEvent(const std::string& event, const std::string& data)
{
    if (m_asyncHandle) {
        return m_asyncHandle->notifyAsyncEvent(event, data);
    } else {
        return false;
    }
}

bool QuicOut::notifyAsyncEventInEmergency(const std::string& event, const std::string& data)
{
    if (m_asyncHandle) {
            return m_asyncHandle->notifyAsyncEventInEmergency(event, data);
        } else {
            return false;
        }
}
