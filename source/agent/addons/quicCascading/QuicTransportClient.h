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
    static NAN_METHOD(onConnection);
    static NAN_METHOD(onConnectionFailed);
    static NAN_METHOD(onConnectionClosed);
    static NAN_METHOD(onNewStream);
    static NAN_METHOD(onClosedStream);
    static NAN_METHOD(close);
    static NAN_METHOD(createBidirectionalStream);
    static NAN_METHOD(getId);
    static NAN_METHOD(closeStream);

    static NAUV_WORK_CB(onConnectedCallback);
    static NAUV_WORK_CB(onConnectionFailedCallback);
    static NAUV_WORK_CB(onConnectionClosedCallback);
    static NAUV_WORK_CB(onNewStreamCallback);
    static NAUV_WORK_CB(onStreamClosedCallback);

protected:
    explicit QuicTransportClient(const char* dest_ip, int dest_port);
    virtual ~QuicTransportClient();

    // Implements QuicTransportClientInterface.
    void OnConnected() override;
    void OnConnectionFailed() override;
    void OnConnectionClosed(char* sessionId, size_t len) override;
    void OnIncomingStream(owt::quic::QuicTransportStreamInterface*) override;
    void OnStreamClosed(uint32_t id) override;

private:

    std::shared_ptr<owt::quic::QuicTransportClientInterface> m_quicClient;
    //std::unordered_map<uint32_t, v8::Local<v8::Object>> streams_;

    uv_async_t m_asyncOnConnected;
    uv_async_t m_asyncOnConnectFail;
    uv_async_t m_asyncOnConnectionClosed;
    uv_async_t m_asyncOnNewStream;
    uv_async_t m_asyncOnStreamClosed;

    bool has_connected_callback_;
    bool has_connectionFailed_callback_;
    bool has_connectionClosed_callback_;
    bool has_stream_callback_;
    bool has_streamClosed_callback_;

    Nan::AsyncResource *asyncResourceConnection_;
    Nan::AsyncResource *asyncResourceFailedConnection_;
    Nan::AsyncResource *asyncResourceClosedConnection_;
    Nan::AsyncResource *asyncResourceStream_;
    Nan::AsyncResource *asyncResourceStreamClosed_;

    std::queue<owt::quic::QuicTransportStreamInterface*> stream_messages;
    std::queue<uint32_t> streamclosed_messages;
    std::queue<std::string> sessionId_messages;

    Nan::Callback *connected_callback_;
    Nan::Callback *connectionfailed_callback_;
    Nan::Callback *connectionClosed_callback_;
    Nan::Callback *stream_callback_;
    Nan::Callback *streamClosed_callback_;

    boost::mutex mutex;
    static Nan::Persistent<v8::Function> s_constructor;
};

#endif  // QUIC_TRANSPORT_CLIENT_H_
