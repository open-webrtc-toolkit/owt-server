// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AcmmFrameMixer.h"

namespace mcu {

static inline AudioConferenceMixer::Frequency convert2Frequency(int32_t freq)
{
    switch (freq) {
        case 8000:
            return AudioConferenceMixer::kNbInHz;

        case 16000:
            return AudioConferenceMixer::kWbInHz;

        case 32000:
            return AudioConferenceMixer::kSwbInHz;

        case 48000:
            return AudioConferenceMixer::kFbInHz;

        default:
            return AudioConferenceMixer::kLowestPossible;
    }
}

DEFINE_LOGGER(AcmmFrameMixer, "mcu.media.AcmmFrameMixer");

AcmmFrameMixer::AcmmFrameMixer()
    : m_asyncHandle(NULL)
    , m_vadEnabled(false)
    , m_frequency(0)
{
    m_mixerModule.reset(AudioConferenceMixer::Create(0));
    m_mixerModule->RegisterMixedStreamCallback(this);
    m_mixerModule->SetMultipleInputs(true);

    m_groupIds.resize(MAX_GROUPS + 1);
    for (size_t i = 1; i < MAX_GROUPS + 1; ++i)
        m_groupIds[i] = true;

    //reserved for broadcast group
    m_groupIds[0] = false;
    m_broadcastGroup.reset(new AcmmBroadcastGroup());

    m_jobTimer.reset(new JobTimer(MIXER_FREQUENCY, this));
}

AcmmFrameMixer::~AcmmFrameMixer()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    m_jobTimer->stop();

    m_mixerModule->UnRegisterMixedStreamCallback();

    if (m_vadEnabled) {
        m_mixerModule->UnRegisterMixerVadCallback();
        m_vadEnabled = false;
    }
}

bool AcmmFrameMixer::getFreeGroupId(uint16_t *id)
{
    for (size_t i = 0; i < m_groupIds.size(); ++i) {
        if (m_groupIds[i]) {
            m_groupIds[i] = false;

            *id = i;
            return true;
        }
    }

    ELOG_WARN("No free Id, max groups reached(%d)!", MAX_GROUPS);
    return false;
}

boost::shared_ptr<AcmmGroup> AcmmFrameMixer::getGroup(const std::string& group)
{
    if (m_groupIdMap.find(group) == m_groupIdMap.end())
        return NULL;

    if (m_groups.find(m_groupIdMap[group]) == m_groups.end())
        return NULL;

    return m_groups[m_groupIdMap[group]];
}

boost::shared_ptr<AcmmGroup> AcmmFrameMixer::addGroup(const std::string& group)
{
    uint16_t id = 0;
    boost::shared_ptr<AcmmGroup> acmmGroup;

    if (getFreeGroupId(&id)) {
        m_groupIdMap[group] = id;
        acmmGroup.reset(new AcmmGroup(id));
        m_groups[m_groupIdMap[group]] = acmmGroup;
    }

    return acmmGroup;
}

void AcmmFrameMixer::removeGroup(const std::string& group)
{
    m_groups.erase(m_groupIdMap[group]);
    m_groupIds[m_groupIdMap[group]] = true;
    m_groupIdMap.erase(group);
}

void AcmmFrameMixer::setEventRegistry(EventRegistry* handle)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_TRACE("setEventRegistry(%p)", handle);

    m_asyncHandle = handle;
}

void AcmmFrameMixer::enableVAD(uint32_t period)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("enableVAD, period(%u)", period);

    m_vadEnabled = true;
    m_mostActiveInput.reset();
    m_mixerModule->RegisterMixerVadCallback(this, period / 10);
}

void AcmmFrameMixer::disableVAD()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("disableVAD");

    m_vadEnabled = false;
    m_mostActiveInput.reset();
    m_mixerModule->UnRegisterMixerVadCallback();
}

void AcmmFrameMixer::resetVAD()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("resetVAD");

    m_mostActiveInput.reset();
}

