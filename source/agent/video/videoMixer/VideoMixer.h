// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoMixer_h
#define VideoMixer_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <map>
#include <set>

#include "MediaFramePipeline.h"
#include "VideoLayout.h"

namespace mcu {

class VideoFrameMixer;

struct VideoMixerConfig {
    uint32_t maxInput;
    bool crop;
    std::string resolution;
    struct {
        int r;
        int g;
        int b;
    } bgColor;
    bool useGacc;
    uint32_t MFE_timeout;
};

class VideoMixer {
    DECLARE_LOGGER();

public:
    VideoMixer(const VideoMixerConfig& config);
    virtual ~VideoMixer();

    bool addInput(const int inputIndex, const std::string& codec, woogeen_base::FrameSource* source, const std::string& avatar);
    void removeInput(const int inputIndex);
    void setInputActive(const int inputIndex, bool active);
    bool addOutput(const std::string& outStreamID
            , const std::string& codec
            , const woogeen_base::VideoCodecProfile profile
            , const std::string& resolution
            , const unsigned int framerateFPS
            , const unsigned int bitrateKbps
            , const unsigned int keyFrameIntervalSeconds
            , woogeen_base::FrameDestination* dest);
    void removeOutput(const std::string& outStreamID);
    void forceKeyFrame(const std::string& outStreamID);

    // Update Layout solution
    void updateLayoutSolution(LayoutSolution& solution);

    void drawText(const std::string& textSpec);
    void clearText();

private:
    void closeAll();

    int m_nextOutputIndex;

    boost::shared_ptr<VideoFrameMixer> m_frameMixer;

    uint32_t m_maxInputCount;
    std::set<int> m_inputs;

    boost::shared_mutex m_outputsMutex;
    std::map<std::string, int32_t> m_outputs;
};

} /* namespace mcu */
#endif /* VideoMixer_h */
