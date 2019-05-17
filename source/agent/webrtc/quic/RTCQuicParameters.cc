/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RTCQuicParameters.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

Nan::Persistent<v8::Function> RTCQuicParameters::s_constructor;

DEFINE_LOGGER(RTCQuicParameters, "RTCQuicParameters");

NAN_MODULE_INIT(RTCQuicParameters::Init){
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("RTCQuicParameters").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    s_constructor.Reset(tpl->GetFunction());
    Nan::Set(target, Nan::New("RTCQuicParameters").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}