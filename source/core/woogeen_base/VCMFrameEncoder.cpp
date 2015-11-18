/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#include "VCMFrameEncoder.h"

#include "MediaUtilities.h"
#include <webrtc/modules/video_coding/codecs/vp8/vp8_factory.h>

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(VCMFrameEncoder, "woogeen.VCMFrameEncoder");

VCMFrameEncoder::VCMFrameEncoder(boost::shared_ptr<WebRTCTaskRunner> taskRunner)
    : m_vcm(VideoCodingModule::Create())
    , m_encodeFormat(FRAME_FORMAT_UNKNOWN)
    , m_taskRunner(taskRunner)
{
    m_vcm->InitializeSender();
    m_vcm->RegisterTransportCallback(this);
    m_vcm->EnableFrameDropper(false);
    if (m_taskRunner)
        m_taskRunner->RegisterModule(m_vcm);
}

VCMFrameEncoder::~VCMFrameEncoder()
{
    if (m_taskRunner)
        m_taskRunner->DeRegisterModule(m_vcm);

    VideoCodingModule::Destroy(m_vcm);
    m_vcm = nullptr;
}

int32_t VCMFrameEncoder::addFrameConsumer(const std::string& name, FrameFormat format, FrameConsumer* consumer, const MediaSpecInfo& info)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    if (!webrtc::VP8EncoderFactoryConfig::use_simulcast_adapter() && m_encodeFormat != FRAME_FORMAT_UNKNOWN) {
        if (m_encodeFormat != format)
            return -1;

        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_consumers.push_back({consumer, 0});
        return m_consumers.size() - 1;
    }

    VideoCodec videoCodec;
    uint16_t width {info.video.width};
    uint16_t height {info.video.height};
    uint8_t simulcastId {0};
    if (m_encodeFormat != FRAME_FORMAT_UNKNOWN) { // already initialized
        if (m_encodeFormat != format)
            return -1;
        videoCodec = m_vcm->GetSendCodec();
        if (videoCodec.width * height != videoCodec.height * width) {
            ELOG_DEBUG("resolution (%dx%d) has different aspect ratio from this FrameEncoder (%dx%d)",
                width, height, videoCodec.width, videoCodec.height);
            return -1;
        }
        bool reuse {false};
        if (videoCodec.numberOfSimulcastStreams >= kMaxSimulcastStreams)
            reuse = true; // in current cases, actually [REUSE] would only occur here; see VideoMixer::addOutput().
        else {
            for (int i = 0; i < videoCodec.numberOfSimulcastStreams; ++i) {
                if (videoCodec.simulcastStream[i].width == width) {
                    reuse = true;
                    simulcastId = i;
                    break;
                }
            }
        }
        if (reuse) {
            boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
            m_consumers.push_back({consumer, simulcastId});
            ELOG_DEBUG("addFrameConsumer[%p]: {.width=%d, .height=%d}, simulcastId=%d", this, width, height, simulcastId);
            return m_consumers.size() - 1;
        }
    } else {
        switch (format) {
        case FRAME_FORMAT_VP8:
            if (VideoCodingModule::Codec(kVideoCodecVP8, &videoCodec) != VCM_OK)
                return -1;
            break;
        case FRAME_FORMAT_H264:
            if (VideoCodingModule::Codec(kVideoCodecH264, &videoCodec) != VCM_OK)
                return -1;
            break;
        case FRAME_FORMAT_I420:
        default:
            return -1;
        }
    }

    VideoCodec fbVideoCodec = videoCodec;
    uint32_t targetKbps = calcBitrate(width, height) * (format == FRAME_FORMAT_VP8 ? 0.9 : 1);

    for (; simulcastId < videoCodec.numberOfSimulcastStreams; ++simulcastId) {
        if (videoCodec.simulcastStream[simulcastId].width > width)
            break;
    }
    if (simulcastId < videoCodec.numberOfSimulcastStreams) {
        uint8_t j = videoCodec.numberOfSimulcastStreams;
        while (j > simulcastId) {
            memcpy(&videoCodec.simulcastStream[j], &videoCodec.simulcastStream[j-1], sizeof(SimulcastStream));
            --j;
        }
    } else {
        videoCodec.width = width;
        videoCodec.height = height;
    }

    videoCodec.simulcastStream[simulcastId] = SimulcastStream{
        .width = width,
        .height = height,
        .numberOfTemporalLayers = 0,
        .maxBitrate = targetKbps,
        .targetBitrate = targetKbps,
        .minBitrate = targetKbps / 4,
        .qpMax = videoCodec.qpMax
    };
    ++videoCodec.numberOfSimulcastStreams;

    uint32_t newTargetBitrate = videoCodec.simulcastStream[videoCodec.numberOfSimulcastStreams-1].targetBitrate;
    uint32_t newMaxBitrate = videoCodec.simulcastStream[videoCodec.numberOfSimulcastStreams-1].maxBitrate;
    uint32_t newMinBitrate = videoCodec.simulcastStream[videoCodec.numberOfSimulcastStreams-1].minBitrate;
    for (uint8_t i = 0; i < videoCodec.numberOfSimulcastStreams-1; ++i) {
        newTargetBitrate += videoCodec.simulcastStream[i].targetBitrate;
        newMaxBitrate += videoCodec.simulcastStream[i].maxBitrate;
        newMinBitrate += videoCodec.simulcastStream[i].targetBitrate;
    }
    videoCodec.startBitrate = newMaxBitrate;
    videoCodec.targetBitrate = newTargetBitrate;
    videoCodec.maxBitrate = newMaxBitrate;
    videoCodec.minBitrate = newMinBitrate;

    int32_t ret = m_vcm->RegisterSendCodec(&videoCodec, 1, 1400);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    if (ret != VCM_OK) {
        ELOG_ERROR("RegisterSendCodec error: %d", ret);
        ELOG_DEBUG("Try fallback to previous video codec");
        m_vcm->RegisterSendCodec(&fbVideoCodec, 1, 1400);
        simulcastId = 0;
    } else {
        if (simulcastId != videoCodec.numberOfSimulcastStreams-1) {
            auto it = m_consumers.begin();
            for (; it != m_consumers.end(); ++it) {
                if ((*it).streamId >= simulcastId)
                    (*it).streamId++;
            }
        }
    }
    m_encodeFormat = format;
    m_consumers.push_back({consumer, simulcastId});
    ELOG_DEBUG("addFrameConsumer[%p]: {.width=%d, .height=%d}, simulcastId=%d", this, width, height, simulcastId);
    return m_consumers.size() - 1;
}

