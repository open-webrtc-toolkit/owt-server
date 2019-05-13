// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include "InternalQuic.h"

using namespace v8;

// Class InternalQuicIn
Persistent<Function> InternalQuicIn::constructor;
InternalQuicIn::InternalQuicIn() {};
InternalQuicIn::~InternalQuicIn() {};

void InternalQuicIn::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "InternalQuicIn"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getListeningPort", getListeningPort);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "in"), tpl->GetFunction());
}

void InternalQuicIn::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param0(args[0]->ToString());
  std::string cert_file = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string key_file = std::string(*param1);

  InternalQuicIn* obj = new InternalQuicIn();
  obj->me = new QuicIn(cert_file, key_file);
  obj->src = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void InternalQuicIn::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  InternalQuicIn* obj = ObjectWrap::Unwrap<InternalQuicIn>(args.Holder());
  delete obj->me;
  obj->me = nullptr;
  obj->src = nullptr;
  obj->src = nullptr;
}

void InternalQuicIn::getListeningPort(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  InternalQuicIn* obj = ObjectWrap::Unwrap<InternalQuicIn>(args.This());
  QuicIn* me = obj->me;

  uint32_t port = me->getListeningPort();

  args.GetReturnValue().Set(Number::New(isolate, port));
}

void InternalQuicIn::addDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  InternalQuicIn* obj = ObjectWrap::Unwrap<InternalQuicIn>(args.Holder());
  QuicIn* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
  owt_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    me->addAudioDestination(dest);
  } else if (track == "video") {
    me->addVideoDestination(dest);
  }
}

void InternalQuicIn::removeDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  InternalQuicIn* obj = ObjectWrap::Unwrap<InternalQuicIn>(args.Holder());
  QuicIn* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
  owt_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    me->removeAudioDestination(dest);
  } else if (track == "video") {
    me->removeVideoDestination(dest);
  }
}

// Class InternalQuicOut
Persistent<Function> InternalQuicOut::constructor;
InternalQuicOut::InternalQuicOut() {};
InternalQuicOut::~InternalQuicOut() {};

void InternalQuicOut::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "InternalQuicOut"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "out"), tpl->GetFunction());
}

void InternalQuicOut::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param0(args[0]->ToString());
  std::string ip = std::string(*param0);
  int port = args[1]->Uint32Value();

  InternalQuicOut* obj = new InternalQuicOut();
  obj->me = new QuicOut(ip, port);
  obj->dest = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void InternalQuicOut::close(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  InternalQuicOut* obj = ObjectWrap::Unwrap<InternalQuicOut>(args.Holder());
  delete obj->me;
  obj->me = nullptr;
  obj->dest = nullptr;
}
