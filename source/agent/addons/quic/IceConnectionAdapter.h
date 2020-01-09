/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_ICECONNECTIONADAPTER_H_
#define WEBRTC_ICECONNECTIONADAPTER_H_

#include "owt/quic/p2p_quic_packet_transport_interface.h"
#include "RTCIceTransport.h"
#include <IceConnection.h>

// IceConnectionAdapter could also be a class inherited from IceConnection.
// However, it will lose licode's benifits for supporting both libnice and
// nIcer.
class IceConnectionAdapter : public RTCIceTransport::QuicListener,
                             public owt::quic::P2PQuicPacketTransportInterface {
    DECLARE_LOGGER();

public:
    // Do not have ownership of |iceConnection|, so don't set self as its listener.
    explicit IceConnectionAdapter(erizo::IceConnection* iceConnection);

    // Implements owt::quic::P2PQuicPacketTransportInterface.
    int WritePacket(const char* data, size_t length) override;
    void SetReceiveDelegate(ReceiveDelegate* receiveDelegate) override;
    void SetWriteObserver(WriteObserver* writeObserver) override;
    bool Writable() override;

protected:
    void onReadPacket(const char* buffer, size_t buffer_length) override;
    void onReadyToWrite() override;

private:
    bool m_writeable;
    erizo::IceConnection* m_iceConnection;
    ReceiveDelegate* m_receiveDelegate;
    WriteObserver* m_writeObserver;
};

#endif