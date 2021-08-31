/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "WebTransportFrameDestination.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

DEFINE_LOGGER(WebTransportFrameDestination, "WebTransportFrameDestination");

Nan::Persistent<v8::Function> WebTransportFrameDestination::s_constructor;

WebTransportFrameDestination::WebTransportFrameDestination()
{
}

WebTransportFrameDestination::~WebTransportFrameDestination()
{
}

NAN_MODULE_INIT(WebTransportFrameDestination::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("WebTransportFrameDestination").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("WebTransportFrameDestination").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(WebTransportFrameDestination::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    WebTransportFrameDestination* obj = new WebTransportFrameDestination();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}