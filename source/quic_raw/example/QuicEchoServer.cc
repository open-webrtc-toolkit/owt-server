#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>

#include "quic_raw_lib.h"

using namespace net;

int port = 6121;
std::string unused_cert = "leaf_cert.pem";
std::string unused_key = "leaf_cert.pkcs8";

class ServerListener : public RQuicListener {
 public:
  ServerListener(RQuicServerInterface* server) : server_(server) {}
  void onReady() override {
    std::cout << "Server Started." << std::endl;
    if (server_) {
      std::cout << "Listening Port: " << server_->getServerPort() << std::endl;
    }
  }
  void onData(uint32_t session_id, uint32_t stream_id,
      char* data, uint32_t len) override {
    std::cout << "Session: " << session_id << " stream: " << stream_id
        << " received: " << std::string(data, len) << std::endl;
    if (server_) {
      server_->send(session_id, stream_id, data, len);
    }
  }
 private:
  RQuicServerInterface* server_;
};

int main(int argc, char* argv[]) {
  if (argc > 1) {
    port = atoi(argv[1]);
  }
  if (argc > 3) {
    unused_cert = std::string(argv[2]);
    unused_key = std::string(argv[3]);
  }

  std::shared_ptr<RQuicServerInterface> server(RQuicFactory::createQuicServer(
      unused_cert.c_str(), unused_key.c_str()));

  ServerListener listener(server.get());
  server->setListener(&listener);

  server->listen(port);
  server->waitForClose();
  return 0;
}
