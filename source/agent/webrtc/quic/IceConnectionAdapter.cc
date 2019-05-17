/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "IceConnectionAdapter.h"
#include "third_party/webrtc/rtc_base/time_utils.h"

DEFINE_LOGGER(IceConnectionAdapter, "IceConnectionAdapter");

IceConnectionAdapter::IceConnectionAdapter(erizo::IceConnection* iceConnection)
    : m_iceConnection(iceConnection)
{
    m_originListener = iceConnection->getIceListener();
    iceConnection->setIceListener(this);
}

void IceConnectionAdapter::onPacketReceived(erizo::packetPtr packet)
{
    ELOG_DEBUG("IceConnectionAdapter::onPacketReceived");
    signalReadPacket(this, packet->data, packet->length, rtc::TimeMicros(), 0);
    if (m_originListener) {
        m_originListener->onPacketReceived(packet);
    }
}

void IceConnectionAdapter::onCandidate(const erizo::CandidateInfo& candidate, erizo::IceConnection* conn)
{
    ELOG_DEBUG("IceConnectionAdapter::onCandidate");
    if (m_originListener) {
        m_originListener->onCandidate(candidate, conn);
    }
}
void IceConnectionAdapter::updateIceState(erizo::IceState state, erizo::IceConnection* conn)
{
    ELOG_DEBUG("IceConnectionAdapter::updateIceState %d", state);
    if (state == erizo::IceState::READY) {
        signalReadyToSend(this);
    }
    if (m_originListener) {
        m_originListener->updateIceState(state, conn);
    }
}

int IceConnectionAdapter::sendPacket(const char* data, int length)
{
    ELOG_DEBUG("IceConnectionAdapter::sendPacket length: %d", length);
    return m_iceConnection->sendData(1, data, length);
}
