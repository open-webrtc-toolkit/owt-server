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

#include "TaskRunner.h"

using namespace webrtc;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(ExternalVideoProcessor, "mcu.ExternalVideoProcessor");

ExternalVideoProcessor::ExternalVideoProcessor(int id)
    : VideoOutputProcessor(id)
{
}

ExternalVideoProcessor::~ExternalVideoProcessor()
{
    close();
}

bool ExternalVideoProcessor::init(woogeen_base::WoogeenTransport<erizo::VIDEO>* transport, boost::shared_ptr<TaskRunner> taskRunner)
{
    m_taskRunner = taskRunner;
    m_videoTransport.reset(transport);

    RtpRtcp::Configuration configuration;
    configuration.id = m_id;
    configuration.outgoing_transport = transport;
    configuration.audio = false;  // Video.
    configuration.intra_frame_callback = this;
    // TODO: Add bitrate observation and control later.
    configuration.bandwidth_callback = nullptr;
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));

    // Enable FEC.
    // TODO: the parameters should be dynamically adjustable.
    m_rtpRtcp->SetGenericFECStatus(true, RED_90000_PT, ULP_90000_PT);
    // Enable NACK.
    // TODO: the parameters should be dynamically adjustable.
    m_rtpRtcp->SetStorePacketsStatus(true, 600);

    m_taskRunner->RegisterModule(m_rtpRtcp.get());
    Config::get()->registerListener(this);

    return true;
}

void ExternalVideoProcessor::close()
{
    Config::get()->unregisterListener(this);
    m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
}

bool ExternalVideoProcessor::setSendVideoCodec(const VideoCodec& videoCodec)
{
    return m_rtpRtcp && m_rtpRtcp->RegisterSendPayload(videoCodec) != -1;
}

void ExternalVideoProcessor::onConfigChanged()
{
    ELOG_DEBUG("onConfigChanged");
}

void ExternalVideoProcessor::onRequestIFrame()
{
}

uint32_t ExternalVideoProcessor::sendSSRC()
{
    return m_rtpRtcp->SSRC();
}

int ExternalVideoProcessor::sendFrame(char* payload, int len)
{
    // TODO: Invoke m_rtpRtcp->SendOutgoingData.
    // Need to provide the necessary parameters like payload type,
    // timestamp and capture time etc.
    return -1;
}

int ExternalVideoProcessor::deliverFeedback(char* buf, int len)
{
    return m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(buf), len) == -1 ? 0 : len;
}

void ExternalVideoProcessor::OnReceivedIntraFrameRequest(uint32_t ssrc)
{
    // TODO: Send I-Frame request to the encoder.
    // May need to valid the ssrc.
}

}
