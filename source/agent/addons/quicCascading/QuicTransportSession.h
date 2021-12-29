// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef QUIC_TRANSPORT_SESSION_H_
#define QUIC_TRANSPORT_SESSION_H_

#include <mutex>
#include <nan.h>
#include <unordered_map>
#include <queue>
#include <logger.h>
#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/mutex.hpp>

#include "QuicTransportStream.h"
#include "owt/quic/quic_transport_session_interface.h"
#include "owt/quic/quic_transport_stream_interface.h"

/*
 * Wrapper class of QuicTransportSession
 *
 * Receives media from one
 */
class QuicTransportSession : public owt::quic::QuicTransportSessionInterface::Visitor, public Nan::ObjectWrap {
    DECLARE_LOGGER();
public:
    explicit QuicTransportSession();
    virtual ~QuicTransportSession();

    static NAN_MODULE_INIT(init);

    static NAN_METHOD(newInstance);
    static NAN_METHOD(createBidirectionalStream);
    static NAN_METHOD(onNewStream);
    static NAN_METHOD(getId);

    static NAUV_WORK_CB(onNewStreamCallback);

    // Implements QuicTransportSessionInterface.
    void OnIncomingStream(owt::quic::QuicTransportStreamInterface*) override;
    static v8::Local<v8::Object> newInstance(owt::quic::QuicTransportSessionInterface* session);
private:

    owt::quic::QuicTransportSessionInterface* m_session;
    uv_async_t m_asyncOnNewStream;
    bool has_stream_callback_;
    std::queue<owt::quic::QuicTransportStreamInterface*> stream_messages;
    Nan::Callback *stream_callback_;
    boost::mutex mutex;
    std::unordered_map<std::string, std::unique_ptr<owt::quic::QuicTransportStreamInterface>> streams_;
    static Nan::Persistent<v8::Function> s_constructor;
};

#endif  // QUIC_TRANSPORT_SESSION_H_
