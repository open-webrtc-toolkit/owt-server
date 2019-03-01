// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef INTERNALIOWRAPPER_H
#define INTERNALIOWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "logger.h"
#include <InternalSctp.h>
#include <node.h>
#include <node_object_wrap.h>


/*
 * Wrapper class of woogeen_base::InternalSctp
 * In fact, class SctpIn & class SctpOut are same,
 * but we cannot extends to FrameSource and FrameDestination both
 */
class SctpIn : public FrameSource {
  DECLARE_LOGGER();
public:
  static void Init(v8::Local<v8::Object> exports);
  woogeen_base::InternalSctp* me;

private:
  SctpIn();
  virtual ~SctpIn();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void getListeningPort(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void connect(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void addDestination(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeDestination(const v8::FunctionCallbackInfo<v8::Value>& args);

};

class SctpOut : public FrameDestination {
  DECLARE_LOGGER();
public:
  static void Init(v8::Local<v8::Object> exports);
  woogeen_base::InternalSctp* me;

private:
  SctpOut();
  virtual ~SctpOut();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void getListeningPort(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void connect(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
