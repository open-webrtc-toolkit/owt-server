/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_QUICPARAMETERS_H_
#define WEBRTC_QUICPARAMETERS_H_

#include <logger.h>
#include <nan.h>

// Node.js addon of RTCQuicParameters.
// https://w3c.github.io/webrtc-quic/#dom-rtcquicparameters
class RTCQuicParameters : public Nan::ObjectWrap {
    DECLARE_LOGGER();

public:
    static NAN_MODULE_INIT(Init);

private:
  static Nan::Persistent<v8::Function> s_constructor;
};

#endif