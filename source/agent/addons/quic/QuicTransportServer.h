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
#include <mutex>
#include <nan.h>
#include <string>
#include <unordered_map>

class QuicTransportServer : public Nan::ObjectWrap, owt::quic::QuicTransportServerInterface::Visitor, QuicTransportConnection::Visitor {
    DECLARE_LOGGER();

public:
    static NAN_MODULE_INIT(init);

protected:
    // Overrides owt::quic::QuicTransportServerInterface::Visitor.
    void OnEnded() override;
    void OnSession(owt::quic::QuicTransportSessionInterface*) override;

    // Overrides QuicTransportConnection::Visitor.
    void onAuthentication(const std::string& id) override;
    // Connection is closed.
    void onClose() override;

    QuicTransportServer() = delete;
    explicit QuicTransportServer(int port, const std::string& pfxPath, const std::string& password);

private:
    static NAN_METHOD(newInstance);
    static NAN_METHOD(start);
    static NAN_METHOD(stop);
    static NAUV_WORK_CB(onConnectionCallback);

    std::unique_ptr<owt::quic::QuicTransportServerInterface> m_quicServer;

    uv_async_t m_asyncOnConnection;
    std::mutex m_connectionQueueMutex;
    std::queue<owt::quic::QuicTransportSessionInterface*> m_connectionsToBeNotified;

    static Nan::Persistent<v8::Function> s_constructor;
};

#endif