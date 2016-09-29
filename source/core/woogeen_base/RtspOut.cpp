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

#include "RtspOut.h"

#include <rtputils.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avstring.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
}

static const char *AUDIO_RAW_FILENAME = "/tmp/rtspOutAudioRaw.pcm";
static const char *DUMP_FILENAME = "/tmp/rtspOut.flv";

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

static inline const char* getShortName(std::string& url)
{
    if (url.compare(0, 7, "rtsp://") == 0)
        return "rtsp";
    else if (url.compare(0, 7, "rtmp://") == 0)
        return "flv";
    else if (url.compare(0, 7, "http://") == 0)
        return "hls";
    return nullptr;
}

namespace woogeen_base {

DEFINE_LOGGER(RtspOut, "woogeen.RtspOut");

RtspOut::RtspOut(const std::string& url, const AVOptions* audio, const AVOptions* video, EventRegistry* handle)
    : AVStreamOut{ handle }
    , m_uri{ url }
    , m_audioReceived(false)
    , m_videoReceived(false)
    , m_ifmtCtx(nullptr)
    , m_inputVideoStream(nullptr)
    , m_context{ nullptr }
    , m_audioStream{ nullptr }
    , m_videoStream{ nullptr }
    , m_audioFifo{ nullptr }
    , m_audioEncodingFrame{ nullptr }
    , m_timeOffset(0)
    , m_lastAudioTimestamp(-1)
    , m_lastVideoTimestamp(-1)
    , m_audioRawDumpFile( nullptr )
    , m_dumpContext( nullptr )
{
    m_videoQueue.reset(new woogeen_base::MediaFrameQueue());
    m_audioQueue.reset(new woogeen_base::MediaFrameQueue());

    if (audio) {
        m_audioOptions = *audio;
    }

    if (video) {
        m_videoOptions = *video;
    }

    ELOG_TRACE("url %s", m_uri.c_str());
    ELOG_TRACE("acodec %s, vcodec %s", m_audioOptions.codec.c_str(), m_videoOptions.codec.c_str());

    if (ELOG_IS_TRACE_ENABLED())
        av_log_set_level(AV_LOG_TRACE);
    else if (ELOG_IS_DEBUG_ENABLED())
        av_log_set_level(AV_LOG_DEBUG);
    else
        av_log_set_level(AV_LOG_INFO);

    avcodec_register_all();

    if(!createContext()) {
        return;
    }

    notifyAsyncEvent("init", "");

    m_audioWorker = boost::thread(&RtspOut::audioRun, this);
    m_videoWorker = boost::thread(&RtspOut::videoRun, this);
}

RtspOut::~RtspOut()
{
    close();
}

void RtspOut::close()
{
    ELOG_INFO("closing");

    if (m_status == AVStreamOut::Context_CLOSED || m_status == AVStreamOut::Context_EMPTY) {
        ELOG_INFO("status %s, do nothing", m_status == AVStreamOut::Context_CLOSED ? "CLOSED" : "EMPTY");

        m_audioWorker.join();
        m_videoWorker.join();
        return;
    }

    m_status = AVStreamOut::Context_CLOSED;

    m_audioWorker.join();
    m_videoWorker.join();

    // video input context
    if (m_ifmtCtx) {
        if (m_ifmtCtx->pb) {
            av_freep(&m_ifmtCtx->pb->buffer);
            av_freep(&m_ifmtCtx->pb);
            m_ifmtCtx->pb = NULL;
        }

       avformat_close_input(&m_ifmtCtx);
    }

    // audio encoder
    if (m_audioEncodingFrame) {
        av_frame_free(&m_audioEncodingFrame);
        m_audioEncodingFrame = NULL;
    }

    if (m_audioFifo) {
        av_audio_fifo_free(m_audioFifo);
        m_audioFifo = NULL;
    }

    if (m_audioStream) {
        avcodec_close(m_audioStream->codec);
    }

    // output context
    if (m_context) {
        av_write_trailer(m_context);
        avformat_free_context(m_context);
        m_context = NULL;
    }

    closeDump();

    ELOG_INFO("closed");
}

int RtspOut::readFunction(void* opaque, uint8_t* buf, int buf_size)
{
    const int timeout = 200; // ms
    const int retry = 5;  // eos if no frame in 1s

    uint32_t len = 0;
    int i = 0;

    RtspOut* This = static_cast<RtspOut*>(opaque);

    while (This->m_status != Context_CLOSED) {
        boost::shared_ptr<woogeen_base::EncodedFrame> frame;
        frame = This->m_videoQueue->popFrame();

        if (frame) {
            if (len + frame->m_payloadSize > (uint32_t)buf_size) {
                ELOG_ERROR("Read buf is full, discard encoded video frame");
                return len;
            }

            memcpy(buf, frame->m_payloadData, frame->m_payloadSize);
            len += frame->m_payloadSize;

            return len;
        } else {
            boost::mutex::scoped_lock slock(This->m_readCbmutex);

            if (i >= retry) {
                ELOG_INFO("No video frame in %d(s)...", timeout * retry / 1000);
                break;
            }

            if (!This->m_readCbcond.timed_wait(slock, boost::get_system_time() + boost::posix_time::milliseconds(timeout))) {
                i++;

                ELOG_INFO("No video frame in last %d(ms)...", timeout * i);
            }

            continue;
        }
    }

    // end of stream
    ELOG_INFO("video EOS");
    return 0;
}

bool RtspOut::detectInputVideoStream()
{
    int ret;
    int videoIndex = -1;

    m_ifmtCtx = avformat_alloc_context();
    m_ifmtCtx->pb = avio_alloc_context(
            reinterpret_cast<unsigned char*>(av_malloc(VIDEO_BUFFER_SIZE + FF_INPUT_BUFFER_PADDING_SIZE)),
            VIDEO_BUFFER_SIZE,
            0,
            this,
            readFunction,
            nullptr,
            nullptr);

    m_ifmtCtx->max_analyze_duration = 1 * AV_TIME_BASE;

    ret = avformat_open_input(&m_ifmtCtx, nullptr, 0, 0);
    if (ret != 0) {
        ELOG_ERROR("could not open video input");

        notifyAsyncEvent("fatal", "could not open video input");
        return false;
    }

#if 1 // compatible w/ rtmp flash media player
    if ((ret = avformat_find_stream_info(m_ifmtCtx, 0)) < 0) {
        ELOG_ERROR("failed to retrieve input stream information");

        notifyAsyncEvent("fatal", "failed to retrieve input stream information");
        return false;
    }
#endif

    videoIndex = av_find_best_stream(m_ifmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoIndex < 0) {
        ELOG_ERROR("invalid input video stream");

        notifyAsyncEvent("fatal", "invalid input video stream");
        return false;
    }

    av_dump_format(m_ifmtCtx, 0, nullptr, 0);

    m_inputVideoStream = m_ifmtCtx->streams[videoIndex];
    if (!m_inputVideoStream) {
        ELOG_ERROR("null input video stream");

        notifyAsyncEvent("fatal", "null input video stream");
        return false;
    }

    return true;
}

void RtspOut::audioRun()
{
    ELOG_DEBUG("audioWork started!");

    while (m_status == AVStreamOut::Context_EMPTY) {
        ELOG_TRACE("wait for context ready");
        usleep(10000);
    }

    if (!hasAudio()) {
        ELOG_DEBUG("video only streaming, audioWorker exited!");
        return;
    }

    while (m_status == AVStreamOut::Context_READY) {
        {
            boost::mutex::scoped_lock slock(m_audioFifoMutex);
            int nbSamples = m_audioStream->codec->frame_size;
            int n;

            while (m_status == AVStreamOut::Context_READY && av_audio_fifo_size(m_audioFifo) < nbSamples) {
                int timeout = 200;
                if (!m_audioFifoCond.timed_wait(slock, boost::get_system_time() + boost::posix_time::milliseconds(timeout))) {
                    ELOG_WARN("No audio frame in last %d(ms)...", timeout);

                    continue;
                }
            }
            if (m_status != AVStreamOut::Context_READY)
                break;

            n = av_audio_fifo_read(m_audioFifo, reinterpret_cast<void**>(m_audioEncodingFrame->data), nbSamples);
            if (n != nbSamples) {
                ELOG_ERROR("cannot read enough data from fifo, needed %d, read %d", nbSamples, n);

                break;
            }
        }

        writeAudioFrame();
    }

    ELOG_DEBUG("audioWork exited!");
}

void RtspOut::videoRun()
{
    ELOG_DEBUG("videoWork started!");

    if (hasVideo() && !detectInputVideoStream()) {
        m_status = AVStreamOut::Context_CLOSED;
        return;
    }

    if (!init()) {
        m_status = AVStreamOut::Context_CLOSED;
        return;
    }

#ifdef DUMP_OUTPUT
#if 1
    initDump(true);
#else
    const char *key = "enableRtspOutDump";
    char *value = NULL;

    value = getenv(key);
    if (value) {
        ELOG_TRACE("getenv key(%s) ok, dump enabled", key);

        initDump(true);
    }
    else {
        ELOG_TRACE("getenv key(%s) failed, dump disabled", key);
    }
#endif
#endif

    m_status = AVStreamOut::Context_READY;
    ELOG_DEBUG("initialized");

    if (!hasVideo()) {
        ELOG_DEBUG("audio only streaming, videoWorker exited!");
        return;
    }

    while (m_status == AVStreamOut::Context_READY) {
        writeVideoFrame();
    }

    ELOG_DEBUG("videoWork exited!");
}

bool RtspOut::createContext()
{
    avformat_alloc_output_context2(&m_context, nullptr, getShortName(m_uri), m_uri.c_str());
    if (!m_context) {
        ELOG_ERROR("cannot allocate output context, url %s, format %s", m_uri.c_str(), getShortName(m_uri));

        notifyAsyncEvent("init", "cannot allocate output context");
        goto fail;
    }

    if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_context->pb, m_context->filename, AVIO_FLAG_WRITE) < 0) {
            ELOG_ERROR("cannot open connection, %s", m_uri.c_str());

            notifyAsyncEvent("init", "cannot open connection");
            goto fail;
        }
    }

    return true;

