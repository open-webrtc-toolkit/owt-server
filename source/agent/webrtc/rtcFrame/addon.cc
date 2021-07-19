// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioFrameConstructorWrapper.h"
#include "AudioFramePacketizerWrapper.h"
#include "VideoFrameConstructorWrapper.h"
#include "VideoFramePacketizerWrapper.h"
#include "CallBaseWrapper.h"

#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  AudioFrameConstructor::Init(exports);
  AudioFramePacketizer::Init(exports);
  VideoFrameConstructor::Init(exports);
  VideoFramePacketizer::Init(exports);
  CallBase::Init(exports);
}

NODE_MODULE(addon, InitAll)
