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

  constructor.Reset(isolate, tpl->GetFunction());
  module->Set(String::NewFromUtf8(isolate, "exports"), tpl->GetFunction());
}

void VideoMixer::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param0(args[0]->ToString());
  std::string config = std::string(*param0);

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
  unsigned int param3 = args[3]->Uint32Value();
  woogeen_base::QualityLevel quality = static_cast<woogeen_base::QualityLevel>(param3);
  FrameDestination* param4 = ObjectWrap::Unwrap<FrameDestination>(args[4]->ToObject());
  woogeen_base::FrameDestination* dest = param4->dest;

  bool r = me->addOutput(outStreamID, codec, resolution, quality, dest);

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
      Local<Object> jsInputRegion = jsSolution->Get(i)->ToObject();
      int input = jsInputRegion->Get(String::NewFromUtf8(isolate, "input"))->NumberValue();
      Local<Object> regObj = jsInputRegion->Get(String::NewFromUtf8(isolate, "region"))->ToObject();

      mcu::Region region;
      String::Utf8Value s(regObj->Get(String::NewFromUtf8(isolate, "id"))->ToString());
      region.id = std::string(*s);
      region.left = regObj->Get(String::NewFromUtf8(isolate, "left"))->NumberValue();
      region.top = regObj->Get(String::NewFromUtf8(isolate, "top"))->NumberValue();
      region.relativeSize = regObj->Get(String::NewFromUtf8(isolate, "relativeSize"))->NumberValue();


      mcu::InputRegion inputRegion = { input, region };
      solution.push_back(inputRegion);
    }
    me->updateLayoutSolution(solution);
    args.GetReturnValue().Set(Boolean::New(isolate, true));
  } else {
    args.GetReturnValue().Set(Boolean::New(isolate, false));
  }
}
