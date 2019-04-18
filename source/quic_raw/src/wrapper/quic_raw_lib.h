#ifndef NET_TOOLS_QUIC_RAW_WRAPPER_QUIC_RAW_LIB_H_
#define NET_TOOLS_QUIC_RAW_WRAPPER_QUIC_RAW_LIB_H_

#include <memory>

namespace net {

class RQuicListener {
 public:
  RQuicListener() {}
  virtual ~RQuicListener() {}
  virtual void onReady() = 0;
  // The session_id will not be used for client
  virtual void onData(uint32_t session_id,
                      uint32_t stream_id,
                      char* data, uint32_t len) = 0;
};

class RQuicClientInterface {
 public:
  virtual ~RQuicClientInterface() {}
  virtual bool start(const char* host, int port) = 0;
  virtual void stop() = 0;
  virtual void waitForClose() = 0;
  // Send data on default stream
  virtual void send(const char* data, uint32_t len) = 0;
  // Send data on specified stream
  virtual void send(uint32_t stream_id, const char* data, uint32_t len) = 0;
  virtual void setListener(RQuicListener* listener) = 0;
};

class RQuicServerInterface {
 public:
  virtual ~RQuicServerInterface() {}
  virtual bool listen(int port) = 0;
  virtual void stop() = 0;
  virtual void waitForClose() = 0;
  // Send data on default(first) stream
  virtual void send(const char* data, uint32_t len) = 0;
  // Send data on specified session, stream
  virtual void send(uint32_t session_id,
                    uint32_t stream_id,
                    const char* data, uint32_t len) = 0;
  virtual int getServerPort() = 0;
  virtual void setListener(RQuicListener* listener) = 0;
};

class RQuicFactory {
 public:
  static RQuicClientInterface* createQuicClient();
  static RQuicServerInterface* createQuicServer(
      const char* cert_file, const char* key_file);
};

} // namespace net

#endif