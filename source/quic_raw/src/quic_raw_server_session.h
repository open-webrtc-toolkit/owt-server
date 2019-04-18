// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_QUIC_RAW_QUIC_RAW_SERVER_SESSION_H_
#define NET_TOOLS_QUIC_RAW_QUIC_RAW_SERVER_SESSION_H_

#include <cstddef>
#include <memory>
#include <string>

#include "net/third_party/quic/core/quic_session.h"
#include "net/third_party/quic/core/quic_versions.h"
#include "net/third_party/quic/platform/api/quic_export.h"
#include "net/third_party/quic/platform/api/quic_string_piece.h"
#include "net/third_party/quic/core/quic_crypto_server_stream.h"

#include "net/tools/quic/raw/quic_raw_stream.h"

namespace quic {

// A QUIC session with raw stream.
class QUIC_EXPORT_PRIVATE QuicRawServerSession
    : public QuicSession {
 public:
  // Visitor receives callbacks from the QuicRawServerSession.
  class QUIC_EXPORT_PRIVATE Visitor {
   public:
    Visitor() {}
    Visitor(const Visitor&) = delete;
    Visitor& operator=(const Visitor&) = delete;

    // Called when new incoming stream created
    virtual void OnIncomingStream(
        QuicRawServerSession* session,
        QuicRawStream* stream) = 0;

   protected:
    virtual ~Visitor() {}
  };

  // Does not take ownership of |connection| or |visitor|.
  QuicRawServerSession(QuicConnection* connection,
                 QuicSession::Visitor* visitor,
                 const QuicConfig& config,
                 const ParsedQuicVersionVector& supported_versions,
                 QuicCryptoServerStream::Helper* helper,
                 const QuicCryptoServerConfig* crypto_config,
                 QuicCompressedCertsCache* compressed_certs_cache);
  QuicRawServerSession(const QuicRawServerSession&) = delete;
  QuicRawServerSession& operator=(const QuicRawServerSession&) = delete;

  ~QuicRawServerSession() override;

  void Initialize() override;

  const QuicCryptoServerStreamBase* crypto_stream() const {
    return crypto_stream_.get();
  }

  void CloseConnectionWithDetails(QuicErrorCode error,
                                  const std::string& details);

  void set_visitor(Visitor* visitor) { visitor_ = visitor; }

 protected:
  // QuicSession methods(override them with return type of QuicSpdyStream*):
  QuicCryptoServerStreamBase* GetMutableCryptoStream() override;

  const QuicCryptoServerStreamBase* GetCryptoStream() const override;

  void OnCryptoHandshakeEvent(CryptoHandshakeEvent event) override;

  void OnConnectionClosed(QuicErrorCode error,
                           const std::string& error_details,
                           ConnectionCloseSource source) override;

  // Override CreateIncomingStream(), CreateOutgoingBidirectionalStream() and
  // CreateOutgoingUnidirectionalStream() with QuicSpdyStream return type to
  // make sure that all data streams are QuicSpdyStreams.
  QuicRawStream* CreateIncomingStream(QuicStreamId id) override;
  // QuicRawStream* CreateIncomingStream(PendingStream pending) override;
  virtual QuicRawStream* CreateOutgoingBidirectionalStream();
  virtual QuicRawStream* CreateOutgoingUnidirectionalStream();

  // If an incoming stream can be created, return true.
  virtual bool ShouldCreateIncomingStream(QuicStreamId id);

  // If an outgoing bidirectional/unidirectional stream can be created, return
  // true.
  virtual bool ShouldCreateOutgoingBidirectionalStream();
  virtual bool ShouldCreateOutgoingUnidirectionalStream();

  // Overridden to buffer incoming streams for version 99.
  bool ShouldBufferIncomingStream(QuicStreamId id) const;


  // Returns true if there are open dynamic streams.
  bool ShouldKeepConnectionAlive() const;

  bool IsConnected() { return connection()->connected(); }

  virtual QuicCryptoServerStreamBase* CreateQuicCryptoServerStream(
      const QuicCryptoServerConfig* crypto_config,
      QuicCompressedCertsCache* compressed_certs_cache);

  const QuicCryptoServerConfig* crypto_config() { return crypto_config_; }

  QuicCryptoServerStream::Helper* stream_helper() { return helper_; }

 private:
 
  // Called when a PRIORITY frame has been received.
  void OnPriority(spdy::SpdyStreamId stream_id, spdy::SpdyPriority priority);

  // Called when the size of the compressed frame payload is available.
  void OnCompressedFrameSize(size_t frame_len);

  const QuicCryptoServerConfig* crypto_config_;

  // The cache which contains most recently compressed certs.
  // Owned by QuicDispatcher.
  QuicCompressedCertsCache* compressed_certs_cache_;

  std::unique_ptr<QuicCryptoServerStreamBase> crypto_stream_;

  // Pointer to the helper used to create crypto server streams. Must outlive
  // streams created via CreateQuicCryptoServerStream.
  QuicCryptoServerStream::Helper* helper_;

  Visitor* visitor_;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_HTTP_QUIC_SPDY_SESSION_H_