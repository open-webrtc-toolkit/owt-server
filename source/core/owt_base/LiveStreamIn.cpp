// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "LiveStreamIn.h"

#include <cstdio>
#include <rtputils.h>
#include <sstream>
#include <sys/time.h>
#include <memory>

#include "MediaUtilities.h"

static inline int64_t timeRescale(uint32_t time, AVRational in, AVRational out)
{
    return av_rescale_q(time, in, out);
}

static inline void notifyAsyncEvent(EventRegistry* handle, const std::string& event, const std::string& data)
{
    if (handle)
        handle->notifyAsyncEvent(event, data);
}

namespace owt_base {

static int filterNALs(uint8_t *data, int size, const std::vector<int> &remove_types, const std::vector<int> &pass_types)
{
    int remove_nals_size = 0;

    int nalu_found_length = 0;
    uint8_t* buffer_start = data;
    int buffer_length = size;
    int nalu_start_offset = 0;
    int nalu_end_offset = 0;
    int sc_len = 0;
    int nalu_type;

    if (remove_types.size() > 0 && pass_types.size() > 0)
        return -1;

    while (buffer_length > 0) {
        nalu_found_length = findNALU(buffer_start, buffer_length, &nalu_start_offset, &nalu_end_offset, &sc_len);
        if (nalu_found_length < 0) {
            /* Error, should never happen */
            break;
        }

        nalu_type = buffer_start[nalu_start_offset] & 0x1F;
        if ((remove_types.size() > 0 && find(remove_types.begin(), remove_types.end(), nalu_type) != remove_types.end())
                || (pass_types.size() > 0 && find(pass_types.begin(), pass_types.end(), nalu_type) == pass_types.end())) {
            std::unique_ptr<uint8_t> buffer_delegate(buffer_start);
            memmove(buffer_delegate.get(), buffer_delegate.get() + nalu_start_offset + nalu_found_length, buffer_length - nalu_start_offset - nalu_found_length);
            buffer_delegate.release();
            buffer_length -= nalu_start_offset + nalu_found_length;

            remove_nals_size += nalu_start_offset + nalu_found_length;
            continue;
        }

        buffer_start += (nalu_start_offset + nalu_found_length);
        buffer_length -= (nalu_start_offset + nalu_found_length);
    }
    return size - remove_nals_size;
}

FramePacket::FramePacket (AVPacket *packet)
    : m_packet(NULL)
{
    m_packet = (AVPacket *)malloc(sizeof(AVPacket));
    if (m_packet) {
      av_init_packet(m_packet);
      av_packet_ref(m_packet, packet);
    }
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

boost::shared_ptr<FramePacket> FramePacketBuffer::backPacket(bool noWait)
{
    boost::mutex::scoped_lock lock(m_queueMutex);
    boost::shared_ptr<FramePacket> packet;

    while (!noWait && m_queue.empty()) {
        m_queueCond.wait(lock);
    }

    if (!m_queue.empty()) {
        packet = m_queue.back();
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

DEFINE_LOGGER(JitterBuffer, "owt.LiveStreamIn.JitterBuffer");

JitterBuffer::JitterBuffer(std::string name, SyncMode syncMode, JitterBufferListener *listener, int64_t maxBufferingMs)
    : m_name(name)
    , m_syncMode(syncMode)
    , m_isClosing(false)
    , m_isRunning(false)
    , m_lastInterval(5)
    , m_isFirstFramePacket(true)
    , m_listener(listener)
    , m_syncTimestamp(AV_NOPTS_VALUE)
    , m_firstTimestamp(AV_NOPTS_VALUE)
    , m_maxBufferingMs(maxBufferingMs)
{
}

JitterBuffer::~JitterBuffer()
{
    stop();
}

void JitterBuffer::start(uint32_t delay)
{
    if (!m_isRunning) {
        ELOG_DEBUG_T("(%s)start", m_name.c_str());

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
        ELOG_DEBUG_T("(%s)stop", m_name.c_str());

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
    ELOG_DEBUG_T("(%s)drain jitter buffer size(%d)", m_name.c_str(), m_buffer.size());

    while(m_buffer.size() > 0) {
        ELOG_DEBUG_T("(%s)drain jitter buffer, size(%d) ...", m_name.c_str(), m_buffer.size());
        usleep(10);
    }
}

uint32_t JitterBuffer::sizeInMs()
{
    int64_t bufferingMs = 0;
    boost::shared_ptr<FramePacket> frontPacket = m_buffer.frontPacket();
    boost::shared_ptr<FramePacket> backPacket = m_buffer.backPacket();

    if (frontPacket && backPacket) {
        bufferingMs = backPacket->getAVPacket()->dts - frontPacket->getAVPacket()->dts;
    }
    return bufferingMs;
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

    ELOG_DEBUG_T("(%s)set sync timestamp %ld -> %ld", m_name.c_str(), m_syncTimestamp, syncTimestamp);

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
        ELOG_DEBUG_T("(%s)timestamp rollback, %ld -> %ld", m_name.c_str(), timestamp, nextTimestamp);
        if (m_syncMode == SYNC_MODE_MASTER)
            m_listener->onSyncTimeChanged(this, nextTimestamp);

        ELOG_DEBUG_T("(%s)reset first timestamp %ld -> %ld", m_name.c_str(), m_firstTimestamp, nextTimestamp);
        m_firstTimestamp = nextTimestamp;
        m_firstLocalTime.reset(new boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time() + boost::posix_time::milliseconds(interval)));
        return interval;
    }

    {
        boost::mutex::scoped_lock lock(m_syncMutex);
        if (m_isFirstFramePacket) {
            ELOG_DEBUG_T("(%s)set first timestamp %ld -> %ld", m_name.c_str(), m_firstTimestamp, timestamp);
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
                ELOG_DEBUG_T("(%s)timestamp(%ld) is behind sync timestamp(%ld), diff %ld!"
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
    boost::shared_ptr<FramePacket> framePacket;

    // if buffering frames exceed maxBufferingMs, do seek to maxBufferingMs / 2
    boost::shared_ptr<FramePacket> frontPacket = m_buffer.frontPacket();
    boost::shared_ptr<FramePacket> backPacket = m_buffer.backPacket();
    if (frontPacket && backPacket) {
        int64_t bufferingMs = backPacket->getAVPacket()->dts - frontPacket->getAVPacket()->dts;
        if (bufferingMs > m_maxBufferingMs) {
            ELOG_DEBUG_T("(%s)Do seek, bufferingMs(%ld), maxBufferingMs(%ld), QueueSize(%d)", m_name.c_str(), bufferingMs, m_maxBufferingMs, m_buffer.size());

            int64_t seekMs = backPacket->getAVPacket()->dts - m_maxBufferingMs / 2;
            while(true) {
                framePacket = m_buffer.frontPacket();
                if (!framePacket || framePacket->getAVPacket()->dts > seekMs)
                    break;

                m_listener->onDeliverFrame(this, framePacket->getAVPacket());
                m_buffer.popPacket();
            }

            m_syncMutex.lock();
            m_firstTimestamp = seekMs;
            m_firstLocalTime.reset(new boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time()));
            m_syncMutex.unlock();

            if (m_syncMode == SYNC_MODE_MASTER) {
                m_listener->onSyncTimeChanged(this, seekMs);
            }

            ELOG_DEBUG_T("(%s)After seek, QueueSize(%d)", m_name.c_str(), m_buffer.size());
        }
    }

    // next frame
    uint32_t interval;

    framePacket = m_buffer.popPacket();
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

DEFINE_LOGGER(LiveStreamIn, "owt.LiveStreamIn");

LiveStreamIn::LiveStreamIn(const Options& options, EventRegistry* handle)
    : m_url(options.url)
    , m_enableAudio(options.enableAudio)
    , m_enableVideo(options.enableVideo)
    , m_asyncHandle(handle)
    , m_options(nullptr)
    , m_running(false)
    , m_keyFrameRequest(false)
    , m_context(nullptr)
    , m_timeoutHandler(nullptr)
    , m_videoStreamIndex(-1)
    , m_videoFormat(FRAME_FORMAT_UNKNOWN)
    , m_videoWidth(0)
    , m_videoHeight(0)
    , m_needCheckVBS(true)
    , m_needApplyVBSF(false)
    , m_vbsf(nullptr)
    , m_audioStreamIndex(-1)
    , m_audioFormat(FRAME_FORMAT_UNKNOWN)
    , m_audioSampleRate(0)
    , m_audioChannels(0)
    , m_isFileInput(false)
    , m_timstampOffset(0)
    , m_lastTimstamp(0)
    , m_enableVideoExtradata(false)
    , m_sps_pps_buffer()
    , m_sps_pps_buffer_length(0)
{
    ELOG_INFO_T("url: %s, audio: %s, video: %s, transport: %s, bufferSize: %d"
            , m_url.c_str(), m_enableAudio.c_str(), m_enableVideo.c_str(), options.transport.c_str(), options.bufferSize);

    if (!m_enableAudio.compare("no") && !m_enableVideo.compare("no")) {
        ELOG_ERROR_T("Audio/Video not enabled");

        m_AsyncEvent.str("");
        m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"Audio/Video not enabled\"}";
        return;
    }

    if (ELOG_IS_TRACE_ENABLED())
        av_log_set_level(AV_LOG_DEBUG);
    else if (ELOG_IS_DEBUG_ENABLED())
        av_log_set_level(AV_LOG_INFO);
    else if (ELOG_IS_INFO_ENABLED())
        av_log_set_level(AV_LOG_WARNING);
    else
        av_log_set_level(AV_LOG_QUIET);

    if(isRtsp()) {
        if (options.transport.compare("udp") == 0) {
            uint32_t buffer_size = options.bufferSize > 0 ? options.bufferSize : DEFAULT_UDP_BUF_SIZE;
            char buf[256];
            snprintf(buf, sizeof(buf), "%u", buffer_size);
            av_dict_set(&m_options, "buffer_size", buf, 0);

            av_dict_set(&m_options, "rtsp_transport", "udp", 0);
            ELOG_INFO_T("rtsp, transport: udp(%u)" , buffer_size);
        } else if (options.transport.compare("tcp") == 0) {
            av_dict_set(&m_options, "rtsp_transport", "tcp", 0);
            ELOG_INFO_T("rtsp, transport: tcp");
        }
    }

    if (isFileInput()) {
        m_isFileInput = true;
    }

    m_running = true;

    srand((unsigned)time(0));
    m_timeoutHandler = new TimeoutHandler();
    m_thread = boost::thread(&LiveStreamIn::receiveLoop, this);
}

LiveStreamIn::~LiveStreamIn()
{
    ELOG_INFO_T("Closing %s" , m_url.c_str());
    m_running = false;
    if (m_timeoutHandler) {
        m_timeoutHandler->stop();
    }
    m_thread.join();

    if (m_videoJitterBuffer) {
        m_videoJitterBuffer->stop();
        m_videoJitterBuffer.reset();
    }

    if (m_audioJitterBuffer) {
        m_audioJitterBuffer->stop();
        m_audioJitterBuffer.reset();
    }

    m_needCheckVBS = true;
    m_needApplyVBSF = false;
    if (m_vbsf) {
        av_bsf_free(&m_vbsf);
        m_vbsf = NULL;
    }

    m_enableVideoExtradata = false;
    if (m_sps_pps_buffer) {
        m_sps_pps_buffer.reset();
        m_sps_pps_buffer_length = 0;
    }

    if (m_context) {
        avformat_close_input(&m_context);
        avformat_free_context(m_context);
        m_context = NULL;
    }

    av_dict_free(&m_options);
    if (m_timeoutHandler) {
        delete m_timeoutHandler;
        m_timeoutHandler = nullptr;
    }

    ELOG_DEBUG_T("Closed");
}

void LiveStreamIn::requestKeyFrame()
{
    ELOG_DEBUG_T("requestKeyFrame");
    if (!m_keyFrameRequest)
        m_keyFrameRequest = true;
}

bool LiveStreamIn::connect()
{
    int res;

    m_context = avformat_alloc_context();
    m_context->interrupt_callback = {&TimeoutHandler::checkInterrupt, m_timeoutHandler};
    //m_context->max_analyze_duration = 3 * AV_TIME_BASE;

    ELOG_DEBUG_T("Opening input");
    m_timeoutHandler->reset(30000);
    res = avformat_open_input(&m_context, m_url.c_str(), nullptr, m_options != NULL ? &m_options : NULL);
    if (res != 0) {
        ELOG_ERROR_T("Error opening input %s", ff_err2str(res));

        m_AsyncEvent.str("");
        m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"error opening input url\"}";
        return false;
    }

    ELOG_DEBUG_T("Finding stream info");
    m_timeoutHandler->reset(10000);
    m_context->fps_probe_size = 0;
    m_context->max_ts_probe = 0;
    res = avformat_find_stream_info(m_context, nullptr);
    if (res < 0) {
        ELOG_ERROR_T("Error finding stream info %s", ff_err2str(res));

        m_AsyncEvent.str("");
        m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"error finding streams info\"}";
        return false;
    }

    ELOG_DEBUG_T("Dump format");
    av_dump_format(m_context, 0, m_url.c_str(), 0);

    m_AsyncEvent.str("");
    m_AsyncEvent << "{\"type\":\"ready\"";

    AVStream *video_st, *audio_st;
    if (!m_enableVideo.compare("yes") || !m_enableVideo.compare("auto")) {
        int streamNo = av_find_best_stream(m_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (streamNo >= 0) {
            video_st = m_context->streams[streamNo];
            ELOG_INFO_T("Has video, video stream number(%d), codec(%s), %s, %dx%d",
                    streamNo,
                    avcodec_get_name(video_st->codecpar->codec_id),
                    avcodec_profile_name(video_st->codecpar->codec_id, video_st->codecpar->profile),
                    video_st->codecpar->width,
                    video_st->codecpar->height
                    );

            AVCodecID videoCodecId = video_st->codecpar->codec_id;
            switch (videoCodecId) {
                case AV_CODEC_ID_VP8:
                    m_videoFormat = FRAME_FORMAT_VP8;
                    m_AsyncEvent << ",\"video\":{\"codec\":" << "\"vp8\"";
                    break;

                case AV_CODEC_ID_VP9:
                    m_videoFormat = FRAME_FORMAT_VP9;
                    m_AsyncEvent << ",\"video\":{\"codec\":" << "\"vp9\"";
                    break;

                case AV_CODEC_ID_H264:
                    m_videoFormat = FRAME_FORMAT_H264;
                    m_AsyncEvent << ",\"video\":{\"codec\":" << "\"h264\"";

                    switch (video_st->codecpar->profile) {
                        case FF_PROFILE_H264_CONSTRAINED_BASELINE:
                            m_AsyncEvent << ",\"profile\":"  << "\"CB\"";
                            break;
                        case FF_PROFILE_H264_BASELINE:
                            m_AsyncEvent << ",\"profile\":"  << "\"B\"";
                            break;
                        case FF_PROFILE_H264_MAIN:
                            m_AsyncEvent << ",\"profile\":"  << "\"M\"";
                            break;
                        case FF_PROFILE_H264_EXTENDED:
                            m_AsyncEvent << ",\"profile\":"  << "\"E\"";
                            break;
                        case FF_PROFILE_H264_HIGH:
                            m_AsyncEvent << ",\"profile\":"  << "\"H\"";
                            break;
                        default:
                            ELOG_WARN_T("Unsupported H264 profile: %s", avcodec_profile_name(video_st->codecpar->codec_id, video_st->codecpar->profile));
                            m_AsyncEvent << ",\"profile\":"  << "\"M\"";
                            break;
                    }

                    break;

                case AV_CODEC_ID_H265:
                    m_videoFormat = FRAME_FORMAT_H265;
                    m_AsyncEvent << ",\"video\":{\"codec\":" << "\"h265\"";
                    break;

                default:
                    ELOG_WARN("Video codec %s is not supported", avcodec_get_name(videoCodecId));
                    break;
            }

            if (m_videoFormat != FRAME_FORMAT_UNKNOWN) {
                m_videoWidth = video_st->codecpar->width;
                m_videoHeight = video_st->codecpar->height;
                m_AsyncEvent << ",\"resolution\":" << "{\"width\":" << video_st->codecpar->width << ", \"height\":" << video_st->codecpar->height << "}}";

                if (!isRtsp())
                    m_videoJitterBuffer.reset(new JitterBuffer("video", JitterBuffer::SYNC_MODE_SLAVE, this));

                m_videoTimeBase.num = 1;
                m_videoTimeBase.den = 90000;

                m_videoStreamIndex = streamNo;
            } else if (!m_enableVideo.compare("yes")) {
                m_AsyncEvent.str("");
                m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"video codec is not supported\"}";
                return false;
            }
        } else {
            ELOG_WARN("No Video stream found");

            if (!m_enableVideo.compare("yes")) {
                m_AsyncEvent.str("");
                m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"no video stream found\"}";
                return false;
            }
        }
    }

    if (!m_enableAudio.compare("yes") || !m_enableAudio.compare("auto")) {
        int streamNo = av_find_best_stream(m_context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (streamNo >= 0) {
            audio_st = m_context->streams[streamNo];
            ELOG_INFO_T("Has audio, audio stream number(%d), codec(%s), %d-%d",
                    streamNo,
                    avcodec_get_name(audio_st->codecpar->codec_id),
                    audio_st->codecpar->sample_rate,
                    audio_st->codecpar->channels
                    );

            AVCodecID audioCodecId = audio_st->codecpar->codec_id;
            switch(audioCodecId) {
                case AV_CODEC_ID_PCM_MULAW:
                    m_audioFormat = FRAME_FORMAT_PCMU;
                    m_AsyncEvent << ",\"audio\":{\"codec\":" << "\"pcmu\"}";
                    break;

                case AV_CODEC_ID_PCM_ALAW:
                    m_audioFormat = FRAME_FORMAT_PCMA;
                    m_AsyncEvent << ",\"audio\":{\"codec\":" << "\"pcma\"}";
                    break;

                case AV_CODEC_ID_OPUS:
                    m_audioFormat = FRAME_FORMAT_OPUS;
                    m_AsyncEvent << ",\"audio\":{\"codec\":" << "\"opus\",\"sampleRate\":48000, \"channelNum\":2}";
                    break;

                case AV_CODEC_ID_AAC:
                    m_audioFormat = FRAME_FORMAT_AAC;
                    m_AsyncEvent << ",\"audio\":{\"codec\":" << "\"aac\"}";
                    break;

                case AV_CODEC_ID_AC3:
                    m_audioFormat = FRAME_FORMAT_AC3;
                    m_AsyncEvent << ",\"audio\":{\"codec\":" << "\"ac3\"}";
                    break;

                case AV_CODEC_ID_NELLYMOSER:
                    m_audioFormat = FRAME_FORMAT_NELLYMOSER;
                    m_AsyncEvent << ",\"audio\":{\"codec\":" << "\"nellymoser\"}";
                    break;

                default:
                    ELOG_WARN("Audio codec %s is not supported ", avcodec_get_name(audioCodecId));
                    break;
            }

            if (m_audioFormat != FRAME_FORMAT_UNKNOWN) {
                if (!isRtsp())
                    m_audioJitterBuffer.reset(new JitterBuffer("audio", JitterBuffer::SYNC_MODE_MASTER, this));

                m_audioTimeBase.num = 1;
                m_audioTimeBase.den = audio_st->codecpar->sample_rate;

                m_audioSampleRate = audio_st->codecpar->sample_rate;
                m_audioChannels = audio_st->codecpar->channels;

                m_audioStreamIndex = streamNo;
            } else if (!m_enableAudio.compare("yes")) {
                m_AsyncEvent.str("");
                m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"audio codec is not supported\"}";
                return false;
            }
        } else {
            ELOG_WARN("No Audio stream found");

            if (!m_enableAudio.compare("yes")) {
                m_AsyncEvent.str("");
                m_AsyncEvent << "{\"type\":\"failed\",\"reason\":\"no audio stream found\"}";
                return false;
            }
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

    if (!strcmp(m_context->iformat->name, "flv")
            && m_videoStreamIndex != -1
            && m_context->streams[m_videoStreamIndex]->codecpar->codec_id == AV_CODEC_ID_H264) {
        m_enableVideoExtradata = true;
        ELOG_INFO_T("Enable video extradata");
    }

    return true;
}

bool LiveStreamIn::reconnect()
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

    m_needCheckVBS = true;
    m_needApplyVBSF = false;
    if (m_vbsf) {
        av_bsf_free(&m_vbsf);
        m_vbsf = NULL;
    }

    m_enableVideoExtradata = false;
    if (m_sps_pps_buffer) {
        m_sps_pps_buffer.reset();
        m_sps_pps_buffer_length = 0;
    }

    if (m_context) {
        avformat_close_input(&m_context);
        avformat_free_context(m_context);
        m_context = NULL;
    }

    m_timeoutHandler->reset(10000);

    m_context = avformat_alloc_context();
    m_context->interrupt_callback = {&TimeoutHandler::checkInterrupt, m_timeoutHandler};
    //m_context->max_analyze_duration = 3 * AV_TIME_BASE;

    ELOG_DEBUG_T("Opening input");
    m_timeoutHandler->reset(60000);
    res = avformat_open_input(&m_context, m_url.c_str(), nullptr, &m_options);
    if (res != 0) {
        ELOG_ERROR_T("Error opening input %s", ff_err2str(res));
        return false;
    }

    ELOG_DEBUG_T("Finding stream info");
    m_timeoutHandler->reset(10000);
    m_context->fps_probe_size = 0;
    m_context->max_ts_probe = 0;
    res = avformat_find_stream_info(m_context, nullptr);
    if (res < 0) {
        ELOG_ERROR_T("Error find stream info %s", ff_err2str(res));
        return false;
    }

    ELOG_DEBUG_T("Dump format");
    av_dump_format(m_context, 0, m_url.c_str(), 0);

    if (m_videoStreamIndex != -1) {
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

    if (m_audioStreamIndex != -1) {
        int streamNo = av_find_best_stream(m_context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (streamNo < 0) {
            ELOG_ERROR_T("No Audio stream found");
            return false;
        }

        if (m_audioStreamIndex != streamNo) {
            ELOG_ERROR_T("Audio stream index changed, %d -> %d", m_audioStreamIndex, streamNo);
            m_audioStreamIndex = streamNo;
        }
    }

    if (m_isFileInput)
        m_timstampOffset = m_lastTimstamp + 1;

    av_read_play(m_context);

    if (m_videoJitterBuffer)
        m_videoJitterBuffer->start();
    if (m_audioJitterBuffer)
        m_audioJitterBuffer->start();

    if (!strcmp(m_context->iformat->name, "flv")
            && m_videoStreamIndex != -1
            && m_context->streams[m_videoStreamIndex]->codecpar->codec_id == AV_CODEC_ID_H264) {
        m_enableVideoExtradata = true;
        ELOG_INFO_T("Enable video extradata");
    }

    return true;
}

void LiveStreamIn::receiveLoop()
{
    int ret = connect();
    if (!ret) {
        ELOG_ERROR_T("Connect failed, %s", m_AsyncEvent.str().c_str());

        ::notifyAsyncEvent(m_asyncHandle, "status", m_AsyncEvent.str());
        return;
    }
    ELOG_DEBUG_T("%s", m_AsyncEvent.str().c_str());
    ::notifyAsyncEvent(m_asyncHandle, "status", m_AsyncEvent.str().c_str());

    ELOG_DEBUG_T("Start playing %s", m_url.c_str() );

    if (m_videoStreamIndex != -1) {
        int i = 0;

        while (m_running && !m_keyFrameRequest) {
            if (i++ >= 100) {
                ELOG_DEBUG_T("No incoming key frame request");
                break;
            }
            deliverNullVideoFrame();
            ELOG_TRACE_T("Wait for key frame request, retry %d", i);
            usleep(10000);
        }
    }

    memset(&m_avPacket, 0, sizeof(m_avPacket));
    while (m_running) {
        if (m_isFileInput) {
            if (m_videoJitterBuffer && m_videoJitterBuffer->sizeInMs() > 500) {
                usleep(1000);
                continue;
            }
            if (m_audioJitterBuffer && m_audioJitterBuffer->sizeInMs() > 500) {
                usleep(1000);
                continue;
            }
        }

        av_init_packet(&m_avPacket);
        m_timeoutHandler->reset(10000);
        ret = av_read_frame(m_context, &m_avPacket);
        if (ret < 0) {
            ELOG_WARN_T("Error read frame, %s", ff_err2str(ret));
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
            m_avPacket.dts = timeRescale(m_avPacket.dts, video_st->time_base, m_msTimeBase) + m_timstampOffset;
            m_avPacket.pts = timeRescale(m_avPacket.pts, video_st->time_base, m_msTimeBase) + m_timstampOffset;

            ELOG_TRACE_T("Receive video frame packet, dts %ld, size %d"
                    , m_avPacket.dts, m_avPacket.size);

            if (filterVBS(video_st, &m_avPacket)) {
                filterPS(video_st, &m_avPacket);

                if (m_videoJitterBuffer)
                    m_videoJitterBuffer->insert(m_avPacket);
                else
                    deliverVideoFrame(&m_avPacket);
            }
        } else if (m_avPacket.stream_index == m_audioStreamIndex) { //packet is audio
            AVStream *audio_st = m_context->streams[m_audioStreamIndex];
            m_avPacket.dts = timeRescale(m_avPacket.dts, audio_st->time_base, m_msTimeBase) + m_timstampOffset;
            m_avPacket.pts = timeRescale(m_avPacket.pts, audio_st->time_base, m_msTimeBase) + m_timstampOffset;

            ELOG_TRACE_T("Receive audio frame packet, dts %ld, duration %ld, size %d"
                    , m_avPacket.dts, m_avPacket.duration, m_avPacket.size);

            if (m_audioJitterBuffer)
                m_audioJitterBuffer->insert(m_avPacket);
            else
                deliverAudioFrame(&m_avPacket);
        }
        m_lastTimstamp = m_avPacket.dts;
        av_packet_unref(&m_avPacket);
    }

    ELOG_DEBUG_T("Thread exited!");
}

void LiveStreamIn::checkVideoBitstream(AVStream *st, const AVPacket *pkt)
{
    int ret;
    const char *filter_name = NULL;
    const AVBitStreamFilter *bsf = NULL;

    if (!m_needCheckVBS)
        return;

    m_needApplyVBSF = false;
    switch(st->codecpar->codec_id) {
        case AV_CODEC_ID_H264:
            if (pkt->size < 5 || AV_RB32(pkt->data) == 0x0000001 || AV_RB24(pkt->data) == 0x000001)
                break;
            filter_name = "h264_mp4toannexb";
            break;
        case AV_CODEC_ID_HEVC:
            if (pkt->size < 5 || AV_RB32(pkt->data) == 0x0000001 || AV_RB24(pkt->data) == 0x000001)
                break;
            filter_name = "hevc_mp4toannexb";
            break;
        default:
            break;
    }

    if (filter_name) {
        bsf = av_bsf_get_by_name(filter_name);
        if(!bsf) {
            ELOG_ERROR_T("Fail to get bsf, %s", filter_name);
            goto exit;
        }

        ret = av_bsf_alloc(bsf, &m_vbsf);
        if (ret) {
            ELOG_ERROR_T("Fail to alloc bsf context");
            goto exit;
        }

        ret = avcodec_parameters_copy(m_vbsf->par_in, st->codecpar);
        if (ret < 0) {
            ELOG_ERROR_T("Fail to copy bsf parameters, %s", ff_err2str(ret));

            av_bsf_free(&m_vbsf);
            m_vbsf = NULL;
            goto exit;
        }

        ret = av_bsf_init(m_vbsf);
        if (ret < 0) {
            ELOG_ERROR_T("Fail to init bsf, %s", ff_err2str(ret));

            av_bsf_free(&m_vbsf);
            m_vbsf = NULL;
            goto exit;
        }

        m_needApplyVBSF = true;
    }

exit:
    m_needCheckVBS = false;
    ELOG_DEBUG_T("%s video bitstream filter", m_needApplyVBSF ? "Apply" : "Not apply");
}

bool LiveStreamIn::filterVBS(AVStream *st, AVPacket *pkt) {
    int ret;

    checkVideoBitstream(st, pkt);
    if (!m_needApplyVBSF)
        return true;

    if (!m_vbsf) {
        ELOG_ERROR_T("Invalid vbs filter");
        return false;
    }

    AVPacket filter_pkt;
    AVPacket filtered_pkt;

    memset(&filter_pkt, 0, sizeof(filter_pkt));
    memset(&filtered_pkt, 0, sizeof(filtered_pkt));

    if ((ret = av_packet_ref(&filter_pkt, pkt)) < 0) {
        ELOG_ERROR_T("Fail to ref input pkt");
        return false;
    }

    if ((ret = av_bsf_send_packet(m_vbsf, &filter_pkt)) < 0) {
        ELOG_ERROR_T("Fail to send packet, %s", ff_err2str(ret));
        av_packet_unref(&filter_pkt);
        return false;
    }

    if ((ret = av_bsf_receive_packet(m_vbsf, &filtered_pkt)) < 0) {
        ELOG_ERROR_T("Fail to receive packet, %s", ff_err2str(ret));
        av_packet_unref(&filter_pkt);
        return false;
    }

    av_packet_unref(&filter_pkt);
    av_packet_unref(pkt);
    av_packet_move_ref(pkt, &filtered_pkt);

    return true;
}

bool LiveStreamIn::parse_avcC(AVPacket *pkt) {
    uint8_t *data;
    int size;

    data = av_packet_get_side_data(&m_avPacket, AV_PKT_DATA_NEW_EXTRADATA, &size);
    if(data == NULL)
        return true;

    if (data[0] == 1) { //AVCDecoderConfigurationRecord
        int nals_size = 0;

        if (m_sps_pps_buffer) {
            m_sps_pps_buffer.reset();
            m_sps_pps_buffer_length = 0;
        }

        int nals_buf_length = 128;
        uint8_t *nals_buf = (uint8_t *)malloc(nals_buf_length);
        if (nals_buf == nullptr) {
            ELOG_ERROR_T("OOM! Allocate size %d", nals_buf_length);
            return false;
        }

        int i, cnt, nalsize;
        const uint8_t *p = data;

        if (size < 7) {
            ELOG_ERROR_T("avcC %d too short", size);

            free(nals_buf);
            return false;
        }

        // Decode sps from avcC
        cnt = *(p + 5) & 0x1f; // Number of sps
        p  += 6;
        for (i = 0; i < cnt; i++) {
            nalsize = AV_RB16(p) + 2;
            if (nalsize > size - (p - data)) {
                ELOG_ERROR_T("avcC %d invalid sps size", nalsize);

                free(nals_buf);
                return false;
            }

            p += 2;
            nalsize -= 2;

            if (nalsize + 4 >= nals_buf_length - nals_size) {
                ELOG_DEBUG_T("enlarge avcC nals buf %d", nalsize + 4);

                nals_buf_length += nalsize + 4;
                nals_buf = (uint8_t *)realloc(nals_buf, nals_buf_length);
                if (nals_buf == nullptr) {
                    ELOG_ERROR_T("OOM! Allocate size %d", nals_buf_length);
                    return false;
                }
            }

            nals_buf[nals_size] = 0;
            nals_buf[nals_size + 1] = 0;
            nals_buf[nals_size + 2] = 0;
            nals_buf[nals_size + 3] = 1;
            memcpy(nals_buf + nals_size + 4, p, nalsize);
            nals_size += nalsize + 4;

            p += nalsize;
        }

        // Decode pps from avcC
        cnt = *(p++); // Number of pps
        for (i = 0; i < cnt; i++) {
            nalsize = AV_RB16(p) + 2;
            if (nalsize > size - (p - data)) {
                ELOG_ERROR_T("avcC %d invalid pps size", nalsize);

                free(nals_buf);
                return false;
            }

            p += 2;
            nalsize -= 2;

            if (nalsize + 4 >= nals_buf_length - nals_size) {
                ELOG_DEBUG_T("enlarge avcC nals buf %d", nalsize + 4);

                nals_buf_length += nalsize + 4;
                nals_buf = (uint8_t *)realloc(nals_buf, nals_buf_length);
                if (nals_buf == nullptr) {
                    ELOG_ERROR_T("OOM! Allocate size %d", nals_buf_length);
                    return false;
                }
            }

            nals_buf[nals_size] = 0;
            nals_buf[nals_size + 1] = 0;
            nals_buf[nals_size + 2] = 0;
            nals_buf[nals_size + 3] = 1;
            memcpy(nals_buf + nals_size + 4, p, nalsize);
            nals_size += nalsize + 4;

            p += nalsize;
        }

        if (nals_size > 0) {
            ELOG_DEBUG_T("New video extradata");

            m_sps_pps_buffer_length = nals_size;
            m_sps_pps_buffer.reset(new uint8_t[m_sps_pps_buffer_length]);
            memcpy(m_sps_pps_buffer.get(), nals_buf , m_sps_pps_buffer_length);
        }

        free(nals_buf);
    } else {
        ELOG_WARN_T("Unsupported video extradata\n");
    }

    return true;
}

bool LiveStreamIn::filterPS(AVStream *st, AVPacket *pkt) {
    if (!m_enableVideoExtradata)
        return true;

    parse_avcC(pkt);
    if (m_sps_pps_buffer && m_sps_pps_buffer_length > 0) {
        int size;

        std::vector<int> sps_pps, pass_types;
        sps_pps.push_back(7);
        sps_pps.push_back(8);

        size = filterNALs(pkt->data, pkt->size, sps_pps, pass_types);
        if (size != pkt->size) {
            ELOG_TRACE_T("Rewrite sps/pps\n");

            av_shrink_packet(pkt, size);

            av_grow_packet(pkt, m_sps_pps_buffer_length);
            memmove(pkt->data + m_sps_pps_buffer_length, pkt->data, pkt->size - m_sps_pps_buffer_length);
            memcpy(pkt->data, m_sps_pps_buffer.get(), m_sps_pps_buffer_length);
        }
    }

    return true;
}

void LiveStreamIn::onSyncTimeChanged(JitterBuffer *jitterBuffer, int64_t syncTimestamp)
{
    if (m_audioJitterBuffer.get() == jitterBuffer) {
        ELOG_DEBUG_T("onSyncTimeChanged audio, timestamp %ld ", syncTimestamp);

        //rtsp audio/video time base is different, it will lost sync after roll back
        if(!isRtsp()) {
            boost::posix_time::ptime mst = boost::posix_time::microsec_clock::local_time();

            m_audioJitterBuffer->setSyncTime(syncTimestamp, mst);
            if (m_videoJitterBuffer)
                m_videoJitterBuffer->setSyncTime(syncTimestamp, mst);
        }
    }
    else if (m_videoJitterBuffer.get() == jitterBuffer) {
        ELOG_DEBUG_T("onSyncTimeChanged video, timestamp %ld ", syncTimestamp);
    } else {
        ELOG_ERROR_T("Invalid JitterBuffer onSyncTimeChanged event!");
    }
}

void LiveStreamIn::deliverNullVideoFrame()
{
    uint8_t dumyData = 0;
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = m_videoFormat;
    frame.payload = &dumyData;
    deliverFrame(frame);

    ELOG_DEBUG_T("deliver null video frame");
}

void LiveStreamIn::deliverVideoFrame(AVPacket *pkt)
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = m_videoFormat;
    frame.payload = reinterpret_cast<uint8_t*>(pkt->data);
    frame.length = pkt->size;
    frame.timeStamp = timeRescale(pkt->dts, m_msTimeBase, m_videoTimeBase);
    frame.additionalInfo.video.width = m_videoWidth;
    frame.additionalInfo.video.height = m_videoHeight;
    frame.additionalInfo.video.isKeyFrame = (pkt->flags & AV_PKT_FLAG_KEY);
    deliverFrame(frame);

    ELOG_TRACE_T("deliver video frame, timestamp %ld(%ld), size %4d, %s"
            , timeRescale(frame.timeStamp, m_videoTimeBase, m_msTimeBase)
            , pkt->dts
            , frame.length
            , (pkt->flags & AV_PKT_FLAG_KEY) ? "key" : "non-key"
            );
}

void LiveStreamIn::deliverAudioFrame(AVPacket *pkt)
{
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = m_audioFormat;
    frame.payload = reinterpret_cast<uint8_t*>(pkt->data);
    frame.length = pkt->size;
    frame.timeStamp = timeRescale(pkt->dts, m_msTimeBase, m_audioTimeBase);
    frame.additionalInfo.audio.isRtpPacket = 0;
    frame.additionalInfo.audio.sampleRate = m_audioSampleRate;
    frame.additionalInfo.audio.channels = m_audioChannels;
    frame.additionalInfo.audio.nbSamples = frame.length / frame.additionalInfo.audio.channels /2;
    deliverFrame(frame);

    ELOG_TRACE_T("deliver audio frame, timestamp %ld(%ld), size %4d"
            , timeRescale(frame.timeStamp, m_audioTimeBase, m_msTimeBase)
            , pkt->dts
            , frame.length);
}

void LiveStreamIn::onDeliverFrame(JitterBuffer *jitterBuffer, AVPacket *pkt)
{
    if (m_videoJitterBuffer.get() == jitterBuffer) {
        deliverVideoFrame(pkt);
    } else if (m_audioJitterBuffer.get() == jitterBuffer) {
        deliverAudioFrame(pkt);
    } else {
        ELOG_ERROR_T("Invalid JitterBuffer onDeliver event!");
    }
}

char *LiveStreamIn::ff_err2str(int errRet)
{
    av_strerror(errRet, (char*)(&m_errbuff), 500);
    return m_errbuff;
}

}
