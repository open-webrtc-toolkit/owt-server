// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoGstAnalyzerWRAP_H
#define VideoGstAnalyzerWRAP_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "../../addons/common/NodeEventRegistry.h"
#include "VideoGstAnalyzer.h"
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>

class VideoGstAnalyzerWrap: public node::ObjectWrap, public NodeEventRegistry {
  public:
  static void Init(v8::Handle<v8::Object>, v8::Handle<v8::Object>);
  mcu::VideoGstAnalyzer* me;

 private:
  VideoGstAnalyzerWrap();
  ~VideoGstAnalyzerWrap();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void getListeningPort(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void createPipeline(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void clearPipeline(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void emitListenTo(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void setPlaying(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void setInputParam(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addElementMany(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void disconnect(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addOutput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addEventListener(const v8::FunctionCallbackInfo<v8::Value>& args);
};


#endif
