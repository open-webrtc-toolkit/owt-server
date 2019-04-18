// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/raw/quic_raw_client.h"

#include <utility>

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_with_source.h"
#include "net/quic/quic_chromium_alarm_factory.h"
#include "net/quic/quic_chromium_connection_helper.h"
#include "net/quic/quic_chromium_packet_reader.h"
#include "net/quic/quic_chromium_packet_writer.h"
#include "net/socket/udp_client_socket.h"
#include "net/third_party/quic/core/crypto/quic_random.h"
#include "net/third_party/quic/core/quic_connection.h"
#include "net/third_party/quic/core/quic_packets.h"
#include "net/third_party/quic/core/quic_server_id.h"
#include "net/third_party/quic/platform/api/quic_flags.h"
#include "net/third_party/quic/platform/api/quic_ptr_util.h"

using std::string;

namespace net {

QuicRawClient::QuicRawClient(
    quic::QuicSocketAddress server_address,
    const quic::QuicServerId& server_id,
    const quic::ParsedQuicVersionVector& supported_versions,
    std::unique_ptr<quic::ProofVerifier> proof_verifier)
    : quic::QuicRawClientBase(
          server_id,
          supported_versions,
          quic::QuicConfig(),
          CreateQuicConnectionHelper(),
          CreateQuicAlarmFactory(),
          quic::QuicWrapUnique(CreateNetworkHelper()),
          std::move(proof_verifier)),
      weak_factory_(this) {
  set_server_address(server_address);
}

QuicRawClient::~QuicRawClient() {
  if (connected()) {
    session()->connection()->CloseConnection(
        quic::QUIC_PEER_GOING_AWAY, "Shutting down",
        quic::ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
  }
}

QuicChromiumConnectionHelper* QuicRawClient::CreateQuicConnectionHelper() {
  return new QuicChromiumConnectionHelper(&clock_,
                                          quic::QuicRandom::GetInstance());
}

QuicClientMessageLooplNetworkHelper* QuicRawClient::CreateNetworkHelper() {
  created_helper_ = new QuicClientMessageLooplNetworkHelper(&clock_, this);
  return created_helper_;
}

int QuicRawClient::SocketPort() {
  return created_helper_->GetLatestClientAddress().port();
}

QuicChromiumAlarmFactory* QuicRawClient::CreateQuicAlarmFactory() {
  return new QuicChromiumAlarmFactory(base::ThreadTaskRunnerHandle::Get().get(),
                                      &clock_);
}

}  // namespace net
