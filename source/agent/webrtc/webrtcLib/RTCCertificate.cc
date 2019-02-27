/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RTCCertificate.h"
#include "third_party/webrtc/rtc_base/rtc_certificate_generator.h"
#include "third_party/webrtc/rtc_base/ssl_adapter.cc"
#include "third_party/webrtc/rtc_base/time_utils.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Value;

Nan::Persistent<v8::Function> RTCCertificate::s_constructor;

DEFINE_LOGGER(RTCCertificate, "RTCCertificate");


NAN_MODULE_INIT(RTCCertificate::Init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(newInstance);
    tpl->SetClassName(Nan::New("RTCCertificate").ToLocalChecked());
    Local<ObjectTemplate> instanceTpl = tpl->InstanceTemplate();
    instanceTpl->SetInternalFieldCount(1);

    // TODO: Move it to RTCPeerConnection.
    Nan::SetMethod(tpl, "generateCertificate", generateCertificate);

    s_constructor.Reset(tpl->GetFunction());
    Nan::Set(target, Nan::New("RTCCertificate").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());

    rtc::InitializeSSL();
    ELOG_DEBUG("Initialized SSL.");
}

NAN_METHOD(RTCCertificate::newInstance)
{
    if (!info.IsConstructCall()) {
        return;
    }
    RTCCertificate* obj = new RTCCertificate();
    obj->m_certificate = rtc::RTCCertificateGenerator::GenerateCertificate(rtc::KeyParams::ECDSA(),
        absl::nullopt);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Object> RTCCertificate::newInstance(const rtc::scoped_refptr<rtc::RTCCertificate> certificate)
{
    Local<Object> certificateObject = Nan::NewInstance(Nan::New(RTCCertificate::s_constructor)).ToLocalChecked();
    auto obj = Nan::ObjectWrap::Unwrap<RTCCertificate>(certificateObject);
    obj->m_certificate = certificate;
    return certificateObject;
}

rtc::scoped_refptr<rtc::RTCCertificate> RTCCertificate::certificate() const
{
    return m_certificate;
}

NAN_METHOD(RTCCertificate::generateCertificate)
{
    info.GetReturnValue().Set(Nan::NewInstance(Nan::New(s_constructor)).ToLocalChecked());
}