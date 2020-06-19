/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mutex>
#include "QuicFactory.h"
#include "owt/quic/quic_transport_factory.h"

DEFINE_LOGGER(QuicFactory, "QuicFactory");

std::once_flag getQuicFactoryOnce;

std::shared_ptr<QuicFactory> QuicFactory::s_quicFactory = nullptr;

QuicFactory::QuicFactory()
    : m_quicTransportFactory(std::shared_ptr<owt::quic::QuicTransportFactory>(owt::quic::QuicTransportFactory::Create()))
{
    ELOG_DEBUG("QuicFactory ctor.");
}

std::shared_ptr<owt::quic::QuicTransportFactory> QuicFactory::getQuicTransportFactory()
{
    ELOG_DEBUG("Before call once.");
    std::call_once(getQuicFactoryOnce, []() {
        ELOG_DEBUG("Before new.");
        QuicFactory* factory=new QuicFactory();
        ELOG_DEBUG("After new.");
        s_quicFactory = std::shared_ptr<QuicFactory>(factory);
    });
    ELOG_DEBUG("After call once.");
    return s_quicFactory->m_quicTransportFactory;
}