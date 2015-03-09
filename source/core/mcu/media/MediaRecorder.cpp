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

#include <Compiler.h>

namespace mcu {

DEFINE_LOGGER(MediaRecorder, "mcu.media.MediaRecorder");

MediaRecorder::MediaRecorder(MediaRecording* videoRecording, MediaRecording* audioRecording, const std::string& recordPath)
    : m_recording(false), m_recordVideoStream(NULL), m_recordAudioStream(NULL)
    , m_recordStartTime(-1), m_firstVideoTimestamp(-1), m_firstAudioTimestamp(-1)
{
    m_videoRecording = videoRecording;
    m_audioRecording = audioRecording;
    m_recordPath = recordPath;
}

MediaRecorder::~MediaRecorder()
{
    if (m_recording)
        stopRecording();
}

bool MediaRecorder::startRecording()
{
    if (m_recordStartTime == -1) {
        timeval time;
        gettimeofday(&time, nullptr);

        m_recordStartTime = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    }

    // FIXME: These should really only be called once per application run
    av_register_all();
    avcodec_register_all();

    m_recordContext = avformat_alloc_context();
    if (m_recordContext == NULL) {
        ELOG_ERROR("Error allocating memory for recording file IO context.");
        return false;
    }

    m_recordPath.copy(m_recordContext->filename, sizeof(m_recordContext->filename), 0);
    m_recordContext->oformat = av_guess_format(NULL, m_recordContext->filename, NULL);
    if (!m_recordContext->oformat){
        ELOG_ERROR("Error guessing recording file format %s", m_recordContext->filename);
        return false;
    }

    m_recordContext->oformat->video_codec = AV_CODEC_ID_VP8;       // FIXME: Currently only VP8 can be recorded as video stream
    m_recordContext->oformat->audio_codec = AV_CODEC_ID_PCM_MULAW; // FIXME: We should figure this out once we start receiving data; it's either PCMU or OPUS

    // Initialize the record context
    if (!initRecordContext())
        return false;

    m_videoQueue.reset(new MediaFrameQueue(m_recordStartTime));
    m_audioQueue.reset(new MediaFrameQueue(m_recordStartTime));
    // Start the recording of video and audio
    m_videoRecording->startRecording(*m_videoQueue);
    m_audioRecording->startRecording(*m_audioQueue);

    // File write thread
    m_recordThread = boost::thread(&MediaRecorder::recordLoop, this);

    m_recording = true;

    return true;
}

void MediaRecorder::stopRecording()
{
    m_recording = false;
    m_recordThread.join();

    m_videoRecording->stopRecording();
    m_audioRecording->stopRecording();

    if (m_recordAudioStream != NULL && m_recordVideoStream != NULL && m_recordContext != NULL)
        av_write_trailer(m_recordContext);

    if (m_recordVideoStream && m_recordVideoStream->codec != NULL)
        avcodec_close(m_recordVideoStream->codec);

    if (m_recordAudioStream && m_recordAudioStream->codec != NULL)
        avcodec_close(m_recordAudioStream->codec);

    if (m_recordContext != NULL){
        avio_close(m_recordContext->pb);
        avformat_free_context(m_recordContext);
        m_recordContext = NULL;
    }

    ELOG_DEBUG("Media recording is closed successfully.");
}

bool MediaRecorder::initRecordContext()
{
    if (m_recordContext->oformat->video_codec != AV_CODEC_ID_NONE
        && m_recordContext->oformat->audio_codec != AV_CODEC_ID_NONE
        && m_recordVideoStream == NULL && m_recordAudioStream == NULL) {
        AVCodec* videoCodec = avcodec_find_encoder(m_recordContext->oformat->video_codec);
        if (videoCodec == NULL) {
            ELOG_ERROR("Media recording could not find video codec.");
            return false;
        }

        m_recordVideoStream = avformat_new_stream (m_recordContext, videoCodec);
        m_recordVideoStream->id = 0;
        m_recordVideoStream->codec->codec_id = m_recordContext->oformat->video_codec;
        // FIXME: Chunbo to set this kind of codec information from VideoMixer
        m_recordVideoStream->codec->width = 640;
        m_recordVideoStream->codec->height = 480;
        // A decent guess here suffices; if processing the file with ffmpeg, use -vsync 0 to force it not to duplicate frames.
        m_recordVideoStream->codec->time_base = (AVRational){1,30};
        m_recordVideoStream->codec->pix_fmt = PIX_FMT_YUV420P;

        if (m_recordContext->oformat->flags & AVFMT_GLOBALHEADER)
            m_recordVideoStream->codec->flags|=CODEC_FLAG_GLOBAL_HEADER;

        m_recordContext->oformat->flags |= AVFMT_VARIABLE_FPS;

        AVCodec* audioCodec = avcodec_find_encoder(m_recordContext->oformat->audio_codec);
        if (audioCodec == NULL) {
            ELOG_ERROR("Media recording could not find audio codec.");
            return false;
        }

        m_recordAudioStream = avformat_new_stream (m_recordContext, audioCodec);
        m_recordAudioStream->id = 1;
        m_recordAudioStream->codec->codec_id = m_recordContext->oformat->audio_codec;
        // FIXME: Chunbo to set this kind of codec information from AudioMixer
        m_recordAudioStream->codec->sample_rate = m_recordContext->oformat->audio_codec == AV_CODEC_ID_PCM_MULAW ? 8000 : 48000; // FIXME: Is it always 48 khz for opus?
        m_recordAudioStream->codec->time_base = (AVRational) {1, m_recordAudioStream->codec->sample_rate};
        m_recordAudioStream->codec->channels = m_recordContext->oformat->audio_codec == AV_CODEC_ID_PCM_MULAW ? 1 : 2;   // FIXME: Is it always two channels for opus?
        if (m_recordContext->oformat->flags & AVFMT_GLOBALHEADER)
            m_recordAudioStream->codec->flags|=CODEC_FLAG_GLOBAL_HEADER;

        m_recordContext->streams[0] = m_recordVideoStream;
        m_recordContext->streams[1] = m_recordAudioStream;
        if (avio_open(&m_recordContext->pb, m_recordContext->filename, AVIO_FLAG_WRITE) < 0) {
            ELOG_ERROR("Media recording error when opening output file.");
            return false;
        }

        if (avformat_write_header(m_recordContext, NULL) < 0) {
            ELOG_ERROR("Media recording error during writing header");
            return false;
        }
    }

    ELOG_DEBUG("Media recording has been initialized successfully.");
    return true;
}

void MediaRecorder::recordLoop()
{
    while (m_recording) {
        boost::shared_ptr<EncodedFrame> mediaFrame;
        while (mediaFrame = m_audioQueue->popFrame())
            this->writeAudioFrame(*mediaFrame);

        while (mediaFrame = m_videoQueue->popFrame())
            this->writeVideoFrame(*mediaFrame);
    }
}

void MediaRecorder::writeVideoFrame(EncodedFrame& encodedVideoFrame) {
    if (m_recordVideoStream == NULL) {
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

    long long timestampToWrite = (currentTimestamp - m_firstVideoTimestamp) / (90000 / m_recordVideoStream->time_base.den);  // All of our video offerings are using a 90khz clock.
    // Adjust for the start time offset
    timestampToWrite += encodedVideoFrame.m_offsetMsec / (1000 / m_recordVideoStream->time_base.den);   //In practice, our timebase den is 1000, so this operation is a no-op.

    AVPacket avpkt;
    av_init_packet(&avpkt);
    avpkt.data = encodedVideoFrame.m_payloadData;
    avpkt.size = encodedVideoFrame.m_payloadSize;
    avpkt.pts = timestampToWrite;
    avpkt.stream_index = 0;
    av_write_frame(m_recordContext, &avpkt);
    av_free_packet(&avpkt);
}

void MediaRecorder::writeAudioFrame(EncodedFrame& encodedAudioFrame) {
    if (m_recordAudioStream == NULL) {
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

    long long timestampToWrite = (currentTimestamp - m_firstAudioTimestamp) / (m_recordAudioStream->codec->time_base.den / m_recordAudioStream->time_base.den);
    // Adjust for our start time offset
    timestampToWrite += encodedAudioFrame.m_offsetMsec / (1000 / m_recordAudioStream->time_base.den);   // In practice, our timebase den is 1000, so this operation is a no-op.

    AVPacket avpkt;
    av_init_packet(&avpkt);
    avpkt.data = encodedAudioFrame.m_payloadData;
    avpkt.size = encodedAudioFrame.m_payloadSize;
    avpkt.pts = timestampToWrite;
    avpkt.stream_index = 1;
    av_write_frame(m_recordContext, &avpkt);
    av_free_packet(&avpkt);
}

}/* namespace mcu */
