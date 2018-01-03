/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#include "MediaDefinitions.h"
#include "VideoFramePacketizerWrapper.h"
#include <WebRtcConnection.h>
#include "WebRtcConnection.h"

using namespace v8;

Persistent<Function> VideoFramePacketizer::constructor;
VideoFramePacketizer::VideoFramePacketizer() {};
VideoFramePacketizer::~VideoFramePacketizer() {};

void VideoFramePacketizer::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "VideoFramePacketizer"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);

  NODE_SET_PROTOTYPE_METHOD(tpl, "bindTransport", bindTransport);
  NODE_SET_PROTOTYPE_METHOD(tpl, "unbindTransport", unbindTransport);
  NODE_SET_PROTOTYPE_METHOD(tpl, "enable", enable);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "VideoFramePacketizer"), tpl->GetFunction());
}

void VideoFramePacketizer::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  bool supportRED = (args[0]->ToBoolean())->BooleanValue();
  bool supportULPFEC = (args[1]->ToBoolean())->BooleanValue();
  VideoFramePacketizer* obj = new VideoFramePacketizer();
  obj->me = new woogeen_base::VideoFramePacketizer(supportRED, supportULPFEC);
  obj->dest = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void VideoFramePacketizer::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  woogeen_base::VideoFramePacketizer* me = obj->me;
  delete me;
}

void VideoFramePacketizer::bindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  woogeen_base::VideoFramePacketizer* me = obj->me;

  WebRtcConnection* param = Nan::ObjectWrap::Unwrap<WebRtcConnection>(Nan::To<v8::Object>(args[0]).ToLocalChecked());
  auto wr = std::shared_ptr<erizo::WebRtcConnection>(param->me).get();

  //MediaSink* param = Nan::ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink* transport = wr;

  me->bindTransport(transport);
}

void VideoFramePacketizer::unbindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  woogeen_base::VideoFramePacketizer* me = obj->me;

  me->unbindTransport();
}

void VideoFramePacketizer::enable(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  woogeen_base::VideoFramePacketizer* me = obj->me;

  bool b = (args[0]->ToBoolean())->BooleanValue();
  me->enable(b);
}

