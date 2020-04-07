/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_QUICTRANSPORTSERVER_H_
#define QUIC_QUICTRANSPORTSERVER_H_

#include "QuicTransportConnection.h"
#include "owt/quic/quic_transport_server_interface.h"
#include <logger.h>
#include <nan.h>
#include <string>
#include <thread>
#include <unordered_map>

class QuicTransportServer : public Nan::ObjectWrap, owt::quic::QuicTransportServerInterface::Visitor, QuicTransportConnection::Visitor {
    DECLARE_LOGGER();

public:
    static NAN_MODULE_INIT(Init);

protected:
    // Overrides owt::quic::QuicTransportServerInterface::Visitor.
    void OnEnded() override;
    void OnSession(owt::quic::QuicTransportSessionInterface*) override;

    // Overrides QuicTransportConnection::Visitor.
    void onAuthentication(const std::string& id) override;
    // Connection is closed.
    void onClose() override;

    QuicTransportServer() = delete;
    explicit QuicTransportServer(int port);
    static NAN_METHOD(newInstance);
    static NAN_METHOD(start);
    static NAN_METHOD(stop);

    std::unique_ptr<owt::quic::QuicTransportServerInterface> m_quicServer;
    // Key is user ID, value is it's QUIC transport connection.
    std::unordered_map<std::string, std::unique_ptr<QuicTransportConnection>> m_connections;
    // Move to `m_connections` after authentication.
    std::vector<std::unique_ptr<QuicTransportConnection>> m_unauthenticatedConnections;

    static Nan::Persistent<v8::Function> s_constructor;
};

#endif