// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef TransportClient_h
#define TransportClient_h

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <memory>
#include "RawTransport.h"
#include "TransportBase.h"
#include <unordered_map>

namespace owt_base {

using boost::asio::ip::tcp;

/*
 * TransportClient
 */
class TransportClient : public TransportSession::Listener {
    DECLARE_LOGGER();
public:
    class Listener {
    public:
        virtual void onConnected() = 0;
        virtual void onData(uint8_t* data, uint32_t len) = 0;
        virtual void onDisconnected() = 0;
    };
    TransportClient(Listener* listener);
    ~TransportClient();

    bool enableSecure();

    void createConnection(const std::string& ip, uint32_t port);
    void sendData(const uint8_t*, uint32_t len);
    void sendData(const uint8_t* header, uint32_t headerLength,
                  const uint8_t* payload, uint32_t payloadLength);
    void close();
    bool initTicket(const std::string& ticket) { return true; }

    unsigned short getListeningPort() { return 0; }

    // Implements TransportSession::Listener
    void onData(uint32_t id, TransportData data) override;
    void onClose(uint32_t id) override;

private:
    void connectHandler(const boost::system::error_code&);
    void handshakeHandler(const boost::system::error_code& ec);

    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLSocket;

    std::shared_ptr<IOService> m_service;
    boost::asio::ip::tcp::socket m_socket;
    std::shared_ptr<TransportSession> m_session;

    bool m_isSecure;
    std::shared_ptr<boost::asio::ssl::context> m_sslContext;
    std::shared_ptr<SSLSocket> m_sslSocket;

    Listener* m_listener;
};

} /* namespace owt_base */
#endif /* TransportServer_h */
