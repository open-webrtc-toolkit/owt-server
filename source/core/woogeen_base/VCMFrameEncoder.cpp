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

using namespace webrtc;

namespace woogeen_base {

VCMFrameEncoder::VCMFrameEncoder(boost::shared_ptr<TaskRunner> taskRunner)
    : m_vcm(VideoCodingModule::Create())
    , m_encoderInitialized(false)
    , m_encodeFormat(FRAME_FORMAT_UNKNOWN)
    , m_taskRunner(taskRunner)
    , m_encodedFrameConsumer(nullptr)
    , m_consumerId(-1)
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

bool VCMFrameEncoder::activateOutput(int id, FrameFormat format, unsigned int framerate, unsigned short bitrate, VideoFrameConsumer* consumer)
{
    if (m_encodedFrameConsumer)
        return false;

    m_encodedFrameConsumer = consumer;
    m_consumerId = id;
    m_encodeFormat = format;
    // m_vcm->SetChannelParameters(bitrate * 1000, 0, 0);
    return true;
}

void VCMFrameEncoder::deActivateOutput(int id)
{
    if (m_consumerId == id) {
        assert(m_encodedFrameConsumer);
        m_encodedFrameConsumer = nullptr;
        m_consumerId = -1;
    }
}

void VCMFrameEncoder::setBitrate(unsigned short bitrate, int id)
{
    int bps = bitrate * 1000;
    // TODO: Notify VCM about the packet lost and rtt information.
    m_vcm->SetChannelParameters(bps, 0, 0);
}

void VCMFrameEncoder::requestKeyFrame(int id)
{
    m_vcm->IntraFrameRequest(0);
}

void VCMFrameEncoder::onFrame(FrameFormat format, unsigned char* payload, int len, unsigned int ts)
{
    switch (format) {
    case FRAME_FORMAT_I420: {
        // Currently we should only receive I420 format frame.
        I420VideoFrame* rawFrame = reinterpret_cast<I420VideoFrame*>(payload);
        initializeEncoder(rawFrame->width(), rawFrame->height());
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
    if (m_encodedFrameConsumer) {
        m_encodedFrameConsumer->onFrame(m_encodeFormat, encoded_image._buffer, encoded_image._length, encoded_image._timeStamp);
        return 0;
    }

    return -1;
}

bool VCMFrameEncoder::initializeEncoder(uint32_t width, uint32_t height)
{
    if (m_encoderInitialized || !m_encodedFrameConsumer)
        return m_encoderInitialized;

    VideoCodec videoCodec;
    bool supportedCodec = false;
    switch (m_encodeFormat) {
    case FRAME_FORMAT_VP8:
        supportedCodec = (VideoCodingModule::Codec(webrtc::kVideoCodecVP8, &videoCodec) == VCM_OK);
        break;
    case FRAME_FORMAT_H264:
        supportedCodec = (VideoCodingModule::Codec(webrtc::kVideoCodecH264, &videoCodec) == VCM_OK);
        break;
    case FRAME_FORMAT_I420:
    default:
        break;
    }

    if (!supportedCodec) {
        m_encodedFrameConsumer = nullptr;
        m_consumerId = -1;
        return false;
    }

    videoCodec.width = width;
    videoCodec.height = height;

    m_vcm->RegisterSendCodec(&videoCodec, 1, 1400);
    m_encoderInitialized = true;
    return true;
}

}
