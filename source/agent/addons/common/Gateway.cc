// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "Gateway.h"

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
  obj->me = owt_base::Gateway::createGatewayInstance(customParam);
  obj->me->setEventRegistry(obj);
  obj->Wrap(args.This());

  args.GetReturnValue().Set(args.This());
}

void Gateway::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  owt_base::Gateway* me = obj->me;
  delete me;
}

void Gateway::addPublisher(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  owt_base::Gateway* me = obj->me;

  WebRtcConnection* param0 =
      ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
  param0->ref();
  erizo::WebRtcConnection* wr = (erizo::WebRtcConnection*)param0->me;

  String::Utf8Value param1(args[1]->ToString());
  std::string clientId = std::string(*param1);

  String::Utf8Value param2(args[2]->ToString());
  std::string videoResolution = std::string(*param2);

  bool added = me->addPublisher(wr, clientId, videoResolution);

  args.GetReturnValue().Set(Boolean::New(isolate, added));
}

void Gateway::removePublisher(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  owt_base::Gateway* me = obj->me;
  String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);

  me->removePublisher(id);
}

void Gateway::addSubscriber(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  owt_base::Gateway* me = obj->me;

  WebRtcConnection* param =
      ObjectWrap::Unwrap<WebRtcConnection>(args[0]->ToObject());
  param->ref();
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
  owt_base::Gateway* me = obj->me;

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
  owt_base::Gateway* me = obj->me;
  v8::String::Utf8Value str(args[0]->ToString());
  std::string message = std::string(*str);
  me->customMessage(message);
}

void Gateway::retrieveStatistics(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  owt_base::Gateway* me = obj->me;

  std::string stats = me->retrieveStatistics();

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, stats.c_str()));
}

void Gateway::subscribeStream(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  owt_base::Gateway* me = obj->me;
  v8::String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  bool isAudio = args[1]->BooleanValue();
  me->subscribeStream(id, isAudio);
}

void Gateway::unsubscribeStream(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  owt_base::Gateway* me = obj->me;
  v8::String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  bool isAudio = args[1]->BooleanValue();
  me->unsubscribeStream(id, isAudio);
}

void Gateway::publishStream(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  owt_base::Gateway* me = obj->me;
  v8::String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  bool isAudio = args[1]->BooleanValue();
  me->publishStream(id, isAudio);
}

void Gateway::unpublishStream(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Gateway* obj = ObjectWrap::Unwrap<Gateway>(args.Holder());
  owt_base::Gateway* me = obj->me;
  v8::String::Utf8Value param(args[0]->ToString());
  std::string id = std::string(*param);
  bool isAudio = args[1]->BooleanValue();
  me->unpublishStream(id, isAudio);
}

