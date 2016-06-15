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

static inline const char* getShortName(std::string& url)
{
    if (url.compare(0, 7, "rtsp://") == 0)
        return "rtsp";
    else if (url.compare(0, 7, "rtmp://") == 0)
        return "flv";
    return nullptr;
}

namespace woogeen_base {

DEFINE_LOGGER(RtspOut, "woogeen.RtspOut");

RtspOut::RtspOut(const std::string& url, const AVOptions* audio, const AVOptions* video, EventRegistry* handle)
    : AVStreamOut{ handle }
    , m_context{ nullptr }
    , m_resampleContext{ nullptr }
    , m_audioFifo{ nullptr }
    , m_videoStream{ nullptr }
    , m_audioStream{ nullptr }
    , m_uri{ url }
    , m_audioEncodingFrame{ nullptr }
    , m_hasAudio(false)
    , m_hasVideo(false)
    , m_state(IDLE)
    , m_StatDetectFrames(0)
    , m_ifmtCtx(nullptr)
    , m_inputVideoStream(nullptr)
    , m_inputVideoStartTimestamp(0)
    , m_inputVideoFrameIndex(0)
{
    m_videoQueue.reset(new woogeen_base::MediaFrameQueue());
    m_audioQueue.reset(new woogeen_base::MediaFrameQueue());
    m_jobTimer.reset(new woogeen_base::JobTimer(100, this));
#ifdef DUMP_RAW
    m_dumpFile.reset(new std::ofstream("/tmp/rtspAudioRaw.pcm", std::ios::binary));
#endif

    if (audio) {
        m_audio = *audio;
        m_hasAudio = true;
    }

    if (video) {
        m_video = *video;
        m_hasVideo = true;
    }

    ELOG_TRACE("hasAudio(%d), hasVideo(%d)", m_hasAudio, m_hasVideo);

    notifyAsyncEvent("init", "");
    ELOG_DEBUG("context ready");
    deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });
    m_worker = boost::thread(&RtspOut::run, this);
}

RtspOut::~RtspOut()
{
    close();
}

void RtspOut::close()
{
    m_state = IDLE;
    m_worker.join();

    if (m_status == AVStreamOut::Context_CLOSED)
        return;
    if (m_jobTimer)
        m_jobTimer->stop();
    if (m_audioEncodingFrame)
        av_frame_free(&m_audioEncodingFrame);
    if (m_audioFifo)
        av_audio_fifo_free(m_audioFifo);
    if (m_resampleContext) {
        swr_close(m_resampleContext);
        swr_free(&m_resampleContext);
    }
    if (m_audioStream)
        avcodec_close(m_audioStream->codec);
    if (m_status == AVStreamOut::Context_READY)
        av_write_trailer(m_context);
    if (m_context->pb && !(m_context->oformat->flags & AVFMT_NOFILE))
        avio_closep(&m_context->pb);
    avformat_free_context(m_context);

    av_freep(&m_ifmtCtx->pb->buffer);
    av_freep(&m_ifmtCtx->pb);
    avformat_close_input(&m_ifmtCtx);

    m_status = AVStreamOut::Context_CLOSED;
    ELOG_DEBUG("closed");
#ifdef DUMP_RAW
    m_dumpFile.reset();
#endif
}

int RtspOut::readFunction(void* opaque, uint8_t* buf, int buf_size)
{
    uint32_t len = 1;

    ELOG_TRACE("%s Enter", __FUNCTION__);

    RtspOut* This = static_cast<RtspOut*>(opaque);

    while (This->m_state == INITIALIZING || This->m_state == READY) {
        boost::shared_ptr<woogeen_base::EncodedFrame> frame;
        frame = This->m_videoQueue->popFrame();

        if (frame) {
            if (!This->m_inputVideoStartTimestamp)
                This->m_inputVideoStartTimestamp = frame->m_timeStamp;

            if (This->m_state == INITIALIZING)
                This->m_StatDetectFrames++;

            if (len + frame->m_payloadSize > (uint32_t)buf_size) {
                ELOG_ERROR("Read buf is full, discard encoded video frame");
                return len;
            }

            memcpy(buf, frame->m_payloadData, frame->m_payloadSize);
            len += frame->m_payloadSize;

            ELOG_TRACE("%s, read %d done", __FUNCTION__, len);
            return len;
        } else {
            boost::mutex::scoped_lock slock(This->m_readCbmutex);

            ELOG_TRACE("Wait for video frame...");
            //This->m_readCbcond.wait(slock);

            int timeout = 1;
            if (!This->m_readCbcond.timed_wait(slock, boost::get_system_time() + boost::posix_time::seconds(timeout))) {
                ELOG_ERROR("No video frame in last %d(s)...", timeout);
            } else {
                ELOG_TRACE("New video frame comes");
            }

            continue;
        }
    }

    // end of stream
    return 0;
}

