// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef QUIC_TRANSPORT_STREAM_H_
#define QUIC_TRANSPORT_STREAM_H_

#include <string>
#include <mutex>
#include <nan.h>
#include <unordered_map>
#include <queue>
#include <logger.h>
#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/mutex.hpp>

#include "../../core/owt_base/MediaFramePipeline.h"
#include "../common/MediaFramePipelineWrapper.h"
#include "owt/quic/quic_transport_stream_interface.h"

/*
 * Wrapper class of TQuicServer
 *
 * Receives media from one
 */
class QuicTransportStream : public owt_base::FrameSource, public owt_base::FrameDestination, public owt::quic::QuicTransportStreamInterface::Visitor, public NanFrameNode {
    DECLARE_LOGGER();
public:
    explicit QuicTransportStream();
    explicit QuicTransportStream(owt::quic::QuicTransportStreamInterface* stream);
    virtual ~QuicTransportStream();

    static v8::Local<v8::Object> newInstance(owt::quic::QuicTransportStreamInterface* stream);

    static NAN_MODULE_INIT(init);

    static NAN_METHOD(newInstance);
    static NAN_METHOD(addDestination);
    static NAN_METHOD(removeDestination);
    // addInputStream(stream, kind)
    // kind could be "data", "audio" or "video".
    static NAN_METHOD(addInputStream);
    static NAN_METHOD(close);
    static NAN_METHOD(send);
    static NAN_METHOD(onStreamData);
    static NAN_METHOD(getId);
    static NAN_GETTER(trackKindGetter);
    static NAN_SETTER(trackKindSetter);

    static NAUV_WORK_CB(onStreamDataCallback);


    // Overrides owt_base::FrameSource.
    void onFeedback(const owt_base::FeedbackMsg&) override;

    // Overrides owt_base::FrameDestination.
    void onFrame(const owt_base::Frame&) override;
    void onVideoSourceChanged() override;

    // Overrides NanFrameNode.
    owt_base::FrameSource* FrameSource() override { return this; }
    owt_base::FrameDestination* FrameDestination() override { return this; }

    void OnData(owt::quic::QuicTransportStreamInterface* stream, char* buf, size_t len) override;

    void sendData(const std::string& data);

    uint32_t id;
private:
    void sendFeedback(const owt_base::FeedbackMsg& msg);
    typedef struct {
        boost::shared_array<char> buffer;
        int length;
    } TransportData;

    std::unordered_map<std::string, bool> hasStream_;
    size_t m_bufferSize;
    TransportData m_receiveData;
    uint32_t m_receivedBytes;
    uv_async_t m_asyncOnData;
    bool has_data_callback_;
    std::queue<std::string> data_messages;
    Nan::Callback *data_callback_;
    //boost::mutex mutex;
    owt::quic::QuicTransportStreamInterface* m_stream;
    static Nan::Persistent<v8::Function> s_constructor;
    bool m_needKeyFrame;
    std::string m_trackKind;
};

#endif  // QUIC_TRANSPORT_SERVER_H_
