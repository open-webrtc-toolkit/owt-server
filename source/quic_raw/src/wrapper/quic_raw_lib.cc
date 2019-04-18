
#include "net/tools/quic/raw/wrapper/quic_raw_lib.h"

#include <iostream>
#include <thread>
#include <string>

#include "base/at_exit.h"
#include "base/run_loop.h"
#include "base/message_loop/message_loop.h"
#include "base/task/post_task.h"
#include "base/task/task_scheduler/task_scheduler.h"
#include "net/base/net_errors.h"
#include "net/base/privacy_mode.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/http/transport_security_state.h"
#include "net/quic/crypto/proof_verifier_chromium.h"
#include "net/third_party/quic/core/quic_error_codes.h"
#include "net/third_party/quic/core/quic_packets.h"
#include "net/third_party/quic/core/quic_server_id.h"
#include "net/third_party/quic/platform/api/quic_socket_address.h"
#include "net/third_party/quic/platform/api/quic_str_cat.h"
#include "net/third_party/quic/platform/api/quic_string_piece.h"
#include "net/third_party/quic/platform/api/quic_text_utils.h"
#include "net/tools/quic/synchronous_host_resolver.h"

#include "base/strings/string_number_conversions.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/crypto/proof_source_chromium.h"

#include "url/gurl.h"

#include "net/tools/quic/raw/quic_raw_stream.h"
#include "net/tools/quic/raw/quic_raw_client.h"
#include "net/tools/quic/raw/quic_raw_server.h"
#include "net/tools/quic/raw/quic_raw_dispatcher.h"
#include "net/tools/quic/raw/quic_raw_server_session.h"

namespace net {

using net::CertVerifier;
using net::CTVerifier;
using net::MultiLogCTVerifier;
using quic::ProofVerifier;
using net::ProofVerifierChromium;
using quic::QuicStringPiece;
using net::TransportSecurityState;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

// FakeProofVerifier for client
class FakeProofVerifier : public quic::ProofVerifier {
 public:
  quic::QuicAsyncStatus VerifyProof(
      const string& hostname,
      const uint16_t port,
      const string& server_config,
      quic::QuicTransportVersion quic_version,
      quic::QuicStringPiece chlo_hash,
      const std::vector<string>& certs,
      const string& cert_sct,
      const string& signature,
      const quic::ProofVerifyContext* context,
      string* error_details,
      std::unique_ptr<quic::ProofVerifyDetails>* details,
      std::unique_ptr<quic::ProofVerifierCallback> callback) override {
    return quic::QUIC_SUCCESS;
  }

  quic::QuicAsyncStatus VerifyCertChain(
      const std::string& hostname,
      const std::vector<std::string>& certs,
      const quic::ProofVerifyContext* verify_context,
      std::string* error_details,
      std::unique_ptr<quic::ProofVerifyDetails>* verify_details,
      std::unique_ptr<quic::ProofVerifierCallback> callback) override {
    return quic::QUIC_SUCCESS;
  }

