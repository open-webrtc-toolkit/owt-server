// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MediaFileOut_h
#define MediaFileOut_h

#include "AVStreamOut.h"
#include <logger.h>
#include <string>

namespace owt_base {

class MediaFileOut : public AVStreamOut {
    DECLARE_LOGGER();

public:
    MediaFileOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle, int recordingTimeout);
    ~MediaFileOut();

    void onVideoSourceChanged() override;

protected:
    bool isAudioFormatSupported(FrameFormat format) override;
    bool isVideoFormatSupported(FrameFormat format) override;
    const char *getFormatName(std::string& url) override;
    bool getHeaderOpt(std::string& url, AVDictionary **options) override;

    uint32_t getKeyFrameInterval(void) override {return 120000;} //120s
    uint32_t getReconnectCount(void) override {return 0;}
};

} /* namespace owt_base */

#endif /* MediaFileOut_h */
