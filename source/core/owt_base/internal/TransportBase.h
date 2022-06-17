// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef TransportBase_h
#define TransportBase_h

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>

#include "IOService.h"
#include <memory>
#include <queue>

namespace owt_base {

// const char TDT_FEEDBACK_MSG = 0x5A;
// const char TDT_MEDIA_FRAME = 0x8F;

using boost::asio::ip::tcp;

/*
 * TransportMessage
 * | 4 bytes (payload length) | payload length bytes (payload data) |
 */
class TransportMessage {
public:
    // Construct an incomplete empty message
    TransportMessage();
    // Construct a complete message with data and length
    TransportMessage(const uint8_t* data, uint32_t length);

    // If the message format is complete (4 bytes header + payload)
    bool isComplete() const;
    // How many bytes missing according to the current buffer
    uint32_t missingBytes() const;
    // Fill data at the end of current buffer
    uint32_t fillData(const uint8_t* data, uint32_t length);
    // Clear the buffer and mark as incomplete
    void clear();

    // Return the payload data if it's complete
    uint8_t* payloadData() const;
    uint32_t payloadLength() const;
    // Return the buffer data
    uint8_t* messageData() const;
    uint32_t messageLength() const;

private:
    boost::shared_array<uint8_t> m_buffer;
    uint32_t m_bufferSize;
    uint32_t m_receivedBytes;
};

/*
 * Combine buffer and size
 */
struct TransportData{
    TransportData() {}
    TransportData(const uint8_t* data, uint32_t len)
        : buffer(new uint8_t[len]), length(len)
    {
        memcpy(buffer.get(), data, len);
    }
    boost::shared_array<uint8_t> buffer;
    uint32_t length;
} ;

/*
 * BaseSession for RawTransport
 */
class TransportSession
    : public std::enable_shared_from_this<TransportSession> {
    DECLARE_LOGGER();
public:
    class Listener {
    public:
        virtual void onData(uint32_t id, TransportData data) = 0;
        virtual void onClose(uint32_t id) = 0;
    };
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLSocket;

    TransportSession(uint32_t id,
                     std::shared_ptr<IOService> service,
                     boost::asio::ip::tcp::socket socket,
                     Listener* listener);
    // Constructor for secured session
    TransportSession(uint32_t id,
                     std::shared_ptr<IOService> service,
                     std::shared_ptr<SSLSocket> sslSocket,
                     Listener* listener);

    virtual ~TransportSession();

    void sendData(TransportData data);
    void start();
    void close();

private:
    void receiveData();
    void readHandler(const boost::system::error_code&, std::size_t);
    void prepareSend(TransportData data);
    void sendHandler();
    void writeHandler(const boost::system::error_code&, std::size_t);

    uint32_t m_id;
    std::shared_ptr<IOService> m_service;
    boost::asio::ip::tcp::socket m_socket;
    std::shared_ptr<SSLSocket> m_sslSocket;
    TransportMessage m_receivedMessage;
    boost::shared_array<uint8_t> m_receivedBuffer;
    std::queue<TransportData> m_sendQueue;
    uint32_t m_receivedBufferSize;
    bool m_isClosed;
    Listener* m_listener;
};

/*
 * Secret for transport
 */
class TransportSecret {
public:
    static void setPassphrase(std::string p);
    static std::string getPassphrase();
};

} /* namespace owt_base */
#endif /* TransportBase_h */