bool RtspOut::detectInputVideoStream()
{
    int ret;
    int avioBufferSize;

    avioBufferSize = m_video.spec.video.width * m_video.spec.video.height * 3 / 2;

    m_ifmtCtx = avformat_alloc_context();
    m_ifmtCtx->pb = avio_alloc_context(reinterpret_cast<unsigned char*>(av_malloc(avioBufferSize + FF_INPUT_BUFFER_PADDING_SIZE)), avioBufferSize, 0, this, readFunction, nullptr, nullptr);
    m_ifmtCtx->max_analyze_duration2 = 1 * AV_TIME_BASE;

    // blocked
    if ((ret = avformat_open_input(&m_ifmtCtx, nullptr, 0, 0)) < 0) {
        ELOG_ERROR("Could not open input file");
        return false;
    }

    ELOG_DEBUG("Open input video stream after %d video frame", m_StatDetectFrames);

    if ((ret = avformat_find_stream_info(m_ifmtCtx, 0)) < 0) {
        ELOG_ERROR("Failed to retrieve input stream information");
        return false;
    }

    ELOG_DEBUG("Find input video stream after %d video frame", m_StatDetectFrames);

    if (m_ifmtCtx->nb_streams != 1 || m_ifmtCtx->streams[0]->codec->codec_type != AVMEDIA_TYPE_VIDEO) {
        ELOG_ERROR("Invalid input video stream");
        return false;
    }

    av_dump_format(m_ifmtCtx, 0, nullptr, 0);

    m_inputVideoStream = m_ifmtCtx->streams[0];
    if (!m_inputVideoStream)
        return false;

    return true;
}

void RtspOut::run()
{
    m_state = INITIALIZING;

    if (m_hasVideo && !detectInputVideoStream())
    {
        m_state = IDLE;
        close();
        return;
    }

    if (!init()) {
        m_state = IDLE;
        close();
        return;
    }

    m_state = READY;

    ELOG_DEBUG("initialized");

    if (!m_hasVideo) {
        ELOG_DEBUG("audio only streaming, worker thread exited!");
        return;
    }

    while (m_state == READY) {
        writeVideoPkt();
    }

    ELOG_DEBUG("worker thread exited!");
}

bool RtspOut::init()
{
    AVOptions* audio = NULL;
    AVOptions* video = NULL;

    if (m_hasAudio)
        audio = &m_audio;

    if (m_hasVideo)
        video = &m_video;

    avformat_alloc_output_context2(&m_context, nullptr, getShortName(m_uri), m_uri.c_str());
    if (!m_context) {
        m_status = AVStreamOut::Context_CLOSED;
        notifyAsyncEvent("init", "cannot allocate context");
        ELOG_ERROR("avformat_alloc_output_context2 failed");
        return false;
    }

    if (!audio || !video) {
        notifyAsyncEvent("init", "no a/v options specified");
        return false;
    }
    if (video->codec.compare("h264") != 0) {
        notifyAsyncEvent("init", "invalid video codec");
        return false;
    }
    if (audio->codec.compare("pcm_raw") != 0) {
        notifyAsyncEvent("init", "invalid audio codec");
        return false;
    }
    if (!addVideoStream(AV_CODEC_ID_H264, video->spec.video.width, video->spec.video.height))
        return false;
    if (!addAudioStream(AV_CODEC_ID_AAC, audio->spec.audio.channels, audio->spec.audio.sampleRate))
        return false;

    if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_context->pb, m_context->filename, AVIO_FLAG_WRITE) < 0) {
            notifyAsyncEvent("init", "cannot open connection");
            ELOG_ERROR("avio_open failed");
            return false;
        }
    }
    if (avformat_write_header(m_context, nullptr) < 0) {
        notifyAsyncEvent("init", "cannot write header");
        ELOG_ERROR("avformat_write_header failed");
        return false;
    }
    av_dump_format(m_context, 0, m_context->filename, 1);
    m_status = AVStreamOut::Context_READY;
    return true;
}

