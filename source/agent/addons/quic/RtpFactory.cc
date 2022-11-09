/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RtpFactory.h"
#include "VideoRtpPacketizer.h"
#include "test/FakeVideoRtpPacketizer.h"

DEFINE_LOGGER(RtpFactoryBase, "RtpFactoryBase");

class RtpFactoryDefault : public RtpFactoryBase {
public:
    RtpFactoryDefault() = default;
    ~RtpFactoryDefault() override = default;
    std::unique_ptr<VideoRtpPacketizerInterface> createVideoPacketizer() override
    {
        return std::make_unique<VideoRtpPacketizer>();
    }
};

class RtpFactoryFake : public RtpFactoryBase {
public:
    RtpFactoryFake() = default;
    ~RtpFactoryFake() override = default;
    std::unique_ptr<VideoRtpPacketizerInterface> createVideoPacketizer() override
    {
        return std::make_unique<FakeVideoRtpPacketizer>();
    }
};

std::unique_ptr<RtpFactoryBase> RtpFactoryBase::createDefaultFactory()
{
    return std::make_unique<RtpFactoryDefault>();
}

std::unique_ptr<RtpFactoryBase> RtpFactoryBase::createFakeFactory()
{
    return std::make_unique<RtpFactoryFake>();
}