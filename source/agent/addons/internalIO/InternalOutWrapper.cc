// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "InternalOutWrapper.h"

using namespace v8;

Persistent<Function> InternalOut::constructor;
InternalOut::InternalOut() {};
InternalOut::~InternalOut() {};

void InternalOut::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "InternalOut"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "Out"), tpl->GetFunction());
}

void InternalOut::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param0(args[0]->ToString());
  std::string protocol = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string dest_ip = std::string(*param1);
  unsigned int dest_port = args[2]->Uint32Value();

  InternalOut* obj = new InternalOut();
  obj->me = new woogeen_base::InternalOut(protocol, dest_ip, dest_port);
  obj->dest = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void InternalOut::close(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  InternalOut* obj = ObjectWrap::Unwrap<InternalOut>(args.Holder());
  woogeen_base::InternalOut* me = obj->me;
  delete me;
}
