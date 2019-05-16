/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_QUICCONNECTIONADAPTER_H_
#define WEBRTC_QUICCONNECTIONADAPTER_H_

#include "net/third_party/quiche/src/quic/quartc/quartc_packet_writer.h"
#include "rtc_base/third_party/sigslot/sigslot.h"
#include "IceConnectionAdapter.h"

namespace base {
class TaskRunner;
};

// P2PQuicPacketTransport uses ICE as its underlying transport channel.
class P2PQuicPacketTransportIceAdapter : public quic::QuartcPacketTransport, public sigslot::has_slots<> {
    DECLARE_LOGGER();
public:
    P2PQuicPacketTransportIceAdapter(std::shared_ptr<IceConnectionAdapter> iceConnection, base::TaskRunner* runner);
    ~P2PQuicPacketTransportIceAdapter();

    virtual int Write(const char* buffer, size_t bufLen, const PacketInfo& info) override;
    virtual void SetDelegate(quic::QuartcPacketTransport::Delegate* delegate) override;

private:
    void onReadPacket(IceConnectionAdapter* packetTransport, const char* buffer, size_t bufferLength, const int64_t packetTime, int flag);
    void onReadyToSend(IceConnectionAdapter* packetTransport);
    void doReadPacket(IceConnectionAdapter* packetTransport, std::unique_ptr<char[]> buffer, size_t bufferLength, const int64_t packetTime, int flag);
    std::shared_ptr<IceConnectionAdapter> m_iceConnection;
    quic::QuartcPacketTransport::Delegate* m_transportDelegate;
    base::TaskRunner* m_runner;
};

#endif