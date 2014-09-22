#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif
#include <node.h>
#include "../erizoAPI/ExternalInput.h"
#include "../erizoAPI/ExternalOutput.h"
#include "../erizoAPI/WebRtcConnection.h"
#include "ManyToManyTranscoder.h"

using namespace v8;

void InitAll(Handle<Object> target) {
  WebRtcConnection::Init(target);
  ManyToManyTranscoder::Init(target);
  ExternalInput::Init(target);
  ExternalOutput::Init(target);
}

NODE_MODULE(addon, InitAll)
