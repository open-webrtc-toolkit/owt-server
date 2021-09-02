/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_ADDON_WEB_TRANSPORT_FRAME_DESTINATION_H_
#define QUIC_ADDON_WEB_TRANSPORT_FRAME_DESTINATION_H_

#include "../../core/owt_base/MediaFramePipeline.h"
#include "../common/MediaFramePipelineWrapper.h"
#include "VideoRtpPacketizer.h"
#include "owt/quic/web_transport_stream_interface.h"
#include <logger.h>
#include <nan.h>

// A WebTransportFrameDestination is a hub for a single InternalIO input to multiple WebTransport outputs.
class WebTransportFrameDestination : public owt_base::FrameDestination, public NanFrameNode {
    DECLARE_LOGGER();

public:
    explicit WebTransportFrameDestination();
    ~WebTransportFrameDestination();

    static NAN_MODULE_INIT(init);

    // Overrides owt_base::FrameDestination.
    void onFrame(const owt_base::Frame&) override;
    void onVideoSourceChanged() override;

    // Overrides NanFrameNode.
    owt_base::FrameSource* FrameSource() override { return nullptr; }
    owt_base::FrameDestination* FrameDestination() override { return this; }

private:
    static Nan::Persistent<v8::Function> s_constructor;
    static NAN_METHOD(newInstance);
    static NAN_METHOD(addDatagramOutput);
    // receiver() is required by connection.js.
    static NAN_METHOD(receiver);

    NanFrameNode* m_datagramOutput;
    std::unique_ptr<VideoRtpPacketizer> m_videoRtpPacketizer;
};

#endif