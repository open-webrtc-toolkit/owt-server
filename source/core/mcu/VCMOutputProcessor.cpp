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

#include "VCMOutputProcessor.h"

#include "TaskRunner.h"

#include <boost/bind.hpp>
#include <webrtc/common.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/system_wrappers/interface/tick_util.h>

using namespace webrtc;
using namespace erizo;

namespace mcu {

VPMPool::VPMPool(unsigned int size)
    : m_size(size)
{
    m_vpms = new VideoProcessingModule*[size];
    memset(m_vpms, 0, sizeof(VideoProcessingModule*) * size);
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
    if (m_vpms[slot] == nullptr) {
        VideoProcessingModule* vpm = VideoProcessingModule::Create(slot);
        vpm->SetInputFrameResampleMode(webrtc::kFastRescaling);
        vpm->SetTargetResolution(m_layout.m_subWidth, m_layout.m_subHeight, 30);
        m_vpms[slot] = vpm;
    }
    return m_vpms[slot];
}

void VPMPool::update(VCMOutputProcessor::Layout& layout)
{
    m_layout  = layout;
    for (unsigned int i = 0; i < m_size; i++) {
        if (m_vpms[i])
            m_vpms[i]->SetTargetResolution(layout.m_subWidth, layout.m_subHeight, 30);
    }
}

DEFINE_LOGGER(VCMOutputProcessor, "media.VCMOutputProcessor");

VCMOutputProcessor::VCMOutputProcessor(int id)
    : m_id(id)
    , m_isClosing(false)
    , m_maxSlot(0)
    , m_layoutLock(CriticalSectionWrapper::CreateCriticalSection())
    , m_recordStarted(false)
    , m_composedFrame(nullptr)
    , m_mockFrame(nullptr)
{
    m_currentLayout.m_subWidth = 640;
    m_currentLayout.m_subHeight = 480;
    m_currentLayout.m_divFactor = 1;
    m_newLayout = m_currentLayout;

    m_ntpDelta = Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() -
                                  TickTime::MillisecondTimestamp();
    m_recorder.reset(new DebugRecorder());
}

VCMOutputProcessor::~VCMOutputProcessor()
{
    m_recorder->Stop();
    close();
}

bool VCMOutputProcessor::init(woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<BufferManager> bufferManager, boost::shared_ptr<TaskRunner> taskRunner)
{
    m_taskRunner = taskRunner;
    m_bufferManager = bufferManager;
    m_videoTransport.reset(transport);
    m_vpmPool.reset(new VPMPool(BufferManager::SLOT_SIZE));
    m_vpmPool->update(m_currentLayout);

    m_bitrateController.reset(webrtc::BitrateController::CreateBitrateController(Clock::GetRealTimeClock(), true));
    m_bandwidthObserver.reset(m_bitrateController->CreateRtcpBandwidthObserver());
    webrtc::Config config;
    m_videoEncoder.reset(new ViEEncoder(m_id, m_id, 4, config, *(m_taskRunner->unwrap()), m_bitrateController.get()));
    m_videoEncoder->Init();

    RtpRtcp::Configuration configuration;
    configuration.id = m_id;
    configuration.outgoing_transport = transport;
    configuration.audio = false;  // Video.
    configuration.default_module = m_videoEncoder->SendRtpRtcpModule();
    configuration.intra_frame_callback = m_videoEncoder.get();
    configuration.bandwidth_callback = m_bandwidthObserver.get();
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));
    m_taskRunner->RegisterModule(m_rtpRtcp.get());

    VideoCodec video_codec;
    if (m_videoEncoder->GetEncoder(&video_codec) == 0) {
        video_codec.width = 640;
        video_codec.height = 480;
        m_videoEncoder->SetEncoder(video_codec);
        m_rtpRtcp->RegisterSendPayload(video_codec);
    } else
        assert(false);

    m_composedFrame = new webrtc::I420VideoFrame();
    m_composedFrame->CreateEmptyFrame(640, 480,
                                      640, 640 / 2, 640 / 2);
    clearFrame(m_composedFrame);
    m_recordStarted = false;

    m_timer.reset(new boost::asio::deadline_timer(m_ioService, boost::posix_time::milliseconds(33)));
    m_timer->async_wait(boost::bind(&VCMOutputProcessor::layoutTimerHandler, this, boost::asio::placeholders::error));
    m_encodingThread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService)));

    return true;
}

