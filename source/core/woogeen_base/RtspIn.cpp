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

#include "RtspIn.h"

#include <cstdio>
#include <rtputils.h>
#include <sstream>
#include <sys/time.h>

//#define DUMP_AUDIO_PCM

using namespace erizo;

static inline uint32_t timestampRtp(int64_t ts, AVRational tb, AVRational tbRtp)
{
    return av_rescale_q(ts, tb, tbRtp);
}

static inline uint32_t calibrateTimestamp(uint32_t aSamples, uint32_t aSampleRate, uint32_t bSamples, uint32_t bSampleRate, uint32_t bTs, AVRational tb)
{
    int32_t diff = av_rescale_rnd(aSamples, bSampleRate, aSampleRate, AV_ROUND_UP) - bSamples;
    return bTs + diff / av_q2d(tb) / bSampleRate;
}

static inline void notifyAsyncEvent(EventRegistry* handle, const std::string& event, const std::string& data)
{
    if (handle)
        handle->notifyAsyncEvent(event, data);
}

namespace woogeen_base {

DEFINE_LOGGER(RtspIn, "woogeen.RtspIn");

RtspIn::RtspIn(const Options& options, EventRegistry* handle)
    : m_url(options.url)
    , m_needAudio(options.enableAudio)
    , m_needVideo(options.enableVideo)
    , m_transportOpts(nullptr)
    , m_enableH264(options.enableH264)
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
    , m_audioRcvSampleCount(0)
    , m_audioRcvSampleDts(0)
    , m_audioEncodedSampleCount(0)
{
    if (options.transport.compare("tcp") == 0) {
        av_dict_set(&m_transportOpts, "rtsp_transport", "tcp", 0);
        ELOG_DEBUG("url: %s, audio: %d, video: %d, transport::tcp", m_url.c_str(), m_needAudio, m_needVideo);
    } else {
        char buf[256];
        snprintf(buf, sizeof(buf), "%u", options.bufferSize);
        av_dict_set(&m_transportOpts, "buffer_size", buf, 0);
        ELOG_DEBUG("url: %s, audio: %d, video: %d, transport::%s, buffer_size: %u",
                   m_url.c_str(), m_needAudio, m_needVideo, options.transport.c_str(), options.bufferSize);
    }
    m_running = true;
    m_thread = boost::thread(&RtspIn::receiveLoop, this);
}

RtspIn::RtspIn (const std::string& url, const std::string& transport, uint32_t bufferSize, bool enableAudio, bool enableVideo, EventRegistry* handle)
    : m_url(url)
    , m_needAudio(enableAudio)
    , m_needVideo(enableVideo)
    , m_transportOpts(nullptr)
    , m_enableH264(true)
    , m_running(false)
    , m_context(nullptr)
    , m_timeoutHandler(nullptr)
    , m_videoStreamIndex(-1)
    , m_videoFormat(FRAME_FORMAT_UNKNOWN)
    , m_videoSize({1920, 1080})
    , m_vbsf(NULL)
    , m_vbsf_buffer(nullptr)
    , m_vbsf_buffer_size(0)
    , m_audioStreamIndex(-1)
    , m_audioFormat(FRAME_FORMAT_UNKNOWN)
    , m_asyncHandle(handle)
    , m_audioDecFrame(nullptr)
    , m_audioEncCtx(nullptr)
    , m_audioSwrCtx(nullptr)
    , m_audioSwrSamplesData(nullptr)
    , m_audioSwrSamplesLinesize(0)
    , m_audioSwrSamplesCount(0)
    , m_audioEncFifo(nullptr)
    , m_audioEncFrame(nullptr)
    , m_audioRcvSampleCount(0)
    , m_audioRcvSampleDts(0)
    , m_audioEncodedSampleCount(0)
{
    if (transport.compare("tcp") == 0) {
        av_dict_set(&m_transportOpts, "rtsp_transport", "tcp", 0);
        ELOG_DEBUG("url: %s, audio: %d, video: %d, transport::tcp", m_url.c_str(), m_needAudio, m_needVideo);
    } else {
        char buf[256];
        snprintf(buf, sizeof(buf), "%u", bufferSize);
        av_dict_set(&m_transportOpts, "buffer_size", buf, 0);
        ELOG_DEBUG("url: %s, audio: %d, video: %d, transport::%s, buffer_size: %u",
                   m_url.c_str(), m_needAudio, m_needVideo, transport.c_str(), bufferSize);
    }
    m_running = true;
    m_thread = boost::thread(&RtspIn::receiveLoop, this);
}

RtspIn::~RtspIn()
{
    ELOG_DEBUG("closing %s" , m_url.c_str());
    m_running = false;
    m_thread.join();
    av_free_packet(&m_avPacket);
    if (m_context)
        avformat_free_context(m_context);
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

#ifdef DUMP_AUDIO_PCM
    m_audioRawDumpFile.reset();
#endif

    ELOG_DEBUG("closed");
}

bool RtspIn::connect()
{
    srand((unsigned)time(0));

    m_context = avformat_alloc_context();
    m_timeoutHandler = new TimeoutHandler(20000);
    m_context->interrupt_callback = {&TimeoutHandler::checkInterrupt, m_timeoutHandler};
    //open rtsp
    av_init_packet(&m_avPacket);
    m_avPacket.data = nullptr;
    int res = avformat_open_input(&m_context, m_url.c_str(), nullptr, &m_transportOpts);
    char errbuff[500];
    if (res != 0) {
        av_strerror(res, (char*)(&errbuff), 500);
        ELOG_ERROR("Error opening input %s", errbuff);
        return false;
    }
    res = avformat_find_stream_info(m_context, nullptr);
    if (res < 0) {
        av_strerror(res, (char*)(&errbuff), 500);
        ELOG_ERROR("Error finding stream info %s", errbuff);
        return false;
    }

    av_dump_format(m_context, 0, nullptr, 0);

    std::ostringstream status;
    status << "{\"type\":\"ready\"";

    AVStream *st, *audio_st;
    if (m_needVideo) {
        int streamNo = av_find_best_stream(m_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (streamNo < 0) {
            ELOG_WARN("No Video stream found");
        } else {
            m_videoStreamIndex = streamNo;
            st = m_context->streams[streamNo];
            ELOG_DEBUG("Has video, video stream number %d. time base = %d / %d, codec type = %d ",
                      m_videoStreamIndex,
                      st->time_base.num,
                      st->time_base.den,
                      st->codec->codec_id);

            int videoCodecId = st->codec->codec_id;
            if (videoCodecId == AV_CODEC_ID_VP8 || (m_enableH264 && videoCodecId == AV_CODEC_ID_H264)) {
                if (videoCodecId == AV_CODEC_ID_VP8) {
                    m_videoFormat = FRAME_FORMAT_VP8;
                    status << ",\"video_codecs\":" << "[\"vp8\"]";
                } else if (videoCodecId == AV_CODEC_ID_H264) {
                    m_needVBSF = true;
                    if (initVBSFilter()) {
                        m_videoFormat = FRAME_FORMAT_H264;
                        status << ",\"video_codecs\":" << "[\"h264\"]";
                    }
                }
                //FIXME: the resolution info should be retrieved from the rtsp video source.
                std::string resolution = "hd1080p";
                status << ",\"video_resolution\":" << "\"hd1080p\"";
                VideoResolutionHelper::getVideoSize(resolution, m_videoSize);
            } else {
                ELOG_WARN("Video codec %d is not supported ", st->codec->codec_id);
            }

            m_videoTimeBase.num = 1;
            m_videoTimeBase.den = 90000;
        }
    }

    if (m_needAudio) {
        int audioStreamNo = av_find_best_stream(m_context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audioStreamNo < 0) {
            ELOG_WARN("No Audio stream found");
        } else {
            m_audioStreamIndex = audioStreamNo;
            audio_st = m_context->streams[m_audioStreamIndex];
            ELOG_DEBUG("Has audio, audio stream number %d. time base = %d / %d, codec type = %d ",
                      m_audioStreamIndex,
                      audio_st->time_base.num,
                      audio_st->time_base.den,
                      audio_st->codec->codec_id);

            int audioCodecId = audio_st->codec->codec_id;
            switch(audioCodecId) {
                case AV_CODEC_ID_PCM_MULAW:
                    m_audioFormat = FRAME_FORMAT_PCMU;
                    status << ",\"audio_codecs\":" << "[\"pcmu\"]";
                    break;

                case AV_CODEC_ID_PCM_ALAW:
                    m_audioFormat = FRAME_FORMAT_PCMA;
                    status << ",\"audio_codecs\":" << "[\"pcma\"]";
                    break;

                case AV_CODEC_ID_OPUS:
                    m_audioFormat = FRAME_FORMAT_OPUS;
                    status << ",\"audio_codecs\":" << "[\"opus_48000_2\"]";
                    break;

                case AV_CODEC_ID_AAC:
                    m_needAudioTranscoder = true;
                    if (initAudioTranscoder(AV_CODEC_ID_AAC, AV_CODEC_ID_OPUS)) {
                        m_audioFormat = FRAME_FORMAT_OPUS;
                        status << ",\"audio_codecs\":" << "[\"opus_48000_2\"]";
                    }
                    break;

                default:
                    ELOG_WARN("Audio codec %d is not supported ", audioCodecId);
            }

            if (m_audioFormat == FRAME_FORMAT_OPUS) {
                m_audioTimeBase.num = 1;
                m_audioTimeBase.den = 48000;
            }
            else {
                m_audioTimeBase.num = 1;
                m_audioTimeBase.den = 8000;
            }
        }
    }

    if (m_audioFormat == FRAME_FORMAT_UNKNOWN && m_videoFormat == FRAME_FORMAT_UNKNOWN)
        return false;

    status << "}";
    ::notifyAsyncEvent(m_asyncHandle, "status", status.str());

    av_init_packet(&m_avPacket);
    return true;
}

void RtspIn::receiveLoop()
{
    bool ret = connect();
    if (!ret) {
        ::notifyAsyncEvent(m_asyncHandle, "status", "{\"type\":\"failed\",\"reason\":\"opening input url error\"}");
        return;
    }

    av_read_play(m_context); // play RTSP

    ELOG_DEBUG("Start playing %s", m_url.c_str() );
    while (m_running) {
        m_timeoutHandler->reset(1000);
        if (av_read_frame(m_context, &m_avPacket) < 0) {
            // Try to re-open the input - silently.
            m_timeoutHandler->reset(10000);
            av_read_pause(m_context);
            avformat_close_input(&m_context);
            ELOG_WARN("Read input data failed; trying to reopen input from url %s", m_url.c_str());
            m_timeoutHandler->reset(10000);
            int res = avformat_open_input(&m_context, m_url.c_str(), nullptr, &m_transportOpts);
            char errbuff[500];
            if (res != 0) {
                av_strerror(res, (char*)(&errbuff), 500);
                ELOG_ERROR("Error opening input %s", errbuff);
                m_running = false;
                ::notifyAsyncEvent(m_asyncHandle, "status", "{\"type\":\"failed\",\"reason\":\"reopening input url error\"}");
                return;
            }
            av_read_play(m_context);
            continue;
        }

        if (m_avPacket.stream_index == m_videoStreamIndex) { //packet is video
            AVStream *video_st = m_context->streams[m_videoStreamIndex];
            ELOG_TRACE("Receive video frame packet with size %d, dts %d "
                    , m_avPacket.size
                    , timestampRtp(m_avPacket.dts, video_st->time_base, m_videoTimeBase));

            if (!m_needVBSF || filterVBS()) {
                Frame frame;
                memset(&frame, 0, sizeof(frame));
                frame.format = m_videoFormat;
                frame.payload = reinterpret_cast<uint8_t*>(m_avPacket.data);
                frame.length = m_avPacket.size;
                frame.timeStamp = timestampRtp(m_avPacket.dts, video_st->time_base, m_videoTimeBase);
                frame.additionalInfo.video.width = m_videoSize.width;
                frame.additionalInfo.video.height = m_videoSize.height;
                deliverFrame(frame);

                ELOG_DEBUG("Deliver video frame size %4d, timestamp %6d ", frame.length, frame.timeStamp);
            }
        } else if (m_avPacket.stream_index == m_audioStreamIndex) { //packet is audio
            AVStream *audio_st = m_context->streams[m_audioStreamIndex];
            ELOG_TRACE("Receive audio frame packet with size %d, dts %d "
                    , m_avPacket.size
                    , timestampRtp(m_avPacket.dts, audio_st->time_base, m_audioTimeBase));

            if (!m_needAudioTranscoder) {
                Frame frame;
                memset(&frame, 0, sizeof(frame));
                frame.format = m_audioFormat;
                frame.payload = reinterpret_cast<uint8_t*>(m_avPacket.data);
                frame.length = m_avPacket.size;
                frame.timeStamp = timestampRtp(m_avPacket.dts, audio_st->time_base, m_audioTimeBase);
                frame.additionalInfo.audio.isRtpPacket = 0;
                frame.additionalInfo.audio.nbSamples = m_avPacket.duration;
                frame.additionalInfo.audio.sampleRate = m_audioFormat == FRAME_FORMAT_OPUS ? 48000 : 8000;
                frame.additionalInfo.audio.channels = m_audioFormat == FRAME_FORMAT_OPUS ? 2 : 1;
                deliverFrame(frame);

                ELOG_TRACE("Deliver audio frame size %4d, timestamp %6d ", frame.length, frame.timeStamp);
            } else if(decAudioFrame()) {
                while(encAudioFrame()) {
                    Frame frame;
                    memset(&frame, 0, sizeof(frame));
                    frame.format = m_audioFormat;
                    frame.payload = reinterpret_cast<uint8_t*>(m_avPacket.data);
                    frame.length = m_avPacket.size;
                    frame.timeStamp = timestampRtp(m_avPacket.dts, audio_st->time_base, m_audioTimeBase);
                    frame.additionalInfo.audio.isRtpPacket = 0;
                    frame.additionalInfo.audio.nbSamples = m_avPacket.duration;
                    frame.additionalInfo.audio.sampleRate = m_audioFormat == FRAME_FORMAT_OPUS ? 48000 : 8000;
                    frame.additionalInfo.audio.channels = m_audioFormat == FRAME_FORMAT_OPUS ? 2 : 1;
                    deliverFrame(frame);

                    ELOG_TRACE("Deliver audio frame size %4d, timestamp %6d ", frame.length, frame.timeStamp);
                }
            }
        }
        av_free_packet(&m_avPacket);
        av_init_packet(&m_avPacket);
    }
    m_running = false;
    av_read_pause(m_context);
}

bool RtspIn::initVBSFilter() {
    m_vbsf = av_bitstream_filter_init("h264_mp4toannexb");
    if (!m_vbsf) {
        ELOG_ERROR("Cound not init h264 bitstream filter");
        return false;
    }

    ELOG_TRACE("Init vidoe bitstream filter OK");
    return true;
}

bool RtspIn::filterVBS() {
    int ret;

    if (!m_vbsf)
        return false;

    if (m_vbsf_buffer) {
        av_free(m_vbsf_buffer);
        m_vbsf_buffer = NULL;
        m_vbsf_buffer_size = 0;
    }

    ret = av_bitstream_filter_filter(m_vbsf,
            m_context->streams[m_videoStreamIndex]->codec, NULL, &m_vbsf_buffer, &m_vbsf_buffer_size,
            m_avPacket.data, m_avPacket.size, m_avPacket.flags & AV_PKT_FLAG_KEY);
    if(ret <= 0) {
        ELOG_ERROR("Fail to filter video bitstream, %d", ret);
        return false;
    }

    m_avPacket.data = m_vbsf_buffer;
    m_avPacket.size = m_vbsf_buffer_size;

    return true;
}

bool RtspIn::initAudioTranscoder(int inCodec, int outCodec) {
    int ret;
    AVStream *audio_st = m_context->streams[m_audioStreamIndex];
    AVCodec *m_audioDec = NULL;
    AVCodec *m_audioEnc = NULL;

    if (inCodec != AV_CODEC_ID_AAC) {
        ELOG_ERROR("Support inCodec AAC only, %d", inCodec);
        return false;
    }

    if (outCodec != AV_CODEC_ID_OPUS) {
        ELOG_ERROR("Support outCodec OPUS only, %d", outCodec);
        return false;
    }

    avcodec_register_all();

    m_audioDec = avcodec_find_decoder_by_name("libfdk_aac");
    if (!m_audioDec) {
        ELOG_ERROR("Cound not fine audio decoder");
        goto failed;
    }

    audio_st->codec->sample_fmt = AV_SAMPLE_FMT_S16;

#ifdef DUMP_AUDIO_PCM
    m_audioRawDumpFile.reset(new std::ofstream("/tmp/audio_pcm_s16.pcm", std::ios::binary));
#endif

    if (avcodec_open2(audio_st->codec, m_audioDec , NULL) < 0) {
        ELOG_ERROR("Could not open audio decoder context");
        goto failed;
    }

    ELOG_TRACE("Audio dec sample_rate %d, channels %d, frame_size %d"
            , audio_st->codec->sample_rate
            , audio_st->codec->channels
            , audio_st->codec->frame_size
            );

    m_audioDecFrame = av_frame_alloc();
    if (!m_audioDecFrame) {
        ELOG_ERROR("Could not allocate audio dec frame");
        goto failed;
    }

    m_audioEnc = avcodec_find_encoder(AV_CODEC_ID_OPUS);
    if (!m_audioEnc) {
        ELOG_ERROR("Cound not fine audio encoder");
        goto failed;
    }

    m_audioEncCtx = avcodec_alloc_context3(m_audioEnc);
    if (!m_audioEncCtx ) {
        ELOG_ERROR("Could not alloc audio encoder context");
        goto failed;
    }

    m_audioEncCtx->channels         = 2;
    m_audioEncCtx->channel_layout   = av_get_default_channel_layout(2);
    m_audioEncCtx->sample_rate      = 48000;
    m_audioEncCtx->sample_fmt       = audio_st->codec->sample_fmt;
    m_audioEncCtx->bit_rate         = 64000;

    /* open it */
    if (avcodec_open2(m_audioEncCtx, m_audioEnc, NULL) < 0) {
        ELOG_ERROR("Could not open audio encoder context");
        goto failed;
    }

    ELOG_TRACE("Audio enc sample_rate %d, channels %d, frame_size %d"
            , m_audioEncCtx->sample_rate
            , m_audioEncCtx->channels
            , m_audioEncCtx->frame_size
            );

    if (audio_st->codec->sample_rate != m_audioEncCtx->sample_rate || audio_st->codec->channels != m_audioEncCtx->channels) {
        ELOG_TRACE("Init audio resampler");

        m_audioSwrCtx = swr_alloc();
        if (!m_audioSwrCtx) {
            ELOG_ERROR("Could not allocate resampler context");
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
            ELOG_ERROR("Failed to initialize the resampling context");
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
            ELOG_ERROR("Could not allocate swr samples data");
            goto failed;
        }
    }

    m_audioEncFifo = av_audio_fifo_alloc(m_audioEncCtx->sample_fmt, m_audioEncCtx->channels, 1);
    if (!m_audioEncFifo ) {
        ELOG_ERROR("Could not allocate audio enc fifo");
        goto failed;
    }

    m_audioEncFrame  = av_frame_alloc();
    if (!m_audioEncFrame) {
        ELOG_ERROR("Could not allocate audio enc frame");
        goto failed;
    }

    m_audioEncFrame->nb_samples        = m_audioEncCtx->frame_size;
    m_audioEncFrame->format            = m_audioEncCtx->sample_fmt;
    m_audioEncFrame->channel_layout    = m_audioEncCtx->channel_layout;
    m_audioEncFrame->sample_rate       = m_audioEncCtx->sample_rate;

    ret = av_frame_get_buffer(m_audioEncFrame, 0);
    if (ret < 0) {
        ELOG_ERROR("Could not get audio frame buffer");
        goto failed;
    }

    ELOG_TRACE("Init audio transcoder OK");
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

bool RtspIn::decAudioFrame() {
    AVStream *audio_st = m_context->streams[m_audioStreamIndex];
    int got;
    int len;
    int nSamples;
    int ret;

    if (!m_audioEncCtx) {
        ELOG_ERROR("Invalid transcode params");
        return false;
    }

    len = avcodec_decode_audio4(audio_st->codec, m_audioDecFrame, &got, &m_avPacket);
    if (len < 0) {
        ELOG_ERROR("Error while decoding");
        return false;
    }

    if (!got) {
        ELOG_TRACE("No decoded frame output");
        return false;
    }

    m_audioRcvSampleDts = m_avPacket.dts;
    m_audioRcvSampleCount += m_audioDecFrame->nb_samples;

    ELOG_TRACE("dts %d, nbSample %d", m_audioRcvSampleDts, m_audioRcvSampleCount);

    if (m_audioSwrCtx) {
        int dst_nb_samples;

        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(
                swr_get_delay(m_audioSwrCtx, audio_st->codec->sample_rate) + audio_st->codec->frame_size
                , m_audioEncCtx->sample_rate
                , audio_st->codec->sample_rate
                , AV_ROUND_UP);

        if (dst_nb_samples > m_audioSwrSamplesCount) {
            ELOG_TRACE("Realloc audio swr samples buffer");

            av_freep(&m_audioSwrSamplesData[0]);
            ret = av_samples_alloc(m_audioSwrSamplesData, &m_audioSwrSamplesLinesize, m_audioEncCtx->channels,
                dst_nb_samples, m_audioEncCtx->sample_fmt, 1);
            if (ret < 0) {
                ELOG_ERROR("Fail to realloc swr samples");
                return false;
            }
            m_audioSwrSamplesCount = dst_nb_samples;
        }

        /* convert to destination format */
        ret = swr_convert(m_audioSwrCtx, m_audioSwrSamplesData, dst_nb_samples, (const uint8_t **)m_audioDecFrame->data, m_audioDecFrame->nb_samples);
        if (ret < 0) {
            ELOG_ERROR("Error while converting");
            return false;
        }

        ELOG_TRACE("swr convert nbSample %d -> %d", m_audioDecFrame->nb_samples, ret);

#ifdef DUMP_AUDIO_PCM
        int bufsize = av_samples_get_buffer_size(&m_audioSwrSamplesLinesize, m_audioEncCtx->channels,
                ret, m_audioEncCtx->sample_fmt, 1);

        m_audioRawDumpFile->write((const char*)m_audioSwrSamplesData[0], bufsize);
#endif

        nSamples = av_audio_fifo_write(m_audioEncFifo, reinterpret_cast<void**>(m_audioSwrSamplesData), ret);
        if (nSamples < ret) {
            ELOG_ERROR("Can not write audio enc fifo, nbSamples %d, writed %d", ret, nSamples);
            return false;
        }
    }
    else {
        nSamples = av_audio_fifo_write(m_audioEncFifo, reinterpret_cast<void**>(&m_audioDecFrame->data), m_audioDecFrame->nb_samples);
        if (nSamples < m_audioDecFrame->nb_samples) {
            ELOG_ERROR("Can not write audio enc fifo, nbSamples %d, writed %d", m_audioDecFrame->nb_samples, nSamples);
            return false;
        }
    }

    return true;
}

bool RtspIn::encAudioFrame() {
    AVPacket pkt;
    int ret;
    int got;
    int nSamples;

    if (!m_audioEncCtx) {
        ELOG_ERROR("Invalid transcode params");
        return false;
    }

    if (av_audio_fifo_size(m_audioEncFifo) < m_audioEncCtx->frame_size) {
        return false;
    }

    nSamples = av_audio_fifo_read(m_audioEncFifo, reinterpret_cast<void**>(m_audioEncFrame->data), m_audioEncCtx->frame_size);
    if (nSamples < m_audioEncCtx->frame_size) {
        ELOG_ERROR("Can not read audio enc fifo, nbSamples %d, writed %d", m_audioEncCtx->frame_size, nSamples);
        return false;
    }

    av_init_packet(&pkt);
    pkt.data = NULL; // packet data will be allocated by the encoder
    pkt.size = 0;

    ret = avcodec_encode_audio2(m_audioEncCtx, &pkt, m_audioEncFrame, &got);
    if (ret < 0) {
        ELOG_ERROR("Fail to encode audio frame, ret %d", ret);
        return false;
    }

    if (!got) {
        ELOG_TRACE("Not get encoded audio frame");
        return false;
    }

    av_free_packet(&m_avPacket);
    av_copy_packet(&m_avPacket, &pkt);
    av_free_packet(&pkt);

    if (av_audio_fifo_size(m_audioEncFifo) >= m_audioEncCtx->frame_size) {
        ELOG_TRACE("More data in fifo to encode %d >= %d", av_audio_fifo_size(m_audioEncFifo), m_audioEncCtx->frame_size);
    }

    m_avPacket.dts = calibrateTimestamp(
            m_audioEncodedSampleCount
            , m_audioEncCtx->sample_rate
            , m_audioRcvSampleCount - m_audioDecFrame->nb_samples
            , m_context->streams[m_audioStreamIndex]->codec->sample_rate
            , m_audioRcvSampleDts
            , m_context->streams[m_audioStreamIndex]->time_base
            );

    ELOG_TRACE("Encoded audio frame dts %ld(%d), sample count %d(%d) "
            , m_avPacket.dts
            , m_audioRcvSampleDts
            , m_audioEncodedSampleCount
            , m_audioRcvSampleCount - m_audioDecFrame->nb_samples
            );

    m_audioEncodedSampleCount += m_audioEncFrame->nb_samples;
    return true;
}

}
