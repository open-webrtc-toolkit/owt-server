// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/raw/quic_raw_server.h"

#include <string.h>

#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "net/socket/udp_server_socket.h"
#include "net/third_party/quic/core/crypto/crypto_handshake.h"
#include "net/third_party/quic/core/crypto/quic_random.h"
#include "net/third_party/quic/core/quic_crypto_stream.h"
#include "net/third_party/quic/core/quic_data_reader.h"
#include "net/third_party/quic/core/quic_packets.h"
#include "net/third_party/quic/core/tls_server_handshaker.h"
#include "net/tools/quic/quic_simple_per_connection_packet_writer.h"
#include "net/tools/quic/quic_simple_server_packet_writer.h"
#include "net/tools/quic/quic_simple_server_session_helper.h"

namespace net {

namespace {

const char kSourceAddressTokenSecret[] = "secret";
const size_t kNumSessionsToCreatePerSocketEvent = 16;

// Allocate some extra space so we can send an error if the client goes over
// the limit.
const int kReadBufferSize = 16 * quic::kMaxPacketSize;

}  // namespace

QuicRawServer::QuicRawServer(
    std::unique_ptr<quic::ProofSource> proof_source,
    const quic::QuicConfig& config,
    const quic::QuicCryptoServerConfig::ConfigOptions& crypto_config_options,
    const quic::ParsedQuicVersionVector& supported_versions)
    : version_manager_(supported_versions),
      helper_(
          new QuicChromiumConnectionHelper(&clock_,
                                           quic::QuicRandom::GetInstance())),
      alarm_factory_(new QuicChromiumAlarmFactory(
          base::ThreadTaskRunnerHandle::Get().get(),
          &clock_)),
      config_(config),
      crypto_config_options_(crypto_config_options),
      crypto_config_(kSourceAddressTokenSecret,
                     quic::QuicRandom::GetInstance(),
                     std::move(proof_source),
                     quic::KeyExchangeSource::Default(),
                     quic::TlsServerHandshaker::CreateSslCtx()),
      read_pending_(false),
      synchronous_read_count_(0),
      read_buffer_(base::MakeRefCounted<IOBufferWithSize>(kReadBufferSize)),
      weak_factory_(this) {
  Initialize();
}

void QuicRawServer::Initialize() {
#if MMSG_MORE
  use_recvmmsg_ = true;
#endif

  // If an initial flow control window has not explicitly been set, then use a
  // sensible value for a server: 1 MB for session, 64 KB for each stream.
  const uint32_t kInitialSessionFlowControlWindow = 1 * 1024 * 1024;  // 1 MB
  const uint32_t kInitialStreamFlowControlWindow = 64 * 1024;         // 64 KB
  if (config_.GetInitialStreamFlowControlWindowToSend() ==
      quic::kMinimumFlowControlSendWindow) {
    config_.SetInitialStreamFlowControlWindowToSend(
        kInitialStreamFlowControlWindow);
  }
  if (config_.GetInitialSessionFlowControlWindowToSend() ==
      quic::kMinimumFlowControlSendWindow) {
    config_.SetInitialSessionFlowControlWindowToSend(
        kInitialSessionFlowControlWindow);
  }

  std::unique_ptr<quic::CryptoHandshakeMessage> scfg(
      crypto_config_.AddDefaultConfig(helper_->GetRandomGenerator(),
                                      helper_->GetClock(),
                                      crypto_config_options_));
}

QuicRawServer::~QuicRawServer() = default;

int QuicRawServer::Listen(const IPEndPoint& address) {
  std::unique_ptr<UDPServerSocket> socket(
      new UDPServerSocket(&net_log_, NetLogSource()));

  socket->AllowAddressReuse();

  int rc = socket->Listen(address);
  if (rc < 0) {
    LOG(ERROR) << "Listen() failed: " << ErrorToString(rc);
    return rc;
  }

  // These send and receive buffer sizes are sized for a single connection,
  // because the default usage of QuicRawServer is as a test server with
  // one or two clients.  Adjust higher for use with many clients.
  rc = socket->SetReceiveBufferSize(
      static_cast<int32_t>(quic::kDefaultSocketReceiveBuffer));
  if (rc < 0) {
    LOG(ERROR) << "SetReceiveBufferSize() failed: " << ErrorToString(rc);
    return rc;
  }

  rc = socket->SetSendBufferSize(320 * quic::kMaxPacketSize);
  if (rc < 0) {
    LOG(ERROR) << "SetSendBufferSize() failed: " << ErrorToString(rc);
    return rc;
  }

  rc = socket->GetLocalAddress(&server_address_);
  if (rc < 0) {
    LOG(ERROR) << "GetLocalAddress() failed: " << ErrorToString(rc);
    return rc;
  }

  DVLOG(1) << "Listening on " << server_address_.ToString();

  socket_.swap(socket);

  dispatcher_.reset(new quic::QuicRawDispatcher(
      config_, &crypto_config_, &version_manager_,
      std::unique_ptr<quic::QuicConnectionHelperInterface>(helper_),
      std::unique_ptr<quic::QuicCryptoServerStream::Helper>(
          new QuicSimpleServerSessionHelper(quic::QuicRandom::GetInstance())),
      std::unique_ptr<quic::QuicAlarmFactory>(alarm_factory_)));
  QuicSimpleServerPacketWriter* writer =
      new QuicSimpleServerPacketWriter(socket_.get(), dispatcher_.get());
  dispatcher_->InitializeWithWriter(writer);

  StartReading();

  return OK;
}

void QuicRawServer::Shutdown() {
  // Before we shut down the epoll server, give all active sessions a chance to
  // notify clients that they're closing.
  dispatcher_->Shutdown();

  if (!socket_) {
    return;
  }
  socket_->Close();
  socket_.reset();
}

void QuicRawServer::StartReading() {
  if (synchronous_read_count_ == 0) {
    // Only process buffered packets once per message loop.
    dispatcher_->ProcessBufferedChlos(kNumSessionsToCreatePerSocketEvent);
  }

  if (read_pending_) {
    return;
  }
  read_pending_ = true;

  int result = socket_->RecvFrom(
      read_buffer_.get(), read_buffer_->size(), &client_address_,
      base::Bind(&QuicRawServer::OnReadComplete, base::Unretained(this)));

  if (result == ERR_IO_PENDING) {
    synchronous_read_count_ = 0;
    if (dispatcher_->HasChlosBuffered()) {
      // No more packets to read, so yield before processing buffered packets.
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::Bind(&QuicRawServer::StartReading,
                                weak_factory_.GetWeakPtr()));
    }
    return;
  }

  if (++synchronous_read_count_ > 32) {
    synchronous_read_count_ = 0;
    // Schedule the processing through the message loop to 1) prevent infinite
    // recursion and 2) avoid blocking the thread for too long.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&QuicRawServer::OnReadComplete,
                              weak_factory_.GetWeakPtr(), result));
  } else {
    OnReadComplete(result);
  }
}

void QuicRawServer::OnReadComplete(int result) {
  read_pending_ = false;
  if (result == 0)
    result = ERR_CONNECTION_CLOSED;

  if (result < 0) {
    LOG(ERROR) << "QuicRawServer read failed: " << ErrorToString(result);
    Shutdown();
    return;
  }

  quic::QuicReceivedPacket packet(read_buffer_->data(), result,
                                  helper_->GetClock()->Now(), false);
  dispatcher_->ProcessPacket(
      quic::QuicSocketAddress(quic::QuicSocketAddressImpl(server_address_)),
      quic::QuicSocketAddress(quic::QuicSocketAddressImpl(client_address_)),
      packet);

  StartReading();
}

}  // namespace net
