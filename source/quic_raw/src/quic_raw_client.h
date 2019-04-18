// A client transport raw data on QUIC

#ifndef NET_TOOLS_QUIC_RAW_QUIC_RAW_CLIENT_H_
#define NET_TOOLS_QUIC_RAW_QUIC_RAW_CLIENT_H_

#include <stddef.h>

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/macros.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/log/net_log.h"
#include "net/quic/quic_chromium_packet_reader.h"
#include "net/third_party/quic/core/quic_config.h"
#include "net/third_party/quic/platform/impl/quic_chromium_clock.h"
#include "net/tools/quic/quic_client_message_loop_network_helper.h"

#include "net/tools/quic/raw/quic_raw_client_base.h"
#include "net/tools/quic/raw/quic_raw_stream.h"


namespace net {

class QuicChromiumAlarmFactory;
class QuicChromiumConnectionHelper;

class QuicRawClient : public quic::QuicRawClientBase {
 public:
  // Create a quic client, which will have events managed by the message loop.
  QuicRawClient(quic::QuicSocketAddress server_address,
                   const quic::QuicServerId& server_id,
                   const quic::ParsedQuicVersionVector& supported_versions,
                   std::unique_ptr<quic::ProofVerifier> proof_verifier);

  ~QuicRawClient() override;

  int SocketPort();

 private:

  QuicChromiumAlarmFactory* CreateQuicAlarmFactory();
  QuicChromiumConnectionHelper* CreateQuicConnectionHelper();
  QuicClientMessageLooplNetworkHelper* CreateNetworkHelper();

  //  Used by |helper_| to time alarms.
  quic::QuicChromiumClock clock_;

  QuicClientMessageLooplNetworkHelper* created_helper_;

  base::WeakPtrFactory<QuicRawClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(QuicRawClient);
};

}  // namespace net

#endif  // NET_TOOLS_QUIC_RAW_QUIC_RAW_CLIENT_H_
