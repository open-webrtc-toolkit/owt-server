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
    : m_datagramOutput(nullptr)
    , m_videoRtpPacketizer(nullptr)
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

    Nan::SetPrototypeMethod(tpl, "addDatagramOutput", addDatagramOutput);
    Nan::SetPrototypeMethod(tpl, "receiver", receiver);

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

NAN_METHOD(WebTransportFrameDestination::addDatagramOutput)
{
    WebTransportFrameDestination* obj = Nan::ObjectWrap::Unwrap<WebTransportFrameDestination>(info.Holder());
    if (obj->m_datagramOutput) {
        ELOG_WARN("Datagram output exists, will be replaced by the new one.");
    }
    if (!obj->m_videoRtpPacketizer) {
        obj->m_videoRtpPacketizer = std::make_unique<VideoRtpPacketizer>();
        obj->m_videoRtpPacketizer->addDataDestination(obj);
    }
    NanFrameNode* output = Nan::ObjectWrap::Unwrap<NanFrameNode>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
    obj->m_datagramOutput = output;
}

NAN_METHOD(WebTransportFrameDestination::receiver)
{
    info.GetReturnValue().Set(info.This());
}

void WebTransportFrameDestination::onFrame(const owt_base::Frame& frame)
{
    if (!m_datagramOutput) {
        ELOG_WARN("Datagram output is not ready, dropping frame.");
        return;
    }
    // Packetize video frames or send RTP packets.
    if (frame.format != owt_base::FRAME_FORMAT_RTP) {
        m_videoRtpPacketizer->onFrame(frame);
    } else {
        m_datagramOutput->FrameDestination()->onFrame(frame);
    }
}

void WebTransportFrameDestination::onVideoSourceChanged() { }