void VCMOutputProcessor::close()
{
    m_isClosing = true;
    m_timer->cancel();
    m_encodingThread->join();

    m_videoEncoder->StopDebugRecording();
    if (m_composedFrame) {
        delete m_composedFrame;
        m_composedFrame = nullptr;
    }
    if (m_mockFrame) {
        delete m_mockFrame;
        m_mockFrame = nullptr;
    }
}

// A return value of false is interpreted as that the function has no
// more work to do and that the thread can be released.
void VCMOutputProcessor::layoutTimerHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        layoutFrames();
        if (!m_isClosing) {
            m_timer->expires_at(m_timer->expires_at() + boost::posix_time::milliseconds(33));
            m_timer->async_wait(boost::bind(&VCMOutputProcessor::layoutTimerHandler, this, boost::asio::placeholders::error));
        }
    } else {
        ELOG_INFO("VCMOutputProcessor timer error: %s", ec.message().c_str());
    }
}

bool VCMOutputProcessor::setSendVideoCodec(const VideoCodec& video_codec)
{
    return m_videoEncoder ? (m_videoEncoder->SetEncoder(video_codec) == 0) : false;
}

void VCMOutputProcessor::onRequestIFrame()
{
    m_videoEncoder->SendKeyFrame();
}

void VCMOutputProcessor::updateMaxSlot(int newMaxSlot)
{
    m_maxSlot = newMaxSlot;
    Layout layout;
    if (newMaxSlot == 1)
        layout.m_divFactor = 1;
    else if (newMaxSlot <= 4)
        layout.m_divFactor = 2;
    else if (newMaxSlot <= 9)
        layout.m_divFactor = 3;
    else
        layout.m_divFactor = 4;

    layout.m_subWidth = 640 / layout.m_divFactor;
    layout.m_subHeight = 480 / layout.m_divFactor;

    m_newLayout = layout; // atomic

    ELOG_DEBUG("div_factor is changed to %d,  subWidth is %d, subHeight is %d", m_newLayout.m_divFactor, m_newLayout.m_subWidth, m_newLayout.m_subHeight);
}

#define SCALE_BY_OUTPUT 1
#define SCALE_BY_INPUT 0
#define DEBUG_RECORDING 0

/**
 * this should be called whenever a new frame is decoded from
 * one particular publisher with index
 */
void VCMOutputProcessor::handleInputFrame(webrtc::I420VideoFrame& frame, int index)
{
#if SCALE_BY_OUTPUT
    I420VideoFrame* freeFrame = m_bufferManager->getFreeBuffer();
    if (!freeFrame)
        return;    //no free frame, simply ignore it
    else {
        freeFrame->CopyFrame(frame);
        I420VideoFrame* busyFrame = m_bufferManager->postFreeBuffer(freeFrame, index);
        if (busyFrame) {
            ELOG_DEBUG("handleInputFrame: returning busy frame, index is %d", index);
            m_bufferManager->releaseBuffer(busyFrame);
        }
    }
#elif SCALE_BY_INPUT
    I420VideoFrame* processedFrame = nullptr;
    int ret = vpm_->PreprocessFrame(frame, &processedFrame);
    if (ret == VPM_OK && processedFrame) {
        I420VideoFrame* freeFrame = m_bufferManager->getFreeBuffer();
        if (!freeFrame)
            return;
        else {
            freeFrame->CopyFrame(*processedFrame);
            I420VideoFrame* busyFrame = m_bufferManager->postFreeBuffer(freeFrame, index);
               if (busyFrame)
                   m_bufferManager->releaseBuffer(busyFrame);
        }
    }
#else
//    CriticalSectionScoped cs(cs_.get());
    m_composedFrame->CopyFrame(frame);
    if (!m_mockFrame) {
        m_mockFrame = new webrtc::I420VideoFrame();
        m_mockFrame->CreateEmptyFrame(640, 480,
                                      640, 640 / 2, 640 / 2);
        m_mockFrame->CopyFrame(frame);
    }
#endif
}

