// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameMixer_h
#define VideoFrameMixer_h

#include "VideoLayout.h"
#include <MediaFramePipeline.h>

namespace mcu {

// VideoFrameCompositor accepts the raw I420VideoFrame from multiple inputs and
// composites them into one I420VideoFrame with the given VideoLayout.
// The composited I420VideoFrame will be handed over to one VideoFrameConsumer.
class VideoFrameCompositor : public owt_base::FrameSource {
public:
    virtual bool activateInput(int input) = 0;
    virtual void deActivateInput(int input) = 0;
    virtual bool setAvatar(int input, const std::string& avatar) = 0;
    virtual bool unsetAvatar(int input) = 0;
    virtual void pushInput(int input, const owt_base::Frame&) = 0;
    virtual void updateLayoutSolution(LayoutSolution& solution) = 0;

    virtual bool addOutput(const uint32_t width, const uint32_t height, const uint32_t framerateFPS, owt_base::FrameDestination *dst) = 0;
    virtual bool removeOutput(owt_base::FrameDestination *dst) = 0;

    virtual void drawText(const std::string& textSpec) = 0;
    virtual void clearText() = 0;
};

// VideoFrameMixer accepts frames from multiple inputs and mixes them.
// It can have multiple outputs with different FrameFormat or framerate/bitrate settings.
class VideoFrameMixer {
public:
    virtual bool addInput(int input, owt_base::FrameFormat, owt_base::FrameSource*, const std::string& avatar) = 0;
    virtual void removeInput(int input) = 0;
    virtual void setInputActive(int input, bool active) = 0;

    virtual bool addOutput(int output,
            owt_base::FrameFormat,
            const owt_base::VideoCodecProfile profile,
            const owt_base::VideoSize&,
            const unsigned int framerateFPS,
            const unsigned int bitrateKbps,
            const unsigned int keyFrameIntervalSeconds,
            owt_base::FrameDestination*) = 0;
    virtual void removeOutput(int output) = 0;

    //virtual void setBitrate(unsigned short kbps, int output) = 0;
    virtual void requestKeyFrame(int output) = 0;

    virtual void updateLayoutSolution(LayoutSolution& solution) = 0;

    virtual void drawText(const std::string& textSpec) = 0;
    virtual void clearText() = 0;
};

}
#endif
