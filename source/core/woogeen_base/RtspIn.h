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

#ifndef RtspIn_h
#define RtspIn_h

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
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
#include <libavutil/audio_fifo.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <fstream>
#include <memory>

namespace woogeen_base {

class JitterBuffer;

static inline int64_t currentTimeMillis()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

class TimeoutHandler {
public:
    TimeoutHandler(int32_t timeout = 100000) : m_timeout(timeout), m_lastTime(currentTimeMillis()) { }

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

    JitterBuffer (std::string name, SyncMode syncMode, JitterBufferListener *listener, int64_t maxBufferingMs = 500);
    virtual ~JitterBuffer ();

    void start(uint32_t delay = 0);
    void stop();
    void drain();

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

class RtspIn : public FrameSource, public JitterBufferListener {
    DECLARE_LOGGER();

    static const uint32_t DEFAULT_UDP_BUF_SIZE = 8 * 1024 * 1024;
public:
    struct Options {
        std::string url;
        std::string transport;
        uint32_t bufferSize;
        bool enableAudio;
        bool enableVideo;
        bool enableH264;
        Options() : url{""}, transport{"tcp"}, bufferSize{DEFAULT_UDP_BUF_SIZE}, enableAudio{false}, enableVideo{false}, enableH264{false} { }
    };

    RtspIn (const Options&, EventRegistry*);
    virtual ~RtspIn();

    void setEventRegistry(EventRegistry* handle) { m_asyncHandle = handle; }

    void onDeliverFrame(JitterBuffer *jitterBuffer, AVPacket *pkt);
    void onSyncTimeChanged(JitterBuffer *jitterBuffer, int64_t syncTimestamp);

    void deliverNullVideoFrame();
    void deliverVideoFrame(AVPacket *pkt);
    void deliverAudioFrame(AVPacket *pkt);

    void onFeedback(const woogeen_base::FeedbackMsg& msg) {
        if (msg.type == woogeen_base::VIDEO_FEEDBACK) {
            if (msg.cmd == REQUEST_KEY_FRAME) {
                requestKeyFrame();
            }
        }
    }

private:
    bool m_enableTranscoding;

    std::string m_url;
    bool m_needAudio;
    bool m_needVideo;
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
    VideoSize m_videoSize;
    bool m_needCheckVBS;
    bool m_needApplyVBSF;
    AVBitStreamFilterContext *m_vbsf;

    int m_audioStreamIndex;
    FrameFormat m_audioFormat;
    bool m_needAudioTranscoder;
    AVCodecID m_audioDecId;
    AVCodecContext *m_audioDecCtx;
    AVFrame *m_audioDecFrame;
    AVCodecContext *m_audioEncCtx;
    struct SwrContext *m_audioSwrCtx;
    uint8_t **m_audioSwrSamplesData;
    int m_audioSwrSamplesLinesize;
    int m_audioSwrSamplesCount;
    AVAudioFifo* m_audioEncFifo;
    AVFrame *m_audioEncFrame;
    int64_t m_audioFifoTimeBegin;
    int64_t m_audioFifoTimeEnd;
    int64_t m_audioEncTimestamp;

    AVRational m_msTimeBase;
    AVRational m_videoTimeBase;
    AVRational m_audioTimeBase;

    uint32_t m_audioSampleRate;
    uint32_t m_audioChannels;

    boost::shared_ptr<JitterBuffer> m_videoJitterBuffer;
    boost::shared_ptr<JitterBuffer> m_audioJitterBuffer;

    std::unique_ptr<std::ofstream> m_audioRawDumpFile;
    std::unique_ptr<std::ofstream> m_audioResampleDumpFile;
    AVFormatContext* m_dumpContext;

    char m_errbuff[500];
    char *ff_err2str(int errRet);

    std::ostringstream m_AsyncEvent;

    enum AVSampleFormat getSampleFmt(AVCodec *codec, enum AVSampleFormat sample_fmt);

    bool isRtsp() {return (m_url.compare(0, 7, "rtsp://") == 0);}

    void requestKeyFrame();

    bool connect();
    bool reconnect();
    void receiveLoop();

    void checkVideoBitstream(AVStream *st, const AVPacket *pkt);
    bool filterVBS(AVStream *st, AVPacket *pkt);
    void filterSEI(AVPacket *pkt);
    int  getNextNaluPosition(uint8_t *buffer, int buffer_size, bool &is_sei);

    bool initAudioDecoder(AVCodecID codec);
    bool initAudioTranscoder(AVCodecID inCodec, AVCodecID outCodec);

    bool decAudioFrame(AVPacket &packet);
    bool encAudioFrame(AVPacket *packet);

    bool initDump(char *dumpFile);
};

}
#endif /* RtspIn_h */
