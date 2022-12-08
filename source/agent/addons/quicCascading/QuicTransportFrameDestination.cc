/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicTransportFrameDestination.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

DEFINE_LOGGER(QuicTransportFrameDestination, "QuicTransportFrameDestination");

Nan::Persistent<v8::Function> QuicTransportFrameDestination::s_constructor;

QuicTransportFrameDestination::QuicTransportFrameDestination()
{
}

QuicTransportFrameDestination::~QuicTransportFrameDestination()
{
}

NAN_MODULE_INIT(QuicTransportFrameDestination::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportFrameDestination").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("QuicTransportFrameDestination").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(QuicTransportFrameDestination::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    QuicTransportFrameDestination* obj = new QuicTransportFrameDestination();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}