fail:
    if (m_context) {
        avformat_free_context(m_context);
        m_context = NULL;
    }

    return false;
}

bool RtspOut::init()
{
    AVDictionary *options = NULL;

    if (!hasAudio() || !hasVideo()) {
        ELOG_ERROR("no a/v options specified, audio %d, video %d", hasAudio(), hasVideo());

        notifyAsyncEvent("fatal", "no a/v options specified");
        return false;
    }

    const int retry = 100; // timeout in 1s
    int i;

    while ((!m_audioReceived || !m_videoReceived) && i++ < retry) {
        ELOG_INFO("wait for av options available, audio %d, video %d, retry %d"
                , m_audioReceived, m_videoReceived, i);

        usleep(10000);
    }
    if (!m_audioReceived || !m_videoReceived) {
        ELOG_ERROR("no a/v frames available, audio %d, video %d"
                , m_audioReceived, m_videoReceived);

        notifyAsyncEvent("fatal", "no a/v frames available");
        return false;
    }

    if (m_audioOptions.codec.compare("pcm_raw") != 0) {
        ELOG_ERROR("invalid audio codec, %s", m_audioOptions.codec.c_str());

        notifyAsyncEvent("fatal", "invalid audio codec");
        return false;
    }
    if (m_videoOptions.codec.compare("h264") != 0) {
        ELOG_ERROR("invalid video codec, %s", m_videoOptions.codec.c_str());

        notifyAsyncEvent("fatal", "invalid video codec");
        return false;
    }

#if 0
    avformat_alloc_output_context2(&m_context, nullptr, getShortName(m_uri), m_uri.c_str());
    if (!m_context) {
        ELOG_ERROR("cannot allocate output context, url %s, format %s", m_uri.c_str(), getShortName(m_uri));

        notifyAsyncEvent("fatal", "cannot allocate output context");
        return false;
    }

    if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_context->pb, m_context->filename, AVIO_FLAG_WRITE) < 0) {
            ELOG_ERROR("cannot open connection, %s", m_uri.c_str());

            notifyAsyncEvent("fatal", "cannot open connection");
            return false;
        }
    }
