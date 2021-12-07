// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef QUIC_TRANSPORT_CLIENT_H_
#define QUIC_TRANSPORT_CLIENT_H_

#include <string>
#include <nan.h>
#include <unordered_map>
#include <queue>
#include <logger.h>
#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/mutex.hpp>

#include "QuicTransportStream.h"
#include "owt/quic/quic_transport_client_interface.h"

/*
 * Wrapper class of TQuicClient
 *
 * Sends media to server
 */
class QuicTransportClient : public owt::quic::QuicTransportClientInterface::Visitor, public Nan::ObjectWrap {
    DECLARE_LOGGER();
public:

    static NAN_MODULE_INIT(init);

    // new QuicTransportFrameSource(contentSessionId, options)
    static NAN_METHOD(newInstance);
    static NAN_METHOD(connect);
    static NAN_METHOD(onNewStream);
    static NAN_METHOD(onConnection);
    static NAN_METHOD(createBidirectionalStream);

    static Nan::Persistent<v8::Function> s_constructor;

    static NAUV_WORK_CB(onNewStreamCallback);
    static NAUV_WORK_CB(onConnectedCallback);

protected:
    explicit QuicTransportClient(const char* dest_ip, int dest_port);
    virtual ~QuicTransportClient();

    // Implements QuicTransportClientInterface.
    void OnIncomingStream(owt::quic::QuicTransportStreamInterface*) override;
    void OnConnected() override;
    void OnConnectionFailed() override;

private:

    std::shared_ptr<owt::quic::QuicTransportClientInterface> m_quicClient;
    std::unordered_map<std::string, std::unique_ptr<QuicTransportStream>> streams_;

    uv_async_t m_asyncOnNewStream;
    uv_async_t m_asyncOnConnected;
    bool has_stream_callback_;
    bool has_connected_callback_;
    std::queue<owt::quic::QuicTransportStreamInterface*> stream_messages;
    Nan::Callback *stream_callback_;
    Nan::Callback *connected_callback_;
    boost::mutex mutex;
};

#endif  // QUIC_TRANSPORT_CLIENT_H_
