// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "MediaDefinitions.h"
#include "SipCallConnection.h"
#include "SipGateway.h"

using namespace v8;

Persistent<Function> SipCallConnection::constructor;
SipCallConnection::SipCallConnection() {}
SipCallConnection::~SipCallConnection() {}

void SipCallConnection::Init(Local<Object> exports) {
  // Prepare constructor template
  Isolate* isolate = Isolate::GetCurrent();
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "SipCallConnection"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setAudioReceiver", setAudioReceiver);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setVideoReceiver", setVideoReceiver);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "SipCallConnection"), tpl->GetFunction());
}

void SipCallConnection::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  SipCallConnection* obj = new SipCallConnection();

  SipGateway* gateway = ObjectWrap::Unwrap<SipGateway>(args[0]->ToObject());

  v8::String::Utf8Value str0(args[1]->ToString());
  std::string calleeURI = std::string(*str0);
  obj->me = new sip_gateway::SipCallConnection(gateway->me, calleeURI);
  obj->msink = obj->me;
  obj->msource = obj->me;
  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void SipCallConnection::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SipCallConnection* obj = ObjectWrap::Unwrap<SipCallConnection>(args.Holder());
  sip_gateway::SipCallConnection* me = obj->me;
  delete me;
  me = NULL;
}

void SipCallConnection::setAudioReceiver(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SipCallConnection* obj = ObjectWrap::Unwrap<SipCallConnection>(args.Holder());
  sip_gateway::SipCallConnection* me = obj->me;
  MediaSink* param = ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink* mr = param->msink;
  me->setAudioSink(mr);
}

void SipCallConnection::setVideoReceiver(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SipCallConnection* obj = ObjectWrap::Unwrap<SipCallConnection>(args.Holder());
  sip_gateway::SipCallConnection* me = obj->me;

  MediaSink* param = ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSink* mr = param->msink;

  me->setVideoSink(mr);
}

