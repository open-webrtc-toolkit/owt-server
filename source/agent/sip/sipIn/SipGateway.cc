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

#include "SipGateway.h"
#include "../../addons/common/NodeEventRegistry.h"

using namespace v8;

Persistent<Function> SipGateway::constructor;
SipGateway::SipGateway() {}
SipGateway::~SipGateway() {}

void SipGateway::Init(Local<Object> exports) {
  // Prepare constructor template
  Isolate* isolate = Isolate::GetCurrent();
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "SipGateway"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  SETUP_EVENTED_PROTOTYPE_METHODS(tpl);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "register", sipReg);
  NODE_SET_PROTOTYPE_METHOD(tpl, "makeCall", makeCall);
  NODE_SET_PROTOTYPE_METHOD(tpl, "hangup", hangup);
  NODE_SET_PROTOTYPE_METHOD(tpl, "accept", accept);
  NODE_SET_PROTOTYPE_METHOD(tpl, "reject", reject);
  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "SipGateway"), tpl->GetFunction());
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
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
  }
  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;
  v8::String::Utf8Value str0(args[0]->ToString());
  std::string calleeURI = std::string(*str0);
  bool requireAudio = args[1]->BooleanValue();
  bool requireVideo = args[2]->BooleanValue();
  bool isSuccess = me->makeCall(calleeURI, requireAudio, requireVideo);
  args.GetReturnValue().Set(Boolean::New(isolate,isSuccess));
}

void SipGateway::hangup(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  v8::String::Utf8Value str0(args[0]->ToString());
  std::string calleeURI = std::string(*str0);

  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;
  me->hangup(calleeURI);
}

void SipGateway::accept(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  v8::String::Utf8Value str0(args[0]->ToString());
  std::string calleeURI = std::string(*str0);

  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;
  bool isSuccess = me->accept(calleeURI);
  args.GetReturnValue().Set(Boolean::New(isolate, isSuccess));
}

void SipGateway::reject(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  v8::String::Utf8Value str0(args[0]->ToString());
  std::string calleeURI = std::string(*str0);

  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;
  me->reject(calleeURI);
}

void SipGateway::sipReg(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.Length() < 4 || !args[0]->IsString() || !args[1]->IsString() ||
      !args[2]->IsString() || !args[3]->IsString()) {
     isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
  }
  SipGateway* obj = ObjectWrap::Unwrap<SipGateway>(args.Holder());
  sip_gateway::SipGateway* me = obj->me;
  v8::String::Utf8Value str0(args[0]->ToString());
  v8::String::Utf8Value str1(args[1]->ToString());
  v8::String::Utf8Value str2(args[2]->ToString());
  v8::String::Utf8Value str3(args[3]->ToString());
  std::string sipServerAddr = std::string(*str0);
  std::string userName = std::string(*str1);
  std::string password = std::string(*str2);
  std::string displayName = std::string(*str3);
  bool isSuccess = me->sipRegister(sipServerAddr, userName, password, displayName);
  args.GetReturnValue().Set(Boolean::New(isolate, isSuccess));
}
