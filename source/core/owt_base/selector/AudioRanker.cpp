// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioRanker.h"
#include <chrono>

namespace owt_base {

using std::string;
using std::vector;

// Treat streams without frames in a certain period as muted
static constexpr uint64_t kNoFrameThreshold = 300;
static constexpr int kRankChangeInterval = 200;

DEFINE_LOGGER(AudioRanker, "owt.AudioRanker");

AudioRanker::AudioRanker(AudioRanker::Visitor* visitor, bool detectMute, int minChangeInterval)
    : m_detectMute(detectMute)
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
    m_service->service().dispatch([this, output]() {
        m_outputIndexes.emplace(output, m_outputIndexes.size());
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

            triggerRankChange();
        }
    });
}

void AudioRanker::addInput(FrameSource* input, std::string streamId, std::string ownerId)
{
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
            triggerRankChange();
        }
    });
}

void AudioRanker::removeInput(std::string streamId)
{
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
    m_service->service().dispatch([this, streamId, level]() {
        privUpdateInput(streamId, level);
    });
}

void AudioRanker::privUpdateInput(std::string streamId, int level)
{
    // Put this in IO service
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
                triggerRankChange();
            }
        }

        if (inTopK && level != audioProc->iter()->first) {
            m_topK.erase(audioProc->iter());
            auto newIt = m_topK.emplace(level, audioProc);
            audioProc->setIter(newIt);
            triggerRankChange();
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
                triggerRankChange();
            }
        }

        if (!inTopK && level != audioProc->iter()->first) {
            m_others.erase(audioProc->iter());
            auto newIt = m_others.emplace(level, audioProc);
            audioProc->setIter(newIt);
        }
    }
}

void AudioRanker::triggerRankChange()
{
    // In IO service thread
    uint64_t tsNow = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

    if (tsNow - m_lastChangeTime < kRankChangeInterval) {
        // No trigger within rank change interval
        return;
    }

    if (m_visitor) {
        ELOG_DEBUG("update output size: %zu", m_outputIndexes.size());
        vector<std::pair<string, string>> updates(
            m_outputIndexes.size(), std::pair<string, string>());
        // Check last update time before change
        vector<string> mutedStreamIds;
        for (auto& pair : m_topK) {
            auto audioProc = pair.second;
            if (tsNow - audioProc->lastUpdateTime() > kNoFrameThreshold) {
                mutedStreamIds.push_back(audioProc->streamId());
            }
        }

        if (!mutedStreamIds.empty()) {
            // Detect muted streams
            for (string& mutedId : mutedStreamIds) {
                privUpdateInput(mutedId, 0);
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
        m_lastChangeTime = tsNow;
        m_visitor->onRankChange(updates);
    }
}


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

void AudioRanker::AudioLevelProcessor::onFrame(const Frame& frame)
{
    if (frame.additionalInfo.audio.voice) {
        // Less the original level, larger the volumn
        int revLevel = 127 - frame.additionalInfo.audio.audioLevel;
        m_parent->updateInput(m_streamId, revLevel);
    }
    // Add lock for m_linkedOutput
    if (m_linkedOutput) {
        m_linkedOutput->onFrame(frame);
    }
}


} // namespace owt_base