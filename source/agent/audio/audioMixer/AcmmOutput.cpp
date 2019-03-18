// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioUtilities.h"

#include "AcmmOutput.h"

#include "AcmEncoder.h"
#include "PcmEncoder.h"
#include "FfEncoder.h"

namespace mcu {

using namespace webrtc;
using namespace owt_base;

DEFINE_LOGGER(AcmmOutput, "mcu.media.AcmmOutput");

AcmmOutput::AcmmOutput(int32_t id)
    : m_id(id)
    , m_dstFormat(FRAME_FORMAT_UNKNOWN)
{
    ELOG_DEBUG_T("AcmmOutput(0x%x)", id);
}

AcmmOutput::~AcmmOutput()
{
    ELOG_DEBUG_T("~AcmmOutput, dst count(%ld)", m_destinations.size());

    for (auto dst : m_destinations)
        m_encoder->removeAudioDestination(dst);

    m_dstFormat = FRAME_FORMAT_UNKNOWN;
    m_encoder.reset();
}

bool AcmmOutput::addDest(FrameFormat format, FrameDestination* destination)
{
    ELOG_DEBUG_T("addDest, format(%s), dest(%p)", getFormatStr(format), destination);

    if (m_dstFormat != FRAME_FORMAT_UNKNOWN
            && m_dstFormat != format) {

        ELOG_ERROR_T("Don't support to update format(%s -> %s)", getFormatStr(m_dstFormat), getFormatStr(format));
        return false;
    }

    if (m_dstFormat == FRAME_FORMAT_UNKNOWN) {
        switch(format) {
            case FRAME_FORMAT_PCM_48000_2:
                m_encoder.reset(new PcmEncoder(format));
                break;
            case FRAME_FORMAT_AAC:
                ELOG_WARN_T("FRAME_FORMAT_AAC is deprecated for audio output, using FRAME_FORMAT_AAC_48000_2!");
                format = FRAME_FORMAT_AAC_48000_2;
                m_encoder.reset(new FfEncoder(FRAME_FORMAT_AAC_48000_2));
                break;
            case FRAME_FORMAT_AAC_48000_2:
                m_encoder.reset(new FfEncoder(FRAME_FORMAT_AAC_48000_2));
                break;
            case FRAME_FORMAT_PCMU:
            case FRAME_FORMAT_PCMA:
            case FRAME_FORMAT_OPUS:
            case FRAME_FORMAT_ISAC16:
            case FRAME_FORMAT_ISAC32:
            case FRAME_FORMAT_ILBC:
            case FRAME_FORMAT_G722_16000_1:
            case FRAME_FORMAT_G722_16000_2:
                m_encoder.reset(new AcmEncoder(format));
                break;
            default:
                ELOG_ERROR_T("Unsupported format(%s), %d", getFormatStr(format), format);
                return false;
        }

        if (!m_encoder->init()) {
            m_encoder.reset();
            return false;
        }

        m_dstFormat = format;
    }

    m_encoder->addAudioDestination(destination);
    m_destinations.push_back(destination);
    return true;
}

void AcmmOutput::removeDest(FrameDestination* destination)
{
    ELOG_DEBUG_T("removeDest, dst(%p)", destination);

    m_destinations.remove(destination);
    m_encoder->removeAudioDestination(destination);
}

int32_t AcmmOutput::NeededFrequency()
{
    if (!m_destinations.size())
        return 0;

    return getAudioSampleRate(m_dstFormat);
}

bool AcmmOutput::newAudioFrame(const webrtc::AudioFrame *audioFrame)
{
    if (!m_destinations.size())
        return true;

    ELOG_TRACE_T("newAudioFrame, frame id(0x%x), groupId(%u), streamId(%u), sample_rate(%d), channels(%ld), samples_per_channel(%ld), timestamp(%d)",
            audioFrame->id_, (m_id >> 16) & 0xffff, m_id & 0xffff,
            audioFrame->sample_rate_hz_,
            audioFrame->num_channels_,
            audioFrame->samples_per_channel_,
            audioFrame->timestamp_
            );

    if (m_encoder) {
        m_encoder->addAudioFrame(audioFrame);
    }

    return true;
}

} /* namespace mcu */
