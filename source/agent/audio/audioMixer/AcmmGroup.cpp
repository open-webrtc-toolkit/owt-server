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

#include "AcmmGroup.h"

namespace mcu {

using namespace webrtc;
using namespace woogeen_base;

DEFINE_LOGGER(AcmmGroup, "mcu.media.AcmmGroup");

AcmmGroup::AcmmGroup(uint16_t id)
    : m_groupId(id)
{
    ELOG_DEBUG_T("AcmmGroup(%u)", id);

    m_inputIds.resize(_MAX_INPUT_STREAMS_);
    for (size_t i = 0; i < _MAX_INPUT_STREAMS_; ++i)
        m_inputIds[i] = true;

    m_outputIds.resize(_MAX_OUTPUT_STREAMS_);
    for (size_t i = 0; i < _MAX_OUTPUT_STREAMS_; ++i)
        m_outputIds[i] = true;
}

AcmmGroup::~AcmmGroup()
{
    ELOG_DEBUG_T("~AcmmGroup");
}

bool AcmmGroup::getFreeInputId(uint16_t *id)
{
    for (size_t i = 0; i < m_inputIds.size(); ++i) {
        if (m_inputIds[i]) {
            m_inputIds[i] = false;

            *id = i;
            return true;
        }
    }

    ELOG_WARN_T("No free Id, max input reached(%d)!", _MAX_INPUT_STREAMS_);
    return false;
}

bool AcmmGroup::getFreeOutputId(uint16_t *id)
{
    for (size_t i = 0; i < m_outputIds.size(); ++i) {
        if (m_outputIds[i]) {
            m_outputIds[i] = false;

            *id = i;
            return true;
        }
    }

    ELOG_WARN_T("No free Id, max output reached(%d)!", _MAX_OUTPUT_STREAMS_);
    return false;
}

boost::shared_ptr<AcmmInput> AcmmGroup::getInput(const std::string& inStream)
{
    if (m_inputIdMap.find(inStream) == m_inputIdMap.end())
        return NULL;

    if (m_inputs.find(m_inputIdMap[inStream]) == m_inputs.end())
        return NULL;

    return m_inputs[m_inputIdMap[inStream]];
}

boost::shared_ptr<AcmmOutput> AcmmGroup::getOutput(const std::string& outStream)
{
    if (m_outputIdMap.find(outStream) == m_outputIdMap.end())
        return NULL;

    if (m_outputs.find(m_outputIdMap[outStream]) == m_outputs.end())
        return NULL;

    return m_outputs[m_outputIdMap[outStream]];
}

boost::shared_ptr<AcmmInput> AcmmGroup::getInput(uint16_t streamId)
{
    if (m_inputs.find(streamId) == m_inputs.end())
        return NULL;

    return m_inputs[streamId];
}

void AcmmGroup::getInputs(std::vector<boost::shared_ptr<AcmmInput>> &inputs)
{
    uint32_t size = m_inputs.size();
    int i = 0;

    inputs.resize(size);
    for (auto& it : m_inputs) {
         inputs[i++] = it.second;
    }
}

void AcmmGroup::getOutputs(std::vector<boost::shared_ptr<AcmmOutput>> &outputs)
{
    uint32_t size = m_outputs.size();
    int i = 0;

    outputs.resize(size);
    for (auto& it : m_outputs) {
         outputs[i++] = it.second;
    }
}

boost::shared_ptr<AcmmInput> AcmmGroup::addInput(const std::string& inStream)
{
    boost::shared_ptr<AcmmInput> acmmInput;

    if (getInput(inStream)) {
        ELOG_ERROR_T("Input(%s) already added!", inStream.c_str());
        return NULL;
    }

    uint16_t inputId;
    if(!getFreeInputId(&inputId)) {
        return NULL;
    }

    int32_t id = (((int32_t)m_groupId << 16) & 0xffff0000) | inputId;
    acmmInput.reset(new AcmmInput(id, inStream));

    m_inputIdMap[inStream] = inputId;
    m_inputs[inputId] = acmmInput;

    return acmmInput;
}

void AcmmGroup::removeInput(const std::string& inStream)
{
    m_inputs.erase(m_inputIdMap[inStream]);
    m_inputIds[m_inputIdMap[inStream]] = true;
    m_inputIdMap.erase(inStream);
}

boost::shared_ptr<AcmmOutput> AcmmGroup::addOutput(const std::string& outStream)
{
    boost::shared_ptr<AcmmOutput> acmmOutput;

    if (getOutput(outStream)) {
        ELOG_ERROR_T("output(%s) already added!", outStream.c_str());
        return NULL;
    }

    uint16_t outputId;
    if(!getFreeOutputId(&outputId)) {
        return NULL;
    }

    int32_t id = (((int32_t)m_groupId << 16) & 0xffff0000) | outputId;
    acmmOutput.reset(new AcmmOutput(id));

    m_outputIdMap[outStream] = outputId;
    m_outputs[outputId] = acmmOutput;

    return acmmOutput;
}

void AcmmGroup::removeOutput(const std::string& outStream)
{
    m_outputs.erase(m_outputIdMap[outStream]);
    m_outputIds[m_outputIdMap[outStream]] = true;
    m_outputIdMap.erase(outStream);
}

bool AcmmGroup::allInputsMuted()
{
    for (auto& it : m_inputs) {
        if (it.second->isActive())
            return false;
    }

    return true;
}

bool AcmmGroup::anyOutputsConnected()
{
    for (auto& it : m_outputs) {
        if (it.second->hasDest())
            return true;
    }

    return false;
}

int32_t AcmmGroup::NeededFrequency()
{
    int32_t neededFreq = 0;

    for (auto& it : m_outputs) {
        boost::shared_ptr<AcmmOutput> output = it.second;
        int32_t freq = output->NeededFrequency();

        if (neededFreq < freq)
            neededFreq = freq;
    }

    return neededFreq;
}

void AcmmGroup::NewMixedAudio(const AudioFrame* audioFrame)
{
    ELOG_TRACE_T("NewMixedAudio, frame id(0x%x), groupId(%u), sample_rate(%d), channels(%ld), samples_per_channel(%ld), timestamp(%d)",
            audioFrame->id_, m_groupId,
            audioFrame->sample_rate_hz_,
            audioFrame->num_channels_,
            audioFrame->samples_per_channel_,
            audioFrame->timestamp_
            );

    for (auto& it : m_outputs) {
        boost::shared_ptr<AcmmOutput> output = it.second;
        output->newAudioFrame(audioFrame);
    }
}

} /* namespace mcu */
