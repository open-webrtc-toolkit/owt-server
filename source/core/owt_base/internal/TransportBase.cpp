// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "TransportBase.h"

#include <netinet/in.h>

namespace owt_base {

using boost::asio::ip::tcp;

DEFINE_LOGGER(TransportSession, "owt.TransportSession");

static const int kHeaderSize = 4;
static const int kInitalBufferSize = 1600;
static const int kBufferAlignment = 16;
static const double kExpansionMultiplier = 1.3;

TransportMessage::TransportMessage()
    : m_buffer(new char[kInitalBufferSize])
    , m_bufferSize(kInitalBufferSize)
    , m_receivedBytes(0)
{
}

TransportMessage::TransportMessage(const char* data, uint32_t length)
    : m_buffer(new char[kHeaderSize + length])
    , m_bufferSize(kHeaderSize + length)
    , m_receivedBytes(m_bufferSize)
{
    *(reinterpret_cast<uint32_t*>(m_buffer.get())) = htonl(length);
    memcpy(m_buffer.get() + kHeaderSize, data, length);
}

bool TransportMessage::isComplete() const
{
    if (m_receivedBytes < kHeaderSize) {
        return false;
    }
    uint32_t payloadLen = payloadLength();
    return (payloadLen + kHeaderSize == m_receivedBytes);
}

uint32_t TransportMessage::missingBytes() const
{
    if (m_receivedBytes < kHeaderSize) {
        return (kHeaderSize - m_receivedBytes);
    } else {
        uint32_t payloadLen = payloadLength();
        if (m_receivedBytes >= kHeaderSize + payloadLen) {
            return 0;
        }
        return (payloadLen + kHeaderSize - m_receivedBytes);
    }
}

uint32_t TransportMessage::fillData(const char* data, uint32_t length)
{
    int toFill= missingBytes();
    if (toFill == 0) {
        return 0;
    }
    if (m_receivedBytes + toFill > m_bufferSize) {
        // Increasing the buffer size: %zu
        boost::shared_array<char> oldBuffer = m_buffer;
        uint32_t newLength = m_receivedBytes + toFill;
        m_bufferSize = (newLength * kExpansionMultiplier + kBufferAlignment - 1) /
            kBufferAlignment * kBufferAlignment;
        m_bufferSize = std::max(m_bufferSize, m_receivedBytes + toFill);

        m_buffer.reset(new char[m_bufferSize]);
        memcpy(m_buffer.get(), oldBuffer.get(), m_receivedBytes);
    }
    if (data) {
        memcpy(m_buffer.get() + m_receivedBytes, data, toFill);
    }
    m_receivedBytes += toFill;
    return toFill;
}

void TransportMessage::clear()
{
    m_receivedBytes = 0;
}

char* TransportMessage::payloadData() const
{
    return isComplete() ? m_buffer.get() + kHeaderSize : nullptr;
}

uint32_t TransportMessage::payloadLength() const
{
    if (m_receivedBytes >= kHeaderSize) {
        return ntohl(*(reinterpret_cast<uint32_t*>(m_buffer.get())));
    }
    return 0;
}

char* TransportMessage::messageData() const
{
    return isComplete() ? m_buffer.get() : nullptr;
}

uint32_t TransportMessage::messageLength() const
{
    return payloadLength() ? payloadLength() + kHeaderSize : 0;
}

TransportSession::TransportSession(
    uint32_t id,
    std::shared_ptr<IOService> service,
    boost::asio::ip::tcp::socket socket,
    TransportSession::Listener* listener)
    : m_id(id)
    , m_service(service)
    , m_socket(std::move(socket))
    , m_receivedBuffer(new char[kInitalBufferSize])
    , m_receivedBufferSize(kInitalBufferSize)
    , m_isClosed(false)
    , m_listener(listener)
{
    receiveData();
}

TransportSession::~TransportSession()
{
    close();
}

void TransportSession::sendData(TransportData data)
{
    if (m_isClosed) {
        ELOG_DEBUG("sendData: already closed");
        return;
    }
    m_service->post(boost::bind(&TransportSession::sendHandler, this, data));
}

void TransportSession::sendHandler(TransportData data)
{
    if (!m_socket.is_open()) {
        return;
    }
    TransportMessage toSend(data.buffer.get(), data.length);
    TransportData wrappedData{toSend.messageData(),
                              toSend.messageLength()};
    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(wrappedData.buffer.get(), wrappedData.length),
        boost::bind(&TransportSession::writeHandler, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void TransportSession::writeHandler(
    const boost::system::error_code& ec,
    std::size_t bytes)
{
    if (ec) {
        ELOG_DEBUG("Error writing data: %s", ec.message().c_str());
        if (!m_isClosed.exchange(true)) {
            // Notify the listener about the socket error if the listener is not closing me.
            m_listener->onClose(m_id);
        }
    }
}

void TransportSession::close()
{
    ELOG_DEBUG("Closing...");
    if (!m_isClosed.exchange(true)) {
        if (m_socket.is_open()) {
            boost::system::error_code ec;
            m_socket.cancel();
            m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            m_socket.close();
        }
    }
    ELOG_DEBUG("Closed");
}

void TransportSession::receiveData()
{
    if (m_isClosed) {
        ELOG_DEBUG("receiveData: already closed");
        return;
    }
    if (!m_socket.is_open()) {
        ELOG_WARN("receiveData: socket is not open");
        return;
    }

    if (m_receivedMessage.isComplete()) {
        TransportData data{m_receivedMessage.payloadData(),
                           m_receivedMessage.payloadLength()};
        m_listener->onData(m_id, data);
        m_receivedMessage.clear();
    }

    uint32_t bytesToRead = m_receivedMessage.missingBytes();
    assert(bytesToRead > 0);
    if (bytesToRead > m_receivedBufferSize) {
        // Double the received buffer size
        m_receivedBufferSize = std::max(m_receivedBufferSize * 2, bytesToRead);
        m_receivedBuffer.reset(new char[m_receivedBufferSize]);
        ELOG_DEBUG("Increasing the buffer size: %u", m_receivedBufferSize);
    }

    ELOG_DEBUG("Try to read %u", bytesToRead);
    m_socket.async_read_some(boost::asio::buffer(m_receivedBuffer.get(), bytesToRead),
        boost::bind(&TransportSession::readHandler, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void TransportSession::readHandler(
    const boost::system::error_code& ec, std::size_t bytes)
{
    if (!ec || ec.value() == boost::asio::error::message_size) {
        uint32_t bytesToRead = m_receivedMessage.missingBytes();
        assert(bytesToRead >= bytes);
        m_receivedMessage.fillData(m_receivedBuffer.get(), bytes);
        receiveData();
    } else {
        if (ec.value() != boost::system::errc::operation_canceled) {
            ELOG_WARN("Error receiving data: %s", ec.message().c_str());
        } else {
            ELOG_DEBUG("Error receiving data: %s", ec.message().c_str());
        }
        if (!m_isClosed.exchange(true)) {
            // Notify the listener about the socket error if the listener is not closing me.
            m_listener->onClose(m_id);
        }
    }
}

}
/* namespace owt_base */
