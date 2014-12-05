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

using namespace webrtc;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(VCMOutputProcessor, "mcu.media.VCMOutputProcessor");

VCMOutputProcessor::VCMOutputProcessor(int id, woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<TaskRunner> taskRunner)
    : VideoOutputProcessor(id)
    , m_sendFormat(FRAME_FORMAT_UNKNOWN)
{
    init(transport, taskRunner);
}

VCMOutputProcessor::~VCMOutputProcessor()
{
    close();
}

bool VCMOutputProcessor::setSendCodec(FrameFormat frameFormat, VideoSize videoSize)
{
    VideoCodec videoCodec;
    bool validCodec = false;

    // We need to use the default codec setting in the videoEncoder
    // if the default codec type is same as the new one.
    // Some status of the codec is modified inside the videoEncoder
    // and if we set a new codec with the same type, the videoEncoder
    // won't work correctly.
    validCodec = (m_videoEncoder->GetEncoder(&videoCodec) == 0);

    switch (frameFormat) {
    case FRAME_FORMAT_VP8:
        if (!validCodec || videoCodec.codecType != webrtc::kVideoCodecVP8)
            validCodec = (VideoCodingModule::Codec(webrtc::kVideoCodecVP8, &videoCodec) == VCM_OK);
        break;
    case FRAME_FORMAT_H264:
        if (!validCodec || videoCodec.codecType != webrtc::kVideoCodecH264)
            validCodec = (VideoCodingModule::Codec(webrtc::kVideoCodecH264, &videoCodec) == VCM_OK);
        break;
    case FRAME_FORMAT_I420:
    default:
        break;
    }

    if (validCodec) {
        videoCodec.width = videoSize.width;
        videoCodec.height = videoSize.height;
        // TODO: Set startBitrate, minBitrate and maxBitrate of the codec according to the (future) configurable parameters.
        videoCodec.minBitrate = 300 * 1000;

        if (!m_rtpRtcp || m_rtpRtcp->RegisterSendPayload(videoCodec) == -1)
            return false;

        if (m_videoEncoder && m_videoEncoder->SetEncoder(videoCodec) != -1) {
            m_sendFormat = frameFormat;
            return true;
        }
    }

    return false;
}

void VCMOutputProcessor::handleIntraFrameRequest()
{
    m_videoEncoder->SendKeyFrame();
}

uint32_t VCMOutputProcessor::sendSSRC()
{
    return m_rtpRtcp->SSRC();
}

void VCMOutputProcessor::onFrame(FrameFormat format, unsigned char* payload, int len, unsigned int ts)
{
    if (format != FRAME_FORMAT_I420 && format != m_sendFormat) {
        ELOG_INFO("Frame format %d is not supported, ignored.", format);
        return;
    }

    switch (format) {
    case FRAME_FORMAT_I420: {
        // Currently we should only receive I420 format frame.
        I420VideoFrame* composedFrame = reinterpret_cast<I420VideoFrame*>(payload);
        m_videoEncoder->DeliverFrame(m_id, composedFrame);
        break;
    }
    case FRAME_FORMAT_VP8:
        assert(false);
        break;
    case FRAME_FORMAT_H264:
        assert(false);
    default:
        break;
    }
}

int VCMOutputProcessor::deliverFeedback(char* buf, int len)
{
    return m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(buf), len) == -1 ? 0 : len;
}

bool VCMOutputProcessor::init(woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<TaskRunner> taskRunner)
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
    configuration.id = m_id;
    configuration.outgoing_transport = transport;
    configuration.audio = false;  // Video.
    configuration.default_module = m_videoEncoder->SendRtpRtcpModule();
    configuration.intra_frame_callback = m_videoEncoder.get();
    configuration.bandwidth_callback = m_bandwidthObserver.get();
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));

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

    return true;
}

void VCMOutputProcessor::close()
{
    m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
}

}