void RtspOut::onFrame(const woogeen_base::Frame& frame)
{
    switch (frame.format) {
    case woogeen_base::FRAME_FORMAT_H264:
        if (m_videoStream && (frame.additionalInfo.video.width != m_videoStream->codec->width || frame.additionalInfo.video.height != m_videoStream->codec->height)) {
            ELOG_ERROR("invalid video frame resolution: %dx%d", frame.additionalInfo.video.width, frame.additionalInfo.video.height);
            notifyAsyncEvent("fatal", "invalid video frame resolution");
            return close();
        }
        m_videoQueue->pushFrame(frame.payload, frame.length);
        m_readCbcond.notify_one();

        break;
    case woogeen_base::FRAME_FORMAT_PCM_RAW:
        if (m_status != AVStreamOut::Context_READY || !m_audioStream)
            return;
        if (frame.additionalInfo.audio.channels != m_audioStream->codec->channels || int(frame.additionalInfo.audio.sampleRate) != m_audioStream->codec->sample_rate) {
            ELOG_ERROR("invalid audio frame channels %d, or sample rate: %d", frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);
            notifyAsyncEvent("fatal", "invalid audio frame channels or sample rate");
            return close();
        }
        processAudio(frame.payload, frame.additionalInfo.audio.nbSamples);
        break;
    default:
        ELOG_ERROR("unsupported frame format: %d", frame.format);
        notifyAsyncEvent("fatal", "unsupported frame format");
        return close();
    }
}

int RtspOut::writeAudioFrame()
{
    int ret;

    encodeAudio();

    boost::shared_ptr<woogeen_base::EncodedFrame> frame = m_audioQueue->popFrame();
    if (!frame) {
        return false;
    }

    ELOG_TRACE("writeAudioFrame");

    AVPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    av_init_packet(&pkt);
    pkt.data = frame->m_payloadData;
    pkt.size = frame->m_payloadSize;
    pkt.pts = (int64_t)(frame->m_timeStamp / (av_q2d(m_audioStream->time_base) * 1000));
    pkt.stream_index = m_audioStream->index;
    if (frame->m_timeStamp - m_lastKeyFrameReqTime > KEYFRAME_REQ_INTERVAL) {
        m_lastKeyFrameReqTime = frame->m_timeStamp;
        deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });
    }
    ELOG_TRACE("audio_frame pts: %ld, size %d", pkt.pts, pkt.size);
    {
        boost::unique_lock<boost::shared_mutex> lock(m_writerMutex);
        ret = av_interleaved_write_frame(m_context, &pkt);
    }
    if (ret < 0) {
        ELOG_ERROR("Error muxing packet");

        av_free_packet(&pkt);
        return false;
    }

    return true;
}

void RtspOut::processAudio(uint8_t* data, int nbSamples)
{
#ifdef DUMP_RAW
    if (m_dumpFile)
        m_dumpFile->write(reinterpret_cast<char*>(data), nbSamples * sizeof(int16_t) * m_audioStream->codec->channels);
#endif
    if (!m_audioFifo)
        return;
#ifndef FORCE_RESAMPLE
    if (av_audio_fifo_realloc(m_audioFifo, av_audio_fifo_size(m_audioFifo) + nbSamples) < 0) {
        ELOG_ERROR("cannot reallocate fifo");
        return;
    }
    if (av_audio_fifo_write(m_audioFifo, reinterpret_cast<void**>(&data), nbSamples) < nbSamples) {
        ELOG_ERROR("cannot not write data to fifo");
    }
#else
    if (!m_resampleContext) {
        ELOG_ERROR("resample context not initialized");
        return;
    }
    uint8_t* converted_input_samples = nullptr;
    if (!(converted_input_samples = reinterpret_cast<uint8_t*>(calloc(m_audioStream->codec->channels, sizeof(converted_input_samples))))) {
        ELOG_ERROR("cannot allocate converted input sample pointers");
        return;
    }
    if (av_samples_alloc(&converted_input_samples, nullptr, m_audioStream->codec->channels, nbSamples, AV_SAMPLE_FMT_S16, 0) < 0) {
        ELOG_ERROR("cannot allocate converted input samples");
        goto done;
    }
    if (swr_convert(m_resampleContext, &converted_input_samples, nbSamples, const_cast<const uint8_t**>(&data), nbSamples) < 0) {
        ELOG_ERROR("cannot convert input samples");
        goto done;
    }
    if (av_audio_fifo_realloc(m_audioFifo, av_audio_fifo_size(m_audioFifo) + nbSamples) < 0) {
        ELOG_ERROR("cannot reallocate fifo");
        goto done;
    }
    if (av_audio_fifo_write(m_audioFifo, reinterpret_cast<void**>(&converted_input_samples), nbSamples) < nbSamples) {
        ELOG_ERROR("cannot not write data to fifo");
    }
done:
    av_freep(&converted_input_samples);
    free(converted_input_samples);
#endif
}

