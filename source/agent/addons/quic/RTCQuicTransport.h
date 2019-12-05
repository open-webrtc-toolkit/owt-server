/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_RTCQUICTRANSPORT_H_
#define WEBRTC_RTCQUICTRANSPORT_H_

#include <IceConnection.h>
#include <logger.h>
#include <memory>
#include <nan.h>
#include "RTCIceTransport.h"
#include "owt/quic/p2p_quic_transport_interface.h"

// Node.js addon of RTCQuicTransport.
// https://w3c.github.io/webrtc-quic/#dom-rtcquictransport
// Some of its implementation is based on blink::P2PQuicTransportImpl.
class RTCQuicTransport : public Nan::ObjectWrap {
    DECLARE_LOGGER();

public:
    static NAN_MODULE_INIT(Init);

    static NAN_METHOD(newInstance);
    static NAN_METHOD(start);
    static NAN_METHOD(getLocalParameters);
    static NAN_METHOD(createBidirectionalStream);
    static NAN_METHOD(listen);
    static NAN_METHOD(stop);

protected:
    //virtual void OnStream(P2PQuicStream* stream) override;

private:
    explicit RTCQuicTransport();
    ~RTCQuicTransport();

    static NAUV_WORK_CB(onStreamCallback);

    static Nan::Persistent<v8::Function> s_constructor;

    RTCIceTransport* m_iceTransport;
    std::unique_ptr<owt::quic::P2PQuicTransportInterface> m_transport;
};

#endif