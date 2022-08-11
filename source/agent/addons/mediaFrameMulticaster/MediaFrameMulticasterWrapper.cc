// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "MediaFrameMulticasterWrapper.h"

using namespace v8;

Persistent<Function> MulticasterSource::constructor;

MulticasterSource::MulticasterSource() {};
MulticasterSource::~MulticasterSource() {};

void MulticasterSource::Init(Local<Object> exports, Local<Object> module) {
  Isolate* isolate = exports->GetIsolate();

  // Constructor for FrameSource
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(Nan::New("MulticasterSource").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
}

void MulticasterSource::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.Length() > 0) {
    MediaFrameMulticaster* parent = ObjectWrap::Unwrap<MediaFrameMulticaster>(
      Nan::To<v8::Object>(args[0]).ToLocalChecked());
    MulticasterSource* obj = new MulticasterSource();
    obj->me = parent->me;
    obj->src = obj->me;
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  }
}

Persistent<Function> MediaFrameMulticaster::constructor;

MediaFrameMulticaster::MediaFrameMulticaster() {};
MediaFrameMulticaster::~MediaFrameMulticaster() {};

void MediaFrameMulticaster::Init(Local<Object> exports, Local<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(Nan::New("MediaFrameMulticaster").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "source", source);

  constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(module, Nan::New("exports").ToLocalChecked(),
      Nan::GetFunction(tpl).ToLocalChecked());

  // Init MulticasterSource
  MulticasterSource::Init(exports, module);
}

void MediaFrameMulticaster::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  MediaFrameMulticaster* obj = new MediaFrameMulticaster();
  obj->me = new owt_base::MediaFrameMulticaster();
  obj->dest = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void MediaFrameMulticaster::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  MediaFrameMulticaster* obj = ObjectWrap::Unwrap<MediaFrameMulticaster>(args.Holder());
  owt_base::MediaFrameMulticaster* me = obj->me;
  delete me;
}

void MediaFrameMulticaster::addDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  MediaFrameMulticaster* obj = ObjectWrap::Unwrap<MediaFrameMulticaster>(args.Holder());
  owt_base::MediaFrameMulticaster* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(
    Nan::To<v8::Object>(args[1]).ToLocalChecked());
  owt_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    me->addAudioDestination(dest);
  } else if (track == "video") {
    me->addVideoDestination(dest);
  }
}

void MediaFrameMulticaster::removeDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  MediaFrameMulticaster* obj = ObjectWrap::Unwrap<MediaFrameMulticaster>(args.Holder());
  owt_base::MediaFrameMulticaster* me = obj->me;

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(
    Nan::To<v8::Object>(args[1]).ToLocalChecked());
  owt_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    me->removeAudioDestination(dest);
  } else if (track == "video") {
    me->removeVideoDestination(dest);
  }
}

void MediaFrameMulticaster::source(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {args.Holder()};
  v8::Local<v8::Function> cons = Nan::New(MulticasterSource::constructor);
  args.GetReturnValue().Set(Nan::NewInstance(cons, 1, argv).ToLocalChecked());
}

