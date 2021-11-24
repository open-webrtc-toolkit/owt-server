/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_QUICTRANSPORTCONNECTION_H_
#define QUIC_QUICTRANSPORTCONNECTION_H_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <logger.h>
#include <nan.h>

#include "QuicTransportStream.h"
#include "owt/quic/web_transport_session_interface.h"

class QuicTransportConnection : public owt_base::FrameDestination, public NanFrameNode, public owt::quic::WebTransportSessionInterface::Visitor, QuicTransportStream::Visitor {
    DECLARE_LOGGER();

public:
    class Visitor {
        // Authentication passed with user ID `id`.
        virtual void onAuthentication(const std::string& id) = 0;
        // Connection is closed.
        virtual void onClose() = 0;
    };
    explicit QuicTransportConnection();
    ~QuicTransportConnection();
    void setVisitor(Visitor* visitor);
    static v8::Local<v8::Object> newInstance(owt::quic::WebTransportSessionInterface* session);

    static NAN_MODULE_INIT(init);
    static NAN_METHOD(newInstance);
    static NAN_METHOD(createBidirectionalStream);
    // Close a WebTransport session, take an optional argument of WebTransportCloseInfo defined in https://w3c.github.io/webtransport/#web-transport-close-info.
    static NAN_METHOD(close);
    static NAUV_WORK_CB(onStreamCallback);
    static NAUV_WORK_CB(onCloseCallback);

    static Nan::Persistent<v8::Function> s_constructor;

    // Overrides owt_base::FrameDestination.
    void onFrame(const owt_base::Frame&) override;
    void onVideoSourceChanged() override;

    // Overrides NanFrameNode.
    owt_base::FrameSource* FrameSource() override { return nullptr; }
    owt_base::FrameDestination* FrameDestination() override { return this; }

protected:
    // Overrides owt::quic::WebTransportSessionInterface::Visitor.
    void OnIncomingStream(owt::quic::WebTransportStreamInterface*) override;
    void OnCanCreateNewOutgoingStream(bool unidirectional) override { }
    void OnDatagramReceived(const uint8_t* data, size_t length) override { }
    void OnConnectionClosed() override;

    // Overrides QuicTransportStream::Visitor.
    void onEnded() override;

private:
    owt::quic::WebTransportSessionInterface* m_session;
    Visitor* m_visitor;
    // Key is content session ID, i.e.: publication ID, subscription ID.
    std::unordered_map<std::string, std::unique_ptr<QuicTransportStream>> m_streams;
    // Move to `m_streams` after associate with a publication or subscription.
    std::vector<std::unique_ptr<QuicTransportStream>> m_unassociatedStreams;

    uv_async_t m_asyncOnStream;
    std::mutex m_streamQueueMutex;
    std::queue<owt::quic::WebTransportStreamInterface*> m_streamsToBeNotified;
    uv_async_t m_asyncOnClose;
};

#endif