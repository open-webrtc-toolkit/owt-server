// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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

namespace owt_base {

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

class MediaFrame {
public:
    MediaFrame(const owt_base::Frame& frame, int64_t timeStamp = 0)
        : m_timeStamp(timeStamp)
        , m_duration(0)
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
    int64_t m_duration;
    owt_base::Frame m_frame;
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

    void pushFrame(const owt_base::Frame& frame)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if (!m_valid)
            return;

        boost::shared_ptr<MediaFrame> lastFrame;

        boost::shared_ptr<MediaFrame> mediaFrame(new MediaFrame(frame, currentTimeMs() - m_startTimeOffset));
        if (isAudioFrame(frame)) {
            if (!m_lastAudioFrame) {
                m_lastAudioFrame = mediaFrame;
                return;
            }

            m_lastAudioFrame->m_duration = mediaFrame->m_timeStamp - m_lastAudioFrame->m_timeStamp;
            if (m_lastAudioFrame->m_duration <= 0) {
                m_lastAudioFrame->m_duration = 1;
                mediaFrame->m_timeStamp = m_lastAudioFrame->m_timeStamp + 1;
            }

            lastFrame = m_lastAudioFrame;
            m_lastAudioFrame = mediaFrame;
        } else {
            if (!m_lastVideoFrame) {
                m_lastVideoFrame = mediaFrame;
                return;
            }

            m_lastVideoFrame->m_duration = mediaFrame->m_timeStamp - m_lastVideoFrame->m_timeStamp;
            if (m_lastVideoFrame->m_duration <= 0) {
                m_lastVideoFrame->m_duration = 1;
                mediaFrame->m_timeStamp = m_lastVideoFrame->m_timeStamp + 1;
            }

            lastFrame = m_lastVideoFrame;
            m_lastVideoFrame = mediaFrame;
        }

        m_queue.push(lastFrame);
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

    boost::shared_ptr<MediaFrame> m_lastAudioFrame;
    boost::shared_ptr<MediaFrame> m_lastVideoFrame;

    bool m_valid;
    int64_t m_startTimeOffset;
};

class AVStreamOut : public owt_base::FrameDestination, public EventRegistry {
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
    virtual bool isAudioFormatSupported(FrameFormat format) = 0;
    virtual bool isVideoFormatSupported(FrameFormat format) = 0;
    virtual const char *getFormatName(std::string& url) = 0;
    virtual uint32_t getKeyFrameInterval(void) = 0;
    virtual uint32_t getReconnectCount(void) = 0;

    virtual bool writeHeader(void);
    virtual bool getHeaderOpt(std::string& url, AVDictionary **options) = 0;

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

    void setVideoSourceChanged() {m_videoSourceChanged = true;};

    char *ff_err2str(int errRet);

private:
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

    boost::shared_ptr<owt_base::MediaFrame> m_videoKeyFrame;
    MediaFrameQueue m_frameQueue;

    AVFormatContext *m_context;
    AVStream *m_audioStream;
    AVStream *m_videoStream;

    int64_t m_lastKeyFrameTimestamp;

    char m_errbuff[500];

    boost::thread m_thread;
};

} /* namespace owt_base */

#endif /* AVStreamOut_h */
