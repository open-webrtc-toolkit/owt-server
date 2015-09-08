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

#include "EncodedVideoFrameSender.h"

#include "MediaUtilities.h"

using namespace webrtc;
using namespace erizo;

namespace woogeen_base {

// To make it consistent with the webrtc library, we allow packets to be transmitted
// in up to 2 times max video bitrate if the bandwidth estimate allows it.
static const int TRANSMISSION_MAXBITRATE_MULTIPLIER = 2;

DEFINE_LOGGER(EncodedVideoFrameSender, "woogeen.EncodedVideoFrameSender");

EncodedVideoFrameSender::EncodedVideoFrameSender(boost::shared_ptr<VideoFrameProvider> source, FrameFormat frameFormat, int targetKbps, WebRTCTransport<erizo::VIDEO>* transport, boost::shared_ptr<WebRTCTaskRunner> taskRunner, uint16_t width, uint16_t height)
    : m_source(source)
    , m_frameFormat(frameFormat)
    , m_id(-1)
    , m_targetKbps(targetKbps)
    , m_frameConsumer(nullptr)
{
    m_mediaSpecInfo.video = VideoFrameSpecificInfo{
        .width = width,
        .height = height
    };
    init(transport, taskRunner);
}

EncodedVideoFrameSender::~EncodedVideoFrameSender()
{
    m_source->removeFrameConsumer(m_id);
    close();
}

bool EncodedVideoFrameSender::updateVideoSize(unsigned int width, unsigned int height)
{
    uint32_t targetKbps = 0;
    if (m_targetKbps > 0)
        targetKbps = m_targetKbps;
    else
        targetKbps = calcBitrate(width, height) * (m_frameFormat == FRAME_FORMAT_VP8 ? 0.9 : 1);

    uint32_t minKbps = targetKbps / 4;
    // The bitrate controller is accepting "bps".
    m_bitrateController->SetBitrateObserver(this, targetKbps * 1000, minKbps * 1000, TRANSMISSION_MAXBITRATE_MULTIPLIER * targetKbps * 1000);
    m_source->setBitrate(targetKbps, m_id);
    return true;
}

bool EncodedVideoFrameSender::setSendCodec(FrameFormat frameFormat, unsigned int width, unsigned int height)
{
    // The send video format should be identical to the input video format,
    // because we (EncodedVideoFrameSender) don't have the transcoding capability.
    assert(frameFormat == m_frameFormat);

    VideoCodec codec;
    memset(&codec, 0, sizeof(codec));

    switch (frameFormat) {
    case FRAME_FORMAT_VP8:
        codec.codecType = webrtc::kVideoCodecVP8;
        strcpy(codec.plName, "VP8");
        codec.plType = VP8_90000_PT;
        break;
    case FRAME_FORMAT_H264:
        codec.codecType = webrtc::kVideoCodecH264;
        strcpy(codec.plName, "H264");
        codec.plType = H264_90000_PT;
        break;
    case FRAME_FORMAT_I420:
    default:
        break;
    }

    uint32_t targetKbps = 0;
    if (m_targetKbps > 0)
        targetKbps = m_targetKbps;
    else
        targetKbps = calcBitrate(width, height) * (frameFormat == FRAME_FORMAT_VP8 ? 0.9 : 1);

    uint32_t minKbps = targetKbps / 4;
    // The bitrate controller is accepting "bps".
    m_bitrateController->SetBitrateObserver(this, targetKbps * 1000, minKbps * 1000, TRANSMISSION_MAXBITRATE_MULTIPLIER * targetKbps * 1000);
    m_source->setBitrate(targetKbps, m_id);

    return m_rtpRtcp && m_rtpRtcp->RegisterSendPayload(codec) != -1;
}

void EncodedVideoFrameSender::handleIntraFrameRequest()
{
    m_source->requestKeyFrame(m_id);
}

uint32_t EncodedVideoFrameSender::sendSSRC(bool, bool)
{
    return m_rtpRtcp->SSRC();
}

void EncodedVideoFrameSender::OnReceivedIntraFrameRequest(uint32_t ssrc)
{
    m_source->requestKeyFrame(m_id);
}

int EncodedVideoFrameSender::deliverFeedback(char* buf, int len)
{
    return m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(buf), len) == -1 ? 0 : len;
}

