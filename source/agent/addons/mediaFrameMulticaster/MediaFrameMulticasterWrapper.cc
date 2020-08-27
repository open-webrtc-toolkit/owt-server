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

void MulticasterSource::Init(Handle<Object> exports, Handle<Object> module) {
  Isolate* isolate = exports->GetIsolate();

  // Constructor for FrameSource
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "MulticasterSource"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  constructor.Reset(isolate, tpl->GetFunction());
}

void MulticasterSource::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.Length() > 0) {
    MediaFrameMulticaster* parent = ObjectWrap::Unwrap<MediaFrameMulticaster>(args[0]->ToObject());
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

void MediaFrameMulticaster::Init(Handle<Object> exports, Handle<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "MediaFrameMulticaster"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "source", source);

  constructor.Reset(isolate, tpl->GetFunction());
  module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());

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

  String::Utf8Value param0(args[0]->ToString());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
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

  String::Utf8Value param0(args[0]->ToString());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
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

