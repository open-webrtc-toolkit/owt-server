// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioUtilities_h
#define AudioUtilities_h

#include <webrtc/common_types.h>

#include "MediaFramePipeline.h"

namespace owt_base {

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

FrameFormat getAudioFrameFormat(int pltype);
bool getAudioCodecInst(owt_base::FrameFormat format, webrtc::CodecInst& audioCodec);
int getAudioPltype(owt_base::FrameFormat format);
int32_t getAudioSampleRate(const owt_base::FrameFormat format);
uint32_t getAudioChannels(const owt_base::FrameFormat format);

} /* namespace owt_base */

#endif /* AudioUtilities_h */
