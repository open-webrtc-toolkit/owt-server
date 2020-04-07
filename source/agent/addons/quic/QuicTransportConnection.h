/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_QUICTRANSPORTCONNECTION_H_
#define QUIC_QUICTRANSPORTCONNECTION_H_

#include "owt/quic/quic_transport_session_interface.h"
#include "QuicTransportStream.h"
#include <logger.h>
#include <memory>
#include <unordered_map>
#include <vector>

class QuicTransportConnection : public owt::quic::QuicTransportSessionInterface::Visitor, QuicTransportStream::Visitor {
    DECLARE_LOGGER();

public:
    class Visitor {
        // Authentication passed with user ID `id`.
        virtual void onAuthentication(const std::string& id) = 0;
        // Connection is closed.
        virtual void onClose() = 0;
    };
    explicit QuicTransportConnection(owt::quic::QuicTransportSessionInterface* session);
    void setVisitor(Visitor* visitor);

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
};

#endif