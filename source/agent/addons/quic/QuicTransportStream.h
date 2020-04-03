/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_QUICTRANSPORTSTREAM_H_
#define QUIC_QUICTRANSPORTSTREAM_H_

#include "owt/quic/quic_transport_stream_interface.h"
#include <logger.h>
#include <string>

class QuicTransportStream : public owt::quic::QuicTransportStreamInterface::Visitor {
    DECLARE_LOGGER();

public:
    // `sessionId` is the ID of publication or subscription, NOT the ID of QUIC session.
    explicit QuicTransportStream(const std::string& sessionId, const std::string& userId);

private:
    const std::string m_sessionId;
    const std::string m_userId;
};

#endif