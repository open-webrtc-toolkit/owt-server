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

VPMPool::VPMPool(unsigned int size) {
	size_ = size;
	vpms_ = new VideoProcessingModule*[size];
	memset(vpms_, 0, sizeof(VideoProcessingModule*)*size);
}

VPMPool::~VPMPool() {
	for (unsigned int i = 0; i < size_; i++) {
		if (vpms_[i])
			VideoProcessingModule::Destroy(vpms_[i]);
	}
	delete []vpms_;
}

VideoProcessingModule* VPMPool::get(unsigned int slot) {
	assert (slot < size_);
	if(vpms_[slot] == nullptr) {
		VideoProcessingModule* vpm = VideoProcessingModule::Create(slot);
	    vpm->SetInputFrameResampleMode(webrtc::kFastRescaling);
		vpm->SetTargetResolution(layout_.subWidth_, layout_.subHeight_, 30);
		vpms_[slot] = vpm;
	}
	return vpms_[slot];
}

void VPMPool::update(VCMOutputProcessor::Layout& layout) {
	layout_  = layout;
	for (unsigned int i = 0; i < size_; i++) {
		if (vpms_[i])
			vpms_[i]->SetTargetResolution(layout.subWidth_, layout.subHeight_, 30);
	}

}

DEFINE_LOGGER(VCMOutputProcessor, "media.VCMOutputProcessor");

VCMOutputProcessor::VCMOutputProcessor(int id)
    : id_(id)
    , layoutLock_(CriticalSectionWrapper::CreateCriticalSection())
{
    layout_.subWidth_ = 640;
    layout_.subHeight_ = 480;
    layout_.div_factor_ = 1;
    layoutNew_ = layout_;
    composedFrame_ = NULL;
    encodingThread_.reset();
    timer_.reset();
    maxSlot_ = 0;

    delta_ntp_internal_ms_ = Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() -
              	  	  	  	  TickTime::MillisecondTimestamp();
    // recorder_->Start("/home/qzhang8/webrtc/webrtc.frame.i420");
    recorder_.reset(new DebugRecorder());
    recordStarted_= false;
    mockFrame_ = NULL;
}

VCMOutputProcessor::~VCMOutputProcessor()
{
    recorder_->Stop();
    this->close();
}

bool VCMOutputProcessor::init(woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<BufferManager> bufferManager, boost::shared_ptr<TaskRunner> taskRunner)
{
    taskRunner_ = taskRunner;
    bufferManager_ = bufferManager;
    m_videoTransport.reset(transport);
    vpmPool_.reset(new VPMPool(BufferManager::SLOT_SIZE));
    vpmPool_->update(layout_);

    m_bitrateController.reset(webrtc::BitrateController::CreateBitrateController(Clock::GetRealTimeClock(), true));
    m_bandwidthObserver.reset(m_bitrateController->CreateRtcpBandwidthObserver());
    webrtc::Config config;
    m_videoEncoder.reset(new ViEEncoder(id_, id_, 4, config, *(taskRunner_->unwrap()), m_bitrateController.get()));
    m_videoEncoder->Init();

    RtpRtcp::Configuration configuration;
    configuration.id = id_;
    configuration.outgoing_transport = transport;
    configuration.audio = false;  // Video.
    configuration.default_module = m_videoEncoder->SendRtpRtcpModule();
    configuration.intra_frame_callback = m_videoEncoder.get();
    configuration.bandwidth_callback = m_bandwidthObserver.get();
    default_rtp_rtcp_.reset(RtpRtcp::CreateRtpRtcp(configuration));
    taskRunner_->RegisterModule(default_rtp_rtcp_.get());

    VideoCodec video_codec;
    if (m_videoEncoder->GetEncoder(&video_codec) == 0) {
        video_codec.width = 640;
        video_codec.height = 480;
        m_videoEncoder->SetEncoder(video_codec);
        default_rtp_rtcp_->RegisterSendPayload(video_codec);
    } else {
      assert(false);
    }

    composedFrame_ = new webrtc::I420VideoFrame();
    composedFrame_->CreateEmptyFrame(640, 480,
                                     640, 640/2, 640/2);
    clearFrame(composedFrame_);
    recordStarted_ = false;

    timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::milliseconds(33)));
    timer_->async_wait(boost::bind(&VCMOutputProcessor::layoutTimerHandler, this, boost::asio::placeholders::error));

    encodingThread_.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)));

    return true;
}

void VCMOutputProcessor::close()
{
    timer_->cancel();
    io_service_.stop();
    encodingThread_->join();
    io_service_.reset();
    encodingThread_.reset();

    m_videoEncoder->StopDebugRecording();
    if (composedFrame_) {
        delete composedFrame_, composedFrame_ = NULL;
    }
}

