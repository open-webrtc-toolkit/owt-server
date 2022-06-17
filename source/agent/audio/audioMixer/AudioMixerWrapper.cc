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

void AudioMixer::Init(Local<Object> exports, Local<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(Nan::New("AudioMixer").ToLocalChecked());
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


  constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(module, Nan::New("exports").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

void AudioMixer::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args[0]->IsString()) {
    Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
    std::string config = std::string(*param0);

    AudioMixer* obj = new AudioMixer();
    obj->me = new mcu::AudioMixer(config);

    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    Nan::ThrowError("Argument is not string");
  }
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

  int period = Nan::To<int32_t>(args[0]).FromJust();

  obj->me->setEventRegistry(obj);
  obj->me->enableVAD(period);
  if (args.Length() > 1 && args[1]->IsFunction())
    Nan::Set(Nan::New(obj->m_store), Nan::New("vad").ToLocalChecked(), args[1]);
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

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string endpointID = std::string(*param0);
  Nan::Utf8String param1(Nan::To<v8::String>(args[1]).ToLocalChecked());
  std::string streamID = std::string(*param1);
  Nan::Utf8String param2(Nan::To<v8::String>(args[2]).ToLocalChecked());
  std::string codec = std::string(*param2);
  FrameSource* param3 = ObjectWrap::Unwrap<FrameSource>(args[3]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameSource* src = param3->src;

  bool r = me->addInput(endpointID, streamID, codec, src);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void AudioMixer::removeInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string endpointID = std::string(*param0);
  Nan::Utf8String param1(Nan::To<v8::String>(args[1]).ToLocalChecked());
  std::string streamID = std::string(*param1);

  me->removeInput(endpointID, streamID);
}

void AudioMixer::setInputActive(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string endpointID = std::string(*param0);
  Nan::Utf8String param1(Nan::To<v8::String>(args[1]).ToLocalChecked());
  std::string streamID = std::string(*param1);
  bool active = Nan::To<bool>(args[2]).FromJust();

  me->setInputActive(endpointID, streamID, active);
}

void AudioMixer::addOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string endpointID = std::string(*param0);
  Nan::Utf8String param1(Nan::To<v8::String>(args[1]).ToLocalChecked());
  std::string streamID = std::string(*param1);
  Nan::Utf8String param2(Nan::To<v8::String>(args[2]).ToLocalChecked());
  std::string codec = std::string(*param2);
  FrameDestination* param3 = ObjectWrap::Unwrap<FrameDestination>(
      args[3]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameDestination* dest = param3->dest;

  bool r = me->addOutput(endpointID, streamID, codec, dest);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void AudioMixer::removeOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioMixer* obj = ObjectWrap::Unwrap<AudioMixer>(args.Holder());
  mcu::AudioMixer* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string endpointID = std::string(*param0);
  Nan::Utf8String param1(Nan::To<v8::String>(args[1]).ToLocalChecked());
  std::string streamID = std::string(*param1);

  me->removeOutput(endpointID, streamID);
}
