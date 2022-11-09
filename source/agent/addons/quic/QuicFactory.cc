/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicFactory.h"
#include "owt/quic/web_transport_factory.h"
#include "owt/quic/logging.h"
#include <mutex>

DEFINE_LOGGER(QuicFactory, "QuicFactory");

std::once_flag getQuicFactoryOnce;

std::shared_ptr<QuicFactory> QuicFactory::s_quicFactory = nullptr;

QuicFactory::QuicFactory()
    : m_quicTransportFactory(std::shared_ptr<owt::quic::WebTransportFactory>(owt::quic::WebTransportFactory::Create()))
{
}

std::shared_ptr<owt::quic::WebTransportFactory> QuicFactory::getQuicTransportFactory()
{
    std::call_once(getQuicFactoryOnce, []() {
        QuicFactory* factory = new QuicFactory();
        s_quicFactory = std::shared_ptr<QuicFactory>(factory);
    });
    return s_quicFactory->m_quicTransportFactory;
}