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

#include "RtspInWrapper.h"

using namespace v8;

Persistent<Function> RtspIn::constructor;
RtspIn::RtspIn() {};
RtspIn::~RtspIn() {};

void RtspIn::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "RtspIn"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "init", init);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "RtspIn"), tpl->GetFunction());
}

void RtspIn::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param0(args[0]->ToString());
  std::string url = std::string(*param0);

  bool a = (args[1]->ToBoolean())->BooleanValue();
  bool v = (args[2]->ToBoolean())->BooleanValue();

  String::Utf8Value param3(args[3]->ToString());
  std::string transport = std::string(*param3);

  int buffSize = args[4]->IntegerValue();
  if (buffSize <= 0) {
    buffSize = 1024*1024*2;
  }
  RtspIn* obj = new RtspIn();
  obj->me = new woogeen_base::RtspIn(url, transport, buffSize, a, v);
  obj->src = obj->me;
  obj->me->setStatusListener(obj);

  obj->Wrap(args.This());
  uv_async_init(uv_default_loop(), &obj->async_, &RtspIn::statusCallback);
  args.GetReturnValue().Set(args.This());
}

void RtspIn::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  RtspIn* obj = ObjectWrap::Unwrap<RtspIn>(args.Holder());
  woogeen_base::RtspIn* me = obj->me;
  delete me;
}

void RtspIn::init(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  RtspIn* obj = ObjectWrap::Unwrap<RtspIn>(args.Holder());
  woogeen_base::RtspIn* me = (woogeen_base::RtspIn*) obj->me;

  obj->statusCallback_.Reset(isolate, Local<Function>::Cast(args[0]));
  me->init();
}

void RtspIn::addDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  RtspIn* obj = ObjectWrap::Unwrap<RtspIn>(args.Holder());
  woogeen_base::RtspIn* me = obj->me;

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

void RtspIn::removeDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  RtspIn* obj = ObjectWrap::Unwrap<RtspIn>(args.Holder());
  woogeen_base::RtspIn* me = obj->me;

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

void RtspIn::notifyStatus(const std::string& message) {
  boost::mutex::scoped_lock lock(statusMutex);
  this->statsMsgs.push(message);
  async_.data = this;
  uv_async_send(&async_);
}

void RtspIn::statusCallback(uv_async_t* handle) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  RtspIn* obj = (RtspIn*)handle->data;
  if (!obj)
    return;
  boost::mutex::scoped_lock lock(obj->statusMutex);
  while(!obj->statsMsgs.empty()){
    Local<Value> args[] = {String::NewFromUtf8(isolate, obj->statsMsgs.front().c_str())};
    Local<Function>::New(isolate, obj->statusCallback_)->Call(isolate->GetCurrentContext()->Global(), 1, args);
    obj->statsMsgs.pop();
  }
}

