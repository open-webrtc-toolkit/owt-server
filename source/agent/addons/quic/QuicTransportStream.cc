/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicTransportStream.h"

DEFINE_LOGGER(QuicTransportStream, "QuicTransportStream");

QuicTransportStream::QuicTransportStream(owt::quic::QuicTransportStreamInterface* stream)
    : m_stream(stream)
{
}

void QuicTransportStream::OnCanRead()
{
    ELOG_DEBUG("On can read.");
}

void QuicTransportStream::OnCanWrite()
{
    ELOG_DEBUG("On can write.");
}

void QuicTransportStream::Write(uint8_t* data, size_t length)
{
    m_stream->Write(data, length);
}

void QuicTransportStream::Close(){
    m_stream->Close();
}