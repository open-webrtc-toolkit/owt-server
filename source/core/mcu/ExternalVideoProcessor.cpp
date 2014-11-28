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

#include "ExternalVideoProcessor.h"
#include "HardwareVideoMixer.h"

#include "TaskRunner.h"

using namespace webrtc;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(ExternalVideoProcessor, "mcu.ExternalVideoProcessor");

ExternalVideoProcessor::ExternalVideoProcessor(int id, boost::shared_ptr<VideoMixerInterface> mixer, FrameFormat frameFormat)
    : VideoOutputProcessor(id)
    , m_mixer(mixer)
    , m_frameFormat(frameFormat)
{
}

ExternalVideoProcessor::~ExternalVideoProcessor()
{
    close();
}

bool ExternalVideoProcessor::init(woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<TaskRunner> taskRunner, VideoCodecType videoCodecType, VideoSize videoSize)
{
    m_taskRunner = taskRunner;
    m_videoTransport.reset(transport);

    m_bitrateController.reset(webrtc::BitrateController::CreateBitrateController(Clock::GetRealTimeClock(), true));
    m_bandwidthObserver.reset(m_bitrateController->CreateRtcpBandwidthObserver());
    // FIXME: Provide the correct bitrate settings (start, min and max bitrates).
    m_bitrateController->SetBitrateObserver(this, 300 * 1000, 0, 0);

    RtpRtcp::Configuration configuration;
    configuration.id = m_id;
    configuration.outgoing_transport = transport;
    configuration.audio = false;  // Video.
    configuration.intra_frame_callback = this;
    configuration.bandwidth_callback = m_bandwidthObserver.get();
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));

    // Enable FEC.
    // TODO: the parameters should be dynamically adjustable.
    m_rtpRtcp->SetGenericFECStatus(true, RED_90000_PT, ULP_90000_PT);
    // Enable NACK.
    // TODO: the parameters should be dynamically adjustable.
    m_rtpRtcp->SetStorePacketsStatus(true, 600);

    m_taskRunner->RegisterModule(m_rtpRtcp.get());

    VideoCodec videoCodec = {webrtc::kVideoCodecVP8, "VP8", VP8_90000_PT};

    if (videoCodecType == VCT_H264) {
        videoCodec.codecType = webrtc::kVideoCodecH264;
        strcpy(videoCodec.plName, "H264");
        videoCodec.plType = H264_90000_PT;
    }

    return m_rtpRtcp && m_rtpRtcp->RegisterSendPayload(videoCodec) != -1;
}

void ExternalVideoProcessor::close()
{
    if (m_bitrateController)
        m_bitrateController->RemoveBitrateObserver(this);
    m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
}

void ExternalVideoProcessor::onRequestIFrame()
{
    m_mixer->requestKeyFrame(m_frameFormat);
}

uint32_t ExternalVideoProcessor::sendSSRC()
{
    return m_rtpRtcp->SSRC();
}

void ExternalVideoProcessor::OnReceivedIntraFrameRequest(uint32_t ssrc)
{
    m_mixer->requestKeyFrame(m_frameFormat);
}

int ExternalVideoProcessor::deliverFeedback(char* buf, int len)
{
    return m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(buf), len) == -1 ? 0 : len;
}

void ExternalVideoProcessor::OnNetworkChanged(const uint32_t target_bitrate, const uint8_t fraction_loss, const uint32_t rtt)
{
    // TODO: Send the bitrate adjustment request to the encoder.
}

void ExternalVideoProcessor::onFrame(FrameFormat format, unsigned char* payload, int len, unsigned int ts)
{
    webrtc::RTPVideoHeader h;
    h.codec = webrtc::kRtpVideoVp8;
    h.codecHeader.VP8.InitRTPVideoHeaderVP8();

    m_rtpRtcp->SendOutgoingData(webrtc::kVideoFrameKey, 100, ts*90,
                                ts, payload, len, NULL, &h);
}

}
