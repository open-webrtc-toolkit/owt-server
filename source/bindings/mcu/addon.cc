#include "../erizoAPI/WebRtcConnection.h"
#include "../woogeen_base/AudioFrameConstructorWrapper.h"
#include "../woogeen_base/AudioFramePacketizerWrapper.h"
#include "../woogeen_base/MediaFrameMulticasterWrapper.h"
#include "../woogeen_base/InternalInWrapper.h"
#include "../woogeen_base/InternalOutWrapper.h"
#include "../woogeen_base/MediaFileInWrapper.h"
#include "../woogeen_base/MediaFileOutWrapper.h"
#include "../woogeen_base/RtspInWrapper.h"
#include "../woogeen_base/RtspOutWrapper.h"
#include "../woogeen_base/VideoFrameConstructorWrapper.h"
#include "../woogeen_base/VideoFramePacketizerWrapper.h"
#include "./AudioMixerWrapper.h"
#include "./VideoMixerWrapper.h"

#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  WebRtcConnection::Init(exports);
  AudioFrameConstructor::Init(exports);
  AudioFramePacketizer::Init(exports);
  MediaFrameMulticaster::Init(exports);
  InternalIn::Init(exports);
  InternalOut::Init(exports);
  MediaFileIn::Init(exports);
  MediaFileOut::Init(exports);
  RtspIn::Init(exports);
  RtspOut::Init(exports);
  VideoFrameConstructor::Init(exports);
  VideoFramePacketizer::Init(exports);
  AudioMixer::Init(exports);
  VideoMixer::Init(exports);
}

NODE_MODULE(addon, InitAll)
