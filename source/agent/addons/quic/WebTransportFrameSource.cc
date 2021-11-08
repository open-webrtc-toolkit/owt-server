/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "WebTransportFrameSource.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

DEFINE_LOGGER(WebTransportFrameSource, "WebTransportFrameSource");

Nan::Persistent<v8::Function> WebTransportFrameSource::s_constructor;

WebTransportFrameSource::WebTransportFrameSource()
    : m_audioStream(nullptr)
    , m_videoStream(nullptr)
    , m_dataStream(nullptr)
{
}

WebTransportFrameSource::~WebTransportFrameSource(){}

NAN_MODULE_INIT(WebTransportFrameSource::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("WebTransportFrameSource").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
    Nan::SetPrototypeMethod(tpl, "addInputStream", addInputStream);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("WebTransportFrameSource").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(WebTransportFrameSource::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    WebTransportFrameSource* obj = new WebTransportFrameSource();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(WebTransportFrameSource::addInputStream)
{
    WebTransportFrameSource* obj = Nan::ObjectWrap::Unwrap<WebTransportFrameSource>(info.Holder());
    NanFrameNode* inputStream = Nan::ObjectWrap::Unwrap<NanFrameNode>(
        Nan::To<v8::Object>(info[0]).ToLocalChecked());
    inputStream->FrameSource()->addDataDestination(obj);
    inputStream->FrameSource()->addAudioDestination(obj);
    inputStream->FrameSource()->addVideoDestination(obj);
}

NAN_METHOD(WebTransportFrameSource::addDestination)
{
    WebTransportFrameSource* obj = Nan::ObjectWrap::Unwrap<WebTransportFrameSource>(info.Holder());
    if (info.Length() > 3) {
        Nan::ThrowTypeError("Invalid argument length for addDestination.");
        return;
    }
    Nan::Utf8String param0(Nan::To<v8::String>(info[0]).ToLocalChecked());
    std::string track = std::string(*param0);
    bool isNanDestination(false);
    if (info.Length() == 3) {
        isNanDestination = Nan::To<bool>(info[2]).FromJust();
    }
    owt_base::FrameDestination* dest(nullptr);
    if (isNanDestination) {
        NanFrameNode* param = Nan::ObjectWrap::Unwrap<NanFrameNode>(
            Nan::To<v8::Object>(info[1]).ToLocalChecked());
        dest = param->FrameDestination();
    } else {
        ::FrameDestination* param = node::ObjectWrap::Unwrap<::FrameDestination>(
            Nan::To<v8::Object>(info[1]).ToLocalChecked());
        dest = param->dest;
    }
    obj->addAudioDestination(dest);
    obj->addVideoDestination(dest);
    obj->addDataDestination(dest);
}

void WebTransportFrameSource::onFeedback(const owt_base::FeedbackMsg&)
{
    // No feedbacks righ now.
}

void WebTransportFrameSource::onFrame(const owt_base::Frame& frame)
{
    deliverFrame(frame);
}

void WebTransportFrameSource::onVideoSourceChanged()
{
    // Do nothing.
}