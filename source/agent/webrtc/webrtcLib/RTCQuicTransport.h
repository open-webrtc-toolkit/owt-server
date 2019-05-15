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
//#include "RTCQuicTransportBase.h"
#include "QuicDefinitions.h"
#include "QuicStream.h"
#include "RTCCertificate.h"
#include "RTCIceTransport.h"
#include "P2PQuicTransport.h"
#include "base/at_exit.h"
#include "net/third_party/quiche/src/quic/core/crypto/quic_compressed_certs_cache.h"
#include "net/third_party/quiche/src/quic/core/crypto/quic_crypto_server_config.h"
#include "net/third_party/quiche/src/quic/core/quic_alarm_factory.h"
#include "net/third_party/quiche/src/quic/core/quic_connection.h"
#include "net/third_party/quiche/src/quic/core/quic_types.h"
#include "net/third_party/quiche/src/quic/quartc/quartc_packet_writer.h"
#include "third_party/webrtc/api/scoped_refptr.h"

// Node.js addon of RTCQuicTransport.
// https://w3c.github.io/webrtc-quic/#dom-rtcquictransport
// Some of its implementation is based on blink::P2PQuicTransportImpl.
class RTCQuicTransport : public Nan::ObjectWrap, public P2PQuicTransport::Delegate {
    DECLARE_LOGGER();

public:
    static NAN_MODULE_INIT(Init);

    static NAN_METHOD(newInstance);
    static NAN_METHOD(getCertificates);
    static NAN_METHOD(start);
    static NAN_METHOD(getLocalParameters);
    static NAN_METHOD(createBidirectionalStream);
    static NAN_METHOD(listen);
    static NAN_METHOD(stop);

protected:
    virtual void OnStream(P2PQuicStream* stream) override;

private:
    explicit RTCQuicTransport();
    ~RTCQuicTransport();
    std::shared_ptr<quic::QuicCryptoServerConfig> createServerCryptoConfig();
    void createAndStartP2PQuicTransport(RTCIceTransport* iceTransport, std::shared_ptr<quic::QuicCryptoServerConfig> serverCryptoConfig, base::TaskRunner* runner);
    std::unique_ptr<P2PQuicTransport> createP2PQuicTransport(RTCIceTransport* iceTransport, std::shared_ptr<quic::QuicCryptoServerConfig> serverCryptoConfig, base::TaskRunner* runner);

    static NAUV_WORK_CB(onStreamCallback);

    static Nan::Persistent<v8::Function> s_constructor;

    RTCIceTransport* m_iceTransport;
    std::unique_ptr<base::Thread> m_ioThread;
    scoped_refptr<base::SequencedTaskRunner> m_ioTaskRunner;
    std::shared_ptr<quic::QuicAlarmFactory> m_alarmFactory;
    std::shared_ptr<quic::QuicConnectionHelperInterface> m_helper;
    std::shared_ptr<quic::QuicCompressedCertsCache> m_compressedCertsCache;
    std::unique_ptr<P2PQuicTransport> m_transport;
    base::AtExitManager m_exitManager;
    std::shared_ptr<quic::QuartcPacketTransport> m_quicPacketTransport;
    uv_async_t m_asyncOnStream;
    std::mutex m_streamQueueMutex;
    std::queue<P2PQuicStream*> m_streamsToBeNotified;
    std::vector<QuicStream*> m_quicStreams;
};

#endif