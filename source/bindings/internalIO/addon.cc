#include "InternalInWrapper.h"
#include "InternalOutWrapper.h"

#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  InternalIn::Init(exports);
  InternalOut::Init(exports);
}

NODE_MODULE(addon, InitAll)
