/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mutex>
#include "QuicFactory.h"
#include "owt/quic/quic_transport_factory.h"

std::once_flag getQuicFactoryOnce;

QuicFactory::QuicFactory()
    : m_quicTransportFactory(std::make_shared<owt::quic::QuicTransportFactory>())
{
}

std::shared_ptr<owt::quic::QuicTransportFactory> QuicFactory::getQuicTransportFactory()
{
    std::call_once(getQuicFactoryOnce, []() {
        s_quicFactory = std::shared_ptr<QuicFactory>();
    });
    return s_quicFactory->m_quicTransportFactory;
}