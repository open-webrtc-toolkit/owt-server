
#include "net/tools/quic/raw/quic_raw_client_session.h"

#include <string>

#include "net/third_party/quic/core/crypto/crypto_protocol.h"
#include "net/third_party/quic/core/quic_server_id.h"
#include "net/third_party/quic/core/quic_utils.h"
#include "net/third_party/quic/platform/api/quic_bug_tracker.h"
#include "net/third_party/quic/platform/api/quic_flag_utils.h"
#include "net/third_party/quic/platform/api/quic_flags.h"
#include "net/third_party/quic/platform/api/quic_logging.h"
#include "net/third_party/quic/platform/api/quic_ptr_util.h"

namespace quic {

QuicRawClientSession::QuicRawClientSession(
    QuicConnection* connection,
    QuicSession::Visitor* visitor,
    const QuicConfig& config,
    const ParsedQuicVersionVector& supported_versions,
    const QuicServerId& server_id,
    QuicCryptoClientConfig* crypto_config)
    : QuicSession(connection, visitor, config, supported_versions),
      server_id_(server_id),
      crypto_config_(crypto_config),
      respect_goaway_(false) {}

QuicRawClientSession::~QuicRawClientSession() = default;

void QuicRawClientSession::Initialize() {
  crypto_stream_ = CreateQuicCryptoStream();
  QuicSession::Initialize();
}

void QuicRawClientSession::OnProofValid(
    const QuicCryptoClientConfig::CachedState& /*cached*/) {}

void QuicRawClientSession::OnProofVerifyDetailsAvailable(
    const ProofVerifyDetails& /*verify_details*/) {}

bool QuicRawClientSession::ShouldCreateOutgoingBidirectionalStream() {
  if (!crypto_stream_->encryption_established()) {
    QUIC_DLOG(INFO) << "Encryption not active so no outgoing stream created.";
    return false;
  }
  // if (!GetQuicReloadableFlag(quic_use_common_stream_check) &&
  //     connection()->transport_version() != QUIC_VERSION_99) {
  //   if (GetNumOpenOutgoingStreams() >=
  //       stream_id_manager().max_open_outgoing_streams()) {
  //     QUIC_DLOG(INFO) << "Failed to create a new outgoing stream. "
  //                     << "Already " << GetNumOpenOutgoingStreams() << " open.";
  //     return false;
  //   }
  //   if (goaway_received() && respect_goaway_) {
  //     QUIC_DLOG(INFO) << "Failed to create a new outgoing stream. "
  //                     << "Already received goaway.";
  //     return false;
  //   }
  //   return true;
  // }
  // if (goaway_received() && respect_goaway_) {
  //   QUIC_DLOG(INFO) << "Failed to create a new outgoing stream. "
  //                   << "Already received goaway.";
  //   return false;
  // }
  // QUIC_RELOADABLE_FLAG_COUNT_N(quic_use_common_stream_check, 1, 2);
  // return CanOpenNextOutgoingBidirectionalStream();
  return true;
}

bool QuicRawClientSession::ShouldCreateOutgoingUnidirectionalStream() {
  QUIC_BUG << "Try to create outgoing unidirectional client data streams";
  return false;
}

QuicRawStream*
QuicRawClientSession::CreateOutgoingBidirectionalStream() {
  if (!ShouldCreateOutgoingBidirectionalStream()) {
    return nullptr;
  }
  std::unique_ptr<QuicRawStream> stream = CreateClientStream();
  QuicRawStream* stream_ptr = stream.get();
  ActivateStream(std::move(stream));
  return stream_ptr;
}

QuicRawStream*
QuicRawClientSession::CreateOutgoingUnidirectionalStream() {
  QUIC_BUG << "Try to create outgoing unidirectional client data streams";
  return nullptr;
}

std::unique_ptr<QuicRawStream>
QuicRawClientSession::CreateClientStream() {
    //GetNextOutgoingBidirectionalStreamId
  return QuicMakeUnique<QuicRawStream>(
      GetNextOutgoingStreamId(), this, BIDIRECTIONAL);
}

QuicCryptoClientStreamBase* QuicRawClientSession::GetMutableCryptoStream() {
  return crypto_stream_.get();
}

const QuicCryptoClientStreamBase* QuicRawClientSession::GetCryptoStream()
    const {
  return crypto_stream_.get();
}

void QuicRawClientSession::CryptoConnect() {
  DCHECK(flow_controller());
  crypto_stream_->CryptoConnect();
}

int QuicRawClientSession::GetNumSentClientHellos() const {
  return crypto_stream_->num_sent_client_hellos();
}

int QuicRawClientSession::GetNumReceivedServerConfigUpdates() const {
  return crypto_stream_->num_scup_messages_received();
}

bool QuicRawClientSession::ShouldCreateIncomingStream(QuicStreamId id) {
  if (!connection()->connected()) {
    QUIC_BUG << "ShouldCreateIncomingStream called when disconnected";
    return false;
  }
  if (goaway_received() && respect_goaway_) {
    QUIC_DLOG(INFO) << "Failed to create a new outgoing stream. "
                    << "Already received goaway.";
    return false;
  }
  // if (QuicUtils::IsClientInitiatedStreamId(connection()->transport_version(),
  //                                          id) ||
  //     (connection()->transport_version() == QUIC_VERSION_99 &&
  //      QuicUtils::IsBidirectionalStreamId(id))) {
  //   QUIC_LOG(WARNING) << "Received invalid push stream id " << id;
  //   connection()->CloseConnection(
  //       QUIC_INVALID_STREAM_ID,
  //       "Server created non write unidirectional stream",
  //       ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
  //   return false;
  // }
  return true;
}

// QuicRawStream* QuicRawClientSession::CreateIncomingStream(
//     PendingStream pending) {
//   QuicRawStream* stream =
//       new QuicRawStream(std::move(pending), this, READ_UNIDIRECTIONAL);
//   ActivateStream(QuicWrapUnique(stream));
//   return stream;
// }

QuicRawStream* QuicRawClientSession::CreateIncomingStream(QuicStreamId id) {
  if (!ShouldCreateIncomingStream(id)) {
    return nullptr;
  }
  QuicRawStream* stream =
      new QuicRawStream(id, this, READ_UNIDIRECTIONAL);
  ActivateStream(QuicWrapUnique(stream));
  return stream;
}

std::unique_ptr<QuicCryptoClientStreamBase>
QuicRawClientSession::CreateQuicCryptoStream() {
  return QuicMakeUnique<QuicCryptoClientStream>(
      server_id_, this,
      crypto_config_->proof_verifier()->CreateDefaultContext(), crypto_config_,
      this);
}

void QuicRawClientSession::OnConfigNegotiated() {
  QuicSession::OnConfigNegotiated();
}

void QuicRawClientSession::OnCryptoHandshakeEvent(
    CryptoHandshakeEvent event) {
  QuicSession::OnCryptoHandshakeEvent(event);
}


}  // namespace quic