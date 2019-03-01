// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOFRAMECONSTRUCTORWRAPPER_H
#define AUDIOFRAMECONSTRUCTORWRAPPER_H

#include "MediaDefinitions.h"
#include <AudioFrameConstructor.h>
#include <node.h>
#include <node_object_wrap.h>

/*
 * Wrapper class of woogeen_base::AudioFrameConstructor
 */
class AudioFrameConstructor : public MediaSink {
 public:
  static void Init(v8::Local<v8::Object> exports);
  woogeen_base::AudioFrameConstructor* me;
  woogeen_base::FrameSource* src;

 private:
  AudioFrameConstructor();
  ~AudioFrameConstructor();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void bindTransport(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void unbindTransport(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void addDestination(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeDestination(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
