/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
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
  exports->Set(String::NewFromUtf8(isolate, "InternalIn"), tpl->GetFunction());
}

void InternalIn::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param0(args[0]->ToString());
  std::string protocol = std::string(*param0);

  InternalIn* obj = new InternalIn();
  obj->me = new woogeen_base::InternalIn(protocol);
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
  woogeen_base::InternalIn *me = obj->me;

  uint32_t port = me-> getListeningPort();

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
  woogeen_base::FrameDestination *dest = param->dest;

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
  woogeen_base::FrameDestination *dest = param->dest;

  if (track == "audio") {
    me->removeAudioDestination(dest);
  } else if (track == "video") {
    me->removeVideoDestination(dest);
  }
}

