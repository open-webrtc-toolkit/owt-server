#include "MediaFileInWrapper.h"
#include "MediaFileOutWrapper.h"

#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  MediaFileIn::Init(exports);
  MediaFileOut::Init(exports);
}

NODE_MODULE(addon, InitAll)
