// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "VideoGstAnalyzerWrap.h"
#include "VideoHelper.h"
#include <iostream>

using namespace v8;

Persistent<Function> VideoGstAnalyzerWrap::constructor;
VideoGstAnalyzerWrap::VideoGstAnalyzerWrap() {};
VideoGstAnalyzerWrap::~VideoGstAnalyzerWrap() {};

void VideoGstAnalyzerWrap::Init(Handle<Object> exports, Handle<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "VideoGstAnalyzer"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "createPipeline", createPipeline);
  NODE_SET_PROTOTYPE_METHOD(tpl, "clearPipeline", clearPipeline);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeOutput", removeOutput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addOutput", addOutput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "linkInput", linkInput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addEventListener", addEventListener);

  constructor.Reset(isolate, tpl->GetFunction());
  module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());
}

void VideoGstAnalyzerWrap::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoGstAnalyzerWrap* obj = new VideoGstAnalyzerWrap();
  obj->me = new mcu::VideoGstAnalyzer(obj);

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void VideoGstAnalyzerWrap::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzerWrap* obj = ObjectWrap::Unwrap<VideoGstAnalyzerWrap>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;
  delete me;
}

void VideoGstAnalyzerWrap::createPipeline(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzerWrap* obj = ObjectWrap::Unwrap<VideoGstAnalyzerWrap>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string codec = std::string(*param0);

  String::Utf8Value param1(args[1]->ToString());
  std::string resolution = std::string(*param1);
  owt_base::VideoSize vSize{0, 0};
  owt_base::VideoResolutionHelper::getVideoSize(resolution, vSize);
  unsigned int width = vSize.width;
  unsigned int height = vSize.height;

  unsigned int framerateFPS = args[2]->Uint32Value();
  unsigned int bitrateKbps = args[3]->Uint32Value();
  unsigned int keyFrameIntervalSeconds = args[4]->Uint32Value();

  String::Utf8Value param6(args[5]->ToString());
  std::string algorithm = std::string(*param6);

  String::Utf8Value param7(args[6]->ToString());
  std::string pluginName = std::string(*param7);

  bool result = me->createPipeline(codec,width,height,framerateFPS,bitrateKbps,
                       keyFrameIntervalSeconds,algorithm,pluginName);

  args.GetReturnValue().Set(Number::New(isolate, result));
}

void VideoGstAnalyzerWrap::clearPipeline(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzerWrap* obj = ObjectWrap::Unwrap<VideoGstAnalyzerWrap>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  me->clearPipeline();
}

void VideoGstAnalyzerWrap::linkInput(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoGstAnalyzerWrap* obj = ObjectWrap::Unwrap<VideoGstAnalyzerWrap>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  unsigned int index = 0;

  FrameSource* param8 = ObjectWrap::Unwrap<FrameSource>(args[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameSource* src = param8->src;

  bool result = me->linkInput(src);
  args.GetReturnValue().Set(Number::New(isolate, result));
}

void VideoGstAnalyzerWrap::removeOutput(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzerWrap* obj = ObjectWrap::Unwrap<VideoGstAnalyzerWrap>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  FrameDestination* param8 = ObjectWrap::Unwrap<FrameDestination>(args[0]->ToObject());
  owt_base::FrameDestination* out = param8->dest;

  me->removeOutput(out);
}

void VideoGstAnalyzerWrap::addOutput(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzerWrap* obj = ObjectWrap::Unwrap<VideoGstAnalyzerWrap>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  FrameDestination* param8 = ObjectWrap::Unwrap<FrameDestination>(args[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameDestination* out = param8->dest;

  bool result = me->addOutput(out);
  args.GetReturnValue().Set(Number::New(isolate, result));
}

void VideoGstAnalyzerWrap::addEventListener(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }
    VideoGstAnalyzerWrap* obj = ObjectWrap::Unwrap<VideoGstAnalyzerWrap>(args.Holder());
    if (!obj->me)
        return;
    Local<Object>::New(isolate, obj->m_store)->Set(args[0], args[1]);
}

