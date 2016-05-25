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

#include "YamiFrameDecoder.h"

#include "VideoDisplay.h"
#include "YamiVideoFrame.h"
#include <webrtc/modules/video_coding/codecs/h264/include/h264.h>
#include <webrtc/modules/video_coding/codecs/vp8/include/vp8.h>
#include <webrtc/system_wrappers/interface/clock.h>
#include <webrtc/system_wrappers/interface/tick_util.h>
#include <VideoDecoderHost.h>

using namespace webrtc;
using namespace YamiMediaCodec;

namespace woogeen_base {

DEFINE_LOGGER(YamiFrameDecoder, "woogeen.YamiFrameDecoder");
#if 0
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
    decodedImage.set_render_time_ms(TickTime::MillisecondTimestamp() + m_ntpDelta);

    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = FRAME_FORMAT_I420;
    frame.payload = reinterpret_cast<uint8_t*>(&decodedImage);
    frame.length = 0;
    frame.timeStamp = decodedImage.timestamp();
    frame.additionalInfo.video.width = decodedImage.width();
    frame.additionalInfo.video.height = decodedImage.height();

    m_consumer->onFrame(frame);
    return 0;
}
#endif

YamiFrameDecoder::YamiFrameDecoder()
    : m_needDecode(false)
{
}

YamiFrameDecoder::~YamiFrameDecoder()
{
    m_needDecode = false;
}

bool YamiFrameDecoder::supportFormat(FrameFormat format)
{
    // TODO: Query the hardware/libyami capability to encode the specified format.
    return (format == FRAME_FORMAT_H264);
}

bool YamiFrameDecoder::init(FrameFormat format)
{
    switch (format) {
    case FRAME_FORMAT_VP8:
        m_decoder.reset(createVideoDecoder(YAMI_MIME_VP8), releaseVideoDecoder);
        ELOG_DEBUG("Created VP8 deocder....");
        break;
    case FRAME_FORMAT_H264:
        m_decoder.reset(createVideoDecoder(YAMI_MIME_H264), releaseVideoDecoder);
        ELOG_DEBUG("Created H.264 deocder....");
        break;
    default:
        ELOG_ERROR("Unspported video frame format %d", format);
        return false;
    }
    if (!m_decoder) {
        ELOG_ERROR("Failed to create %d", format);
        return false;
    }
    boost::shared_ptr<VADisplay> vaDisplay = GetVADisplay();
    if (!vaDisplay) {
        ELOG_ERROR("get va display failed");
        return false;
    }

    NativeDisplay display;
    display.type = NATIVE_DISPLAY_VA;
    display.handle = (intptr_t)*vaDisplay;
    m_decoder->setNativeDisplay(&display);

    VideoConfigBuffer config;
    memset(&config, 0, sizeof(config));
    config.profile = VAProfileNone;
    Decode_Status status = m_decoder->start(&config);
    if (status != DECODE_SUCCESS) {
         ELOG_ERROR("start decode failed status = %d", status);
    } else {
        m_needDecode = true;
    }
    return true;
}

void YamiFrameDecoder::onFrame(const Frame& frame)
{
    if (!m_needDecode)
        return;

    Decode_Status status;
    VideoDecodeBuffer inputBuffer;
    memset(&inputBuffer, 0, sizeof(inputBuffer));
    inputBuffer.timeStamp = frame.timeStamp;
    inputBuffer.data = frame.payload;
    inputBuffer.size = frame.length;

    ELOG_DEBUG("decode : %d, %lu", frame.timeStamp, inputBuffer.timeStamp);

    status = m_decoder->decode(&inputBuffer);
    if (DECODE_FORMAT_CHANGE == status) {
        //resend the buffer
        status = m_decoder->decode(&inputBuffer);
    }
    if (status != DECODE_SUCCESS) {
        ELOG_ERROR("Decode frame error: %d", status);
        FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
        deliverFeedbackMsg(msg);
        return;
    }
    auto output = m_decoder->getOutput();
    while (output) {
        YamiVideoFrame holder;
        holder.frame = output;

        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = FRAME_FORMAT_YAMI;
        frame.payload = reinterpret_cast<uint8_t*>(&holder);
        frame.length = 0;
        frame.timeStamp = output->timeStamp;
        frame.additionalInfo.video.width = output->crop.width;
        frame.additionalInfo.video.height = output->crop.width;
        deliverFrame(frame);
        //get next frame.
        output = m_decoder->getOutput();
    }
}
}//namespace woogeen_base
