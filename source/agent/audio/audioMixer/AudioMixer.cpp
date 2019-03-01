// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <webrtc/base/logging.h>
#include <webrtc/system_wrappers/include/trace.h>

#include "AudioMixer.h"
#include "AcmmFrameMixer.h"

#include "AudioUtilities.h"
#include "AudioTime.h"

using namespace webrtc;

namespace mcu {

DEFINE_LOGGER(AudioMixer, "mcu.media.AudioMixer");

AudioMixer::AudioMixer(const std::string& configStr)
{
    if (ELOG_IS_TRACE_ENABLED()) {
        rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
        rtc::LogMessage::LogTimestamps(true);

        webrtc::Trace::CreateTrace();
        webrtc::Trace::SetTraceFile(NULL, false);
        webrtc::Trace::set_level_filter(webrtc::kTraceAll);
    } else if (ELOG_IS_DEBUG_ENABLED()) {
        rtc::LogMessage::LogToDebug(rtc::LS_INFO);
        rtc::LogMessage::LogTimestamps(true);

        const int kTraceFilter = webrtc::kTraceNone | webrtc::kTraceTerseInfo |
            webrtc::kTraceWarning | webrtc::kTraceError |
            webrtc::kTraceCritical | webrtc::kTraceDebug |
            webrtc::kTraceInfo;

        webrtc::Trace::CreateTrace();
        webrtc::Trace::SetTraceFile(NULL, false);
        webrtc::Trace::set_level_filter(kTraceFilter);
    }

    AudioTime::setTimestampOffset(currentTimeMs());

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

bool AudioMixer::addInput(const std::string& endpoint, const std::string& inStreamId, const std::string& codec, woogeen_base::FrameSource* source)
{
    assert(source);

    woogeen_base::FrameFormat format = getFormat(codec);
    if (format == woogeen_base::FRAME_FORMAT_UNKNOWN) {
        ELOG_ERROR("addInput, invalid codec(%s)", codec.c_str());
        return false;
    }

    return m_mixer->addInput(endpoint, inStreamId, format, source);
}

void AudioMixer::removeInput(const std::string& endpoint, const std::string& inStreamId)
{
    m_mixer->removeInput(endpoint, inStreamId);
    return;
}

void AudioMixer::setInputActive(const std::string& endpoint, const std::string& inStreamId, bool active)
{
    m_mixer->setInputActive(endpoint, inStreamId, active);
    return;
}

bool AudioMixer::addOutput(const std::string& endpoint, const std::string& outStreamId, const std::string& codec, woogeen_base::FrameDestination* dest)
{
    assert(dest);

    woogeen_base::FrameFormat format = getFormat(codec);
    if (format == woogeen_base::FRAME_FORMAT_UNKNOWN) {
        ELOG_ERROR("addOutput, invalid codec(%s)", codec.c_str());
        return false;
    }

    return m_mixer->addOutput(endpoint, outStreamId, format, dest);
}

void AudioMixer::removeOutput(const std::string& endpoint, const std::string& outStreamId)
{
    m_mixer->removeOutput(endpoint, outStreamId);
    return;
}

} /* namespace mcu */
