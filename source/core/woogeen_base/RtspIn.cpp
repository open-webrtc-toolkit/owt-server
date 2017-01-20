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

#include "RtspIn.h"

#include <cstdio>
#include <rtputils.h>
#include <sstream>
#include <sys/time.h>

//#define DUMP_AUDIO

using namespace erizo;

static inline int64_t timeRescale(uint32_t time, AVRational in, AVRational out)
{
    return av_rescale_q(time, in, out);
}

static inline void notifyAsyncEvent(EventRegistry* handle, const std::string& event, const std::string& data)
{
    if (handle)
        handle->notifyAsyncEvent(event, data);
}

namespace woogeen_base {

FramePacket::FramePacket (AVPacket *packet)
    : m_packet(NULL)
{
    m_packet = (AVPacket *)malloc(sizeof(AVPacket));

    av_init_packet(m_packet);
    av_packet_ref(m_packet, packet);
}

FramePacket::~FramePacket()
{
    if (m_packet) {
        av_packet_unref(m_packet);
        free(m_packet);
        m_packet = NULL;
    }
}

void FramePacketBuffer::pushPacket(boost::shared_ptr<FramePacket> &FramePacket)
{
    boost::mutex::scoped_lock lock(m_queueMutex);
    m_queue.push_back(FramePacket);

    if (m_queue.size() == 1)
        m_queueCond.notify_one();
}

boost::shared_ptr<FramePacket> FramePacketBuffer::popPacket(bool noWait)
{
    boost::mutex::scoped_lock lock(m_queueMutex);
    boost::shared_ptr<FramePacket> packet;

    while (!noWait && m_queue.empty()) {
        m_queueCond.wait(lock);
    }

    if (!m_queue.empty()) {
        packet = m_queue.front();
        m_queue.pop_front();
    }

    return packet;
}

boost::shared_ptr<FramePacket> FramePacketBuffer::frontPacket(bool noWait)
{
    boost::mutex::scoped_lock lock(m_queueMutex);
    boost::shared_ptr<FramePacket> packet;

    while (!noWait && m_queue.empty()) {
        m_queueCond.wait(lock);
    }

    if (!m_queue.empty()) {
        packet = m_queue.front();
    }

    return packet;
}

uint32_t FramePacketBuffer::size()
{
    boost::mutex::scoped_lock lock(m_queueMutex);
    return m_queue.size();
}

void FramePacketBuffer::clear()
{
    boost::mutex::scoped_lock lock(m_queueMutex);
    m_queue.clear();
    return;
}

DEFINE_LOGGER(JitterBuffer, "woogeen.RtspIn.JitterBuffer");

JitterBuffer::JitterBuffer(std::string name, SyncMode syncMode, JitterBufferListener *listener)
    : m_name(name)
    , m_syncMode(syncMode)
    , m_isClosing(false)
    , m_isRunning(false)
    , m_lastInterval(5)
    , m_isFirstFramePacket(true)
    , m_listener(listener)
    , m_syncTimestamp(AV_NOPTS_VALUE)
    , m_firstTimestamp(AV_NOPTS_VALUE)
{
}

JitterBuffer::~JitterBuffer()
{
    stop();
}

void JitterBuffer::start(uint32_t delay)
{
    if (!m_isRunning) {
        ELOG_INFO_T("(%s)start", m_name.c_str());

        m_timer.reset(new boost::asio::deadline_timer(m_ioService));
        m_timer->expires_from_now(boost::posix_time::milliseconds(delay));
        m_timer->async_wait(boost::bind(&JitterBuffer::onTimeout, this, boost::asio::placeholders::error));
        m_timingThread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService)));
        m_isRunning = true;
    }
}

void JitterBuffer::stop()
{
    if (m_isRunning) {
        ELOG_INFO_T("(%s)stop", m_name.c_str());

        m_timer->cancel();

        m_isClosing = true;
        m_timingThread->join();
        m_buffer.clear();
        m_ioService.reset();
        m_isRunning = false;
        m_isClosing = false;

        m_isFirstFramePacket = true;
        m_syncTimestamp = AV_NOPTS_VALUE;
        m_syncLocalTime.reset();
        m_firstTimestamp = AV_NOPTS_VALUE;
        m_firstLocalTime.reset();
    }
}

void JitterBuffer::drain()
{
    ELOG_INFO_T("(%s)drain jitter buffer size(%d)", m_name.c_str(), m_buffer.size());

    while(m_buffer.size() > 0) {
        ELOG_DEBUG_T("(%s)drain jitter buffer, size(%d) ...", m_name.c_str(), m_buffer.size());
        usleep(10);
    }
}

void JitterBuffer::onTimeout(const boost::system::error_code& ec)
{
    if (!ec) {
        if (!m_isClosing)
            handleJob();
    }
}

void JitterBuffer::insert(AVPacket &pkt)
{
    boost::shared_ptr<FramePacket> framePacket(new FramePacket(&pkt));
    m_buffer.pushPacket(framePacket);
}

void JitterBuffer::setSyncTime(int64_t &syncTimestamp, boost::posix_time::ptime &syncLocalTime)
{
    boost::mutex::scoped_lock lock(m_syncMutex);

    ELOG_INFO_T("(%s)set sync timestamp %ld -> %ld", m_name.c_str(), m_syncTimestamp, syncTimestamp);

    m_syncTimestamp = syncTimestamp;
    m_syncLocalTime.reset(new boost::posix_time::ptime(syncLocalTime));
}

