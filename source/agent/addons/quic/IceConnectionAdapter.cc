/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "IceConnectionAdapter.h"

DEFINE_LOGGER(IceConnectionAdapter, "IceConnectionAdapter");

IceConnectionAdapter::IceConnectionAdapter(erizo::IceConnection* iceConnection)
    : m_writeable(false)
    , m_iceConnection(iceConnection)
    , m_receiveDelegate(nullptr)
    , m_writeObserver(nullptr)
{
}

void IceConnectionAdapter::onPacketReceived(erizo::packetPtr packet)
{
    ELOG_DEBUG("IceConnectionAdapter::onPacketReceived");
    if (m_receiveDelegate) {
        m_receiveDelegate->OnPacketDataReceived(packet->data, packet->length);
    }
}

void IceConnectionAdapter::onCandidate(const erizo::CandidateInfo& candidate, erizo::IceConnection* conn)
{
    ELOG_DEBUG("IceConnectionAdapter::onCandidate");
}
void IceConnectionAdapter::updateIceState(erizo::IceState state, erizo::IceConnection* conn)
{
    ELOG_DEBUG("IceConnectionAdapter::updateIceState %d", state);
    if (state == erizo::IceState::READY) {
        m_writeable = true;
    } else if (state == erizo::IceState::FAILED) {
        m_writeable = false;
    }
}

int IceConnectionAdapter::WritePacket(const char* data, size_t length)
{
    ELOG_DEBUG("IceConnectionAdapter::sendPacket length: %d", length);
    return m_iceConnection->sendData(1, data, length);
}

void IceConnectionAdapter::SetReceiveDelegate(ReceiveDelegate* receiveDelegate)
{
    m_receiveDelegate = receiveDelegate;
}

void IceConnectionAdapter::SetWriteObserver(WriteObserver* writeObserver)
{
    m_writeObserver = writeObserver;
}

bool IceConnectionAdapter::Writable()
{
    return m_writeable;
}