// A return value of false is interpreted as that the function has no
// more work to do and that the thread can be released.
void VCMOutputProcessor::layoutTimerHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        layoutFrames();
        timer_->expires_at(timer_->expires_at() + boost::posix_time::milliseconds(33));
        timer_->async_wait(boost::bind(&VCMOutputProcessor::layoutTimerHandler, this, boost::asio::placeholders::error));
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
    maxSlot_ = newMaxSlot;
    Layout layout;
    if (newMaxSlot == 1) {
        layout.div_factor_ = 1;
    } else if (newMaxSlot <= 4) {
        layout.div_factor_ = 2;
    } else if (newMaxSlot <= 9) {
        layout.div_factor_ = 3;
    } else layout.div_factor_ = 4;

    layout.subWidth_ = 640/layout.div_factor_;
    layout.subHeight_ = 480/layout.div_factor_;

    layoutNew_ = layout;    //atomic

    ELOG_DEBUG("div_factor is changed to %d,  subWidth is %d, subHeight is %d", layoutNew_.div_factor_, layoutNew_.subWidth_, layoutNew_.subHeight_);
}
#define SCALE_BY_OUTPUT 1
#define SCALE_BY_INPUT 0
#define DEBUG_RECORDING 0
/**
 * this should be called whenever a new frame is decoded from
 * one particular publisher with index
 */
