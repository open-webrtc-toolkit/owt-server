// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "VideoSwitchWrapper.h"

using namespace v8;

Nan::Persistent<Function> VideoSwitch::constructor;

VideoSwitch::VideoSwitch() {}
VideoSwitch::~VideoSwitch() {}

NAN_MODULE_INIT(VideoSwitch::Init) {
  // Constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("VideoSwitch").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "close", close);
  Nan::SetPrototypeMethod(tpl, "setTargetBitrate", setTargetBitrate);
  Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
  Nan::SetPrototypeMethod(tpl, "removeDestination", removeDestination);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("VideoSwitch").ToLocalChecked(),
           Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(VideoSwitch::New) {
  if (info.IsConstructCall()) {
    std::vector<owt_base::FrameSource*> sources;
    for (int i = 0; i < info.Length(); i++) {
      FrameSource* param = node::ObjectWrap::Unwrap<FrameSource>(
        info[i]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
      owt_base::FrameSource* src = param->src;
      sources.push_back(src);
    }

    VideoSwitch* obj = new VideoSwitch();
    obj->me.reset(new owt_base::VideoQualitySwitch(sources));
    obj->src = obj->me.get();

    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    // ELOG_WARN("Not construct call");
  }
}

NAN_METHOD(VideoSwitch::close) {
  VideoSwitch* obj = ObjectWrap::Unwrap<VideoSwitch>(info.Holder());
  obj->src = nullptr;
  obj->me.reset();
}


NAN_METHOD(VideoSwitch::setTargetBitrate) {
  VideoSwitch* obj = ObjectWrap::Unwrap<VideoSwitch>(info.Holder());
  unsigned int bitrate = Nan::To<unsigned int>(info[0]).FromJust();
  obj->me->setTargetBitrate(bitrate);
}

NAN_METHOD(VideoSwitch::addDestination) {
  VideoSwitch* obj = ObjectWrap::Unwrap<VideoSwitch>(info.Holder());

  Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string track = std::string(*param0);

  FrameDestination* param =
    ObjectWrap::Unwrap<FrameDestination>(
      info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    obj->me->addAudioDestination(dest);
  } else if (track == "video") {
    obj->me->addVideoDestination(dest);
  }
}

NAN_METHOD(VideoSwitch::removeDestination) {
  VideoSwitch* obj = ObjectWrap::Unwrap<VideoSwitch>(info.Holder());

  Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string track = std::string(*param0);

  FrameDestination* param =
    ObjectWrap::Unwrap<FrameDestination>(
      info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
  owt_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    obj->me->removeAudioDestination(dest);
  } else if (track == "video") {
    obj->me->removeVideoDestination(dest);
  }
}