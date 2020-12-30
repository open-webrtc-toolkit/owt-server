// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef ResourceUtilWRAPPER_H
#define ResourceUtilWRAPPER_H

#include "ResourceUtil.h"
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>

class ResourceUtil: public node::ObjectWrap{
  public:
  static void Init(v8::Handle<v8::Object>, v8::Handle<v8::Object>);
  mcu::ResourceUtil* me;

 private:
  ResourceUtil();
  ~ResourceUtil();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void getVPUUtil(const v8::FunctionCallbackInfo<v8::Value>& args);
};


#endif
