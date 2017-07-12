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

#include <webrtc/common_types.h>

#include "AudioUtilities.h"
#include "AcmmParticipant.h"
#include "AcmInput.h"
#include "FfInput.h"
#include "AcmOutput.h"
#include "PcmOutput.h"
#include "FfOutput.h"

namespace mcu {

using namespace webrtc;
using namespace woogeen_base;

DEFINE_LOGGER(AcmmParticipant, "mcu.media.AcmmParticipant");

AcmmParticipant::AcmmParticipant(int32_t id)
    : m_id(id)
    , m_srcFormat(FRAME_FORMAT_UNKNOWN)
    , m_source(NULL)
    , m_dstFormat(FRAME_FORMAT_UNKNOWN)
    , m_destination(NULL)
{
    ELOG_DEBUG_T("AcmmParticipant");
}

AcmmParticipant::~AcmmParticipant()
{
    ELOG_DEBUG_T("~AcmmParticipant");
}

bool AcmmParticipant::setInput(FrameFormat format, FrameSource* source)
{
    ELOG_DEBUG_T("setInput, format(%s)", getFormatStr(format));

    switch(format) {
        case FRAME_FORMAT_AAC:
        case FRAME_FORMAT_AAC_48000_2:
        case FRAME_FORMAT_AC3:
        case FRAME_FORMAT_NELLYMOSER:
            m_input.reset(new FfInput(format));
            break;
        case FRAME_FORMAT_PCM_48000_2:
        case FRAME_FORMAT_PCMU:
        case FRAME_FORMAT_PCMA:
        case FRAME_FORMAT_OPUS:
        case FRAME_FORMAT_ISAC16:
        case FRAME_FORMAT_ISAC32:
            m_input.reset(new AcmInput(format));
            break;
        default:
            ELOG_ERROR_T("Unsupported format(%s), %d", getFormatStr(format), format);
            return false;
    }

    if (!m_input->init()) {
        m_input.reset();
        return false;
    }

    source->addAudioDestination(m_input.get());
    m_srcFormat = format;
    m_source = source;
    return true;
}

void AcmmParticipant::unsetInput()
{
    ELOG_DEBUG_T("unsetInput");

    m_source->removeAudioDestination(m_input.get());
    m_source = NULL;
    m_srcFormat = FRAME_FORMAT_UNKNOWN;
    m_input.reset();
}

bool AcmmParticipant::setOutput(FrameFormat format, FrameDestination* destination)
{
    ELOG_DEBUG_T("setOutput, format(%s)", getFormatStr(format));

    switch(format) {
        case FRAME_FORMAT_PCM_48000_2:
            m_output.reset(new PcmOutput(format));
            break;
        case FRAME_FORMAT_AAC:
        case FRAME_FORMAT_AAC_48000_2:
            m_output.reset(new FfOutput(FRAME_FORMAT_AAC_48000_2));
            break;
        case FRAME_FORMAT_PCMU:
        case FRAME_FORMAT_PCMA:
        case FRAME_FORMAT_OPUS:
        case FRAME_FORMAT_ISAC16:
        case FRAME_FORMAT_ISAC32:
            m_output.reset(new AcmOutput(format));
            break;
        default:
            ELOG_ERROR_T("Unsupported format(%s), %d", getFormatStr(format), format);
            return false;
    }

    if (!m_output->init()) {
        m_output.reset();
        return false;
    }

    m_output->addAudioDestination(destination);
    m_dstFormat = format;
    m_destination = destination;
    return true;
}

void AcmmParticipant::unsetOutput()
{
    ELOG_DEBUG_T("unsetOutput");

    m_output->removeAudioDestination(m_destination);
    m_destination = NULL;
    m_dstFormat = FRAME_FORMAT_UNKNOWN;
    m_output.reset();
}

int32_t AcmmParticipant::GetAudioFrame(int32_t id, AudioFrame* audio_frame)
{
    if (!m_input || !m_input->getAudioFrame(audio_frame)) {
        ELOG_DEBUG_T("Error GetAudioFrame");
        return -1;
    }

    audio_frame->id_ = m_id;

    ELOG_TRACE_T("GetAudioFrame, id(%d), sample_rate(%d), channels(%ld), samples_per_channel(%ld)",
            m_id, audio_frame->sample_rate_hz_, audio_frame->num_channels_, audio_frame->samples_per_channel_);

    return 0;
}

int32_t AcmmParticipant::NeededFrequency(int32_t id) const
{
    return getAudioSampleRate(m_dstFormat);
}

void AcmmParticipant::NewMixedAudio(const AudioFrame* audioFrame)
{
    ELOG_TRACE_T("NewMixedAudio, id(%d), m_id(%d), sample_rate(%d), channels(%ld), samples_per_channel(%ld), timestamp(%d)",
            audioFrame->id_, m_id,
            audioFrame->sample_rate_hz_,
            audioFrame->num_channels_,
            audioFrame->samples_per_channel_,
            audioFrame->timestamp_
            );

    if (m_output) {
        m_output->addAudioFrame(audioFrame);
    }
}

} /* namespace mcu */
