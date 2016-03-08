#include <node.h>
#include "../../agent/access/webrtc/WebRtcConnection.h"
#include "../../agent/addons/woogeen_base/Gateway.h"

using namespace v8;

void InitAll(Local<Object> exports) {
  WebRtcConnection::Init(exports);
  Gateway::Init(exports);
}

NODE_MODULE(addon, InitAll)
