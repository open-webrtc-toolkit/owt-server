// Copyright (C) <2019> Intel Corporation
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
class TransportServer : public RawTransportInterface,
                        public TransportSession::Listener {
    DECLARE_LOGGER();
public:
    class Listener {
    public:
        virtual void onSessionAdded(int id) = 0;
        virtual void onSessionData(int id, char* data, int len) = 0;
        virtual void onSessionRemoved(int id) = 0;
    };
    TransportServer(Listener* listener);
    ~TransportServer();

    // Implements RawTransportInterface
    void createConnection(const std::string& ip, uint32_t port) {}
    void listenTo(uint32_t port);
    void listenTo(uint32_t minPort, uint32_t maxPort);
    void sendData(const char*, int len);
    void sendData(const char* header, int headerLength, const char* payload, int payloadLength);
    void close();
    bool initTicket(const std::string& ticket) { return true; }

    unsigned short getListeningPort();

    // Implements TransportSession::Listener
    void onData(uint32_t id, TransportData data) override;
    void onClose(uint32_t id) override;

    void sendSessionData(int id, const char* data, int len);
    void closeSession(int id);

private:
    void doAccept();
    void acceptHandler(const boost::system::error_code&);
    void onSessionRemoved(int id);

    int m_nextSessionId;
    std::unordered_map<int, std::shared_ptr<TransportSession>> m_sessions;

    std::shared_ptr<IOService> m_service;

    boost::asio::ip::tcp::socket m_socket;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::atomic<bool> m_isClosed;
    Listener* m_listener;
};

} /* namespace owt_base */
#endif /* TransportServer_h */