bool posted = false;
void VCMOutputProcessor::handleInputFrame(webrtc::I420VideoFrame& frame, int index)
{
#if SCALE_BY_OUTPUT
    I420VideoFrame* freeFrame = bufferManager_->getFreeBuffer();
    if (freeFrame == NULL)
        return;    //no free frame, simply ignore it
    else {
        freeFrame->CopyFrame(frame);
        I420VideoFrame* busyFrame = bufferManager_->postFreeBuffer(freeFrame, index);
        if (busyFrame) {
            ELOG_DEBUG("handleInputFrame: returning busy frame, index is %d", index);
            bufferManager_->releaseBuffer(busyFrame);
        }
        posted = true;
    }
#elif SCALE_BY_INPUT
    I420VideoFrame* processedFrame;
    int ret = vpm_->PreprocessFrame(frame, &processedFrame);
    if (ret == VPM_OK && processedFrame != NULL) {
        I420VideoFrame* freeFrame = bufferManager_->getFreeBuffer();
        if (freeFrame == NULL)
            return;
        else {
            freeFrame->CopyFrame(*processedFrame);
            I420VideoFrame* busyFrame = bufferManager_->postFreeBuffer(freeFrame, index);
               if (busyFrame)
                   bufferManager_->releaseBuffer(busyFrame);
        }
    }
#else
//    CriticalSectionScoped cs(cs_.get());
    composedFrame_->CopyFrame(frame);
    if (mockFrame_ == NULL) {
        mockFrame_ = new webrtc::I420VideoFrame();
        mockFrame_->CreateEmptyFrame(640, 480,
                                     640, 640/2, 640/2);
        mockFrame_->CopyFrame(frame);
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
    webrtc::I420VideoFrame* target = composedFrame_;
    for (int input = 0; input < maxSlot_; input++) {
        if ((input == 0) && !(layout_ == layoutNew_)) {
            // commit new layout config at the beginning of each iteration
            layout_ = layoutNew_;
            vpmPool_->update(layout_);
            clearFrame(composedFrame_);
        }
        unsigned int offset_width = (input%layout_.div_factor_) * layout_.subWidth_;
        unsigned int offset_height = (input/layout_.div_factor_) * layout_.subHeight_;
        webrtc::I420VideoFrame* sub_image = bufferManager_->getBusyBuffer(input);
        if (sub_image == NULL) {
            for (int i = 0; i < layout_.subHeight_; i++) {
                memset(target->buffer(webrtc::kYPlane) + (i+offset_height) * target->stride(webrtc::kYPlane) + offset_width,
                    0,
                    layout_.subWidth_);
            }

            for (int i = 0; i < layout_.subHeight_/2; i++) {
                memset(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                    128,
                    layout_.subWidth_/2);
                memset(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                    128,
                    layout_.subWidth_/2);
            }
            continue;
        }
        // do the scale first
        else {
            I420VideoFrame* processedFrame;
            int ret = vpmPool_->get(input)->PreprocessFrame(*sub_image, &processedFrame);
            if (ret == VPM_OK && processedFrame == NULL) {
                // do nothing
            } else if (ret == VPM_OK && processedFrame != NULL) {
            	for (int i = 0; i < layout_.subHeight_; i++) {
            		memcpy(target->buffer(webrtc::kYPlane) + (i+offset_height)* target->stride(webrtc::kYPlane) + offset_width,
            				processedFrame->buffer(webrtc::kYPlane) + i * processedFrame->stride(webrtc::kYPlane),
            				layout_.subWidth_);
            	}

            	for (int i = 0; i < layout_.subHeight_/2; i++) {
            		memcpy(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
            				processedFrame->buffer(webrtc::kUPlane) + i * processedFrame->stride(webrtc::kUPlane),
            				layout_.subWidth_/2);
            		memcpy(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
            				processedFrame->buffer(webrtc::kVPlane) + i * processedFrame->stride(webrtc::kVPlane),
            				layout_.subWidth_/2);
            	}
            }
            // if return busy frame failed, which means a new busy frame has been posted
            // simply release the busy frame
            if (bufferManager_->returnBusyBuffer(sub_image, input)) {
                bufferManager_->releaseBuffer(sub_image);
                ELOG_DEBUG("releasing busyFrame[%d]", input);
            }
        }

#if DEBUG_RECORDING
        if (recordStarted_ == false) {
            m_videoEncoder->StartDebugRecording("/home/qzhang8/webrtc/encoded.frame.i420");
            recordStarted_ = true;
        }
#endif
    }

    composedFrame_->set_render_time_ms(TickTime::MillisecondTimestamp() - delta_ntp_internal_ms_);
    m_videoEncoder->DeliverFrame(id_, composedFrame_);
#elif SCALE_BY_INPUT
//    CriticalSectionScoped cs(layoutLock_.get());
    webrtc::I420VideoFrame* target = composedFrame_;
    for (int input = 0; input < maxSlot_; input++) {
        if (input == 0) {
            // commit new layout config at the beginning of each iteration
            layout_ = layoutNew_;
        }
        unsigned int offset_width = (input%layout_.div_factor_) * layout_.subWidth_;
        unsigned int offset_height = (input/layout_.div_factor_) * layout_.subHeight_;
        webrtc::I420VideoFrame* sub_image = bufferManager_->getBusyBuffer(input);
        if (sub_image == NULL) {
            for (int i = 0; i < layout_.subHeight_; i++) {
                memset(target->buffer(webrtc::kYPlane) + (i+offset_height) * target->stride(webrtc::kYPlane) + offset_width,
                    0,
                    layout_.subWidth_);
            }

            for (int i = 0; i < layout_.subHeight_/2; i++) {
                memset(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                    128,
                    layout_.subWidth_/2);
                memset(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                    128,
                    layout_.subWidth_/2);
            }
            continue;
        }
        // check whether the layout should be changed
        if (sub_image->width() != layout_.subWidth_ || sub_image->height() != layout_.subHeight_) {
            bufferManager_->releaseBuffer(sub_image);
            ELOG_DEBUG("busyFrame[%d] is returned due to layout change", input);
        }
        for (int i = 0; i < layout_.subHeight_; i++) {
            memcpy(target->buffer(webrtc::kYPlane) + (i+offset_height)* target->stride(webrtc::kYPlane) + offset_width,
                sub_image->buffer(webrtc::kYPlane) + i * sub_image->stride(webrtc::kYPlane),
                layout_.subWidth_);
        }

        for (int i = 0; i < layout_.subHeight_/2; i++) {
            memcpy(target->buffer(webrtc::kUPlane) + (i+offset_height/2) * target->stride(webrtc::kUPlane) + offset_width/2,
                sub_image->buffer(webrtc::kUPlane) + i * sub_image->stride(webrtc::kUPlane),
                layout_.subWidth_/2);
            memcpy(target->buffer(webrtc::kVPlane) + (i+offset_height/2) * target->stride(webrtc::kVPlane) + offset_width/2,
                sub_image->buffer(webrtc::kVPlane) + i * sub_image->stride(webrtc::kVPlane),
                layout_.subWidth_/2);
        }
        // if return busy frame failed, which means a new busy frame has been posted
        // simply release the busy frame
        if (bufferManager_->returnBusyBuffer(sub_image, input))
            bufferManager_->releaseBuffer(sub_image);

        if (recordStarted_ == false) {
            m_videoEncoder->StartDebugRecording("/home/qzhang8/webrtc/encoded.frame.i420");
            recordStarted_ = true;
        }
    }

    composedFrame_->set_render_time_ms(TickTime::MillisecondTimestamp() - delta_ntp_internal_ms_);
    m_videoEncoder->DeliverFrame(id_, composedFrame_);
#else
    if (recordStarted_ == true) {
        m_videoEncoder->StartDebugRecording("/home/qzhang8/webrtc/encoded.frame.i420");
        recordStarted_ = true;
    }
    if (mockFrame_)
        m_videoEncoder->DeliverFrame(mockFrame_, id_);
#endif
    return true;
}

}

