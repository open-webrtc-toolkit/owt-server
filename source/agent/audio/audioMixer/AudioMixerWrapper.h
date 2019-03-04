// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOMIXERWRAPPER_H
#define AUDIOMIXERWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "../../addons/common/NodeEventRegistry.h"
#include "AudioMixer.h"

/*
 * Wrapper class of mcu::AudioMixer
 */
class AudioMixer : public node::ObjectWrap, public NodeEventRegistry {
 public:
  static void Init(v8::Handle<v8::Object>, v8::Handle<v8::Object>);
  mcu::AudioMixer* me;

 private:
  AudioMixer();
  ~AudioMixer();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void enableVAD(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void disableVAD(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void resetVAD(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addInput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeInput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void setInputActive(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void addOutput(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeOutput(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
