/*
 * Copyright 2014 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "VideoFrameConstructorWrapper.h"
#include "WebRtcConnection.h"

using namespace v8;

Persistent<Function> VideoFrameConstructor::constructor;
VideoFrameConstructor::VideoFrameConstructor() {};
VideoFrameConstructor::~VideoFrameConstructor() {};

void VideoFrameConstructor::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "VideoFrameConstructor"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);

  NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setBitrate", setBitrate);
  NODE_SET_PROTOTYPE_METHOD(tpl, "requestKeyFrame", requestKeyFrame);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "VideoFrameConstructor"), tpl->GetFunction());
}

void VideoFrameConstructor::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  WebRtcConnection* param = ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
  erizo::FeedbackSink *fbSink = param->me;

  VideoFrameConstructor* obj = new VideoFrameConstructor();
  obj->me = new woogeen_base::VideoFrameConstructor(fbSink);
  obj->src = obj->me;
  obj->msink = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void VideoFrameConstructor::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;
  delete me;
}

void VideoFrameConstructor::addDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[0]->ToObject());
  woogeen_base::FrameDestination *dest = param->dest;

  me->addVideoDestination(dest);
}

void VideoFrameConstructor::removeDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[0]->ToObject());
  woogeen_base::FrameDestination *dest = param->dest;

  me->removeVideoDestination(dest);
}

void VideoFrameConstructor::setBitrate(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  int bitrate = args[0]->IntegerValue();

  me->setBitrate(bitrate);
}

void VideoFrameConstructor::requestKeyFrame(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFrameConstructor* obj = ObjectWrap::Unwrap<VideoFrameConstructor>(args.Holder());
  woogeen_base::VideoFrameConstructor* me = obj->me;

  me->RequestKeyFrame();
}

