// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "InternalConfig.h"
#include <RawTransport.h>

using namespace v8;

void setPassphrase(const FunctionCallbackInfo<Value>& args) {
  String::Utf8Value param0(args[0]->ToString());
  std::string p = std::string(*param0);
  owt_base::RawTransport<owt_base::Protocol::TCP>::setPassphrase(p);
}

void InitInternalConfig(v8::Local<v8::Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, setPassphrase);
  exports->Set(String::NewFromUtf8(isolate, "setPassphrase"), tpl->GetFunction());
}
