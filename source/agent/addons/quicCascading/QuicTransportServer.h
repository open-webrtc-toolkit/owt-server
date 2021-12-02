// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef QUIC_TRANSPORT_SERVER_H_
#define QUIC_TRANSPORT_SERVER_H_

#include <string>
#include <mutex>
#include <nan.h>
#include <unordered_map>
#include <queue>
#include <logger.h>
#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/mutex.hpp>

#include "QuicTransportSession.h"
#include "owt/quic/quic_transport_server_interface.h"
#include "owt/quic/quic_transport_session_interface.h"

/*
 * Wrapper class of TQuicServer
 *
 * Receives media from one
 */
class QuicTransportServer : public owt::quic::QuicTransportServerInterface::Visitor, public Nan::ObjectWrap {
    DECLARE_LOGGER();
public:
    static NAN_MODULE_INIT(init);

    static NAN_METHOD(newInstance);
    static NAN_METHOD(stop);
    static NAN_METHOD(start);
    static NAN_METHOD(onNewSession);

    static Nan::Persistent<v8::Function> s_constructor;

    static NAUV_WORK_CB(onNewSessionCallback);

protected:
    QuicTransportServer(unsigned int port, const std::string& cert_file, const std::string& key_file);
    virtual ~QuicTransportServer();
    QuicTransportServer() = delete;

    // Implements QuicTransportClientInterface.
    void OnSession(owt::quic::QuicTransportSessionInterface*) override;
private:

    unsigned int getServerPort();

    std::shared_ptr<owt::quic::QuicTransportServerInterface> m_quicServer;
    uv_async_t m_asyncOnNewSession;
    bool has_session_callback_;
    std::queue<owt::quic::QuicTransportSessionInterface*> session_messages;
    Nan::Callback *session_callback_;
    boost::mutex mutex;
    std::unordered_map<std::string, std::unique_ptr<owt::quic::QuicTransportSessionInterface*>> sessions_;
};

#endif  // QUIC_TRANSPORT_SERVER_H_
