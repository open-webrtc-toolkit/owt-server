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

#include "Mixer.h"

using namespace v8;

Mixer::Mixer() {};
Mixer::~Mixer() {};

void Mixer::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("Mixer"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("close"),
      FunctionTemplate::New(close)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("addPublisher"),
      FunctionTemplate::New(addPublisher)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("removePublisher"),
      FunctionTemplate::New(removePublisher)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("addSubscriber"),
      FunctionTemplate::New(addSubscriber)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("removeSubscriber"),
      FunctionTemplate::New(removeSubscriber)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("addExternalOutput"),
      FunctionTemplate::New(addExternalOutput)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("removeExternalOutput"),
      FunctionTemplate::New(removeExternalOutput)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("addExternalPublisher"),
      FunctionTemplate::New(addExternalPublisher)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getRegion"),
      FunctionTemplate::New(getRegion)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setRegion"),
      FunctionTemplate::New(setRegion)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("setVideoBitrate"),
      FunctionTemplate::New(setVideoBitrate)->GetFunction());

  Persistent<Function> constructor =
      Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("Mixer"), constructor);
}

Handle<Value> Mixer::New(const Arguments& args) {
  HandleScope scope;

  String::Utf8Value param(args[0]->ToString());
  std::string customParam = std::string(*param);

  Mixer* obj = new Mixer();
  obj->me = dynamic_cast<mcu::MixerInterface*>(woogeen_base::Gateway::createGatewayInstance(customParam));

  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> Mixer::close(const Arguments& args) {
  HandleScope scope;
  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  me->destroyAsyncEvents();
  delete me;

  return scope.Close(Null());
}

Handle<Value> Mixer::addPublisher(const Arguments& args) {
  HandleScope scope;

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

  return scope.Close(Boolean::New(added));
}

Handle<Value> Mixer::addExternalPublisher(const Arguments& args) {
  HandleScope scope;

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  ExternalInput* param = ObjectWrap::Unwrap<ExternalInput>(args[0]->ToObject());
  erizo::ExternalInput* wr = (erizo::ExternalInput*)param->me;

  String::Utf8Value param1(args[1]->ToString());
  std::string clientId = std::string(*param1);

  bool added = me->addPublisher(wr, clientId);

  return scope.Close(Boolean::New(added));
}

Handle<Value> Mixer::removePublisher(const Arguments& args) {
  HandleScope scope;

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;
  String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);

  me->removePublisher(id);

  return scope.Close(Null());
}

Handle<Value> Mixer::addSubscriber(const Arguments& args) {
  HandleScope scope;

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  WebRtcConnection* param =
      ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
  erizo::WebRtcConnection* wr = param->me;

  // get the param
  v8::String::Utf8Value param1(args[1]->ToString());

  // convert it to string
  std::string peerId = std::string(*param1);
  me->addSubscriber(wr, peerId);

  return scope.Close(Null());
}

Handle<Value> Mixer::removeSubscriber(const Arguments& args) {
  HandleScope scope;

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  // get the param
  v8::String::Utf8Value param(args[0]->ToString());

  // convert it to string
  std::string peerId = std::string(*param);
  me->removeSubscriber(peerId);

  return scope.Close(Null());
}

Handle<Value> Mixer::addExternalOutput(const Arguments& args) {
  HandleScope scope;

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  String::Utf8Value param(args[0]->ToString());
  std::string configParam = std::string(*param);

  bool succeeded = me->addExternalOutput(configParam);

  return scope.Close(Boolean::New(succeeded));
}

Handle<Value> Mixer::removeExternalOutput(const Arguments& args) {
  HandleScope scope;

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  // get the param
  v8::String::Utf8Value param(args[0]->ToString());

  // convert it to string
  std::string outputId = std::string(*param);
  bool succeeded = me->removeExternalOutput(outputId);

  return scope.Close(Boolean::New(succeeded));
}

Handle<Value> Mixer::getRegion(const Arguments& args) {
  HandleScope scope;

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);

  std::string region = me->getRegion(id);

  return scope.Close(String::New(region.c_str()));
}

Handle<Value> Mixer::setRegion(const Arguments& args) {
  HandleScope scope;

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  String::Utf8Value param1(args[1]->ToString());
  std::string region = std::string(*param1);

  bool ret = me->setRegion(id, region);

  return scope.Close(Boolean::New(ret));
}

Handle<Value> Mixer::setVideoBitrate(const Arguments& args) {
  HandleScope scope;

  Mixer* obj = ObjectWrap::Unwrap<Mixer>(args.This());
  mcu::MixerInterface* me = obj->me;

  String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  int bitrate = args[1]->IntegerValue();

  bool ret = me->setVideoBitrate(id, bitrate);

  return scope.Close(Boolean::New(ret));
}
