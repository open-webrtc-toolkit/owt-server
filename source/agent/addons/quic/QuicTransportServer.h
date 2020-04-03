/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_QUICTRANSPORTSERVER_H_
#define QUIC_QUICTRANSPORTSERVER_H_

#include "owt/quic/quic_transport_server_interface.h"
#include "owt/quic/quic_transport_session_interface.h"
#include <logger.h>
#include <nan.h>
#include <thread>

class QuicTransportServer : public Nan::ObjectWrap, owt::quic::QuicTransportServerInterface::Visitor, owt::quic::QuicTransportSessionInterface::Visitor {
    DECLARE_LOGGER();

public:
    static NAN_MODULE_INIT(Init);

protected:
    // Override owt::quic::QuicTransportServerInterface::Visitor.
    void OnEnded() override;
    void OnSession(owt::quic::QuicTransportSessionInterface*) override;

    // Override owt::quic::QuicTransportSessionInterface::Visitor.
    void OnIncomingStream(owt::quic::QuicTransportStreamInterface*) override;

    QuicTransportServer() = delete;
    explicit QuicTransportServer(int port);
    static NAN_METHOD(newInstance);
    static NAN_METHOD(start);
    static NAN_METHOD(stop);

    std::unique_ptr<owt::quic::QuicTransportServerInterface> m_quicServer;

    static Nan::Persistent<v8::Function> s_constructor;
};

#endif