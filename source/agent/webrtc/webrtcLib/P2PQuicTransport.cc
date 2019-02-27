/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "P2PQuicTransport.h"
#include "net/third_party/quiche/src/quic/core/crypto/proof_verifier.h"
#include "net/third_party/quiche/src/quic/core/quic_crypto_client_stream.h"
#include "net/third_party/quiche/src/quic/core/quic_crypto_stream.h"
#include "net/third_party/quiche/src/quic/core/quic_types.h"
#include "net/third_party/quiche/src/quic/core/tls_client_handshaker.h"
#include "net/quic/platform/impl/quic_chromium_clock.cc"
#include "net/third_party/quiche/src/quic/quartc/quartc_crypto_helpers.h"
#include "third_party/blink/renderer/modules/peerconnection/adapters/p2p_quic_crypto_config_factory_impl.h"
#include "third_party/webrtc/rtc_base/ssl_certificate.h"

DEFINE_LOGGER(P2PQuicStream, "P2PQuicStream");
DEFINE_LOGGER(P2PQuicTransport, "P2PQuicTransport");

static const size_t s_hostnameLength = 32;

P2PQuicStream::P2PQuicStream(quic::QuartcStream* stream)
    : m_quartcStream(stream)
{
    stream->SetDelegate(this);
}

size_t P2PQuicStream::OnReceived(quic::QuartcStream* stream, iovec* iov, size_t iov_length, bool fin){
        ELOG_DEBUG("OnReceived");
    size_t bytes_consumed=0;
    for (size_t i = 0; i < iov_length; ++i) {
      bytes_consumed += iov[i].iov_len;
    }
    return bytes_consumed;
}

void P2PQuicStream::OnClose(quic::QuartcStream* stream)
{
    ELOG_DEBUG("OnClose");
}

void P2PQuicStream::OnBufferChanged(quic::QuartcStream* stream)
{
    ELOG_DEBUG("OnBufferChanged");
}

P2PQuicTransport::P2PQuicTransport(
    std::unique_ptr<quic::QuicConnection> connection,
    const quic::QuicConfig& config,
    quic::QuicClock* clock,
    std::shared_ptr<quic::QuartcPacketWriter> packetWriter,
    std::shared_ptr<quic::QuicCryptoServerConfig> cryptoServerConfig,
    quic::QuicCompressedCertsCache* const compressedCertsCache)
    : quic::QuartcServerSession(std::move(connection), nullptr, config, quic::CurrentSupportedVersions(), clock, cryptoServerConfig.get(), compressedCertsCache, new quic::QuartcCryptoServerStreamHelper())
{
    quic::QuartcServerSession::SetDelegate(this);
    m_writer = packetWriter;
    m_cryptoServerConfig = cryptoServerConfig;
    m_delegate = nullptr;
}

std::unique_ptr<P2PQuicTransport> P2PQuicTransport::create(const quic::QuartcSessionConfig& quartcSessionConfig,
    quic::Perspective perspective,
    std::shared_ptr<quic::QuartcPacketTransport> transport,
    quic::QuicClock* clock,
    std::shared_ptr<quic::QuicAlarmFactory> alarmFactory,
    std::shared_ptr<quic::QuicConnectionHelperInterface> helper,
    std::shared_ptr<quic::QuicCryptoServerConfig> cryptoServerConfig,
    quic::QuicCompressedCertsCache* const compressedCertsCache)
{
    ELOG_DEBUG("Create quic::QuartcPacketWriter.");
    auto writer = std::make_shared<quic::QuartcPacketWriter>(transport.get(), quartcSessionConfig.max_packet_size);
    quic::QuicConfig quicConfig = quic::CreateQuicConfig(quartcSessionConfig);
    ELOG_DEBUG("Create QUIC connection.");
    std::unique_ptr<quic::QuicConnection> quicConnection = createQuicConnection(perspective, writer, alarmFactory, helper);
    return std::make_unique<P2PQuicTransport>(std::move(quicConnection), quicConfig, clock, writer, cryptoServerConfig, compressedCertsCache);
}
std::unique_ptr<quic::QuicConnection> P2PQuicTransport::createQuicConnection(quic::Perspective perspective,
        std::shared_ptr<quic::QuartcPacketWriter> writer,
        std::shared_ptr<quic::QuicAlarmFactory> alarmFactory,
        std::shared_ptr<quic::QuicConnectionHelperInterface> connectionHelper)
{
    ELOG_DEBUG("P2PQuicTransport::createQuicConnection");
    // Copied from net/third_party/quiche/src/quic/quartc/quartc_factory.cc.
    // |dummyId| and |dummyAddress| are used because Quartc network layer will
    // not use these two.
    quic::QuicConnectionId dummyId;
    char connectionIdBytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    dummyId = quic::QuicConnectionId(connectionIdBytes, sizeof(connectionIdBytes));
    quic::QuicSocketAddress dummyAddress(quic::QuicIpAddress::Any4(), /*port=*/0);
    return quic::CreateQuicConnection(dummyId, dummyAddress, connectionHelper.get(), alarmFactory.get(), writer.get(), perspective, quic::CurrentSupportedVersions());
}

std::vector<rtc::scoped_refptr<rtc::RTCCertificate>> P2PQuicTransport::getCertificates() const
{
    return std::vector<rtc::scoped_refptr<rtc::RTCCertificate>>();
}

void P2PQuicTransport::start(std::unique_ptr<RTCQuicParameters> remoteParameters)
{
    ELOG_DEBUG("P2PQuicTransport::start.");
    StartCryptoHandshake();
    QuartcServerSession::Initialize();
    ELOG_DEBUG("After start crypto handshake.");
}

RTCQuicParameters P2PQuicTransport::getLocalParameters() const {
    return RTCQuicParameters();
}

void P2PQuicTransport::OnCryptoHandshakeComplete()
{
    ELOG_DEBUG("P2PQuicTransport::OnCryptoHandshakeComplete");
}

void P2PQuicTransport::OnConnectionWritable()
{
    ELOG_DEBUG("P2PQuicTransport::OnConnectionWritable");
}

void P2PQuicTransport::OnIncomingStream(quic::QuartcStream* stream)
{
    ELOG_DEBUG("P2PQuicTransport::OnIncomingStream");
    std::unique_ptr<P2PQuicStream> p2pQuicStream = std::make_unique<P2PQuicStream>(stream);
    m_streams.push_back(std::move(p2pQuicStream));
    if (m_delegate) {
        return;
        //m_delegate->OnStream(stream);
    }
}

void P2PQuicTransport::OnCongestionControlChange(quic::QuicBandwidth bandwidth_estimate,
    quic::QuicBandwidth pacing_rate,
    quic::QuicTime::Delta latest_rtt)
{
    ELOG_DEBUG("P2PQuicTransport::OnCongestionControlChange");
}

void P2PQuicTransport::OnConnectionClosed(quic::QuicErrorCode error_code,
    const std::string& error_details,
    quic::ConnectionCloseSource source)
{
    ELOG_DEBUG("P2PQuicTransport::OnConnectionClosed");
}

void P2PQuicTransport::OnMessageReceived(quic::QuicStringPiece message)
{
    ELOG_DEBUG("P2PQuicTransport::OnMessageReceived");
}

P2PQuicTransport::~P2PQuicTransport()
{
    ELOG_DEBUG("~P2PQuicTransport");
}
