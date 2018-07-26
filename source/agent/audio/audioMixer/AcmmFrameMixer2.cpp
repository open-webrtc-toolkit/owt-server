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

#include "AcmmFrameMixer2.h"

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

DEFINE_LOGGER(AcmmFrameMixer2, "mcu.media.AcmmFrameMixer2");

AcmmFrameMixer2::AcmmFrameMixer2()
    : m_asyncHandle(NULL)
    , m_vadEnabled(false)
    , m_frequency(0)
{
    m_mixerModule.reset(AudioConferenceMixer::Create(0));
    m_mixerModule->RegisterMixedStreamCallback(this);
    m_mixerModule->SetMultipleInputs(true);

    m_groupIds.resize(MAX_GROUPS);
    for (size_t i = 0; i < MAX_GROUPS; ++i)
        m_groupIds[i] = true;

    m_jobTimer.reset(new JobTimer(MIXER_FREQUENCY, this));
}

AcmmFrameMixer2::~AcmmFrameMixer2()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    m_jobTimer->stop();

    m_mixerModule->UnRegisterMixedStreamCallback();

    if (m_vadEnabled) {
        m_mixerModule->UnRegisterMixerVadCallback();
        m_vadEnabled = false;
    }
}

bool AcmmFrameMixer2::getFreeGroupId(uint16_t *id)
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

boost::shared_ptr<AcmmGroup> AcmmFrameMixer2::getGroup(const std::string& group)
{
    if (m_groupIdMap.find(group) == m_groupIdMap.end())
        return NULL;

    if (m_groups.find(m_groupIdMap[group]) == m_groups.end())
        return NULL;

    return m_groups[m_groupIdMap[group]];
}

boost::shared_ptr<AcmmGroup> AcmmFrameMixer2::addGroup(const std::string& group)
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

void AcmmFrameMixer2::removeGroup(const std::string& group)
{
    m_groups.erase(m_groupIdMap[group]);
    m_groupIds[m_groupIdMap[group]] = true;
    m_groupIdMap.erase(group);
}

void AcmmFrameMixer2::setEventRegistry(EventRegistry* handle)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_TRACE("setEventRegistry(%p)", handle);

    m_asyncHandle = handle;
}

void AcmmFrameMixer2::enableVAD(uint32_t period)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("enableVAD, period(%u)", period);

    m_vadEnabled = true;
    m_mostActiveInput.reset();
    m_mixerModule->RegisterMixerVadCallback(this, period / 10);
}

void AcmmFrameMixer2::disableVAD()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("disableVAD");

    m_vadEnabled = false;
    m_mostActiveInput.reset();
    m_mixerModule->UnRegisterMixerVadCallback();
}

void AcmmFrameMixer2::resetVAD()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("resetVAD");

    m_mostActiveInput.reset();
}

bool AcmmFrameMixer2::addInput(const std::string& group, const std::string& inStream, const woogeen_base::FrameFormat format, woogeen_base::FrameSource* source)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    boost::shared_ptr<AcmmGroup> acmmGroup;
    boost::shared_ptr<AcmmInput> acmmInput;
    int ret;

    ELOG_DEBUG("addInput: group(%s), inStream(%s), format(%s)", group.c_str(), inStream.c_str(), getFormatStr(format));

    acmmGroup = getGroup(group);
    if (!acmmGroup) {
        acmmGroup = addGroup(group);
        if (!acmmGroup) {
            ELOG_WARN("Can not add input group");
            return false;
        }
    }

    acmmInput = acmmGroup->getInput(inStream);
    if (acmmInput) {
        ELOG_DEBUG("Update previous input");

        acmmInput->unsetSource();
        if(!acmmInput->setSource(format, source)) {
            ELOG_ERROR("Fail to update input");
            return false;
        }
    } else {
        acmmInput = acmmGroup->addInput(inStream);
        if(!acmmInput) {
            ELOG_ERROR("Fail to add input");
            return false;
        }

        if (!acmmInput->setSource(format, source)) {
            ELOG_ERROR("Fail to set input");
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
    }

    return true;
}

void AcmmFrameMixer2::removeInput(const std::string& group, const std::string& inStream)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    boost::shared_ptr<AcmmGroup> acmmGroup;
    boost::shared_ptr<AcmmInput> acmmInput;
    int ret;

    ELOG_DEBUG("removeInput: group(%s), inStream(%s)", group.c_str(), inStream.c_str());

    acmmGroup = getGroup(group);
    if (!acmmGroup) {
        ELOG_ERROR("removeInput: Invalid gropu(%s)", group.c_str());
        return;
    }

    acmmInput = acmmGroup->getInput(inStream);
    if (!acmmInput) {
        ELOG_ERROR("removeInput: Invalid input(%s)", inStream.c_str());
        return;
    }

    ret = m_mixerModule->SetMixabilityStatus(acmmInput.get(), false);
    if (ret != 0) {
        ELOG_ERROR("Fail to unSetMixabilityStatus");
        return;
    }

    acmmGroup->removeInput(inStream);
    if (!acmmGroup->numOfInputs() && !acmmGroup->numOfOutputs()) {
        removeGroup(group);
    }

    if (m_mostActiveInput == acmmInput)
        m_mostActiveInput.reset();

    return;
}

