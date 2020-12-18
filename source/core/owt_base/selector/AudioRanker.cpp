// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioRanker.h"
#include <chrono>

namespace owt_base {

using std::string;
using std::vector;

// Treat streams without frames in a certain period as muted
static constexpr uint64_t kNoFrameThresholdMs = 600;
static constexpr int kRankChangeInterval = 200;

DEFINE_LOGGER(AudioRanker, "owt.AudioRanker");

AudioRanker::AudioRanker(AudioRanker::Visitor* visitor, bool detectMute, uint32_t minChangeInterval)
    : m_detectMute(detectMute)
    , m_stashChange(false)
    , m_minChangeInterval(minChangeInterval)
    , m_lastChangeTime(0)
    , m_service(new IOService())
    , m_visitor(visitor)
{
}

AudioRanker::~AudioRanker()
{
}

void AudioRanker::addOutput(FrameDestination* output)
{
    ELOG_DEBUG("addOutput");
    m_service->service().dispatch([this, output]() {
        ELOG_DEBUG("addOutput %p %zu", output, m_outputIndexes.size());

        if (m_outputIndexes.count(output) == 0) {
            m_outputIndexes.emplace(output, m_outputIndexes.size());
        }
        if (m_others.empty()) {
            // No pending inputs
            m_unlinkedOutputs.push_back(output);
        } else {
            // Link inputs with largest level
            auto it = m_others.end();
            it--;
            auto newIt = m_topK.emplace(it->first, it->second);
            m_others.erase(it);
            newIt->second->setLinkedOutput(output);
            newIt->second->setIter(newIt);
        }
        ELOG_DEBUG("triggerRankChange when addOutput");
        triggerRankChange();
    });
}

void AudioRanker::addInput(FrameSource* input, std::string streamId, std::string ownerId)
{
    ELOG_DEBUG("addInput: %s %s", streamId.c_str(), ownerId.c_str());
    m_service->service().dispatch([this, input, streamId, ownerId]() {
        if (m_processors.count(streamId) > 0) {
            // Already exist
            return;
        }
        auto audioProc = std::make_shared<AudioLevelProcessor>(this, input, streamId, ownerId);
        m_processors.emplace(streamId, audioProc);
        if (m_unlinkedOutputs.empty()) {
            auto newIt = m_others.emplace(0, audioProc);
            audioProc->setIter(newIt);
        } else {
            FrameDestination* output = m_unlinkedOutputs.back();
            m_unlinkedOutputs.pop_back();

            audioProc->setLinkedOutput(output);
            auto newIt = m_topK.emplace(0, audioProc);
            audioProc->setIter(newIt);

            ELOG_DEBUG("triggerRankChange when addInput");
            triggerRankChange();
        }
    });
}

void AudioRanker::removeInput(std::string streamId)
{
    ELOG_DEBUG("removeInput: %s", streamId.c_str());
    m_service->service().dispatch([this, streamId]() {
        if (m_processors.count(streamId) == 0) {
            // Not exist
            return;
        }
        auto audioProc = m_processors[streamId];
        m_processors.erase(streamId);

        if (audioProc->linkedOutput()) {
            // Remove in top K
            m_topK.erase(audioProc->iter());
            addOutput(audioProc->linkedOutput());
        } else {
            // Remove in others
            m_others.erase(audioProc->iter());
        }
    });
}

void AudioRanker::updateInput(std::string streamId, int level)
{
    ELOG_TRACE("updateInput %s", streamId.c_str());
    m_service->service().dispatch([this, streamId, level]() {
        updateInputInternal(streamId, level, true);
    });
}

void AudioRanker::updateInputInternal(std::string streamId, int level, bool triggerChange)
{
    // Put this in IO service
    ELOG_TRACE("updateInputInternal %s %d", streamId.c_str(), level);
    if (m_processors.count(streamId) == 0) {
        return;
    }

    auto audioProc = m_processors[streamId];
    if (audioProc->linkedOutput()) {
        // Previous in top K, check if it's still in
        bool inTopK = true;
        if (!m_others.empty()) {
            auto it = m_others.end();
            it--;
            if (it->first > level) {
                inTopK = false;
                // Swap them
                auto newIt = m_topK.emplace(it->first, it->second);
                m_others.erase(it);
                newIt->second->setLinkedOutput(audioProc->linkedOutput());
                newIt->second->setIter(newIt);

                newIt = m_others.emplace(level, audioProc);
                m_topK.erase(audioProc->iter());
                audioProc->setLinkedOutput(nullptr);
                audioProc->setIter(newIt);
                ELOG_TRACE("triggerRankChange updateInputInternal %s remove in top", streamId.c_str());
                if (triggerChange) {
                    triggerRankChange();
                }
            }
        }

        if (inTopK && level != audioProc->iter()->first) {
            m_topK.erase(audioProc->iter());
            auto newIt = m_topK.emplace(level, audioProc);
            audioProc->setIter(newIt);
        }
    } else {
        // Previous in others, check if it's still in
        bool inTopK = false;
        if (!m_topK.empty()) {
            auto it = m_topK.begin();
            if (level > it->first) {
                inTopK = true;
                // Swap them
                auto newIt = m_topK.emplace(level, audioProc);
                m_others.erase(audioProc->iter());
                audioProc->setLinkedOutput(it->second->linkedOutput());
                audioProc->setIter(newIt);

                newIt = m_others.emplace(it->first, it->second);
                m_topK.erase(it);
                newIt->second->setLinkedOutput(nullptr);
                newIt->second->setIter(newIt);
                ELOG_TRACE("triggerRankChange updateInputInternal %s add in top", streamId.c_str());
                if (triggerChange) {
                    triggerRankChange();
                }
            }
        }

        if (!inTopK && level != audioProc->iter()->first) {
            m_others.erase(audioProc->iter());
            auto newIt = m_others.emplace(level, audioProc);
            audioProc->setIter(newIt);
        }
    }

    if (triggerChange && m_stashChange) {
        ELOG_TRACE("triggerRankChange updateInputInternal stash");
        triggerRankChange();
    }
}

void AudioRanker::triggerRankChange()
{
    // In IO service thread
    uint64_t tsNow = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

    ELOG_TRACE("triggerRankChange %zu, %zu", tsNow, m_lastChangeTime);
    if (tsNow - m_lastChangeTime < m_minChangeInterval) {
        // No trigger within rank change interval
        ELOG_TRACE("within change interval");
        m_stashChange = true;
        return;
    }
    m_lastChangeTime = tsNow;
    m_stashChange = false;

    if (m_visitor) {
        ELOG_DEBUG("update output size: %zu", m_outputIndexes.size());
        vector<std::pair<string, string>> updates(
            m_outputIndexes.size(), std::pair<string, string>());

        if (m_detectMute) {
            // Check last update time before change
            vector<string> mutedStreamIds;
            for (auto& pair : m_topK) {
                auto audioProc = pair.second;
                if (tsNow - audioProc->lastUpdateTime() > kNoFrameThresholdMs) {
                    mutedStreamIds.push_back(audioProc->streamId());
                }
            }

            if (!mutedStreamIds.empty()) {
                // Detect muted streams
                for (string& mutedId : mutedStreamIds) {
                    ELOG_DEBUG("muted stream: %s", mutedId.c_str());
                    updateInputInternal(mutedId, 0, false);
                }
            }
        }

        for (auto& pair : m_topK) {
            auto audioProc = pair.second;
            FrameDestination* output = audioProc->linkedOutput();
            int index = m_outputIndexes[output];
            ELOG_DEBUG("update output index: %d, streamId: %s",
                index, audioProc->streamId().c_str());
            updates[index].first = audioProc->streamId();
            updates[index].second = audioProc->ownerId();
        }

        bool hasChange = false;
        if (m_lastUpdates.size() == updates.size()) {
            for (size_t i = 0; i < updates.size(); i++) {
                if (updates[i].first != m_lastUpdates[i].first ||
                    updates[i].second != m_lastUpdates[i].second) {
                    hasChange = true;
                    break;
                }
            }
        } else {
            hasChange = true;
        }

        if (hasChange) {
            ELOG_DEBUG("Notify rank changes");
            m_lastUpdates = updates;
            m_visitor->onRankChange(updates);
        }
    }
}

//=============================================================================

AudioRanker::AudioLevelProcessor::AudioLevelProcessor(
    AudioRanker* parent, FrameSource* source,
    std::string streamId, std::string ownerId)
    : m_parent(parent)
    , m_source(source)
    , m_streamId(streamId)
    , m_ownerId(ownerId)
    , m_lastUpdateTime(0)
    , m_linkedOutput(nullptr)
{
    m_source->addAudioDestination(this);
}

AudioRanker::AudioLevelProcessor::~AudioLevelProcessor()
{
    m_source->removeAudioDestination(this);
}

void AudioRanker::AudioLevelProcessor::setLinkedOutput(FrameDestination* output)
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_linkedOutput) {
        removeAudioDestination(m_linkedOutput);
    }
    m_linkedOutput = output;
    addAudioDestination(m_linkedOutput);
    deliverOwnerData();
}

