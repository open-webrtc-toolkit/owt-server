// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOFRAMECONSTRUCTORWRAPPER_H
#define AUDIOFRAMECONSTRUCTORWRAPPER_H

#include "MediaWrapper.h"
#include <AudioFrameConstructor.h>
#include <node.h>
#include <node_object_wrap.h>
#include <nan.h>

/*
 * Wrapper class of owt_base::AudioFrameConstructor
 */
class AudioFrameConstructor : public MediaSink {
 public:
  static NAN_MODULE_INIT(Init);
  owt_base::AudioFrameConstructor* me;
  owt_base::FrameSource* src;

 private:
  AudioFrameConstructor();
  ~AudioFrameConstructor();

  static NAN_METHOD(New);

  static NAN_METHOD(close);

  static NAN_METHOD(bindTransport);
  static NAN_METHOD(unbindTransport);

  static NAN_METHOD(enable);

  static NAN_METHOD(addDestination);
  static NAN_METHOD(removeDestination);

  static Nan::Persistent<v8::Function> constructor;
};

#endif
