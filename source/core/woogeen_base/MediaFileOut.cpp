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
    : m_expectedVideo(AV_CODEC_ID_NONE)
    , m_videoStream(nullptr)
    , m_expectedAudio(AV_CODEC_ID_NONE)
    , m_audioStream(nullptr)
    , m_context(nullptr)
    , m_recordPath(url)
    , m_snapshotInterval(snapshotInterval)
    , m_videoWidth(0)
    , m_videoHeight(0)
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
    m_jobTimer.reset(new JobTimer(100, this));
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
    if (video) {
        if (video->codec.compare("vp8") == 0) {
            m_expectedVideo = AV_CODEC_ID_VP8;
        } else if (video->codec.compare("h264") == 0) {
            m_expectedVideo = AV_CODEC_ID_H264;
        } else {
            notifyAsyncEvent("init", "invalid video codec");
            return false;
        }
        ELOG_DEBUG("expected video codec:(%d, %s)", m_expectedVideo, video->codec.c_str());
    }
    if (audio) {
        if (audio->codec.compare("pcmu") == 0) {
            m_expectedAudio = AV_CODEC_ID_PCM_MULAW;
        } else if (audio->codec.compare("opus_48000_2") == 0) {
            m_expectedAudio = AV_CODEC_ID_OPUS;
        } else {
            notifyAsyncEvent("init", "invalid audio codec");
            return false;
        }
        ELOG_DEBUG("expected audio codec:(%d, %s)", m_expectedAudio, audio->codec.c_str());
    }

    if (m_expectedVideo == AV_CODEC_ID_NONE && m_expectedAudio == AV_CODEC_ID_NONE) {
        notifyAsyncEvent("init", "no a/v options specified");
        return false;
    }

    m_status = AVStreamOut::Context_INITIALIZING;
    notifyAsyncEvent("init", "");
    deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });
    return true;
}

void MediaFileOut::onFrame(const Frame& frame)
{
    if (m_status == AVStreamOut::Context_EMPTY || m_status == AVStreamOut::Context_CLOSED) {
        return;
    }

    switch (frame.format) {
    case FRAME_FORMAT_VP8:
    case FRAME_FORMAT_H264:
        if (m_expectedVideo == frameFormat2VideoCodecID(frame.format)) {
            bool addStreamOK = true;
            if (!m_videoStream) {
                if (frame.additionalInfo.video.width != 0 && frame.additionalInfo.video.height != 0) {
                    addStreamOK = addVideoStream(m_expectedVideo, frame.additionalInfo.video.width, frame.additionalInfo.video.height);

                    m_videoWidth = frame.additionalInfo.video.width;
                    m_videoHeight = frame.additionalInfo.video.height;
                } else {
                    return;
                }
            } else if (frame.additionalInfo.video.width != m_videoWidth || frame.additionalInfo.video.height != m_videoHeight) {
                ELOG_DEBUG("video resolution changed: %dx%d -> %dx%d"
                        , m_videoWidth, m_videoHeight
                        , frame.additionalInfo.video.width, frame.additionalInfo.video.height
                        );

                m_videoWidth = frame.additionalInfo.video.width;
                m_videoHeight = frame.additionalInfo.video.height;
            }

            if (addStreamOK && m_status == AVStreamOut::Context_READY) {
                m_videoQueue->pushFrame(frame.payload, frame.length);
            }
        } else if (m_expectedVideo != AV_CODEC_ID_NONE){
            ELOG_ERROR("invalid video frame format");
            notifyAsyncEvent("fatal", "invalid video frame format");
            return close();
        }
        break;
    case FRAME_FORMAT_PCMU:
    case FRAME_FORMAT_OPUS: {
        if (m_expectedAudio == frameFormat2AudioCodecID(frame.format)) {
            bool addStreamOK = true;
            if (!m_audioStream) {
                addStreamOK = addAudioStream(m_expectedAudio, frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);
            } else if (frame.additionalInfo.audio.channels != m_audioStream->codec->channels || int(frame.additionalInfo.audio.sampleRate) != m_audioStream->codec->sample_rate) {
                ELOG_ERROR("invalid audio frame channels %d, or sample rate: %d", frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);
                notifyAsyncEvent("fatal", "invalid audio frame channels or sample rate");
                return close();
            }

            if (addStreamOK && m_status == AVStreamOut::Context_READY) {
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
            }
        } else if (m_expectedAudio != AV_CODEC_ID_NONE) {
            ELOG_ERROR("invalid audio frame format");
            notifyAsyncEvent("fatal", "invalid audio frame format");
            return close();
        }
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
        notifyAsyncEvent("fatal", "cannot add audio stream");
        close();
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

    if ((m_expectedVideo == AV_CODEC_ID_NONE) || m_videoStream) {
        return getReady();
    }
    return true;
}

bool MediaFileOut::addVideoStream(enum AVCodecID codec_id, unsigned int width, unsigned int height)
{
    m_context->oformat->video_codec = codec_id;
    AVStream* stream = avformat_new_stream(m_context, nullptr);
    if (!stream) {
        notifyAsyncEvent("fatal", "cannot add audio stream");
        close();
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

    if ((m_expectedAudio == AV_CODEC_ID_NONE) || m_audioStream) {
        return getReady();
    }
    return true;
}

bool MediaFileOut::getReady()
{
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
    ELOG_DEBUG("context ready");
    deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME });
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