#endif

    if (!addVideoStream(AV_CODEC_ID_H264, m_videoOptions.spec.video.width, m_videoOptions.spec.video.height))
        return false;
    if (!addAudioStream(AV_CODEC_ID_AAC, m_audioOptions.spec.audio.channels, m_audioOptions.spec.audio.sampleRate))
        return false;

    if (isHls(m_uri)) {
        std::string::size_type pos1 = m_uri.rfind('/');
        if (pos1 == std::string::npos) {
            ELOG_ERROR("cant not find base url %s", m_uri.c_str());

            return false;
        }

        std::string::size_type pos2 = m_uri.rfind('.');
        if (pos2 == std::string::npos) {
            ELOG_ERROR("cant not find base url %s", m_uri.c_str());

            return false;
        }

        if (pos2 <= pos1) {
             ELOG_ERROR("cant not find base url %s", m_uri.c_str());

             return false;
         }

        std::string segment_uri(m_uri.substr(0, pos1));
        segment_uri.append("/intel_");
        segment_uri.append(m_uri.substr(pos1 + 1, pos2 - pos1 - 1));
        segment_uri.append("_%09d.ts");

        ELOG_TRACE("index url %s", m_uri.c_str());
        ELOG_TRACE("segment url %s", segment_uri.c_str());

        av_dict_set(&options, "hls_segment_filename", segment_uri.c_str(), 0);
        av_dict_set(&options, "hls_time", "10", 0);
        av_dict_set(&options, "hls_list_size", "4", 0);
        av_dict_set(&options, "hls_flags", "delete_segments", 0);
        av_dict_set(&options, "method", "PUT", 0);
    }

    if (avformat_write_header(m_context, &options) < 0) {
        ELOG_ERROR("cannot write header");

        notifyAsyncEvent("fatal", "cannot write header");
        return false;
    }

    av_dump_format(m_context, 0, m_context->filename, 1);
    return true;
}