bool AcmmFrameMixer::addInput(const std::string& group, const std::string& inStream, const owt_base::FrameFormat format, owt_base::FrameSource* source)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    boost::shared_ptr<AcmmGroup> acmmGroup;
    boost::shared_ptr<AcmmInput> acmmInput;
    int ret;

    ELOG_DEBUG("addInput: group(%s), inStream(%s), format(%s), source(%p)", group.c_str(), inStream.c_str(), getFormatStr(format), source);

    acmmGroup = getGroup(group);
    if (!acmmGroup) {
        acmmGroup = addGroup(group);
        if (!acmmGroup) {
            ELOG_ERROR("Can not add input group");
            return false;
        }
    }

    acmmInput = acmmGroup->getInput(inStream);
    if (acmmInput) {
        ELOG_DEBUG("Update previous input");

        acmmInput->unsetSource();
        if(!acmmInput->setSource(format, source)) {
            ELOG_ERROR("Fail to update source");
            return false;
        }
    } else {
        acmmInput = acmmGroup->addInput(inStream);
        if(!acmmInput) {
            ELOG_ERROR("Fail to add input");
            return false;
        }

        if (!acmmInput->setSource(format, source)) {
            ELOG_ERROR("Fail to set source");
            return false;
        }

        ret = m_mixerModule->SetMixabilityStatus(acmmInput.get(), true);
        if (ret != 0) {
            ELOG_ERROR("Fail to SetMixabilityStatus");
            return false;
        }

        if (!acmmGroup->numOfOutputs()) {
            ret = m_mixerModule->SetAnonymousMixabilityStatus(acmmInput.get(), true);
            if (ret != 0) {
                ELOG_ERROR("Fail to SetAnonymousMixabilityStatus");
                return false;
            }
        }

        if (!acmmGroup->anyOutputsConnected()) {
            std::vector<boost::shared_ptr<AcmmOutput>> outputs;
            acmmGroup->getOutputs(outputs);
            for(auto& o : outputs) {
                m_broadcastGroup->removeDest(m_outputInfoMap[o.get()].dest);
                if (!o->addDest(m_outputInfoMap[o.get()].format, m_outputInfoMap[o.get()].dest)) {
                    ELOG_ERROR("Fail to reconnect dest");
                    return false;
                }
            }
        }
    }

    statistics();
    return true;
}

void AcmmFrameMixer::removeInput(const std::string& group, const std::string& inStream)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    boost::shared_ptr<AcmmGroup> acmmGroup;
    boost::shared_ptr<AcmmInput> acmmInput;
    int ret;

    ELOG_DEBUG("removeInput: group(%s), inStream(%s)", group.c_str(), inStream.c_str());

    acmmGroup = getGroup(group);
    if (!acmmGroup) {
        ELOG_ERROR("Invalid gropu(%s)", group.c_str());
        return;
    }

    acmmInput = acmmGroup->getInput(inStream);
    if (!acmmInput) {
        ELOG_ERROR("Invalid input(%s)", inStream.c_str());
        return;
    }

    ret = m_mixerModule->SetMixabilityStatus(acmmInput.get(), false);
    if (ret != 0) {
        ELOG_ERROR("Fail to unSetMixabilityStatus");
        return;
    }

    acmmGroup->removeInput(inStream);

    if (acmmGroup->allInputsMuted() && acmmGroup->anyOutputsConnected()) {
        std::vector<boost::shared_ptr<AcmmOutput>> outputs;
        acmmGroup->getOutputs(outputs);
        for(auto& o : outputs) {
            o->removeDest(m_outputInfoMap[o.get()].dest);
            if (!m_broadcastGroup->addDest(m_outputInfoMap[o.get()].format, m_outputInfoMap[o.get()].dest)) {
                ELOG_ERROR("Fail to reconnect broadcast dest");
                return;
            }
        }
    }

    if (!acmmGroup->numOfInputs() && !acmmGroup->numOfOutputs()) {
        removeGroup(group);
    }

    if (m_mostActiveInput == acmmInput)
        m_mostActiveInput.reset();

    statistics();
    return;
}

