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

#include "VideoCompositor.h"

#include "BufferManager.h"
#include <webrtc/common_video/interface/i420_video_frame.h>
#include <webrtc/modules/video_processing/main/interface/video_processing.h>
#include <webrtc/system_wrappers/interface/critical_section_wrapper.h>

using namespace webrtc;

namespace mcu {

VPMPool::VPMPool(unsigned int size)
    : m_size(size)
{
    m_subVideSize.resize(size);
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

VideoProcessingModule* VPMPool::get(unsigned int slot)
{
    assert (slot < m_size);
    return m_vpms[slot];
}

void VPMPool::update(unsigned int slot, VideoSize& videoSize)
{
    m_subVideSize[slot] = videoSize;
    // FIXME: Get rid of the hard coded fps here.
    // Also it may need to be associated with the layout timer interval configured in VCMOutputProcessor.
    if (m_vpms[slot])
        m_vpms[slot]->SetTargetResolution(videoSize.width, videoSize.height, 30);
}


DEFINE_LOGGER(SoftVideoCompositor, "mcu.SoftVideoCompositor");

SoftVideoCompositor::SoftVideoCompositor(boost::shared_ptr<BufferManager>& bufferManager)
    : m_configLock(CriticalSectionWrapper::CreateCriticalSection())
    , m_configChanged(false)
    , m_bufferManager(bufferManager)
    , m_composedFrame(nullptr)
{
    m_currentLayout.rootsize = vga;    //default is VGA but no region defined
    m_currentLayout.divFactor = 1;
    m_currentLayout.subHeight = VideoSizes[m_currentLayout.rootsize].height;
    m_currentLayout.subWidth = VideoSizes[m_currentLayout.rootsize].width;
    m_vpmPool.reset(new VPMPool(BufferManager::SLOT_SIZE));
    m_composedFrame.reset(new webrtc::I420VideoFrame());
    // create max size  frame
    unsigned int width = VideoSizes[VideoResolutionType::vga].width;
    unsigned int height = VideoSizes[VideoResolutionType::vga].height;
    m_composedFrame->CreateEmptyFrame(width, height,  width, width / 2, width / 2);

    config(m_currentLayout);
}


bool SoftVideoCompositor::config(VideoLayout& layout)
{
    webrtc::CriticalSectionScoped cs(m_configLock.get());
    ELOG_DEBUG("Configuring layout");
    m_newLayout = layout;
    m_configChanged = true;
    ELOG_DEBUG("configChanged is true");
    return true;
}

// only back for now
void SoftVideoCompositor::setBackgroundColor()
{
    if (m_composedFrame) {
        ELOG_DEBUG("setBGColor");
        memset(m_composedFrame->buffer(webrtc::kYPlane), 0x00, m_composedFrame->allocated_size(webrtc::kYPlane));
        memset(m_composedFrame->buffer(webrtc::kUPlane), 128, m_composedFrame->allocated_size(webrtc::kUPlane));
        memset(m_composedFrame->buffer(webrtc::kVPlane), 128, m_composedFrame->allocated_size(webrtc::kVPlane));
    }
}

bool SoftVideoCompositor::commitLayout()
{
    m_currentLayout = m_newLayout;
    VideoSize rootSize = VideoSizes[m_currentLayout.rootsize];
    if (m_currentLayout.regions.empty()) {
        //fluid layout
        VideoSize videoSize;
        videoSize.width = rootSize.width / m_currentLayout.divFactor;
        videoSize.height = rootSize.height / m_currentLayout.divFactor;
        m_currentLayout.subHeight = videoSize.height;
        m_currentLayout.subWidth = videoSize.width;
        for (uint32_t i = 0; i < m_vpmPool->size(); i++)
            m_vpmPool->update(i, videoSize);

        ELOG_DEBUG("commit fluidlayout, rooSize is %d, current subHeight is %d, current subWidth is %d",
                m_currentLayout.rootsize,  m_currentLayout.subHeight, m_currentLayout.subWidth);
    } else { //custom layout
        for (uint32_t i = 0; i < m_currentLayout.regions.size(); i++) {
            Region region = m_currentLayout.regions[i];
            VideoSize videoSize;
            videoSize.width = (int)(rootSize.width*region.relativesize);
            videoSize.height = (int)(rootSize.height*region.relativesize);
            m_vpmPool->update(i, videoSize);
        }
        ELOG_DEBUG("commit customlayout");
    }
    m_configChanged = false;
    ELOG_DEBUG("configChanged is false");
    return true;
}

webrtc::I420VideoFrame* SoftVideoCompositor::layout()
{
    if (m_configChanged) {
        webrtc::CriticalSectionScoped cs(m_configLock.get());
        commitLayout();
        setBackgroundColor();
    }

    if (m_currentLayout.regions.empty())
        return fluidLayout();

    return customLayout();
}

webrtc::I420VideoFrame* SoftVideoCompositor::customLayout()
{
    VideoSize rootSize = VideoSizes[m_currentLayout.rootsize];
    webrtc::I420VideoFrame* target = m_composedFrame.get();

    int input = 0;
    for (int index = 0; index < BufferManager::SLOT_SIZE; ++index) {
        if (!m_bufferManager->isActive(index))
            continue;

        Region region = m_currentLayout.regions[input];

        int sub_width = (int)(rootSize.width*region.relativesize);
        int sub_height = (int)(rootSize.height*region.relativesize);
        unsigned int offset_width = (unsigned int)(rootSize.width*region.left);
        unsigned int offset_height = (unsigned int)(rootSize.height*region.top);

        webrtc::I420VideoFrame* sub_image = m_bufferManager->getBusyBuffer(index);
        if (!sub_image) {
            for (int i = 0; i < sub_height; i++) {
                memset(target->buffer(webrtc::kYPlane) + (i+offset_height) * target->stride(webrtc::kYPlane) + offset_width,
                        0,
                        sub_width);
            }

            for (int i = 0; i < sub_height/2; i++) {
                memset(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                        128,
                        sub_width/2);
                memset(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                        128,
                        sub_width/2);
            }
        } else {
            I420VideoFrame* processedFrame = nullptr;
            int ret = m_vpmPool->get(input)->PreprocessFrame(*sub_image, &processedFrame);
            if (ret == VPM_OK && !processedFrame) {
                // do nothing
            } else if (ret == VPM_OK && processedFrame) {
                for (int i = 0; i < sub_height; i++) {
                    memcpy(target->buffer(webrtc::kYPlane) + (i+offset_height)* target->stride(webrtc::kYPlane) + offset_width,
                            processedFrame->buffer(webrtc::kYPlane) + i * processedFrame->stride(webrtc::kYPlane),
                            sub_width);
                }

                for (int i = 0; i < sub_height/2; i++) {
                    memcpy(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                            processedFrame->buffer(webrtc::kUPlane) + i * processedFrame->stride(webrtc::kUPlane),
                            sub_width/2);
                    memcpy(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                            processedFrame->buffer(webrtc::kVPlane) + i * processedFrame->stride(webrtc::kVPlane),
                            sub_width/2);
                }
            }
            // if return busy frame failed, which means a new busy frame has been posted
            // simply release the busy frame
            if (m_bufferManager->returnBusyBuffer(sub_image, index)) {
                m_bufferManager->releaseBuffer(sub_image);
            }
        }
        ++input;
    }
    return m_composedFrame.get();
}

webrtc::I420VideoFrame* SoftVideoCompositor::fluidLayout()
{
    webrtc::I420VideoFrame* target = m_composedFrame.get();

    int input = 0;
    for (int index = 0; index < BufferManager::SLOT_SIZE; ++index) {
        if (!m_bufferManager->isActive(index))
            continue;

        unsigned int offset_width = (input%m_currentLayout.divFactor) * m_currentLayout.subWidth;
        unsigned int offset_height = (input/m_currentLayout.divFactor) * m_currentLayout.subHeight;
        webrtc::I420VideoFrame* sub_image = m_bufferManager->getBusyBuffer(index);
        if (!sub_image) {
            for (uint32_t i = 0; i < m_currentLayout.subHeight; i++) {
                memset(target->buffer(webrtc::kYPlane) + (i+offset_height) * target->stride(webrtc::kYPlane) + offset_width,
                    0,
                    m_currentLayout.subWidth);
            }

            for (uint32_t i = 0; i < m_currentLayout.subHeight/2; i++) {
                memset(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                    128,
                    m_currentLayout.subWidth/2);
                memset(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                    128,
                    m_currentLayout.subWidth/2);
            }
        } else {
            I420VideoFrame* processedFrame = nullptr;
            int ret = m_vpmPool->get(input)->PreprocessFrame(*sub_image, &processedFrame);
            if (ret == VPM_OK && !processedFrame) {
                // do nothing
            } else if (ret == VPM_OK && processedFrame) {
                for (uint32_t i = 0; i < m_currentLayout.subHeight; i++) {
                    memcpy(target->buffer(webrtc::kYPlane) + (i+offset_height)* target->stride(webrtc::kYPlane) + offset_width,
                        processedFrame->buffer(webrtc::kYPlane) + i * processedFrame->stride(webrtc::kYPlane),
                        m_currentLayout.subWidth);
                }

                for (uint32_t i = 0; i < m_currentLayout.subHeight/2; i++) {
                    memcpy(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                        processedFrame->buffer(webrtc::kUPlane) + i * processedFrame->stride(webrtc::kUPlane),
                        m_currentLayout.subWidth/2);
                    memcpy(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                        processedFrame->buffer(webrtc::kVPlane) + i * processedFrame->stride(webrtc::kVPlane),
                        m_currentLayout.subWidth/2);
                }
            }
            // if return busy frame failed, which means a new busy frame has been posted
            // simply release the busy frame
            if (m_bufferManager->returnBusyBuffer(sub_image, index)) {
                m_bufferManager->releaseBuffer(sub_image);
            }
        }
        ++input;
    }
    return m_composedFrame.get();
};

}