void RtspOut::encodeAudio()
{
    if (!m_audioStream)
        return;
    if (av_audio_fifo_size(m_audioFifo) < m_audioStream->codec->frame_size)
        return;

    if (!m_audioEncodingFrame) {
        m_audioEncodingFrame = allocAudioFrame(m_audioStream->codec);
        if (!m_audioEncodingFrame)
            return;
    }

    if (av_audio_fifo_read(m_audioFifo, reinterpret_cast<void**>(m_audioEncodingFrame->data), m_audioStream->codec->frame_size) < m_audioStream->codec->frame_size) {
        ELOG_ERROR("cannot read enough data from fifo");
        return;
    }
    AVPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    av_init_packet(&pkt);
    int data_present = 0;
    if (avcodec_encode_audio2(m_audioStream->codec, &pkt, m_audioEncodingFrame, &data_present) < 0) {
        ELOG_ERROR("cannot encode audio frame.");
        av_free_packet(&pkt);
        return;
    }
    if (data_present)
        m_audioQueue->pushFrame(pkt.data, pkt.size);

    av_free_packet(&pkt);
}

AVFrame* RtspOut::allocAudioFrame(AVCodecContext* c)
{
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        ELOG_ERROR("cannot allocate audio frame");
        return nullptr;
    }
    frame->nb_samples = c->frame_size;
    frame->format = AV_SAMPLE_FMT_S16;
    frame->channel_layout = c->channel_layout;
    frame->sample_rate = c->sample_rate;
    if (av_frame_get_buffer(frame, 0) < 0) {
        ELOG_ERROR("cannot allocate output frame samples");
        av_frame_free(&frame);
        return nullptr;
    }
    return frame;
}

