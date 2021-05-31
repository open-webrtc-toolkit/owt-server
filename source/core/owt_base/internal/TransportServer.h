// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef TransportServer_h
#define TransportServer_h

#include "IOService.h"
#include "RawTransport.h"
#include "TransportBase.h"

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace owt_base {

// const char TDT_FEEDBACK_MSG = 0x5A;
// const char TDT_MEDIA_FRAME = 0x8F;

using boost::asio::ip::tcp;

/*
 * TransportServer
 */
class TransportServer : public TransportSession::Listener {
    DECLARE_LOGGER();
public:
    class Listener {
    public:
        virtual void onSessionAdded(int id) = 0;
        virtual void onSessionData(int id, uint8_t* data, uint32_t len) = 0;
        virtual void onSessionRemoved(int id) = 0;
    };
    TransportServer(Listener* listener);
    ~TransportServer();

    bool enableSecure();

    // Follow RawTransportInterface
    void listenTo(uint32_t port);
    void listenTo(uint32_t minPort, uint32_t maxPort);
    void sendData(const uint8_t* data, uint32_t len);
    void sendData(const uint8_t* header, uint32_t headerLength,
                  const uint8_t* payload, uint32_t payloadLength);
    void close();
    bool initTicket(const std::string& ticket) { return true; }

    unsigned short getListeningPort();

    // Implements TransportSession::Listener
    void onData(uint32_t id, TransportData data) override;
    void onClose(uint32_t id) override;

    void sendSessionData(int id, const uint8_t* data, uint32_t len);
    void closeSession(int id);

private:
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLSocket;

    void doAccept();
    void acceptHandler(const boost::system::error_code&);
    void handshakeHandler(std::shared_ptr<SSLSocket> sock,
                          const boost::system::error_code& ec);
    void onSessionRemoved(int id);

    int m_nextSessionId;
    std::unordered_map<int, std::shared_ptr<TransportSession>> m_sessions;

    std::shared_ptr<IOService> m_service;

    bool m_isSecure;
    std::string m_pass;
    std::unique_ptr<boost::asio::ssl::context> m_sslContext;
    std::shared_ptr<SSLSocket> m_sslSocket;

    boost::asio::ip::tcp::socket m_socket;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::atomic<bool> m_isClosed;
    Listener* m_listener;
};

} /* namespace owt_base */
#endif /* TransportServer_h */
