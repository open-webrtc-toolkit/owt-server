// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioFrameConstructorWrapper.h"
#include "AudioFramePacketizerWrapper.h"
#include "VideoFrameConstructorWrapper.h"
#include "VideoFramePacketizerWrapper.h"
#include "WebRtcConnection.h"
#include "ThreadPool.h"
#include "IOThreadPool.h"
#include "MediaStream.h"
#if defined OMS_ENABLE_QUIC
#include "RTCIceTransport.h"
#include "RTCIceCandidate.h"
#endif
#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  AudioFrameConstructor::Init(exports);
  AudioFramePacketizer::Init(exports);
  VideoFrameConstructor::Init(exports);
  VideoFramePacketizer::Init(exports);
#if defined OMS_ENABLE_QUIC
  RTCIceTransport::Init(exports);
  RTCIceCandidate::Init(exports);
#endif
}

NODE_MODULE(addon, InitAll)
