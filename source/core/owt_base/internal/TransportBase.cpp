// Copyright (C) <2021> Intel Corporation
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
    : m_buffer(new uint8_t[kInitalBufferSize])
    , m_bufferSize(kInitalBufferSize)
    , m_receivedBytes(0)
{
}

TransportMessage::TransportMessage(const uint8_t* data, uint32_t length)
    : m_buffer(new uint8_t[kHeaderSize + length])
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

uint32_t TransportMessage::fillData(const uint8_t* data, uint32_t length)
{
    uint32_t toFill= missingBytes();
    if (toFill == 0) {
        return 0;
    }
    length = std::min(length, toFill);
    if (m_receivedBytes + toFill > m_bufferSize) {
        // Increasing the buffer size: %zu
        boost::shared_array<uint8_t> oldBuffer = m_buffer;
        uint32_t newLength = m_receivedBytes + toFill;
        m_bufferSize = (newLength * kExpansionMultiplier + kBufferAlignment - 1) /
            kBufferAlignment * kBufferAlignment;
        m_bufferSize = std::max(m_bufferSize, m_receivedBytes + toFill);
        m_buffer.reset(new uint8_t[m_bufferSize]);
        memcpy(m_buffer.get(), oldBuffer.get(), m_receivedBytes);
    }
    if (data) {
        memcpy(m_buffer.get() + m_receivedBytes, data, length);
    } else {
        length = 0;
    }
    m_receivedBytes += length;
    return length;
}

void TransportMessage::clear()
{
    m_receivedBytes = 0;
}

uint8_t* TransportMessage::payloadData() const
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

uint8_t* TransportMessage::messageData() const
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
    , m_receivedBuffer(new uint8_t[kInitalBufferSize])
    , m_receivedBufferSize(kInitalBufferSize)
    , m_isClosed(false)
    , m_listener(listener)
{
}

TransportSession::TransportSession(
    uint32_t id,
    std::shared_ptr<IOService> service,
    std::shared_ptr<SSLSocket> sslSocket,
    TransportSession::Listener* listener)
    : m_id(id)
    , m_service(service)
    , m_socket(m_service->service())
    , m_sslSocket(sslSocket)
    , m_receivedBuffer(new uint8_t[kInitalBufferSize])
    , m_receivedBufferSize(kInitalBufferSize)
    , m_isClosed(false)
    , m_listener(listener)
{
}

TransportSession::~TransportSession()
{
    ELOG_DEBUG("Destructor begin");
    close();
    ELOG_DEBUG("Destructor end");
}

void TransportSession::start()
{
    receiveData();
}

void TransportSession::sendData(TransportData data)
{
    if (m_isClosed) {
        ELOG_DEBUG("sendData: already closed");
        return;
    }
    auto self(shared_from_this());
    m_service->post(boost::bind(&TransportSession::prepareSend, self, data));
}

void TransportSession::prepareSend(TransportData data)
{
    // Only access m_sendQueue in IO service thread.
    TransportMessage toSend(data.buffer.get(), data.length);
    TransportData wrappedData{toSend.messageData(),
                              toSend.messageLength()};
    m_sendQueue.push(wrappedData);
    if (m_sendQueue.size() == 1) {
        sendHandler();
    }
}