void AcmmFrameMixer::setInputActive(const std::string& group, const std::string& inStream, bool active)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    boost::shared_ptr<AcmmGroup> acmmGroup;
    boost::shared_ptr<AcmmInput> acmmInput;

    ELOG_DEBUG("+++setInputActive: group(%s), inStream(%s), active(%d)", group.c_str(), inStream.c_str(), active);

    acmmGroup = getGroup(group);
    if (!acmmGroup) {
        ELOG_ERROR("Invalid gropu(%s)", group.c_str());
        return;
    }

    acmmInput = acmmGroup->getInput(inStream);
    if (!acmmInput) {
        ELOG_ERROR("Invalid input(%s)", inStream.c_str());
        return;
    }

    if (acmmInput->isActive() == active)
        return;

    acmmInput->setActive(active);

    if (!acmmGroup->numOfOutputs())
        return;

    if (acmmGroup->allInputsMuted() && acmmGroup->anyOutputsConnected()) {
        std::vector<boost::shared_ptr<AcmmOutput>> outputs;
        acmmGroup->getOutputs(outputs);
        for(auto& o : outputs) {
            o->removeDest(m_outputInfoMap[o.get()].dest);
            if (!m_broadcastGroup->addDest(m_outputInfoMap[o.get()].format, m_outputInfoMap[o.get()].dest)) {
                ELOG_ERROR("Fail to reconnect broadcast dest");
                return;
            }
        }
    } else if (!acmmGroup->allInputsMuted() && !acmmGroup->anyOutputsConnected()) {
        std::vector<boost::shared_ptr<AcmmOutput>> outputs;
        acmmGroup->getOutputs(outputs);
        for(auto& o : outputs) {
            m_broadcastGroup->removeDest(m_outputInfoMap[o.get()].dest);
            if (!o->addDest(m_outputInfoMap[o.get()].format, m_outputInfoMap[o.get()].dest)) {
                ELOG_ERROR("Fail to reconnect dest");
                return;
            }
        }
    }

    statistics();
    ELOG_DEBUG("---setInputActive: group(%s), inStream(%s), active(%d)", group.c_str(), inStream.c_str(), active);
}

bool AcmmFrameMixer::addOutput(const std::string& group, const std::string& outStream, const owt_base::FrameFormat format, owt_base::FrameDestination* destination)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    boost::shared_ptr<AcmmGroup> acmmGroup;
    boost::shared_ptr<AcmmOutput> acmmOutput;
    boost::shared_ptr<AcmmOutput> acmmBroadcastOutput;
    int ret;

    ELOG_DEBUG("addOutput: group(%s), outStream(%s), format(%s), dest(%p)", group.c_str(), outStream.c_str(), getFormatStr(format), destination);

    acmmGroup = getGroup(group);
    if (!acmmGroup) {
        acmmGroup = addGroup(group);
        if (!acmmGroup) {
            ELOG_ERROR("Can not add output group");
            return false;
        }
    }

    acmmOutput = acmmGroup->getOutput(outStream);
    if (acmmOutput) {
        ELOG_DEBUG("Update previous output");

        if (acmmGroup->allInputsMuted()) {
            m_broadcastGroup->removeDest(m_outputInfoMap[acmmOutput.get()].dest);
            m_outputInfoMap.erase(acmmOutput.get());

            if (!m_broadcastGroup->addDest(format, destination)) {
                ELOG_ERROR("Fail to update broadcast dest");
                return false;
            }
        } else {
            acmmOutput->removeDest(m_outputInfoMap[acmmOutput.get()].dest);
            m_outputInfoMap.erase(acmmOutput.get());

            if (!acmmOutput->addDest(format, destination)) {
                ELOG_ERROR("Fail to update dest");
                return false;
            }
        }

        OutputInfo outputInfo;
        outputInfo.format = format;
        outputInfo.dest = destination;
        m_outputInfoMap[acmmOutput.get()] = outputInfo;
    } else {
        acmmOutput = acmmGroup->addOutput(outStream);
        if(!acmmOutput) {
            ELOG_ERROR("Fail to add output");
            return false;
        }

        if ((acmmGroup->numOfOutputs() == 1) && acmmGroup->numOfInputs()) {
            std::vector<boost::shared_ptr<AcmmInput>> inputs;
            acmmGroup->getInputs(inputs);
            for(auto& i : inputs) {
                ret = m_mixerModule->SetAnonymousMixabilityStatus(i.get(), false);
                if (ret != 0) {
                    ELOG_WARN("Fail to unSetAnonymousMixabilityStatus");
                }
            }
        }

        if (acmmGroup->allInputsMuted()) {
            if (!m_broadcastGroup->addDest(format, destination)) {
                ELOG_ERROR("Fail to add broadcast dest");
                return false;
            }
        } else {
            if (!acmmOutput->addDest(format, destination)) {
                ELOG_ERROR("Fail to add dest");
                return false;
            }
        }

        OutputInfo outputInfo;
        outputInfo.format = format;
        outputInfo.dest = destination;
        m_outputInfoMap[acmmOutput.get()] = outputInfo;
    }

    updateFrequency();

    statistics();
    return true;
}