bool RtspOut::addDumpStream(AVStream *stream)
{
    AVStream* dumpStream = NULL;

    dumpStream = avformat_new_stream(m_dumpContext, stream->codec->codec);
    if (!dumpStream) {
        ELOG_ERROR("cannot add dump stream");

        return false;
    }

    //Copy the settings of AVCodecContext
    if (avcodec_copy_context(dumpStream->codec, stream->codec) < 0) {
        ELOG_ERROR("Failed to copy dump stream codec context");

        return false;
    }

    dumpStream->codec->codec_tag = 0;
    if (m_dumpContext->oformat->flags & AVFMT_GLOBALHEADER)
        dumpStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return true;
}

bool RtspOut::initDump(bool audioRaw)
{
    if(audioRaw)
        m_audioRawDumpFile.reset(new std::ofstream(AUDIO_RAW_FILENAME, std::ios::binary));

    avformat_alloc_output_context2(&m_dumpContext, nullptr, getShortName(m_uri), DUMP_FILENAME);
    if (!m_dumpContext) {
        ELOG_ERROR("avformat_alloc_output_context2 failed, url %s, format %s", DUMP_FILENAME, getShortName(m_uri));

        closeDump();
        return false;
    }

    addDumpStream(m_videoStream);
    addDumpStream(m_audioStream);

    if (!(m_dumpContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_dumpContext->pb, m_dumpContext->filename, AVIO_FLAG_WRITE) < 0) {
            ELOG_ERROR("avio_open failed, %s", m_uri.c_str());

            closeDump();
            return false;
        }
    }

    if (avformat_write_header(m_dumpContext, nullptr) < 0) {
        ELOG_ERROR("avformat_write_header failed");

        closeDump();
        return false;
    }

    av_dump_format(m_dumpContext, 0, m_dumpContext->filename, 1);
    return true;
}

void RtspOut::closeDump()
{
    m_audioRawDumpFile.reset();

    if (m_dumpContext) {
        av_write_trailer(m_dumpContext);
        avformat_free_context(m_dumpContext);
        m_dumpContext = NULL;
    }
}

