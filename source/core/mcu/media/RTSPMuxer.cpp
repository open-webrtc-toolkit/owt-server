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

extern "C" {
#include <libavutil/avstring.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
}

#define STREAM_FRAME_RATE 30
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P

namespace mcu {

DEFINE_LOGGER(RTSPMuxer, "mcu.media.RTSPMuxer");

RTSPMuxer::RTSPMuxer(const std::string& uri, woogeen_base::MediaStreaming* video, woogeen_base::MediaStreaming* audio)
    : m_videoSink(video)
    , m_audioSink(audio)
{
    m_muxing = false;
    m_firstVideoTimestamp = -1;
    m_firstAudioTimestamp = -1;

    av_register_all();
    avformat_network_init();
    // av_log_set_level(AV_LOG_DEBUG);

    m_context = avformat_alloc_context();
    assert (m_context);

    m_context->oformat = av_guess_format("rtsp", uri.c_str(), nullptr);
    assert (m_context->oformat);

    av_strlcpy(m_context->filename, uri.c_str(), sizeof(m_context->filename));
}

RTSPMuxer::~RTSPMuxer()
{
    if (m_muxing)
        stop();
}

void RTSPMuxer::stop()
{
    m_muxing = false;
    m_thread.join();

    m_videoSink->stopStreaming();
    m_audioSink->stopStreaming();

    av_write_trailer(m_context);
    avcodec_close(m_videoStream->codec);
    if (!(m_context->oformat->flags & AVFMT_NOFILE))
        avio_close(m_context->pb);
    avformat_free_context(m_context);
}

bool RTSPMuxer::start()
{
    addVideoStream(AV_CODEC_ID_H264);
    addAudioStream(AV_CODEC_ID_PCM_MULAW);
    if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_context->pb, m_context->filename, AVIO_FLAG_WRITE) < 0) {
            ELOG_ERROR("open output file failed");
            return false;
        }
    }
    // av_dump_format(m_context, 0, m_context->filename, 1);
    avformat_write_header(m_context, nullptr);

    m_videoQueue.reset(new woogeen_base::MediaFrameQueue(0));
    m_audioQueue.reset(new woogeen_base::MediaFrameQueue(0));
    m_videoSink->startStreaming(*m_videoQueue);
    m_audioSink->startStreaming(*m_audioQueue);

    m_muxing = true;
    m_thread = boost::thread(&RTSPMuxer::loop, this);
    return true;
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
    c->channels       = 1;
    c->channel_layout = av_get_default_channel_layout(c->channels);
    c->sample_rate    = 8000;
    c->sample_fmt     = AV_SAMPLE_FMT_S16;
    c->bit_rate       = 48000;
    m_audioStream->time_base = (AVRational){ 1, c->sample_rate };

    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
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
    c->width    = 640;
    c->height   = 480;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
    * of which frame timestamps are represented. For fixed-fps content,
    * timebase should be 1/framerate and timestamp increments should be
    * identical to 1. */
    m_videoStream->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
    c->time_base             = m_videoStream->time_base;

    c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt       = STREAM_PIX_FMT;
    /* Some formats want stream headers to be separate. */
    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
}

} /* namespace mcu */
