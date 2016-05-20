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

#include "MediaFileOut.h"
#include <rtputils.h>

extern "C" {
#include <libavutil/avstring.h>
#include <libavutil/channel_layout.h>
}

namespace woogeen_base {

DEFINE_LOGGER(MediaFileOut, "woogeen.media.MediaFileOut");

inline AVCodecID frameFormat2VideoCodecID(int frameFormat)
{
    switch (frameFormat) {
    case FRAME_FORMAT_VP8:
        return AV_CODEC_ID_VP8;
    case FRAME_FORMAT_H264:
        return AV_CODEC_ID_H264;
    default:
        return AV_CODEC_ID_VP8;
    }
}

inline AVCodecID frameFormat2AudioCodecID(int frameFormat)
{
    switch (frameFormat) {
    case FRAME_FORMAT_PCMU:
        return AV_CODEC_ID_PCM_MULAW;
    case FRAME_FORMAT_OPUS:
        return AV_CODEC_ID_OPUS;
    default:
        return AV_CODEC_ID_PCM_MULAW;
    }
}

MediaFileOut::MediaFileOut(const std::string& url, const AVOptions* audio, const AVOptions* video, int snapshotInterval, EventRegistry* handle)
    : m_videoStream(nullptr)
    , m_audioStream(nullptr)
    , m_context(nullptr)
    , m_recordPath(url)
    , m_snapshotInterval(snapshotInterval)
{
    m_videoQueue.reset(new MediaFrameQueue());
    m_audioQueue.reset(new MediaFrameQueue());
    setEventRegistry(handle);

    m_context = avformat_alloc_context();
    if (!m_context) {
        m_status = AVStreamOut::Context_CLOSED;
        notifyAsyncEvent("init", "cannot allocate context");
        ELOG_ERROR("avformat_alloc_context failed");
        return;
    }
    m_context->oformat = av_guess_format(nullptr, url.c_str(), nullptr);
    if (!m_context->oformat) {
        m_status = AVStreamOut::Context_CLOSED;
        notifyAsyncEvent("init", "cannot find proper output format");
        ELOG_ERROR("av_guess_format failed");
        avformat_free_context(m_context);
        return;
    }
    av_strlcpy(m_context->filename, url.c_str(), sizeof(m_context->filename));
    if (!init(audio, video)) {
        close();
        return;
    }
    m_jobTimer.reset(new woogeen_base::JobTimer(100, this));
    ELOG_DEBUG("created");
}

MediaFileOut::~MediaFileOut()
{
    close();
}

void MediaFileOut::close()
{
    if (m_status == AVStreamOut::Context_CLOSED)
        return;
    if (m_jobTimer)
        m_jobTimer->stop();
    if (m_status == AVStreamOut::Context_READY)
        av_write_trailer(m_context);
    if (m_videoStream && m_videoStream->codec)
        avcodec_close(m_videoStream->codec);
    if (m_audioStream && m_audioStream->codec)
        avcodec_close(m_audioStream->codec);
    if (m_context) {
        if (m_context->pb && !(m_context->oformat->flags & AVFMT_NOFILE))
            avio_close(m_context->pb);
        avformat_free_context(m_context);
        m_context = nullptr;
    }
    m_status = AVStreamOut::Context_CLOSED;
    ELOG_DEBUG("closed");
}

bool MediaFileOut::init(const AVOptions* audio, const AVOptions* video)
{
    enum AVCodecID codec_id = AV_CODEC_ID_NONE;
    if (video) {
        if (video->codec.compare("vp8") == 0) {
            codec_id = AV_CODEC_ID_VP8;
        } else if (video->codec.compare("h264") == 0) {
            codec_id = AV_CODEC_ID_H264;
        } else {
            notifyAsyncEvent("init", "invalid video codec");
            return false;
        }
        if (!addVideoStream(codec_id, video->spec.video.width, video->spec.video.height)) {
            notifyAsyncEvent("init", "cannot add video stream");
            return false;
        }
    }
    if (audio) {
        if (audio->codec.compare("pcmu") == 0) {
            codec_id = AV_CODEC_ID_PCM_MULAW;
        } else if (audio->codec.compare("opus_48000_2") == 0) {
            codec_id = AV_CODEC_ID_OPUS;
        } else {
            notifyAsyncEvent("init", "invalid audio codec");
            return false;
        }
        if (!addAudioStream(codec_id, audio->spec.audio.channels, audio->spec.audio.sampleRate)) {
            notifyAsyncEvent("init", "cannot add audio stream");
            return false;
        }
    }
    if (codec_id == AV_CODEC_ID_NONE) {
        notifyAsyncEvent("init", "no a/v options specified");
        return false;
    }

    if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_context->pb, m_context->filename, AVIO_FLAG_WRITE) < 0) {
            notifyAsyncEvent("init", "output file does not exist or cannot be opened for write");
            ELOG_ERROR("avio_open failed");
            return false;
        }
    }
    if (avformat_write_header(m_context, nullptr) < 0) {
        notifyAsyncEvent("init", "cannot write file header");
        ELOG_ERROR("avformat_write_header failed");
        return false;
    }
    av_dump_format(m_context, 0, m_context->filename, 1);
    m_status = AVStreamOut::Context_READY;
    notifyAsyncEvent("init", "");
    ELOG_DEBUG("context ready");
    deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });
    return true;
}

