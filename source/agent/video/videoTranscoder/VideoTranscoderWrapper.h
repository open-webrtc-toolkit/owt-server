// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoTranscoderWRAPPER_H
#define VideoTranscoderWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "VideoTranscoder.h"
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>

/*
 * Wrapper class of mcu::VideoTranscoder
 */
class VideoTranscoder : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object>, v8::Handle<v8::Object>);
  mcu::VideoTranscoder* me;

 private:
  VideoTranscoder();
  ~VideoTranscoder();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void setInput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void unsetInput(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void addOutput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeOutput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void forceKeyFrame(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void drawText(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void clearText(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
