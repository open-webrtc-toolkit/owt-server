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

#include "MediaUtilities.h"

#include <webrtc/common.h>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>

using namespace webrtc;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(VCMOutputProcessor, "mcu.media.VCMOutputProcessor");

VCMOutputProcessor::VCMOutputProcessor(boost::shared_ptr<woogeen_base::VideoFrameProvider> source, int targetKbps, woogeen_base::WebRTCTransport<erizo::VIDEO>* transport, boost::shared_ptr<woogeen_base::WebRTCTaskRunner> taskRunner)
    : m_id(-1)
    , m_sendFormat(woogeen_base::FRAME_FORMAT_UNKNOWN)
    , m_targetKbps(targetKbps)
    , m_source(source)
{
    init(transport, taskRunner);
}

VCMOutputProcessor::~VCMOutputProcessor()
{
    close();
}

bool VCMOutputProcessor::updateVideoSize(unsigned int width, unsigned int height)
{
    VideoCodec videoCodec;
    bool validCodec = false;
    validCodec = (m_videoEncoder->GetEncoder(&videoCodec) == 0);

    if (validCodec) {
        videoCodec.width = width;
        videoCodec.height = height;

        uint32_t targetKbps = 0;
        if (m_targetKbps > 0)
            targetKbps = m_targetKbps;
        else
            targetKbps = woogeen_base::calcBitrate(videoCodec.width, videoCodec.height) * (m_sendFormat == woogeen_base::FRAME_FORMAT_VP8 ? 0.9 : 1);

        videoCodec.maxBitrate = targetKbps;
        videoCodec.minBitrate = targetKbps / 4;
        videoCodec.startBitrate = targetKbps;

        return (m_videoEncoder->SetEncoder(videoCodec) != -1);
    }

    return false;
}

