// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VIDEOFRAMEPACKETIZERWRAPPER_H
#define VIDEOFRAMEPACKETIZERWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <VideoFramePacketizer.h>
#include <node.h>
#include <node_object_wrap.h>

/*
 * Wrapper class of owt_base::VideoFramePacketizer
 */
class VideoFramePacketizer : public FrameDestination {
 public:
  static void Init(v8::Local<v8::Object> exports);
  owt_base::VideoFramePacketizer* me;

 private:
  VideoFramePacketizer();
  ~VideoFramePacketizer();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void bindTransport(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void unbindTransport(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void enable(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void getSsrc(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
