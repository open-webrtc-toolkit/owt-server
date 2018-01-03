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

#include "AudioFrameConstructorWrapper.h"
#include "../../addons/common/MediaFramePipelineWrapper.h"

using namespace v8;

Nan::Persistent<Function> AudioFrameConstructor::constructor;

AudioFrameConstructor::AudioFrameConstructor() {}
AudioFrameConstructor::~AudioFrameConstructor() {}

NAN_MODULE_INIT(AudioFrameConstructor::Init) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("AudioFrameConstructor").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "close", close);
  Nan::SetPrototypeMethod(tpl, "bindTransport", bindTransport);
  Nan::SetPrototypeMethod(tpl, "unbindTransport", unbindTransport);
  Nan::SetPrototypeMethod(tpl, "enable", enable);
  Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
  Nan::SetPrototypeMethod(tpl, "removeDestination", removeDestination);

  constructor.Reset(tpl->GetFunction());
  Nan::Set(target, Nan::New("AudioFrameConstructor").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(AudioFrameConstructor::New) {
  if (info.IsConstructCall()) {
    AudioFrameConstructor* obj = new AudioFrameConstructor();
    obj->me = new woogeen_base::AudioFrameConstructor();
    obj->src = obj->me;
    obj->msink = obj->me;

    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    // const int argc = 1;
    // v8::Local<v8::Value> argv[argc] = {info[0]};
    // v8::Local<v8::Function> cons = Nan::New(constructor);
    // info.GetReturnValue().Set(cons->NewInstance(argc, argv));
  }
}

NAN_METHOD(AudioFrameConstructor::close) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;
  delete me;
}

NAN_METHOD(AudioFrameConstructor::bindTransport) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;

  WebRtcConnection* param = Nan::ObjectWrap::Unwrap<WebRtcConnection>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  auto wr = std::shared_ptr<erizo::WebRtcConnection>(param->me).get();

  //MediaSink* param = Nan::ObjectWrap::Unwrap<MediaSink>(args[0]->ToObject());
  erizo::MediaSource* source = wr;
  me->bindTransport(source, source->getFeedbackSink());
}

NAN_METHOD(AudioFrameConstructor::unbindTransport) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;

  me->unbindTransport();
}

NAN_METHOD(AudioFrameConstructor::addDestination) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;

  FrameDestination* param = node::ObjectWrap::Unwrap<FrameDestination>(info[0]->ToObject());
  woogeen_base::FrameDestination* dest = param->dest;

  me->addAudioDestination(dest);
}

NAN_METHOD(AudioFrameConstructor::removeDestination) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;

  FrameDestination* param = node::ObjectWrap::Unwrap<FrameDestination>(info[0]->ToObject());
  woogeen_base::FrameDestination* dest = param->dest;

  me->removeAudioDestination(dest);
}

NAN_METHOD(AudioFrameConstructor::enable) {
  AudioFrameConstructor* obj = Nan::ObjectWrap::Unwrap<AudioFrameConstructor>(info.Holder());
  woogeen_base::AudioFrameConstructor* me = obj->me;

  bool b = (info[0]->ToBoolean())->BooleanValue();
  me->enable(b);
}