bool VCMOutputProcessor::setSendCodec(woogeen_base::FrameFormat frameFormat, unsigned int width, unsigned int height)
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
    case woogeen_base::FRAME_FORMAT_VP8:
        if (!validCodec || videoCodec.codecType != webrtc::kVideoCodecVP8)
            validCodec = (VideoCodingModule::Codec(webrtc::kVideoCodecVP8, &videoCodec) == VCM_OK);
        break;
    case woogeen_base::FRAME_FORMAT_H264:
        if (!validCodec || videoCodec.codecType != webrtc::kVideoCodecH264)
            validCodec = (VideoCodingModule::Codec(webrtc::kVideoCodecH264, &videoCodec) == VCM_OK);
        break;
    case woogeen_base::FRAME_FORMAT_I420:
    default:
        break;
    }

    if (validCodec) {
        videoCodec.width = width;
        videoCodec.height = height;

        uint32_t targetKbps = 0;
        if (m_targetKbps > 0)
            targetKbps = m_targetKbps;
        else
            targetKbps = woogeen_base::calcBitrate(videoCodec.width, videoCodec.height) * (m_sendFormat == woogeen_base::FRAME_FORMAT_VP8 ? 0.9 : 1);

        videoCodec.maxBitrate = targetKbps;
        videoCodec.minBitrate = targetKbps / 4;
        videoCodec.startBitrate = targetKbps;

        std::list<RtpRtcp*>::iterator it = m_rtpRtcps.begin();
        for (; it != m_rtpRtcps.end(); ++it) {
            if ((*it)->RegisterSendPayload(videoCodec) == -1)
                return false;
        }

        if (m_videoEncoder->SetEncoder(videoCodec) != -1) {
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

bool VCMOutputProcessor::startSend(bool nack, bool fec)
{
    std::list<uint32_t> ssrcs;
    std::list<RtpRtcp*>::iterator it = m_rtpRtcps.begin();
    for (; it != m_rtpRtcps.end(); ++it) {
        bool fecEnabled = false;
        uint8_t dummyRedPayloadType = 0;
        uint8_t dummyFecPayloadType = 0;
        (*it)->GenericFECStatus(fecEnabled, dummyRedPayloadType, dummyFecPayloadType);
        bool nackEnabled = (*it)->StorePackets();

        if (nackEnabled == nack && fecEnabled == fec)
            return true;

        ssrcs.push_back((*it)->SSRC());
    }

    RtpRtcp::Configuration configuration;
    configuration.id = m_id + m_rtpRtcps.size();
    configuration.outgoing_transport = m_videoTransport.get();
    configuration.audio = false;  // Video.
    configuration.default_module = m_videoEncoder->SendRtpRtcpModule();
    configuration.intra_frame_callback = m_videoEncoder.get();
    configuration.bandwidth_callback = m_bandwidthObserver.get();
    RtpRtcp* rtpRtcp = RtpRtcp::CreateRtpRtcp(configuration);

    // Enable/Disable FEC.
    rtpRtcp->SetGenericFECStatus(fec, RED_90000_PT, ULP_90000_PT);

    // Enable/Disable NACK.
    rtpRtcp->SetStorePacketsStatus(nack, webrtc::kSendSidePacketHistorySize);

    if (nack || fec)
        m_videoEncoder->UpdateProtectionMethod(nack || m_videoEncoder->nack_enabled());

    VideoCodec videoCodec;
    if (m_videoEncoder->GetEncoder(&videoCodec) == 0)
        rtpRtcp->RegisterSendPayload(videoCodec);

    // Register the SSRC so that the video encoder is able to respond to
    // the intra frame request to a given SSRC.
    ssrcs.push_back(rtpRtcp->SSRC());
    m_videoEncoder->SetSsrcs(ssrcs);

    m_taskRunner->RegisterModule(rtpRtcp);

    m_rtpRtcps.push_back(rtpRtcp);
    m_payloadRouter->SetSendingRtpModules(m_rtpRtcps);
    m_payloadRouter->set_active(true);

    return true;
}

bool VCMOutputProcessor::stopSend(bool nack, bool fec)
{
    std::list<RtpRtcp*>::iterator it = m_rtpRtcps.begin();
    for (; it != m_rtpRtcps.end(); ++it) {
        bool fecEnabled = false;
        uint8_t dummyRedPayloadType = 0;
        uint8_t dummyFecPayloadType = 0;
        (*it)->GenericFECStatus(fecEnabled, dummyRedPayloadType, dummyFecPayloadType);
        bool nackEnabled = (*it)->StorePackets();

        if (nackEnabled == nack && fecEnabled == fec) {
            m_taskRunner->DeRegisterModule(*it);
            m_rtpRtcps.erase(it);
            m_payloadRouter->SetSendingRtpModules(m_rtpRtcps);
            delete *it;
            // FIXME: This is not accurate.
            // We should change the NACK enabling status if there's no NACK enabled
            // stream now.
            m_videoEncoder->UpdateProtectionMethod(m_videoEncoder->nack_enabled());
            break;
        }
    }

    return true;
}

uint32_t VCMOutputProcessor::sendSSRC(bool nack, bool fec)
{
    std::list<RtpRtcp*>::iterator it = m_rtpRtcps.begin();
    for (; it != m_rtpRtcps.end(); ++it) {
        bool fecEnabled = false;
        uint8_t dummyRedPayloadType = 0;
        uint8_t dummyFecPayloadType = 0;
        (*it)->GenericFECStatus(fecEnabled, dummyRedPayloadType, dummyFecPayloadType);
        bool nackEnabled = (*it)->StorePackets();

        if (nackEnabled == nack && fecEnabled == fec)
            return (*it)->SSRC();
    }

    return 0;
}

void VCMOutputProcessor::onFrame(const woogeen_base::Frame& frame)
{
    if (frame.format != woogeen_base::FRAME_FORMAT_I420 && frame.format != m_sendFormat) {
        ELOG_INFO("Frame format %d is not supported, ignored.", frame.format);
        return;
    }

    switch (frame.format) {
    case woogeen_base::FRAME_FORMAT_I420: {
        // Currently we should only receive I420 format frame.
        I420VideoFrame* compositedFrame = reinterpret_cast<I420VideoFrame*>(frame.payload);
        std::vector<uint32_t> csrcs;
        m_videoEncoder->DeliverFrame(m_id, compositedFrame, csrcs);
        break;
    }
    case woogeen_base::FRAME_FORMAT_VP8:
        assert(false);
        break;
    case woogeen_base::FRAME_FORMAT_H264:
        assert(false);
    default:
        break;
    }
}

int VCMOutputProcessor::deliverFeedback(char* buf, int len)
{
    std::list<RtpRtcp*>::iterator it = m_rtpRtcps.begin();
    for (; it != m_rtpRtcps.end(); ++it)
        (*it)->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(buf), len);
    return len;
}

bool VCMOutputProcessor::init(woogeen_base::WebRTCTransport<erizo::VIDEO>* transport, boost::shared_ptr<woogeen_base::WebRTCTaskRunner> taskRunner)
{
    m_taskRunner = taskRunner;
    m_videoTransport.reset(transport);
    m_id = m_source->addFrameConsumer("VCMOutputProcessor", woogeen_base::FRAME_FORMAT_I420, this);

    m_bitrateController.reset(webrtc::BitrateController::CreateBitrateController(Clock::GetRealTimeClock(), true));
    m_bandwidthObserver.reset(m_bitrateController->CreateRtcpBandwidthObserver());
    m_payloadRouter = new PayloadRouter();
    // FIXME: "config" is currently not respected.
    // The number of cores is currently hard coded to be 4. Need a function to get the correct information from the system.
    webrtc::Config config;
    m_videoEncoder.reset(new ViEEncoder(m_id, -1, 4, config, *(m_taskRunner->unwrap()), m_bitrateController.get(), false));
    m_videoEncoder->Init();
    m_videoEncoder->StartThreadsAndSetSendPayloadRouter(m_payloadRouter);

    return true;
}

void VCMOutputProcessor::registerPreSendFrameCallback(woogeen_base::FrameConsumer* frameConsumer)
{
    m_encodedFrameCallback.reset(new woogeen_base::VideoEncodedFrameCallbackAdapter(frameConsumer));
    m_videoEncoder->RegisterPostEncodeImageCallback(m_encodedFrameCallback.get());
}

void VCMOutputProcessor::deRegisterPreSendFrameCallback()
{
    if (m_encodedFrameCallback)
        m_videoEncoder->DeRegisterPostEncodeImageCallback();
}

void VCMOutputProcessor::close()
{
    deRegisterPreSendFrameCallback();

    m_videoEncoder->StopThreadsAndRemovePayloadRouter();
    m_payloadRouter->SetSendingRtpModules(std::list<RtpRtcp*>());
    while (m_rtpRtcps.size() > 0) {
        std::list<RtpRtcp*>::iterator it = m_rtpRtcps.begin();
        m_taskRunner->DeRegisterModule(*it);
        delete *it;
        m_rtpRtcps.erase(it);
    }
}

}
