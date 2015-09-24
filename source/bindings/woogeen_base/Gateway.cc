/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
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

#include "Gateway.h"

#include "NodeEventRegistry.h"
#include "ExternalInput.h"
#include "../erizoAPI/MediaDefinitions.h"
#include "../erizoAPI/WebRtcConnection.h"

using namespace v8;

Persistent<Function> Gateway::constructor;
Gateway::Gateway() {};
Gateway::~Gateway() {};

void Gateway::Init(Local<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "Gateway"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  SETUP_EVENTED_PROTOTYPE_METHODS(tpl);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addPublisher", addPublisher);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removePublisher", removePublisher);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addSubscriber", addSubscriber);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeSubscriber", removeSubscriber);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addExternalPublisher", addExternalPublisher);
  NODE_SET_PROTOTYPE_METHOD(tpl, "customMessage", customMessage);
  NODE_SET_PROTOTYPE_METHOD(tpl, "retrieveStatistics", retrieveStatistics);
  NODE_SET_PROTOTYPE_METHOD(tpl, "subscribeStream", subscribeStream);
  NODE_SET_PROTOTYPE_METHOD(tpl, "unsubscribeStream", unsubscribeStream);
  NODE_SET_PROTOTYPE_METHOD(tpl, "publishStream", publishStream);
  NODE_SET_PROTOTYPE_METHOD(tpl, "unpublishStream", unpublishStream);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "Gateway"), tpl->GetFunction());
}

void Gateway::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  String::Utf8Value param(args[0]->ToString());
  std::string customParam = std::string(*param);

  Gateway* obj = new Gateway();
  obj->me = woogeen_base::Gateway::createGatewayInstance(customParam);
  obj->me->setEventRegistry(obj);
  obj->Wrap(args.This());

  args.GetReturnValue().Set(args.This());
}

void Gateway::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;
  delete me;
}

void Gateway::addPublisher(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;

  WebRtcConnection* param0 =
      ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
  erizo::WebRtcConnection* wr = (erizo::WebRtcConnection*)param0->me;

  String::Utf8Value param1(args[1]->ToString());
  std::string clientId = std::string(*param1);

  String::Utf8Value param2(args[2]->ToString());
  std::string videoResolution = std::string(*param2);

  bool added = me->addPublisher(wr, clientId, videoResolution);

  args.GetReturnValue().Set(Boolean::New(isolate, added));
}

void Gateway::addExternalPublisher(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;

  ExternalInput* param = ObjectWrap::Unwrap<ExternalInput>(args[0]->ToObject());
  woogeen_base::ExternalInput* wr = (woogeen_base::ExternalInput*)param->me;

  String::Utf8Value param1(args[1]->ToString());
  std::string clientId = std::string(*param1);

  bool added = me->addPublisher(wr, clientId);

  args.GetReturnValue().Set(Boolean::New(isolate, added));
}

void Gateway::removePublisher(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;
  String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);

  me->removePublisher(id);
}

void Gateway::addSubscriber(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;

  WebRtcConnection* param =
      ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
  erizo::WebRtcConnection* wr = param->me;

  // get the param
  v8::String::Utf8Value param1(args[1]->ToString());

  // convert it to string
  std::string peerId = std::string(*param1);
  me->addSubscriber(wr, peerId, "");
}

void Gateway::removeSubscriber(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;

  // get the param
  v8::String::Utf8Value param(args[0]->ToString());

  // convert it to string
  std::string peerId = std::string(*param);
  me->removeSubscriber(peerId);
}

void Gateway::customMessage(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  if (args.Length() < 1 || !args[0]->IsString() ) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
    return;
  }

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;
  v8::String::Utf8Value str(args[0]->ToString());
  std::string message = std::string(*str);
  me->customMessage(message);
}

void Gateway::retrieveStatistics(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway *me = obj->me;

  std::string stats = me->retrieveStatistics();

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, stats.c_str()));
}

void Gateway::subscribeStream(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;
  v8::String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  bool isAudio = args[1]->BooleanValue();
  me->subscribeStream(id, isAudio);
}

void Gateway::unsubscribeStream(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;
  v8::String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  bool isAudio = args[1]->BooleanValue();
  me->unsubscribeStream(id, isAudio);
}

void Gateway::publishStream(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;
  v8::String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  bool isAudio = args[1]->BooleanValue();
  me->publishStream(id, isAudio);
}

void Gateway::unpublishStream(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  woogeen_base::Gateway* me = obj->me;
  v8::String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  bool isAudio = args[1]->BooleanValue();
  me->unpublishStream(id, isAudio);
}