void EncodedVideoFrameSender::OnNetworkChanged(const uint32_t target_bitrate, const uint8_t fraction_loss, const int64_t rtt)
{
    m_source->setBitrate(target_bitrate / 1000, m_id);
}

void EncodedVideoFrameSender::onFrame(const Frame& frame)
{
    webrtc::RTPVideoHeader h;

    if (frame.format == FRAME_FORMAT_VP8) {
        h.codec = webrtc::kRtpVideoVp8;
        h.codecHeader.VP8.InitRTPVideoHeaderVP8();
        if (frame.length > 0 && m_frameConsumer)
            m_frameConsumer->onFrame(frame);
        m_rtpRtcp->SendOutgoingData(webrtc::kVideoFrameKey, VP8_90000_PT, frame.timeStamp, frame.timeStamp / 90, frame.payload, frame.length, nullptr, &h);
    } else if (frame.format == FRAME_FORMAT_H264) {
        int nalu_found_length = 0;
        uint8_t* buffer_start = frame.payload;
        int buffer_length = frame.length;
        int nalu_start_offset = 0;
        int nalu_end_offset = 0;
        RTPFragmentationHeader frag_info;

        h.codec = webrtc::kRtpVideoH264;
        while (buffer_length > 0) {
            nalu_found_length = findNALU(buffer_start, buffer_length, &nalu_start_offset, &nalu_end_offset);
            if (nalu_found_length < 0) {
                /* Error, should never happen */
                break;
            } else {
                /* SPS, PPS, I, P*/
                uint16_t last = frag_info.fragmentationVectorSize;
                frag_info.VerifyAndAllocateFragmentationHeader(last + 1);
                frag_info.fragmentationOffset[last] = nalu_start_offset + (buffer_start - frame.payload);
                frag_info.fragmentationLength[last] = nalu_found_length;
                buffer_start += (nalu_start_offset + nalu_found_length);
                buffer_length -= (nalu_start_offset + nalu_found_length);
            }
        }
        m_rtpRtcp->SendOutgoingData(webrtc::kVideoFrameKey, H264_90000_PT, frame.timeStamp, frame.timeStamp / 90, frame.payload, frame.length, &frag_info, &h);
    }
}

bool EncodedVideoFrameSender::init(WebRTCTransport<erizo::VIDEO>* transport, boost::shared_ptr<WebRTCTaskRunner> taskRunner)
{
    m_taskRunner = taskRunner;
    m_videoTransport.reset(transport);
    m_bitrateController.reset(webrtc::BitrateController::CreateBitrateController(Clock::GetRealTimeClock(), true));
    m_bandwidthObserver.reset(m_bitrateController->CreateRtcpBandwidthObserver());
    // FIXME: Provide the correct bitrate settings (start, min and max bitrates).
    m_bitrateController->SetBitrateObserver(this, 300 * 1000, 0, 0);

    RtpRtcp::Configuration configuration;
    // configuration.id = m_id;
    configuration.outgoing_transport = transport;
    configuration.audio = false;  // Video.
    configuration.intra_frame_callback = this;
    configuration.bandwidth_callback = m_bandwidthObserver.get();
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));

    // Disable FEC.
    // TODO: the parameters should be dynamically adjustable.
    m_rtpRtcp->SetGenericFECStatus(false, RED_90000_PT, ULP_90000_PT);
    // Enable NACK.
    // TODO: the parameters should be dynamically adjustable.
    m_rtpRtcp->SetStorePacketsStatus(true, 600);

    m_taskRunner->RegisterModule(m_rtpRtcp.get());

    setSendCodec(m_frameFormat, m_mediaSpecInfo.video.width, m_mediaSpecInfo.video.height);

    m_id = m_source->addFrameConsumer("packetizer", m_frameFormat, this, m_mediaSpecInfo);
    return true;
}

void EncodedVideoFrameSender::registerPreSendFrameCallback(FrameConsumer* frameConsumer)
{
    m_frameConsumer = frameConsumer;
}

void EncodedVideoFrameSender::deRegisterPreSendFrameCallback()
{
    m_frameConsumer = nullptr;
}

void EncodedVideoFrameSender::close()
{
    deRegisterPreSendFrameCallback();

    if (m_bitrateController)
        m_bitrateController->RemoveBitrateObserver(this);
    m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
}

}
