// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "AudioFrameConstructorWrapper.h"
#include "../../addons/common/MediaFramePipelineWrapper.h"

using namespace v8;

Nan::Persistent<Function> AudioFrameConstructor::constructor;

AudioFrameConstructor::AudioFrameConstructor() {}
AudioFrameConstructor::~AudioFrameConstructor() {}

NAN_MODULE_INIT(AudioFrameConstructor::Init) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("AudioFrameConstructor").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "close", close);
  Nan::SetPrototypeMethod(tpl, "bindTransport", bindTransport);
  Nan::SetPrototypeMethod(tpl, "unbindTransport", unbindTransport);
  Nan::SetPrototypeMethod(tpl, "enable", enable);
  Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
  Nan::SetPrototypeMethod(tpl, "removeDestination", removeDestination);

  constructor.Reset(tpl->GetFunction());
  Nan::Set(target, Nan::New("AudioFrameConstructor").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(AudioFrameConstructor::New) {
  if (info.IsConstructCall()) {
    AudioFrameConstructor* obj = new AudioFrameConstructor();
    obj->me = new owt_base::AudioFrameConstructor();
    obj->src = obj->me;
    obj->msink = obj->me;

    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    // const int argc = 1;
    // v8::Local<v8::Value> argv[argc] = {info[0]};
    // v8::Local<v8::Function> cons = Nan::New(constructor);
    // info.GetReturnValue().Set(cons->NewInstance(argc, argv));
  }
}

NAN_METHOD(AudioFrameConstructor::close) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  owt_base::AudioFrameConstructor* me = obj->me;
  delete me;
}

NAN_METHOD(AudioFrameConstructor::bindTransport) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  owt_base::AudioFrameConstructor* me = obj->me;

  MediaFilter* param = Nan::ObjectWrap::Unwrap<MediaFilter>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  erizo::MediaSource* source = param->msource;

  me->bindTransport(source, source->getFeedbackSink());
}

NAN_METHOD(AudioFrameConstructor::unbindTransport) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  owt_base::AudioFrameConstructor* me = obj->me;

  me->unbindTransport();
}

NAN_METHOD(AudioFrameConstructor::addDestination) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  owt_base::AudioFrameConstructor* me = obj->me;

  FrameDestination* param = node::ObjectWrap::Unwrap<FrameDestination>(info[0]->ToObject());
  owt_base::FrameDestination* dest = param->dest;

  me->addAudioDestination(dest);
}

NAN_METHOD(AudioFrameConstructor::removeDestination) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  owt_base::AudioFrameConstructor* me = obj->me;

  FrameDestination* param = node::ObjectWrap::Unwrap<FrameDestination>(info[0]->ToObject());
  owt_base::FrameDestination* dest = param->dest;

  me->removeAudioDestination(dest);
}

NAN_METHOD(AudioFrameConstructor::enable) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  owt_base::AudioFrameConstructor* me = obj->me;

  bool b = (info[0]->ToBoolean())->BooleanValue();
  me->enable(b);
}

