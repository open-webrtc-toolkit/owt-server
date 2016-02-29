#include <node.h>
#include "../webrtc/WebRtcConnection.h"
#include "../woogeen_base/Gateway.h"

using namespace v8;

void InitAll(Local<Object> exports) {
  WebRtcConnection::Init(exports);
  Gateway::Init(exports);
}

NODE_MODULE(addon, InitAll)
