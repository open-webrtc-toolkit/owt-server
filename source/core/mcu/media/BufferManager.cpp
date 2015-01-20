/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
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

#include "BufferManager.h"

#include <webrtc/common_video/interface/i420_video_frame.h>

namespace mcu {

DEFINE_LOGGER(BufferManager, "mcu.media.BufferManager");

BufferManager::BufferManager(uint32_t maxInput, uint32_t width, uint32_t height)
    : m_maxInput(maxInput)
{
    for (uint32_t i = 0; i < m_maxInput * 2; i++) {
        webrtc::I420VideoFrame* buffer =  new webrtc::I420VideoFrame();
        buffer->CreateEmptyFrame(width, height, width, width / 2, width / 2);
        m_freeQ.push(buffer);
    }

    m_busyQ = new volatile webrtc::I420VideoFrame*[m_maxInput];
    for (uint32_t i = 0; i < m_maxInput; i++) {
        m_busyQ[i] = nullptr;
        m_activeSlots.push_back(false);
    }

    ELOG_DEBUG("BufferManager constructed")
}

BufferManager::~BufferManager()
{
    webrtc::I420VideoFrame* buffer = nullptr;
    while (m_freeQ.pop(buffer))
        delete buffer;
    delete [] m_busyQ;
    ELOG_DEBUG("BufferManager destroyed")
}

webrtc::I420VideoFrame* BufferManager::getFreeBuffer()
{
    webrtc::I420VideoFrame* buffer = nullptr;
    if (m_freeQ.pop(buffer))
        return buffer;

    ELOG_DEBUG("freeQ is empty")
    return nullptr;
}

void BufferManager::releaseBuffer(webrtc::I420VideoFrame* frame)
{
    if (frame)
        m_freeQ.push(frame);
}

webrtc::I420VideoFrame* BufferManager::getBusyBuffer(uint32_t slot)
{
    assert (slot <= m_maxInput);
    webrtc::I420VideoFrame* busyFrame = reinterpret_cast<webrtc::I420VideoFrame*>(BufferManager::exchange((volatile uint64_t*)&(m_busyQ[slot]), 0));
    ELOG_TRACE("getBusyBuffer: busyQ[%d] is 0x%p, busyFrame is 0x%p", slot, m_busyQ[slot], busyFrame);
    return busyFrame;
}

webrtc::I420VideoFrame* BufferManager::returnBusyBuffer(
    webrtc::I420VideoFrame* frame, uint32_t slot)
{
    assert (slot <= m_maxInput);
    webrtc::I420VideoFrame* busyFrame = reinterpret_cast<webrtc::I420VideoFrame*>(BufferManager::cmpexchange((volatile uint64_t*)&(m_busyQ[slot]), reinterpret_cast<uint64_t>(frame), 0));
    ELOG_TRACE("after returnBusyBuffer: busyQ[%d] is 0x%p, busyFrame is 0x%p", slot, m_busyQ[slot], busyFrame);
    return busyFrame;
}

webrtc::I420VideoFrame* BufferManager::postFreeBuffer(webrtc::I420VideoFrame* frame, uint32_t slot)
{
    assert (slot <= m_maxInput);
    ELOG_TRACE("Posting buffer to slot %d", slot);
    webrtc::I420VideoFrame* busyFrame = reinterpret_cast<webrtc::I420VideoFrame*>(BufferManager::exchange((volatile uint64_t*)&(m_busyQ[slot]), reinterpret_cast<uint64_t>(frame)));
    ELOG_TRACE("after postFreeBuffer: busyQ[%d] is 0x%p,  busyFrame is 0x%p", slot, m_busyQ[slot], busyFrame);
    return busyFrame;
}

} /* namespace mcu */
