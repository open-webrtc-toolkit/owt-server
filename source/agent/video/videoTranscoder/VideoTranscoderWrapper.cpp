// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "VideoTranscoderWrapper.h"

using namespace v8;

Persistent<Function> VideoTranscoder::constructor;
VideoTranscoder::VideoTranscoder() {};
VideoTranscoder::~VideoTranscoder() {};

void VideoTranscoder::Init(Handle<Object> exports, Handle<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "VideoTranscoder"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setInput", setInput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "unsetInput", unsetInput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addOutput", addOutput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeOutput", removeOutput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "forceKeyFrame", forceKeyFrame);
#ifndef BUILD_FOR_ANALYTICS
  NODE_SET_PROTOTYPE_METHOD(tpl, "drawText", drawText);
  NODE_SET_PROTOTYPE_METHOD(tpl, "clearText", clearText);
#endif

  constructor.Reset(isolate, tpl->GetFunction());
  module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());
}

void VideoTranscoder::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Local<Object> options = args[0]->ToObject();
  mcu::VideoTranscoderConfig config;
  config.useGacc = options->Get(String::NewFromUtf8(isolate, "gaccplugin"))->ToBoolean()->BooleanValue();
  config.MFE_timeout = options->Get(String::NewFromUtf8(isolate, "MFE_timeout"))->Int32Value();

  VideoTranscoder* obj = new VideoTranscoder();
  obj->me = new mcu::VideoTranscoder(config);

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void VideoTranscoder::close(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  obj->me = NULL;

  delete me;
}

void VideoTranscoder::setInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string inStreamID = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string codec = std::string(*param1);
  FrameSource* param2 = ObjectWrap::Unwrap<FrameSource>(args[2]->ToObject());
  owt_base::FrameSource* src = param2->src;

  bool r = me->setInput(inStreamID, codec, src);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void VideoTranscoder::unsetInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string inStreamID = std::string(*param0);

  me->unsetInput(inStreamID);
}

void VideoTranscoder::addOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string outStreamID = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string codec = std::string(*param1);
  String::Utf8Value param2(args[2]->ToString());
  std::string resolution = std::string(*param2);
  unsigned int framerateFPS = args[3]->Uint32Value();
  unsigned int bitrateKbps = args[4]->Uint32Value();
  unsigned int keyFrameIntervalSeconds = args[5]->Uint32Value();
#ifdef BUILD_FOR_ANALYTICS
  String::Utf8Value param6(args[6]->ToString());
  std::string algorithm = std::string(*param6);

  String::Utf8Value param7(args[7]->ToString());
  std::string pluginName = std::string(*param7);

  FrameDestination* param8 = ObjectWrap::Unwrap<FrameDestination>(args[8]->ToObject());
  owt_base::FrameDestination* dest = param8->dest;
#else
  FrameDestination* param6 = ObjectWrap::Unwrap<FrameDestination>(args[6]->ToObject());
  owt_base::FrameDestination* dest = param6->dest;
#endif

  owt_base::VideoCodecProfile profile = owt_base::PROFILE_UNKNOWN;
  if (codec.find("h264") != std::string::npos) {
    if (codec == "h264_cb") {
      profile = owt_base::PROFILE_AVC_CONSTRAINED_BASELINE;
    } else if (codec == "h264_b") {
      profile = owt_base::PROFILE_AVC_BASELINE;
    } else if (codec == "h264_m") {
      profile = owt_base::PROFILE_AVC_MAIN;
    } else if (codec == "h264_e") {
      profile = owt_base::PROFILE_AVC_MAIN;
    } else if (codec == "h264_h") {
      profile = owt_base::PROFILE_AVC_HIGH;
    }
    codec = "h264";
  }

#ifdef BUILD_FOR_ANALYTICS
  bool r = me->addOutput(outStreamID, codec, profile, resolution, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, algorithm, pluginName, dest);
#else
  bool r = me->addOutput(outStreamID, codec, profile, resolution, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest);
#endif

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void VideoTranscoder::removeOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string outStreamID = std::string(*param0);

  me->removeOutput(outStreamID);
}

void VideoTranscoder::forceKeyFrame(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string outStreamID = std::string(*param0);

  me->forceKeyFrame(outStreamID);
}

#ifndef BUILD_FOR_ANALYTICS
void VideoTranscoder::drawText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string textSpec = std::string(*param0);

  me->drawText(textSpec);
}

void VideoTranscoder::clearText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  me->clearText();
}
#endif

