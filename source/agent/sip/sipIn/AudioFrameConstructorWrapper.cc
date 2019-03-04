// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "AudioFrameConstructorWrapper.h"
#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "SipCallConnection.h"

using namespace v8;

Persistent<Function> AudioFrameConstructor::constructor;
AudioFrameConstructor::AudioFrameConstructor() {};
AudioFrameConstructor::~AudioFrameConstructor() {};

void AudioFrameConstructor::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "AudioFrameConstructor"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);

  NODE_SET_PROTOTYPE_METHOD(tpl, "bindTransport", bindTransport);
  NODE_SET_PROTOTYPE_METHOD(tpl, "unbindTransport", unbindTransport);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "AudioFrameConstructor"), tpl->GetFunction());
}

void AudioFrameConstructor::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioFrameConstructor* obj = new AudioFrameConstructor();
  obj->me = new woogeen_base::AudioFrameConstructor();
  obj->src = obj->me;
  obj->msink = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void AudioFrameConstructor::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  AudioFrameConstructor* obj = ObjectWrap::Unwrap<AudioFrameConstructor>(args.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;
  delete me;
}

void AudioFrameConstructor::bindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioFrameConstructor* obj = ObjectWrap::Unwrap<AudioFrameConstructor>(args.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;

  SipCallConnection* param = ObjectWrap::Unwrap<SipCallConnection>(args[0]->ToObject());
  sip_gateway::SipCallConnection* transport = param->me;

  me->bindTransport(transport, transport);
}

void AudioFrameConstructor::unbindTransport(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioFrameConstructor* obj = ObjectWrap::Unwrap<AudioFrameConstructor>(args.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;

  me->unbindTransport();
}

void AudioFrameConstructor::addDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioFrameConstructor* obj = ObjectWrap::Unwrap<AudioFrameConstructor>(args.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[0]->ToObject());
  woogeen_base::FrameDestination* dest = param->dest;

  me->addAudioDestination(dest);
}

void AudioFrameConstructor::removeDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  AudioFrameConstructor* obj = ObjectWrap::Unwrap<AudioFrameConstructor>(args.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[0]->ToObject());
  woogeen_base::FrameDestination* dest = param->dest;

  me->removeAudioDestination(dest);
}

