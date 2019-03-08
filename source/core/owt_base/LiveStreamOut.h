// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef LiveStreamOut_h
#define LiveStreamOut_h

#include <string>

#include <logger.h>

#include "AVStreamOut.h"

namespace owt_base {

class LiveStreamOut : public AVStreamOut {
    DECLARE_LOGGER();

public:
    LiveStreamOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle, int streamingTimeout);
    ~LiveStreamOut();

protected:
    bool isAudioFormatSupported(FrameFormat format) override;
    bool isVideoFormatSupported(FrameFormat format) override;
    const char *getFormatName(std::string& url) override;
    bool getHeaderOpt(std::string& url, AVDictionary **options) override;

    uint32_t getKeyFrameInterval(void) override {return 2000;}
    uint32_t getReconnectCount(void) override {return 1;}
};

}

#endif // LiveStreamOut_h
