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

#include <webrtc/system_wrappers/interface/trace.h>

using namespace erizo;
using namespace webrtc;

namespace mcu {

DEFINE_LOGGER(AudioMixer, "mcu.media.AudioMixer");

AudioMixer::AudioMixer(const std::string& config)
    : m_vadEnabled(false)
    , m_asyncHandle(nullptr)
    , m_jitterCount(200)
    , m_jitterHold(200)
    , m_mostActiveChannel(-1)
{
    m_voiceEngine = VoiceEngine::Create();
    m_adm.reset(new webrtc::FakeAudioDeviceModule);

    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    voe->Init(m_adm.get());

    // FIXME: hard coded timer interval.
    m_jobTimer.reset(new JobTimer(100, this));

    if (ELOG_IS_TRACE_ENABLED()) {
        webrtc::Trace::CreateTrace();
        webrtc::Trace::SetTraceFile("webrtc_trace_audioMixer.txt");
        webrtc::Trace::set_level_filter(webrtc::kTraceAll);
    }
}

AudioMixer::~AudioMixer()
{
    m_jobTimer->stop();

    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);

    boost::unique_lock<boost::shared_mutex> lock(m_channelsMutex);
    m_channels.clear();
    lock.unlock();

    voe->Terminate();
    VoiceEngine::Delete(m_voiceEngine);

    if (ELOG_IS_TRACE_ENABLED()) {
        webrtc::Trace::ReturnTrace();
    }
}

void AudioMixer::onTimeout()
{
    boost::shared_lock<boost::shared_mutex> lock(m_channelsMutex);
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        it->second->performMix(it == m_channels.begin());
    }
    lock.unlock();

    if (m_vadEnabled)
        performVAD();
}

void AudioMixer::enableVAD(uint32_t period)
{
    ELOG_DEBUG("enableVAD, period:%u", period);
    m_jitterHold = period / 10;
    m_vadEnabled = true;
}

void AudioMixer::disableVAD()
{
    ELOG_DEBUG("disableVAD.");
    m_vadEnabled = false;
}

void AudioMixer::resetVAD()
{
    ELOG_DEBUG("resetVAD");
    m_mostActiveChannel = -1;
}

bool  AudioMixer::addInput(const std::string& participant, const std::string& codec, woogeen_base::FrameSource* source)
{
    assert(source);

    boost::upgrade_lock<boost::shared_mutex> lock(m_channelsMutex);
    if (m_channels.find(participant) == m_channels.end()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniquePartLock(lock);
        if (!addChannel(participant)) {
            return false;
        }
    }

    m_channels[participant]->setInput(source);
    m_mostActiveChannel = -1;
    return true;
}

void AudioMixer::removeInput(const std::string& participant)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_channelsMutex);
    if (m_channels.find(participant) != m_channels.end()) {
        m_channels[participant]->unsetInput();
        if (m_channels[participant]->isIdle()) {
            boost::upgrade_to_unique_lock<boost::shared_mutex> uniquePartLock(lock);
            removeChannel(participant);
        }
        m_mostActiveChannel = -1;
    }
}

static woogeen_base::FrameFormat getFormat(const std::string& codec) {
    if (codec == "pcmu") {
        return woogeen_base::FRAME_FORMAT_PCMU;
    } else if (codec == "pcma") {
        return woogeen_base::FRAME_FORMAT_PCMA;
    } else if (codec == "isac_16000") {
        return woogeen_base::FRAME_FORMAT_ISAC16;
    } else if (codec == "isac_32000") {
        return woogeen_base::FRAME_FORMAT_ISAC32;
    } else if (codec == "opus_48000_2") {
        return woogeen_base::FRAME_FORMAT_OPUS;
    } else if (codec == "pcm_raw") {
        return woogeen_base::FRAME_FORMAT_PCM_RAW;
    } else {
        return woogeen_base::FRAME_FORMAT_UNKNOWN;
    }
}

bool AudioMixer::addOutput(const std::string& participant, const std::string& codec, woogeen_base::FrameDestination* dest)
{
    assert(dest);

    boost::upgrade_lock<boost::shared_mutex> lock(m_channelsMutex);
    if (m_channels.find(participant) == m_channels.end()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniquePartLock(lock);
        if (!addChannel(participant)) {
            return false;
        }
    }

    return m_channels[participant]->setOutput(getFormat(codec), dest);
}

void AudioMixer::removeOutput(const std::string& participant)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_channelsMutex);
    if (m_channels.find(participant) != m_channels.end()) {
        m_channels[participant]->unsetOutput();
        if (m_channels[participant]->isIdle()) {
            boost::upgrade_to_unique_lock<boost::shared_mutex> uniquePartLock(lock);
            removeChannel(participant);
        }
    }
}

bool AudioMixer::addChannel(const std::string& participant)
{
    boost::shared_ptr<AudioChannel> channel(new AudioChannel(m_voiceEngine));
    if (channel && channel->init()) {
        m_channels[participant] = channel;
        return true;
    }
    return false;
}

void AudioMixer::removeChannel(const std::string& participant)
{
    m_channels.erase(participant);
}

void AudioMixer::performVAD()
{
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    std::vector<int32_t> activeChannels;

    if (m_jitterCount > 0) {
        m_jitterCount--;
        return;
    }

    if (!voe->GetActiveMixedInChannels(activeChannels)) {
        if (activeChannels.size() > 0 && (activeChannels[0] != m_mostActiveChannel)) {
            m_mostActiveChannel = activeChannels[0];
            ELOG_DEBUG("m_mostActiveChannel change to:%d", m_mostActiveChannel);
            notifyVAD();
            m_jitterCount = m_jitterHold; /* 200 * 10ms = 2 seconds */
        }
    }
}

void AudioMixer::notifyVAD()
{
    // FIXME: only notify the most active participant
    std::string mostActiveParticipant;

    boost::shared_lock<boost::shared_mutex> lock(m_channelsMutex);
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        if (it->second->id() == m_mostActiveChannel) {
            mostActiveParticipant = it->first;
            ELOG_DEBUG("mostActiveParticipant now is :%s", mostActiveParticipant.c_str());
        }
    }
    lock.unlock();

    if (m_asyncHandle)
        m_asyncHandle->notifyAsyncEvent("vad", mostActiveParticipant);
}

} /* namespace mcu */
