// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioUtilities.h"
#include "PcmEncoder.h"

#include "AudioTime.h"

namespace mcu {

using namespace webrtc;
using namespace woogeen_base;

DEFINE_LOGGER(PcmEncoder, "mcu.media.PcmEncoder");

PcmEncoder::PcmEncoder(const FrameFormat format)
    : m_format(format)
    , m_timestampOffset(0)
    , m_valid(false)
{
}

PcmEncoder::~PcmEncoder()
{
    if (!m_valid)
        return;

    m_format = FRAME_FORMAT_UNKNOWN;
}

bool PcmEncoder::init()
{
    if (m_format != FRAME_FORMAT_PCM_48000_2) {
        ELOG_ERROR_T("Error invalid format, %s(%d)", getFormatStr(m_format), m_format);
        return false;
    }

    //m_timestampOffset = currentTimeMs();
    m_valid = true;

    return true;
}

bool PcmEncoder::addAudioFrame(const AudioFrame* audioFrame)
{
    if (!m_valid)
        return false;

    if (!audioFrame) {
        return false;
    }

    if (audioFrame->sample_rate_hz_ != getAudioSampleRate(m_format) ||
            audioFrame->num_channels_ != getAudioChannels(m_format)) {
        ELOG_ERROR_T("Invalid audio frame, %s(%d-%ld), want(%d-%d)",
                getFormatStr(m_format),
                audioFrame->sample_rate_hz_,
                audioFrame->num_channels_,
                getAudioSampleRate(m_format),
                getAudioChannels(m_format)
                );
        return false;
    }

    ELOG_TRACE_T("NewMixedAudio, id(%d), sample_rate(%d), channels(%ld), samples_per_channel(%ld), timestamp(%d)",
            audioFrame->id_,
            audioFrame->sample_rate_hz_,
            audioFrame->num_channels_,
            audioFrame->samples_per_channel_,
            audioFrame->timestamp_
            );

    woogeen_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = woogeen_base::FRAME_FORMAT_PCM_48000_2;
    frame.payload = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(audioFrame->data_));
    frame.length = audioFrame->samples_per_channel_ * audioFrame->num_channels_ * sizeof(int16_t);
    frame.additionalInfo.audio.nbSamples = audioFrame->samples_per_channel_;
    frame.additionalInfo.audio.sampleRate = audioFrame->sample_rate_hz_;
    frame.additionalInfo.audio.channels = audioFrame->num_channels_;
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

    return true;
}

} /* namespace mcu */
