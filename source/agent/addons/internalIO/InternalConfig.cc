// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "InternalConfig.h"
#include <nan.h>
#include <TransportBase.h>

using namespace v8;

void setPassphrase(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
  std::string p = std::string(*param0);
  owt_base::TransportSecret::setPassphrase(p);
}

void InitInternalConfig(v8::Local<v8::Object> exports) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(setPassphrase);
  Nan::Set(exports, Nan::New("setPassphrase").ToLocalChecked(),
           Nan::GetFunction(tpl).ToLocalChecked());
}
