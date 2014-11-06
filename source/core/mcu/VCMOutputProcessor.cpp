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

DEFINE_LOGGER(VCMOutputProcessor, "mcu.VCMOutputProcessor");

VCMOutputProcessor::VCMOutputProcessor(int id)
    : m_id(id)
    , m_isClosing(false)
    , m_maxSlot(0)
    , m_videoCompositor(nullptr)
    , m_recordStarted(false)
{
    m_ntpDelta = Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() -
                                  TickTime::MillisecondTimestamp();
    m_recorder.reset(new DebugRecorder());
}

VCMOutputProcessor::~VCMOutputProcessor()
{
    close();
}

bool VCMOutputProcessor::init(woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<BufferManager> bufferManager, boost::shared_ptr<TaskRunner> taskRunner)
{
    m_taskRunner = taskRunner;
    m_bufferManager = bufferManager;
    m_videoTransport.reset(transport);

    m_bitrateController.reset(webrtc::BitrateController::CreateBitrateController(Clock::GetRealTimeClock(), true));
    m_bandwidthObserver.reset(m_bitrateController->CreateRtcpBandwidthObserver());
    // FIXME: "config" is currently not respected.
    // The number of cores is currently hard coded to be 4. Need a function to get the correct information from the system.
    webrtc::Config config;
    m_videoEncoder.reset(new ViEEncoder(m_id, -1, 4, config, *(m_taskRunner->unwrap()), m_bitrateController.get()));
    m_videoEncoder->Init();

    RtpRtcp::Configuration configuration;
    configuration.id = ViEModuleId(m_id);
    configuration.outgoing_transport = transport;
    configuration.audio = false;  // Video.
    configuration.default_module = m_videoEncoder->SendRtpRtcpModule();
    configuration.intra_frame_callback = m_videoEncoder.get();
    configuration.bandwidth_callback = m_bandwidthObserver.get();
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));

    m_videoCompositor.reset(new SoftVideoCompositor(m_bufferManager));
    VideoLayout layout;
    layout.rootsize = vga;
    layout.divFactor = 1;
    m_videoCompositor->config(layout);

    VideoCodec videoCodec;
    if (m_videoEncoder->GetEncoder(&videoCodec) == 0) {
        videoCodec.width = VideoSizes[layout.rootsize].width;
        videoCodec.height = VideoSizes[layout.rootsize].height;
        // TODO: Set startBitrate, minBitrate and maxBitrate of the codec according to the (future) configurable parameters.
        if (!setSendVideoCodec(videoCodec))
            return false;
    } else
        assert(false);

    // Enable FEC.
    // TODO: the parameters should be dynamically adjustable.
    m_rtpRtcp->SetGenericFECStatus(true, RED_90000_PT, ULP_90000_PT);
    // Enable NACK.
    // TODO: the parameters should be dynamically adjustable.
    m_rtpRtcp->SetStorePacketsStatus(true, webrtc::kSendSidePacketHistorySize);
    m_videoEncoder->UpdateProtectionMethod(true);

    // Register the SSRC so that the video encoder is able to respond to
    // the intra frame request to a given SSRC.
    std::list<uint32_t> ssrcs;
    ssrcs.push_back(m_rtpRtcp->SSRC());
    m_videoEncoder->SetSsrcs(ssrcs);

    m_taskRunner->RegisterModule(m_rtpRtcp.get());
    Config::get()->registerListener(this);

    m_recordStarted = false;

    // FIXME: Get rid of the hard coded timer interval here.
    // Also it may need to be associated with the target fps configured in VPM.
    m_timer.reset(new boost::asio::deadline_timer(m_ioService, boost::posix_time::milliseconds(33)));
    m_timer->async_wait(boost::bind(&VCMOutputProcessor::layoutTimerHandler, this, boost::asio::placeholders::error));
    m_encodingThread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService)));

    return true;
}

void VCMOutputProcessor::close()
{
    m_isClosing = true;
    Config::get()->unregisterListener(this);
    m_timer->cancel();
    m_encodingThread->join();
}

// A return value of false is interpreted as that the function has no
// more work to do and that the thread can be released.
void VCMOutputProcessor::layoutTimerHandler(const boost::system::error_code& ec)
{
    if (!ec) {
        layoutFrames();
        if (!m_isClosing) {
            // FIXME: Get rid of the hard coded timer interval here.
            // Also it may need to be associated with the target fps configured in VPM.
            m_timer->expires_at(m_timer->expires_at() + boost::posix_time::milliseconds(33));
            m_timer->async_wait(boost::bind(&VCMOutputProcessor::layoutTimerHandler, this, boost::asio::placeholders::error));
        }
    } else {
        ELOG_INFO("VCMOutputProcessor timer error: %s", ec.message().c_str());
    }
}

bool VCMOutputProcessor::setSendVideoCodec(const VideoCodec& videoCodec)
{
    if (!m_rtpRtcp || m_rtpRtcp->RegisterSendPayload(videoCodec) == -1)
        return false;

    return m_videoEncoder ? (m_videoEncoder->SetEncoder(videoCodec) != -1) : false;
}

void VCMOutputProcessor::onConfigChanged()
{
    ELOG_DEBUG("VCMOutputProcessor::onConfigChanged");
    VideoLayout* layout = Config::get()->getVideoLayout();
    m_videoCompositor->config(*layout);

    VideoCodec videoCodec;
    if (m_videoEncoder->GetEncoder(&videoCodec) == 0) {
        videoCodec.width = VideoSizes[layout->rootsize].width;
        videoCodec.height = VideoSizes[layout->rootsize].height;
        if (!setSendVideoCodec(videoCodec))
            assert(!"VCMOutputProcessor::onConfigChanged, setSendVideoCodec error!");
    }
}

void VCMOutputProcessor::onRequestIFrame()
{
    m_videoEncoder->SendKeyFrame();
}

uint32_t VCMOutputProcessor::sendSSRC()
{
    return m_rtpRtcp->SSRC();
}

void VCMOutputProcessor::updateMaxSlot(int newMaxSlot)
{
    m_maxSlot = newMaxSlot;
    VideoLayout layout;
    m_videoCompositor->getLayout(layout);
    if (newMaxSlot == 1)
        layout.divFactor = 1;
    else if (newMaxSlot <= 4)
        layout.divFactor = 2;
    else if (newMaxSlot <= 9)
        layout.divFactor = 3;
    else
        layout.divFactor = 4;
    m_videoCompositor->config(layout);

    ELOG_DEBUG("maxSlot is changed to %d", m_maxSlot);
}

/**
 * this should be called whenever a new frame is decoded from
 * one particular publisher with index
 */
void VCMOutputProcessor::handleInputFrame(webrtc::I420VideoFrame& frame, int index)
{
    I420VideoFrame* freeFrame = m_bufferManager->getFreeBuffer();
    if (freeFrame) {
        freeFrame->CopyFrame(frame);
        I420VideoFrame* busyFrame = m_bufferManager->postFreeBuffer(freeFrame, index);
        if (busyFrame)
            m_bufferManager->releaseBuffer(busyFrame);
    }
}

int VCMOutputProcessor::deliverFeedback(char* buf, int len)
{
    return m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(buf), len);
}


bool VCMOutputProcessor::layoutFrames()
{
    I420VideoFrame* composedFrame = m_videoCompositor->layout(m_maxSlot);
    composedFrame->set_render_time_ms(TickTime::MillisecondTimestamp() - m_ntpDelta);
    m_videoEncoder->DeliverFrame(m_id, composedFrame);
    return true;
}

}
