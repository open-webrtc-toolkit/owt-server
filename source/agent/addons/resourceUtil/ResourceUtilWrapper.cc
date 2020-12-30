// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "ResourceUtilWrapper.h"
#include "VideoHelper.h"
#include <iostream>

using namespace v8;
using namespace owt_base;

Persistent<Function> ResourceUtil::constructor;
ResourceUtil::ResourceUtil() {};
ResourceUtil::~ResourceUtil() {};

void ResourceUtil::Init(Handle<Object> exports, Handle<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  printf("Wrapper init");
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "ResourceUtil"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "getVPUUtil", getVPUUtil);

  constructor.Reset(isolate, tpl->GetFunction());
  module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());
}

void ResourceUtil::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  printf("Wrapper new");
  

  ResourceUtil* obj = new ResourceUtil();
  obj->me = new mcu::ResourceUtil();

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void ResourceUtil::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  ResourceUtil* obj = ObjectWrap::Unwrap<ResourceUtil>(args.Holder());
  mcu::ResourceUtil* me = obj->me;
  delete me;
}

void ResourceUtil::getVPUUtil(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  ResourceUtil* obj = ObjectWrap::Unwrap<ResourceUtil>(args.Holder());
  mcu::ResourceUtil* me = obj->me;

  float port = me->getVPUUtil();
  args.GetReturnValue().Set(Number::New(isolate, port));
}
