// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef INTERNALINWRAPPER_H
#define INTERNALINWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <InternalIn.h>
#include <node.h>
#include <node_object_wrap.h>


/*
 * Wrapper class of owt_base::InternalIn
 */
class InternalIn : public FrameSource {
 public:
  static void Init(v8::Local<v8::Object> exports);
  owt_base::InternalIn* me;

 private:
  InternalIn();
  ~InternalIn();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void getListeningPort(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addDestination(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeDestination(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
