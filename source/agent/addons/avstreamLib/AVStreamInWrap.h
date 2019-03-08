// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AVStreamInWrap_h
#define AVStreamInWrap_h

#include "../../addons/common/NodeEventRegistry.h"
#include <MediaFramePipeline.h>

/*
 * Wrapper class of owt_base::FrameSource
 */
class AVStreamInWrap : public NodeEventedObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object>);
  static void Init(v8::Handle<v8::Object>, v8::Handle<v8::Object>);
  owt_base::FrameSource* me;

 private:
  AVStreamInWrap();
  ~AVStreamInWrap();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addDestination(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeDestination(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif // AVStreamInWrap_h
