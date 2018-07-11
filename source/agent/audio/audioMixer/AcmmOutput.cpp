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
#include "AcmmOutput.h"

#include "AcmEncoder.h"
#include "PcmEncoder.h"
#include "FfEncoder.h"

namespace mcu {

using namespace webrtc;
using namespace woogeen_base;

DEFINE_LOGGER(AcmmOutput, "mcu.media.AcmmOutput");

AcmmOutput::AcmmOutput(int32_t id)
    : m_id(id)
    , m_dstFormat(FRAME_FORMAT_UNKNOWN)
    , m_destination(NULL)
{
    ELOG_DEBUG_T("AcmmOutput(0x%x)", id);
}

AcmmOutput::~AcmmOutput()
{
    ELOG_DEBUG_T("~AcmmOutput");

    if (m_destination)
        unsetDest();
}

bool AcmmOutput::setDest(FrameFormat format, FrameDestination* destination)
{
    ELOG_DEBUG_T("setDest, format(%s)", getFormatStr(format));

    switch(format) {
        case FRAME_FORMAT_PCM_48000_2:
            m_encoder.reset(new PcmEncoder(format));
            break;
        case FRAME_FORMAT_AAC:
            ELOG_WARN_T("FRAME_FORMAT_AAC is deprecated for audio output, using FRAME_FORMAT_AAC_48000_2!");
            format = FRAME_FORMAT_AAC_48000_2;
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

    m_encoder->addAudioDestination(destination);
    m_dstFormat = format;
    m_destination = destination;
    return true;
}

void AcmmOutput::unsetDest()
{
    ELOG_DEBUG_T("unsetDest");

    m_encoder->removeAudioDestination(m_destination);
    m_destination = NULL;
    m_dstFormat = FRAME_FORMAT_UNKNOWN;
    m_encoder.reset();
}

int32_t AcmmOutput::NeededFrequency()
{
    return getAudioSampleRate(m_dstFormat);
}

bool AcmmOutput::newAudioFrame(const webrtc::AudioFrame *audioFrame)
{
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
