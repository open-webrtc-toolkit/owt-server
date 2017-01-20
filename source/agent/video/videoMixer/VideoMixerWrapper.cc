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
  NODE_SET_PROTOTYPE_METHOD(tpl, "setRegion", setRegion);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getRegion", getRegion);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setPrimary", setPrimary);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getCurrentRegions", getCurrentRegions);

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

  String::Utf8Value param0(args[0]->ToString());
  std::string inStreamID = std::string(*param0);
  String::Utf8Value param1(args[1]->ToString());
  std::string codec = std::string(*param1);
  FrameSource* param2 = ObjectWrap::Unwrap<FrameSource>(args[2]->ToObject());
  woogeen_base::FrameSource* src = param2->src;

  // Set avatar data
  String::Utf8Value param3(args[3]->ToString());
  std::string avatarData = std::string(*param3);

  bool r = me->addInput(inStreamID, codec, src, avatarData);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void VideoMixer::removeInput(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string inStreamID = std::string(*param0);

  me->removeInput(inStreamID);
}

void VideoMixer::setInputActive(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string inStreamID = std::string(*param0);

  bool active = args[1]->ToBoolean()->Value();

  me->setInputActive(inStreamID, active);
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

void VideoMixer::setRegion(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string inStreamID = std::string(*param0);

  String::Utf8Value param1(args[1]->ToString());
  std::string regionID = std::string(*param1);

  bool r = me->setRegion(inStreamID, regionID);

  args.GetReturnValue().Set(Boolean::New(isolate, r));
}

void VideoMixer::getRegion(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string inStreamID = std::string(*param0);

  std::string regionID = me->getRegion(inStreamID);

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, regionID.c_str()));
}

void VideoMixer::setPrimary(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  String::Utf8Value param0(args[0]->ToString());
  std::string inStreamID = std::string(*param0);

  me->setPrimary(inStreamID);
}

void VideoMixer::getCurrentRegions(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();

  VideoMixer* obj = ObjectWrap::Unwrap<VideoMixer>(args.Holder());
  mcu::VideoMixer* me = obj->me;

  boost::shared_ptr<std::map<std::string, mcu::Region>> currentRegions = me->getCurrentRegions();
  if (currentRegions && currentRegions->size()) {
    // Construct a js array
    Local<Array> regionArray = Array::New(isolate, currentRegions->size());

    int arrayPos = 0;
    for (std::map<std::string, mcu::Region>::iterator it = currentRegions->begin(); it != currentRegions->end(); it++) {
      Local<Object> region = Object::New(isolate);
      region->Set(String::NewFromUtf8(isolate, "streamID"), String::NewFromUtf8(isolate, it->first.c_str()));
      region->Set(String::NewFromUtf8(isolate, "id"), String::NewFromUtf8(isolate, it->second.id.c_str()));
      region->Set(String::NewFromUtf8(isolate, "left"), Number::New(isolate, it->second.left));
      region->Set(String::NewFromUtf8(isolate, "top"), Number::New(isolate, it->second.top));
      region->Set(String::NewFromUtf8(isolate, "relativeSize"), Number::New(isolate, it->second.relativeSize));

      regionArray->Set(arrayPos, region);
      arrayPos++;
    }

    args.GetReturnValue().Set(regionArray);
  } else {
    args.GetReturnValue().Set(v8::Null(isolate));
  }
}
