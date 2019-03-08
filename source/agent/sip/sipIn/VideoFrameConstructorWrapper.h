// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VIDEOFRAMECONSTRUCTORWRAPPER_H
#define VIDEOFRAMECONSTRUCTORWRAPPER_H

#include "MediaDefinitions.h"
#include "../../addons/common/NodeEventRegistry.h"
#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <VideoFrameConstructor.h>
#include <node.h>
#include <node_object_wrap.h>

/*
 * Wrapper class of owt_base::VideoFrameConstructor
 */
class VideoFrameConstructor : public MediaSink,
                              public NodeEventRegistry,
                              public owt_base::VideoInfoListener {
 public:
  static void Init(v8::Local<v8::Object> exports);
  owt_base::VideoFrameConstructor* me;
  owt_base::FrameSource* src;

  erizo::MediaSink* msink;
 private:
  VideoFrameConstructor();
  ~VideoFrameConstructor();
  static v8::Persistent<v8::Function> constructor;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void close(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void bindTransport(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void unbindTransport(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void addDestination(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void removeDestination(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void setBitrate(const v8::FunctionCallbackInfo<v8::Value>& args);

  //FIXME: Temporarily add this interface to workround the hardware mode's absence of feedback mechanism.
  static void requestKeyFrame(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void addEventListener(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Implement owt_base::VideoInfoListener
  void onVideoInfo(const std::string& message) override;
};

#endif
