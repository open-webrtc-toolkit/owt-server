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

#ifndef ExternalOutput_h
#define ExternalOutput_h

#include <boost/thread.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <string>

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

class ExternalInputStatusListener {
public:
    virtual ~ExternalInputStatusListener() { }

    virtual void notifyStatus(const std::string& message) = 0;
};

class ExternalInput : public erizo::MediaSource {
    DECLARE_LOGGER();
public:
    DLL_PUBLIC ExternalInput (const std::string& options);
    virtual ~ExternalInput();
    DLL_PUBLIC void init();
    int sendFirPacket();
    void setStatusListener(ExternalInputStatusListener* listener);

private:
    std::string m_url;
    bool m_needAudio;
    bool m_needVideo;
    AVDictionary* m_transportOpts;
    bool m_running;
    boost::thread m_thread;
    AVFormatContext* m_context;
    TimeoutHandler* m_timeoutHandler;
    AVPacket m_avPacket;
    int m_videoStreamIndex;
    int m_audioStreamIndex;
    uint32_t m_audioSeqNumber;
    ExternalInputStatusListener* m_statusListener;

    bool connect();
    void receiveLoop();
};
}
#endif /* ExternalInput_h */
