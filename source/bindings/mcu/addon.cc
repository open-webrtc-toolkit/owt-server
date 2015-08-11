#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif
#include <node.h>
#include "Mixer.h"
#include "../erizoAPI/WebRtcConnection.h"
#include "../woogeen_base/ExternalInput.h"
#include "../woogeen_base/Gateway.h"

using namespace v8;

void InitAll(Handle<Object> target) {
  WebRtcConnection::Init(target);
  Gateway::Init(target);
  Mixer::Init(target);
  ExternalInput::Init(target);
}

NODE_MODULE(addon, InitAll)
