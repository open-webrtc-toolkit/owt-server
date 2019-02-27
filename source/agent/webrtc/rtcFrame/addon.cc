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
#include "RTCQuicTransport.h"
#include "RTCCertificate.h"
#include "QuicStream.h"
#endif
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
#ifdef OMS_ENABLE_QUIC
  RTCQuicTransport::Init(exports);
  RTCIceTransport::Init(exports);
  RTCIceCandidate::Init(exports);
  RTCCertificate::Init(exports);
  QuicStream::Init(exports);
#endif
}

NODE_MODULE(addon, InitAll)
