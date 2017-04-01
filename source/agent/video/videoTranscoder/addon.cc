#include "VideoTranscoderWrapper.h"

#include <node.h>

using namespace v8;

NODE_MODULE(addon, VideoTranscoder::Init)
