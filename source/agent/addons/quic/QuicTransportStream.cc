/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicTransportStream.h"

DEFINE_LOGGER(QuicTransportStream, "QuicTransportStream");

QuicTransportStream::QuicTransportStream(const std::string& sessionId, const std::string& userId)
    : m_sessionId(sessionId)
    , m_userId(userId)
{
}