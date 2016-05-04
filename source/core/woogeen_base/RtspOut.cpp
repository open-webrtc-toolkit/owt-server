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
}

namespace woogeen_base {

DEFINE_LOGGER(RtspOut, "woogeen.media.RtspOut");

RtspOut::RtspOut(const std::string& url)
    : m_context(nullptr)
    , m_resampleContext(nullptr)
    , m_audioFifo(nullptr)
    , m_videoStream(nullptr)
    , m_audioStream(nullptr)
    , m_videoId(-1)
    , m_audioId(-1)
    , m_uri(url)
    , m_audioEncodingFrame(nullptr)
{
    m_videoQueue.reset(new woogeen_base::MediaFrameQueue());
    m_audioQueue.reset(new woogeen_base::MediaFrameQueue());

    m_context = avformat_alloc_context();
    assert(m_context);

    m_context->oformat = av_guess_format("rtsp", m_uri.c_str(), nullptr);
    assert(m_context->oformat);

    av_strlcpy(m_context->filename, m_uri.c_str(), sizeof(m_context->filename));
#ifdef DUMP_RAW
    m_dumpFile.reset(new std::ofstream("/tmp/rtspAudioRaw.pcm", std::ios::binary));
#endif
    m_jobTimer.reset(new woogeen_base::JobTimer(100, this));
    ELOG_DEBUG("created");
}

RtspOut::~RtspOut()
{
    close();
#ifdef DUMP_RAW
    m_dumpFile.reset();
#endif
}

void RtspOut::close()
{
    m_jobTimer->stop();
    if (m_audioEncodingFrame)
        av_frame_free(&m_audioEncodingFrame);
    if (m_audioFifo)
        av_audio_fifo_free(m_audioFifo);
    if (m_resampleContext) {
        swr_close(m_resampleContext);
        swr_free(&m_resampleContext);
    }
    av_write_trailer(m_context);
    avcodec_close(m_audioStream->codec);
    if (!(m_context->oformat->flags & AVFMT_NOFILE))
        avio_close(m_context->pb);
    avformat_free_context(m_context);
    ELOG_DEBUG("closed");
}

void RtspOut::onFrame(const woogeen_base::Frame& frame)
{
    if (m_status == woogeen_base::AVStreamOut::Context_ERROR)
        return;
    switch (frame.format) {
    case woogeen_base::FRAME_FORMAT_H264:
        if (!m_videoStream) {
            FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
            deliverFeedbackMsg(msg);

            if (addVideoStream(AV_CODEC_ID_H264, frame.additionalInfo.video.width, frame.additionalInfo.video.height)) {
                ELOG_DEBUG("video stream added: %dx%d", frame.additionalInfo.video.width, frame.additionalInfo.video.height);
            }
        }
        m_videoQueue->pushFrame(frame.payload, frame.length);
        break;
    case woogeen_base::FRAME_FORMAT_PCM_RAW:
        if (m_videoStream && !m_audioStream) { // make sure video stream is added first.
            if (addAudioStream(AV_CODEC_ID_AAC, frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate)) {
                ELOG_DEBUG("audio stream added: %d channel(s), %d Hz", frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);
            }
        }
        processAudio(frame.payload,
                    frame.additionalInfo.audio.nbSamples,
                    frame.additionalInfo.audio.channels,
                    frame.additionalInfo.audio.sampleRate);
        break;
    default:
        break;
    }
}

int RtspOut::writeVideoFrame(uint8_t* data, size_t size, int64_t timestamp)
{
    AVPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    av_init_packet(&pkt);
    pkt.data = data;
    pkt.size = size;
    pkt.pts = timestamp;
    pkt.stream_index = m_videoStream->index;
    return av_interleaved_write_frame(m_context, &pkt);
}

void RtspOut::processAudio(uint8_t* data, int nbSamples, int nbChannels, int sampleRate)
{
#ifdef DUMP_RAW
    if (m_dumpFile)
        m_dumpFile->write(reinterpret_cast<char*>(data), nbSamples * sizeof(int16_t) * nbChannels);
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
    if (!(converted_input_samples = reinterpret_cast<uint8_t*>(calloc(nbChannels, sizeof(converted_input_samples))))) {
        ELOG_ERROR("cannot allocate converted input sample pointers");
        return;
    }
    if (av_samples_alloc(&converted_input_samples, nullptr,
                            nbChannels, nbSamples, AV_SAMPLE_FMT_S16, 0) < 0) {
        ELOG_ERROR("cannot allocate converted input samples");
        goto done;
    }
    if (swr_convert(m_resampleContext, &converted_input_samples, nbSamples,
                    const_cast<const uint8_t**>(&data), nbSamples) < 0) {
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

void RtspOut::encodeAudio() {
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
    frame->nb_samples     = c->frame_size;
    frame->format         = AV_SAMPLE_FMT_S16;
    frame->channel_layout = c->channel_layout;
    frame->sample_rate    = c->sample_rate;
    if (av_frame_get_buffer(frame, 0) < 0) {
        ELOG_ERROR("cannot allocate output frame samples");
        av_frame_free(&frame);
        return nullptr;
    }
    return frame;
}

int RtspOut::writeAudioFrame(uint8_t* data, size_t size, int64_t timestamp)
{
    AVPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    av_init_packet(&pkt);
    pkt.data = data;
    pkt.size = size;
    pkt.pts = timestamp;
    pkt.stream_index = m_audioStream->index;
    return av_interleaved_write_frame(m_context, &pkt);
}

void RtspOut::onTimeout()
{
    switch (m_status) {
    case woogeen_base::AVStreamOut::Context_EMPTY:
        if (m_audioStream && m_videoStream) {
            if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
                if (avio_open(&m_context->pb, m_context->filename, AVIO_FLAG_WRITE) < 0) {
                    ELOG_ERROR("avio_open failed");
                    m_status = AVStreamOut::Context_ERROR;
                    notifyAsyncEvent("RTSPStream", "cannot open output file");
                    return;
                }
            }
            av_dump_format(m_context, 0, m_context->filename, 1);
            if (avformat_write_header(m_context, nullptr) < 0) {
                m_status = AVStreamOut::Context_ERROR;
                notifyAsyncEvent("RTSPStream", "cannot setup connection");
                ELOG_ERROR("avformat_write_header failed");
                return;
            }
            m_status = woogeen_base::AVStreamOut::Context_READY;
            ELOG_DEBUG("context ready");
        } else
            return;
        break;
    case woogeen_base::AVStreamOut::Context_READY:
        break;
    case woogeen_base::AVStreamOut::Context_ERROR:
    default:
        notifyAsyncEvent("RTSPStream", "init failed");
        ELOG_ERROR("context error");
        return;
    }

    encodeAudio();

    boost::shared_ptr<woogeen_base::EncodedFrame> audioFrame = m_audioQueue->popFrame();
    boost::shared_ptr<woogeen_base::EncodedFrame> videoFrame = m_videoQueue->popFrame();
    if (audioFrame) {
        writeAudioFrame(audioFrame->m_payloadData, audioFrame->m_payloadSize, (int64_t)(audioFrame->m_timeStamp / (av_q2d(m_audioStream->time_base) * 1000)));
    }

    if (videoFrame)
        writeVideoFrame(videoFrame->m_payloadData, videoFrame->m_payloadSize, (int64_t)(videoFrame->m_timeStamp / (av_q2d(m_videoStream->time_base) * 1000)));
}

bool RtspOut::addAudioStream(enum AVCodecID codec_id, int nbChannels, int sampleRate)
{
    boost::lock_guard<boost::mutex> lock(m_contextMutex);
    AVCodec* codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        ELOG_ERROR("cannot find audio encoder %d", codec_id);
        m_status = AVStreamOut::Context_ERROR;
        return false;
    }
    AVStream* stream = avformat_new_stream(m_context, codec);
    if (!stream) {
        ELOG_ERROR("cannot add audio stream");
        m_status = AVStreamOut::Context_ERROR;
        return false;
    }

    AVCodecContext* c = stream->codec;
    c->codec_id       = codec_id;
    c->codec_type     = AVMEDIA_TYPE_AUDIO;
    c->channels       = nbChannels;
    c->channel_layout = av_get_default_channel_layout(nbChannels);
    c->sample_rate    = sampleRate;
    c->sample_fmt     = AV_SAMPLE_FMT_S16;
    c->time_base      = (AVRational){ 1, c->sample_rate };

    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    if (c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
        c->frame_size = 10000;
    if (avcodec_open2(c, codec, nullptr) < 0)
        ELOG_ERROR("cannot open output codec.");
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
            ELOG_ERROR("cannot open resample context");
            swr_free(&m_resampleContext);
            m_resampleContext = nullptr;
        }
    }
#endif
    if (!m_audioFifo) {
        if (!(m_audioFifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, c->channels, 1))) {
            ELOG_ERROR("cannot allocate audio fifo");
        }
    }
    m_audioStream = stream;

    return true;
}

bool RtspOut::addVideoStream(enum AVCodecID codec_id, unsigned int width, unsigned int height)
{
    boost::lock_guard<boost::mutex> lock(m_contextMutex);
    m_context->oformat->video_codec = codec_id;
    AVStream* stream = avformat_new_stream(m_context, nullptr);
    if (!stream) {
        ELOG_ERROR("cannot add video stream");
        m_status = AVStreamOut::Context_ERROR;
        return false;
    }
    AVCodecContext* c = stream->codec;
    c->codec_id   = codec_id;
    c->codec_type = AVMEDIA_TYPE_VIDEO;
    /* Resolution must be a multiple of two. */
    c->width    = width;
    c->height   = height;
    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt  = AV_PIX_FMT_YUV420P;
    /* Some formats want stream headers to be separate. */
    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    m_videoStream = stream;

    return true;
}

} /* namespace mcu */