void VCMOutputProcessor::clearFrame(webrtc::I420VideoFrame* frame) {
    if (frame) {
        memset(frame->buffer(webrtc::kYPlane), 0x00, frame->allocated_size(webrtc::kYPlane));
        memset(frame->buffer(webrtc::kUPlane), 128, frame->allocated_size(webrtc::kUPlane));
        memset(frame->buffer(webrtc::kVPlane), 128, frame->allocated_size(webrtc::kVPlane));
    }
}

bool VCMOutputProcessor::layoutFrames()
{
#if SCALE_BY_OUTPUT
    webrtc::I420VideoFrame* target = m_composedFrame;
    for (int input = 0; input < m_maxSlot; input++) {
        if ((input == 0) && !(m_currentLayout == m_newLayout)) {
            // commit new layout config at the beginning of each iteration
            m_currentLayout = m_newLayout;
            m_vpmPool->update(m_currentLayout);
            clearFrame(m_composedFrame);
        }
        unsigned int offset_width = (input%m_currentLayout.m_divFactor) * m_currentLayout.m_subWidth;
        unsigned int offset_height = (input/m_currentLayout.m_divFactor) * m_currentLayout.m_subHeight;
        webrtc::I420VideoFrame* sub_image = m_bufferManager->getBusyBuffer(input);
        if (!sub_image) {
            for (int i = 0; i < m_currentLayout.m_subHeight; i++) {
                memset(target->buffer(webrtc::kYPlane) + (i+offset_height) * target->stride(webrtc::kYPlane) + offset_width,
                    0,
                    m_currentLayout.m_subWidth);
            }

            for (int i = 0; i < m_currentLayout.m_subHeight/2; i++) {
                memset(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                    128,
                    m_currentLayout.m_subWidth/2);
                memset(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                    128,
                    m_currentLayout.m_subWidth/2);
            }
            continue;
        }
        // do the scale first
        else {
            I420VideoFrame* processedFrame = nullptr;
            int ret = m_vpmPool->get(input)->PreprocessFrame(*sub_image, &processedFrame);
            if (ret == VPM_OK && !processedFrame) {
                // do nothing
            } else if (ret == VPM_OK && processedFrame) {
               for (int i = 0; i < m_currentLayout.m_subHeight; i++) {
                  memcpy(target->buffer(webrtc::kYPlane) + (i+offset_height)* target->stride(webrtc::kYPlane) + offset_width,
                        processedFrame->buffer(webrtc::kYPlane) + i * processedFrame->stride(webrtc::kYPlane),
                        m_currentLayout.m_subWidth);
               }

               for (int i = 0; i < m_currentLayout.m_subHeight/2; i++) {
                  memcpy(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                        processedFrame->buffer(webrtc::kUPlane) + i * processedFrame->stride(webrtc::kUPlane),
                        m_currentLayout.m_subWidth/2);
                  memcpy(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                        processedFrame->buffer(webrtc::kVPlane) + i * processedFrame->stride(webrtc::kVPlane),
                        m_currentLayout.m_subWidth/2);
               }
            }
            // if return busy frame failed, which means a new busy frame has been posted
            // simply release the busy frame
            if (m_bufferManager->returnBusyBuffer(sub_image, input)) {
                m_bufferManager->releaseBuffer(sub_image);
                ELOG_DEBUG("releasing busyFrame[%d]", input);
            }
        }

#if DEBUG_RECORDING
        if (m_recordStarted == false) {
            m_videoEncoder->StartDebugRecording("/home/qzhang8/webrtc/encoded.frame.i420");
            m_recordStarted = true;
        }
#endif
    }

    m_composedFrame->set_render_time_ms(TickTime::MillisecondTimestamp() - m_ntpDelta);
    m_videoEncoder->DeliverFrame(m_id, m_composedFrame);
#elif SCALE_BY_INPUT
//    CriticalSectionScoped cs(m_layoutLock.get());
    webrtc::I420VideoFrame* target = m_composedFrame;
    for (int input = 0; input < m_maxSlot; input++) {
        if (input == 0) {
            // commit new layout config at the beginning of each iteration
            m_currentLayout = m_newLayout;
        }
        unsigned int offset_width = (input%m_currentLayout.m_divFactor) * m_currentLayout.m_subWidth;
        unsigned int offset_height = (input/m_currentLayout.m_divFactor) * m_currentLayout.m_subHeight;
        webrtc::I420VideoFrame* sub_image = m_bufferManager->getBusyBuffer(input);
        if (!sub_image) {
            for (int i = 0; i < m_currentLayout.m_subHeight; i++) {
                memset(target->buffer(webrtc::kYPlane) + (i+offset_height) * target->stride(webrtc::kYPlane) + offset_width,
                    0,
                    m_currentLayout.m_subWidth);
            }

            for (int i = 0; i < m_currentLayout.m_subHeight/2; i++) {
                memset(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                    128,
                    m_currentLayout.m_subWidth/2);
                memset(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                    128,
                    m_currentLayout.m_subWidth/2);
            }
            continue;
        }
        // check whether the layout should be changed
        if (sub_image->width() != m_currentLayout.m_subWidth || sub_image->height() != m_currentLayout.m_subHeight) {
            m_bufferManager->releaseBuffer(sub_image);
            ELOG_DEBUG("busyFrame[%d] is returned due to layout change", input);
        }
        for (int i = 0; i < m_currentLayout.m_subHeight; i++) {
            memcpy(target->buffer(webrtc::kYPlane) + (i+offset_height)* target->stride(webrtc::kYPlane) + offset_width,
                sub_image->buffer(webrtc::kYPlane) + i * sub_image->stride(webrtc::kYPlane),
                m_currentLayout.m_subWidth);
        }

        for (int i = 0; i < m_currentLayout.m_subHeight/2; i++) {
            memcpy(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                sub_image->buffer(webrtc::kUPlane) + i * sub_image->stride(webrtc::kUPlane),
                m_currentLayout.m_subWidth/2);
            memcpy(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                sub_image->buffer(webrtc::kVPlane) + i * sub_image->stride(webrtc::kVPlane),
                m_currentLayout.m_subWidth/2);
        }
        // if return busy frame failed, which means a new busy frame has been posted
        // simply release the busy frame
        if (m_bufferManager->returnBusyBuffer(sub_image, input))
            m_bufferManager->releaseBuffer(sub_image);

        if (m_recordStarted == false) {
            m_videoEncoder->StartDebugRecording("/home/qzhang8/webrtc/encoded.frame.i420");
            m_recordStarted = true;
        }
    }

    m_composedFrame->set_render_time_ms(TickTime::MillisecondTimestamp() - m_ntpDelta);
    m_videoEncoder->DeliverFrame(m_id, m_composedFrame);
#else
    if (m_recordStarted == true) {
        m_videoEncoder->StartDebugRecording("/home/qzhang8/webrtc/encoded.frame.i420");
        m_recordStarted = true;
    }
    if (m_mockFrame)
        m_videoEncoder->DeliverFrame(m_mockFrame, m_id);
#endif
    return true;
}

}

