// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/raw/quic_raw_dispatcher.h"


namespace quic {

QuicRawDispatcher::QuicRawDispatcher(
    const QuicConfig& config,
    const QuicCryptoServerConfig* crypto_config,
    QuicVersionManager* version_manager,
    std::unique_ptr<QuicConnectionHelperInterface> helper,
    std::unique_ptr<QuicCryptoServerStream::Helper> session_helper,
    std::unique_ptr<QuicAlarmFactory> alarm_factory)
    : QuicDispatcher(config,
                     crypto_config,
                     version_manager,
                     std::move(helper),
                     std::move(session_helper),
                     std::move(alarm_factory)),
      visitor_(nullptr){}

QuicRawDispatcher::~QuicRawDispatcher() = default;

int QuicRawDispatcher::GetRstErrorCount(
    QuicRstStreamErrorCode error_code) const {
  auto it = rst_error_map_.find(error_code);
  if (it == rst_error_map_.end()) {
    return 0;
  } else {
    return it->second;
  }
}

void QuicRawDispatcher::OnRstStreamReceived(
    const QuicRstStreamFrame& frame) {
  auto it = rst_error_map_.find(frame.error_code);
  if (it == rst_error_map_.end()) {
    rst_error_map_.insert(std::make_pair(frame.error_code, 1));
  } else {
    it->second++;
  }
}

void QuicRawDispatcher::OnConnectionClosed(QuicConnectionId connection_id,
                        QuicErrorCode error,
                        const std::string& error_details) {
  if (sessions_.count(connection_id) > 0) {
    if (visitor_) {
      visitor_->OnSessionClosed(sessions_[connection_id]);
    }
    sessions_.erase(connection_id);
  }
  QuicDispatcher::OnConnectionClosed(connection_id, error, error_details);
}

QuicRawServerSession* QuicRawDispatcher::CreateQuicSession(
    QuicConnectionId connection_id,
    const QuicSocketAddress& client_address,
    QuicStringPiece /*alpn*/,
    const ParsedQuicVersion& version) {
  // The QuicServerSessionBase takes ownership of |connection| below.
  QuicConnection* connection = new QuicConnection(
      connection_id, client_address, helper(), alarm_factory(), writer(),
      /* owns_writer= */ false, Perspective::IS_SERVER,
      ParsedQuicVersionVector{version});

  QuicRawServerSession* session = new QuicRawServerSession(
      connection, this, config(), GetSupportedVersions(), session_helper(),
      crypto_config(), compressed_certs_cache());
  session->Initialize();
  sessions_[connection_id] = session;
  if (visitor_) {
    visitor_->OnSessionCreated(session);
  }
  return session;
}

}  // namespace quic
