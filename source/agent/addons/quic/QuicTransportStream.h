/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_QUICTRANSPORTSTREAM_H_
#define QUIC_QUICTRANSPORTSTREAM_H_

#include "owt/quic/web_transport_stream_interface.h"
#include "../../core/owt_base/MediaFramePipeline.h"
#include <logger.h>
#include <nan.h>
#include <mutex>
#include <string>

class QuicTransportStream : public owt_base::FrameSource, public owt_base::FrameDestination, public Nan::ObjectWrap, public owt::quic::WebTransportStreamInterface::Visitor {
    DECLARE_LOGGER();

public:
    class Visitor {
        // Stream is ended.
        virtual void onEnded() = 0;
    };
    explicit QuicTransportStream();
    // `sessionId` is the ID of publication or subscription, NOT the ID of QUIC session.
    explicit QuicTransportStream(owt::quic::WebTransportStreamInterface* stream);
    virtual ~QuicTransportStream();

    static v8::Local<v8::Object> newInstance(owt::quic::WebTransportStreamInterface* stream);

    static NAN_MODULE_INIT(init);
    static NAN_METHOD(newInstance);
    static NAN_METHOD(write);
    static NAN_METHOD(close);
    static NAN_METHOD(addDestination);
    static NAN_METHOD(removeDestination);
    static NAUV_WORK_CB(onContentSessionId);
    static NAUV_WORK_CB(onData);  // TODO: Move to pipe.

    // Overrides owt_base::FrameSource.
    void onFeedback(const owt_base::FeedbackMsg&) override;

    // Overrides owt_base::FrameDestination.
    void onFrame(const owt_base::Frame&) override;
    void onVideoSourceChanged() override;

    static Nan::Persistent<v8::Function> s_constructor;

protected:
    // Overrides owt::quic::WebTransportStreamInterface::Visitor.
    void OnCanRead() override;
    void OnCanWrite() override;
    void OnFinRead() override;

private:
    // Try to read content session ID from data buffered.
    void MaybeReadContentSessionId();
    void SignalOnData();
    void ReallocateBuffer(size_t size);

    owt::quic::WebTransportStreamInterface* m_stream;
    std::vector<uint8_t> m_contentSessionId;
    bool m_receivedContentSessionId;
    // True if it's piped to a receiver in C++ layer. In this case, data will not be sent to JavaScript code.
    bool m_isPiped;
    uint8_t* m_buffer;
    size_t m_bufferSize;

    uv_async_t m_asyncOnContentSessionId;
    uv_async_t m_asyncOnData;
};

#endif