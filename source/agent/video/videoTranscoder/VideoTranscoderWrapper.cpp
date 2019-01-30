/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

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
  woogeen_base::FrameSource* src = param2->src;

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
  FrameDestination* param6 = ObjectWrap::Unwrap<FrameDestination>(args[6]->ToObject());
  woogeen_base::FrameDestination* dest = param6->dest;

  woogeen_base::VideoCodecProfile profile = woogeen_base::PROFILE_UNKNOWN;
  if (codec.find("h264") != std::string::npos) {
    if (codec == "h264_cb") {
      profile = woogeen_base::PROFILE_AVC_CONSTRAINED_BASELINE;
    } else if (codec == "h264_b") {
      profile = woogeen_base::PROFILE_AVC_BASELINE;
    } else if (codec == "h264_m") {
      profile = woogeen_base::PROFILE_AVC_MAIN;
    } else if (codec == "h264_e") {
      profile = woogeen_base::PROFILE_AVC_MAIN;
    } else if (codec == "h264_h") {
      profile = woogeen_base::PROFILE_AVC_HIGH;
    }
    codec = "h264";
  }
  bool r = me->addOutput(outStreamID, codec, profile, resolution, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest);

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

