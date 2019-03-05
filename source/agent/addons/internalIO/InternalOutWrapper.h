// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef INTERNALOUTWRAPPER_H
#define INTERNALOUTWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <InternalOut.h>
#include <node.h>
#include <node_object_wrap.h>

/*
 * Wrapper class of owt_base::InternalOut
 */
class InternalOut : public FrameDestination {
 public:
  static void Init(v8::Local<v8::Object> exports);
  owt_base::InternalOut* me;

 private:
  InternalOut();
  ~InternalOut();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
