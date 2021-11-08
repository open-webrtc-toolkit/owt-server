// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include "CallBaseWrapper.h"

using namespace v8;

Nan::Persistent<Function> CallBase::constructor;

CallBase::CallBase() {};
CallBase::~CallBase() {};

NAN_MODULE_INIT(CallBase::Init) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("CallBase").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "close", close);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("CallBase").ToLocalChecked(),
           Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(CallBase::New) {
  if (info.IsConstructCall()) {
    CallBase* obj = new CallBase();
    obj->rtcAdapter.reset(rtc_adapter::RtcAdapterFactory::CreateRtcAdapter());

    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    // Not construct call
  }
}

NAN_METHOD(CallBase::close) {
  CallBase* obj = Nan::ObjectWrap::Unwrap<CallBase>(info.Holder());
  obj->rtcAdapter.reset();
}
