/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_RTCCERTIFICATE_H_
#define WEBRTC_RTCCERTIFICATE_H_

#include <logger.h>
#include <nan.h>
#include "third_party/webrtc/api/scoped_refptr.h"
#include "third_party/webrtc/rtc_base/rtc_certificate.h"

namespace rtc{
    class RTCCertificate;
}

// Node.js addon of RTCCertificate.
// https://w3c.github.io/webrtc-pc/#rtccertificate-interface
class RTCCertificate : public Nan::ObjectWrap {
    DECLARE_LOGGER();

public:
    static NAN_MODULE_INIT(Init);

    static v8::Local<v8::Object> newInstance(const rtc::scoped_refptr<rtc::RTCCertificate> certificate);
    rtc::scoped_refptr<rtc::RTCCertificate> certificate() const;

protected:
    explicit RTCCertificate(){};
    ~RTCCertificate(){};

private:
    static NAN_METHOD(newInstance);
    static NAN_METHOD(generateCertificate);
    static Nan::Persistent<v8::Function> s_constructor;
    rtc::scoped_refptr<rtc::RTCCertificate> m_certificate;
};

#endif