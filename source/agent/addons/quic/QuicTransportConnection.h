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
#include "owt/quic/quic_transport_session_interface.h"

class QuicTransportConnection : public Nan::ObjectWrap, public owt::quic::QuicTransportSessionInterface::Visitor, QuicTransportStream::Visitor {
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
    static v8::Local<v8::Object> newInstance(owt::quic::QuicTransportSessionInterface* session);

    static NAN_MODULE_INIT(init);
    static NAN_METHOD(newInstance);
    static NAN_METHOD(createBidirectionalStream);
    static NAUV_WORK_CB(onStreamCallback);

    static Nan::Persistent<v8::Function> s_constructor;

protected:
    // Overrides owt::quic::QuicTransportSessionInterface::Visitor.
    void OnIncomingStream(owt::quic::QuicTransportStreamInterface*) override;

    // Overrides QuicTransportStream::Visitor.
    void onEnded() override;

private:
    owt::quic::QuicTransportSessionInterface* m_session;
    Visitor* m_visitor;
    // Key is content session ID, i.e.: publication ID, subscription ID.
    std::unordered_map<std::string, std::unique_ptr<QuicTransportStream>> m_streams;
    // Move to `m_streams` after associate with a publication or subscription.
    std::vector<std::unique_ptr<QuicTransportStream>> m_unassociatedStreams;

    uv_async_t m_asyncOnStream;
    std::mutex m_streamQueueMutex;
    std::queue<owt::quic::QuicTransportStreamInterface*> m_streamsToBeNotified;
};

#endif