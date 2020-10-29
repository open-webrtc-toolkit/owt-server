// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoGstAnalyzerWRAPPER_H
#define VideoGstAnalyzerWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "VideoGstAnalyzer.h"
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>

class VideoGstAnalyzer: public node::ObjectWrap{
  public:
  static void Init(v8::Handle<v8::Object>, v8::Handle<v8::Object>);
  mcu::VideoGstAnalyzer* me;

 private:
  VideoGstAnalyzer();
  ~VideoGstAnalyzer();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void getListeningPort(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void createPipeline(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void clearPipeline(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void emitListenTo(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void setPlaying(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void setOutputParam(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addElementMany(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void stopLoop(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void disconnect(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addOutput(const v8::FunctionCallbackInfo<v8::Value>& args);
};


#endif
