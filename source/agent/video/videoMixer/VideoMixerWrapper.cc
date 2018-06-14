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

#include "VideoMixerWrapper.h"
#include "VideoLayout.h"

using namespace v8;

Persistent<Function> VideoMixer::constructor;
VideoMixer::VideoMixer() {};
VideoMixer::~VideoMixer() {};

void VideoMixer::Init(Handle<Object> exports, Handle<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "VideoMixer"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addInput", addInput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeInput", removeInput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setInputActive", setInputActive);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addOutput", addOutput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeOutput", removeOutput);
  NODE_SET_PROTOTYPE_METHOD(tpl, "updateLayoutSolution", updateLayoutSolution);
  NODE_SET_PROTOTYPE_METHOD(tpl, "forceKeyFrame", forceKeyFrame);

  constructor.Reset(isolate, tpl->GetFunction());
  module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());
}

void VideoMixer::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Local<Object> options = args[0]->ToObject();
  mcu::VideoMixerConfig config;

  config.maxInput = options->Get(String::NewFromUtf8(isolate, "maxinput"))->Int32Value();
  config.crop = options->Get(String::NewFromUtf8(isolate, "crop"))->ToBoolean()->BooleanValue();

  Local<Value> resolution = options->Get(String::NewFromUtf8(isolate, "resolution"));
  if (resolution->IsString()) {
    config.resolution = std::string(*String::Utf8Value(resolution->ToString()));
  }

  Local<Value> background = options->Get(String::NewFromUtf8(isolate, "backgroundcolor"));
  if (background->IsObject()) {
    Local<Object> colorObj = background->ToObject();
    config.bgColor.r = colorObj->Get(String::NewFromUtf8(isolate, "r"))->Int32Value();
    config.bgColor.g = colorObj->Get(String::NewFromUtf8(isolate, "g"))->Int32Value();
    config.bgColor.b = colorObj->Get(String::NewFromUtf8(isolate, "b"))->Int32Value();
  }
  config.useGacc = options->Get(String::NewFromUtf8(isolate, "gaccplugin"))->ToBoolean()->BooleanValue();
  config.MFE_timeout = options->Get(String::NewFromUtf8(isolate, "MFE_timeout"))->Int32Value();

  VideoMixer* obj = new VideoMixer();
  obj->me = new mcu::VideoMixer(config);

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void VideoMixer::close(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  obj->me = NULL;

  delete me;
}

void VideoMixer::addInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  int inputIndex = args[0]->Int32Value();
  String::Utf8Value param1(args[1]->ToString());
  std::string codec = std::string(*param1);
  FrameSource* param2 = ObjectWrap::Unwrap<FrameSource>(args[2]->ToObject());
  woogeen_base::FrameSource* src = param2->src;

  // Set avatar data
  String::Utf8Value param3(args[3]->ToString());
  std::string avatarData = std::string(*param3);

  int r = me->addInput(inputIndex, codec, src, avatarData);

  args.GetReturnValue().Set(Number::New(isolate, r));
}

void VideoMixer::removeInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  int inputIndex = args[0]->Int32Value();
  me->removeInput(inputIndex);
}

void VideoMixer::setInputActive(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  int inputIndex = args[0]->Int32Value();
  bool active = args[1]->ToBoolean()->Value();

  me->setInputActive(inputIndex, active);
}

void VideoMixer::addOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

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

  bool r = me->addOutput(outStreamID, codec, resolution, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void VideoMixer::removeOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string outStreamID = std::string(*param0);

  me->removeOutput(outStreamID);
}

mcu::Rational parseRational(Isolate* isolate, Local<Value> rational) {
  mcu::Rational ret = { 0, 1 };
  if (rational->IsObject()) {
    Local<Object> obj = rational->ToObject();
    ret.numerator = obj->Get(String::NewFromUtf8(isolate, "numerator"))->Int32Value();
    ret.denominator = obj->Get(String::NewFromUtf8(isolate, "denominator"))->Int32Value();
  }
  return ret;
}

void VideoMixer::updateLayoutSolution(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  if (args.Length() < 1 || !args[0]->IsObject()) {
    args.GetReturnValue().Set(Boolean::New(isolate, false));
    return;
  }

  Local<Object> jsSolution = args[0]->ToObject();
  if (jsSolution->IsArray()) {
    mcu::LayoutSolution solution;
    int length = jsSolution->Get(String::NewFromUtf8(isolate, "length"))->ToObject()->Uint32Value();
    for (int i = 0; i < length; i++) {
      if (!jsSolution->Get(i)->IsObject())
        continue;
      Local<Object> jsInputRegion = jsSolution->Get(i)->ToObject();
      int input = jsInputRegion->Get(String::NewFromUtf8(isolate, "input"))->NumberValue();
      Local<Object> regObj = jsInputRegion->Get(String::NewFromUtf8(isolate, "region"))->ToObject();

      mcu::Region region;
      region.id = *String::Utf8Value(regObj->Get(String::NewFromUtf8(isolate, "id")));

      Local<Value> area = regObj->Get(String::NewFromUtf8(isolate, "area"));
      if (area->IsObject()) {
        Local<Object> areaObj = area->ToObject();
        region.shape = *String::Utf8Value(regObj->Get(String::NewFromUtf8(isolate, "shape")));
        if (region.shape == "rectangle") {
          region.area.rect.left = parseRational(isolate, areaObj->Get(String::NewFromUtf8(isolate, "left")));
          region.area.rect.top = parseRational(isolate, areaObj->Get(String::NewFromUtf8(isolate, "top")));
          region.area.rect.width = parseRational(isolate, areaObj->Get(String::NewFromUtf8(isolate, "width")));
          region.area.rect.height = parseRational(isolate, areaObj->Get(String::NewFromUtf8(isolate, "height")));
        } else if (region.shape == "circle") {
          region.area.circle.centerW = parseRational(isolate, areaObj->Get(String::NewFromUtf8(isolate, "centerW")));
          region.area.circle.centerH = parseRational(isolate, areaObj->Get(String::NewFromUtf8(isolate, "centerH")));
          region.area.circle.radius = parseRational(isolate, areaObj->Get(String::NewFromUtf8(isolate, "radius")));
        }
      }

      mcu::InputRegion inputRegion = { input, region };
      solution.push_back(inputRegion);
    }
    me->updateLayoutSolution(solution);
    args.GetReturnValue().Set(Boolean::New(isolate, true));
  } else {
    args.GetReturnValue().Set(Boolean::New(isolate, false));
  }
}

void VideoMixer::forceKeyFrame(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string outStreamID = std::string(*param0);

  me->forceKeyFrame(outStreamID);
}