bool AcmmFrameMixer2::addOutput(const std::string& group, const std::string& outStream, const woogeen_base::FrameFormat format, woogeen_base::FrameDestination* destination)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    boost::shared_ptr<AcmmGroup> acmmGroup;
    boost::shared_ptr<AcmmOutput> acmmOutput;
    int ret;

    ELOG_DEBUG("addOutput: group(%s), outStream(%s), format(%s)", group.c_str(), outStream.c_str(), getFormatStr(format));

    acmmGroup = getGroup(group);
    if (!acmmGroup) {
        acmmGroup = addGroup(group);
        if (!acmmGroup) {
            ELOG_WARN("Can not add input group");
            return false;
        }
    }

    acmmOutput = acmmGroup->getOutput(outStream);
    if (acmmOutput) {
        ELOG_DEBUG("Update previous output");

        acmmOutput->removeDest(m_dstMap[acmmOutput.get()]);
        m_dstMap.erase(acmmOutput.get());

        if(!acmmOutput->addDest(format, destination)) {
            ELOG_ERROR("Fail to update output");
            return false;
        }

        m_dstMap[acmmOutput.get()] = destination;
    } else {
        acmmOutput = acmmGroup->addOutput(outStream);
        if(!acmmOutput) {
            ELOG_ERROR("Fail to add output");
            return false;
        }

        if (!acmmOutput->addDest(format, destination)) {
            ELOG_ERROR("Fail to set output");
            return false;
        }

        if ((acmmGroup->numOfOutputs() == 1) && acmmGroup->numOfInputs() ) {
            std::vector<boost::shared_ptr<AcmmInput>> inputs;
            acmmGroup->getInputs(inputs);
            for(auto& i : inputs) {
                ret = m_mixerModule->SetAnonymousMixabilityStatus(i.get(), false);
                if (ret != 0) {
                    ELOG_WARN("Fail to unSetAnonymousMixabilityStatus");
                }
            }
        }

        m_dstMap[acmmOutput.get()] = destination;
    }

    updateFrequency();
    return true;
}

void AcmmFrameMixer2::removeOutput(const std::string& group, const std::string& outStream)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    boost::shared_ptr<AcmmGroup> acmmGroup;
    boost::shared_ptr<AcmmOutput> acmmOutput;
    int ret;

    ELOG_DEBUG("removeOutput: group(%s), outStream(%s)", group.c_str(), outStream.c_str());

    acmmGroup = getGroup(group);
    if (!acmmGroup) {
        ELOG_ERROR("removeOutput: Invalid gropu(%s)", group.c_str());
        return;
    }

    acmmOutput = acmmGroup->getOutput(outStream);
    if (!acmmOutput) {
        ELOG_ERROR("removeOutput: Invalid output(%s)", outStream.c_str());
        return;
    }

    if ((acmmGroup->numOfOutputs() == 1) && acmmGroup->numOfInputs() ) {
        std::vector<boost::shared_ptr<AcmmInput>> inputs;
        acmmGroup->getInputs(inputs);
        for(auto& i : inputs) {
            ret = m_mixerModule->SetAnonymousMixabilityStatus(i.get(), true);
            if (ret != 0) {
                ELOG_WARN("Fail to unSetAnonymousMixabilityStatus");
            }
        }
    }

    m_dstMap.erase(acmmOutput.get());
    acmmGroup->removeOutput(outStream);
    if (!acmmGroup->numOfInputs() && !acmmGroup->numOfOutputs()) {
        removeGroup(group);
    }

    updateFrequency();
    return;
}

void AcmmFrameMixer2::updateFrequency()
{
    int32_t maxFreq = m_frequency;
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

        m_frequency = maxFreq;
    }

    return;
}

void AcmmFrameMixer2::onTimeout()
{
    performMix();
}

void AcmmFrameMixer2::performMix()
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    m_mixerModule->Process();
}

void AcmmFrameMixer2::NewMixedAudio(int32_t id,
        const AudioFrame& generalAudioFrame,
        const AudioFrame** uniqueAudioFrames,
        uint32_t size)
{
    std::map<uint16_t, bool> groupMap;
    for(uint32_t i = 0; i< size; i++) {
        uint16_t groupId = (uniqueAudioFrames[i]->id_ >> 16) & 0xffff;

        ELOG_TRACE_T("NewMixedAudio, frame id(0x%x), groupId(%u)"
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
}

boost::shared_ptr<AcmmInput> AcmmFrameMixer2::getInputById(int32_t id)
{
    uint16_t groupId = (id >> 16) & 0xffff;
    uint16_t streamId = id & 0xffff;

    if (m_groups.find(groupId) != m_groups.end())
        return m_groups[groupId]->getInput(streamId);

    return NULL;
}

void AcmmFrameMixer2::VadParticipants(const ParticipantVadStatistics *statistics, const uint32_t size)
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
        ELOG_TRACE("active vad %s -> %s"
                , m_mostActiveInput ? m_mostActiveInput->name().c_str() : "NULL"
                , activeAcmmInput->name().c_str());

        m_mostActiveInput = activeAcmmInput;
        m_asyncHandle->notifyAsyncEvent("vad", m_mostActiveInput->name().c_str());
    }
}

} /* namespace mcu */
