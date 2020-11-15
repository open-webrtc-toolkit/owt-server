// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameTranscoder_h
#define VideoFrameTranscoder_h

#include "VideoHelper.h"
#include <MediaFramePipeline.h>

namespace mcu {

class VideoFrameTranscoder {
public:
    virtual bool setInput(int input, owt_base::FrameFormat, owt_base::FrameSource*) = 0;
    virtual void unsetInput(int input) = 0;

#ifndef BUILD_FOR_ANALYTICS
    virtual bool addOutput(int output,
            owt_base::FrameFormat,
            const owt_base::VideoCodecProfile profile,
            const owt_base::VideoSize&,
            const unsigned int framerateFPS,
            const unsigned int bitrateKbps,
            const unsigned int keyFrameIntervalSeconds,
            owt_base::FrameDestination*) = 0;
#else
    virtual bool addOutput(int output,
            owt_base::FrameFormat,
            const owt_base::VideoCodecProfile profile,
            const owt_base::VideoSize&,
            const unsigned int framerateFPS,
            const unsigned int bitrateKbps,
            const unsigned int keyFrameIntervalSeconds,
            const std::string& algorithm,
            const std::string& pluginName,
            owt_base::FrameDestination*) = 0;
#endif
    virtual void removeOutput(int output) = 0;

    virtual void requestKeyFrame(int output) = 0;
    virtual void setMaxResolution(int output, int width, int height) = 0;
#ifndef BUILD_FOR_ANALYTICS
    virtual void drawText(const std::string& textSpec) = 0;
    virtual void clearText() = 0;
#endif
};

}
#endif
