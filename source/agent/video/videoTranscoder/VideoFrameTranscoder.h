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
    virtual bool setInput(int input, woogeen_base::FrameFormat, woogeen_base::FrameSource*) = 0;
    virtual void unsetInput(int input) = 0;

    virtual bool addOutput(int output,
            woogeen_base::FrameFormat,
            const woogeen_base::VideoCodecProfile profile,
            const woogeen_base::VideoSize&,
            const unsigned int framerateFPS,
            const unsigned int bitrateKbps,
            const unsigned int keyFrameIntervalSeconds,
            woogeen_base::FrameDestination*) = 0;
    virtual void removeOutput(int output) = 0;

    virtual void requestKeyFrame(int output) = 0;

    virtual void drawText(const std::string& textSpec) = 0;
    virtual void clearText() = 0;
};

}
#endif
