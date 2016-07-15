/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