int64_t JitterBuffer::getNextTime(AVPacket *pkt)
{
    int64_t interval = m_lastInterval;
    int64_t diff = 0;
    int64_t timestamp = 0;
    int64_t nextTimestamp = 0;

    boost::shared_ptr<FramePacket> nextFramePacket = m_buffer.frontPacket();
    AVPacket *nextPkt = nextFramePacket != NULL ? nextFramePacket->getAVPacket() : NULL;

    if (!pkt || !nextPkt) {
        interval = 10;

        ELOG_DEBUG_T("(%s)no next frame, next time %ld", m_name.c_str(), interval);
        return interval;
    }

    timestamp = pkt->dts;
    nextTimestamp = nextPkt->dts;

    diff = nextTimestamp - timestamp;
    if (diff < 0 || diff > 2000) { // revised
        ELOG_INFO_T("(%s)timestamp rollback, %ld -> %ld", m_name.c_str(), timestamp, nextTimestamp);
        if (m_syncMode == SYNC_MODE_MASTER)
            m_listener->onSyncTimeChanged(this, nextTimestamp);

        ELOG_INFO_T("(%s)reset first timestamp %ld -> %ld", m_name.c_str(), m_firstTimestamp, nextTimestamp);
        m_firstTimestamp = nextTimestamp;
        m_firstLocalTime.reset(new boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time() + boost::posix_time::milliseconds(interval)));
        return interval;
    }

    {
        boost::mutex::scoped_lock lock(m_syncMutex);
        if (m_isFirstFramePacket) {
            ELOG_INFO_T("(%s)set first timestamp %ld -> %ld", m_name.c_str(), m_firstTimestamp, timestamp);
            m_firstTimestamp = timestamp;
            m_firstLocalTime.reset(new boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time()));

            if (m_syncTimestamp == AV_NOPTS_VALUE && m_syncMode == SYNC_MODE_MASTER) {
                m_syncMutex.unlock();
                m_listener->onSyncTimeChanged(this, timestamp);
                m_syncMutex.lock();
            }

            m_isFirstFramePacket = false;
        }

        boost::posix_time::ptime mst = boost::posix_time::microsec_clock::local_time();
        if (m_syncTimestamp != AV_NOPTS_VALUE) {
            // sync timestamp changed
            if (nextTimestamp < m_syncTimestamp) {
                ELOG_INFO_T("(%s)timestamp(%ld) is behind sync timestamp(%ld), diff %ld!"
                        , m_name.c_str(), nextTimestamp, m_syncTimestamp, nextTimestamp - m_syncTimestamp);
                interval = diff;
            }
            else {
                interval = (*m_syncLocalTime - mst).total_milliseconds() + (nextTimestamp - m_syncTimestamp);
            }
        }
        else {
            interval = (*m_firstLocalTime - mst).total_milliseconds() + (nextTimestamp - m_firstTimestamp);
        }

        if (m_syncMode == SYNC_MODE_MASTER) {
            if (interval < 0) {
                m_syncMutex.unlock();
                m_listener->onSyncTimeChanged(this, timestamp);
                m_syncMutex.lock();

                ELOG_DEBUG_T("(%s)force next time %ld -> %ld", m_name.c_str(), interval, m_lastInterval);
                interval = m_lastInterval;
            } else if (interval > 1000) {
                ELOG_DEBUG_T("(%s)force next time %ld -> %ld", m_name.c_str(), interval, 1000l);
                interval = 1000;
            }
        } else {
            if (interval < 0) {
                ELOG_DEBUG_T("(%s)force next time %ld -> %ld", m_name.c_str(), interval, 0l);
                interval = 0;
            } else if (interval > 1000) {
                ELOG_DEBUG_T("(%s)force next time %ld -> %ld", m_name.c_str(), interval, 1000l);
                interval = 1000;
            }
        }

        m_lastInterval = (m_lastInterval * 4.0 + interval) / 5;
        return interval;
    }
}

void JitterBuffer::handleJob()
{
    uint32_t interval;

    boost::shared_ptr<FramePacket> framePacket = m_buffer.popPacket();
    AVPacket *pkt = framePacket != NULL ? framePacket->getAVPacket() : NULL;

    interval = getNextTime(pkt);
    m_timer->expires_from_now(boost::posix_time::milliseconds(interval));

    if (pkt != NULL)
        m_listener->onDeliverFrame(this, pkt);
    else
        ELOG_DEBUG_T("(%s)no frame in JitterBuffer", m_name.c_str());

    ELOG_TRACE_T("(%s)buffer size %d, next time %d", m_name.c_str(), m_buffer.size(), interval);

    m_timer->async_wait(boost::bind(&JitterBuffer::onTimeout, this, boost::asio::placeholders::error));
}

DEFINE_LOGGER(RtspIn, "woogeen.RtspIn");

RtspIn::RtspIn(const Options& options, EventRegistry* handle)
    : m_url(options.url)
    , m_needAudio(options.enableAudio)
    , m_needVideo(options.enableVideo)
    , m_transportOpts(nullptr)
    , m_running(false)
    , m_context(nullptr)
    , m_timeoutHandler(nullptr)
    , m_videoStreamIndex(-1)
    , m_videoFormat(FRAME_FORMAT_UNKNOWN)
    , m_needVBSF(false)
    , m_vbsf(nullptr)
    , m_vbsf_buffer(nullptr)
    , m_vbsf_buffer_size(0)
    , m_audioStreamIndex(-1)
    , m_audioFormat(FRAME_FORMAT_UNKNOWN)
    , m_asyncHandle(handle)
    , m_needAudioTranscoder(false)
    , m_audioDecFrame(nullptr)
    , m_audioEncCtx(nullptr)
    , m_audioSwrCtx(nullptr)
    , m_audioSwrSamplesData(nullptr)
    , m_audioSwrSamplesLinesize(0)
    , m_audioSwrSamplesCount(0)
    , m_audioEncFifo(nullptr)
    , m_audioEncFrame(nullptr)
    , m_audioFifoTimeBegin(AV_NOPTS_VALUE)
    , m_audioFifoTimeEnd(AV_NOPTS_VALUE)
    , m_audioEncTimestamp(AV_NOPTS_VALUE)
    , m_dumpContext(nullptr)
{
    if (options.transport.compare("tcp") == 0) {
        av_dict_set(&m_transportOpts, "rtsp_transport", "tcp", 0);
        ELOG_INFO_T("url: %s, audio: %d, video: %d, transport::tcp", m_url.c_str(), m_needAudio, m_needVideo);
    } else {
        char buf[256];
        snprintf(buf, sizeof(buf), "%u", options.bufferSize);
        av_dict_set(&m_transportOpts, "buffer_size", buf, 0);
        ELOG_INFO_T("url: %s, audio: %d, video: %d, transport::%s, buffer_size: %u",
                   m_url.c_str(), m_needAudio, m_needVideo, options.transport.c_str(), options.bufferSize);
    }
    m_thread = boost::thread(&RtspIn::receiveLoop, this);
}

