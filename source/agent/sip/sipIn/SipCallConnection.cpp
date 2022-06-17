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
  tpl->SetClassName(Nan::New("SipCallConnection").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setAudioReceiver", setAudioReceiver);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setVideoReceiver", setVideoReceiver);

  constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(exports, Nan::New("SipCallConnection").ToLocalChecked(),
           Nan::GetFunction(tpl).ToLocalChecked());
}

void SipCallConnection::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  SipCallConnection* obj = new SipCallConnection();

  SipGateway* gateway = node::ObjectWrap::Unwrap<SipGateway>(
    Nan::To<v8::Object>(args[0]).ToLocalChecked());

  Nan::Utf8String str0(Nan::To<v8::String>(args[0]).ToLocalChecked());
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

  SipCallConnection* obj = Nan::ObjectWrap::Unwrap<SipCallConnection>(args.Holder());
  sip_gateway::SipCallConnection* me = obj->me;
  delete me;
  me = NULL;
}

void SipCallConnection::setAudioReceiver(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SipCallConnection* obj = Nan::ObjectWrap::Unwrap<SipCallConnection>(args.Holder());
  sip_gateway::SipCallConnection* me = obj->me;
  MediaSink* param = Nan::ObjectWrap::Unwrap<MediaSink>(
    Nan::To<v8::Object>(args[0]).ToLocalChecked());
  erizo::MediaSink* mr = param->msink;
  me->setAudioSink(mr);
}

void SipCallConnection::setVideoReceiver(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SipCallConnection* obj = Nan::ObjectWrap::Unwrap<SipCallConnection>(args.Holder());
  sip_gateway::SipCallConnection* me = obj->me;

  MediaSink* param = Nan::ObjectWrap::Unwrap<MediaSink>(
    Nan::To<v8::Object>(args[0]).ToLocalChecked());
  erizo::MediaSink* mr = param->msink;

  me->setVideoSink(mr);
}
