// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "AudioFramePacketizerWrapper.h"
#include "MediaDefinitions.h"
#include "WebRtcConnection.h"
#include "MediaStream.h"
#include <WebRtcConnection.h>

using namespace v8;

Persistent<Function> AudioFramePacketizer::constructor;
AudioFramePacketizer::AudioFramePacketizer() {};
AudioFramePacketizer::~AudioFramePacketizer() {};

void AudioFramePacketizer::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "AudioFramePacketizer"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);

  NODE_SET_PROTOTYPE_METHOD(tpl, "bindTransport", bindTransport);
  NODE_SET_PROTOTYPE_METHOD(tpl, "unbindTransport", unbindTransport);
  NODE_SET_PROTOTYPE_METHOD(tpl, "enable", enable);
  NODE_SET_PROTOTYPE_METHOD(tpl, "ssrc", getSsrc);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "AudioFramePacketizer"), tpl->GetFunction());
}

void AudioFramePacketizer::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioFramePacketizer* obj = new AudioFramePacketizer();
  obj->me = new owt_base::AudioFramePacketizer();
  obj->dest = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void AudioFramePacketizer::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  AudioFramePacketizer* obj = ObjectWrap::Unwrap<AudioFramePacketizer>(args.Holder());
  owt_base::AudioFramePacketizer* me = obj->me;
  delete me;
}

void AudioFramePacketizer::bindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioFramePacketizer* obj = ObjectWrap::Unwrap<AudioFramePacketizer>(args.Holder());
  owt_base::AudioFramePacketizer* me = obj->me;

  MediaStream* param = Nan::ObjectWrap::Unwrap<MediaStream>(Nan::To<v8::Object>(args[0]).ToLocalChecked());
  auto wr = std::shared_ptr<erizo::MediaStream>(param->me).get();

  erizo::MediaSink* transport = wr;

  me->bindTransport(transport);
}

void AudioFramePacketizer::unbindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioFramePacketizer* obj = ObjectWrap::Unwrap<AudioFramePacketizer>(args.Holder());
  owt_base::AudioFramePacketizer* me = obj->me;

  me->unbindTransport();
}

void AudioFramePacketizer::enable(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioFramePacketizer* obj = ObjectWrap::Unwrap<AudioFramePacketizer>(args.Holder());
  owt_base::AudioFramePacketizer* me = obj->me;

  bool b = (args[0]->ToBoolean())->BooleanValue();
  me->enable(b);
}

void AudioFramePacketizer::getSsrc(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioFramePacketizer* obj = ObjectWrap::Unwrap<AudioFramePacketizer>(args.Holder());
  owt_base::AudioFramePacketizer* me = obj->me;

  uint32_t ssrc = me->getSsrc();
  args.GetReturnValue().Set(Number::New(isolate, ssrc));
}
