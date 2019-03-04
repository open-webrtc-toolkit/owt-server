// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "InternalInWrapper.h"

using namespace v8;

Persistent<Function> InternalIn::constructor;
InternalIn::InternalIn() {};
InternalIn::~InternalIn() {};

void InternalIn::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "InternalIn"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getListeningPort", getListeningPort);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "In"), tpl->GetFunction());
}

void InternalIn::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param0(args[0]->ToString());
  std::string protocol = std::string(*param0);
  unsigned int minPort = 0, maxPort = 0;

  if (args.Length() >= 3) {
    minPort = args[1]->Uint32Value();
    maxPort = args[2]->Uint32Value();
  }

  InternalIn* obj = new InternalIn();
  obj->me = new woogeen_base::InternalIn(protocol, minPort, maxPort);
  obj->src = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void InternalIn::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  InternalIn* obj = ObjectWrap::Unwrap<InternalIn>(args.Holder());
  woogeen_base::InternalIn* me = obj->me;
  delete me;
}

void InternalIn::getListeningPort(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  InternalIn* obj = ObjectWrap::Unwrap<InternalIn>(args.This());
  woogeen_base::InternalIn* me = obj->me;

  uint32_t port = me->getListeningPort();

  args.GetReturnValue().Set(Number::New(isolate, port));
}

void InternalIn::addDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  InternalIn* obj = ObjectWrap::Unwrap<InternalIn>(args.Holder());
  woogeen_base::InternalIn* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
  woogeen_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    me->addAudioDestination(dest);
  } else if (track == "video") {
    me->addVideoDestination(dest);
  }
}

void InternalIn::removeDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  InternalIn* obj = ObjectWrap::Unwrap<InternalIn>(args.Holder());
  woogeen_base::InternalIn* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
  woogeen_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    me->removeAudioDestination(dest);
  } else if (track == "video") {
    me->removeVideoDestination(dest);
  }
}
