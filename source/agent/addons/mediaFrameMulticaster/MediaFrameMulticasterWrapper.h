// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FRAMEMULTICASTERERWRAPPER_H
#define FRAMEMULTICASTERERWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <MediaFrameMulticaster.h>
#include <node.h>
#include <node_object_wrap.h>

#include <nan.h>

/*
 * Wrapper class of owt_base::MediaFrameMulticaster
 */
class MediaFrameMulticaster : public FrameDestination {
 public:
  static void Init(v8::Handle<v8::Object>, v8::Handle<v8::Object>);
  owt_base::MediaFrameMulticaster* me;

 private:
  MediaFrameMulticaster();
  ~MediaFrameMulticaster();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void addDestination(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeDestination(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void source(const v8::FunctionCallbackInfo<v8::Value>& args);
};

class MulticasterSource : public FrameSource {
 public:
  static void Init(v8::Handle<v8::Object>, v8::Handle<v8::Object>);
  owt_base::MediaFrameMulticaster* me;

 private:
  MulticasterSource();
  ~MulticasterSource();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  friend class MediaFrameMulticaster;
};

#endif
