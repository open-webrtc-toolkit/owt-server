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

using namespace webrtc;

namespace woogeen_base {

VCMFrameEncoder::VCMFrameEncoder(boost::shared_ptr<WebRTCTaskRunner> taskRunner)
    : m_vcm(VideoCodingModule::Create())
    , m_encoderInitialized(false)
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

int32_t VCMFrameEncoder::addFrameConsumer(const std::string& name, FrameFormat format, FrameConsumer* consumer)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    // For now we only support the consumers requiring the same frame format.
    if (m_encodeFormat != FRAME_FORMAT_UNKNOWN && m_encodeFormat != format)
        return -1;

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_encodeFormat = format;
    m_consumers.push_back(consumer);

    return m_consumers.size() - 1;
}

void VCMFrameEncoder::removeFrameConsumer(int id)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    if (id < 0 || static_cast<size_t>(id) >= m_consumers.size())
        return;

    std::list<woogeen_base::VideoFrameConsumer*>::iterator it = m_consumers.begin();
    advance(it, id);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    *it = nullptr;
}

void VCMFrameEncoder::setBitrate(unsigned short kbps, int id)
{
    int bps = kbps * 1000;
    // TODO: Notify VCM about the packet lost and rtt information.
    m_vcm->SetChannelParameters(bps, 0, 0);
}

void VCMFrameEncoder::requestKeyFrame(int id)
{
    m_vcm->IntraFrameRequest(0);
}

void VCMFrameEncoder::onFrame(const Frame& frame)
{
    switch (frame.format) {
    case FRAME_FORMAT_I420: {
        // Currently we should only receive I420 format frame.
        I420VideoFrame* rawFrame = reinterpret_cast<I420VideoFrame*>(frame.payload);
        initializeEncoder(rawFrame->width(), rawFrame->height());
        if (!m_encoderInitialized)
            break;

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

        std::list<VideoFrameConsumer*>::iterator it = m_consumers.begin();
        for (; it != m_consumers.end(); ++it) {
            if (*it)
                (*it)->onFrame(frame);
        }
        return 0;
    }

    return -1;
}

bool VCMFrameEncoder::initializeEncoder(uint32_t width, uint32_t height)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    if (m_encoderInitialized || m_encodeFormat == FRAME_FORMAT_UNKNOWN)
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
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_encodeFormat = FRAME_FORMAT_UNKNOWN;
        m_consumers.clear();
        return false;
    }

    videoCodec.width = width;
    videoCodec.height = height;
    uint32_t targetKbps = calcBitrate(width, height) * (m_encodeFormat == FRAME_FORMAT_VP8 ? 0.9 : 1);
    videoCodec.startBitrate = targetKbps;
    videoCodec.maxBitrate = targetKbps;
    videoCodec.minBitrate = targetKbps / 4;

    m_vcm->RegisterSendCodec(&videoCodec, 1, 1400);
    m_encoderInitialized = true;
    return true;
}

}
