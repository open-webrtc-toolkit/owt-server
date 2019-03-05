// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "I420BufferManager.h"

namespace owt_base {

DEFINE_LOGGER(I420BufferManager, "owt.I420BufferManager");

I420BufferManager::I420BufferManager(uint32_t maxFrames)
{
    m_bufferPool.reset(new webrtc::I420BufferPool(false, maxFrames));
}

I420BufferManager::~I420BufferManager()
{
    m_bufferPool->Release();
}

rtc::scoped_refptr<webrtc::I420Buffer> I420BufferManager::getFreeBuffer(uint32_t width, uint32_t height)
{
    rtc::scoped_refptr<webrtc::I420Buffer> buffer = m_bufferPool->CreateBuffer(width, height);
    if (!buffer.get()) {
        return NULL;
    }

    return buffer;
}

} /* namespace owt_base */
