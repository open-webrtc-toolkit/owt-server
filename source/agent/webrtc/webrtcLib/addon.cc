#include "AudioFrameConstructorWrapper.h"
#include "AudioFramePacketizerWrapper.h"
#include "VideoFrameConstructorWrapper.h"
#include "VideoFramePacketizerWrapper.h"
#include "WebRtcConnection.h"
#include "ThreadPool.h"
#include "IOThreadPool.h"
#include "MediaStream.h"

#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  WebRtcConnection::Init(exports);
  MediaStream::Init(exports);
  ThreadPool::Init(exports);
  IOThreadPool::Init(exports);
  AudioFrameConstructor::Init(exports);
  AudioFramePacketizer::Init(exports);
  VideoFrameConstructor::Init(exports);
  VideoFramePacketizer::Init(exports);
}

NODE_MODULE(addon, InitAll)
