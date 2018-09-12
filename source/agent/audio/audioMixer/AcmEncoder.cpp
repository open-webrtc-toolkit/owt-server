/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#include "AudioUtilities.h"
#include "AcmEncoder.h"

#include "AudioTime.h"

namespace mcu {

using namespace webrtc;
using namespace woogeen_base;

DEFINE_LOGGER(AcmEncoder, "mcu.media.AcmEncoder");

AcmEncoder::AcmEncoder(const FrameFormat format)
    : m_format(format)
    , m_timestampOffset(0)
    , m_valid(false)
    , m_running(false)
    , m_incomingFrameCount(0)
{
    AudioCodingModule::Config config;
    m_audioCodingModule.reset(AudioCodingModule::Create(config));

    m_frame.reset(new AudioFrame());
    m_running = true;
    m_thread = boost::thread(&AcmEncoder::encodeLoop, this);
}

AcmEncoder::~AcmEncoder()
{
    int ret;

    m_running = false;
    m_cond.notify_one();
    m_thread.join();

    if (!m_valid)
        return;

    m_format = FRAME_FORMAT_UNKNOWN;

    ret = m_audioCodingModule->RegisterTransportCallback(NULL);
    if (ret != 0) {
        ELOG_ERROR_T("Error RegisterTransportCallback");
        return;
    }
}

bool AcmEncoder::init()
{
    int ret;
    struct CodecInst codec = CodecInst();

    if (!getAudioCodecInst(m_format, codec)) {
        ELOG_ERROR_T("Error fillAudioCodec(%s)", getFormatStr(m_format));
        return false;
    }

    ret = m_audioCodingModule->RegisterSendCodec(codec);
    if (ret != 0) {
        ELOG_ERROR_T("Error RegisterSendCodec(%s)", getFormatStr(m_format));
        return false;
    }

    ret = m_audioCodingModule->RegisterTransportCallback(this);
    if (ret != 0) {
        ELOG_ERROR_T("Error RegisterTransportCallback");
        return false;
    }

    //m_timestampOffset = currentTimeMs();
    m_valid = true;

    return true;
}

bool AcmEncoder::addAudioFrame(const AudioFrame* audioFrame)
{
    if (!m_valid)
        return false;

    ELOG_TRACE_T("NewMixedAudio, id(%d), sample_rate(%d), channels(%ld), samples_per_channel(%ld), timestamp(%d)",
            audioFrame->id_,
            audioFrame->sample_rate_hz_,
            audioFrame->num_channels_,
            audioFrame->samples_per_channel_,
            audioFrame->timestamp_
            );

    {
        boost::mutex::scoped_lock lock(m_mutex);

        m_frame->CopyFrom(*audioFrame);

        if (m_incomingFrameCount > 1)
            ELOG_DEBUG_T("Too many pending frames(%d)", m_incomingFrameCount);

        if (m_incomingFrameCount == 3)
            ELOG_WARN_T("Too many pending frames(%d)", m_incomingFrameCount);

        m_incomingFrameCount++;
        m_cond.notify_one();
    }

    return true;
}

void AcmEncoder::encodeLoop()
{
    while (true) {
        boost::shared_ptr<AudioFrame> frame;

        {
            boost::mutex::scoped_lock lock(m_mutex);

            while (m_running && m_incomingFrameCount == 0) {
                m_cond.wait(lock);
            }

            if (!m_running)
                break;

            m_incomingFrameCount = 0;
            frame = m_frame;
        }

        int ret = m_audioCodingModule->Add10MsData(*frame.get());
        if (ret < 0) {
            ELOG_ERROR_T("Fail to insert raw into acm");
        }
    }

    ELOG_DEBUG_T("Thread exited!");
}

int32_t AcmEncoder::SendData(FrameType frame_type,
        uint8_t payload_type,
        uint32_t timestamp,
        const uint8_t* payload_data,
        size_t payload_len_bytes,
        const RTPFragmentationHeader* fragmentation)
{
    if (!m_valid)
        return -1;

    if (!payload_data || payload_len_bytes <= 0) {
        ELOG_ERROR_T("SendData, invalid data");
        return -1;
    }

    if (payload_type != getAudioPltype(m_format)) {
        ELOG_ERROR_T("SendData, invalid payload type(%d)", payload_type);
        return -1;
    }

    woogeen_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = m_format;
    frame.additionalInfo.audio.sampleRate = getAudioSampleRate(frame.format);
    frame.additionalInfo.audio.channels = getAudioChannels(frame.format);
    frame.payload = const_cast<uint8_t*>(payload_data);
    frame.length = payload_len_bytes;
    frame.timeStamp = (AudioTime::currentTime()) * frame.additionalInfo.audio.sampleRate / 1000;

    ELOG_TRACE_T("deliverFrame(%s), sampleRate(%d), channels(%d), timeStamp(%d), length(%d), %s",
            getFormatStr(frame.format),
            frame.additionalInfo.audio.sampleRate,
            frame.additionalInfo.audio.channels,
            frame.timeStamp * 1000 / frame.additionalInfo.audio.sampleRate,
            frame.length,
            frame.additionalInfo.audio.isRtpPacket ? "RtpPacket" : "NonRtpPacket"
            );

    deliverFrame(frame);

    return 0;
}

} /* namespace mcu */
