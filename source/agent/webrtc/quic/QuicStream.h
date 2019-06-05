/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_QUICSTREAM_H_
#define WEBRTC_QUICSTREAM_H_

#include <vector>
#include <logger.h>
#include <nan.h>
#include "net/third_party/quiche/src/quic/core/quic_session.h"
#include "net/third_party/quiche/src/quic/quartc/quartc_stream.h"
#include "P2PQuicTransport.h"
#include "../../addons/common/MediaFramePipelineWrapper.h"

// Node.js addon of BidirectionalStream.
// https://w3c.github.io/webrtc-quic/webtransport.html#bidirectional-stream*
// Name it as QuicStream since it is a QUIC implementation of BidirectionalStream.
class QuicStream : public Nan::ObjectWrap, P2PQuicStream::Delegate, owt_base::FrameSource, owt_base::FrameDestination {
    DECLARE_LOGGER();

public:
    static NAN_MODULE_INIT(Init);

    static v8::Local<v8::Object> newInstance(P2PQuicStream* p2pQuicStream);

protected:
    // Implements P2PQuicStream::Delegate.
    virtual void OnDataReceived(std::vector<uint8_t> data, bool fin) override;
    // Implements FrameDestination.
    virtual void onFrame(const owt_base::Frame& frame) override;

private:
    explicit QuicStream(P2PQuicStream* p2pQuicStream);
    static NAN_METHOD(newInstance);
    static NAN_METHOD(write);
    static NAUV_WORK_CB(onDataCallback);

    // JavaScript API for |owt_base::FrameSource|.
    static NAN_METHOD(addDestination);

    static Nan::Persistent<v8::Function> s_constructor;
    P2PQuicStream* m_p2pQuicStream;
    uv_async_t m_asyncOnData;
    std::mutex m_dataQueueMutex;
    std::queue<std::vector<uint8_t>> m_dataToBeNotified;
    bool m_deliveryDataToCppSink;
};

#endif // WEBRTC_QUICSTREAM_H_