void RtspOut::onFrame(const woogeen_base::Frame& frame)
{
    if (!m_timeOffset) {
        m_timeOffset = currentTimeMs();

        ELOG_TRACE("initial time offset %ld", m_timeOffset);
    }

    switch (frame.format) {
        case woogeen_base::FRAME_FORMAT_PCM_RAW:
            if (!m_audioReceived) {
                ELOG_TRACE("initial audio options channels %d, sample rate: %d"
                        , frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);

                m_audioOptions.spec.audio.channels = frame.additionalInfo.audio.channels;
                m_audioOptions.spec.audio.sampleRate = frame.additionalInfo.audio.sampleRate;
                m_audioReceived = true;
            }

            if (m_status != AVStreamOut::Context_READY || !m_audioStream)
                return;

            if (frame.additionalInfo.audio.channels != m_audioStream->codec->channels
                    || int(frame.additionalInfo.audio.sampleRate) != m_audioStream->codec->sample_rate) {
                ELOG_ERROR("invalid audio frame channels %d, or sample rate: %d"
                        , frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);

                notifyAsyncEvent("fatal", "invalid audio frame channels or sample rate");
                return close();
            }

            if (m_audioRawDumpFile)
                m_audioRawDumpFile->write((const char*)frame.payload, frame.length);

            addToAudioFifo(frame.payload, frame.additionalInfo.audio.nbSamples);

            break;

        case woogeen_base::FRAME_FORMAT_H264:
            if (!m_videoReceived) {
                ELOG_TRACE("initial video options: %dx%d",
                        frame.additionalInfo.video.width, frame.additionalInfo.video.height);

                m_videoOptions.spec.video.width = frame.additionalInfo.video.width;
                m_videoOptions.spec.video.height = frame.additionalInfo.video.height;
                m_videoReceived = true;

                ELOG_INFO("request video key frame");
                deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });
            }

            if (frame.additionalInfo.video.width != m_videoOptions.spec.video.width
                    || frame.additionalInfo.video.height != m_videoOptions.spec.video.height) {
                ELOG_ERROR("invalid video frame resolution: %dx%d",
                        frame.additionalInfo.video.width, frame.additionalInfo.video.height);

                notifyAsyncEvent("fatal", "invalid video frame resolution");
                return close();
            }
            m_videoQueue->pushFrame(frame.payload, frame.length);
            m_readCbcond.notify_one();

            break;

        default:
            ELOG_ERROR("unsupported frame format: %d", frame.format);

            notifyAsyncEvent("fatal", "unsupported frame format");
            return close();
    }
}

void RtspOut::addToAudioFifo(uint8_t* data, int nbSamples)
{
    boost::mutex::scoped_lock slock(m_audioFifoMutex);
    int n;

    if (!m_audioFifo || !m_audioStream)
        return;

    n = av_audio_fifo_write(m_audioFifo, reinterpret_cast<void**>(&data), nbSamples);
    if (n < nbSamples) {
        ELOG_ERROR("cannot not write data to fifo, bnSamples %d, writed %d", nbSamples, n);

        return;
    }

    if (av_audio_fifo_size(m_audioFifo) >= m_audioStream->codec->frame_size) {
        //trigger audio encoder
        m_audioFifoCond.notify_one();
    }
}

bool RtspOut::encodeAudioFrame(AVPacket *pkt)
{
    int ret;
    int got_output = 0;

    if (!m_audioStream) {
        ELOG_ERROR("invalid audio stream");

        return false;
    }

    av_init_packet(pkt);
    ret = avcodec_encode_audio2(m_audioStream->codec, pkt, m_audioEncodingFrame, &got_output);
    if (ret < 0) {
        ELOG_ERROR("cannot encode audio frame.");

        return false;
    }

    return got_output;
}

