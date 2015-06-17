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

#include "RTSPMuxer.h"
#include <rtputils.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avstring.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

#define STREAM_FRAME_RATE 30

namespace mcu {

DEFINE_LOGGER(RTSPMuxer, "mcu.media.RTSPMuxer");

RTSPMuxer::RTSPMuxer(const std::string& uri, woogeen_base::FrameDispatcher* video, woogeen_base::FrameDispatcher* audio)
    : m_videoSource(video)
    , m_audioSource(audio)
    , m_resampleContext(nullptr)
    , m_audioFifo(nullptr)
    , m_pts(0)
    , m_videoId(-1)
    , m_audioId(-1)
    , m_uri(uri)
{
    m_muxing = false;
    m_firstVideoTimestamp = -1;
    m_firstAudioTimestamp = -1;

    av_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_WARNING);

    m_context = avformat_alloc_context();
    assert (m_context);

    m_context->oformat = av_guess_format("rtsp", uri.c_str(), nullptr);
    assert (m_context->oformat);

    av_strlcpy(m_context->filename, uri.c_str(), sizeof(m_context->filename));
#ifdef DUMP_RAW
    m_dumpFile.reset(new std::ofstream("/tmp/rtspAudioRaw.pcm", std::ios::binary));
#endif
}

RTSPMuxer::~RTSPMuxer()
{
    if (m_muxing)
        stop();
#ifdef DUMP_RAW
    m_dumpFile.reset();
#endif
}

void RTSPMuxer::stop()
{
    m_muxing = false;
    m_thread.join();
    m_audioTransThread.join();

    m_videoSource->removeFrameConsumer(m_videoId);
    m_audioSource->removeFrameConsumer(m_audioId);
    if (m_audioFifo)
        av_audio_fifo_free(m_audioFifo);
    if (m_resampleContext) {
        avresample_close(m_resampleContext);
        avresample_free(&m_resampleContext);
    }
    av_write_trailer(m_context);
    avcodec_close(m_audioStream->codec);
    if (!(m_context->oformat->flags & AVFMT_NOFILE))
        avio_close(m_context->pb);
    avformat_free_context(m_context);
}

bool RTSPMuxer::start()
{
    m_videoQueue.reset(new woogeen_base::MediaFrameQueue(0));
    m_audioQueue.reset(new woogeen_base::MediaFrameQueue(0));
    m_audioRawQueue.reset(new woogeen_base::MediaFrameQueue(0));
    m_videoId = m_videoSource->addFrameConsumer(m_uri, H264_90000_PT, this);
    m_audioId = m_audioSource->addFrameConsumer(m_uri, OPUS_48000_PT, this); // FIXME: should be AAC_44100_PT or so.
    if (m_videoId == -1 || m_audioId == -1)
        return false;

    addVideoStream(AV_CODEC_ID_H264);
    addAudioStream(AV_CODEC_ID_AAC);
    if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_context->pb, m_context->filename, AVIO_FLAG_WRITE) < 0) {
            ELOG_ERROR("open output file failed");
            return false;
        }
    }
    av_dump_format(m_context, 0, m_context->filename, 1);
    if (avformat_write_header(m_context, nullptr) < 0) {
        return false;
    }
    m_muxing = true;
    m_thread = boost::thread(&RTSPMuxer::loop, this);
    m_audioTransThread = boost::thread(&RTSPMuxer::encodeAudioLoop, this);
    return true;
}

void RTSPMuxer::onFrame(const woogeen_base::Frame& frame)
{
    switch (frame.format) {
    case woogeen_base::FRAME_FORMAT_H264:
        m_videoQueue->pushFrame(frame.payload, frame.length, frame.timeStamp);
        break;
    case woogeen_base::FRAME_FORMAT_PCM_RAW:
        // TODO: Get rid of the raw audio queue. The data should be pushed into the av audio fifo directly.
        m_audioRawQueue->pushFrame(frame.payload, frame.length, frame.timeStamp);
        break;
    default:
        break;
    }
}

int RTSPMuxer::writeVideoFrame(uint8_t* data, size_t size, int timestamp)
{
    if (m_firstVideoTimestamp == -1)
        m_firstVideoTimestamp = timestamp;
    timestamp -= m_firstVideoTimestamp;
    if (timestamp < 0)
        timestamp += 0xFFFFFFFF;
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    pkt.data = data;
    pkt.size = size;
    pkt.pts = timestamp;
    pkt.stream_index = m_videoStream->index;
    return av_interleaved_write_frame(m_context, &pkt);
}

