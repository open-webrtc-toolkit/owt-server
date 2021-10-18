/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_QUICTRANSPORTSTREAM_H_
#define QUIC_QUICTRANSPORTSTREAM_H_

#include "../../core/owt_base/MediaFramePipeline.h"
#include "../common/MediaFramePipelineWrapper.h"
#include "owt/quic/web_transport_stream_interface.h"
#include <logger.h>
#include <mutex>
#include <nan.h>
#include <string>

class QuicTransportStream : public owt_base::FrameSource, public owt_base::FrameDestination, public NanFrameNode, public owt::quic::WebTransportStreamInterface::Visitor {
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
    // Read 128 bits after content session ID. Only media streams have track ID. Result will be returned by onData callback.
    // TODO: Make this as an async method when it's supported.
    static NAN_METHOD(readTrackId);

    static NAN_GETTER(trackKindGetter);
    static NAN_SETTER(trackKindSetter);
    static NAN_GETTER(onDataGetter);
    static NAN_SETTER(onDataSetter);

    static NAUV_WORK_CB(onContentSessionId);
    static NAUV_WORK_CB(onTrackId);
    // Only signaling stream (UUID: 0) invokes this method.
    static NAUV_WORK_CB(onData); // TODO: Move to pipe.

    // Overrides owt_base::FrameSource.
    void onFeedback(const owt_base::FeedbackMsg&) override;

    // Overrides owt_base::FrameDestination.
    void onFrame(const owt_base::Frame&) override;
    void onVideoSourceChanged() override;

    // Overrides NanFrameNode.
    owt_base::FrameSource* FrameSource() override { return this; }
    owt_base::FrameDestination* FrameDestination() override { return this; }

    static Nan::Persistent<v8::Function> s_constructor;

protected:
    // Overrides owt::quic::WebTransportStreamInterface::Visitor.
    void OnCanRead() override;
    void OnCanWrite() override;
    void OnFinRead() override;

    // Overrides owt_base::FrameSource.
    void addAudioDestination(owt_base::FrameDestination*) override;
    void addVideoDestination(owt_base::FrameDestination*) override;
    void addDataDestination(owt_base::FrameDestination*) override;

private:
    // Read content session ID from data buffered.
    void ReadContentSessionId();
    void ReadTrackId();
    void SignalOnData();
    void ReallocateBuffer(size_t size);
    // Check whether there is readable data. If so, fire ondata event.
    void CheckReadableData();
    void AddedDestination();
    void RemovedDestination();

    owt::quic::WebTransportStreamInterface* m_stream;
    std::vector<uint8_t> m_contentSessionId;
    size_t m_receivedContentSessionIdSize;
    std::vector<uint8_t> m_trackId;
    size_t m_receivedTrackIdSize;
    bool m_readingTrackId;
    // True if it's piped to a receiver in C++ layer. In this case, data will not be sent to JavaScript code.
    bool m_isPiped;
    // If a stream is piped but doesn't have a sink, no data from WebTransport stream will be consumed.
    bool m_hasSink;
    uint8_t* m_buffer;
    size_t m_bufferSize;

    // Indicates the kind of this stream, could be 'audio', 'video', 'data'. If this is not a data track, it can only be piped to another QUIC agent.
    std::string m_trackKind;
    owt_base::FrameFormat m_frameFormat;
    Nan::Persistent<v8::Value> m_onDataCallback;

    size_t m_readingFrameSize;
    size_t m_frameSizeOffset;
    uint8_t* m_frameSizeArray;
    size_t m_currentFrameSize;
    size_t m_receivedFrameOffset;

    uv_async_t m_asyncOnContentSessionId;
    uv_async_t m_asyncOnTrackId;
    uv_async_t m_asyncOnData;
};

#endif