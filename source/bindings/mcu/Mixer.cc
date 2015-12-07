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

#include "Mixer.h"
#include "../erizoAPI/MediaDefinitions.h"
#include "../erizoAPI/WebRtcConnection.h"
#include "../woogeen_base/ExternalInput.h"

using namespace v8;

Persistent<Function> Mixer::constructor;
Mixer::Mixer() {};
Mixer::~Mixer() {};

void Mixer::Init(Local<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "Mixer"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  SETUP_EVENTED_PROTOTYPE_METHODS(tpl);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addPublisher", addPublisher);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removePublisher", removePublisher);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addSubscriber", addSubscriber);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeSubscriber", removeSubscriber);
  NODE_SET_PROTOTYPE_METHOD(tpl, "subscribeStream", subscribeStream);
  NODE_SET_PROTOTYPE_METHOD(tpl, "unsubscribeStream", unsubscribeStream);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addExternalPublisher", addExternalPublisher);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getRegion", getRegion);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setRegion", setRegion);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setVideoBitrate", setVideoBitrate);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "Mixer"), tpl->GetFunction());
}

void Mixer::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param(args[0]->ToString());
  std::string customParam = std::string(*param);

  Mixer* obj = new Mixer();
  obj->me = dynamic_cast<mcu::MixerInterface*>(woogeen_base::Gateway::createGatewayInstance(customParam));
  obj->me->setEventRegistry(obj);
  obj->Wrap(args.This());

  args.GetReturnValue().Set(args.This());
}

void Mixer::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;
  delete me;
}

void Mixer::addPublisher(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  Gateway* param0 =
      ObjectWrap::Unwrap<Gateway>(args[0]->ToObject());
  erizo::MediaSource* wr = param0->me;

  String::Utf8Value param1(args[1]->ToString());
  std::string clientId = std::string(*param1);

  String::Utf8Value param2(args[2]->ToString());
  std::string videoResolution = std::string(*param2);

  bool added = me->addPublisher(wr, clientId, videoResolution);

  args.GetReturnValue().Set(Boolean::New(isolate, added));
}

void Mixer::addExternalPublisher(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  ExternalInput* param = ObjectWrap::Unwrap<ExternalInput>(args[0]->ToObject());
  woogeen_base::ExternalInput* wr = (woogeen_base::ExternalInput*)param->me;

  String::Utf8Value param1(args[1]->ToString());
  std::string clientId = std::string(*param1);

  bool added = me->addPublisher(wr, clientId);

  args.GetReturnValue().Set(Boolean::New(isolate, added));
}

void Mixer::removePublisher(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;
  String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);

  me->removePublisher(id);
}

void Mixer::addSubscriber(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  WebRtcConnection* param =
      ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
  erizo::WebRtcConnection* wr = param->me;

  v8::String::Utf8Value param1(args[1]->ToString());
  std::string peerId = std::string(*param1);
  std::string options {""};
  if (args.Length() > 2 && args[2]->IsString()) {
    v8::String::Utf8Value param(args[2]->ToString());
    options = std::string(*param);
  }
  me->addSubscriber(wr, peerId, options);
}

void Mixer::removeSubscriber(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  // get the param
  v8::String::Utf8Value param(args[0]->ToString());

  // convert it to string
  std::string peerId = std::string(*param);
  me->removeSubscriber(peerId);
}

void Mixer::subscribeStream(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  v8::String::Utf8Value param(args[0]->ToString());
  std::string peerId = std::string(*param);
  bool isAudio = args[1]->BooleanValue();
  me->subscribeStream(peerId, isAudio);
}

void Mixer::unsubscribeStream(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  v8::String::Utf8Value param(args[0]->ToString());
  std::string peerId = std::string(*param);
  bool isAudio = args[1]->BooleanValue();
  me->unsubscribeStream(peerId, isAudio);
}

void Mixer::getRegion(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);

  std::string region = me->getRegion(id);

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, region.c_str()));
}

void Mixer::setRegion(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  String::Utf8Value param1(args[1]->ToString());
  std::string region = std::string(*param1);

  bool ret = me->setRegion(id, region);

  args.GetReturnValue().Set(Boolean::New(isolate, ret));
}

void Mixer::setVideoBitrate(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  int bitrate = args[1]->IntegerValue();

  bool ret = me->setVideoBitrate(id, bitrate);

  args.GetReturnValue().Set(Boolean::New(isolate, ret));
}
