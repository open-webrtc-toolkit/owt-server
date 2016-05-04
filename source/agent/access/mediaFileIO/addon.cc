#include "MediaFileInWrapper.h"
#include "MediaFileOutWrapper.h"

#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  MediaFileIn::Init(exports);
  MediaFileOut::Init(exports);

  av_register_all();
  avcodec_register_all();
  av_log_set_level(AV_LOG_WARNING);
}

NODE_MODULE(addon, InitAll)