void RTSPMuxer::processAudio(uint8_t* data, int nbSamples, int sampleRate, bool isStereo)
{
#ifdef DUMP_RAW
    if (m_dumpFile)
        m_dumpFile->write(reinterpret_cast<char*>(data), nbSamples * sizeof(int16_t) * (isStereo ? 2 : 1));
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
    int channels = isStereo ? 2 : 1;
    uint8_t* converted_input_samples = nullptr;
    if (!(converted_input_samples = reinterpret_cast<uint8_t*>(calloc(channels, sizeof(converted_input_samples))))) {
        ELOG_ERROR("cannot allocate converted input sample pointers");
        return;
    }
    if (av_samples_alloc(&converted_input_samples, nullptr,
                            channels, nbSamples, AV_SAMPLE_FMT_S16, 0) < 0) {
        ELOG_ERROR("cannot allocate converted input samples");
        goto done;
    }
    if (avresample_convert(m_resampleContext, &converted_input_samples, 0,
                            nbSamples, reinterpret_cast<uint8_t**>(&data), 0, nbSamples) < 0) {
        ELOG_ERROR("cannot convert input samples");
        goto done;
    }
    if (avresample_available(m_resampleContext)) {
        ELOG_WARN("converted samples left over");
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

void RTSPMuxer::encodeAudioLoop() {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        ELOG_ERROR("allocate audio frame failed");
        return;
    }
    int frame_size = m_audioStream->codec->frame_size;
    frame->nb_samples     = frame_size;
    frame->format         = AV_SAMPLE_FMT_S16;
    frame->channel_layout = m_audioStream->codec->channel_layout;
    frame->sample_rate    = m_audioStream->codec->sample_rate;
    if (av_frame_get_buffer(frame, 0) < 0) {
        ELOG_ERROR("cannot allocate output frame samples");
        av_frame_free(&frame);
        return;
    }
    boost::shared_ptr<woogeen_base::EncodedFrame> audioSrc;
    while (m_muxing) {
        while (av_audio_fifo_size(m_audioFifo) < frame_size) {
            audioSrc = m_audioRawQueue->popFrame();
            if (!audioSrc) {
                usleep(1000);
                continue;
            }
            processAudio(audioSrc->m_payloadData, audioSrc->m_payloadSize/2/sizeof(int16_t));
        }
        if (av_audio_fifo_read(m_audioFifo, reinterpret_cast<void**>(frame->data), frame_size) < frame_size) {
            ELOG_ERROR("cannot read enough data from fifo");
            continue;
        }
        AVPacket pkt = { 0 };
        av_init_packet(&pkt);
        int data_present = 0;
        frame->pts = m_pts;
        m_pts += frame->nb_samples;
        if (avcodec_encode_audio2(m_audioStream->codec, &pkt, frame, &data_present) < 0) {
            ELOG_ERROR("cannot encode audio frame.");
            av_free_packet(&pkt);
            continue;
        }
        if (data_present) {
            av_packet_rescale_ts(&pkt, m_audioStream->codec->time_base, m_audioStream->time_base);
            m_audioQueue->pushFrame(pkt.data, pkt.size, pkt.pts);
        }
        av_free_packet(&pkt);
    }
    av_frame_free(&frame);
}


AVFrame* RTSPMuxer::allocAudioFrame()
{
    AVFrame* frame = av_frame_alloc();
    frame->nb_samples     = m_audioStream->codec->frame_size;
    frame->channel_layout = m_audioStream->codec->channel_layout;
    frame->format         = m_audioStream->codec->sample_fmt;
    frame->sample_rate    = m_audioStream->codec->sample_rate;
    if (av_frame_get_buffer(frame, 0) < 0) {
        ELOG_ERROR("allocate audio frame failed");
        av_frame_free(&frame);
        return nullptr;
    }
    return frame;
}

int RTSPMuxer::writeAudioFrame(uint8_t* data, size_t size, int timestamp)
{
    if (m_firstAudioTimestamp == -1)
        m_firstAudioTimestamp = timestamp;
    timestamp -= m_firstAudioTimestamp;
    if (timestamp < 0)
        timestamp += 0xFFFFFFFF;
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    pkt.data = data;
    pkt.size = size;
    pkt.pts = timestamp;
    pkt.stream_index = m_audioStream->index;
    return av_interleaved_write_frame(m_context, &pkt);
}

void RTSPMuxer::loop()
{
    boost::shared_ptr<woogeen_base::EncodedFrame> video;
    boost::shared_ptr<woogeen_base::EncodedFrame> audio;

    while (m_muxing) {
        if (!audio)
            audio = m_audioQueue->popFrame();
        if (!video)
            video = m_videoQueue->popFrame();
        if (!audio && !video)
            continue;
        else if (!audio) {
            writeVideoFrame(video->m_payloadData, video->m_payloadSize, video->m_timeStamp);
            video.reset();
        } else if (!video) {
            writeAudioFrame(audio->m_payloadData, audio->m_payloadSize, audio->m_timeStamp);
            audio.reset();
        } else {
            if (audio->m_offsetMsec > video->m_offsetMsec) {
                writeVideoFrame(video->m_payloadData, video->m_payloadSize, video->m_timeStamp);
                video.reset();
            } else {
                writeAudioFrame(audio->m_payloadData, audio->m_payloadSize, audio->m_timeStamp);
                audio.reset();
            }
        }
    }
}

void RTSPMuxer::addAudioStream(enum AVCodecID codec_id)
{
    AVCodec* codec = avcodec_find_encoder(codec_id);
    assert (codec);
    m_audioStream = avformat_new_stream(m_context, codec);
    assert (m_audioStream);

    AVCodecContext* c = m_audioStream->codec;
    c->codec_id       = codec_id;
    c->codec_type     = AVMEDIA_TYPE_AUDIO;
    c->channels       = 2;
    c->channel_layout = av_get_default_channel_layout(c->channels);
    c->sample_rate    = 48000;
    c->sample_fmt     = AV_SAMPLE_FMT_S16;
    m_audioStream->time_base = (AVRational){ 1, c->sample_rate };

    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    if (c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
        c->frame_size = 10000;
    if (avcodec_open2(c, codec, nullptr) < 0)
        ELOG_ERROR("cannot open output codec.");
#ifdef FORCE_RESAMPLE
    m_resampleContext = avresample_alloc_context();
    assert (m_resampleContext);
    av_opt_set_int(m_resampleContext, "in_channel_layout",
                   av_get_default_channel_layout(2), 0);
    av_opt_set_int(m_resampleContext, "out_channel_layout",
                   av_get_default_channel_layout(2), 0);
    av_opt_set_int(m_resampleContext, "in_sample_rate",
                   48000, 0);
    av_opt_set_int(m_resampleContext, "out_sample_rate",
                   c->sample_rate, 0);
    av_opt_set_int(m_resampleContext, "in_sample_fmt",
                   AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(m_resampleContext, "out_sample_fmt",
                   AV_SAMPLE_FMT_S16, 0);
    if (avresample_open(m_resampleContext) < 0) {
        ELOG_ERROR("cannot open resample context");
        avresample_free(&m_resampleContext);
        m_resampleContext = nullptr;
    }
#endif
    if (!m_audioFifo) {
        if (!(m_audioFifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, c->channels, 1))) {
            ELOG_ERROR("cannot allocate audio fifo");
        }
    }
}

void RTSPMuxer::addVideoStream(enum AVCodecID codec_id)
{
    m_context->oformat->video_codec = codec_id;
    m_videoStream = avformat_new_stream(m_context, nullptr);
    assert (m_videoStream);

    AVCodecContext* c = m_videoStream->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_VIDEO;
    /* Put sample parameters. */
    c->bit_rate = 400000;
    /* Resolution must be a multiple of two. */
    unsigned int width = 640, height = 480;
    m_videoSource->getVideoSize(width, height);
    c->width    = width;
    c->height   = height;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
    * of which frame timestamps are represented. For fixed-fps content,
    * timebase should be 1/framerate and timestamp increments should be
    * identical to 1. */
    m_videoStream->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
    c->time_base             = m_videoStream->time_base;

    c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt       = AV_PIX_FMT_YUV420P;
    /* Some formats want stream headers to be separate. */
    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
}

} /* namespace mcu */
