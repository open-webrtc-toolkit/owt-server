/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicTransportConnection.h"
#include "QuicTransportStream.h"

DEFINE_LOGGER(QuicTransportConnection, "QuicTransportConnection");

QuicTransportConnection::QuicTransportConnection(owt::quic::QuicTransportSessionInterface* session)
    : m_session(session)
    , m_visitor(nullptr)
{
}

void QuicTransportConnection::setVisitor(Visitor* visitor)
{
    m_visitor = visitor;
}

void QuicTransportConnection::OnIncomingStream(owt::quic::QuicTransportStreamInterface* stream)
{
    ELOG_DEBUG("OnIncomingStream");
    std::unique_ptr<QuicTransportStream> quicStream = std::make_unique<QuicTransportStream>(stream);
    stream->SetVisitor(quicStream.get());
    m_unassociatedStreams.push_back(std::move(quicStream));
}

void QuicTransportConnection::onEnded(){

}