int RtspOut::writeAudioFrame()
{
    AVPacket pkt;
    int ret;

    av_init_packet(&pkt);
    ret = encodeAudioFrame(&pkt);
    if (!ret) {
        ELOG_TRACE("No audio frame!");
        return false;
    }

    int64_t timestamp = currentTimeMs() - m_timeOffset;

    pkt.stream_index = m_audioStream->index;
    pkt.pts = (timestamp * 1000) / (av_q2d(m_audioStream->time_base) * AV_TIME_BASE);
    pkt.dts = pkt.pts;
    pkt.duration = (m_lastAudioTimestamp != -1) ? (pkt.pts - m_lastAudioTimestamp) : 0;

    if (m_lastVideoTimestamp != -1) {
        if (pkt.pts <= m_lastVideoTimestamp - 1000) {
            ELOG_INFO("Lost av sync, audio pts (%ld) is too fast than video pts(%ld), > %d ms"
                    , pkt.pts, m_lastVideoTimestamp, 1000);
        }
	}

    m_lastAudioTimestamp = pkt.pts;
    ELOG_TRACE("audio_frame pts: %ld, duration: %ld, size: %d", pkt.pts, pkt.duration, pkt.size);

    {
        boost::unique_lock<boost::shared_mutex> lock(m_contextMutex);

        if (m_dumpContext) {
            AVPacket dump_pkt;
            av_copy_packet(&dump_pkt, &pkt);
            ret = av_interleaved_write_frame(m_dumpContext, &dump_pkt);
        }

        ret = av_interleaved_write_frame(m_context, &pkt);
    }
    if (ret < 0) {
        ELOG_ERROR("Error muxing audio packet");

        av_free_packet(&pkt);
        return false;
    }

    av_free_packet(&pkt);
    return true;
}

int RtspOut::writeVideoFrame()
{
    int ret;
    AVPacket pkt;

    ret = av_read_frame(m_ifmtCtx, &pkt);
    if (ret < 0) {
        ELOG_ERROR("video EOS, error av_read_frame for vide stream");

        notifyAsyncEvent("fatal", "video EOS");
        return false;
    }

    int64_t timestamp = currentTimeMs() - m_timeOffset;

    if (pkt.flags & AV_PKT_FLAG_KEY)
        m_lastKeyFrameReqTime = timestamp;

    if (timestamp - m_lastKeyFrameReqTime > KEYFRAME_REQ_INTERVAL) {
        ELOG_TRACE("Request key frame!");

        m_lastKeyFrameReqTime = timestamp;
        deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });
    }

    if (pkt.pts != AV_NOPTS_VALUE)
        ELOG_WARN("Raw H264 bitstream PTS should be none!");

    pkt.pts = (timestamp * 1000) / (av_q2d(m_videoStream->time_base) * AV_TIME_BASE);
    pkt.dts = pkt.pts;
    pkt.duration = (m_lastVideoTimestamp != -1) ? (pkt.pts - m_lastVideoTimestamp) : 0;

    if (m_lastAudioTimestamp != -1) {
        if (pkt.pts  <= m_lastAudioTimestamp - 1000) {
            ELOG_INFO("Lost av sync, video pts (%ld) is too fast than audio pts(%ld), > %d ms"
                    , pkt.pts, m_lastAudioTimestamp, 1000);
        }
    }

    m_lastVideoTimestamp = pkt.pts;
    ELOG_TRACE("video_frame pts: %ld, duration: %ld, size: %d %s"
            , pkt.pts, pkt.duration, pkt.size, (pkt.flags & AV_PKT_FLAG_KEY) ? "key frame" : "");
    {
        boost::unique_lock<boost::shared_mutex> lock(m_contextMutex);

        if (m_dumpContext) {
            AVPacket dump_pkt;
            av_copy_packet(&dump_pkt, &pkt);
            ret = av_interleaved_write_frame(m_dumpContext, &dump_pkt);
        }

        ret = av_interleaved_write_frame(m_context, &pkt);
    }
    if (ret < 0) {
        ELOG_ERROR("Error muxing video packet");

        av_free_packet(&pkt);
        return false;
    }

    av_free_packet(&pkt);
    return true;
}

