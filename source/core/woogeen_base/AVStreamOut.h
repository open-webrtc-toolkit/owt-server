/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#ifndef AVStreamOut_h
#define AVStreamOut_h

#include <queue>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <logger.h>
#include <EventRegistry.h>
#include <rtputils.h>

#include "MediaFramePipeline.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace woogeen_base {

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

class MediaFrame {
public:
    MediaFrame(const woogeen_base::Frame& frame, int64_t timeStamp = 0)
        : m_timeStamp(timeStamp)
    {
        m_frame = frame;
        if (frame.length > 0) {
            uint8_t *payload = frame.payload;
            uint32_t length = frame.length;

            if (isAudioFrame(frame) && frame.additionalInfo.audio.isRtpPacket) {
                RTPHeader* rtp = reinterpret_cast<RTPHeader*>(payload);
                uint32_t headerLength = rtp->getHeaderLength();
                assert(length >= headerLength);
                payload += headerLength;
                length -= headerLength;
                m_frame.additionalInfo.audio.isRtpPacket = false;
                m_frame.length = length;
            }

            m_frame.payload = (uint8_t *)malloc(length);
            memcpy(m_frame.payload, payload, length);
        } else {
            m_frame.payload = NULL;
        }
    }

    ~MediaFrame()
    {
        if (m_frame.payload) {
            free(m_frame.payload);
            m_frame.payload = NULL;
        }
    }

    int64_t m_timeStamp;
    woogeen_base::Frame m_frame;
};

class MediaFrameQueue {
public:
    MediaFrameQueue()
        : m_valid(true)
        , m_startTimeOffset(currentTimeMs())
    {
    }

    virtual ~MediaFrameQueue()
    {
    }

    void pushFrame(const woogeen_base::Frame& frame)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if (!m_valid)
            return;

        boost::shared_ptr<MediaFrame> mediaFrame(new MediaFrame(frame, currentTimeMs() - m_startTimeOffset));
        m_queue.push(mediaFrame);
        if (m_queue.size() == 1)
            m_cond.notify_one();
    }

    boost::shared_ptr<MediaFrame> popFrame(int timeout = 0)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        boost::shared_ptr<MediaFrame> mediaFrame;

        if (!m_valid)
            return NULL;

        if (m_queue.size() == 0 && timeout > 0) {
            m_cond.timed_wait(lock, boost::get_system_time() + boost::posix_time::milliseconds(timeout));
        }

        if (m_queue.size() > 0) {
            mediaFrame = m_queue.front();
            m_queue.pop();
        }

        return mediaFrame;
    }

    void cancel()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        m_valid = false;
        m_cond.notify_all();
    }

private:
    std::queue<boost::shared_ptr<MediaFrame>> m_queue;
    boost::mutex m_mutex;
    boost::condition_variable m_cond;

    bool m_valid;
    int64_t m_startTimeOffset;
};

class AVStreamOut : public woogeen_base::FrameDestination, public EventRegistry {
    DECLARE_LOGGER();

public:
    enum Status {
        Context_CLOSED = -1,
        Context_EMPTY = 0,
        Context_INITIALIZING = 1,
        Context_READY = 2
    };

    AVStreamOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle, int timeout);
    virtual ~AVStreamOut();

    // FrameDestination
    virtual void onFrame(const Frame&);
    virtual void onVideoSourceChanged(void) {deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });}

protected:
    void start(void);

    virtual bool isAudioFormatSupported(FrameFormat format) = 0;
    virtual bool isVideoFormatSupported(FrameFormat format) = 0;
    virtual const char *getFormatName(std::string& url) = 0;
    virtual uint32_t getKeyFrameInterval(void) = 0;
    virtual uint32_t getReconnectCount(void) = 0;

    virtual bool writeHeader(void);

    // EventRegistry
    virtual bool notifyAsyncEvent(const std::string& event, const std::string& data)
    {
        if (m_asyncHandle) {
            return m_asyncHandle->notifyAsyncEvent(event, data);
        } else {
            return false;
        }
    }

    virtual bool notifyAsyncEventInEmergency(const std::string& event, const std::string& data)
    {
        if (m_asyncHandle) {
            return m_asyncHandle->notifyAsyncEventInEmergency(event, data);
        } else {
            return false;
        }
    }

    void close();
    bool connect(void);
    void disconnect(void);
    bool addAudioStream(FrameFormat format, uint32_t sampleRate, uint32_t channels);
    bool addVideoStream(FrameFormat format, uint32_t width, uint32_t height);

    bool writeFrame(AVStream *stream, boost::shared_ptr<MediaFrame> mediaFrame);

    void sendLoop(void);

    char *ff_err2str(int errRet);

protected:
    Status m_status;

    std::string m_url;
    bool m_hasAudio;
    bool m_hasVideo;
    EventRegistry *m_asyncHandle;
    uint32_t m_timeOutMs;

    FrameFormat m_audioFormat;
    uint32_t m_sampleRate;
    uint32_t m_channels;
    FrameFormat m_videoFormat;
    uint32_t m_width;
    uint32_t m_height;

    bool m_videoSourceChanged;

    boost::shared_ptr<woogeen_base::MediaFrame> m_videoKeyFrame;
    MediaFrameQueue m_frameQueue;

    AVFormatContext *m_context;
    AVStream *m_audioStream;
    AVStream *m_videoStream;

    int64_t m_lastKeyFrameTimestamp;
    int64_t m_lastAudioDts;
    int64_t m_lastVideoDts;

    char m_errbuff[500];

    boost::thread m_thread;
};

} /* namespace woogeen_base */

#endif /* AVStreamOut_h */