RtspIn::~RtspIn()
{
    ELOG_INFO_T("Closing %s" , m_url.c_str());
    m_running = false;
    m_thread.join();

    if (m_videoJitterBuffer) {
        m_videoJitterBuffer->stop();
        m_videoJitterBuffer.reset();
    }

    if (m_audioJitterBuffer) {
        m_audioJitterBuffer->stop();
        m_audioJitterBuffer.reset();
    }

    if (m_context) {
        avformat_free_context(m_context);
        m_context = NULL;
    }
    if (m_timeoutHandler) {
        delete m_timeoutHandler;
        m_timeoutHandler = nullptr;
    }
    av_dict_free(&m_transportOpts);
    if (m_vbsf) {
        av_bitstream_filter_close(m_vbsf);
        m_vbsf = NULL;
    }
    if (m_vbsf_buffer) {
        av_free(m_vbsf_buffer);
        m_vbsf_buffer = NULL;
        m_vbsf_buffer_size = 0;
    }
    if (m_audioEncFrame) {
        av_frame_free(&m_audioEncFrame);
        m_audioEncFrame = NULL;
    }
    if (m_audioEncFifo) {
        av_audio_fifo_free(m_audioEncFifo);
        m_audioEncFifo = NULL;
    }
    if (m_audioSwrCtx) {
        swr_free(&m_audioSwrCtx);
        m_audioSwrCtx = NULL;
    }
    if (m_audioSwrSamplesData) {
        av_freep(&m_audioSwrSamplesData[0]);
        av_freep(&m_audioSwrSamplesData);
        m_audioSwrSamplesData = NULL;

        m_audioSwrSamplesLinesize = 0;
    }
    m_audioSwrSamplesCount = 0;
    if (m_audioEncCtx) {
        avcodec_close(m_audioEncCtx);
        m_audioEncCtx = NULL;
    }
    if (m_audioDecFrame) {
        av_frame_free(&m_audioDecFrame);
        m_audioDecFrame = NULL;
    }

#ifdef DUMP_AUDIO
    m_audioRawDumpFile.reset();
    m_audioResampleDumpFile.reset();
#endif

    ELOG_DEBUG_T("Closed");
}

