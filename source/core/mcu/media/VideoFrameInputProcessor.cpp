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

#include "VideoFrameInputProcessor.h"

#include <rtputils.h>

using namespace webrtc;
using namespace erizo;
using namespace woogeen_base;

namespace mcu {

DEFINE_LOGGER(DecodedFrameHandler, "mcu.media.DecodedFrameHandler");

DEFINE_LOGGER(RawFrameDecoder, "mcu.media.RawFrameDecoder");

DEFINE_LOGGER(VideoFrameInputProcessor, "mcu.media.VideoFrameInputProcessor");

DecodedFrameHandler::DecodedFrameHandler(int index,
                       boost::shared_ptr<VideoFrameMixer> frameMixer,
                       VideoFrameProvider* provider,
                       InputProcessorCallback* initCallback)
        : m_index(index),
          m_frameMixer(frameMixer)
{
    assert(provider);
    assert(initCallback);
    if (m_frameMixer->activateInput(index, FRAME_FORMAT_I420, provider)) {
        initCallback->onInputProcessorInitOK(index);
    }
}

DecodedFrameHandler::~DecodedFrameHandler() {
    m_frameMixer->deActivateInput(m_index);
}

int32_t DecodedFrameHandler::Decoded(I420VideoFrame& decodedImage) {
    m_frameMixer->pushInput(m_index, reinterpret_cast<unsigned char*>(&decodedImage), 0);
    return 0;
}

RawFrameDecoder::RawFrameDecoder(int index,
                       boost::shared_ptr<VideoFrameMixer> frameMixer,
                       VideoFrameProvider* provider,
                       InputProcessorCallback* initCallback)
        : m_index(index),
          m_frameMixer(frameMixer),
          m_provider(provider),
          m_initCallback(initCallback)
{
    assert(provider);
    assert(initCallback);
}

int32_t RawFrameDecoder::InitDecode(const VideoCodec* codecSettings)
{
    FrameFormat frameFormat = FRAME_FORMAT_UNKNOWN;
    if (codecSettings->codecType == webrtc::kVideoCodecVP8)
        frameFormat = woogeen_base::FRAME_FORMAT_VP8;
    else if (codecSettings->codecType == webrtc::kVideoCodecH264)
        frameFormat = woogeen_base::FRAME_FORMAT_H264;

    if (m_frameMixer->activateInput(m_index, frameFormat, m_provider)) {
        m_initCallback->onInputProcessorInitOK(m_index);
        return 0;
    }

    return -1;
}

RawFrameDecoder::~RawFrameDecoder() {
    m_frameMixer->deActivateInput(m_index);
}

int32_t RawFrameDecoder::Decode(unsigned char* payload, int len) {
    m_frameMixer->pushInput(m_index, payload, len);
    return 0;
}

VideoFrameInputProcessor::VideoFrameInputProcessor(int index, bool externalDecoding)
    : m_index(index)
    , m_externalDecoding(externalDecoding)
{
}

VideoFrameInputProcessor::~VideoFrameInputProcessor()
{
    boost::unique_lock<boost::shared_mutex> lock(m_sinkMutex);
}

bool VideoFrameInputProcessor::init(int payloadType,
                                    boost::shared_ptr<VideoFrameMixer> frameMixer,
                                    InputProcessorCallback* initCallback)
{
    VideoCodecType codecType = VideoCodecType::kVideoCodecUnknown;

    switch (payloadType) {
    case VP8_90000_PT:
        codecType = VideoCodecType::kVideoCodecVP8;
        break;
    case H264_90000_PT:
        codecType = VideoCodecType::kVideoCodecH264;
        break;
    default:
        ELOG_ERROR("Unspported video payload type %d", payloadType);
        return false;
    }

    VideoCodec codecSettings;
    codecSettings.codecType = codecType;

    if (!m_externalDecoding) {
        if (codecType == VideoCodecType::kVideoCodecH264) {
            m_decoder.reset(H264Decoder::Create());
            ELOG_DEBUG("Created H.264 deocder for RTSP input.");
        } else if (codecType == VideoCodecType::kVideoCodecVP8) {
            m_decoder.reset(VP8Decoder::Create());
            ELOG_DEBUG("Created VP8 deocder for RTSP input.");
        }

        if (m_decoder->InitDecode(&codecSettings, 0) != 0) {
            ELOG_ERROR("Video decoder init faild.");
            return false;
        }

        m_decodedFrameHandler.reset(new DecodedFrameHandler(m_index, frameMixer, this, initCallback));
        m_decoder->RegisterDecodeCompleteCallback(m_decodedFrameHandler.get());
    } else {
        m_externalDecoder.reset(new RawFrameDecoder(m_index, frameMixer, this, initCallback));
        if (m_externalDecoder->InitDecode(&codecSettings) != 0)
            return false;
    }

    m_codecInfo.codecType = codecType;

    return true;
}

int VideoFrameInputProcessor::deliverVideoData(char* buf, int len)
{
    boost::unique_lock<boost::shared_mutex> lock(m_sinkMutex);
    int ret = 0;
    if (!m_externalDecoding) {
        EncodedImage image((uint8_t*)buf, len, 0);
        image._frameType = VideoFrameType::kKeyFrame;
        image._completeFrame = true;
        ret = m_decoder->Decode(image, false, nullptr, &m_codecInfo);
    } else
        ret = m_externalDecoder->Decode((unsigned char*)buf, len);

    if (ret != 0) {
        ELOG_ERROR("Decode frame error: %d", ret);
    }
    return ret;
}

int VideoFrameInputProcessor::deliverAudioData(char* buf, int len)
{
    assert(false);
    return 0;
}

}
