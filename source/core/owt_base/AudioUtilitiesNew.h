// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioUtilities_h
#define AudioUtilities_h

#include "MediaFramePipeline.h"

namespace owt_base {

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

struct CodecInst {
  int pltype;
  char plname[32];
  int plfreq;
  int pacsize;
  size_t channels;
  int rate;  // bits/sec unlike {start,min,max}Bitrate elsewhere in this file!
};

FrameFormat getAudioFrameFormat(int pltype);
bool getAudioCodecInst(FrameFormat format, CodecInst& audioCodec);
int getAudioPltype(FrameFormat format);
int32_t getAudioSampleRate(const FrameFormat format);
uint32_t getAudioChannels(const FrameFormat format);

} /* namespace owt_base */

#endif /* AudioUtilities_h */
