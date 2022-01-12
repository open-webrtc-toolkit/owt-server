/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_RTPFACTORY_H_
#define QUIC_RTPFACTORY_H_

#include "RtpPacketizerInterface.h"
#include <logger.h>

class RtpFactoryBase {
public:
    DECLARE_LOGGER();
    virtual ~RtpFactoryBase() = default;
    // Get a default RTP factory.
    static std::unique_ptr<RtpFactoryBase> createDefaultFactory();
    // Get a fake RTP factory for testing. It doesn't depend on WebRTC.
    static std::unique_ptr<RtpFactoryBase> createFakeFactory();

    virtual std::unique_ptr<VideoRtpPacketizerInterface> createVideoPacketizer() = 0;

protected:
    RtpFactoryBase() = default;
};

#endif