// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef OWT_BASE_SELECTOR_VIDEO_QUALITY_SWITCH_H
#define OWT_BASE_SELECTOR_VIDEO_QUALITY_SWITCH_H

#include "MediaFramePipeline.h"
#include <logger.h>

#include <deque>
#include <memory>
#include <vector>

namespace owt_base {

class VideoQualitySwitch : public FrameSource,
                           public FrameDestination {
    DECLARE_LOGGER();
public:
    VideoQualitySwitch(std::vector<FrameSource*> sources);
    ~VideoQualitySwitch();

    // Implements FrameDestination
    void onFrame(const Frame&) override;
    void onMetaData(const MetaData&) override;

    // Implements FrameDestination
    void onFeedback(const owt_base::FeedbackMsg& msg) override;

    void setTargetBitrate(uint32_t targetBps);

    class BitrateCounter : public FrameDestination {
    public:
        BitrateCounter(): m_totalBits  (0) {}
        BitrateCounter(VideoQualitySwitch* parent)
            : m_totalBits(0)
            , m_parent(parent) {}
        ~BitrateCounter() = default;

        // Implements FrameDestination
        void onFrame(const Frame&) override;

        uint32_t bitrate();
    private:
        struct Bucket {
            Bucket(uint64_t ts) : timeStamp(ts), total(0) {}
            uint64_t timeStamp;
            uint32_t total;
        };
        std::deque<Bucket> m_timeFrames;
        uint32_t m_totalBits;

        VideoQualitySwitch* m_parent;
    };

    // class KeyFrameDetector : public FrameDestination {
    // public:
    //     KeyFrameDetector(VideoQualitySwitch* parent)
    //         : m_targetIdx(-1)
    //         , m_parent(parent) {}
    //     ~KeyFrameDetector() = default;

    //     // Implements FrameDestination
    //     void onFrame(const Frame&) override;
    // private:
    //     int m_targetIdx;
    //     VideoQualitySwitch* m_parent;
    // };

private:
    std::vector<FrameSource*> m_sources;
    std::vector<std::shared_ptr<BitrateCounter>> m_bitrateCounters;
    int m_current;
    bool m_keyFrameArrived;
    uint64_t m_lastUpdateTime;
};

} // namespace owt_base

#endif // OWT_BASE_SELECTOR_VIDEO_QUALITY_SWITCH_H
