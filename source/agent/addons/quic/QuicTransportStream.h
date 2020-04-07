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
    class Visitor {
        virtual void onEnded() = 0;
    };
    // `sessionId` is the ID of publication or subscription, NOT the ID of QUIC session.
    explicit QuicTransportStream(owt::quic::QuicTransportStreamInterface* stream);

protected:
    // Overrides owt::quic::QuicTransportStreamInterface::Visitor.
    void OnCanRead() override;
    void OnCanWrite() override;

private:
    owt::quic::QuicTransportStreamInterface* m_stream;
};

#endif