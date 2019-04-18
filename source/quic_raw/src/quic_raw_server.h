// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A basic server with raw data transport

#ifndef NET_TOOLS_QUIC_RAW_QUIC_RAW_SERVER_H_
#define NET_TOOLS_QUIC_RAW_QUIC_RAW_SERVER_H_

#include <memory>

#include "base/macros.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_endpoint.h"
#include "net/log/net_log.h"
#include "net/quic/quic_chromium_alarm_factory.h"
#include "net/quic/quic_chromium_connection_helper.h"
#include "net/third_party/quic/core/crypto/quic_crypto_server_config.h"
#include "net/third_party/quic/core/quic_config.h"
#include "net/third_party/quic/core/quic_version_manager.h"
#include "net/third_party/quic/platform/impl/quic_chromium_clock.h"
#include "net/tools/quic/raw/quic_raw_dispatcher.h"

namespace net {

class UDPServerSocket;

}  // namespace net
namespace quic {
class QuicDispatcher;
}  // namespace quic
namespace net {

class QuicRawServer {
 public:
  QuicRawServer(
      std::unique_ptr<quic::ProofSource> proof_source,
      const quic::QuicConfig& config,
      const quic::QuicCryptoServerConfig::ConfigOptions& crypto_config_options,
      const quic::ParsedQuicVersionVector& supported_versions);

  virtual ~QuicRawServer();

  // Start listening on the specified address. Returns an error code.
  int Listen(const IPEndPoint& address);

  // Server deletion is imminent. Start cleaning up.
  void Shutdown();

  // Start reading on the socket. On asynchronous reads, this registers
  // OnReadComplete as the callback, which will then call StartReading again.
  void StartReading();

  // Called on reads that complete asynchronously. Dispatches the packet and
  // continues the read loop.
  void OnReadComplete(int result);

  quic::QuicRawDispatcher* dispatcher() { return dispatcher_.get(); }

  IPEndPoint server_address() const { return server_address_; }

 private:

  // Initialize the internal state of the server.
  void Initialize();

  quic::QuicVersionManager version_manager_;

  // Accepts data from the framer and demuxes clients to sessions.
  std::unique_ptr<quic::QuicRawDispatcher> dispatcher_;

  // Used by the helper_ to time alarms.
  quic::QuicChromiumClock clock_;

  // Used to manage the message loop. Owned by dispatcher_.
  QuicChromiumConnectionHelper* helper_;

  // Used to manage the message loop. Owned by dispatcher_.
  QuicChromiumAlarmFactory* alarm_factory_;

  // Listening socket. Also used for outbound client communication.
  std::unique_ptr<UDPServerSocket> socket_;

  // config_ contains non-crypto parameters that are negotiated in the crypto
  // handshake.
  quic::QuicConfig config_;
  // crypto_config_ contains crypto parameters that are negotiated in the crypto
  // handshake.
  quic::QuicCryptoServerConfig::ConfigOptions crypto_config_options_;
  // crypto_config_ contains crypto parameters for the handshake.
  quic::QuicCryptoServerConfig crypto_config_;

  // The address that the server listens on.
  IPEndPoint server_address_;

  // Keeps track of whether a read is currently in flight, after which
  // OnReadComplete will be called.
  bool read_pending_;

  // The number of iterations of the read loop that have completed synchronously
  // and without posting a new task to the message loop.
  int synchronous_read_count_;

  // The target buffer of the current read.
  scoped_refptr<IOBufferWithSize> read_buffer_;

  // The source address of the current read.
  IPEndPoint client_address_;

  // The log to use for the socket.
  NetLog net_log_;


  base::WeakPtrFactory<QuicRawServer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(QuicRawServer);
};

}  // namespace net

#endif  // NET_TOOLS_QUIC_RAW_QUIC_RAW_SERVER_H_
