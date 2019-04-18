// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/raw/quic_raw_client_base.h"

#include "net/third_party/quic/core/crypto/quic_random.h"
#include "net/third_party/quic/core/quic_server_id.h"
#include "net/third_party/quic/platform/api/quic_flags.h"
#include "net/third_party/quic/platform/api/quic_logging.h"
#include "net/third_party/quic/platform/api/quic_ptr_util.h"
#include "net/third_party/quic/platform/api/quic_text_utils.h"

using base::StringToInt;

namespace quic {

QuicRawClientBase::QuicRawClientBase(
    const QuicServerId& server_id,
    const ParsedQuicVersionVector& supported_versions,
    const QuicConfig& config,
    QuicConnectionHelperInterface* helper,
    QuicAlarmFactory* alarm_factory,
    std::unique_ptr<NetworkHelper> network_helper,
    std::unique_ptr<ProofVerifier> proof_verifier)
    : QuicClientBase(server_id,
                     supported_versions,
                     config,
                     helper,
                     alarm_factory,
                     std::move(network_helper),
                     std::move(proof_verifier)) {}

QuicRawClientBase::~QuicRawClientBase() {
  // If we own something. We need to explicitly kill
  // the session before something goes out of scope.
  ResetSession();
}

QuicRawClientSession* QuicRawClientBase::client_session() {
  return static_cast<QuicRawClientSession*>(QuicClientBase::session());
}

void QuicRawClientBase::InitializeSession() {
  client_session()->Initialize();
  client_session()->CryptoConnect();
}

void QuicRawClientBase::OnClose(QuicRawStream* stream) {
  DCHECK(stream != nullptr);
}

void QuicRawClientBase::OnData(QuicRawStream* stream, char* data, size_t len) {

}

std::unique_ptr<QuicSession> QuicRawClientBase::CreateQuicClientSession(
    const quic::ParsedQuicVersionVector& supported_versions,
    QuicConnection* connection) {
  return QuicMakeUnique<QuicRawClientSession>(
      connection, nullptr, *config(), supported_versions, server_id(),
      crypto_config());
}


QuicRawStream* QuicRawClientBase::CreateClientStream() {
  if (!connected()) {
    return nullptr;
  }

  auto* stream = static_cast<QuicRawStream*>(
      client_session()->CreateOutgoingBidirectionalStream());
  if (stream) {
    stream->SetPriority(QuicStream::kDefaultPriority);
    stream->set_visitor(this);
  }
  return stream;
}

int QuicRawClientBase::GetNumSentClientHellosFromSession() {
  return client_session()->GetNumSentClientHellos();
}

int QuicRawClientBase::GetNumReceivedServerConfigUpdatesFromSession() {
  return client_session()->GetNumReceivedServerConfigUpdates();
}

}  // namespace quic
