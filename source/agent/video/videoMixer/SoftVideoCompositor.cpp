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

#include "SoftVideoCompositor.h"

#include <webrtc/common_video/interface/i420_video_frame.h>
#include <webrtc/modules/video_processing/main/interface/video_processing.h>
#include <webrtc/system_wrappers/interface/clock.h>
#include <webrtc/system_wrappers/interface/tick_util.h>

using namespace webrtc;

namespace mcu {

VPMPool::VPMPool(unsigned int size)
    : m_size(size)
{
    m_vpms = new VideoProcessingModule*[size];
    for (unsigned int i = 0; i < m_size; i++) {
        VideoProcessingModule* vpm = VideoProcessingModule::Create(i);
        vpm->SetInputFrameResampleMode(webrtc::kFastRescaling);
        m_vpms[i] = vpm;
    }
}

VPMPool::~VPMPool()
{
    for (unsigned int i = 0; i < m_size; i++) {
        if (m_vpms[i])
            VideoProcessingModule::Destroy(m_vpms[i]);
    }
    delete[] m_vpms;
}

VideoProcessingModule* VPMPool::get(unsigned int input)
{
    assert (input < m_size);
    return m_vpms[input];
}

void VPMPool::update(unsigned int input, VideoSize& videoSize)
{
    // FIXME: Get rid of the hard coded fps here.
    // Also it may need to be associated with the layout timer interval configured in VideoCompositor.
    if (m_vpms[input])
        m_vpms[input]->SetTargetResolution(videoSize.width, videoSize.height, 30);
}

DEFINE_LOGGER(SoftVideoCompositor, "mcu.media.SoftVideoCompositor");

SoftVideoCompositor::SoftVideoCompositor(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, bool crop)
    : m_compositeSize(rootSize)
    , m_bgColor(bgColor)
    , m_solutionState(UN_INITIALIZED)
    , m_crop(crop)
{
    m_ntpDelta = Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() - TickTime::MillisecondTimestamp();
    m_vpmPool.reset(new VPMPool(maxInput));

    // Initialize frame buffer and buffer manager for video composition
    m_compositeFrame.reset(new webrtc::I420VideoFrame());
    m_compositeFrame->CreateEmptyFrame(m_compositeSize.width, m_compositeSize.height, m_compositeSize.width, m_compositeSize.width / 2, m_compositeSize.width / 2);
    m_bufferManager.reset(new BufferManager(maxInput, m_compositeSize.width, m_compositeSize.height));

    m_jobTimer.reset(new woogeen_base::JobTimer(30, this));
    m_jobTimer->start();
}

SoftVideoCompositor::~SoftVideoCompositor()
{
    m_jobTimer->stop();
}

void SoftVideoCompositor::updateRootSize(VideoSize& videoSize)
{
    m_newCompositeSize = videoSize;
    m_solutionState = CHANGING;
}

void SoftVideoCompositor::updateBackgroundColor(YUVColor& bgColor)
{
    m_bgColor = bgColor;
}

void SoftVideoCompositor::updateLayoutSolution(LayoutSolution& solution)
{
    ELOG_DEBUG("Configuring layout");
    m_newLayout = solution;
    m_solutionState = CHANGING;
    ELOG_DEBUG("configChanged is true");
}

bool SoftVideoCompositor::activateInput(int input)
{
    m_bufferManager->setActive(input, true);
    return true;
}

void SoftVideoCompositor::deActivateInput(int input)
{
    m_bufferManager->setActive(input, false);

    // Clean-up the last frame in this slot.
    I420VideoFrame* busyFrame = m_bufferManager->postFreeBuffer(nullptr, input);
    if (busyFrame)
        m_bufferManager->releaseBuffer(busyFrame);
}

void SoftVideoCompositor::pushInput(int input, const Frame& frame)
{
    assert(frame.format == woogeen_base::FRAME_FORMAT_I420);
    webrtc::I420VideoFrame* i420Frame = reinterpret_cast<webrtc::I420VideoFrame*>(frame.payload);

    I420VideoFrame* freeFrame = m_bufferManager->getFreeBuffer();
    if (freeFrame) {
        freeFrame->CopyFrame(*i420Frame);
        I420VideoFrame* busyFrame = m_bufferManager->postFreeBuffer(freeFrame, input);
        if (busyFrame)
            m_bufferManager->releaseBuffer(busyFrame);
    }
}

void SoftVideoCompositor::onTimeout()
{
    generateFrame();
}

void SoftVideoCompositor::generateFrame()
{
    I420VideoFrame* compositeFrame = layout();
    compositeFrame->set_render_time_ms(TickTime::MillisecondTimestamp() + m_ntpDelta);

    woogeen_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = woogeen_base::FRAME_FORMAT_I420;
    frame.payload = reinterpret_cast<uint8_t*>(compositeFrame);
    frame.length = 0; // unused.
    frame.timeStamp = compositeFrame->timestamp();
    frame.additionalInfo.video.width = compositeFrame->width();
    frame.additionalInfo.video.height = compositeFrame->height();

    deliverFrame(frame);
}

void SoftVideoCompositor::setBackgroundColor()
{
    if (m_compositeFrame) {
        ELOG_TRACE("setBackgroundColor");

        // Set the background color
        memset(m_compositeFrame->buffer(webrtc::kYPlane), m_bgColor.y, m_compositeFrame->allocated_size(webrtc::kYPlane));
        memset(m_compositeFrame->buffer(webrtc::kUPlane), m_bgColor.cb, m_compositeFrame->allocated_size(webrtc::kUPlane));
        memset(m_compositeFrame->buffer(webrtc::kVPlane), m_bgColor.cr, m_compositeFrame->allocated_size(webrtc::kVPlane));
    }
}

bool SoftVideoCompositor::commitLayout()
{
    // Update the current video layout
    // m_compositeSize = m_newCompositeSize;
    m_currentLayout = m_newLayout;
    ELOG_DEBUG("commit customlayout");

    m_solutionState = IN_WORK;
    ELOG_DEBUG("configChanged sets to false after commitLayout!");
    return true;
}

webrtc::I420VideoFrame* SoftVideoCompositor::layout()
{
    if (m_solutionState == CHANGING)
        commitLayout();

    // Update the background color
    setBackgroundColor();
    return customLayout();
}

webrtc::I420VideoFrame* SoftVideoCompositor::customLayout()
{
    webrtc::I420VideoFrame* target = m_compositeFrame.get();
    for (LayoutSolution::iterator it = m_currentLayout.begin(); it != m_currentLayout.end(); ++it) {
        int index = it->input;
        if (!m_bufferManager->isActive(index))
            continue;

        Region region = it->region;
        assert(!(region.relativeSize < 0.0 || region.relativeSize > 1.0)
            && !(region.left < 0.0 || region.left > 1.0)
            && !(region.top < 0.0 || region.top > 1.0));

        unsigned int sub_width = (unsigned int)(m_compositeSize.width * region.relativeSize);
        unsigned int sub_height = (unsigned int)(m_compositeSize.height * region.relativeSize);
        unsigned int offset_width = (unsigned int)(m_compositeSize.width * region.left);
        unsigned int offset_height = (unsigned int)(m_compositeSize.height * region.top);
        if (offset_width + sub_width > m_compositeSize.width)
            sub_width = m_compositeSize.width - offset_width;

        if (offset_height + sub_height > m_compositeSize.height)
            sub_height = m_compositeSize.height - offset_height;

        webrtc::I420VideoFrame* sub_image = m_bufferManager->getBusyBuffer((uint32_t)index);
        if (!sub_image) {
            for (unsigned int i = 0; i < sub_height; i++) {
                memset(target->buffer(webrtc::kYPlane) + (i+offset_height) * target->stride(webrtc::kYPlane) + offset_width,
                    0,
                    sub_width);
            }

            for (unsigned int i = 0; i < sub_height/2; i++) {
                memset(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                    128,
                    sub_width/2);
                memset(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                    128,
                    sub_width/2);
            }
        } else {
            uint32_t cropped_sub_width = sub_width;
            uint32_t cropped_sub_height = sub_height;
            if (!m_crop) {
                // If we are *not* required to crop the video input to fit in its region,
                // we need to adjust the region to be filled to match the input's width/height.
                cropped_sub_width = std::min(sub_width, sub_image->width() * sub_height / sub_image->height());
                cropped_sub_height = std::min(sub_height, sub_image->height() * sub_width / sub_image->width());
            }
            offset_width += ((sub_width - cropped_sub_width) / 2) & ~1;
            offset_height += ((sub_height - cropped_sub_height) / 2) & ~1;
            VideoSize sub_size {cropped_sub_width, cropped_sub_height};
            m_vpmPool->update(index, sub_size);
            I420VideoFrame* processedFrame = nullptr;
            int ret = m_vpmPool->get((unsigned int)index)->PreprocessFrame(*sub_image, &processedFrame);
            if (ret == VPM_OK) {
                if (!processedFrame)
                    processedFrame = sub_image;

                for (unsigned int i = 0; i < cropped_sub_height; i++) {
                    memcpy(target->buffer(webrtc::kYPlane) + (i+offset_height) * target->stride(webrtc::kYPlane) + offset_width,
                        processedFrame->buffer(webrtc::kYPlane) + i * processedFrame->stride(webrtc::kYPlane),
                        cropped_sub_width);
                }

                for (unsigned int i = 0; i < cropped_sub_height/2; i++) {
                    memcpy(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                        processedFrame->buffer(webrtc::kUPlane) + i * processedFrame->stride(webrtc::kUPlane),
                        cropped_sub_width/2);
                    memcpy(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                        processedFrame->buffer(webrtc::kVPlane) + i * processedFrame->stride(webrtc::kVPlane),
                        cropped_sub_width/2);
                }
            }
            // if return busy frame failed, which means a new busy frame has been posted
            // simply release the busy frame
            if (m_bufferManager->returnBusyBuffer(sub_image, (uint32_t)index))
                m_bufferManager->releaseBuffer(sub_image);
        }
    }

    return m_compositeFrame.get();
}


}
