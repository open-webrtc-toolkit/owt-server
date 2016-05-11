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

VCMFrameEncoder::VCMFrameEncoder(FrameFormat format, boost::shared_ptr<WebRTCTaskRunner> taskRunner, bool useSimulcast)
    : m_streamId(0)
    , m_vcm(VideoCodingModule::Create())
    , m_encodeFormat(format)
    , m_taskRunner(taskRunner)
{
    webrtc::VP8EncoderFactoryConfig::set_use_simulcast_adapter(useSimulcast);
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
    m_streamId = 0;
}

bool VCMFrameEncoder::canSimulcastFor(FrameFormat format, uint32_t width, uint32_t height)
{
    VideoCodec videoCodec;
    videoCodec = m_vcm->GetSendCodec();
    return webrtc::VP8EncoderFactoryConfig::use_simulcast_adapter()
           && m_encodeFormat == format
           && videoCodec.width * height == videoCodec.height * width;
}

bool VCMFrameEncoder::isIdle()
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    return m_streams.size() == 0;
}

int32_t VCMFrameEncoder::generateStream(uint32_t width, uint32_t height, woogeen_base::FrameDestination* dest)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);

    VideoCodec videoCodec;
    uint8_t simulcastId {0};
    if (m_streams.size() > 0) {
        videoCodec = m_vcm->GetSendCodec();
        for (int i = 0; i < videoCodec.numberOfSimulcastStreams; ++i) {
            if (videoCodec.simulcastStream[i].width == width) {
                simulcastId = i;
                boost::shared_ptr<EncodeOut> encodeOut;
                encodeOut.reset(new EncodeOut(m_streamId, this, dest));
                OutStream stream = {.width = width, .height = height, .simulcastId = simulcastId, .encodeOut = encodeOut};
                boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
                m_streams[m_streamId] = stream;
                ELOG_DEBUG("generateStream[%p]: {.width=%d, .height=%d}, simulcastId=%d", this, width, height, simulcastId);
                return m_streamId++;
            }
        }
        if (videoCodec.numberOfSimulcastStreams >= kMaxSimulcastStreams)
            return -1;
    } else {
        switch (m_encodeFormat) {
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
    uint32_t targetKbps = calcBitrate(width, height) * (m_encodeFormat == FRAME_FORMAT_VP8 ? 0.9 : 1);

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
        .width = static_cast<short unsigned>(width),
        .height = static_cast<short unsigned>(height),
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
        return -1;
    } else {
        if (simulcastId != videoCodec.numberOfSimulcastStreams-1) {
            for (auto it = m_streams.begin(); it != m_streams.end(); ++it) {
                if (it->second.simulcastId >= simulcastId) {
                    it->second.simulcastId++;
                }
            }
        }
    }

    boost::shared_ptr<EncodeOut> encodeOut;
    encodeOut.reset(new EncodeOut(m_streamId, this, dest));
    OutStream stream = {.width = width, .height = height, .simulcastId = simulcastId, .encodeOut = encodeOut};
    m_streams[m_streamId] = stream;
    ELOG_DEBUG("generateStream[%p]: {.width=%d, .height=%d}, simulcastId=%d", this, width, height, simulcastId);
    return m_streamId++;
}

void VCMFrameEncoder::degenerateStream(int32_t streamId)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    auto it = m_streams.find(streamId);
    if (it != m_streams.end()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_streams.erase(streamId);
    }
}

void VCMFrameEncoder::setBitrate(unsigned short kbps, int32_t streamId)
{
    int bps = kbps * 1000;
    // TODO: Notify VCM about the packet lost and rtt information.
    m_vcm->SetChannelParameters(bps, 0, 0);
}

void VCMFrameEncoder::requestKeyFrame(int32_t streamId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    auto it = m_streams.find(streamId);
    if (it != m_streams.end()) {
        m_vcm->IntraFrameRequest(it->second.simulcastId);
    }
}

