/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_QUICTRANSPORTSTREAM_H_
#define QUIC_QUICTRANSPORTSTREAM_H_

#include "owt/quic/quic_transport_stream_interface.h"
#include <logger.h>
#include <nan.h>
#include <mutex>
#include <string>

// QuicTransportStream is expected to implement ReadableStream and WriteableStream. But we only implement the most essential APIs at first. It works in flowing mode only.
// As QuicTransportStream doesn't implement EventEmitter, caller should bind event handler as `onevent=func` instead of `on('event', func)`.
class QuicTransportStream : public Nan::ObjectWrap, public owt::quic::QuicTransportStreamInterface::Visitor {
    DECLARE_LOGGER();

public:
    class Visitor {
        // Stream is ended.
        virtual void onEnded() = 0;
    };
    explicit QuicTransportStream();
    // `sessionId` is the ID of publication or subscription, NOT the ID of QUIC session.
    explicit QuicTransportStream(owt::quic::QuicTransportStreamInterface* stream);
    ~QuicTransportStream();

    static v8::Local<v8::Object> newInstance(owt::quic::QuicTransportStreamInterface* stream);

    static NAN_MODULE_INIT(init);
    static NAN_METHOD(newInstance);
    static NAUV_WORK_CB(onContentSessionId);

    static Nan::Persistent<v8::Function> s_constructor;

protected:
    // Overrides owt::quic::QuicTransportStreamInterface::Visitor.
    void OnCanRead() override;
    void OnCanWrite() override;

private:
    // Try to read content session ID from data buffered.
    void MaybeReadContentSessionId();

    owt::quic::QuicTransportStreamInterface* m_stream;
    std::string m_contentSessionId;
    bool m_receivedContentSessionId;

    uv_async_t m_asyncOnContentSessionId;
};

#endif