/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_ADDON_WEB_TRANSPORT_FRAME_SOURCE_H_
#define QUIC_ADDON_WEB_TRANSPORT_FRAME_SOURCE_H_

#include "../../core/owt_base/MediaFramePipeline.h"
#include "../common/MediaFramePipelineWrapper.h"
#include "owt/quic/web_transport_stream_interface.h"
#include <logger.h>
#include <nan.h>

// A WebTransportFrameSource is a hub for multiple WebTransport inputs to a single InternalIO output.
class WebTransportFrameSource : public owt_base::FrameSource, public owt_base::FrameDestination, public NanFrameNode {
    DECLARE_LOGGER();

public:
    explicit WebTransportFrameSource();
    ~WebTransportFrameSource();

    static NAN_MODULE_INIT(init);

    // Overrides owt_base::FrameSource.
    void onFeedback(const owt_base::FeedbackMsg&) override;

    // Overrides owt_base::FrameDestination.
    void onFrame(const owt_base::Frame&) override;
    void onVideoSourceChanged() override;

    // Overrides NanFrameNode.
    owt_base::FrameSource* FrameSource() override { return this; }
    owt_base::FrameDestination* FrameDestination() override { return this; }

private:
    // new WebTransportFrameSource(contentSessionId, options)
    static NAN_METHOD(newInstance);
    static NAN_METHOD(addDestination);
    static NAN_METHOD(removeDestination);
    // addInputStream(stream, kind)
    // kind could be "data", "audio" or "video".
    static NAN_METHOD(addInputStream);

    static Nan::Persistent<v8::Function> s_constructor;

    owt::quic::WebTransportStreamInterface* m_audioStream;
    owt::quic::WebTransportStreamInterface* m_videoStream;
    owt::quic::WebTransportStreamInterface* m_dataStream;
};

#endif