void MediaFileOut::onFrame(const Frame& frame)
{
    if (m_status != AVStreamOut::Context_READY)
        return;

    switch (frame.format) {
    case FRAME_FORMAT_VP8:
    case FRAME_FORMAT_H264:
        if (!m_videoStream)
            return;
        if (m_videoStream->codec->codec_id != frameFormat2VideoCodecID(frame.format)) {
            ELOG_ERROR("invalid video frame format");
            notifyAsyncEvent("fatal", "invalid video frame format");
            return close();
        }
        if (frame.additionalInfo.video.width != m_videoStream->codec->width || frame.additionalInfo.video.height != m_videoStream->codec->height) {
            ELOG_ERROR("invalid video frame resolution: %dx%d", frame.additionalInfo.video.width, frame.additionalInfo.video.height);
            notifyAsyncEvent("fatal", "invalid video frame resolution");
            return close();
        }
        m_videoQueue->pushFrame(frame.payload, frame.length);
        break;
    case FRAME_FORMAT_PCMU:
    case FRAME_FORMAT_OPUS: {
        if (!m_audioStream)
            return;
        if (m_audioStream->codec->codec_id != frameFormat2AudioCodecID(frame.format)) {
            ELOG_ERROR("invalid audio frame format");
            notifyAsyncEvent("fatal", "invalid audio frame format");
            return close();
        }
        if (frame.additionalInfo.audio.channels != m_audioStream->codec->channels || int(frame.additionalInfo.audio.sampleRate) != m_audioStream->codec->sample_rate) {
            ELOG_ERROR("invalid audio frame channels %d, or sample rate: %d", frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);
            notifyAsyncEvent("fatal", "invalid audio frame channels or sample rate");
            return close();
        }
        uint8_t* payload = frame.payload;
        uint32_t length = frame.length;
        if (frame.additionalInfo.audio.isRtpPacket) {
            RTPHeader* rtp = reinterpret_cast<RTPHeader*>(payload);
            uint32_t headerLength = rtp->getHeaderLength();
            assert(length >= headerLength);
            payload += headerLength;
            length -= headerLength;
        }
        m_audioQueue->pushFrame(payload, length);
        break;
    }
    default:
        ELOG_ERROR("unsupported frame format: %d", frame.format);
        notifyAsyncEvent("fatal", "unsupported frame format");
        return close();
    }
}

bool MediaFileOut::addAudioStream(enum AVCodecID codec_id, int nbChannels, int sampleRate)
{
    m_context->oformat->audio_codec = codec_id;
    AVStream* stream = avformat_new_stream(m_context, nullptr);
    if (!stream) {
        ELOG_ERROR("cannot add audio stream");
        return false;
    }
    AVCodecContext* c = stream->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_AUDIO;
    c->channels = nbChannels;
    c->channel_layout = av_get_default_channel_layout(nbChannels);
    c->sample_rate = sampleRate;
    if (codec_id == AV_CODEC_ID_OPUS)
        c->sample_fmt = AV_SAMPLE_FMT_FLT;
    else
        c->sample_fmt = AV_SAMPLE_FMT_S16;

    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    m_audioStream = stream;
    ELOG_DEBUG("audio stream added: %d channel(s), %d Hz, %s", nbChannels, sampleRate, (codec_id == AV_CODEC_ID_OPUS) ? "OPUS" : "PCMU");
    return true;
}

bool MediaFileOut::addVideoStream(enum AVCodecID codec_id, unsigned int width, unsigned int height)
{
    m_context->oformat->video_codec = codec_id;
    AVStream* stream = avformat_new_stream(m_context, nullptr);
    if (!stream) {
        ELOG_ERROR("cannot add video stream");
        return false;
    }
    AVCodecContext* c = stream->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_VIDEO;
    c->width = width;
    c->height = height;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    /* Some formats want stream headers to be separate. */
    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    m_context->oformat->flags |= AVFMT_VARIABLE_FPS;
    m_videoStream = stream;
    ELOG_DEBUG("video stream added: %dx%d, %s", width, height, (codec_id == AV_CODEC_ID_H264) ? "H264" : "VP8");
    return true;
}

void MediaFileOut::onTimeout()
{
    if (m_status != AVStreamOut::Context_READY)
        return;

    boost::shared_ptr<EncodedFrame> mediaFrame;
    while (mediaFrame = m_audioQueue->popFrame())
        this->writeAVFrame(m_audioStream, *mediaFrame);

    while (mediaFrame = m_videoQueue->popFrame())
        this->writeAVFrame(m_videoStream, *mediaFrame);
}

int MediaFileOut::writeAVFrame(AVStream* stream, const EncodedFrame& frame)
{
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = frame.m_payloadData;
    pkt.size = frame.m_payloadSize;
    pkt.pts = (int64_t)(frame.m_timeStamp / (av_q2d(stream->time_base) * 1000));
    pkt.stream_index = stream->index;
    if (frame.m_timeStamp - m_lastKeyFrameReqTime > KEYFRAME_REQ_INTERVAL) {
        m_lastKeyFrameReqTime = frame.m_timeStamp;
        deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });
    }
    return av_interleaved_write_frame(m_context, &pkt);
}

} // namespace woogeen_base
