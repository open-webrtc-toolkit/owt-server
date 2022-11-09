// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VIDEOMIXERWRAPPER_H
#define VIDEOMIXERWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "VideoMixer.h"
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <nan.h>

/*
 * Wrapper class of mcu::VideoMixer
 */
class VideoMixer : public node::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object>, v8::Local<v8::Object>);
  mcu::VideoMixer* me;

 private:
  VideoMixer();
  ~VideoMixer();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void addInput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeInput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void setInputActive(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addOutput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeOutput(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void updateLayoutSolution(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void forceKeyFrame(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void drawText(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void clearText(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
