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

void IceConnectionAdapter::onReadPacket(const char* buffer, size_t buffer_length)
{
    ELOG_DEBUG("IceConnectionAdapter::onReadPacket");
    if (m_receiveDelegate) {
        m_receiveDelegate->OnPacketDataReceived(buffer, buffer_length);
    } else {
        ELOG_DEBUG("ReceiveDelegate is nullptr");
    }
}

void IceConnectionAdapter::onReadyToWrite()
{
    ELOG_DEBUG("IceConnectionAdapter::onReadyToWrite");
    if (m_writeObserver) {
        m_writeObserver->OnCanWrite();
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