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
    class WebTransportFactory;
}
}

class QuicFactory {
public:
    DECLARE_LOGGER();
    virtual ~QuicFactory() = default;
    static std::shared_ptr<owt::quic::WebTransportFactory> getQuicTransportFactory();

private:
    explicit QuicFactory();

    static std::shared_ptr<QuicFactory> s_quicFactory;
    std::shared_ptr<owt::quic::WebTransportFactory> m_quicTransportFactory;
};

#endif