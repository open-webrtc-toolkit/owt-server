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

VCMFrameDecoder::VCMFrameDecoder(FrameFormat format)
    : m_needDecode(false)
    , m_ntpDelta(Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() - TickTime::MillisecondTimestamp())
{
}

VCMFrameDecoder::~VCMFrameDecoder()
{
    m_needDecode = false;
    if (m_decoder)
        m_decoder->RegisterDecodeCompleteCallback(nullptr);
}

bool VCMFrameDecoder::init(FrameFormat format)
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

    m_decoder->RegisterDecodeCompleteCallback(this);

    m_needDecode = true;
    m_codecInfo.codecType = codecType;
    return true;
}

int32_t VCMFrameDecoder::Decoded(I420VideoFrame& decodedImage)
{
    decodedImage.set_render_time_ms(TickTime::MillisecondTimestamp() + m_ntpDelta);

    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = FRAME_FORMAT_I420;
    frame.payload = reinterpret_cast<uint8_t*>(&decodedImage);
    frame.length = 0;
    frame.timeStamp = decodedImage.timestamp();
    frame.additionalInfo.video.width = decodedImage.width();
    frame.additionalInfo.video.height = decodedImage.height();

    deliverFrame(frame);
    return 0;
}

void VCMFrameDecoder::onFrame(const Frame& frame)
{
    if (!m_needDecode)
        return;

    EncodedImage image(reinterpret_cast<uint8_t*>(frame.payload), frame.length, 0);
    image._frameType = VideoFrameType::kKeyFrame;
    image._completeFrame = true;
    image._timeStamp = frame.timeStamp;
    int ret = m_decoder->Decode(image, false, nullptr, &m_codecInfo);

    if (ret != 0) {
        ELOG_ERROR("Decode frame error: %d", ret);
        FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
        deliverFeedbackMsg(msg);
    }
}

}
