/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_RTCQUICTRANSPORT_H_
#define WEBRTC_RTCQUICTRANSPORT_H_

#include <memory>
#include <nan.h>
#include "RTCIceTransport.h"
#include "owt/quic/p2p_quic_stream_interface.h"
#include "owt/quic/p2p_quic_transport_interface.h"
#include "owt/quic/p2p_quic_packet_transport_interface.h"

// Node.js addon of RTCQuicTransport.
// https://w3c.github.io/webrtc-quic/#dom-rtcquictransport
// Some of its implementation is based on blink::P2PQuicTransportImpl.
class RTCQuicTransport : public Nan::ObjectWrap, public owt::quic::P2PQuicTransportInterface::Delegate {
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
    void OnIncomingStream(owt::quic::P2PQuicStreamInterface* stream) override;

private:
    explicit RTCQuicTransport();
    ~RTCQuicTransport();

    static NAUV_WORK_CB(onStreamCallback);

    static Nan::Persistent<v8::Function> s_constructor;

    std::unique_ptr<owt::quic::P2PQuicPacketTransportInterface> m_packetTransport;
    std::unique_ptr<owt::quic::P2PQuicTransportInterface> m_transport;
    uv_async_t m_asyncOnStream;
    std::mutex m_onStreamMutex;
    std::queue<owt::quic::P2PQuicStreamInterface*> m_unnotifiedStreams;
};

#endif