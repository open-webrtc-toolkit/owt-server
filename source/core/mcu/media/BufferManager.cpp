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

BufferManager::BufferManager()
{
    for (int i = 0; i < SLOT_SIZE*2; i++) {
        webrtc::I420VideoFrame* buffer =  new webrtc::I420VideoFrame();
        buffer->CreateEmptyFrame(640, 480, 640, 640/2, 640/2);
        m_freeQ.push(buffer);
    }

    for (int i = 0; i < SLOT_SIZE; i++)
        m_busyQ[i] = nullptr;

    ELOG_DEBUG("BufferManager constructed")
}

BufferManager::~BufferManager()
{
    webrtc::I420VideoFrame* buffer = nullptr;
    while (m_freeQ.pop(buffer))
        delete buffer;
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

webrtc::I420VideoFrame* BufferManager::getBusyBuffer(int slot)
{
    assert (slot <= SLOT_SIZE);
    webrtc::I420VideoFrame* busyFrame = reinterpret_cast<webrtc::I420VideoFrame*>(BufferManager::exchange((volatile uint64_t*)&(m_busyQ[slot]), 0));
    ELOG_TRACE("getBusyBuffer: busyQ[%d] is 0x%p, busyFrame is 0x%p", slot, m_busyQ[slot], busyFrame);
    return busyFrame;
}

webrtc::I420VideoFrame* BufferManager::returnBusyBuffer(
    webrtc::I420VideoFrame* frame, int slot)
{
    assert (slot <= SLOT_SIZE);
    webrtc::I420VideoFrame* busyFrame = reinterpret_cast<webrtc::I420VideoFrame*>(BufferManager::cmpexchange((volatile uint64_t*)&(m_busyQ[slot]), reinterpret_cast<uint64_t>(frame), 0));
    ELOG_TRACE("after returnBusyBuffer: busyQ[%d] is 0x%p, busyFrame is 0x%p", slot, m_busyQ[slot], busyFrame);
    return busyFrame;
}

webrtc::I420VideoFrame* BufferManager::postFreeBuffer(webrtc::I420VideoFrame* frame, int slot)
{
    assert (slot <= SLOT_SIZE);
    ELOG_TRACE("Posting buffer to slot %d", slot);
    webrtc::I420VideoFrame* busyFrame = reinterpret_cast<webrtc::I420VideoFrame*>(BufferManager::exchange((volatile uint64_t*)&(m_busyQ[slot]), reinterpret_cast<uint64_t>(frame)));
    ELOG_TRACE("after postFreeBuffer: busyQ[%d] is 0x%p,  busyFrame is 0x%p", slot, m_busyQ[slot], busyFrame);
    return busyFrame;
}

} /* namespace mcu */
