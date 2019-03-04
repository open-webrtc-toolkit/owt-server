// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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
  NODE_SET_PROTOTYPE_METHOD(tpl, "setInputActive", setInputActive);
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
  obj->me = new mcu::AudioMixer(config);

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void AudioMixer::close(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;
  obj->me = nullptr;
  delete me;
}

void AudioMixer::enableVAD(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  if (obj->me == nullptr)
    return;

  int period = args[0]->IntegerValue();

  obj->me->setEventRegistry(obj);
  obj->me->enableVAD(period);
  if (args.Length() > 1 && args[1]->IsFunction())
    Local<Object>::New(isolate, obj->m_store)->Set(String::NewFromUtf8(isolate, "vad"), args[1]);
}

void AudioMixer::disableVAD(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  obj->me->disableVAD();
  obj->me->setEventRegistry(nullptr);
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
  std::string endpointID = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string streamID = std::string(*param1);
  String::Utf8Value param2(args[2]->ToString());
  std::string codec = std::string(*param2);
  FrameSource* param3 = ObjectWrap::Unwrap<FrameSource>(args[3]->ToObject());
  woogeen_base::FrameSource* src = param3->src;

  bool r = me->addInput(endpointID, streamID, codec, src);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void AudioMixer::removeInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string endpointID = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string streamID = std::string(*param1);

  me->removeInput(endpointID, streamID);
}

void AudioMixer::setInputActive(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string endpointID = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string streamID = std::string(*param1);
  bool active = args[2]->ToBoolean()->Value();

  me->setInputActive(endpointID, streamID, active);
}

void AudioMixer::addOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string endpointID = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string streamID = std::string(*param1);
  String::Utf8Value param2(args[2]->ToString());
  std::string codec = std::string(*param2);
  FrameDestination* param3 = ObjectWrap::Unwrap<FrameDestination>(args[3]->ToObject());
  woogeen_base::FrameDestination* dest = param3->dest;

  bool r = me->addOutput(endpointID, streamID, codec, dest);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void AudioMixer::removeOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string endpointID = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string streamID = std::string(*param1);

  me->removeOutput(endpointID, streamID);
}