  std::unique_ptr<quic::ProofVerifyContext> CreateDefaultContext() override {
    return nullptr;
  }
};

// RawClient Implementation
class RawClientImpl : public RQuicClientInterface,
                      public quic::QuicRawStream::Visitor {
 public:
  RawClientImpl()
      : stream_{nullptr},
        session_{nullptr},
        mtx_{},
        message_loop_{nullptr},
        run_loop_{nullptr},
        client_thread_{nullptr},
        listener_{nullptr} {}

  ~RawClientImpl() override {
    stop();
    waitForClose();
  }

  // Implement RQuicClientInterface
  bool start(const char* host, int port) override {
    if (!client_thread_) {
      std::string s_host(host);
      client_thread_.reset(
          new std::thread(&RawClientImpl::InitAndRun, this, s_host, port));
      return true;
    }
    return false;
  }

  // Implement RQuicClientInterface
  void stop() override {
    {
      std::unique_lock<std::mutex> lck(mtx_);
      if (message_loop_) {
        message_loop_->task_runner()->PostTask(FROM_HERE, run_loop_->QuitClosure());
      }
    }
  }

  // Implement RQuicClientInterface
  void waitForClose() override {
    if (client_thread_) {
      client_thread_->join();
      client_thread_.reset();
    }
  }

  // Implement RQuicClientInterface
  void send(const char* data, uint32_t len) override {
    if (stream_) {
      send(stream_->id(), data, len);
    }
  }

  // Implement RQuicClientInterface
  void send(uint32_t stream_id, const char* data, uint32_t len) override {
    std::unique_lock<std::mutex> lck(mtx_);
    if (message_loop_) {
      std::string s_data(data, len);
      message_loop_->task_runner()->PostTask(FROM_HERE,
          base::BindOnce(&RawClientImpl::SendOnStream,
              base::Unretained(this), stream_id, s_data, false));
    }
  }

  // Implement RQuicClientInterface
  void setListener(RQuicListener* listener) override {
    listener_ = listener;
  }

  // Implement quic::QuicRawStream::Visitor
  void OnClose(quic::QuicRawStream* stream) override {
    if (stream == stream_) {
      stream_ = nullptr;
    }
  };
  void OnData(quic::QuicRawStream* stream, char* data, size_t len) override {
    if (listener_) {
      // Use 0 for client session
      listener_->onData(0, stream->id(), data, len);
    }
  }
 private:
  void InitAndRun(std::string host, int port) {
    base::MessageLoopForIO message_loop;
    base::RunLoop run_loop;

    // Determine IP address to connect to from supplied hostname.
    quic::QuicIpAddress ip_addr;

    GURL url("https://www.example.org");
    
    if (!ip_addr.FromString(host)) {
      net::AddressList addresses;
      int rv = net::SynchronousHostResolver::Resolve(host, &addresses);
      if (rv != net::OK) {
        LOG(ERROR) << "Unable to resolve '" << host
                   << "' : " << net::ErrorToShortString(rv);
        return;
      }
      ip_addr =
          quic::QuicIpAddress(quic::QuicIpAddressImpl(addresses[0].address()));
    }

    quic::QuicServerId server_id(url.host(), url.EffectiveIntPort(),
                                 net::PRIVACY_MODE_DISABLED);
    quic::ParsedQuicVersionVector versions = quic::CurrentSupportedVersions();

    // For secure QUIC we need to verify the cert chain.
    std::unique_ptr<CertVerifier> cert_verifier(CertVerifier::CreateDefault());
    std::unique_ptr<TransportSecurityState> transport_security_state(
        new TransportSecurityState);
    std::unique_ptr<MultiLogCTVerifier> ct_verifier(new MultiLogCTVerifier());
    std::unique_ptr<net::CTPolicyEnforcer> ct_policy_enforcer(
        new net::DefaultCTPolicyEnforcer());
    std::unique_ptr<quic::ProofVerifier> proof_verifier;

    bool disable_cert = true;
    if (disable_cert) {
      proof_verifier.reset(new FakeProofVerifier());
    } else {
      proof_verifier.reset(new ProofVerifierChromium(
          cert_verifier.get(), ct_policy_enforcer.get(),
          transport_security_state.get(), ct_verifier.get()));
    }

    net::QuicRawClient client(quic::QuicSocketAddress(ip_addr, port),
                                server_id, versions, std::move(proof_verifier));

    client.set_initial_max_packet_length(quic::kDefaultMaxPacketSize);
    if (!client.Initialize()) {
      std::cerr << "Failed to initialize client." << std::endl;
      return;
    }
    if (!client.Connect()) {
      std::cerr << "Failed to connect." << std::endl;
      return;
    }

    session_ = client.client_session();
    stream_ = session_->CreateOutgoingBidirectionalStream();
    stream_->set_visitor(this);

    {
      std::unique_lock<std::mutex> lck(mtx_);
      message_loop_ = &message_loop;
      run_loop_ = &run_loop;
    }
    if (listener_) {
      listener_->onReady();
    }
    // cout << "get port:" << client.SocketPort() << endl;
    run_loop_->Run();
    {
      std::unique_lock<std::mutex> lck(mtx_);
      message_loop_ = nullptr;
      run_loop_ = nullptr;
    }
  }

  void SendOnStream(uint32_t stream_id, const std::string& data, bool fin) {
    quic::QuicStream* stream = session_->GetOrCreateStream(stream_id);
    if (stream) {
      stream->WriteOrBufferData(data, fin, nullptr);
    } else {
      cerr << "Failed to get stream: " << stream_id << endl;
    }
  }
  quic::QuicRawStream* stream_;
  quic::QuicRawClientSession* session_;
  std::mutex mtx_;
  base::MessageLoopForIO* message_loop_;
  base::RunLoop* run_loop_;
  std::unique_ptr<std::thread> client_thread_;
  RQuicListener* listener_;
};


// RawServer Implementation
class RawServerImpl : public RQuicServerInterface,
                      public quic::QuicRawDispatcher::Visitor,
                      public quic::QuicRawServerSession::Visitor,
                      public quic::QuicRawStream::Visitor {
 public:
  RawServerImpl(std::string cert_file, std::string key_file)
      : cert_file_{cert_file},
        key_file_{key_file},
        mtx_{},
        message_loop_{nullptr},
        run_loop_{nullptr},
        server_thread_{nullptr},
        listener_{nullptr},
        server_port_{0},
        stream_{nullptr},
        next_session_id_{0} {}

  ~RawServerImpl() override {
    stop();
    waitForClose();
  }

  // Implement RQuicServerInterface
  bool listen(int port) override {
    if (!server_thread_) {
      server_thread_.reset(
          new std::thread(&RawServerImpl::InitAndRun, this, port));
      return true;
    }
    return false;
  }

  // Implement RQuicServerInterface
  void stop() override {
    {
      std::unique_lock<std::mutex> lck(mtx_);
      if (message_loop_) {
        message_loop_->task_runner()->PostTask(FROM_HERE, run_loop_->QuitClosure());
      }
    }
  }

  // Implement RQuicServerInterface
  void waitForClose() override {
    if (server_thread_) {
      server_thread_->join();
      server_thread_.reset();
    }
  }

  // Implement RQuicServerInterface
  int getServerPort() override {
    return server_port_;
  }

  // Implement RQuicServerInterface
  void send(const char* data, uint32_t len) override {
    // Send on first session, stream
    if (stream_) {
      send(0, stream_->id(), data, len);
    }
  }

  // Implement RQuicServerInterface
  void send(uint32_t session_id,
            uint32_t stream_id,
            const char* data, uint32_t len) override {
    std::unique_lock<std::mutex> lck(mtx_);
    if (message_loop_) {
      std::string s_data(data);
      message_loop_->task_runner()->PostTask(FROM_HERE,
          base::BindOnce(&RawServerImpl::SendOnSession,
              base::Unretained(this), session_id, stream_id, s_data, false));
    }
  }

  // Implement RQuicServerInterface
  void setListener(RQuicListener* listener) override {
    listener_ = listener;
  }

  // Implement quic::QuicRawDispatcher::Visitor
  void OnSessionCreated(quic::QuicRawServerSession* session) override {
    session_ids_[session] = next_session_id_;
    session_ptrs_[next_session_id_] = session;
    next_session_id_++;
    session->set_visitor(this);
  }
  void OnSessionClosed(quic::QuicRawServerSession* session) override {
    if (session_ids_.count(session) > 0) {
      uint32_t session_id = session_ids_[session];
      session_ids_.erase(session);
      session_ptrs_.erase(session_id);
    }
  }

  // Implement quic::QuicRawServerSession::Visitor
  void OnIncomingStream(quic::QuicRawServerSession* session,
                        quic::QuicRawStream* stream) override {
    if (session_ids_.count(session) > 0) {
      uint32_t session_id = session_ids_[session];
      stream_sessions_[stream] = session_id;
    } else {
      cerr << "No mapping sessions for incoming stream" << endl;
    }
    stream->set_visitor(this);
    if (!stream_) {
      stream_ = stream;
    }
  }

  // Implement quic::QuicRawStream::Visitor
  void OnClose(quic::QuicRawStream* stream) override {
    if (stream_sessions_.count(stream)) {
      stream_sessions_.erase(stream);
    } else {
      cerr << "No mapping session for closing stream" << endl;
    }
    if (stream == stream_) {
      stream_ = nullptr;
    }
  }
  void OnData(quic::QuicRawStream* stream, char* data, size_t len) override {
    if (listener_) {
      uint32_t session_id = stream_sessions_[stream];
      listener_->onData(session_id, stream->id(), data, len);
    }
  }

 private:
  std::unique_ptr<quic::ProofSource> CreateProofSource(
    const base::FilePath& cert_path,
    const base::FilePath& key_path) {
    std::unique_ptr<net::ProofSourceChromium> proof_source(
        new net::ProofSourceChromium());
    CHECK(proof_source->Initialize(cert_path, key_path, base::FilePath()));
    return std::move(proof_source);
  }

  void InitAndRun(int port) {
    base::MessageLoopForIO message_loop;
    base::RunLoop run_loop;

    // Determine IP address to connect to from supplied hostname.
    net::IPAddress ip = net::IPAddress::IPv6AllZeros();

    quic::QuicConfig config;
    net::QuicRawServer server(
        CreateProofSource(base::FilePath(cert_file_), base::FilePath(key_file_)),
        config, quic::QuicCryptoServerConfig::ConfigOptions(),
        quic::AllSupportedVersions());

    int rc = server.Listen(net::IPEndPoint(ip, port));
    if (rc < 0) {
      return;
    }

    server.dispatcher()->set_visitor(this);
    server_port_ = server.server_address().port();

    {
      std::unique_lock<std::mutex> lck(mtx_);
      message_loop_ = &message_loop;
      run_loop_ = &run_loop;
    }
    if (listener_) {
      listener_->onReady();
    }
    run_loop_->Run();
    {
      std::unique_lock<std::mutex> lck(mtx_);
      message_loop_ = nullptr;
      run_loop_ = nullptr;
    }
  }

  void SendOnSession(uint32_t session_id, uint32_t stream_id,
                     const std::string& data, bool fin) {
    if (session_ptrs_.count(session_id) > 0) {
      quic::QuicStream* stream =
          session_ptrs_[session_id]->GetOrCreateStream(stream_id);
      if (stream) {
        stream->WriteOrBufferData(data, fin, nullptr);
      } else {
        cerr << "Failed to get stream: " << stream_id << endl;
      }
    }
  }

  std::string cert_file_;
  std::string key_file_;
  std::mutex mtx_;
  base::MessageLoopForIO* message_loop_;
  base::RunLoop* run_loop_;
  std::unique_ptr<std::thread> server_thread_;
  RQuicListener* listener_;
  int server_port_;
  quic::QuicRawStream* stream_;
  uint32_t next_session_id_;
  std::unordered_map<quic::QuicRawServerSession*, uint32_t> session_ids_;
  std::unordered_map<uint32_t, quic::QuicRawServerSession*> session_ptrs_;
  std::unordered_map<quic::QuicRawStream*, uint32_t> stream_sessions_;
};

bool raw_factory_intialized = false;
std::shared_ptr<base::AtExitManager> exit_manager;

void initialize() {
  if (!raw_factory_intialized) {
    base::TaskScheduler::CreateAndStartWithDefaultParams("raw_quic_factory");
    exit_manager.reset(new base::AtExitManager());
    raw_factory_intialized = true;
  }
}

RQuicClientInterface* RQuicFactory::createQuicClient() {
  initialize();
  RQuicClientInterface* client = new RawClientImpl();
  return client;
}

RQuicServerInterface* RQuicFactory::createQuicServer(const char* cert_file, const char* key_file) {
  initialize();
  RQuicServerInterface* server = new RawServerImpl(cert_file, key_file);
  return server;
}

} //namespace net