void AcmmFrameMixer::removeOutput(const std::string& group, const std::string& outStream)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    boost::shared_ptr<AcmmGroup> acmmGroup;
    boost::shared_ptr<AcmmOutput> acmmOutput;
    int ret;

    ELOG_DEBUG("removeOutput: group(%s), outStream(%s)", group.c_str(), outStream.c_str());

    acmmGroup = getGroup(group);
    if (!acmmGroup) {
        ELOG_ERROR("Invalid gropu(%s)", group.c_str());
        return;
    }

    acmmOutput = acmmGroup->getOutput(outStream);
    if (!acmmOutput) {
        ELOG_ERROR("Invalid output(%s)", outStream.c_str());
        return;
    }

    if ((acmmGroup->numOfOutputs() == 1) && acmmGroup->numOfInputs()) {
        std::vector<boost::shared_ptr<AcmmInput>> inputs;
        acmmGroup->getInputs(inputs);
        for(auto& i : inputs) {
            ret = m_mixerModule->SetAnonymousMixabilityStatus(i.get(), true);
            if (ret != 0) {
                ELOG_WARN("Fail to unSetAnonymousMixabilityStatus");
            }
        }
    }

    if (acmmGroup->allInputsMuted()) {
        m_broadcastGroup->removeDest(m_outputInfoMap[acmmOutput.get()].dest);
    } else {
        acmmOutput->removeDest(m_outputInfoMap[acmmOutput.get()].dest);
    }
    m_outputInfoMap.erase(acmmOutput.get());

    acmmGroup->removeOutput(outStream);
    if (!acmmGroup->numOfInputs() && !acmmGroup->numOfOutputs()) {
        removeGroup(group);
    }

    updateFrequency();

    statistics();
    return;
}

void AcmmFrameMixer::updateFrequency()
{
    int32_t maxFreq = m_broadcastGroup->NeededFrequency();
    int32_t freq;
    int ret;

    for (auto& g : m_groups) {
        freq = g.second->NeededFrequency();
        if (freq > maxFreq)
            maxFreq= freq;
    }

    if (m_frequency != maxFreq) {
        ret = m_mixerModule->SetMinimumMixingFrequency(convert2Frequency(maxFreq));
        if (ret != 0) {
            ELOG_WARN("Fail to SetMinimumMixingFrequency, %d", maxFreq);
            return;
        }

        ELOG_DEBUG("Max mixing frequency %d -> %d", m_frequency, maxFreq);
        m_frequency = maxFreq;
    }

    return;
}

void AcmmFrameMixer::onTimeout()
{
    performMix();
}

void AcmmFrameMixer::performMix()
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    m_mixerModule->Process();
}

