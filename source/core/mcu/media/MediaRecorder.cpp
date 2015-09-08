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

#include "MediaRecorder.h"
#include <rtputils.h>

extern "C" {
#include <libavutil/channel_layout.h>
}

namespace mcu {

DEFINE_LOGGER(MediaRecorder, "mcu.media.MediaRecorder");

inline AVCodecID payloadType2VideoCodecID(int payloadType)
{
    switch (payloadType) {
        case VP8_90000_PT: return AV_CODEC_ID_VP8;
        case H264_90000_PT: return AV_CODEC_ID_H264;
        default: return AV_CODEC_ID_VP8;
    }
}

inline AVCodecID payloadType2AudioCodecID(int payloadType)
{
    switch (payloadType) {
        case PCMU_8000_PT: return AV_CODEC_ID_PCM_MULAW;
        case OPUS_48000_PT: return AV_CODEC_ID_OPUS;
        default: return AV_CODEC_ID_PCM_MULAW;
    }
}

MediaRecorder::MediaRecorder(const std::string& recordUrl, int snapshotInterval, woogeen_base::EventRegistry* cb)
    : woogeen_base::MediaMuxer(cb)
    , m_videoSource(nullptr)
    , m_audioSource(nullptr)
    , m_videoStream(nullptr)
    , m_audioStream(nullptr)
    , m_context(nullptr)
    , m_videoId(-1)
    , m_audioId(-1)
    , m_recordPath(recordUrl)
    , m_snapshotInterval(snapshotInterval)
{
    m_videoQueue.reset(new woogeen_base::MediaFrameQueue());
    m_audioQueue.reset(new woogeen_base::MediaFrameQueue());

    init();
    ELOG_DEBUG("created");
}

MediaRecorder::~MediaRecorder()
{
    close();
}

void MediaRecorder::setMediaSource(woogeen_base::FrameProvider* videoSource, woogeen_base::FrameProvider* audioSource)
{
    if (m_status == woogeen_base::MediaMuxer::Context_READY) {
        callback("success");
        ELOG_DEBUG("continuous recording");
    }

    // Reset media source
    unsetMediaSource();

    // Assign new media source
    m_videoSource = videoSource;
    m_audioSource = audioSource;

    // Start the recording of video and audio
    woogeen_base::MediaSpecInfo info {0};
    if (m_videoSource)
        m_videoId = m_videoSource->addFrameConsumer(m_recordPath, woogeen_base::FRAME_FORMAT_VP8, this, info);

    if (m_audioSource)
        m_audioId = m_audioSource->addFrameConsumer(m_recordPath, woogeen_base::FRAME_FORMAT_PCMU, this, info);
}

void MediaRecorder::unsetMediaSource()
{
    if (m_videoSource && m_videoId != -1)
        m_videoSource->removeFrameConsumer(m_videoId);

    m_videoSource = nullptr;
    m_videoId = -1;

    if (m_audioSource && m_audioId != -1)
        m_audioSource->removeFrameConsumer(m_audioId);

    m_audioSource = nullptr;
    m_audioId = -1;
}

bool MediaRecorder::init()
{
    // FIXME: These should really only be called once per application run
    av_register_all();
    avcodec_register_all();
    av_log_set_level(AV_LOG_WARNING);

    m_context = avformat_alloc_context();
    if (m_context == NULL) {
        ELOG_ERROR("Error allocating memory for recording file IO context.");
        return false;
    }

    m_recordPath.copy(m_context->filename, sizeof(m_context->filename), 0);
    m_context->oformat = av_guess_format(NULL, m_context->filename, NULL);
    if (!m_context->oformat){
        ELOG_ERROR("Error guessing recording file format %s", m_context->filename);
        return false;
    }

    m_jobTimer.reset(new woogeen_base::JobTimer(100, this));
    return true;
}

void MediaRecorder::close()
{
    m_jobTimer->stop();

    if (m_audioStream != NULL && m_videoStream != NULL && m_context != NULL)
        av_write_trailer(m_context);

    if (m_videoStream && m_videoStream->codec != NULL)
        avcodec_close(m_videoStream->codec);

    if (m_audioStream && m_audioStream->codec != NULL)
        avcodec_close(m_audioStream->codec);

    if (m_context != NULL){
        if (!(m_context->oformat->flags & AVFMT_NOFILE))
            avio_close(m_context->pb);
        avformat_free_context(m_context);
        m_context = NULL;
    }
    ELOG_DEBUG("closed");
}

void MediaRecorder::onFrame(const woogeen_base::Frame& frame)
{
    if (m_status == woogeen_base::MediaMuxer::Context_ERROR)
        return;

    switch (frame.format) {
    case woogeen_base::FRAME_FORMAT_VP8:
        if (!m_videoStream) {
            addVideoStream(payloadType2VideoCodecID(VP8_90000_PT), frame.additionalInfo.video.width, frame.additionalInfo.video.height);
            ELOG_DEBUG("video stream added: %dx%d", frame.additionalInfo.video.width, frame.additionalInfo.video.height);
        }
        m_videoQueue->pushFrame(frame.payload, frame.length);
        break;
    case woogeen_base::FRAME_FORMAT_PCMU:
        if (m_videoStream && !m_audioStream) { // make sure video stream is added first.
            addAudioStream(payloadType2AudioCodecID(PCMU_8000_PT), frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);
            ELOG_DEBUG("audio stream added: %d channel(s), %d Hz", frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);
        }
        m_audioQueue->pushFrame(frame.payload, frame.length);
        break;
    default:
        ELOG_ERROR("improper frame format. only VP8 and PCM_MULAW can be recorded currently");
        m_status = woogeen_base::MediaMuxer::Context_ERROR;
        break;
    }
}

void MediaRecorder::addAudioStream(enum AVCodecID codec_id, int nbChannels, int sampleRate)
{
    boost::lock_guard<boost::mutex> lock(m_contextMutex);
    AVStream* stream = avformat_new_stream(m_context, nullptr);
    if (!stream) {
        ELOG_ERROR("cannot add audio stream");
        m_status = woogeen_base::MediaMuxer::Context_ERROR;
        return;
    }
    AVCodecContext* c = stream->codec;
    c->codec_id       = codec_id;
    c->codec_type     = AVMEDIA_TYPE_AUDIO;
    c->channels       = nbChannels;
    c->channel_layout = av_get_default_channel_layout(nbChannels);
    c->sample_rate    = sampleRate;
    c->sample_fmt     = AV_SAMPLE_FMT_S16;

    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    m_audioStream = stream;
}

void MediaRecorder::addVideoStream(enum AVCodecID codec_id, unsigned int width, unsigned int height)
{
    boost::lock_guard<boost::mutex> lock(m_contextMutex);
    m_context->oformat->video_codec = codec_id;
    AVStream* stream = avformat_new_stream(m_context, nullptr);
    if (!stream) {
        ELOG_ERROR("cannot add video stream");
        m_status = woogeen_base::MediaMuxer::Context_ERROR;
        return;
    }
    AVCodecContext* c = stream->codec;
    c->codec_id   = codec_id;
    c->codec_type = AVMEDIA_TYPE_VIDEO;
    c->width      = width;
    c->height     = height;
    c->pix_fmt    = AV_PIX_FMT_YUV420P;
    /* Some formats want stream headers to be separate. */
    if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    m_context->oformat->flags |= AVFMT_VARIABLE_FPS;
    m_videoStream = stream;
}

void MediaRecorder::onTimeout()
{
    switch (m_status) {
    case woogeen_base::MediaMuxer::Context_EMPTY:
        if (m_audioStream && m_videoStream) {
            if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
                if (avio_open(&m_context->pb, m_context->filename, AVIO_FLAG_WRITE) < 0) {
                    ELOG_ERROR("open output file failed");
                    m_status = woogeen_base::MediaMuxer::Context_ERROR;
                    callback("output file does not exist or cannot be opened for write");
                    return;
                }
            }
            av_dump_format(m_context, 0, m_context->filename, 1);
            if (avformat_write_header(m_context, nullptr) < 0) {
                m_status = woogeen_base::MediaMuxer::Context_ERROR;
                callback("write file header error");
                ELOG_ERROR("avformat_write_header failed");
                return;
            }
            m_status = woogeen_base::MediaMuxer::Context_READY;
            callback("success");
            ELOG_DEBUG("context ready");
        } else
            return;
        break;
    case woogeen_base::MediaMuxer::Context_READY:
        break;
    case woogeen_base::MediaMuxer::Context_ERROR:
    default:
        callback("context initialization failed");
        ELOG_ERROR("context error");
        return;
    }

