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

#include "AudioMixerWrapper.h"

using namespace v8;

Persistent<Function> AudioMixer::constructor;
AudioMixer::AudioMixer() {};
AudioMixer::~AudioMixer() {};

void AudioMixer::Init(Handle<Object> exports, Handle<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "AudioMixer"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "enableVAD", enableVAD);
  NODE_SET_PROTOTYPE_METHOD(tpl, "disableVAD", disableVAD);
  NODE_SET_PROTOTYPE_METHOD(tpl, "resetVAD", resetVAD);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addInput", addInput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeInput", removeInput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addOutput", addOutput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeOutput", removeOutput);

  constructor.Reset(isolate, tpl->GetFunction());
  module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());
}

void AudioMixer::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param0(args[0]->ToString());
  std::string config = std::string(*param0);

  AudioMixer* obj = new AudioMixer();
  obj->hasCallback_ = false;
  obj->me = new mcu::AudioMixer(config);

  obj->Wrap(args.This());
  uv_async_init(uv_default_loop(), &obj->asyncVAD_, &AudioMixer::vadCallback);
  args.GetReturnValue().Set(args.This());
}

void AudioMixer::close(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  obj->me = NULL;
  obj->hasCallback_ = false;

  if(!uv_is_closing((uv_handle_t*)&obj->asyncVAD_)) {
    uv_close((uv_handle_t*)&obj->asyncVAD_, NULL);
  }
  delete me;
}

void AudioMixer::enableVAD(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  if (obj->me == NULL){
    return;
  }

  int period = args[0]->IntegerValue();

  obj->me->enableVAD(period, obj);
  obj->hasCallback_ = true;
  obj->vadCallback_.Reset(isolate, Local<Function>::Cast(args[1]));
}

void AudioMixer::disableVAD(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());

  if (obj->hasCallback_) {
    obj->me->disableVAD();
    obj->vadCallback_.Reset();
    obj->vadMsgs.clear();
    obj->hasCallback_ = false;
  }
}

void AudioMixer::resetVAD(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());

  obj->me->resetVAD();
}

void AudioMixer::addInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string participantID = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string codec = std::string(*param1);
  FrameSource* param2 = ObjectWrap::Unwrap<FrameSource>(args[2]->ToObject());
  woogeen_base::FrameSource* src = param2->src;

  bool r = me->addInput(participantID, codec, src);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void AudioMixer::removeInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string participantID = std::string(*param0);

  me->removeInput(participantID);
}

void AudioMixer::addOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string participantID = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string codec = std::string(*param1);
  FrameDestination* param2 = ObjectWrap::Unwrap<FrameDestination>(args[2]->ToObject());
  woogeen_base::FrameDestination* dest = param2->dest;

  bool r = me->addOutput(participantID, codec, dest);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void AudioMixer::removeOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string participantID = std::string(*param0);

  me->removeOutput(participantID);
}

void AudioMixer::notifyVAD(const std::string& participantID) {
  if (!this->hasCallback_){
    return;
  }
  boost::mutex::scoped_lock lock(vadMutex);
  this->vadMsgs.push_back(participantID);
  asyncVAD_.data = this;
  uv_async_send(&asyncVAD_);
}

void AudioMixer::vadCallback(uv_async_t* handle){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  AudioMixer* obj = (AudioMixer*)handle->data;

  if (!obj || obj->me == NULL)
    return;
  boost::mutex::scoped_lock lock(obj->vadMutex);
  if (obj->hasCallback_) {
    while(!obj->vadMsgs.empty()){
      Local<Value> args[] = {String::NewFromUtf8(isolate, obj->vadMsgs.front().c_str())};
      Local<Function>::New(isolate, obj->vadCallback_)->Call(isolate->GetCurrentContext()->Global(), 1, args);
      obj->vadMsgs.pop_front();
    }
  }
}
