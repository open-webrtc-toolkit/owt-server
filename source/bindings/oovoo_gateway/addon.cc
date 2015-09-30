#include <node.h>
#include "../erizoAPI/WebRtcConnection.h"
#include "../woogeen_base/Gateway.h"

using namespace v8;

void InitAll(Handle<Object> exports) {
  WebRtcConnection::Init(exports);
  Gateway::Init(exports);
}

NODE_MODULE(addon, InitAll)
