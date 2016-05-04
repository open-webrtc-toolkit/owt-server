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
RtspIn::RtspIn(bool a, bool v) : enableAudio{a}, enableVideo{v} {};
RtspIn::~RtspIn() {};

void RtspIn::Init(Handle<Object> exports, Handle<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "RtspIn"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  SETUP_EVENTED_PROTOTYPE_METHODS(tpl);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);

  constructor.Reset(isolate, tpl->GetFunction());
  module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());

  av_register_all();
  avcodec_register_all();
  avformat_network_init();
  av_log_set_level(AV_LOG_WARNING);
}

void RtspIn::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  std::string url = std::string(*String::Utf8Value(args[0]->ToString()));
  bool a = (args[1]->ToBoolean())->BooleanValue();
  bool v = (args[2]->ToBoolean())->BooleanValue();
  std::string transport = std::string(*String::Utf8Value(args[3]->ToString()));
  int buffSize = args[4]->IntegerValue();
  if (buffSize <= 0) {
    buffSize = 1024*1024*2;
  }
  RtspIn* obj = new RtspIn(a, v);
  obj->me = new woogeen_base::RtspIn(url, transport, buffSize, a, v);
  obj->me->setEventRegistry(obj);
  obj->me->init();
  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void RtspIn::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  RtspIn* obj = ObjectWrap::Unwrap<RtspIn>(args.Holder());
  woogeen_base::RtspIn* me = obj->me;
  delete me;
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

  if (obj->enableAudio && track == "audio") {
    me->addAudioDestination(dest);
  } else if (obj->enableVideo && track == "video") {
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

  if (obj->enableAudio && track == "audio") {
    me->removeAudioDestination(dest);
  } else if (obj->enableVideo && track == "video") {
    me->removeVideoDestination(dest);
  }
}
