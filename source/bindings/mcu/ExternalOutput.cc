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

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "ExternalOutput.h"

#include "../woogeen_base/Gateway.h"
#include "../woogeen_base/NodeEventRegistry.h"
#include "Mixer.h"

#include <Gateway.h>

using namespace v8;

ExternalOutput::ExternalOutput() {};
ExternalOutput::~ExternalOutput() {};

void ExternalOutput::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("ExternalOutput"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("close"), FunctionTemplate::New(close)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setMediaSource"), FunctionTemplate::New(setMediaSource)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("unsetMediaSource"), FunctionTemplate::New(unsetMediaSource)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("ExternalOutput"), constructor);
}

Handle<Value> ExternalOutput::New(const Arguments& args) {
  HandleScope scope;

  v8::String::Utf8Value param(args[0]->ToString());
  std::string configParam = std::string(*param);
  NodeEventRegistry* callback = nullptr;
  if (args.Length() > 1 && args[1]->IsFunction()) {
    Persistent<Function> cb = Persistent<Function>::New(Local<Function>::Cast(args[1]));
    callback = new NodeEventRegistry(cb);
  }

  ExternalOutput* obj = new ExternalOutput();
  obj->me = woogeen_base::MediaMuxer::createMediaMuxerInstance(configParam, callback);

  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> ExternalOutput::close(const Arguments& args) {
  HandleScope scope;

  // FIXME: Possibly, some closing events can be added here.

  return scope.Close(Null());
}

Handle<Value> ExternalOutput::setMediaSource(const Arguments& args) {
  HandleScope scope;

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

  me->setMediaSource(videoSource->getVideoFrameProvider(), audioSource->getAudioFrameProvider());

  return scope.Close(Null());
}

Handle<Value> ExternalOutput::unsetMediaSource(const Arguments& args) {
  HandleScope scope;

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

  return scope.Close(Boolean::New(succeeded));
}
