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

#include "VCMFrameDecoder.h"

#include <webrtc/modules/video_coding/codecs/h264/include/h264.h>
#include <webrtc/modules/video_coding/codecs/vp8/include/vp8.h>
#include <webrtc/system_wrappers/interface/clock.h>
#include <webrtc/system_wrappers/interface/tick_util.h>

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(VCMFrameDecoder, "woogeen.VCMFrameDecoder");

DecodedFrameHandler::DecodedFrameHandler(boost::shared_ptr<VideoFrameConsumer> consumer)
    : m_ntpDelta(Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() - TickTime::MillisecondTimestamp())
    , m_consumer(consumer)
{
}

DecodedFrameHandler::~DecodedFrameHandler()
{
}

int32_t DecodedFrameHandler::Decoded(I420VideoFrame& decodedImage)
{
    decodedImage.set_render_time_ms(TickTime::MillisecondTimestamp() - m_ntpDelta);
    m_consumer->onFrame(FRAME_FORMAT_I420, reinterpret_cast<unsigned char*>(&decodedImage), 0, decodedImage.timestamp());
    return 0;
}

VCMFrameDecoder::VCMFrameDecoder(boost::shared_ptr<VideoFrameConsumer> consumer)
    : m_decodedFrameConsumer(consumer)
{
}

VCMFrameDecoder::~VCMFrameDecoder()
{
    m_decoder->RegisterDecodeCompleteCallback(nullptr);
}

bool VCMFrameDecoder::setInput(FrameFormat format, VideoFrameProvider* provider)
{
    VideoCodecType codecType = VideoCodecType::kVideoCodecUnknown;

    switch (format) {
    case FRAME_FORMAT_VP8:
        codecType = VideoCodecType::kVideoCodecVP8;
        m_decoder.reset(VP8Decoder::Create());
        ELOG_DEBUG("Created VP8 deocder.");
        break;
    case FRAME_FORMAT_H264:
        codecType = VideoCodecType::kVideoCodecH264;
        m_decoder.reset(H264Decoder::Create());
        ELOG_DEBUG("Created H.264 deocder.");
        break;
    default:
        ELOG_ERROR("Unspported video frame format %d", format);
        return false;
    }

    VideoCodec codecSettings;
    codecSettings.codecType = codecType;

    if (m_decoder->InitDecode(&codecSettings, 0) != 0) {
        ELOG_ERROR("Video decoder init faild.");
        return false;
    }

    m_decodedFrameHandler.reset(new DecodedFrameHandler(m_decodedFrameConsumer));
    m_decoder->RegisterDecodeCompleteCallback(m_decodedFrameHandler.get());

    m_codecInfo.codecType = codecType;
    return true;
}

void VCMFrameDecoder::unsetInput()
{
    m_decoder->RegisterDecodeCompleteCallback(nullptr);
    m_decodedFrameHandler.reset();
    m_decoder.reset();
}

void VCMFrameDecoder::onFrame(FrameFormat format, unsigned char* payload, int len, unsigned int ts)
{
    int ret = 0;

    EncodedImage image(payload, len, 0);
    image._frameType = VideoFrameType::kKeyFrame;
    image._completeFrame = true;
    image._timeStamp = ts;
    ret = m_decoder->Decode(image, false, nullptr, &m_codecInfo);

    if (ret != 0) {
        ELOG_ERROR("Decode frame error: %d", ret);
    }
}

}
