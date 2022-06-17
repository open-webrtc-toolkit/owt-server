/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_ADDON_WEB_TRANSPORT_FRAME_DESTINATION_H_
#define QUIC_ADDON_WEB_TRANSPORT_FRAME_DESTINATION_H_

#include "RtpFactory.h"
#include "owt/quic/web_transport_stream_interface.h"
#include <shared_mutex>
#include <unordered_map>
#include <logger.h>
#include <nan.h>

// A WebTransportFrameDestination is a hub for a single InternalIO input to multiple WebTransport outputs.
class WebTransportFrameDestination : public owt_base::FrameSource, public owt_base::FrameDestination, public NanFrameNode {
    DECLARE_LOGGER();

public:
    explicit WebTransportFrameDestination(const std::string& subscriptionId, bool isDatagram);
    ~WebTransportFrameDestination() override = default;

    static NAN_MODULE_INIT(init);

    // Overrides owt_base::FrameSource.
    void onFeedback(const owt_base::FeedbackMsg&) override;

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
    static NAN_METHOD(removeDatagramOutput);
    // addStreamOutput(string:trackId, WebTransportStream:stream).
    static NAN_METHOD(addStreamOutput);
    // removeStreamOutput(string:trackId).
    static NAN_METHOD(removeStreamOutput);
    // receiver() is required by connection.js.
    static NAN_METHOD(receiver);
    // Returns an object of RTP configuration. {audio:{ssrc}, video:{ssrc}}. Returns undefined if no RTP receiver is available.
    static NAN_GETTER(rtpConfigGetter);

    // Dispatch a media frame to its corresponding WebTransport stream. It works for WebTransport streams only.
    void DispatchMediaFrame(const owt_base::Frame&);

    bool m_isDatagram;
    std::shared_timed_mutex m_datagramOutputMutex;
    NanFrameNode* m_datagramOutput;
    std::shared_timed_mutex m_streamOutputMutex;
    std::unordered_map<std::string, NanFrameNode*> m_streamOutput; // Key is track ID.
    std::unique_ptr<RtpFactoryBase> m_rtpFactory;
    std::unique_ptr<VideoRtpPacketizerInterface> m_videoRtpPacketizer;
};

#endif