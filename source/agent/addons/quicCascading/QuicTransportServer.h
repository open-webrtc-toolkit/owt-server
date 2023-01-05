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
    static NAN_METHOD(onClosedSession);
    static NAN_METHOD(getListenPort);

    static NAUV_WORK_CB(onNewSessionCallback);
    static NAUV_WORK_CB(onClosedSessionCallback);

protected:
    QuicTransportServer() = delete;
    virtual ~QuicTransportServer();
    explicit QuicTransportServer(unsigned int port, const std::string& pfxPath, const std::string& password);

    // Implements QuicTransportClientInterface.
    void OnSession(owt::quic::QuicTransportSessionInterface*) override;
    void OnClosedSession(char* sessionId, size_t len) override;
    void OnEnded() override;
private:

    unsigned int getListeningPort();

    std::unique_ptr<owt::quic::QuicTransportServerInterface> m_quicServer;
    uv_async_t m_asyncOnNewSession;
    uv_async_t m_asyncOnClosedSession;
    bool has_session_callback_;
    bool has_sessionClosed_callback_;
    std::queue<owt::quic::QuicTransportSessionInterface*> session_messages;
    std::queue<std::string> sessionId_messages;
    Nan::Callback *session_callback_;
    Nan::Callback *sessionClosed_callback_;
    Nan::AsyncResource *asyncResourceNewSession_;
    Nan::AsyncResource *asyncResourceClosedSession_;

    boost::mutex mutex;
    boost::mutex closedmutex;
    std::unordered_map<std::string, std::unique_ptr<owt::quic::QuicTransportSessionInterface*>> sessions_;
    static Nan::Persistent<v8::Function> s_constructor;
};

#endif  // QUIC_TRANSPORT_SERVER_H_