void AcmmFrameMixer::NewMixedAudio(int32_t id,
        const AudioFrame& generalAudioFrame,
        const AudioFrame** uniqueAudioFrames,
        uint32_t size)
{
    std::map<uint16_t, bool> groupMap;
    for(uint32_t i = 0; i< size; i++) {
        uint16_t groupId = (uniqueAudioFrames[i]->id_ >> 16) & 0xffff;

        ELOG_TRACE("NewMixedAudio, frame id(0x%x), groupId(%u)"
                , uniqueAudioFrames[i]->id_
                , groupId);

        if (m_groups.find(groupId) != m_groups.end()) {
            boost::shared_ptr<AcmmGroup> acmmGroup = m_groups[groupId];
            if (acmmGroup->numOfInputs()) {
                if (acmmGroup->numOfOutputs()) {
                    acmmGroup->NewMixedAudio(uniqueAudioFrames[i]);
                }

                groupMap[groupId] = true;
            }
        }
    }

    for (auto& p : m_groups) {
        boost::shared_ptr<AcmmGroup> acmmGroup = p.second;
        if (groupMap.find(acmmGroup->id()) == groupMap.end()) {
            if (acmmGroup->numOfOutputs()) {
                acmmGroup->NewMixedAudio(&generalAudioFrame);
            }
        }
    }

    m_broadcastGroup->NewMixedAudio(&generalAudioFrame);
}

boost::shared_ptr<AcmmInput> AcmmFrameMixer::getInputById(int32_t id)
{
    uint16_t groupId = (id >> 16) & 0xffff;
    uint16_t streamId = id & 0xffff;

    if (m_groups.find(groupId) != m_groups.end())
        return m_groups[groupId]->getInput(streamId);

    return NULL;
}

void AcmmFrameMixer::VadParticipants(const ParticipantVadStatistics *statistics, const uint32_t size)
{
    if (!m_asyncHandle || !m_vadEnabled || size < 1) {
        ELOG_TRACE("VAD skipped, asyncHandle(%p), enabled(%d), size(%d)", m_asyncHandle, m_vadEnabled, size);
        return;
    }

    const ParticipantVadStatistics* active = NULL;
    const ParticipantVadStatistics* p = statistics;
    boost::shared_ptr<AcmmInput> activeAcmmInput;
    boost::shared_ptr<AcmmInput> acmmInput;

    for(uint32_t i = 0; i < size; i++, p++) {
        ELOG_TRACE("%d, vad streamId(0x%x), energy(%u)", i, p->id, p->energy);

        if (p->energy == 0)
            continue;

        if (!active || p->energy > active->energy) {
            acmmInput = getInputById(p->id);
            if (!acmmInput) {
                ELOG_TRACE("Not valid vad streamId(0x%x)", p->id);
                continue;
            }

            active = p;
            activeAcmmInput = acmmInput;
        }
    }

    if (activeAcmmInput && m_mostActiveInput != activeAcmmInput) {
        ELOG_TRACE("Active vad %s -> %s"
                , m_mostActiveInput ? m_mostActiveInput->name().c_str() : "NULL"
                , activeAcmmInput->name().c_str());

        m_mostActiveInput = activeAcmmInput;
        m_asyncHandle->notifyAsyncEvent("vad", m_mostActiveInput->name().c_str());
    }
}

void AcmmFrameMixer::statistics()
{
    uint32_t activeCount = 0;
    uint32_t mutedCount = 0;
    uint32_t receivedOnlyCount = 0;
    uint32_t streamInCount = 0;
    uint32_t unknownCount = 0;

    for (auto& p : m_groups) {
        boost::shared_ptr<AcmmGroup> acmmGroup = p.second;

        if(!acmmGroup->allInputsMuted() && acmmGroup->anyOutputsConnected())
            activeCount++;
        else if(acmmGroup->numOfInputs() && acmmGroup->allInputsMuted() && acmmGroup->numOfOutputs())
            mutedCount++;
        else if(acmmGroup->numOfInputs() && acmmGroup->numOfOutputs() == 0)
            streamInCount++;
        else if(acmmGroup->numOfInputs() == 0 && acmmGroup->numOfOutputs() != 0)
            receivedOnlyCount++;
        else
            unknownCount++;
    }

    ELOG_DEBUG("All(%ld), Active(%d), Muted(%d), ReceivedOnly(%d), StreamIn(%d), Unknown(%d)"
            , m_groups.size()
            , activeCount
            , mutedCount
            , receivedOnlyCount
            , streamInCount
            , unknownCount
            );
}

} /* namespace mcu */
