// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "VideoTranscoderWrapper.h"

using namespace v8;

static std::string getString(v8::Local<v8::Value> value) {
  Nan::Utf8String value_str(Nan::To<v8::String>(value).ToLocalChecked());
  return std::string(*value_str);
}

Persistent<Function> VideoTranscoder::constructor;
VideoTranscoder::VideoTranscoder() {};
VideoTranscoder::~VideoTranscoder() {};

void VideoTranscoder::Init(Local<Object> exports, Local<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(Nan::New("VideoTranscoder").ToLocalChecked());
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

  constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(module, Nan::New("exports").ToLocalChecked(),
           Nan::GetFunction(tpl).ToLocalChecked());
}

void VideoTranscoder::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Local<Object> options = Nan::To<v8::Object>(args[0]).ToLocalChecked();
  mcu::VideoTranscoderConfig config;
  config.useGacc = Nan::To<bool>(
    Nan::Get(options, Nan::New("gaccplugin").ToLocalChecked()).ToLocalChecked()).FromJust();
  config.MFE_timeout = Nan::To<int32_t>(
    Nan::Get(options, Nan::New("MFE_timeout").ToLocalChecked()).ToLocalChecked()).FromJust();

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

  std::string inStreamID = getString(args[0]);
  std::string codec = getString(args[1]);
  FrameSource* param2 = ObjectWrap::Unwrap<FrameSource>(
    Nan::To<v8::Object>(args[2]).ToLocalChecked());
  owt_base::FrameSource* src = param2->src;

  bool r = me->setInput(inStreamID, codec, src);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void VideoTranscoder::unsetInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  std::string inStreamID = getString(args[0]);

  me->unsetInput(inStreamID);
}

void VideoTranscoder::addOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  std::string outStreamID = getString(args[0]);
  std::string codec = getString(args[1]);
  std::string resolution = getString(args[2]);
  unsigned int framerateFPS = Nan::To<uint32_t>(args[3]).FromJust();
  unsigned int bitrateKbps = Nan::To<uint32_t>(args[4]).FromJust();
  unsigned int keyFrameIntervalSeconds = Nan::To<uint32_t>(args[5]).FromJust();
#ifdef BUILD_FOR_ANALYTICS
  std::string algorithm = getString(args[6]);
  std::string pluginName = getString(args[7]);

  FrameDestination* param8 = ObjectWrap::Unwrap<FrameDestination>(
    Nan::To<v8::Object>(args[8]).ToLocalChecked());
  owt_base::FrameDestination* dest = param8->dest;
#else
  FrameDestination* param6 = ObjectWrap::Unwrap<FrameDestination>(
    Nan::To<v8::Object>(args[6]).ToLocalChecked());
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

  std::string outStreamID = getString(args[0]);

  me->removeOutput(outStreamID);
}

void VideoTranscoder::forceKeyFrame(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  std::string outStreamID = getString(args[0]);

  me->forceKeyFrame(outStreamID);
}

#ifndef BUILD_FOR_ANALYTICS
void VideoTranscoder::drawText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoTranscoder* obj = ObjectWrap::Unwrap<VideoTranscoder>(args.Holder());
  mcu::VideoTranscoder* me = obj->me;

  std::string textSpec = getString(args[0]);

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

