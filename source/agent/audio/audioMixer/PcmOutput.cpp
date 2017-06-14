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

#include <rtputils.h>

#include "AudioUtilities.h"
#include "PcmOutput.h"

namespace mcu {

using namespace webrtc;
using namespace woogeen_base;

DEFINE_LOGGER(PcmOutput, "mcu.media.PcmOutput");

PcmOutput::PcmOutput(const FrameFormat format, FrameDestination *destination)
    : m_format(format)
    , m_destination(destination)
    , m_timestampOffset(0)
    , m_valid(false)
{
}

PcmOutput::~PcmOutput()
{
    if (!m_valid)
        return;

    this->removeAudioDestination(m_destination);
    m_destination = nullptr;
    m_format = FRAME_FORMAT_UNKNOWN;
}

bool PcmOutput::init()
{
    if (m_format != FRAME_FORMAT_PCM_RAW || getSampleRate(m_format) != 48000) {
        ELOG_ERROR_T("Error invalid format, %s(%d)", getFormatStr(m_format), getSampleRate(m_format));
        return false;
    }

    this->addAudioDestination(m_destination);

    m_timestampOffset = currentTimeMs();
    m_valid = true;

    return true;
}

bool PcmOutput::addAudioFrame(const AudioFrame* audioFrame)
{
    if (!m_valid)
        return false;

    if (!audioFrame) {
        return false;
    }

    if (audioFrame->sample_rate_hz_ != getSampleRate(m_format) ||
            audioFrame->num_channels_ != getChannels(m_format)) {
        ELOG_ERROR_T("Invalid audio frame, %s(%d-%ld), want(%d-%d)",
                getFormatStr(m_format),
                audioFrame->sample_rate_hz_,
                audioFrame->num_channels_,
                getSampleRate(m_format),
                getChannels(m_format)
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
    frame.format = woogeen_base::FRAME_FORMAT_PCM_RAW;
    frame.payload = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(audioFrame->data_));
    frame.length = audioFrame->samples_per_channel_ * audioFrame->num_channels_ * sizeof(int16_t);
    frame.additionalInfo.audio.nbSamples = audioFrame->samples_per_channel_;
    frame.additionalInfo.audio.sampleRate = audioFrame->sample_rate_hz_;
    frame.additionalInfo.audio.channels = audioFrame->num_channels_;
    frame.timeStamp = (currentTimeMs() - m_timestampOffset) * frame.additionalInfo.audio.sampleRate / 1000;

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
