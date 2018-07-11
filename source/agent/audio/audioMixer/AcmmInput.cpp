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
#include "AcmmInput.h"
#include "AcmDecoder.h"
#include "FfDecoder.h"

namespace mcu {

using namespace webrtc;
using namespace woogeen_base;

DEFINE_LOGGER(AcmmInput, "mcu.media.AcmmInput");

AcmmInput::AcmmInput(int32_t id, const std::string &name)
    : m_id(id)
    , m_name(name)
    , m_srcFormat(FRAME_FORMAT_UNKNOWN)
    , m_source(NULL)
{
    ELOG_DEBUG_T("AcmmInput(0x%x)", id);
}

AcmmInput::~AcmmInput()
{
    ELOG_DEBUG_T("~AcmmInput");

    if (m_source)
        unsetSource();
}

bool AcmmInput::setSource(FrameFormat format, FrameSource* source)
{
    ELOG_DEBUG_T("setSource, format(%s)", getFormatStr(format));

    switch(format) {
        case FRAME_FORMAT_AAC:
        case FRAME_FORMAT_AAC_48000_2:
        case FRAME_FORMAT_AC3:
        case FRAME_FORMAT_NELLYMOSER:
            m_decoder.reset(new FfDecoder(format));
            break;
        case FRAME_FORMAT_PCM_48000_2:
        case FRAME_FORMAT_PCMU:
        case FRAME_FORMAT_PCMA:
        case FRAME_FORMAT_OPUS:
        case FRAME_FORMAT_ISAC16:
        case FRAME_FORMAT_ISAC32:
        case FRAME_FORMAT_ILBC:
        case FRAME_FORMAT_G722_16000_1:
        case FRAME_FORMAT_G722_16000_2:
            m_decoder.reset(new AcmDecoder(format));
            break;
        default:
            ELOG_ERROR_T("Unsupported format(%s), %d", getFormatStr(format), format);
            return false;
    }

    if (!m_decoder->init()) {
        m_decoder.reset();
        return false;
    }

    source->addAudioDestination(m_decoder.get());
    m_srcFormat = format;
    m_source = source;
    return true;
}

void AcmmInput::unsetSource()
{
    ELOG_DEBUG_T("unsetSource");

    m_source->removeAudioDestination(m_decoder.get());
    m_source = NULL;
    m_srcFormat = FRAME_FORMAT_UNKNOWN;
    m_decoder.reset();
}

int32_t AcmmInput::GetAudioFrame(int32_t id, AudioFrame* audio_frame)
{
    if (!m_decoder || !m_decoder->getAudioFrame(audio_frame)) {
        ELOG_DEBUG_T("Error GetAudioFrame");
        return -1;
    }

    audio_frame->id_ = m_id;

    ELOG_TRACE_T("GetAudioFrame, groupId(%u), streamId(%u), sample_rate(%d), channels(%ld), samples_per_channel(%ld)",
            (m_id >> 16) & 0xffff, m_id & 0xffff, audio_frame->sample_rate_hz_, audio_frame->num_channels_, audio_frame->samples_per_channel_);

    return 0;
}

int32_t AcmmInput::NeededFrequency(int32_t id) const
{
    return 0;
}

} /* namespace mcu */
