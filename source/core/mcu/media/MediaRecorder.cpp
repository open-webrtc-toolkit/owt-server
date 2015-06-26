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

MediaRecorder::MediaRecorder(const std::string& recordUrl, int snapshotInterval)
    : m_videoSource(nullptr)
    , m_audioSource(nullptr)
    , m_videoStream(nullptr)
    , m_audioStream(nullptr)
    , m_context(nullptr)
    , m_videoId(-1)
    , m_audioId(-1)
    , m_recordPath(recordUrl)
    , m_snapshotInterval(snapshotInterval)
{
    m_muxing = false;
    m_firstVideoTimestamp = -1;
    m_firstAudioTimestamp = -1;

    m_videoQueue.reset(new woogeen_base::MediaFrameQueue(0));
    m_audioQueue.reset(new woogeen_base::MediaFrameQueue(0));

    init();
}

MediaRecorder::~MediaRecorder()
{
    if (m_muxing)
        close();
}

bool MediaRecorder::setMediaSource(woogeen_base::FrameDispatcher* videoSource, woogeen_base::FrameDispatcher* audioSource)
{
    // Reset the media queues
    m_videoQueue.reset(new woogeen_base::MediaFrameQueue(0));
    m_audioQueue.reset(new woogeen_base::MediaFrameQueue(0));

    if (m_videoSource && m_videoId != -1)
        m_videoSource->removeFrameConsumer(m_videoId);

    if (m_audioSource && m_audioId != -1)
        m_audioSource->removeFrameConsumer(m_audioId);

    m_videoSource = videoSource;
    m_audioSource = audioSource;

    // Start the recording of video and audio
    m_videoId = m_videoSource->addFrameConsumer(m_recordPath, VP8_90000_PT, this);
    m_audioId = m_audioSource->addFrameConsumer(m_recordPath, PCMU_8000_PT, this);

    return true;
}

void MediaRecorder::removeMediaSource()
{
    if (m_videoSource && m_videoId != -1)
        m_videoSource->removeFrameConsumer(m_videoId);

    if (m_audioSource && m_audioId != -1)
        m_audioSource->removeFrameConsumer(m_audioId);
}

bool MediaRecorder::init()
{
    // FIXME: These should really only be called once per application run
    av_register_all();
    avcodec_register_all();

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

    m_context->oformat->video_codec = payloadType2VideoCodecID(VP8_90000_PT);
    m_context->oformat->audio_codec = payloadType2AudioCodecID(PCMU_8000_PT);

    // Initialize the record context
    if (!initRecordContext())
        return false;

    // File write thread
    m_muxing = true;
    m_thread = boost::thread(&MediaRecorder::recordLoop, this);

    return true;
}

void MediaRecorder::close()
{
    m_muxing = false;
    m_thread.join();

    if (m_audioStream != NULL && m_videoStream != NULL && m_context != NULL)
        av_write_trailer(m_context);

    if (m_videoStream && m_videoStream->codec != NULL)
        avcodec_close(m_videoStream->codec);

    if (m_audioStream && m_audioStream->codec != NULL)
        avcodec_close(m_audioStream->codec);

    if (m_context != NULL){
        avio_close(m_context->pb);
        avformat_free_context(m_context);
        m_context = NULL;
    }

    ELOG_DEBUG("Media recording is closed successfully.");
}

void MediaRecorder::onFrame(const woogeen_base::Frame& frame)
{
    switch (frame.format) {
    case woogeen_base::FRAME_FORMAT_VP8:
        m_videoQueue->pushFrame(frame.payload, frame.length, frame.timeStamp);
        break;
    case woogeen_base::FRAME_FORMAT_PCMU:
        m_audioQueue->pushFrame(frame.payload, frame.length, frame.timeStamp);
        break;
    default:
        break;
    }
}

