// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "VideoMixerWrapper.h"
#include "VideoLayout.h"

using namespace v8;

static std::string getString(v8::Local<v8::Value> value) {
  Nan::Utf8String value_str(Nan::To<v8::String>(value).ToLocalChecked());
  return std::string(*value_str);
}

static Local<Value> nanGetChecked(v8::Local<v8::Object> obj, std::string key) {
  return Nan::Get(obj, Nan::New(key).ToLocalChecked()).ToLocalChecked();
}

static Local<Value> nanGetChecked(v8::Local<v8::Object> obj, uint32_t index) {
  return Nan::Get(obj, index).ToLocalChecked();
}

static mcu::Rational parseRational(Isolate* isolate, Local<Value> rational) {
  mcu::Rational ret = { 0, 1 };
  if (rational->IsObject()) {
    Local<Object> obj = Nan::To<v8::Object>(rational).ToLocalChecked();
    ret.numerator = Nan::To<int32_t>(nanGetChecked(obj, "numerator")).FromJust();
    ret.denominator = Nan::To<int32_t>(nanGetChecked(obj, "denominator")).FromJust();
  }
  return ret;
}

Persistent<Function> VideoMixer::constructor;
VideoMixer::VideoMixer() {};
VideoMixer::~VideoMixer() {};

void VideoMixer::Init(Local<Object> exports, Local<Object> module) {
  Isolate* isolate = exports->GetIsolate();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(Nan::New("VideoMixer").ToLocalChecked());
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
  NODE_SET_PROTOTYPE_METHOD(tpl, "drawText", drawText);
  NODE_SET_PROTOTYPE_METHOD(tpl, "clearText", clearText);

  constructor.Reset(isolate, Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(module, Nan::New("exports").ToLocalChecked(),
           Nan::GetFunction(tpl).ToLocalChecked());
}

void VideoMixer::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Local<Object> options = Nan::To<v8::Object>(args[0]).ToLocalChecked();
  mcu::VideoMixerConfig config;

  config.maxInput = Nan::To<int32_t>(nanGetChecked(options, "maxinput")).FromJust();
  config.crop = Nan::To<bool>(nanGetChecked(options, "crop")).FromJust();
  Local<Value> resolution = nanGetChecked(options, "resolution");

  if (resolution->IsString()) {
    config.resolution = getString(resolution);
  }
  Local<Value> background = nanGetChecked(options, "backgroundcolor");

  if (background->IsObject()) {
    Local<Object> colorObj = Nan::To<v8::Object>(background).ToLocalChecked();
    config.bgColor.r = Nan::To<int32_t>(nanGetChecked(colorObj, "r")).FromJust();
    config.bgColor.g = Nan::To<int32_t>(nanGetChecked(colorObj, "g")).FromJust();
    config.bgColor.b = Nan::To<int32_t>(nanGetChecked(colorObj, "b")).FromJust();
  }
  config.useGacc = Nan::To<bool>(nanGetChecked(options, "gaccplugin")).FromJust();
  config.MFE_timeout = Nan::To<int32_t>(nanGetChecked(options, "MFE_timeout")).FromJust();

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

  int inputIndex = Nan::To<int32_t>(args[0]).FromJust();
  std::string codec = getString(args[1]);
  FrameSource* param2 = ObjectWrap::Unwrap<FrameSource>(
    Nan::To<v8::Object>(args[2]).ToLocalChecked());
  owt_base::FrameSource* src = param2->src;

  // Set avatar data
  std::string avatarData = getString(args[3]);

  int r = me->addInput(inputIndex, codec, src, avatarData);

  args.GetReturnValue().Set(Number::New(isolate, r));
}

void VideoMixer::removeInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  int inputIndex = Nan::To<int32_t>(args[0]).FromJust();
  me->removeInput(inputIndex);
}

void VideoMixer::setInputActive(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  int inputIndex = Nan::To<int32_t>(args[0]).FromJust();
  bool active = Nan::To<bool>(args[1]).FromJust();

  me->setInputActive(inputIndex, active);
}

void VideoMixer::addOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  std::string outStreamID = getString(args[0]);
  std::string codec = getString(args[1]);
  std::string resolution = getString(args[2]);
  unsigned int framerateFPS = Nan::To<uint32_t>(args[3]).FromJust();
  unsigned int bitrateKbps = Nan::To<uint32_t>(args[4]).FromJust();
  unsigned int keyFrameIntervalSeconds = Nan::To<uint32_t>(args[5]).FromJust();
  FrameDestination* param6 = ObjectWrap::Unwrap<FrameDestination>(
    Nan::To<v8::Object>(args[6]).ToLocalChecked());
  owt_base::FrameDestination* dest = param6->dest;

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
  bool r = me->addOutput(outStreamID, codec, profile, resolution, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void VideoMixer::removeOutput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  std::string outStreamID = getString(args[0]);

  me->removeOutput(outStreamID);
}

void VideoMixer::updateLayoutSolution(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  if (args.Length() < 1 || !args[0]->IsObject()) {
    args.GetReturnValue().Set(Boolean::New(isolate, false));
    return;
  }

  Local<Object> jsSolution = Nan::To<v8::Object>(args[0]).ToLocalChecked();
  if (jsSolution->IsArray()) {
    mcu::LayoutSolution solution;
    int length = Nan::To<int32_t>(nanGetChecked(jsSolution, "length")).FromMaybe(0);
    for (int i = 0; i < length; i++) {
      Local<Value> item = nanGetChecked(jsSolution, i);
      if (!item->IsObject())
        continue;
      Local<Object> jsInputRegion = Nan::To<v8::Object>(item).ToLocalChecked();
      int input = Nan::To<int32_t>(nanGetChecked(jsInputRegion, "input")).FromJust();
      Local<Object> regObj = Nan::To<v8::Object>(
        nanGetChecked(jsInputRegion, "region")).ToLocalChecked();

      mcu::Region region;
      region.id = getString(nanGetChecked(regObj, "id"));

      Local<Value> area = nanGetChecked(regObj, "area");
      if (area->IsObject()) {
        Local<Object> areaObj = Nan::To<v8::Object>(area).ToLocalChecked();
        region.shape = getString(nanGetChecked(regObj, "shape"));
        if (region.shape == "rectangle") {
          region.area.rect.left = parseRational(isolate, nanGetChecked(areaObj, "left"));
          region.area.rect.top = parseRational(isolate, nanGetChecked(areaObj, "top"));
          region.area.rect.width = parseRational(isolate, nanGetChecked(areaObj, "width"));
          region.area.rect.height = parseRational(isolate, nanGetChecked(areaObj, "height"));
        } else if (region.shape == "circle") {
          region.area.circle.centerW = parseRational(isolate, nanGetChecked(areaObj, "centerW"));
          region.area.circle.centerH = parseRational(isolate, nanGetChecked(areaObj, "centerH"));
          region.area.circle.radius = parseRational(isolate, nanGetChecked(areaObj, "radius"));
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

  std::string outStreamID = getString(args[0]);

  me->forceKeyFrame(outStreamID);
}

void VideoMixer::drawText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  std::string textSpec = getString(args[0]);

  me->drawText(textSpec);
}

void VideoMixer::clearText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  me->clearText();
}

