/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Most classes in this file and its implementations are borrowed from Chromium/src/third_party/blink/renderer/modules/peerconnection/adapters/* with modifications.

#ifndef WEBRTC_P2PQUICTRANSPORTIMPL_H_
#define WEBRTC_P2PQUICTRANSPORTIMPL_H_

#include "IceConnectionAdapter.h"
#include "QuicDefinitions.h"
#include "base/at_exit.h"
#include "net/quic/quic_chromium_connection_helper.h"
#include "net/third_party/quiche/src/quic/core/crypto/quic_compressed_certs_cache.h"
#include "net/third_party/quiche/src/quic/core/crypto/quic_crypto_client_config.h"
#include "net/third_party/quiche/src/quic/quartc/quartc_factory.h"
#include "net/third_party/quiche/src/quic/quartc/quartc_session.h"
#include "third_party/webrtc/api/scoped_refptr.h"
#include "third_party/webrtc/rtc_base/rtc_certificate.h"
#include <logger.h>

class P2PQuicStream : public quic::QuartcStream::Delegate {
    DECLARE_LOGGER();

public:
    explicit P2PQuicStream(quic::QuartcStream* stream);
    virtual ~P2PQuicStream(){};

protected:
    // Implements quic::QuartcStream::Delegate.
    virtual size_t OnReceived(quic::QuartcStream* stream, iovec* iov, size_t iov_length, bool fin) override;
    virtual void OnClose(quic::QuartcStream* stream) override;
    virtual void OnBufferChanged(quic::QuartcStream* stream) override;

private:
    quic::QuartcStream* m_quartcStream;
};

// Some ideas of this class are borrowed from
// src/third_party/blink/renderer/modules/peerconnection/adapters/p2p_quic_transport_impl.h.
class P2PQuicTransport : public quic::QuartcServerSession,
                         public quic::QuartcSession::Delegate {
    DECLARE_LOGGER();

public:
    // Delegate for receiving callbacks from the QUIC transport.
    class Delegate {
    public:
        // Called when the remote side has created a new stream.
        virtual void OnStream(P2PQuicStream* stream) {}
    };
    static std::unique_ptr<P2PQuicTransport> create(const quic::QuartcSessionConfig& quartcSessionConfig,
        quic::Perspective perspective,
        std::shared_ptr<quic::QuartcPacketTransport> transport,
        quic::QuicClock* clock,
        std::shared_ptr<quic::QuicAlarmFactory> alarmFactory,
        std::shared_ptr<quic::QuicConnectionHelperInterface> helper,
        std::shared_ptr<quic::QuicCryptoServerConfig> cryptoServerConfig,
        quic::QuicCompressedCertsCache* const compressedCertsCache);
    virtual std::vector<rtc::scoped_refptr<rtc::RTCCertificate>> getCertificates() const;
    virtual void start(std::unique_ptr<RTCQuicParameters> remoteParameters);
    //virtual void listen(const std::string& remoteKey);
    virtual RTCQuicParameters getLocalParameters() const;

    void SetDelegate(Delegate* delegate){ m_delegate = delegate; };

    explicit P2PQuicTransport(
        std::unique_ptr<quic::QuicConnection> connection,
        const quic::QuicConfig& config,
        quic::QuicClock* clock,
        std::shared_ptr<quic::QuartcPacketWriter> packetWriter,
        std::shared_ptr<quic::QuicCryptoServerConfig> cryptoServerConfig,
        quic::QuicCompressedCertsCache* const compressedCertsCache);
    ~P2PQuicTransport();

protected:
    // Implements quic::QuartcSession::Delegate.
    virtual void OnCryptoHandshakeComplete() override;
    virtual void OnConnectionWritable() override;
    virtual void OnIncomingStream(quic::QuartcStream* stream) override;
    virtual void OnCongestionControlChange(quic::QuicBandwidth bandwidth_estimate,
        quic::QuicBandwidth pacing_rate,
        quic::QuicTime::Delta latest_rtt) override;
    virtual void OnConnectionClosed(quic::QuicErrorCode error_code,
        const std::string& error_details,
        quic::ConnectionCloseSource source) override;
    virtual void OnMessageReceived(quic::QuicStringPiece message) override;

private:
    static std::unique_ptr<quic::QuicConnection> createQuicConnection(quic::Perspective perspective,
        std::shared_ptr<quic::QuartcPacketWriter> writer,
        std::shared_ptr<quic::QuicAlarmFactory> alarmFactory,
        std::shared_ptr<quic::QuicConnectionHelperInterface> connectionHelper);
    std::shared_ptr<quic::QuartcPacketWriter> m_writer;
    std::shared_ptr<quic::QuicCryptoServerConfig> m_cryptoServerConfig;
    Delegate* m_delegate;
    std::vector<std::unique_ptr<P2PQuicStream>> m_streams;
};

#endif