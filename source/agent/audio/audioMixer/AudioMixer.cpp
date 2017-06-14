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

#include "AudioMixer.h"
#include "AcmmFrameMixer.h"

using namespace webrtc;

namespace mcu {

DEFINE_LOGGER(AudioMixer, "mcu.media.AudioMixer");

AudioMixer::AudioMixer(const std::string& configStr)
{
    m_mixer.reset(new AcmmFrameMixer());
}

AudioMixer::~AudioMixer()
{
}

void AudioMixer::setEventRegistry(EventRegistry *handle)
{
    m_mixer->setEventRegistry(handle);
}

void AudioMixer::enableVAD(uint32_t period)
{
    m_mixer->enableVAD(period);
}

void AudioMixer::disableVAD()
{
    m_mixer->disableVAD();
}

void AudioMixer::resetVAD()
{
    m_mixer->resetVAD();
}

bool AudioMixer::addInput(const std::string& participant, const std::string& codec, woogeen_base::FrameSource* source)
{
    assert(source);

    woogeen_base::FrameFormat format = getFormat(codec);
    if (format == woogeen_base::FRAME_FORMAT_UNKNOWN) {
        ELOG_ERROR("addInput, invalid codec(%s)", codec.c_str());
        return false;
    }

    return m_mixer->addInput(participant, format, source);
}

void AudioMixer::removeInput(const std::string& participant)
{
    m_mixer->removeInput(participant);
    return;
}

bool AudioMixer::addOutput(const std::string& participant, const std::string& codec, woogeen_base::FrameDestination* dest)
{
    assert(dest);

    woogeen_base::FrameFormat format = getFormat(codec);
    if (format == woogeen_base::FRAME_FORMAT_UNKNOWN) {
        ELOG_ERROR("addOutput, invalid codec(%s)", codec.c_str());
        return false;
    }

    return m_mixer->addOutput(participant, format, dest);
}

void AudioMixer::removeOutput(const std::string& participant)
{
    m_mixer->removeOutput(participant);
    return;
}

} /* namespace mcu */
