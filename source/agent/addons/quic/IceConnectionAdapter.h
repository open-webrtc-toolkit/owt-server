/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_ICECONNECTIONADAPTER_H_
#define WEBRTC_ICECONNECTIONADAPTER_H_

#include "owt/quic/p2p_quic_packet_transport_interface.h"
#include <IceConnection.h>

// IceConnectionAdapter could also be a class inherited from IceConnection.
// However, it will lose licode's benifits for supporting both libnice and
// nIcer.
class IceConnectionAdapter : public erizo::IceConnectionListener,
                             public owt::quic::P2PQuicPacketTransportInterface {
    DECLARE_LOGGER();

public:
    explicit IceConnectionAdapter(erizo::IceConnection* iceConnection);

    // Implements owt::quic::P2PQuicPacketTransportInterface.
    int WritePacket(const char* data, size_t length) override;
    void SetReceiveDelegate(ReceiveDelegate* receiveDelegate) override;
    void SetWriteObserver(WriteObserver* writeObserver) override;
    bool Writable() override;

protected:
    virtual void onPacketReceived(erizo::packetPtr packet);
    virtual void onCandidate(const erizo::CandidateInfo& candidate, erizo::IceConnection* conn);
    virtual void updateIceState(erizo::IceState state, erizo::IceConnection* conn);

private:
    bool m_writeable;
    erizo::IceConnection* m_iceConnection;
    ReceiveDelegate* m_receiveDelegate;
    WriteObserver* m_writeObserver;
};

#endif