int RtspOut::writeVideoPkt()
{
    int ret;
    AVPacket pkt;

    ret = av_read_frame(m_ifmtCtx, &pkt);
    if (ret < 0) {
        ELOG_ERROR("Error av_read_frame");
        return false;
    }

    //Duration between 2 frames (us)
    int64_t calc_duration = 33333 /*(double)AV_TIME_BASE / av_q2d(m_inputVideoStream->r_frame_rate)*/;
    int64_t timestamp = m_inputVideoStartTimestamp * 1000 + m_inputVideoFrameIndex * calc_duration;

    if (timestamp - m_lastKeyFrameReqTime > KEYFRAME_REQ_INTERVAL * 1000) {
        ELOG_TRACE("Request key frame!");

        m_lastKeyFrameReqTime = timestamp;
        deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });
    }

    if (pkt.pts != AV_NOPTS_VALUE)
        ELOG_WARN("Raw H264 bitstream PTS should be none!");

    AVRational time_base_in = m_inputVideoStream->time_base;

    ELOG_TRACE("start %ld, frame duration %ld", m_inputVideoStartTimestamp, calc_duration);

    pkt.pts = (double)(timestamp) / (double)(av_q2d(time_base_in) * AV_TIME_BASE);
    pkt.dts = pkt.pts;
    pkt.duration = (double)calc_duration / (double)(av_q2d(time_base_in) * AV_TIME_BASE);

    /* copy packet */
    pkt.pts = av_rescale_q_rnd(pkt.pts, time_base_in, m_videoStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.dts = av_rescale_q_rnd(pkt.dts, time_base_in, m_videoStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.duration = av_rescale_q(pkt.duration, time_base_in, m_videoStream->time_base);
    pkt.pos = -1;

    m_inputVideoFrameIndex++;

    ELOG_TRACE("video_frame pts: %ld, size: %d", pkt.pts, pkt.size);
    {
        boost::unique_lock<boost::shared_mutex> lock(m_writerMutex);
        ret = av_interleaved_write_frame(m_context, &pkt);
    }
    if (ret < 0) {
        ELOG_ERROR("Error muxing packet");

        av_free_packet(&pkt);
        return false;
    }

    av_free_packet(&pkt);
    return true;
}

void RtspOut::onTimeout()
{
    if (m_status != AVStreamOut::Context_READY)
        return;

    writeAudioFrame();
}

bool RtspOut::addAudioStream(enum AVCodecID codec_id, int nbChannels, int sampleRate)
{
    AVCodec* codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        ELOG_ERROR("cannot find audio encoder %d", codec_id);
        notifyAsyncEvent("init", "cannot find audio encoder");
        return false;
    }
    m_context->oformat->audio_codec = codec_id;
    AVStream* stream = avformat_new_stream(m_context, codec);
    if (!stream) {
        ELOG_ERROR("cannot add audio stream");
        notifyAsyncEvent("init", "cannot add audio stream");
        return false;
    }

    AVCodecContext* c = stream->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_AUDIO;
    c->channels = nbChannels;
    c->channel_layout = av_get_default_channel_layout(nbChannels);
    c->sample_rate = sampleRate;
    c->sample_fmt = AV_SAMPLE_FMT_S16;

    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    if (c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
        c->frame_size = 10000;
    if (avcodec_open2(c, codec, nullptr) < 0) {
        ELOG_ERROR("cannot open output audio codec");
        notifyAsyncEvent("init", "cannot open output audio codec");
        return false;
    }
#ifdef FORCE_RESAMPLE
    m_resampleContext = swr_alloc_set_opts(nullptr,
        av_get_default_channel_layout(nbChannels),
        AV_SAMPLE_FMT_S16,
        c->sample_rate,
        av_get_default_channel_layout(nbChannels),
        c->sample_fmt,
        c->sample_rate,
        0, nullptr);
    if (m_resampleContext) {
        if (swr_init(m_resampleContext) < 0) {
            ELOG_ERROR("cannot open audio resample context");
            notifyAsyncEvent("init", "cannot open audio resample context");
            swr_free(&m_resampleContext);
            m_resampleContext = nullptr;
            return false;
        }
    }
#endif
    if (!m_audioFifo) {
        if (!(m_audioFifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, c->channels, 1))) {
            ELOG_ERROR("cannot allocate audio fifo");
            notifyAsyncEvent("init", "cannot allocate audio fifo");
            return false;
        }
    }
    m_audioStream = stream;
    ELOG_DEBUG("audio stream added: %d channel(s), %d Hz", nbChannels, sampleRate);
    return true;
}

bool RtspOut::addVideoStream(enum AVCodecID codec_id, unsigned int width, unsigned int height)
{
    if (!m_inputVideoStream) {
        ELOG_ERROR("invalid input video stream!");

        return false;
    }

    if (codec_id != m_inputVideoStream->codec->codec_id || width != m_inputVideoStream->codec->width || height != m_inputVideoStream->codec->height) {
        ELOG_ERROR("cannot add video stream");
        notifyAsyncEvent("init", "cannot add video stream");
        return false;
    }

    m_videoStream = avformat_new_stream(m_context, m_inputVideoStream->codec->codec);
    if (!m_videoStream) {
        ELOG_ERROR("cannot add video stream");
        notifyAsyncEvent("init", "cannot add video stream");
        return false;
    }

    //Copy the settings of AVCodecContext
    if (avcodec_copy_context(m_videoStream->codec, m_inputVideoStream->codec) < 0) {
        ELOG_ERROR("Failed to copy context from input to output stream codec context");
        return false;
    }

    m_videoStream->codec->codec_tag = 0;
    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        m_videoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    ELOG_DEBUG("video stream added: %dx%d", width, height);
    return true;
}

} /* namespace mcu */
