// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VIDEOFRAMECONSTRUCTORWRAPPER_H
#define VIDEOFRAMECONSTRUCTORWRAPPER_H

#include "MediaWrapper.h"
#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <VideoFrameConstructor.h>
#include <node.h>
#include <node_object_wrap.h>
#include <nan.h>

/*
 * Wrapper class of owt_base::VideoFrameConstructor
 */
class VideoFrameConstructor : public MediaSink, public owt_base::VideoInfoListener {
 public:
  static NAN_MODULE_INIT(Init);
  owt_base::VideoFrameConstructor* me;
  owt_base::FrameSource* src;

  std::queue<std::string> videoInfoMsgs;
  boost::mutex mutex;

  owt_base::VideoFrameConstructor* parent;
  std::string layerId;
 private:
  Nan::Callback *Callback_;
  uv_async_t async_;
  Nan::AsyncResource *asyncResource_;

  VideoFrameConstructor();
  ~VideoFrameConstructor();

  static NAN_METHOD(New);
  static NAN_METHOD(close);

  static NAN_METHOD(bindTransport);
  static NAN_METHOD(unbindTransport);

  static NAN_METHOD(enable);

  static NAN_METHOD(addDestination);
  static NAN_METHOD(removeDestination);

  static NAN_METHOD(setBitrate);
  static NAN_METHOD(setPreferredLayers);

  static NAN_METHOD(requestKeyFrame);

  static NAN_METHOD(source);

  static Nan::Persistent<v8::Function> constructor;

  static NAUV_WORK_CB(Callback);
  virtual void onVideoInfo(const std::string& message);
};

class VideoFrameSource : public FrameSource {
 public:
  static NAN_MODULE_INIT(Init);
  owt_base::VideoFrameConstructor* me;

 private:
  VideoFrameSource() {};
  ~VideoFrameSource() {};

  static NAN_METHOD(New);

  static Nan::Persistent<v8::Function> constructor;

  friend class VideoFrameConstructor;
};

#endif
