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
QuicIn::QuicIn(const std::string& cert_file, const std::string& key_file)
        : server_(RQuicFactory::createQuicServer(cert_file.c_str(), key_file.c_str()))
        , m_hasStream(false)
        , m_bufferSize(INIT_BUFF_SIZE)
        , m_receivedBytes(0) {
  m_receiveData.buffer.reset(new char[m_bufferSize]);
  server_->setListener(this);
  server_->listen(0);
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

void QuicIn::onReady() {}

void QuicIn::onFeedback(const FeedbackMsg& msg) {
    char sendBuffer[512];
    sendBuffer[0] = TDT_FEEDBACK_MSG;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<FeedbackMsg*>(&msg)), sizeof(FeedbackMsg));
    server_->send((char*)sendBuffer, sizeof(FeedbackMsg) + 1);
}

void QuicIn::dFrame(char* buf) {
    owt_base::Frame* frame = nullptr;
    switch (buf[0]) {
        case TDT_MEDIA_FRAME:
            frame = reinterpret_cast<Frame*>(buf + 1);
            frame->payload = reinterpret_cast<uint8_t*>(buf + 1 + sizeof(Frame));
            deliverFrame(*frame);
            // std::cout << "deliverFrame" << std::endl;
            break;
        default:
            break;
    }
}

void QuicIn::onData(uint32_t session_id, uint32_t stream_id, char* buf, uint32_t len) {
    // std::cout << "onData len:" << len << std::endl;
    if (!m_hasStream) {
        m_hasStream = true;
    }
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
            dFrame(dpos);
            if (m_receivedBytes > 0) {
                std::cout << "not zero m_receiveBytes" << std::endl;
                memcpy(m_receiveData.buffer.get(), m_receiveData.buffer.get() + expectedLen, m_receivedBytes);
            }
        }
    }
}

// QUIC Outgoing
QuicOut::QuicOut(const std::string& dest_ip, unsigned int dest_port)
        : client_(RQuicFactory::createQuicClient()) {
    client_->setListener(this);
    client_->start(dest_ip.c_str(), dest_port);
}

QuicOut::~QuicOut() {
    client_->stop();
    client_.reset();
}

void QuicOut::onFrame(const Frame& frame) {
    char sendBuffer[sizeof(Frame) + 1];
    size_t header_len = sizeof(Frame);

    sendBuffer[0] = TDT_MEDIA_FRAME;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<Frame*>(&frame)), header_len);

    char* header = sendBuffer;
    int headerLength = header_len + 1;
    char* payload = reinterpret_cast<char*>(const_cast<uint8_t*>(frame.payload));
    int payloadLength = frame.length;

    TransportData data;
    data.buffer.reset(new char[headerLength + payloadLength + 4]);
    *(reinterpret_cast<uint32_t*>(data.buffer.get())) = htonl(headerLength + payloadLength);
    memcpy(data.buffer.get() + 4, header, headerLength);
    memcpy(data.buffer.get() + 4 + headerLength, payload, payloadLength);
    data.length = headerLength + payloadLength + 4;

    //std::string str(data.buffer.get(), data.length);
    if (data.length > INIT_BUFF_SIZE + 4) {
        std::cout << "sendFrame " << (data.length  - 4)<< std::endl;
    }
    client_->send(data.buffer.get(), data.length);
}

void QuicOut::onReady() {}

void QuicOut::onData(uint32_t session_id, uint32_t stream_id, char* buf, uint32_t len) {
    switch (buf[0]) {
        case TDT_FEEDBACK_MSG:
            deliverFeedbackMsg(*(reinterpret_cast<FeedbackMsg*>(buf + 1)));
        default:
            break;
    }
}
