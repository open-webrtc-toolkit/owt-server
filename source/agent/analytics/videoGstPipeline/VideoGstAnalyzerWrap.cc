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

void VideoGstAnalyzerWrap::Init(Local<Object> exports, Local<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(Nan::New("VideoGstAnalyzer").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "createPipeline", createPipeline);
  NODE_SET_PROTOTYPE_METHOD(tpl, "clearPipeline", clearPipeline);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeOutput", removeOutput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addOutput", addOutput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "linkInput", linkInput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addEventListener", addEventListener);

  constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(module, Nan::New("exports").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
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

  Nan::Utf8String param0(Nan::To<v8::String>(args[0]).ToLocalChecked());
  std::string codec = std::string(*param0);

  Nan::Utf8String param1(Nan::To<v8::String>(args[1]).ToLocalChecked());
  std::string resolution = std::string(*param1);
  owt_base::VideoSize vSize{0, 0};
  owt_base::VideoResolutionHelper::getVideoSize(resolution, vSize);
  unsigned int width = vSize.width;
  unsigned int height = vSize.height;

  unsigned int framerateFPS = Nan::To<uint32_t>(args[2]).FromJust();
  unsigned int bitrateKbps = Nan::To<uint32_t>(args[3]).FromJust();
  unsigned int keyFrameIntervalSeconds = Nan::To<uint32_t>(args[4]).FromJust();

  Nan::Utf8String param6(Nan::To<v8::String>(args[5]).ToLocalChecked());
  std::string algorithm = std::string(*param6);

  Nan::Utf8String param7(Nan::To<v8::String>(args[6]).ToLocalChecked());
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

  FrameSource* param8 = ObjectWrap::Unwrap<FrameSource>(
    Nan::To<v8::Object>(args[0]).ToLocalChecked());
  owt_base::FrameSource* src = param8->src;

  bool result = me->linkInput(src);
  args.GetReturnValue().Set(Number::New(isolate, result));
}

void VideoGstAnalyzerWrap::removeOutput(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzerWrap* obj = ObjectWrap::Unwrap<VideoGstAnalyzerWrap>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  FrameDestination* param8 = ObjectWrap::Unwrap<FrameDestination>(
    Nan::To<v8::Object>(args[0]).ToLocalChecked());
  owt_base::FrameDestination* out = param8->dest;

  me->removeOutput(out);
}

void VideoGstAnalyzerWrap::addOutput(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzerWrap* obj = ObjectWrap::Unwrap<VideoGstAnalyzerWrap>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  FrameDestination* param8 = ObjectWrap::Unwrap<FrameDestination>(
    Nan::To<v8::Object>(args[0]).ToLocalChecked());
  owt_base::FrameDestination* out = param8->dest;

  bool result = me->addOutput(out);
  args.GetReturnValue().Set(Number::New(isolate, result));
}

void VideoGstAnalyzerWrap::addEventListener(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsFunction()) {
        Nan::ThrowError("Wrong arguments");
        return;
    }
    VideoGstAnalyzerWrap* obj = ObjectWrap::Unwrap<VideoGstAnalyzerWrap>(args.Holder());
    if (!obj->me)
        return;
    Nan::Set(Local<Object>::New(isolate, obj->m_store),
             args[0], args[1]);
}

