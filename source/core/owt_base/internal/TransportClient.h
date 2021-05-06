// Copyright (C) <2019> Intel Corporation
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

// const char TDT_FEEDBACK_MSG = 0x5A;
// const char TDT_MEDIA_FRAME = 0x8F;

using boost::asio::ip::tcp;

/*
 * TransportClient
 */
class TransportClient : public RawTransportInterface,
                        public TransportSession::Listener {
    DECLARE_LOGGER();
public:
    class Listener {
    public:
        virtual void onConnected() = 0;
        virtual void onData(char* data, int len) = 0;
        virtual void onDisconnected() = 0;
    };
    TransportClient(Listener* listener);
    ~TransportClient();

    void createConnection(const std::string& ip, uint32_t port);
    void listenTo(uint32_t port) {}
    void listenTo(uint32_t minPort, uint32_t maxPort) {}
    void sendData(const char*, int len);
    void sendData(const char* header, int headerLength, const char* payload, int payloadLength);
    void close();
    bool initTicket(const std::string& ticket) { return true; }

    unsigned short getListeningPort() { return 0; }

    // Implements TransportSession::Listener
    void onData(uint32_t id, TransportData data) override;
    void onClose(uint32_t id) override;

private:
    void connectHandler(const boost::system::error_code&);

    std::shared_ptr<IOService> m_service;
    boost::asio::ip::tcp::socket m_socket;
    std::shared_ptr<TransportSession> m_session;

    Listener* m_listener;
};

} /* namespace owt_base */
#endif /* TransportServer_h */
