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
  obj->me = new woogeen_base::InternalSctp();
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
  woogeen_base::InternalSctp* me = obj->me;

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
  woogeen_base::InternalSctp* me = obj->me;

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
  woogeen_base::InternalSctp* me = obj->me;

  if (!me) {
    // Skip when function close has been called
    ELOG_WARN("Connection has been closed.");
    return;
  }

  String::Utf8Value param0(args[0]->ToString());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
  woogeen_base::FrameDestination* dest = param->dest;

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
  woogeen_base::InternalSctp* me = obj->me;

  if (!me) {
    // Skip when function close has been called
    ELOG_WARN("Connection has been closed.");
    return;
  }

  String::Utf8Value param0(args[0]->ToString());
  std::string track = std::string(*param0);

  FrameDestination* param = ObjectWrap::Unwrap<FrameDestination>(args[1]->ToObject());
  woogeen_base::FrameDestination* dest = param->dest;

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
  obj->me = new woogeen_base::InternalSctp();
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
  woogeen_base::InternalSctp* me = obj->me;

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
  woogeen_base::InternalSctp* me = obj->me;

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