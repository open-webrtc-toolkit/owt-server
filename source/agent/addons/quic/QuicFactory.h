/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_QUICFACTORY_H_
#define QUIC_QUICFACTORY_H_

#include <memory>
#include <logger.h>

namespace owt {
namespace quic {
    class QuicTransportFactory;
}
}

class QuicFactory {
public:
    DECLARE_LOGGER();
    virtual ~QuicFactory() = default;
    static std::shared_ptr<owt::quic::QuicTransportFactory> getQuicTransportFactory();

private:
    explicit QuicFactory();

    static std::shared_ptr<QuicFactory> s_quicFactory;
    std::shared_ptr<owt::quic::QuicTransportFactory> m_quicTransportFactory;
};

#endif