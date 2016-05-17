/*
 * Copyright 2015 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#ifndef RtspIn_h
#define RtspIn_h

#include <boost/thread.hpp>
#include <EventRegistry.h>
#include <logger.h>
#include <MediaDefinitions.h>
#include <string>
#include "MediaFramePipeline.h"
#include "VideoHelper.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}

namespace woogeen_base {

static inline int64_t currentTimeMillis()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

class TimeoutHandler {
public:
    TimeoutHandler(int32_t timeout) : m_timeout(timeout), m_lastTime(currentTimeMillis()) { }

    void reset(int32_t timeout)
    {
        m_timeout = timeout;
        m_lastTime = currentTimeMillis();
    }

    static int checkInterrupt(void* handler)
    {
        return handler && static_cast<TimeoutHandler *>(handler)->isTimeout();
    }

private:
    bool isTimeout()
    {
        int32_t delay = currentTimeMillis() - m_lastTime;
        return delay > m_timeout;
    }

    int32_t m_timeout;
    int64_t m_lastTime;
};

class RtspIn : public FrameSource {
    DECLARE_LOGGER();
public:
    struct Options {
        std::string url;
        std::string transport;
        uint32_t bufferSize;
        bool enableAudio;
        bool enableVideo;
        bool enableH264;
        Options() : url{""}, transport{"udp"}, bufferSize{2*1024*1024}, enableAudio{false}, enableVideo{false}, enableH264{false} { }
    };

    RtspIn (const Options&, EventRegistry*);
    RtspIn (const std::string& url, const std::string& transport, uint32_t bufferSize, bool enableAudio, bool enableVideo, EventRegistry* handle);
    virtual ~RtspIn();

    void setEventRegistry(EventRegistry* handle) { m_asyncHandle = handle; }

private:
    std::string m_url;
    bool m_needAudio;
    bool m_needVideo;
    AVDictionary* m_transportOpts;
    bool m_enableH264;
    bool m_running;
    boost::thread m_thread;
    AVFormatContext* m_context;
    TimeoutHandler* m_timeoutHandler;
    AVPacket m_avPacket;
    int m_videoStreamIndex;
    FrameFormat m_videoFormat;
    VideoSize m_videoSize;
    int m_audioStreamIndex;
    FrameFormat m_audioFormat;
    EventRegistry* m_asyncHandle;

    bool connect();
    void receiveLoop();
};

}
#endif /* RtspIn_h */
