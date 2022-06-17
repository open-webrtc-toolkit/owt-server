// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "SipGateway.h"
#include "../../addons/common/NodeEventRegistry.h"
#include <nan.h>

using namespace v8;

Persistent<Function> SipGateway::constructor;
SipGateway::SipGateway() {}
SipGateway::~SipGateway() {}

void SipGateway::Init(Local<Object> exports) {
  // Prepare constructor template
  Isolate* isolate = Isolate::GetCurrent();
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(Nan::New("SipGateway").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  SETUP_EVENTED_PROTOTYPE_METHODS(tpl);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "register", sipReg);
  NODE_SET_PROTOTYPE_METHOD(tpl, "makeCall", makeCall);
  NODE_SET_PROTOTYPE_METHOD(tpl, "hangup", hangup);
  NODE_SET_PROTOTYPE_METHOD(tpl, "accept", accept);
  NODE_SET_PROTOTYPE_METHOD(tpl, "reject", reject);
  constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(exports, Nan::New("SipGateway").ToLocalChecked(),
           Nan::GetFunction(tpl).ToLocalChecked());
}

void SipGateway::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SipGateway* obj = new SipGateway();
  obj->me = new sip_gateway::SipGateway();

  obj->me->setEventRegistry(obj);
  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void SipGateway::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;

  delete me;
}

void SipGateway::makeCall(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.Length() < 3 || !args[0]->IsString() || !args[1]->IsBoolean() ||
      !args[2]->IsBoolean()) {
    Nan::ThrowError("Wrong arguments");
    return;
  }
  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string calleeURI = std::string(*param0);
  bool requireAudio = Nan::To<bool>(args[1]).FromJust();
  bool requireVideo = Nan::To<bool>(args[2]).FromJust();
  bool isSuccess = me->makeCall(calleeURI, requireAudio, requireVideo);
  args.GetReturnValue().Set(Boolean::New(isolate,isSuccess));
}

void SipGateway::hangup(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string calleeURI = std::string(*param0);

  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;
  me->hangup(calleeURI);
}

void SipGateway::accept(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string calleeURI = std::string(*param0);

  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;
  bool isSuccess = me->accept(calleeURI);
  args.GetReturnValue().Set(Boolean::New(isolate, isSuccess));
}

void SipGateway::reject(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string calleeURI = std::string(*param0);

  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;
  me->reject(calleeURI);
}

void SipGateway::sipReg(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.Length() < 4 || !args[0]->IsString() || !args[1]->IsString() ||
      !args[2]->IsString() || !args[3]->IsString()) {
    Nan::ThrowError("Wrong arguments");
    return;
  }
  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;
  Nan::Utf8String str0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  Nan::Utf8String str1(Nan::To<v8::String>(args[1]).ToLocalChecked());
  Nan::Utf8String str2(Nan::To<v8::String>(args[2]).ToLocalChecked());
  Nan::Utf8String str3(Nan::To<v8::String>(args[3]).ToLocalChecked());
  std::string sipServerAddr = std::string(*str0);
  std::string userName = std::string(*str1);
  std::string password = std::string(*str2);
  std::string displayName = std::string(*str3);
  bool isSuccess = me->sipRegister(sipServerAddr, userName, password, displayName);
  args.GetReturnValue().Set(Boolean::New(isolate, isSuccess));
}