void VCMFrameEncoder::removeFrameConsumer(int id)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    if (id < 0 || static_cast<size_t>(id) >= m_consumers.size())
        return;

    auto it = m_consumers.begin();
    advance(it, id);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    (*it).consumer = nullptr;
}

void VCMFrameEncoder::setBitrate(unsigned short kbps, int id)
{
    int bps = kbps * 1000;
    // TODO: Notify VCM about the packet lost and rtt information.
    m_vcm->SetChannelParameters(bps, 0, 0);
}

void VCMFrameEncoder::requestKeyFrame(int id)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (id < 0 || static_cast<size_t>(id) >= m_consumers.size())
        return;

    auto it = m_consumers.begin();
    advance(it, id);
    if (it->consumer)
        m_vcm->IntraFrameRequest(it->streamId);
}

void VCMFrameEncoder::onFrame(const Frame& frame)
{
    switch (frame.format) {
    case FRAME_FORMAT_I420: {
        if (m_encodeFormat == FRAME_FORMAT_UNKNOWN)
            return;
        // Currently we should only receive I420 format frame.
        I420VideoFrame* rawFrame = reinterpret_cast<I420VideoFrame*>(frame.payload);
        const int kMsToRtpTimestamp = 90;
        const uint32_t time_stamp =
            kMsToRtpTimestamp *
            static_cast<uint32_t>(rawFrame->render_time_ms());
        rawFrame->set_timestamp(time_stamp);
        m_vcm->AddVideoFrame(*rawFrame);
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

int32_t VCMFrameEncoder::SendData(
    uint8_t payloadType,
    const webrtc::EncodedImage& encoded_image,
    const RTPFragmentationHeader& fragmentationHeader,
    const RTPVideoHeader* rtpVideoHdr)
{
    // New encoded data, hand over to the frame consumer.
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (!m_consumers.empty()) {
        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = m_encodeFormat;
        frame.payload = encoded_image._buffer;
        frame.length = encoded_image._length;
        frame.timeStamp = encoded_image._timeStamp;
        frame.additionalInfo.video.width = encoded_image._encodedWidth;
        frame.additionalInfo.video.height = encoded_image._encodedHeight;

        auto it = m_consumers.begin();
        for (; it != m_consumers.end(); ++it) {
            if ((*it).consumer && (*it).streamId == rtpVideoHdr->simulcastIdx)
                ((*it).consumer)->onFrame(frame);
        }
        return 0;
    }

    return -1;
}

} // namespace woogeen_base
