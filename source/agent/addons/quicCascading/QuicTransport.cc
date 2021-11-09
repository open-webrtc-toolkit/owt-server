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
    TransportData sendData;
    uint32_t payloadLength = data.length();
    sendData.buffer.reset(new char[payloadLength + 4]);
    *(reinterpret_cast<uint32_t*>(sendData.buffer.get())) = htonl(payloadLength);
    memcpy(sendData.buffer.get() + 4, data.c_str(), payloadLength);
    sendData.length = payloadLength + 4;

    //std::string str(data.buffer.get(), data.length);
    if (sendData.length > INIT_BUFF_SIZE + 4) {
        std::cout << "sendFrame " << (sendData.length  - 4)<< std::endl;
    }
    printf("data address in addon is:%s\n", data.c_str());
    server_->send(session_id, stream_id, sendData.buffer.get(), sendData.length);
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

void QuicIn::onReady(uint32_t session_id, uint32_t stream_id) {
    std::cout << "server onReady session id:" << session_id << " stream id:" << stream_id << std::endl;
}

void QuicIn::onData(uint32_t session_id, uint32_t stream_id, char* buf, uint32_t len) {
    std::cout << "server onData: " << buf << " len:" << len << " m_bufferSize:" << m_bufferSize << " m_receivedBytes:" << m_receivedBytes << std::endl;
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
	std::cout << "m_receivedBytes:" << m_receivedBytes << " payloadlen:" << payloadlen << " expectedLen:" << expectedLen << std::endl;
        if (expectedLen > m_receivedBytes) {
            // continue
            break;
        } else {
            std::cout << "receive: " << expectedLen << std::endl;
            m_receivedBytes -= expectedLen;
            char* dpos = m_receiveData.buffer.get() + 4;
            std::string s_data(dpos, payloadlen);

            if (hasStream_.count(sId) == 0) {
                hasStream_[sId] = true;
                std::string str;
                str.append("{\"sessionId\":\"");
                str.append(sessionId);
                str.append("\",\"streamId\":\"");
                str.append(streamId);
                str.append("\",\"room\":\"");
                str.append(s_data);
                str.append("\"}");

                notifyAsyncEvent("newstream", str.c_str());
            } else {
                notifyAsyncEvent("roomevents", s_data);
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
    printf("QuicOut:QiucOut\n");
    client_->setListener(this);
    client_->start(dest_ip.c_str(), dest_port);
}

QuicOut::~QuicOut() {
    client_->stop();
    client_.reset();
}

void QuicOut::send(uint32_t session_id, uint32_t stream_id, std::string data) {
    std::cout << "client send data: " << data << "with size:" << data.length() << std::endl;
    TransportData sendData;
    uint32_t payloadLength = data.length();
    sendData.buffer.reset(new char[payloadLength + 4]);
    *(reinterpret_cast<uint32_t*>(sendData.buffer.get())) = htonl(payloadLength);
    memcpy(sendData.buffer.get() + 4, data.c_str(), payloadLength);
    sendData.length = payloadLength + 4;

    //std::string str(data.buffer.get(), data.length);
    if (sendData.length > INIT_BUFF_SIZE + 4) {
        std::cout << "sendFrame " << (sendData.length  - 4)<< std::endl;
    }
    printf("data address in addon is:%s\n", data.c_str());
    client_->send(session_id, stream_id, sendData.buffer.get(), sendData.length);
}


void QuicOut::onData(uint32_t session_id, uint32_t stream_id, char* buf, uint32_t len) {
    std::cout << "onData len:" << len << std::endl;

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

    std::cout << "Copy data and m_receivedBytes:" << m_receivedBytes << std::endl;
    memcpy(m_receiveData.buffer.get() + m_receivedBytes, buf, len);
    m_receivedBytes += len;
    while (m_receivedBytes >= 4) {
        std::cout << "m_receivedBytes >=4";
        uint32_t payloadlen = 0;
        payloadlen = ntohl(*(reinterpret_cast<uint32_t*>(m_receiveData.buffer.get())));
        uint32_t expectedLen = payloadlen + 4;
        if (expectedLen > m_receivedBytes) {
            // continue
            std::cout << "expectedLen:" << expectedLen << " m_receivedBytes:" << m_receivedBytes << std::endl;
            break;
        } else {
            // std::cout << "receive: " << expectedLen << std::endl;
            m_receivedBytes -= expectedLen;
            char* dpos = m_receiveData.buffer.get() + 4;
            std::cout << "notify roomevents:" << dpos << std::endl;
            notifyAsyncEvent("roomevents", dpos);

            if (m_receivedBytes > 0) {
                std::cout << "not zero m_receiveBytes" << std::endl;
                memcpy(m_receiveData.buffer.get(), m_receiveData.buffer.get() + expectedLen, m_receivedBytes);
            }
        }
    }
}

void QuicOut::onReady(uint32_t session_id, uint32_t stream_id) {
    std::string data("{\"sessionId\":");
    data.append(std::to_string(session_id));
    data.append(", \"streamId\":");
    data.append(std::to_string(stream_id));
    data.append("}");
    printf("Quic out ready with data:%s\n", data.c_str());
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
