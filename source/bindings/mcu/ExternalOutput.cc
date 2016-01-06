/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#include "ExternalOutput.h"
#include "Mixer.h"
#include "../woogeen_base/Gateway.h"

#include <Gateway.h>

using namespace v8;

Persistent<Function> ExternalOutput::constructor;
ExternalOutput::ExternalOutput() {};
ExternalOutput::~ExternalOutput() {};

void ExternalOutput::Init(Local<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "ExternalOutput"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  SETUP_EVENTED_PROTOTYPE_METHODS(tpl);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setMediaSource", setMediaSource);
  NODE_SET_PROTOTYPE_METHOD(tpl, "unsetMediaSource", unsetMediaSource);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "ExternalOutput"), tpl->GetFunction());
}

void ExternalOutput::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  v8::String::Utf8Value param(args[0]->ToString());
  std::string configParam = std::string(*param);

  ExternalOutput* obj = new ExternalOutput();
  obj->me = woogeen_base::MediaMuxer::createMediaMuxerInstance(configParam);
  obj->me->setEventRegistry(obj);
  obj->Wrap(args.This());

  args.GetReturnValue().Set(args.This());
}

void ExternalOutput::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  // FIXME: Possibly, some closing events can be added here.
}

void ExternalOutput::setMediaSource(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ExternalOutput* obj = ObjectWrap::Unwrap<ExternalOutput>(args.This());
  woogeen_base::MediaMuxer* me = (woogeen_base::MediaMuxer*)obj->me;

  woogeen_base::Gateway* videoSource = nullptr;
  Gateway* gateway0 = ObjectWrap::Unwrap<Gateway>(args[0]->ToObject());
  if (gateway0) {
    videoSource = gateway0->me;
  } else {
    Mixer* mixer0  = ObjectWrap::Unwrap<Mixer>(args[0]->ToObject());
    videoSource = mixer0->me;
  }

  woogeen_base::Gateway* audioSource = nullptr;
  Gateway* gateway1 = ObjectWrap::Unwrap<Gateway>(args[1]->ToObject());
  if (gateway1) {
    audioSource = gateway1->me;
  } else {
    Mixer* mixer1 = ObjectWrap::Unwrap<Mixer>(args[1]->ToObject());
    audioSource = mixer1->me;
  }

  String::Utf8Value param2(args[2]->ToString());
  std::string preferredVideoCodec = std::string(*param2);

  String::Utf8Value param3(args[3]->ToString());
  std::string preferredAudioCodec = std::string(*param3);

  me->setMediaSource(videoSource->getVideoFrameProvider(), audioSource->getAudioFrameProvider(), preferredVideoCodec, preferredAudioCodec);
}

void ExternalOutput::unsetMediaSource(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ExternalOutput* obj = ObjectWrap::Unwrap<ExternalOutput>(args.This());
  woogeen_base::MediaMuxer* me = obj->me;

  me->unsetMediaSource();

  bool succeeded = true;
  bool close = (args[1]->ToBoolean())->BooleanValue();
  if (close) {
    String::Utf8Value param0(args[0]->ToString());
    std::string outputId = std::string(*param0);
    succeeded = woogeen_base::MediaMuxer::recycleMediaMuxerInstance(outputId);
  }

  args.GetReturnValue().Set(Boolean::New(isolate, succeeded));
}
