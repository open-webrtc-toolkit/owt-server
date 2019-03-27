// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VCMFrameDecoder.h"

#include <boost/shared_array.hpp>

#include <webrtc/modules/video_coding/codecs/h264/include/h264.h>
#include <webrtc/modules/video_coding/codecs/vp8/include/vp8.h>
#include <webrtc/modules/video_coding/codecs/vp9/include/vp9.h>

#include <webrtc/system_wrappers/include/cpu_info.h>

extern "C" {
#include <libavutil/log.h>
}

using namespace webrtc;

namespace owt_base {

DEFINE_LOGGER(VCMFrameDecoder, "owt.VCMFrameDecoder");

VCMFrameDecoder::VCMFrameDecoder(FrameFormat format)
    : m_needDecode(false)
    , m_needKeyFrame(true)
{
    memset(&m_codecInfo, 0, sizeof(m_codecInfo));
}

VCMFrameDecoder::~VCMFrameDecoder()
{
    m_needDecode = false;
    if (m_decoder) {
        m_decoder->RegisterDecodeCompleteCallback(nullptr);
        m_decoder->Release();
    }
}

bool VCMFrameDecoder::init(FrameFormat format)
{
    VideoCodecType codecType = VideoCodecType::kVideoCodecUnknown;

    ELOG_DEBUG_T("Create deocder(%s)", getFormatStr(format));
    switch (format) {
    case FRAME_FORMAT_VP8:
        codecType = VideoCodecType::kVideoCodecVP8;
        m_decoder.reset(VP8Decoder::Create());
        break;
    case FRAME_FORMAT_VP9:
        codecType = VideoCodecType::kVideoCodecVP9;
        m_decoder.reset(VP9Decoder::Create());
        break;
    case FRAME_FORMAT_H264:
        if (ELOG_IS_TRACE_ENABLED())
            av_log_set_level(AV_LOG_DEBUG);
        else if (ELOG_IS_DEBUG_ENABLED())
            av_log_set_level(AV_LOG_INFO);
        else
            av_log_set_level(AV_LOG_QUIET);

        codecType = VideoCodecType::kVideoCodecH264;
        m_decoder.reset(H264Decoder::Create());
        break;
    default:
        ELOG_ERROR_T("Unspported video frame format %d(%s)", format, getFormatStr(format));
        return false;
    }

    VideoCodec codecSettings;
    memset(&codecSettings, 0, sizeof(codecSettings));
    codecSettings.codecType = codecType;

    if (m_decoder->InitDecode(&codecSettings, webrtc::CpuInfo::DetectNumberOfCores()) != 0) {
        ELOG_ERROR_T("Video decoder init faild.");
        return false;
    }

    m_decoder->RegisterDecodeCompleteCallback(this);

    m_needDecode = true;
    m_needKeyFrame = true;
    m_codecInfo.codecType = codecType;
    return true;
}

int32_t VCMFrameDecoder::Decoded(VideoFrame& decodedImage)
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = FRAME_FORMAT_I420;
    frame.payload = reinterpret_cast<uint8_t*>(&decodedImage);
    frame.length = 0;
    frame.timeStamp = decodedImage.timestamp();
    frame.additionalInfo.video.width = decodedImage.width();
    frame.additionalInfo.video.height = decodedImage.height();

    ELOG_TRACE_T("deliverFrame, %dx%d",
            frame.additionalInfo.video.width,
            frame.additionalInfo.video.height);
    deliverFrame(frame);
    return 0;
}

void VCMFrameDecoder::onFrame(const Frame& frame)
{
    if (!m_needDecode)
        return;

    ELOG_TRACE_T("onFrame(%s), %s, %dx%d, length(%d)",
            getFormatStr(frame.format),
            frame.additionalInfo.video.isKeyFrame ? "key" : "delta",
            frame.additionalInfo.video.width,
            frame.additionalInfo.video.height,
            frame.length
            );

    if (frame.payload == 0 || frame.length == 0) {
        ELOG_DEBUG_T("Null frame, request key frame");
        FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
        deliverFeedbackMsg(msg);
        return;
    }

    if (m_needKeyFrame) {
        if (frame.additionalInfo.video.isKeyFrame) {
            m_needKeyFrame = false;
        } else {
            ELOG_DEBUG_T("Request key frame");
            FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
            deliverFeedbackMsg(msg);
            return;
        }
    }

    uint8_t *payload    = NULL;
    size_t length       = frame.length;
    size_t padding      = EncodedImage::GetBufferPaddingBytes(m_codecInfo.codecType);
    size_t size         = length + padding;
    boost::shared_array<uint8_t> buffer;

    if (padding > 0) {
        buffer.reset(new uint8_t[size]);
        // payload = (uint8_t *)malloc(size);
        if (buffer.get()) {
            memcpy(buffer.get(), frame.payload, length);
            memset(buffer.get() + length, 0, padding);
        } else {
            return;
        }
    } else {
        // payload = frame.payload;
    }

    payload = buffer.get() ? buffer.get() : frame.payload;
    EncodedImage image(payload, length, size);
    image._frameType = frame.additionalInfo.video.isKeyFrame ? kVideoFrameKey : kVideoFrameDelta;
    image._completeFrame = true;
    image._timeStamp = frame.timeStamp;
    int ret = m_decoder->Decode(image, false, nullptr, &m_codecInfo);
    if (ret != 0) {
        ELOG_ERROR_T("Decode frame error: %d", ret);

        m_needKeyFrame = true;
        FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
        deliverFeedbackMsg(msg);
    }

    // if (padding > 0)
    //     free(payload);
}

}
