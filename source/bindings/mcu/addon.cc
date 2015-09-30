#include "ExternalOutput.h"
#include "Mixer.h"
#include "../erizoAPI/WebRtcConnection.h"
#include "../woogeen_base/ExternalInput.h"
#include "../woogeen_base/Gateway.h"

#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  WebRtcConnection::Init(exports);
  Gateway::Init(exports);
  Mixer::Init(exports);
  ExternalInput::Init(exports);
  ExternalOutput::Init(exports);
}

NODE_MODULE(addon, InitAll)
