/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#include "I420BufferManager.h"

namespace woogeen_base {

DEFINE_LOGGER(I420BufferManager, "woogeen.I420BufferManager");

I420BufferManager::I420BufferManager(uint32_t maxFrames)
{
    m_bufferPool.reset(new webrtc::I420BufferPool(false, maxFrames));
}

I420BufferManager::~I420BufferManager()
{
    m_busyFrame.reset();
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

void I420BufferManager::putBusyFrame(boost::shared_ptr<webrtc::VideoFrame> frame)
{
    boost::mutex::scoped_lock lock(m_mutex);

    m_busyFrame = frame;
    return;
}

boost::shared_ptr<webrtc::VideoFrame> I420BufferManager::getBusyFrame()
{
    boost::mutex::scoped_lock lock(m_mutex);

    return m_busyFrame;
}

} /* namespace woogeen_base */
