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

#include <webrtc/common.h>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>
#include <webrtc/system_wrappers/interface/tick_util.h>

using namespace webrtc;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(VCMOutputProcessor, "mcu.VCMOutputProcessor");

VCMOutputProcessor::VCMOutputProcessor(int id)
    : VideoOutputProcessor(id)
{
}

VCMOutputProcessor::~VCMOutputProcessor()
{
    close();
}

bool VCMOutputProcessor::init(woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<TaskRunner> taskRunner, VideoCodecType videoCodecType, VideoSize videoSize)
{
    m_taskRunner = taskRunner;
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

    VideoCodec videoCodec;
    // TODO: enable VP8/H264 in one room later
#if 0
    if (VideoCodingModule::Codec(webrtc::kVideoCodecH264, &videoCodec) == VCM_OK) {
       if (!setVideoSize(videoSize))
           return false;
    }
#else
    if (m_videoEncoder->GetEncoder(&videoCodec) == 0) {
        // TODO: Set startBitrate, minBitrate and maxBitrate of the codec according to the (future) configurable parameters.
        if (!setVideoSize(videoSize))
            return false;
    } else
        assert(false);
#endif

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

    // FIXME: Get rid of the hard coded timer interval here.
    // Also it may need to be associated with the target fps configured in VPM.
    return true;
}

void VCMOutputProcessor::close()
{
    m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
}

bool VCMOutputProcessor::setVideoSize(VideoSize videoSize)
{
    VideoCodec videoCodec;
    if (m_videoEncoder->GetEncoder(&videoCodec) == 0) {
        videoCodec.width = videoSize.width;
        videoCodec.height = videoSize.height;

        if (!m_rtpRtcp || m_rtpRtcp->RegisterSendPayload(videoCodec) == -1)
            return false;
        return m_videoEncoder ? (m_videoEncoder->SetEncoder(videoCodec) != -1) : false;
    }
    return false;
}

void VCMOutputProcessor::onRequestIFrame()
{
    m_videoEncoder->SendKeyFrame();
}

uint32_t VCMOutputProcessor::sendSSRC()
{
    return m_rtpRtcp->SSRC();
}

void VCMOutputProcessor::onFrame(FrameFormat format, unsigned char* payload, int len, unsigned int ts)
{
    if (format == FRAME_FORMAT_I420) {
        I420VideoFrame* composedFrame = reinterpret_cast<I420VideoFrame*>(payload);
        m_videoEncoder->DeliverFrame(m_id, composedFrame);
    } else if (format == FRAME_FORMAT_VP8) {
        webrtc::RTPVideoHeader h;
        h.codec = webrtc::kRtpVideoVp8;
        h.codecHeader.VP8.InitRTPVideoHeaderVP8();
        m_rtpRtcp->SendOutgoingData(webrtc::kVideoFrameKey, 100, ts * 90,
                                    ts, payload, len, nullptr, &h);
    } else {
        // TODO: H264 image should be handled here.
    }
}

int VCMOutputProcessor::deliverFeedback(char* buf, int len)
{
    return m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(buf), len) == -1 ? 0 : len;
}

}
