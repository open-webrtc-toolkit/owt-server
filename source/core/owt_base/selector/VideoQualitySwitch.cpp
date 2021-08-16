// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoQualitySwitch.h"

#include <chrono>
#include <cmath>
#include <memory>

namespace owt_base {

// Treat streams frames in a certain period(ms)
static constexpr uint32_t kBitrateCountPeriod = 5000;
static constexpr uint32_t kBucketNum = 50;
// Minimal period for switching(ms)
static constexpr uint64_t kMinimalUpdatePeriod = 3000;

DEFINE_LOGGER(VideoQualitySwitch, "owt.VideoQualitySwitch");

VideoQualitySwitch::VideoQualitySwitch(std::vector<FrameSource*> sources)
    : m_sources(sources)
    , m_bitrateCounters(m_sources.size())
    , m_current(-1)
    , m_lastUpdateTime(0)
{
    ELOG_DEBUG("Init with sources size: %zu", m_sources.size());
    for (size_t i = 0; i < m_sources.size(); i++) {
        if (m_sources[i]) {
            m_bitrateCounters[i] = std::make_shared<BitrateCounter>();
            m_sources[i]->addVideoDestination(m_bitrateCounters[i].get());
        } else {
            ELOG_WARN("Empty source for quality switch %zu", i);
        }
    }
    setTargetBitrate(0);
}

VideoQualitySwitch::~VideoQualitySwitch()
{
    for (size_t i = 0; i < m_sources.size(); i++) {
        if (m_sources[i]) {
            m_sources[i]->removeVideoDestination(m_bitrateCounters[i].get());
        }
    }
}

void VideoQualitySwitch::onFrame(const Frame& frame)
{
    deliverFrame(frame);
}

void VideoQualitySwitch::onMetaData(const MetaData& metadata)
{
    deliverMetaData(metadata);
}

void VideoQualitySwitch::onFeedback(const owt_base::FeedbackMsg& msg)
{
    if (msg.type == owt_base::VIDEO_FEEDBACK && msg.cmd == SET_BITRATE) {
        setTargetBitrate(msg.data.kbps * 1000);
    } else {
        deliverFeedbackMsg(msg);
    }
}

void VideoQualitySwitch::setTargetBitrate(uint32_t targetBps)
{
    ELOG_DEBUG("setTargetBitrate %u", targetBps);
    uint64_t tsNow = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    if (tsNow - m_lastUpdateTime < kMinimalUpdatePeriod) {
        return;
    }
    m_lastUpdateTime = tsNow;

    int current = -1;
    int currentBitrate = 0;
    for (size_t i = 0; i < m_sources.size(); i++) {
        int bitrate = m_bitrateCounters[i]->bitrate();
        ELOG_DEBUG("BitrateCounter bitrate %zu: %d ", i, bitrate);
        if (current < 0) {
            current = i;
            currentBitrate = bitrate;
        } else {
            if (std::abs(bitrate - (int) targetBps) <
                std::abs(currentBitrate - (int) targetBps)) {
                current = i;
                currentBitrate = bitrate;
            }
        }
    }
    if (current != m_current) {
        if (m_current >= 0 && m_sources[m_current]) {
            ELOG_DEBUG("Disable source index %u", m_current);
            m_sources[m_current]->removeVideoDestination(this);
        }
        m_current = current;
        if (m_current >= 0 && m_sources[m_current]) {
            ELOG_DEBUG("Enable source index %u", m_current);
            m_sources[m_current]->addVideoDestination(this);
        }
    }
}

void VideoQualitySwitch::BitrateCounter::onFrame(const Frame& frame)
{
    ELOG_TRACE("BitrateCounter onFrame %u %p", frame.length, this);
    uint64_t tsNow = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    const uint32_t bucketInterval = kBitrateCountPeriod / kBucketNum;
    if (m_timeFrames.empty() ||
        (tsNow - m_timeFrames.back().timeStamp) >= bucketInterval) {
        Bucket bucket(tsNow);
        m_timeFrames.push_back(bucket);
    }
    m_timeFrames.back().total += frame.length * 8;
    m_totalBits += frame.length * 8;

    while (m_timeFrames.size() > kBucketNum) {
        m_totalBits -= m_timeFrames.front().total;
        m_timeFrames.pop_front();
    }
}

uint32_t VideoQualitySwitch::BitrateCounter::bitrate()
{
    if (m_timeFrames.size() == 0) {
        return 0;
    }
    uint32_t countedMs = m_timeFrames.size() * (kBitrateCountPeriod / kBucketNum);
    uint32_t bitrate = m_totalBits * 1000 / countedMs;
    return bitrate;
}

} // namespace owt_base