void TransportSession::sendHandler()
{
    if (m_isClosed) {
        return;
    }
    if (!m_sslSocket && !m_socket.is_open()) {
        ELOG_WARN("sendHandler: socket is not open");
        return;
    }
    if (m_sendQueue.empty()) {
        return;
    }

    TransportData& data = m_sendQueue.front();

    ELOG_DEBUG("SendHandler- %p %zu", this, (size_t)data.length);
    auto self(shared_from_this());
    if (m_sslSocket) {
        boost::asio::async_write(
            *m_sslSocket,
            boost::asio::buffer(data.buffer.get(), data.length),
            boost::bind(&TransportSession::writeHandler, self,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    } else {
        boost::asio::async_write(
            m_socket,
            boost::asio::buffer(data.buffer.get(), data.length),
            boost::bind(&TransportSession::writeHandler, self,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
}

void TransportSession::writeHandler(
    const boost::system::error_code& ec,
    std::size_t bytes)
{
    assert(m_sendQueue.size() > 0);
    m_sendQueue.pop();
    if (ec) {
        ELOG_DEBUG("Error writing data: %s", ec.message().c_str());
        if (!m_isClosed) {
            m_isClosed = true;
            // Notify the listener about the socket error if the listener is not closing me.
            m_listener->onClose(m_id);
        }
    } else {
        ELOG_DEBUG("Wrote data: %zu", bytes);
        sendHandler();
    }
}

void TransportSession::close()
{
    ELOG_DEBUG("Closing... %p", this);
    m_isClosed = true;
    if (m_sslSocket) {
        auto sock = m_sslSocket;
        sock->lowest_layer().cancel();
        sock->async_shutdown([sock](const boost::system::error_code& ec)
        {
            boost::system::error_code e;
            sock->lowest_layer().shutdown(
                boost::asio::ip::tcp::socket::shutdown_both, e);
            sock->lowest_layer().close();
        });
    }
    if (m_socket.is_open()) {
        boost::system::error_code ec;
        m_socket.cancel();
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close();
        if (ec) {
            ELOG_DEBUG("Shutdown socket error: %s", ec.message().c_str());
        }
    }
    ELOG_DEBUG("Closed %p", this);
}

void TransportSession::receiveData()
{
    if (m_isClosed) {
        ELOG_DEBUG("receiveData: already closed");
        return;
    }
    if (!m_sslSocket && !m_socket.is_open()) {
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
        m_receivedBuffer.reset(new uint8_t[m_receivedBufferSize]);
        ELOG_DEBUG("Increasing the buffer size: %u", m_receivedBufferSize);
    }

    auto self(shared_from_this());
    if (m_sslSocket) {
        m_sslSocket->async_read_some(boost::asio::buffer(m_receivedBuffer.get(), bytesToRead),
            boost::bind(&TransportSession::readHandler, self,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    } else {
        m_socket.async_read_some(boost::asio::buffer(m_receivedBuffer.get(), bytesToRead),
            boost::bind(&TransportSession::readHandler, self,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
}

void TransportSession::readHandler(
    const boost::system::error_code& ec, std::size_t bytes)
{
    if (m_isClosed) {
        ELOG_DEBUG("readHandler: already closed");
        return;
    }
    ELOG_DEBUG("ReadHandler : %p %zu", this, bytes);
    if (!ec || ec.value() == boost::asio::error::message_size) {
        uint32_t bytesToRead = m_receivedMessage.missingBytes();
        assert(bytesToRead >= bytes);
        uint32_t filled = m_receivedMessage.fillData(m_receivedBuffer.get(), bytes);
        if (filled != bytes) {
            ELOG_WARN("Message fill length %u, %zu\n", filled, bytes);
        }
        receiveData();
    } else {
        if (ec.value() != boost::system::errc::operation_canceled &&
            ec != boost::asio::error::eof) {
            ELOG_WARN("Error receiving data: %s", ec.message().c_str());
        } else {
            ELOG_DEBUG("Error receiving data: %s", ec.message().c_str());
        }
        if (!m_isClosed) {
            m_isClosed = true;
            // Notify the listener about the socket error if the listener is not closing me.
            m_listener->onClose(m_id);
        }
    }
}

// TransportSecret
static std::string gSecretPass = "";

void TransportSecret::setPassphrase(std::string p)
{
    std::fill(gSecretPass.begin(), gSecretPass.end(), 0);
    gSecretPass = p;
}

std::string TransportSecret::getPassphrase()
{
    return gSecretPass;
}

}
/* namespace owt_base */
