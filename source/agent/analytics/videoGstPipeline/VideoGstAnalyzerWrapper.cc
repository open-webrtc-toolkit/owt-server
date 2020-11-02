// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "VideoGstAnalyzerWrapper.h"
#include "VideoHelper.h"
#include <iostream>

using namespace v8;

Persistent<Function> VideoGstAnalyzer::constructor;
VideoGstAnalyzer::VideoGstAnalyzer() {};
VideoGstAnalyzer::~VideoGstAnalyzer() {};

void VideoGstAnalyzer::Init(Handle<Object> exports, Handle<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "VideoGstAnalyzer"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getListeningPort", getListeningPort);
  NODE_SET_PROTOTYPE_METHOD(tpl, "createPipeline", createPipeline);
  NODE_SET_PROTOTYPE_METHOD(tpl, "clearPipeline", clearPipeline);
  NODE_SET_PROTOTYPE_METHOD(tpl, "emitListenTo", emitListenTo);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addElementMany", addElementMany);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setPlaying", setPlaying);
  NODE_SET_PROTOTYPE_METHOD(tpl, "stopLoop", stopLoop);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setOutputParam", setOutputParam);
  NODE_SET_PROTOTYPE_METHOD(tpl, "disconnect", disconnect);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addOutput", addOutput);

  constructor.Reset(isolate, tpl->GetFunction());
  module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());
}

void VideoGstAnalyzer::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoGstAnalyzer* obj = new VideoGstAnalyzer();
  obj->me = new mcu::VideoGstAnalyzer();

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void VideoGstAnalyzer::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;
  delete me;
}

void VideoGstAnalyzer::getListeningPort(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  uint32_t port = me->getListeningPort();

  args.GetReturnValue().Set(Number::New(isolate, port));
}


void VideoGstAnalyzer::createPipeline(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  int result = me->createPipeline();

  args.GetReturnValue().Set(Number::New(isolate, result));
}

void VideoGstAnalyzer::clearPipeline(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  me->clearPipeline();
}

void VideoGstAnalyzer::emitListenTo(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  unsigned int minPort = 0, maxPort = 0;

    minPort = args[0]->Uint32Value();
    maxPort = args[1]->Uint32Value();

  me->emitListenTo(minPort,maxPort);
}


void VideoGstAnalyzer::addElementMany(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;


  int result = me->addElementMany();

  args.GetReturnValue().Set(Number::New(isolate, result));
}

void VideoGstAnalyzer::setPlaying(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;
  
  me->setPlaying();
}

void VideoGstAnalyzer::stopLoop(const v8::FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  me->stopLoop();
}

void VideoGstAnalyzer::disconnect(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  FrameDestination* param8 = ObjectWrap::Unwrap<FrameDestination>(args[0]->ToObject());
  owt_base::FrameDestination* out = param8->dest;

  me->disconnect(out);
}

void VideoGstAnalyzer::setOutputParam(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
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

  me->setOutputParam(codec,width,height,framerateFPS,bitrateKbps,
                       keyFrameIntervalSeconds,algorithm,pluginName);
}

void VideoGstAnalyzer::addOutput(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoGstAnalyzer* obj = ObjectWrap::Unwrap<VideoGstAnalyzer>(args.Holder());
  mcu::VideoGstAnalyzer* me = obj->me;

  unsigned int index = 0;

  index = args[0]->Uint32Value();
  FrameDestination* param8 = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
  owt_base::FrameDestination* out = param8->dest;

  me->addOutput(index, out);
}