#ifdef ENABLE_YAMI
uint8_t* VCMFrameEncoder::mapYamiSurfaceToVAImage(intptr_t surface, VAImage& image)
{
    if (!m_vaDisplay)
        m_vaDisplay = YamiGetVADisplay();

    if (!m_vaDisplay) {
        ELOG_ERROR("get va display failed");
        return nullptr;
    }

    VAStatus status = vaDeriveImage(*m_vaDisplay, (VASurfaceID)surface, &image);
    if (status != VA_STATUS_SUCCESS) {
        ELOG_ERROR("vaDeriveImage: %s", vaErrorStr(status));
        return nullptr;
    }

    uint8_t* p = nullptr;
    status = vaMapBuffer(*m_vaDisplay, image.buf, (void**)&p);
    if (status != VA_STATUS_SUCCESS) {
        ELOG_ERROR("vaMapBuffer: %s", vaErrorStr(status));
        status = vaDestroyImage(*m_vaDisplay, image.image_id);
        if (status != VA_STATUS_SUCCESS) {
            ELOG_ERROR("vaDestroyImage: %s", vaErrorStr(status));
        }
        return nullptr;
    }

    return p;
}

void VCMFrameEncoder::unmapVAImage(const VAImage& image)
{
    if (!m_vaDisplay) {
        ELOG_ERROR("No va display");
        return;
    }

    VAStatus status = vaUnmapBuffer(*m_vaDisplay, image.buf);
    if (status != VA_STATUS_SUCCESS) {
        ELOG_ERROR("vaUnmapBuffer: %s", vaErrorStr(status));
    }

    status = vaDestroyImage(*m_vaDisplay, image.image_id);
    if (status != VA_STATUS_SUCCESS) {
        ELOG_ERROR("vaDestroyImage: %s", vaErrorStr(status));
    }
}

bool VCMFrameEncoder::convertYamiVideoFrameToI420VideoFrame(YamiVideoFrame& yamiFrame, I420VideoFrame& i420Frame)
{
    auto input = yamiFrame.frame;
    if (!input) {
        ELOG_ERROR("input null");
        return false;
    }

    VAImage image;
    uint8_t* buffer = mapYamiSurfaceToVAImage(input->surface, image);
    if (!buffer) {
        ELOG_ERROR("mapYamiSurfaceToVAImage failed");
        return false;
    }

    bool ret = false;
    ELOG_DEBUG("timestamp: %lld", input->timeStamp);
    i420Frame.set_timestamp(input->timeStamp);
    switch (image.format.fourcc) {
    case VA_FOURCC_YV12:
        assert(image.num_planes == 3);
        i420Frame.CreateFrame(image.offsets[1] - image.offsets[0],
                              buffer + image.offsets[0],
                              image.data_size - image.offsets[2],
                              buffer + image.offsets[2],
                              image.offsets[2] - image.offsets[1],
                              buffer + image.offsets[1],
                              image.width,
                              image.height,
                              image.pitches[0],
                              image.pitches[2],
                              image.pitches[1],
                              kVideoRotation_0);
        ret = true;
        break;
    case VA_FOURCC_NV12:
        // TODO
        assert(false);
        break;
    default:
        assert(false);
        break;
    }

    unmapVAImage(image);
    return ret;
}
#endif

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
#ifdef ENABLE_YAMI
    case FRAME_FORMAT_YAMI: {
        if (m_encodeFormat == FRAME_FORMAT_UNKNOWN)
            return;

        I420VideoFrame rawFrame;
        YamiVideoFrame yamiFrame = *(reinterpret_cast<YamiVideoFrame*>(frame.payload));
        if (!convertYamiVideoFrameToI420VideoFrame(yamiFrame, rawFrame))
            return;

        m_vcm->AddVideoFrame(rawFrame);
        break;
    }
#endif
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
    if (!m_streams.empty()) {
        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = m_encodeFormat;
        frame.payload = encoded_image._buffer;
        frame.length = encoded_image._length;
        frame.timeStamp = encoded_image._timeStamp;
        frame.additionalInfo.video.width = encoded_image._encodedWidth;
        frame.additionalInfo.video.height = encoded_image._encodedHeight;

        auto it = m_streams.begin();
        for (; it != m_streams.end(); ++it) {
            if (it->second.encodeOut.get() && it->second.simulcastId == rtpVideoHdr->simulcastIdx)
                it->second.encodeOut->onEncoded(frame);
        }
        return 0;
    }

    return -1;
}

} // namespace woogeen_base
