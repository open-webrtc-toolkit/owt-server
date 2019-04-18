// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A base class for the raw client, which connects to a specified port and sends
// QUIC request to that endpoint.

#ifndef NET_TOOLS_QUIC_RAW_QUIC_RAW_CLIENT_BASE_H_
#define NET_TOOLS_QUIC_RAW_QUIC_RAW_CLIENT_BASE_H_

#include <string>

#include "base/macros.h"
#include "net/third_party/quic/core/crypto/crypto_handshake.h"
#include "net/third_party/quic/core/quic_config.h"
#include "net/third_party/quic/platform/api/quic_socket_address.h"
#include "net/third_party/quic/platform/api/quic_string_piece.h"
#include "net/third_party/quic/tools/quic_client_base.h"

#include "net/tools/quic/raw/quic_raw_stream.h"
#include "net/tools/quic/raw/quic_raw_client_session.h"


namespace quic {

class ProofVerifier;
class QuicServerId;

class QuicRawClientBase : public QuicClientBase,
                          public QuicRawStream::Visitor {
 public:
  QuicRawClientBase(const QuicServerId& server_id,
                     const ParsedQuicVersionVector& supported_versions,
                     const QuicConfig& config,
                     QuicConnectionHelperInterface* helper,
                     QuicAlarmFactory* alarm_factory,
                     std::unique_ptr<NetworkHelper> network_helper,
                     std::unique_ptr<ProofVerifier> proof_verifier);
  QuicRawClientBase(const QuicRawClientBase&) = delete;
  QuicRawClientBase& operator=(const QuicRawClientBase&) = delete;

  ~QuicRawClientBase() override;

  // QuicRawStream::Visitor
  void OnClose(QuicRawStream* stream) override;
  void OnData(QuicRawStream* stream, char* data, size_t len) override;

   // QuicClientBase
  void ResendSavedData() override {}
  // If this client supports buffering data, clear it.
  void ClearDataToResend() override {}

  // A raw session has to call CryptoConnect on top of the regular
  // initialization.
  void InitializeSession() override;

  // Returns a newly created QuicRawStream.
  QuicRawStream* CreateClientStream();

  // Returns a the session used for this client downcasted to a
  // QuicRawClientSession.
  QuicRawClientSession* client_session();

 protected:
  int GetNumSentClientHellosFromSession() override;
  int GetNumReceivedServerConfigUpdatesFromSession() override;

  // Takes ownership of |connection|.
  std::unique_ptr<QuicSession> CreateQuicClientSession(
      const quic::ParsedQuicVersionVector& supported_versions,
      QuicConnection* connection) override;

 private:

};

}  // namespace quic

#endif  // NET_TOOLS_QUIC_RAW_QUIC_RAW_CLIENT_BASE_H_
