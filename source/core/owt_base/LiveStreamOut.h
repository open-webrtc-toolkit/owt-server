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
    enum StreamingFormat {
        STREAMING_FORMAT_RTSP,
        STREAMING_FORMAT_RTMP,
        STREAMING_FORMAT_HLS,
        STREAMING_FORMAT_DASH,
    };

    struct StreamingOptions {
        StreamingFormat format;

        union {
            struct {
                uint32_t    hls_time;
                uint32_t    hls_list_size;
                char        hls_method[16];
            };

            struct {
                uint32_t    dash_seg_duration;
                uint32_t    dash_window_size;
                char        dash_method[16];
            };
        };
    };

public:
    LiveStreamOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle, int streamingTimeout, StreamingOptions& options);
    ~LiveStreamOut();

protected:
    bool isAudioFormatSupported(FrameFormat format) override;
    bool isVideoFormatSupported(FrameFormat format) override;
    const char *getFormatName(std::string& url) override;
    bool getHeaderOpt(std::string& url, AVDictionary **options) override;

    uint32_t getKeyFrameInterval(void) override {return 2000;}
    uint32_t getReconnectCount(void) override {return 1;}

private:
    StreamingOptions m_options;
};

}

#endif // LiveStreamOut_h
