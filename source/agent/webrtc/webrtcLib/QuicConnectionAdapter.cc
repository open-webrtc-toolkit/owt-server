/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QuicConnectionAdapter.h"
#include "third_party/webrtc/rtc_base/time_utils.h"
#include "base/task_runner.h"
#include "base/bind.h"

DEFINE_LOGGER(P2PQuicPacketTransportIceAdapter, "P2PQuicPacketTransportIceAdapter");

P2PQuicPacketTransportIceAdapter::P2PQuicPacketTransportIceAdapter(std::shared_ptr<IceConnectionAdapter> iceConnection, base::TaskRunner* runner)
{
    ELOG_DEBUG("P2PQuicPacketTransportIceAdapter::P2PQuicPacketTransportIceAdapter");
    DCHECK(iceConnection);
    m_iceConnection = iceConnection;
    m_runner = runner;
    iceConnection->signalReadPacket.connect(this, &P2PQuicPacketTransportIceAdapter::onReadPacket);
    iceConnection->signalReadyToSend.connect(this, &P2PQuicPacketTransportIceAdapter::onReadyToSend);
}

P2PQuicPacketTransportIceAdapter::~P2PQuicPacketTransportIceAdapter() {
    ELOG_DEBUG("P2PQuicPacketTransportIceAdapter::~P2PQuicPacketTransportIceAdapter");
}

int P2PQuicPacketTransportIceAdapter::Write(const char* buffer, size_t bufLen, const quic::QuartcPacketTransport::PacketInfo& info)
{
    ELOG_DEBUG("P2PQuicPacketTransportIceAdapter::write");
    return m_iceConnection->sendPacket(buffer, bufLen);
}

void P2PQuicPacketTransportIceAdapter::SetDelegate(quic::QuartcPacketTransport::Delegate* delegate)
{
    ELOG_DEBUG("P2PQuicPacketTransportIceAdapter::SetDelegate");
    m_transportDelegate = delegate;
}

void P2PQuicPacketTransportIceAdapter::doReadPacket(IceConnectionAdapter* packetTransport, std::unique_ptr<char[]> buffer, size_t bufferLength, const int64_t packetTime, int flag){
    if (!m_transportDelegate) {
        return;
    }
    m_transportDelegate->OnTransportReceived(buffer.get(), bufferLength);
}

void P2PQuicPacketTransportIceAdapter::onReadPacket(IceConnectionAdapter* packetTransport, const char* buffer, size_t bufferLength, const int64_t packetTime, int flag)
{
    ELOG_DEBUG("P2PQuicPacketTransportIceAdapter::onReadPacket");
    std::unique_ptr<char[]> bufferCopied=std::make_unique<char[]>(bufferLength);
    memcpy(bufferCopied.get(), buffer, bufferLength);
    m_runner->PostTask(FROM_HERE, base::BindOnce(&P2PQuicPacketTransportIceAdapter::doReadPacket, base::Unretained(this), packetTransport, std::move(bufferCopied), bufferLength, packetTime, flag));
}

void P2PQuicPacketTransportIceAdapter::onReadyToSend(IceConnectionAdapter* packetTransport)
{
    ELOG_DEBUG("P2PQuicPacketTransportIceAdapter::onReadyToSend");
    if (!m_transportDelegate) {
        return;
    }
    m_transportDelegate->OnTransportCanWrite();
}