bool MediaRecorder::initRecordContext()
{
    if (m_context->oformat->video_codec != AV_CODEC_ID_NONE
        && m_context->oformat->audio_codec != AV_CODEC_ID_NONE
        && m_videoStream == NULL && m_audioStream == NULL) {
        AVCodec* videoCodec = avcodec_find_encoder(m_context->oformat->video_codec);
        if (videoCodec == NULL) {
            ELOG_ERROR("Media recording could not find video codec.");
            return false;
        }

        m_videoStream = avformat_new_stream(m_context, videoCodec);
        m_videoStream->id = 0;
        m_videoStream->codec->codec_id = m_context->oformat->video_codec;

        // unsigned int width = 0;
        // unsigned int height = 0;
        // if (m_videoSource->getVideoSize(width, height)) {
        //     m_videoStream->codec->width = width;
        //     m_videoStream->codec->height = height;
        // } else {
            // FIXME: Currently, ONLY 720p to be recorded.
            m_videoStream->codec->width = 1280;
            m_videoStream->codec->height = 720;
        //}

        // A decent guess here suffices; if processing the file with ffmpeg, use -vsync 0 to force it not to duplicate frames.
        m_videoStream->codec->time_base = (AVRational){1,30};
        m_videoStream->codec->pix_fmt = PIX_FMT_YUV420P;

        if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
            m_videoStream->codec->flags|=CODEC_FLAG_GLOBAL_HEADER;

        m_context->oformat->flags |= AVFMT_VARIABLE_FPS;

        AVCodec* audioCodec = avcodec_find_encoder(m_context->oformat->audio_codec);
        if (audioCodec == NULL) {
            ELOG_ERROR("Media recording could not find audio codec.");
            return false;
        }

        m_audioStream = avformat_new_stream(m_context, audioCodec);
        m_audioStream->id = 1;
        m_audioStream->codec->codec_id = m_context->oformat->audio_codec;
        // FIXME: Chunbo to set this kind of codec information from AudioMixer
        m_audioStream->codec->sample_rate = m_context->oformat->audio_codec == AV_CODEC_ID_PCM_MULAW ? 8000 : 48000; // FIXME: Is it always 48 khz for opus?
        m_audioStream->codec->time_base = (AVRational) {1, m_audioStream->codec->sample_rate};
        m_audioStream->codec->channels = m_context->oformat->audio_codec == AV_CODEC_ID_PCM_MULAW ? 1 : 2;   // FIXME: Is it always two channels for opus?
        if (m_context->oformat->flags & AVFMT_GLOBALHEADER)
            m_audioStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

        m_context->streams[0] = m_videoStream;
        m_context->streams[1] = m_audioStream;
        if (avio_open(&m_context->pb, m_context->filename, AVIO_FLAG_WRITE) < 0) {
            ELOG_ERROR("Media recording error when opening output file.");
            return false;
        }

        if (avformat_write_header(m_context, NULL) < 0) {
            ELOG_ERROR("Media recording error during writing header");
            return false;
        }
    }

    ELOG_DEBUG("Media recording has been initialized successfully.");
    return true;
}

void MediaRecorder::recordLoop()
{
    while (m_muxing) {
        boost::shared_ptr<woogeen_base::EncodedFrame> mediaFrame;
        while (mediaFrame = m_audioQueue->popFrame())
            this->writeAudioFrame(*mediaFrame);

        while (mediaFrame = m_videoQueue->popFrame())
            this->writeVideoFrame(*mediaFrame);
    }
}

void MediaRecorder::writeVideoFrame(woogeen_base::EncodedFrame& encodedVideoFrame) {
    if (m_videoStream == NULL) {
        // could not init our context yet.
        return;
    }

    if (m_firstVideoTimestamp == -1)
        m_firstVideoTimestamp = encodedVideoFrame.m_timeStamp;

    long long currentTimestamp = encodedVideoFrame.m_timeStamp;
    if (currentTimestamp - m_firstVideoTimestamp < 0) {
        // We wrapped and added 2^32 to correct this. We only handle a single wrap around since that's ~13 hours of recording, minimum.
        currentTimestamp += 0xFFFFFFFF;
    }

    long long timestampToWrite = (currentTimestamp - m_firstVideoTimestamp) / (90000 / m_videoStream->time_base.den);  // All of our video offerings are using a 90khz clock.
    // Adjust for the start time offset
    timestampToWrite += encodedVideoFrame.m_offsetMsec / (1000 / m_videoStream->time_base.den);   //In practice, our timebase den is 1000, so this operation is a no-op.

    AVPacket avpkt;
    av_init_packet(&avpkt);
    avpkt.data = encodedVideoFrame.m_payloadData;
    avpkt.size = encodedVideoFrame.m_payloadSize;
    avpkt.pts = timestampToWrite;
    avpkt.stream_index = 0;
    av_write_frame(m_context, &avpkt);
    av_free_packet(&avpkt);
}

void MediaRecorder::writeAudioFrame(woogeen_base::EncodedFrame& encodedAudioFrame) {
    if (m_audioStream == NULL) {
        // No audio stream has been initialized
        return;
    }

    if (m_firstAudioTimestamp == -1)
        m_firstAudioTimestamp = encodedAudioFrame.m_timeStamp;

    long long currentTimestamp = encodedAudioFrame.m_timeStamp;
    if (currentTimestamp - m_firstAudioTimestamp < 0) {
      // We wrapped and added 2^32 to correct this.  We only handle a single wrap around since that's ~13 hours of recording, minimum.
      currentTimestamp += 0xFFFFFFFF;
    }

    long long timestampToWrite = (currentTimestamp - m_firstAudioTimestamp) / (m_audioStream->codec->time_base.den / m_audioStream->time_base.den);
    // Adjust for our start time offset
    timestampToWrite += encodedAudioFrame.m_offsetMsec / (1000 / m_audioStream->time_base.den);   // In practice, our timebase den is 1000, so this operation is a no-op.

    AVPacket avpkt;
    av_init_packet(&avpkt);
    avpkt.data = encodedAudioFrame.m_payloadData;
    avpkt.size = encodedAudioFrame.m_payloadSize;
    avpkt.pts = timestampToWrite;
    avpkt.stream_index = 1;
    av_write_frame(m_context, &avpkt);
    av_free_packet(&avpkt);
}

}/* namespace mcu */