    boost::shared_ptr<woogeen_base::EncodedFrame> mediaFrame;
    while (mediaFrame = m_audioQueue->popFrame())
        this->writeAudioFrame(*mediaFrame);

    while (mediaFrame = m_videoQueue->popFrame())
        this->writeVideoFrame(*mediaFrame);
}

void MediaRecorder::writeVideoFrame(woogeen_base::EncodedFrame& encodedVideoFrame)
{
    AVPacket avpkt;
    av_init_packet(&avpkt);
    avpkt.data = encodedVideoFrame.m_payloadData;
    avpkt.size = encodedVideoFrame.m_payloadSize;
    avpkt.pts = (int64_t)(encodedVideoFrame.m_timeStamp / (av_q2d(m_videoStream->time_base) * 1000));
    avpkt.stream_index = 0;
    av_write_frame(m_context, &avpkt);
    av_free_packet(&avpkt);
}

void MediaRecorder::writeAudioFrame(woogeen_base::EncodedFrame& encodedAudioFrame)
{
    AVPacket avpkt;
    av_init_packet(&avpkt);
    avpkt.data = encodedAudioFrame.m_payloadData;
    avpkt.size = encodedAudioFrame.m_payloadSize;
    avpkt.pts = (int64_t)(encodedAudioFrame.m_timeStamp / (av_q2d(m_audioStream->time_base) * 1000));
    avpkt.stream_index = 1;
    av_write_frame(m_context, &avpkt);
    av_free_packet(&avpkt);
}

} /* namespace mcu */