FrameDestination* AudioRanker::AudioLevelProcessor::linkedOutput()
{
    boost::mutex::scoped_lock lock(m_mutex);
    return m_linkedOutput;
}

void AudioRanker::AudioLevelProcessor::onFrame(const Frame& frame)
{
    // Pass frame to linked output
    deliverFrame(frame);

    if (frame.additionalInfo.audio.voice) {
        uint64_t tsNow = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        // Less the original level, larger the volume
        int revLevel = 127 - frame.additionalInfo.audio.audioLevel;
        m_lastUpdateTime = tsNow;
        m_parent->updateInput(m_streamId, revLevel);
    } else {
        ELOG_TRACE("Frame from %p has no voice", m_source);
    }
}

void AudioRanker::AudioLevelProcessor::onFeedback(const FeedbackMsg& msg)
{
    if (msg.type == AUDIO_FEEDBACK && msg.cmd == REQUEST_OWNER_ID) {
        deliverOwnerData();
    }
}

void AudioRanker::AudioLevelProcessor::deliverOwnerData()
{
    ELOG_DEBUG("deliver ownerID as metadata");
    // Pass owner ID metadata to linkedoutput
    MetaData ownerData;
    ownerData.type = META_DATA_OWNER_ID;
    ownerData.payload = (uint8_t*) m_ownerId.c_str();
    ownerData.length = m_ownerId.length();
    deliverMetaData(ownerData);
}

} // namespace owt_base
