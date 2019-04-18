// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/raw/quic_raw_server_session.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

#include "net/third_party/quic/core/quic_utils.h"
#include "net/third_party/quic/platform/api/quic_bug_tracker.h"
#include "net/third_party/quic/platform/api/quic_fallthrough.h"
#include "net/third_party/quic/platform/api/quic_flag_utils.h"
#include "net/third_party/quic/platform/api/quic_flags.h"
#include "net/third_party/quic/platform/api/quic_logging.h"
#include "net/third_party/quic/platform/api/quic_ptr_util.h"
#include "net/third_party/quic/platform/api/quic_str_cat.h"
#include "net/third_party/quic/platform/api/quic_text_utils.h"

namespace quic {

QuicRawServerSession::QuicRawServerSession(
    QuicConnection* connection,
    QuicSession::Visitor* visitor,
    const QuicConfig& config,
    const ParsedQuicVersionVector& supported_versions,
    QuicCryptoServerStream::Helper* helper,
    const QuicCryptoServerConfig* crypto_config,
    QuicCompressedCertsCache* compressed_certs_cache)
    : QuicSession(connection, visitor, config, supported_versions),
      crypto_config_(crypto_config),
      compressed_certs_cache_(compressed_certs_cache),
      helper_(helper),
      visitor_(nullptr) {
}

QuicRawServerSession::~QuicRawServerSession() {
  // Set the streams' session pointers in closed and dynamic stream lists
  // to null to avoid subsequent use of this session.
  // for (auto& stream : *closed_streams()) {
  //   static_cast<QuicRawStream*>(stream.get())->ClearSession();
  // }
  // for (auto const& kv : zombie_streams()) {
  //   static_cast<QuicRawStream*>(kv.second.get())->ClearSession();
  // }
  // for (auto const& kv : dynamic_streams()) {
  //   static_cast<QuicRawStream*>(kv.second.get())->ClearSession();
  // }
  delete connection();
}

void QuicRawServerSession::Initialize() {
  crypto_stream_.reset(
      CreateQuicCryptoServerStream(crypto_config_, compressed_certs_cache_));
  QuicSession::Initialize();
}

QuicCryptoServerStreamBase* QuicRawServerSession::GetMutableCryptoStream() {
  return crypto_stream_.get();
}

const QuicCryptoServerStreamBase* QuicRawServerSession::GetCryptoStream()
    const {
  return crypto_stream_.get();
}

void QuicRawServerSession::OnCryptoHandshakeEvent(CryptoHandshakeEvent event) {
  QuicSession::OnCryptoHandshakeEvent(event);
  if (event == HANDSHAKE_CONFIRMED) {
    //SendMaxHeaderListSize(max_inbound_header_list_size_);
  }
}

void QuicRawServerSession::OnConnectionClosed(QuicErrorCode error,
                                               const std::string& error_details,
                                               ConnectionCloseSource source) {
  QuicSession::OnConnectionClosed(error, error_details, source);
  // In the unlikely event we get a connection close while doing an asynchronous
  // crypto event, make sure we cancel the callback.
  if (crypto_stream_ != nullptr) {
    crypto_stream_->CancelOutstandingCallbacks();
  }
}

void QuicRawServerSession::CloseConnectionWithDetails(QuicErrorCode error,
                                                 const std::string& details) {
  connection()->CloseConnection(
      error, details, ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
}


bool QuicRawServerSession::ShouldCreateIncomingStream(QuicStreamId id) {
  if (!connection()->connected()) {
    QUIC_BUG << "ShouldCreateIncomingStream called when disconnected";
    return false;
  }

  if (QuicUtils::IsServerInitiatedStreamId(connection()->transport_version(),
                                           id)) {
    QUIC_DLOG(INFO) << "Invalid incoming even stream_id:" << id;
    connection()->CloseConnection(
        QUIC_INVALID_STREAM_ID, "Client created even numbered stream",
        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
    return false;
  }
  return true;
}

bool QuicRawServerSession::ShouldCreateOutgoingBidirectionalStream() {
  if (!connection()->connected()) {
    QUIC_BUG
        << "ShouldCreateOutgoingBidirectionalStream called when disconnected";
    return false;
  }
  if (!crypto_stream_->encryption_established()) {
    QUIC_BUG << "Encryption not established so no outgoing stream created.";
    return false;
  }

  if (!GetQuicReloadableFlag(quic_use_common_stream_check) &&
      connection()->transport_version() != QUIC_VERSION_99) {
    // if (GetNumOpenOutgoingStreams() >=
    //     stream_id_manager().max_open_outgoing_streams()) {
    //   VLOG(1) << "No more streams should be created. "
    //           << "Already " << GetNumOpenOutgoingStreams() << " open.";
    //   return false;
    // }
  }
  //QUIC_RELOADABLE_FLAG_COUNT_N(quic_use_common_stream_check, 2, 2);
  //return CanOpenNextOutgoingBidirectionalStream();
  return true;
}

bool QuicRawServerSession::ShouldCreateOutgoingUnidirectionalStream() {
  if (!connection()->connected()) {
    QUIC_BUG
        << "ShouldCreateOutgoingUnidirectionalStream called when disconnected";
    return false;
  }
  if (!crypto_stream_->encryption_established()) {
    QUIC_BUG << "Encryption not established so no outgoing stream created.";
    return false;
  }

  // if (!GetQuicReloadableFlag(quic_use_common_stream_check) &&
  //     connection()->transport_version() != QUIC_VERSION_99) {
  //   if (GetNumOpenOutgoingStreams() >=
  //       stream_id_manager().max_open_outgoing_streams()) {
  //     VLOG(1) << "No more streams should be created. "
  //             << "Already " << GetNumOpenOutgoingStreams() << " open.";
  //     return false;
  //   }
  // }

  // return CanOpenNextOutgoingUnidirectionalStream();
  return true;
}

QuicCryptoServerStreamBase*
QuicRawServerSession::CreateQuicCryptoServerStream(
    const QuicCryptoServerConfig* crypto_config,
    QuicCompressedCertsCache* compressed_certs_cache) {
  return new QuicCryptoServerStream(
      crypto_config, compressed_certs_cache,
      GetQuicReloadableFlag(enable_quic_stateless_reject_support), this,
      stream_helper());
}

QuicRawStream* QuicRawServerSession::CreateIncomingStream(QuicStreamId id) {
  if (!ShouldCreateIncomingStream(id)) {
    return nullptr;
  }

  QuicRawStream* stream = new QuicRawStream(
      id, this, BIDIRECTIONAL);
  ActivateStream(QuicWrapUnique(stream));
  if (visitor_) {
    visitor_->OnIncomingStream(this, stream);
  }
  return stream;
}

// QuicRawStream* QuicRawServerSession::CreateIncomingStream(
//     PendingStream pending) {
//   QuicRawStream* stream = new QuicRawStream(
//       std::move(pending), this, BIDIRECTIONAL);
//   ActivateStream(QuicWrapUnique(stream));
//   return stream;
// }

QuicRawStream*
QuicRawServerSession::CreateOutgoingBidirectionalStream() {
  DCHECK(false);
  return nullptr;
}

QuicRawStream*
QuicRawServerSession::CreateOutgoingUnidirectionalStream() {
  if (!ShouldCreateOutgoingUnidirectionalStream()) {
    return nullptr;
  }

  QuicRawStream* stream = new QuicRawStream(
      GetNextOutgoingStreamId(), this, WRITE_UNIDIRECTIONAL);
  ActivateStream(QuicWrapUnique(stream));
  return stream;
}

// True if there are open dynamic streams.
bool QuicRawServerSession::ShouldKeepConnectionAlive() const {
  //return GetNumOpenDynamicStreams() > 0;
  return true;
}

bool QuicRawServerSession::ShouldBufferIncomingStream(QuicStreamId id) const {
  DCHECK_EQ(QUIC_VERSION_99, connection()->transport_version());
  // return !QuicUtils::IsBidirectionalStreamId(id);
  return false;
}

}  // namespace quic