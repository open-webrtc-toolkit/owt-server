#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>

#include "quic_raw_lib.h"

using namespace net;

std::string message;

class ClientListener : public RQuicListener {
 public:
  ClientListener(RQuicClientInterface* client) : client_(client) {}
  void onReady() override {
    std::cout << "Client Started." << std::endl;
    if (client_) {
      client_->send(message.c_str(), message.length());
    }
  }
  void onData(uint32_t session_id, uint32_t stream_id,
      char* data, uint32_t len) override {
    std::cout << "Stream: " << stream_id << " received: "
        << std::string(data, len) << std::endl;
    if (client_) {
      client_->stop();
    }
  }
 private:
  RQuicClientInterface* client_;
};

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cout << "Usage: QuicEchoClient [ServerIP] [ServerPort] [message]" << std::endl;
    return 0;
  }
  int port = atoi(argv[2]);
  message = std::string(argv[3]);

  std::shared_ptr<RQuicClientInterface> client(RQuicFactory::createQuicClient());
  ClientListener listener(client.get());
  client->setListener(&listener);

  client->start(argv[1], port);
  client->waitForClose();
  return 0;
}
