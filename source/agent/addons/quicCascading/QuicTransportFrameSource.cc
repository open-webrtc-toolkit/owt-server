/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicTransportFrameSource.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

DEFINE_LOGGER(QuicTransportFrameSource, "QuicTransportFrameSource");

Nan::Persistent<v8::Function> QuicTransportFrameSource::s_constructor;

QuicTransportFrameSource::QuicTransportFrameSource()
    : m_audioStream(nullptr)
    , m_videoStream(nullptr)
    , m_dataStream(nullptr)
{
}

QuicTransportFrameSource::~QuicTransportFrameSource(){}

NAN_MODULE_INIT(QuicTransportFrameSource::init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("QuicTransportFrameSource").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "addDestination", addDestination);
    Nan::SetPrototypeMethod(tpl, "addInputStream", addInputStream);

    s_constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("QuicTransportFrameSource").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(QuicTransportFrameSource::newInstance)
{
    if (!info.IsConstructCall()) {
        ELOG_DEBUG("Not construct call.");
        return;
    }
    QuicTransportFrameSource* obj = new QuicTransportFrameSource();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(QuicTransportFrameSource::addInputStream)
{
    QuicTransportFrameSource* obj = Nan::ObjectWrap::Unwrap<QuicTransportFrameSource>(info.Holder());
    NanFrameNode* inputStream = Nan::ObjectWrap::Unwrap<NanFrameNode>(
        Nan::To<v8::Object>(info[0]).ToLocalChecked());
    inputStream->FrameSource()->addDataDestination(obj);
    inputStream->FrameSource()->addAudioDestination(obj);
    inputStream->FrameSource()->addVideoDestination(obj);
}

NAN_METHOD(QuicTransportFrameSource::addDestination)
{
    QuicTransportFrameSource* obj = Nan::ObjectWrap::Unwrap<QuicTransportFrameSource>(info.Holder());
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

void QuicTransportFrameSource::onFeedback(const owt_base::FeedbackMsg&)
{
    // No feedbacks righ now.
}

void QuicTransportFrameSource::onFrame(const owt_base::Frame& frame)
{
    deliverFrame(frame);
}

void QuicTransportFrameSource::onVideoSourceChanged()
{
    // Do nothing.
}