bool RtspIn::connect()
{
    int res;

    if (!m_needVideo && !m_needAudio) {
        ELOG_ERROR_T("Audio and video not enabled");

        m_AsyncEvent.str("");
        m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"audio and video not enabled\"}";
        return false;
    }

    if (ELOG_IS_TRACE_ENABLED())
        av_log_set_level(AV_LOG_DEBUG);
    else if (ELOG_IS_DEBUG_ENABLED())
        av_log_set_level(AV_LOG_DEBUG);
    else
        av_log_set_level(AV_LOG_INFO);

    srand((unsigned)time(0));

    m_timeoutHandler = new TimeoutHandler();

    m_context = avformat_alloc_context();
    m_context->interrupt_callback = {&TimeoutHandler::checkInterrupt, m_timeoutHandler};
    //m_context->max_analyze_duration = 3 * AV_TIME_BASE;

    m_timeoutHandler->reset(30000);
    ELOG_INFO_T("Opening input");
    res = avformat_open_input(&m_context, m_url.c_str(), nullptr, &m_transportOpts);
    if (res != 0) {
        ELOG_ERROR_T("Error opening input %s", ff_err2str(res));

        m_AsyncEvent.str("");
        m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"error opening input url\"}";
        return false;
    }

    m_timeoutHandler->reset(10000);
    ELOG_INFO_T("Finding stream info");
    res = avformat_find_stream_info(m_context, nullptr);
    if (res < 0) {
        ELOG_ERROR_T("Error finding stream info %s", ff_err2str(res));

        m_AsyncEvent.str("");
        m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"error finding streams info\"}";
        return false;
    }

    ELOG_INFO_T("Dump format");
    av_dump_format(m_context, 0, nullptr, 0);

    m_AsyncEvent.str("");
    m_AsyncEvent << "{\"type\":\"ready\"";

    AVStream *st, *audio_st;
    if (m_needVideo) {
        int streamNo = av_find_best_stream(m_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (streamNo < 0) {
            ELOG_WARN("No Video stream found");

            m_AsyncEvent.str("");
            m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"no video stream found\"}";
            return false;
        }
        m_videoStreamIndex = streamNo;
        st = m_context->streams[streamNo];
        ELOG_INFO_T("Has video, video stream number %d. time base = %d / %d, codec type = %s ",
                m_videoStreamIndex,
                st->time_base.num,
                st->time_base.den,
                avcodec_get_name(st->codec->codec_id));

        AVCodecID videoCodecId = st->codec->codec_id;
        switch (videoCodecId) {
            case AV_CODEC_ID_VP8:
                m_videoFormat = FRAME_FORMAT_VP8;
                m_AsyncEvent << ",\"video_codecs\":" << "[\"vp8\"]";
                break;

            case AV_CODEC_ID_H264:
                m_needVBSF = true;
                if (!initVBSFilter(videoCodecId)) {
                    ELOG_ERROR_T("Can not init video bitstream filter");

                    m_AsyncEvent.str("");
                    m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"can not init h264\"}";
                    return false;
                }
                m_videoFormat = FRAME_FORMAT_H264;
                m_AsyncEvent << ",\"video_codecs\":" << "[\"h264\"]";
                break;

            case AV_CODEC_ID_H265:
                m_needVBSF = true;
                if (!initVBSFilter(videoCodecId)) {
                    ELOG_ERROR_T("Can not init video bitstream filter");

                    m_AsyncEvent.str("");
                    m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"can not init h265\"}";
                    return false;
                }
                m_videoFormat = FRAME_FORMAT_H265;
                m_AsyncEvent << ",\"video_codecs\":" << "[\"h265\"]";
                break;

            default:
                ELOG_WARN("Video codec %s is not supported", avcodec_get_name(videoCodecId));

                m_AsyncEvent.str("");
                m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"video codec is not supported\"}";
                return false;
        }

        //FIXME: the resolution info should be retrieved from the rtsp video source.
        std::string resolution = "hd1080p";
        m_AsyncEvent << ",\"video_resolution\":" << "\"hd1080p\"";
        VideoResolutionHelper::getVideoSize(resolution, m_videoSize);

        if (!isRtsp())
            m_videoJitterBuffer.reset(new JitterBuffer("video", JitterBuffer::SYNC_MODE_SLAVE, this));

        m_videoTimeBase.num = 1;
        m_videoTimeBase.den = 90000;
    }

    if (m_needAudio) {
        int audioStreamNo = av_find_best_stream(m_context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audioStreamNo < 0) {
            ELOG_WARN("No Audio stream found");

            m_AsyncEvent.str("");
            m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"no audio stream found\"}";
            return false;
        }
        m_audioStreamIndex = audioStreamNo;
        audio_st = m_context->streams[m_audioStreamIndex];
        ELOG_INFO_T("Has audio, audio stream number %d. time base = %d / %d, codec type = %s ",
                m_audioStreamIndex,
                audio_st->time_base.num,
                audio_st->time_base.den,
                avcodec_get_name(audio_st->codec->codec_id));

        AVCodecID audioCodecId = audio_st->codec->codec_id;
        switch(audioCodecId) {
            case AV_CODEC_ID_PCM_MULAW:
                m_audioFormat = FRAME_FORMAT_PCMU;
                m_AsyncEvent << ",\"audio_codecs\":" << "[\"pcmu\"]";
                break;

            case AV_CODEC_ID_PCM_ALAW:
                m_audioFormat = FRAME_FORMAT_PCMA;
                m_AsyncEvent << ",\"audio_codecs\":" << "[\"pcma\"]";
                break;

            case AV_CODEC_ID_OPUS:
                m_audioFormat = FRAME_FORMAT_OPUS;
                m_AsyncEvent << ",\"audio_codecs\":" << "[\"opus_48000_2\"]";
                break;

            case AV_CODEC_ID_AAC:
                m_needAudioTranscoder = true;
                if (!initAudioTranscoder(AV_CODEC_ID_AAC, AV_CODEC_ID_OPUS)) {
                    ELOG_ERROR_T("Can not init audio codec");

                    m_AsyncEvent.str("");
                    m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"can not init audio\"}";
                    return false;
                }
                m_audioFormat = FRAME_FORMAT_OPUS;
                m_AsyncEvent << ",\"audio_codecs\":" << "[\"opus_48000_2\"]";
                break;

            default:
                ELOG_WARN("Audio codec %s is not supported ", avcodec_get_name(audioCodecId));

                m_AsyncEvent.str("");
                m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"audio codec is not supported\"}";
                return false;
        }

        if (!isRtsp())
            m_audioJitterBuffer.reset(new JitterBuffer("audio", JitterBuffer::SYNC_MODE_MASTER, this));

        if (m_audioFormat == FRAME_FORMAT_OPUS) {
            m_audioTimeBase.num = 1;
            m_audioTimeBase.den = 48000;
        }
        else {
            m_audioTimeBase.num = 1;
            m_audioTimeBase.den = 8000;
        }
    }

    m_msTimeBase.num = 1;
    m_msTimeBase.den = 1000;

    m_AsyncEvent << "}";

    av_read_play(m_context);

    if (m_videoJitterBuffer) {
        m_videoJitterBuffer->start();
    }
    if (m_audioJitterBuffer) {
        m_audioJitterBuffer->start();
    }

    return true;
}

bool RtspIn::reconnect()
{
    int res;

    ELOG_WARN("Read input data failed, trying to reopen input from url %s", m_url.c_str());

    if (m_videoJitterBuffer) {
        m_videoJitterBuffer->drain();
        m_videoJitterBuffer->stop();
    }
    if (m_audioJitterBuffer) {
        m_audioJitterBuffer->drain();
        m_audioJitterBuffer->stop();
    }

    m_timeoutHandler->reset(10000);
    avformat_close_input(&m_context);
    avformat_free_context(m_context);
    m_context = NULL;

    m_context = avformat_alloc_context();
    m_context->interrupt_callback = {&TimeoutHandler::checkInterrupt, m_timeoutHandler};
    //m_context->max_analyze_duration = 3 * AV_TIME_BASE;

    m_timeoutHandler->reset(60000);
    ELOG_INFO_T("Opening input");
    res = avformat_open_input(&m_context, m_url.c_str(), nullptr, &m_transportOpts);
    if (res != 0) {
        ELOG_ERROR_T("Error opening input %s", ff_err2str(res));
        return false;
    }

    m_timeoutHandler->reset(10000);
    ELOG_INFO_T("Finding stream info");
    res = avformat_find_stream_info(m_context, nullptr);
    if (res < 0) {
        ELOG_ERROR_T("Error find stream info %s", ff_err2str(res));
        return false;
    }

    ELOG_INFO_T("Dump format");
    av_dump_format(m_context, 0, nullptr, 0);

    if (m_needVideo) {
        int streamNo = av_find_best_stream(m_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (streamNo < 0) {
            ELOG_ERROR_T("No Video stream found");
            return false;
        }

        if (m_videoStreamIndex != streamNo) {
            ELOG_ERROR_T("Video stream index changed, %d -> %d", m_videoStreamIndex, streamNo);
            m_videoStreamIndex = streamNo;
        }
    }

    if (m_needAudio) {
        int streamNo = av_find_best_stream(m_context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (streamNo < 0) {
            ELOG_ERROR_T("No Audio stream found");
            return false;
        }

        if (m_audioStreamIndex != streamNo) {
            ELOG_ERROR_T("Audio stream index changed, %d -> %d", m_audioStreamIndex, streamNo);
            m_audioStreamIndex = streamNo;
        }

        // reinitilize audio transcoder
        if (m_needAudioTranscoder) {
            m_audioEncTimestamp = AV_NOPTS_VALUE;
            if (!initAudioDecoder(AV_CODEC_ID_AAC))
                return false;
        }
    }

    av_read_play(m_context);

    if (m_videoJitterBuffer)
        m_videoJitterBuffer->start();
    if (m_audioJitterBuffer)
        m_audioJitterBuffer->start();

    return true;
}

void RtspIn::receiveLoop()
{
    int ret = connect();
    if (!ret) {
        ELOG_ERROR_T("Connect failed, %s", m_AsyncEvent.str().c_str());

        ::notifyAsyncEvent(m_asyncHandle, "status", m_AsyncEvent.str());
        return;
    }
    ELOG_DEBUG_T("%s", m_AsyncEvent.str().c_str());
    ::notifyAsyncEvent(m_asyncHandle, "status", m_AsyncEvent.str().c_str());

    memset(&m_avPacket, 0, sizeof(m_avPacket));
    m_running = true;
    ELOG_DEBUG_T("Start playing %s", m_url.c_str() );
    while (m_running) {
        av_init_packet(&m_avPacket);
        m_timeoutHandler->reset(10000);
        ret = av_read_frame(m_context, &m_avPacket);
        if (ret < 0) {
            ELOG_ERROR_T("Error read frame, %s", ff_err2str(ret));
            // Try to re-open the input - silently.
            ret = reconnect();
            if (!ret) {
                ELOG_ERROR_T("Reconnect failed");
                ::notifyAsyncEvent(m_asyncHandle, "status", "{\"type\":\"failed\",\"reason\":\"reopening input url error\"}");
                break;
            }
            continue;
        }

        if (m_avPacket.stream_index == m_videoStreamIndex) { //packet is video
            AVStream *video_st = m_context->streams[m_videoStreamIndex];
            m_avPacket.dts = timeRescale(m_avPacket.dts, video_st->time_base, m_msTimeBase);
            m_avPacket.pts = timeRescale(m_avPacket.pts, video_st->time_base, m_msTimeBase);

            ELOG_TRACE_T("Receive video frame packet, dts %ld, size %d"
                    , m_avPacket.dts, m_avPacket.size);

            if (!m_needVBSF || filterVBS(m_avPacket)) {
                if (m_needAudioTranscoder) {
                    m_avPacket.dts += m_audioEncTimestamp - m_audioFifoTimeBegin;
                    m_avPacket.pts += m_audioEncTimestamp - m_audioFifoTimeBegin;
                    ELOG_TRACE_T("Audio transcoder offset %ld", m_audioEncTimestamp - m_audioFifoTimeBegin);
                }

                if (m_videoJitterBuffer)
                    m_videoJitterBuffer->insert(m_avPacket);
                else
                    deliverVideoFrame(&m_avPacket);
            }
        } else if (m_avPacket.stream_index == m_audioStreamIndex) { //packet is audio
            AVStream *audio_st = m_context->streams[m_audioStreamIndex];
            m_avPacket.dts = timeRescale(m_avPacket.dts, audio_st->time_base, m_msTimeBase);
            m_avPacket.pts = timeRescale(m_avPacket.pts, audio_st->time_base, m_msTimeBase);

            ELOG_TRACE_T("Receive audio frame packet, dts %ld, duration %ld, size %d"
                    , m_avPacket.dts, m_avPacket.duration, m_avPacket.size);

            if (!m_needAudioTranscoder) {
                if (m_audioJitterBuffer)
                    m_audioJitterBuffer->insert(m_avPacket);
                else
                    deliverAudioFrame(&m_avPacket);
            } else if(decAudioFrame(m_avPacket)) {
                AVPacket audioPacket;
                memset(&audioPacket, 0, sizeof(audioPacket));

                while(true) {
                    av_init_packet(&audioPacket);
                    if(!encAudioFrame(&audioPacket))
                        break;
                    if (m_audioJitterBuffer)
                        m_audioJitterBuffer->insert(audioPacket);
                    else
                        deliverAudioFrame(&audioPacket);
                    av_packet_unref(&audioPacket);
                }
            }
        }
        av_packet_unref(&m_avPacket);
    }
    m_running = false;

    ELOG_DEBUG_T("Thread exited!");
}

bool RtspIn::initVBSFilter(AVCodecID codec) {
    if (codec == AV_CODEC_ID_H264) {
        m_vbsf = av_bitstream_filter_init("h264_mp4toannexb");
    } else if (codec == AV_CODEC_ID_H265) {
        m_vbsf = av_bitstream_filter_init("hevc_mp4toannexb");
    }
    if (!m_vbsf) {
        ELOG_ERROR_T("Could not init bitstream filter");
        return false;
    }

    ELOG_TRACE_T("Init vidoe bitstream filter OK");
    return true;
}

bool RtspIn::filterVBS(AVPacket &packet) {
    int ret;

    if (!m_vbsf)
        return false;

    ret = av_apply_bitstream_filters(m_context->streams[m_videoStreamIndex]->codec, &packet, m_vbsf);
    if(ret < 0) {
        ELOG_WARN("Fail to filter video bitstream, %s", ff_err2str(ret));
        return false;
    }

    return true;
}

bool RtspIn::initAudioDecoder(AVCodecID codec) {
    AVStream *audio_st = m_context->streams[m_audioStreamIndex];
    AVCodec *audioDec = NULL;
    int ret;

    if (codec != AV_CODEC_ID_AAC) {
        ELOG_ERROR_T("Decoder %s is not supported, AAC only", avcodec_get_name(codec));
        return false;
    }

    audioDec = avcodec_find_decoder_by_name("libfdk_aac");
    if (!audioDec) {
        ELOG_ERROR_T("Could not find audio decoder %s, please check if ffmpeg/libfdk_aac installed", "libfdk_aac");
        return false;
    }

    audio_st->codec->sample_fmt = AV_SAMPLE_FMT_S16;

    ret = avcodec_open2(audio_st->codec, audioDec , NULL);
    if (ret < 0) {
        ELOG_ERROR_T("Could not open audio decoder context, %s", ff_err2str(ret));
        return false;
    }

    ELOG_INFO_T("Audio dec sample_rate %d, channels %d, frame_size %d"
            , audio_st->codec->sample_rate
            , audio_st->codec->channels
            , audio_st->codec->frame_size
            );

    return true;
}

bool RtspIn::initAudioTranscoder(AVCodecID inCodec, AVCodecID outCodec) {
    int ret;
    AVStream *audio_st = m_context->streams[m_audioStreamIndex];
    AVCodec *audioEnc = NULL;

    if (inCodec != AV_CODEC_ID_AAC) {
        ELOG_ERROR_T("Decoder %s is not supported, AAC only", avcodec_get_name(inCodec));
        return false;
    }

    if (outCodec != AV_CODEC_ID_OPUS) {
        ELOG_ERROR_T("Encoder %s is not supported, OPUS only", avcodec_get_name(outCodec));
        return false;
    }

    avcodec_register_all();

    if (!initAudioDecoder(inCodec))
        goto failed;

    m_audioDecFrame = av_frame_alloc();
    if (!m_audioDecFrame) {
        ELOG_ERROR_T("Could not allocate audio dec frame");
        goto failed;
    }

    audioEnc = avcodec_find_encoder(AV_CODEC_ID_OPUS);
    if (!audioEnc) {
        ELOG_ERROR_T("Could not find audio encoder %s", avcodec_get_name(AV_CODEC_ID_OPUS));
        goto failed;
    }

    m_audioEncCtx = avcodec_alloc_context3(audioEnc);
    if (!m_audioEncCtx ) {
        ELOG_ERROR_T("Could not alloc audio encoder context");
        goto failed;
    }

    m_audioEncCtx->channels         = 2;
    m_audioEncCtx->channel_layout   = av_get_default_channel_layout(2);
    m_audioEncCtx->sample_rate      = 48000;
    m_audioEncCtx->sample_fmt       = audio_st->codec->sample_fmt;
    m_audioEncCtx->bit_rate         = 64000;

    /* open it */
    ret = avcodec_open2(m_audioEncCtx, audioEnc, NULL);
    if (ret < 0) {
        ELOG_ERROR_T("Could not open audio encoder context, %s", ff_err2str(ret));
        goto failed;
    }

    ELOG_INFO_T("Audio enc sample_rate %d, channels %d, frame_size %d"
            , m_audioEncCtx->sample_rate
            , m_audioEncCtx->channels
            , m_audioEncCtx->frame_size
            );

    if (audio_st->codec->sample_rate != m_audioEncCtx->sample_rate || audio_st->codec->channels != m_audioEncCtx->channels) {
        ELOG_TRACE_T("Init audio resampler");

        m_audioSwrCtx = swr_alloc();
        if (!m_audioSwrCtx) {
            ELOG_ERROR_T("Could not allocate resampler context");
            goto failed;
        }

        /* set options */
        av_opt_set_int       (m_audioSwrCtx, "in_channel_count",   audio_st->codec->channels,       0);
        av_opt_set_int       (m_audioSwrCtx, "in_sample_rate",     audio_st->codec->sample_rate,    0);
        av_opt_set_sample_fmt(m_audioSwrCtx, "in_sample_fmt",      audio_st->codec->sample_fmt,     0);
        av_opt_set_int       (m_audioSwrCtx, "out_channel_count",  m_audioEncCtx->channels,         0);
        av_opt_set_int       (m_audioSwrCtx, "out_sample_rate",    m_audioEncCtx->sample_rate,      0);
        av_opt_set_sample_fmt(m_audioSwrCtx, "out_sample_fmt",     m_audioEncCtx->sample_fmt,       0);

        ret = swr_init(m_audioSwrCtx);
        if (ret < 0) {
            ELOG_ERROR_T("Failed to initialize the resampling context, %s", ff_err2str(ret));
            goto failed;
        }

        m_audioSwrSamplesCount = av_rescale_rnd(
                audio_st->codec->frame_size
                , m_audioEncCtx->sample_rate
                , audio_st->codec->sample_rate
                , AV_ROUND_UP);

        ret = av_samples_alloc_array_and_samples(&m_audioSwrSamplesData, &m_audioSwrSamplesLinesize, m_audioEncCtx->channels,
                m_audioSwrSamplesCount, m_audioEncCtx->sample_fmt, 0);
        if (ret < 0) {
            ELOG_ERROR_T("Could not allocate swr samples data, %s", ff_err2str(ret));
            goto failed;
        }
    }

    m_audioEncFifo = av_audio_fifo_alloc(m_audioEncCtx->sample_fmt, m_audioEncCtx->channels, 1);
    if (!m_audioEncFifo) {
        ELOG_ERROR_T("Could not allocate audio enc fifo");
        goto failed;
    }

    m_audioEncFrame  = av_frame_alloc();
    if (!m_audioEncFrame) {
        ELOG_ERROR_T("Could not allocate audio enc frame");
        goto failed;
    }

    m_audioEncFrame->nb_samples        = m_audioEncCtx->frame_size;
    m_audioEncFrame->format            = m_audioEncCtx->sample_fmt;
    m_audioEncFrame->channel_layout    = m_audioEncCtx->channel_layout;
    m_audioEncFrame->sample_rate       = m_audioEncCtx->sample_rate;

    ret = av_frame_get_buffer(m_audioEncFrame, 0);
    if (ret < 0) {
        ELOG_ERROR_T("Could not get audio frame buffer, %s", ff_err2str(ret));
        goto failed;
    }

#ifdef DUMP_AUDIO
    char dumpFile[128];

    snprintf(dumpFile, 128, "/tmp/audio-%s-s16-%d-%d.pcm", "raw", audio_st->codec->sample_rate, audio_st->codec->channels);
    m_audioRawDumpFile.reset(new std::ofstream(dumpFile, std::ios::binary));

    snprintf(dumpFile, 128, "/tmp/audio-%s-s16-%d-%d.pcm", "resample", 48000, 2);
    m_audioResampleDumpFile.reset(new std::ofstream(dumpFile, std::ios::binary));

    snprintf(dumpFile, 128, "/tmp/audio-%s-%d-%d.opus", "opus", 48000, 2);
    if(!initDump(dumpFile)) {
        ELOG_ERROR_T("Could not init dump");
        goto failed;
    }
#endif

    ELOG_TRACE_T("Init audio transcoder OK");
    return true;

failed:
    if (m_audioEncFrame) {
        av_frame_free(&m_audioEncFrame);
        m_audioEncFrame = NULL;
    }

    if (m_audioEncFifo) {
        av_audio_fifo_free(m_audioEncFifo);
        m_audioEncFifo = NULL;
    }

    if (m_audioSwrCtx) {
        swr_free(&m_audioSwrCtx);
        m_audioSwrCtx = NULL;
    }

    if (m_audioSwrSamplesData) {
        av_freep(&m_audioSwrSamplesData[0]);
        av_freep(&m_audioSwrSamplesData);
        m_audioSwrSamplesData = NULL;

        m_audioSwrSamplesLinesize = 0;
    }
    m_audioSwrSamplesCount = 0;

    if (m_audioEncCtx) {
        avcodec_close(m_audioEncCtx);
        m_audioEncCtx = NULL;
    }

    if (m_audioDecFrame) {
        av_frame_free(&m_audioDecFrame);
        m_audioDecFrame = NULL;
    }

    return false;
}

bool RtspIn::decAudioFrame(AVPacket &packet) {
    AVStream *audio_st = m_context->streams[m_audioStreamIndex];
    int got;
    int ret;
    int nSamples;
    uint8_t **audioFrameData;
    int audioFrameLen = 0;

    if (!m_audioEncCtx) {
        ELOG_ERROR_T("Invalid transcode params");
        return false;
    }

    ret = avcodec_decode_audio4(audio_st->codec, m_audioDecFrame, &got, &packet);
    if (ret < 0) {
        ELOG_ERROR_T("Error while decoding, %s", ff_err2str(ret));
        return false;
    }

    if (!got) {
        ELOG_TRACE_T("No decoded frame output");
        return false;
    }

    audioFrameData = m_audioDecFrame->data;
    audioFrameLen = m_audioDecFrame->nb_samples;

#ifdef DUMP_AUDIO
    m_audioRawDumpFile->write((const char*)m_audioDecFrame->data[0], m_audioDecFrame->nb_samples * audio_st->codec->channels * 2);
#endif

    if (m_audioSwrCtx) {
        int dst_nb_samples;

        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(
                swr_get_delay(m_audioSwrCtx, audio_st->codec->sample_rate) + audio_st->codec->frame_size
                , m_audioEncCtx->sample_rate
                , audio_st->codec->sample_rate
                , AV_ROUND_UP);

        if (dst_nb_samples > m_audioSwrSamplesCount) {
            ELOG_TRACE_T("Realloc audio swr samples buffer");

            av_freep(&m_audioSwrSamplesData[0]);
            ret = av_samples_alloc(m_audioSwrSamplesData, &m_audioSwrSamplesLinesize, m_audioEncCtx->channels,
                dst_nb_samples, m_audioEncCtx->sample_fmt, 1);
            if (ret < 0) {
                ELOG_ERROR_T("Fail to realloc swr samples, %s", ff_err2str(ret));
                return false;
            }
            m_audioSwrSamplesCount = dst_nb_samples;
        }

        /* convert to destination format */
        ret = swr_convert(m_audioSwrCtx, m_audioSwrSamplesData, dst_nb_samples, (const uint8_t **)m_audioDecFrame->data, m_audioDecFrame->nb_samples);
        if (ret < 0) {
            ELOG_ERROR_T("Error while converting, %s", ff_err2str(ret));
            return false;
        }

#ifdef DUMP_AUDIO
        int bufsize = av_samples_get_buffer_size(&m_audioSwrSamplesLinesize, m_audioEncCtx->channels,
                ret, m_audioEncCtx->sample_fmt, 1);

        m_audioResampleDumpFile->write((const char*)m_audioSwrSamplesData[0], bufsize);
#endif

        audioFrameData = m_audioSwrSamplesData;
        audioFrameLen = ret;
    }

    nSamples = av_audio_fifo_write(m_audioEncFifo, reinterpret_cast<void**>(audioFrameData), audioFrameLen);
    if (nSamples < audioFrameLen) {
        ELOG_ERROR_T("Can not write audio enc fifo, nbSamples %d, writed %d", audioFrameLen, nSamples);
        return false;
    }

    if (m_audioEncTimestamp == AV_NOPTS_VALUE) {
        m_audioEncTimestamp = packet.dts;
        m_audioFifoTimeBegin = packet.dts;
    }
    m_audioFifoTimeEnd = packet.dts + packet.duration;

    return true;
}

bool RtspIn::encAudioFrame(AVPacket *packet) {
    int ret;
    int got;
    int nSamples;

    if (!m_audioEncCtx) {
        ELOG_ERROR_T("Invalid transcode params");
        return false;
    }

    if (av_audio_fifo_size(m_audioEncFifo) < m_audioEncCtx->frame_size) {
        return false;
    }

    nSamples = av_audio_fifo_read(m_audioEncFifo, reinterpret_cast<void**>(m_audioEncFrame->data), m_audioEncCtx->frame_size);
    if (nSamples < m_audioEncCtx->frame_size) {
        ELOG_ERROR_T("Can not read audio enc fifo, nbSamples %d, writed %d", m_audioEncCtx->frame_size, nSamples);
        return false;
    }

    ret = avcodec_encode_audio2(m_audioEncCtx, packet, m_audioEncFrame, &got);
    if (ret < 0) {
        ELOG_ERROR_T("Fail to encode audio frame, %s", ff_err2str(ret));
        return false;
    }

    if (!got) {
        ELOG_TRACE_T("Not get encoded audio frame");
        return false;
    }

    packet->dts = m_audioEncTimestamp;

    m_audioEncTimestamp += 1000.0 * m_audioEncCtx->frame_size / m_audioEncCtx->sample_rate;
    m_audioFifoTimeBegin += (double)(m_audioFifoTimeEnd - m_audioFifoTimeBegin) * m_audioEncCtx->frame_size / (av_audio_fifo_size(m_audioEncFifo) + m_audioEncCtx->frame_size);

    if (av_audio_fifo_size(m_audioEncFifo) >= m_audioEncCtx->frame_size) {
        ELOG_TRACE_T("More data in fifo to encode %d >= %d", av_audio_fifo_size(m_audioEncFifo), m_audioEncCtx->frame_size);
    }

#ifdef DUMP_AUDIO
    if (m_dumpContext) {
        AVPacket dump_pkt;
        av_packet_ref(&dump_pkt, packet);
        ret = av_interleaved_write_frame(m_dumpContext, &dump_pkt);
    }
#endif

    return true;
}

void RtspIn::onSyncTimeChanged(JitterBuffer *jitterBuffer, int64_t syncTimestamp)
{
    if (m_audioJitterBuffer.get() == jitterBuffer) {
        ELOG_INFO_T("onSyncTimeChanged audio, timestamp %ld ", syncTimestamp);

        //rtsp audio/video time base is different, it will lost sync after roll back
        if(!isRtsp()) {
            boost::posix_time::ptime mst = boost::posix_time::microsec_clock::local_time();

            m_audioJitterBuffer->setSyncTime(syncTimestamp, mst);
            if (m_videoJitterBuffer)
                m_videoJitterBuffer->setSyncTime(syncTimestamp, mst);
        }
    }
    else if (m_videoJitterBuffer.get() == jitterBuffer) {
        ELOG_INFO_T("onSyncTimeChanged video, timestamp %ld ", syncTimestamp);
    } else {
        ELOG_ERROR_T("Invalid JitterBuffer onSyncTimeChanged event!");
    }
}

void RtspIn::deliverVideoFrame(AVPacket *pkt)
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = m_videoFormat;
    frame.payload = reinterpret_cast<uint8_t*>(pkt->data);
    frame.length = pkt->size;
    frame.timeStamp = timeRescale(pkt->dts, m_msTimeBase, m_videoTimeBase);
    frame.additionalInfo.video.width = m_videoSize.width;
    frame.additionalInfo.video.height = m_videoSize.height;
    deliverFrame(frame);

    ELOG_DEBUG_T("deliver video frame, timestamp %ld(%ld), size %4d, %s"
            , timeRescale(frame.timeStamp, m_videoTimeBase, m_msTimeBase)
            , pkt->dts
            , frame.length
            , (pkt->flags & AV_PKT_FLAG_KEY) ? "key frame" : ""
            );
}

void RtspIn::deliverAudioFrame(AVPacket *pkt)
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = m_audioFormat;
    frame.payload = reinterpret_cast<uint8_t*>(pkt->data);
    frame.length = pkt->size;
    frame.timeStamp = timeRescale(pkt->dts, m_msTimeBase, m_audioTimeBase);
    frame.additionalInfo.audio.isRtpPacket = 0;
    frame.additionalInfo.audio.nbSamples = pkt->duration;
    frame.additionalInfo.audio.sampleRate = m_audioFormat == FRAME_FORMAT_OPUS ? 48000 : 8000;
    frame.additionalInfo.audio.channels = m_audioFormat == FRAME_FORMAT_OPUS ? 2 : 1;
    deliverFrame(frame);

    ELOG_DEBUG_T("deliver audio frame, timestamp %ld(%ld), size %4d"
            , timeRescale(frame.timeStamp, m_audioTimeBase, m_msTimeBase)
            , pkt->dts
            , frame.length);
}

void RtspIn::onDeliverFrame(JitterBuffer *jitterBuffer, AVPacket *pkt)
{
    if (m_videoJitterBuffer.get() == jitterBuffer) {
        deliverVideoFrame(pkt);
    } else if (m_audioJitterBuffer.get() == jitterBuffer) {
        deliverAudioFrame(pkt);
    } else {
        ELOG_ERROR_T("Invalid JitterBuffer onDeliver event!");
    }
}

char *RtspIn::ff_err2str(int errRet)
{
    av_strerror(errRet, (char*)(&m_errbuff), 500);
    return m_errbuff;
}

bool RtspIn::initDump(char *dumpFile) {

    avformat_alloc_output_context2(&m_dumpContext, nullptr, nullptr, dumpFile);
    if (!m_dumpContext) {
        ELOG_ERROR_T("avformat_alloc_output_context2 dump context failed");
        return false;
        //goto failed;
    }

    AVStream* dumpStream = NULL;
    dumpStream = avformat_new_stream(m_dumpContext, m_audioEncCtx->codec);
    if (!dumpStream) {
        ELOG_ERROR_T("cannot add dump stream");
        return false;
        //goto failed;
    }
#if 1
    //Copy the settings of AVCodecContext
    if (avcodec_copy_context(dumpStream->codec, m_audioEncCtx) < 0) {
        ELOG_ERROR_T("Failed to copy dump stream codec context");
        return false;
        //goto failed;
    }
#endif
    dumpStream->codec->codec_tag = 0;
    if (m_dumpContext->oformat->flags & AVFMT_GLOBALHEADER)
        dumpStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    if (avio_open(&m_dumpContext->pb, m_dumpContext->filename, AVIO_FLAG_WRITE) < 0) {
        ELOG_ERROR_T("avio_open failed, %s", m_dumpContext->filename);
        return false;
        //goto failed;
    }

    if (avformat_write_header(m_dumpContext, nullptr) < 0) {
        ELOG_ERROR_T("avformat_write_header failed");
        return false;
        //goto failed;
    }

    av_dump_format(m_dumpContext, 0, m_dumpContext->filename, 1);
    return true;
}

}
