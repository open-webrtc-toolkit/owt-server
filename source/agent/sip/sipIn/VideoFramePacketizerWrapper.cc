// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "MediaDefinitions.h"
#include "VideoFramePacketizerWrapper.h"
#include "SipCallConnection.h"

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

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "VideoFramePacketizer"), tpl->GetFunction());
}

void VideoFramePacketizer::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  bool supportRED = (args[0]->ToBoolean())->BooleanValue();
  bool supportULPFEC = (args[1]->ToBoolean())->BooleanValue();
  VideoFramePacketizer* obj = new VideoFramePacketizer();
  obj->me = new owt_base::VideoFramePacketizer(supportRED, supportULPFEC, false, true);
  obj->dest = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void VideoFramePacketizer::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;
  delete me;
}

void VideoFramePacketizer::bindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;

  SipCallConnection* param = ObjectWrap::Unwrap<SipCallConnection>(args[0]->ToObject());
  sip_gateway::SipCallConnection* transport = param->me;

  me->bindTransport(transport);
}

void VideoFramePacketizer::unbindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoFramePacketizer* obj = ObjectWrap::Unwrap<VideoFramePacketizer>(args.Holder());
  owt_base::VideoFramePacketizer* me = obj->me;

  me->unbindTransport();
}

