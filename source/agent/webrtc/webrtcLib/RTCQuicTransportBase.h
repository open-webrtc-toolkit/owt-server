/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_RTCQUICTRANSPORTBASE_H_
#define WEBRTC_RTCQUICTRANSPORTBASE_H_

#include <nan.h>

#include <logger.h>

// Node.js addon of RTCQuicTransportBase.
// https://w3c.github.io/webrtc-quic/cs.html#dom-quictransportbase
class RTCQuicTransportBase : public Nan::ObjectWrap {
    DECLARE_LOGGER();

public:
    //static NAN_MODULE_INIT(Init);

protected:
    RTCQuicTransportBase(){};
    ~RTCQuicTransportBase(){};

    static Nan::Persistent<v8::Function> s_constructor;
};

#endif