bool RtspOut::addAudioStream(enum AVCodecID codec_id, int nbChannels, int sampleRate)
{
    int ret;
    AVCodec* codec = NULL;
    AVStream* stream = NULL;
    AVCodecContext* c = NULL;

    if (codec_id == AV_CODEC_ID_AAC)
        codec = avcodec_find_encoder_by_name("libfdk_aac");
    else
        codec = avcodec_find_encoder(codec_id);

    if (!codec) {
        ELOG_ERROR("cannot find audio encoder %d", codec_id);

        notifyAsyncEvent("fatal", "cannot find audio encoder");

        goto fail;
    }

    stream = avformat_new_stream(m_context, codec);
    if (!stream) {
        ELOG_ERROR("cannot add audio stream");

        notifyAsyncEvent("fatal", "cannot add audio stream");
        goto fail;
    }

    c = stream->codec;
    c->channels         = nbChannels;
    c->channel_layout   = av_get_default_channel_layout(nbChannels);
    c->sample_rate      = sampleRate;
    c->sample_fmt       = AV_SAMPLE_FMT_S16;

    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(c, codec, nullptr);
    if (ret < 0) {
        ELOG_ERROR("cannot open output audio codec");

        notifyAsyncEvent("fatal", "cannot open output audio codec");
        goto fail;
    }

    if (m_audioFifo) {
        ELOG_TRACE("free audio fifo");

        av_audio_fifo_free(m_audioFifo);
        m_audioFifo = NULL;
    }

    m_audioFifo = av_audio_fifo_alloc(c->sample_fmt, c->channels, 1);
    if (!m_audioFifo ) {
        ELOG_ERROR("cannot allocate audio fifo");

        notifyAsyncEvent("fatal", "cannot allocate audio fifo");
        goto fail;
    }

    if (m_audioEncodingFrame) {
        ELOG_TRACE("free audio frame");

        av_frame_free(&m_audioEncodingFrame);
        m_audioEncodingFrame = NULL;
    }

    m_audioEncodingFrame  = av_frame_alloc();
    if (!m_audioEncodingFrame) {
        ELOG_ERROR("cannot allocate audio frame");

        notifyAsyncEvent("fatal", "cannot allocate audio frame");
        goto fail;
    }

    m_audioEncodingFrame->nb_samples        = c->frame_size;
    m_audioEncodingFrame->format            = c->sample_fmt;
    m_audioEncodingFrame->channel_layout    = c->channel_layout;
    m_audioEncodingFrame->sample_rate       = c->sample_rate;

    ret = av_frame_get_buffer(m_audioEncodingFrame, 0);
    if (ret < 0) {
        ELOG_ERROR("cannot get audio frame buffer");

        notifyAsyncEvent("fatal", "cannot allocate audio frame buffer");
        goto fail;
    }

    m_audioStream = stream;

    ELOG_DEBUG("audio stream added: %d channel(s), %d Hz", nbChannels, sampleRate);

    return true;

fail:
    if (m_audioEncodingFrame) {
        av_frame_free(&m_audioEncodingFrame);
        m_audioEncodingFrame = NULL;
    }

    if (m_audioFifo) {
        av_audio_fifo_free(m_audioFifo);
        m_audioFifo = NULL;
    }

    if (c) {
        avcodec_close(c);
        codec = NULL;
    }

    if (m_audioStream) {
        m_audioStream = NULL;
    }

    //stream will be closed by context

    return false;
}

bool RtspOut::addVideoStream(enum AVCodecID codec_id, unsigned int width, unsigned int height)
{
    if (!m_inputVideoStream) {
        ELOG_ERROR("invalid input video stream!");

        notifyAsyncEvent("fatal", "invalid input video stream");
        return false;
    }

    if (codec_id != m_inputVideoStream->codec->codec_id) {
        ELOG_ERROR("cannot add video stream, invalid codec param");

        notifyAsyncEvent("fatal", "cannot add video stream");
        return false;
    }

    if (m_inputVideoStream->codec->width != (int)width || m_inputVideoStream->codec->height != (int)height) {
        ELOG_TRACE("set video size %dx%d -> %dx%d"
            , m_inputVideoStream->codec->width
            , m_inputVideoStream->codec->height
            , width
            , height);

        m_inputVideoStream->codec->width = width;
        m_inputVideoStream->codec->height = height;
    }

    m_videoStream = avformat_new_stream(m_context, m_inputVideoStream->codec->codec);
    if (!m_videoStream) {
        ELOG_ERROR("cannot add video stream");

        notifyAsyncEvent("fatal", "cannot add video stream");
        return false;
    }

    //Copy the settings of AVCodecContext
    if (avcodec_copy_context(m_videoStream->codec, m_inputVideoStream->codec) < 0) {
        ELOG_ERROR("Failed to copy context from input to output stream codec context");

        notifyAsyncEvent("fatal", "cannot configure video stream context");
        return false;
    }

    m_videoStream->codec->codec_tag = 0;
    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        m_videoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    ELOG_DEBUG("video stream added: %dx%d", width, height);
    return true;
}

} /* namespace mcu */
