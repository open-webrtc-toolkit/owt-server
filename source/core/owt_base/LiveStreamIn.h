// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef LiveStreamIn_h
#define LiveStreamIn_h

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>
#include <EventRegistry.h>
#include <logger.h>
#include <string>
#include "MediaFramePipeline.h"

extern "C" {
#include <libavformat/avformat.h>
#include<libavutil/intreadwrite.h>
}

#include <fstream>
#include <memory>

namespace owt_base {

class JitterBuffer;

static inline int64_t currentTimeMillis()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

class TimeoutHandler {
public:
    TimeoutHandler(int32_t timeout = 100000) : m_valid(true), m_timeout(timeout), m_lastTime(currentTimeMillis()) { }

    void reset(int32_t timeout)
    {
        m_timeout = timeout;
        m_lastTime = currentTimeMillis();
    }

    void stop()
    {
        m_valid = false;
    }

    static int checkInterrupt(void* handler)
    {
        return handler && static_cast<TimeoutHandler *>(handler)->isTimeout();
    }

private:
    bool isTimeout()
    {
        int32_t delay = currentTimeMillis() - m_lastTime;
        return m_valid ? delay > m_timeout : true;
    }

    bool m_valid;
    int32_t m_timeout;
    int64_t m_lastTime;
};

class FramePacket {
public:
    FramePacket (AVPacket *packet);
    virtual ~FramePacket ();

    AVPacket *getAVPacket() {return m_packet;}
private:
    AVPacket *m_packet;
};

class FramePacketBuffer {
public:
    FramePacketBuffer () { }
    virtual ~FramePacketBuffer() { }

    void pushPacket(boost::shared_ptr<FramePacket> &FramePacket);
    boost::shared_ptr<FramePacket> popPacket(bool noWait = true);

    boost::shared_ptr<FramePacket> frontPacket(bool noWait = true);
    boost::shared_ptr<FramePacket> backPacket(bool noWait = true);

    uint32_t size();
    void clear();

private:
    boost::mutex m_queueMutex;
    boost::condition_variable m_queueCond;
    std::deque<boost::shared_ptr<FramePacket>> m_queue;
};

class JitterBufferListener {
public:
    virtual void onDeliverFrame(JitterBuffer *jitterBuffer, AVPacket *pkt) = 0;
    virtual void onSyncTimeChanged(JitterBuffer *jitterBuffer, int64_t syncTimestamp) = 0;
};

class JitterBuffer {
    DECLARE_LOGGER();
public:
    enum SyncMode {
        SYNC_MODE_MASTER,
        SYNC_MODE_SLAVE,
    };

    JitterBuffer (std::string name, SyncMode syncMode, JitterBufferListener *listener, int64_t maxBufferingMs = 1000);
    virtual ~JitterBuffer ();

    void start(uint32_t delay = 0);
    void stop();
    void drain();
    uint32_t sizeInMs();

    void insert(AVPacket &pkt);
    void setSyncTime(int64_t &syncTimestamp, boost::posix_time::ptime &syncLocalTime);

protected:
    void onTimeout(const boost::system::error_code& ec);
    int64_t getNextTime(AVPacket *pkt);
    void handleJob();

private:
    std::string m_name;
    SyncMode m_syncMode;

    std::atomic<bool> m_isClosing;
    bool m_isRunning;
    int64_t m_lastInterval;
    std::atomic<bool> m_isFirstFramePacket;
    JitterBufferListener *m_listener;

    FramePacketBuffer m_buffer;

    boost::scoped_ptr<boost::thread> m_timingThread;
    boost::asio::io_service m_ioService;
    boost::scoped_ptr<boost::asio::deadline_timer> m_timer;

    boost::scoped_ptr<boost::posix_time::ptime> m_syncLocalTime;
    int64_t m_syncTimestamp;
    boost::scoped_ptr<boost::posix_time::ptime> m_firstLocalTime;
    int64_t m_firstTimestamp;
    boost::mutex m_syncMutex;

    int64_t m_maxBufferingMs;
};

class LiveStreamIn : public FrameSource, public JitterBufferListener {
    DECLARE_LOGGER();

    static const uint32_t DEFAULT_UDP_BUF_SIZE = 8 * 1024 * 1024;
public:
    struct Options {
        std::string url;
        std::string transport;
        uint32_t bufferSize;
        std::string enableAudio;
        std::string enableVideo;
        Options() : url{""}, transport{"tcp"}, bufferSize{DEFAULT_UDP_BUF_SIZE}, enableAudio{"no"}, enableVideo{"no"} { }
    };

    LiveStreamIn (const Options&, EventRegistry*);
    virtual ~LiveStreamIn();

    void setEventRegistry(EventRegistry* handle) { m_asyncHandle = handle; }

    void onDeliverFrame(JitterBuffer *jitterBuffer, AVPacket *pkt);
    void onSyncTimeChanged(JitterBuffer *jitterBuffer, int64_t syncTimestamp);

    void deliverNullVideoFrame();
    void deliverVideoFrame(AVPacket *pkt);
    void deliverAudioFrame(AVPacket *pkt);

    void onFeedback(const owt_base::FeedbackMsg& msg) {
        if (msg.type == owt_base::VIDEO_FEEDBACK) {
            if (msg.cmd == REQUEST_KEY_FRAME) {
                requestKeyFrame();
            }
        }
    }

private:
    std::string m_url;
    std::string m_enableAudio;
    std::string m_enableVideo;
    EventRegistry* m_asyncHandle;
    AVDictionary* m_options;
    bool m_running;
    bool m_keyFrameRequest;
    boost::thread m_thread;
    AVFormatContext* m_context;
    TimeoutHandler* m_timeoutHandler;
    AVPacket m_avPacket;

    int m_videoStreamIndex;
    FrameFormat m_videoFormat;
    uint32_t m_videoWidth;
    uint32_t m_videoHeight;
    bool m_needCheckVBS;
    bool m_needApplyVBSF;
    AVBSFContext *m_vbsf;

    int m_audioStreamIndex;
    FrameFormat m_audioFormat;
    uint32_t m_audioSampleRate;
    uint32_t m_audioChannels;

    AVRational m_msTimeBase;
    AVRational m_videoTimeBase;
    AVRational m_audioTimeBase;

    boost::shared_ptr<JitterBuffer> m_videoJitterBuffer;
    boost::shared_ptr<JitterBuffer> m_audioJitterBuffer;

    bool m_isFileInput;
    int64_t m_timstampOffset;
    int64_t m_lastTimstamp;

    bool m_enableVideoExtradata;
    boost::shared_array<uint8_t> m_sps_pps_buffer;
    int m_sps_pps_buffer_length;

    char m_errbuff[500];
    char *ff_err2str(int errRet);

    std::ostringstream m_AsyncEvent;

    bool isRtsp() {return (m_url.compare(0, 7, "rtsp://") == 0 || m_url.compare(0, 6, "srt://") == 0);}
    bool isFileInput() {return (m_url.compare(0, 7, "file://") == 0
            || m_url.compare(0, 1, "/") == 0 || m_url.compare(0, 1, ".") == 0);}

    void requestKeyFrame();

    bool connect();
    bool reconnect();
    void receiveLoop();

    void checkVideoBitstream(AVStream *st, const AVPacket *pkt);
    bool parse_avcC(AVPacket *pkt);
    bool filterVBS(AVStream *st, AVPacket *pkt);
    bool filterPS(AVStream *st, AVPacket *pkt);
};

}
#endif /* LiveStreamIn_h */
