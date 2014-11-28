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

#include "SoftwareVideoMixer.h"

#include "BufferManager.h"
#include "VCMMediaProcessorHelper.h"
#include "VideoCompositor.h"

#include <webrtc/common.h>
#include <webrtc/system_wrappers/interface/clock.h>
#include <webrtc/system_wrappers/interface/tick_util.h>
#include <webrtc/common_video/interface/i420_video_frame.h>

using namespace webrtc;

namespace mcu {

SoftwareVideoMixer::SoftwareVideoMixer()
    : m_maxSlot(0)
    , m_receiver(nullptr)
{
    m_ntpDelta = Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() -
                                  TickTime::MillisecondTimestamp();

    m_bufferManager.reset(new BufferManager());
    m_videoCompositor.reset(new SoftVideoCompositor(m_bufferManager));
    m_jobTimer.reset(new JobTimer(30, this));
}

SoftwareVideoMixer::~SoftwareVideoMixer()
{
    m_receiver = nullptr;
}

void SoftwareVideoMixer::setBitrate(FrameFormat format, unsigned short bitrate)
{
}

void SoftwareVideoMixer::requestKeyFrame(FrameFormat format)
{
}

void SoftwareVideoMixer::setLayout(struct VideoLayout& layout)
{
    m_videoCompositor->config(layout);
}

bool SoftwareVideoMixer::activateInput(int slot, FrameFormat format, VideoMixInProvider* provider)
{
    assert(format == FRAME_FORMAT_I420);

    m_bufferManager->setActive(slot, true);
    updateMaxSlot(m_bufferManager->activeSlots());
    return true;
}

void SoftwareVideoMixer::deActivateInput(int slot)
{
    m_bufferManager->setActive(slot, false);
    updateMaxSlot(m_bufferManager->activeSlots());
}

void SoftwareVideoMixer::pushInput(int slot, unsigned char* payload, int len)
{
    I420VideoFrame* frame = reinterpret_cast<I420VideoFrame*>(payload);
    I420VideoFrame* freeFrame = m_bufferManager->getFreeBuffer();
    if (freeFrame) {
        freeFrame->CopyFrame(*frame);
        I420VideoFrame* busyFrame = m_bufferManager->postFreeBuffer(freeFrame, slot);
        if (busyFrame)
            m_bufferManager->releaseBuffer(busyFrame);
    }
}

bool SoftwareVideoMixer::activateOutput(FrameFormat format, unsigned int framerate, unsigned short bitrate, VideoMixOutConsumer* receiver)
{
    assert(format == FRAME_FORMAT_I420);
    m_receiver = receiver;
    return true;
}

void SoftwareVideoMixer::deActivateOutput(FrameFormat format)
{
    m_receiver = nullptr;
}

void SoftwareVideoMixer::onTimeout()
{
    generateFrame();
}

void SoftwareVideoMixer::generateFrame()
{
    if (m_receiver) {
        I420VideoFrame* composedFrame = m_videoCompositor->layout();
        composedFrame->set_render_time_ms(TickTime::MillisecondTimestamp() - m_ntpDelta);
        m_receiver->onFrame(FRAME_FORMAT_I420, reinterpret_cast<unsigned char*>(composedFrame), sizeof(I420VideoFrame), 0);
    }
}

void SoftwareVideoMixer::updateMaxSlot(int newMaxSlot)
{
    m_maxSlot = newMaxSlot;
    VideoLayout layout;
    m_videoCompositor->getLayout(layout);
    if (newMaxSlot <= 1)
        layout.divFactor = 1;
    else if (newMaxSlot <= 4)
        layout.divFactor = 2;
    else if (newMaxSlot <= 9)
        layout.divFactor = 3;
    else
        layout.divFactor = 4;
    m_videoCompositor->config(layout);
}

}
