// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "InternalIOWrapper.h"

using namespace v8;

DEFINE_LOGGER(SctpIn, "SctpInWrapper");

Persistent<Function> SctpIn::constructor;

SctpIn::SctpIn() : me(NULL) {}

SctpIn::~SctpIn()
{
  if (me)
    delete me;
}

void SctpIn::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "SctpIn"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getListeningPort", getListeningPort);
  NODE_SET_PROTOTYPE_METHOD(tpl, "connect", connect);

  NODE_SET_PROTOTYPE_METHOD(tpl, "addDestination", addDestination);
  NODE_SET_PROTOTYPE_METHOD(tpl, "removeDestination", removeDestination);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "SctpIn"), tpl->GetFunction());
}

void SctpIn::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SctpIn* obj = new SctpIn();
  obj->me = new owt_base::InternalSctp();
  obj->src = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void SctpIn::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  SctpIn* obj = ObjectWrap::Unwrap<SctpIn>(args.Holder());

  if (obj->me) {
    delete obj->me;
    obj->me = NULL;
    obj->src = NULL;
  }
}

void SctpIn::getListeningPort(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SctpIn* obj = ObjectWrap::Unwrap<SctpIn>(args.This());
  owt_base::InternalSctp* me = obj->me;

  if (!me) {
    // Skip when function close has been called
    ELOG_WARN("Connection has been closed.");
    return;
  }

  uint32_t udpPort = me->getLocalUdpPort();
  uint32_t sctpPort = me->getLocalSctpPort();

  Local<Object> portInfo = Object::New(isolate);
  portInfo->Set(String::NewFromUtf8(isolate, "udp"), Number::New(isolate, udpPort));
  portInfo->Set(String::NewFromUtf8(isolate, "sctp"), Number::New(isolate, sctpPort));

  args.GetReturnValue().Set(portInfo);
}

void SctpIn::connect(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SctpIn* obj = ObjectWrap::Unwrap<SctpIn>(args.Holder());
  owt_base::InternalSctp* me = obj->me;

  if (!me) {
    // Skip when function close has been called
    ELOG_WARN("Connection has been closed.");
    return;
  }

  String::Utf8Value param0(args[0]->ToString());
  std::string destIp = std::string(*param0);
  unsigned int udpPort = args[1]->Uint32Value();
  unsigned int sctpPort = args[2]->Uint32Value();

  me->connect(destIp, udpPort, sctpPort);
}

void SctpIn::addDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SctpIn* obj = ObjectWrap::Unwrap<SctpIn>(args.Holder());
  owt_base::InternalSctp* me = obj->me;

  if (!me) {
    // Skip when function close has been called
    ELOG_WARN("Connection has been closed.");
    return;
  }

  String::Utf8Value param0(args[0]->ToString());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
  owt_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    me->addAudioDestination(dest);
  } else if (track == "video") {
    me->addVideoDestination(dest);
  }
}

void SctpIn::removeDestination(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SctpIn* obj = ObjectWrap::Unwrap<SctpIn>(args.Holder());
  owt_base::InternalSctp* me = obj->me;

  if (!me) {
    // Skip when function close has been called
    ELOG_WARN("Connection has been closed.");
    return;
  }

  String::Utf8Value param0(args[0]->ToString());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
  owt_base::FrameDestination* dest = param->dest;

  if (track == "audio") {
    me->removeAudioDestination(dest);
  } else if (track == "video") {
    me->removeVideoDestination(dest);
  }
}

DEFINE_LOGGER(SctpOut, "SctpOutWrapper");

Persistent<Function> SctpOut::constructor;

SctpOut::SctpOut() : me(NULL) {}

SctpOut::~SctpOut()
{
  if (me)
    delete me;
}

void SctpOut::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "SctpOut"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getListeningPort", getListeningPort);
  NODE_SET_PROTOTYPE_METHOD(tpl, "connect", connect);


  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "SctpOut"), tpl->GetFunction());
}

void SctpOut::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SctpOut* obj = new SctpOut();
  obj->me = new owt_base::InternalSctp();
  obj->dest = obj->me;

  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void SctpOut::close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  SctpOut* obj = ObjectWrap::Unwrap<SctpOut>(args.Holder());

  if (obj->me) {
    delete obj->me;
    obj->me = NULL;
    obj->dest = NULL;
  }
}

void SctpOut::getListeningPort(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SctpOut* obj = ObjectWrap::Unwrap<SctpOut>(args.This());
  owt_base::InternalSctp* me = obj->me;

  if (!me) {
    // Skip when function close has been called
    ELOG_WARN("Connection has been closed.");
    return;
  }

  uint32_t udpPort = me->getLocalUdpPort();
  uint32_t sctpPort = me->getLocalSctpPort();

  Local<Object> portInfo = Object::New(isolate);
  portInfo->Set(String::NewFromUtf8(isolate, "udp"), Number::New(isolate, udpPort));
  portInfo->Set(String::NewFromUtf8(isolate, "sctp"), Number::New(isolate, sctpPort));

  args.GetReturnValue().Set(portInfo);
}

void SctpOut::connect(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  SctpOut* obj = ObjectWrap::Unwrap<SctpOut>(args.Holder());
  owt_base::InternalSctp* me = obj->me;

  if (!me) {
    // Skip when function close has been called
    ELOG_WARN("Connection has been closed.");
    return;
  }

  String::Utf8Value param0(args[0]->ToString());
  std::string destIp = std::string(*param0);
  unsigned int udpPort = args[1]->Uint32Value();
  unsigned int sctpPort = args[2]->Uint32Value();

  me->connect(destIp, udpPort, sctpPort);
}