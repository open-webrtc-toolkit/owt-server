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
#include <sys/time.h>

using namespace erizo;

namespace woogeen_base {

DEFINE_LOGGER(RtspIn, "woogeen.RtspIn");

RtspIn::RtspIn(const Options& options)
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
    , m_audioStreamIndex(-1)
    , m_audioFormat(FRAME_FORMAT_UNKNOWN)
    , m_statusListener(nullptr)
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
}

RtspIn::RtspIn (const std::string& url, const std::string& transport, uint32_t bufferSize, bool enableAudio, bool enableVideo)
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
    , m_audioStreamIndex(-1)
    , m_audioFormat(FRAME_FORMAT_UNKNOWN)
    , m_statusListener(nullptr)
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
    ELOG_DEBUG("closed");
}

void RtspIn::setStatusListener(RtspInStatusListener* listener)
{
    m_statusListener = listener;
}

void RtspIn::init()
{
    m_running = true;
    m_thread = boost::thread(&RtspIn::receiveLoop, this);
}

bool RtspIn::connect()
{
    srand((unsigned)time(0));

    m_context = avformat_alloc_context();
    m_timeoutHandler = new TimeoutHandler(20000);
    m_context->interrupt_callback = {&TimeoutHandler::checkInterrupt, m_timeoutHandler};
    av_register_all();
    avcodec_register_all();
    avformat_network_init();
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
                    notifyStatus("videoCodec:vp8");
                } else if (videoCodecId == AV_CODEC_ID_H264) {
                    m_videoFormat = FRAME_FORMAT_H264;
                    notifyStatus("videoCodec:h264");
                }
            } else {
                ELOG_WARN("Video codec %d is not supported ", st->codec->codec_id);
            }
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
            if (audioCodecId == AV_CODEC_ID_PCM_MULAW ||
                audioCodecId == AV_CODEC_ID_PCM_ALAW ||
                audioCodecId == AV_CODEC_ID_OPUS) {
                if (audioCodecId == AV_CODEC_ID_PCM_MULAW) {
                    m_audioFormat = FRAME_FORMAT_PCMU;
                    notifyStatus("audioCodec:pcmu");
                } else if (audioCodecId == AV_CODEC_ID_PCM_ALAW) {
                    m_audioFormat = FRAME_FORMAT_PCMA;
                    notifyStatus("audioCodec:pcma");
                } else if (audioCodecId == AV_CODEC_ID_OPUS) {
                    m_audioFormat = FRAME_FORMAT_OPUS;
                    notifyStatus("audioCodec:opus_48000_2");
                }
            } else {
                ELOG_WARN("Audio codec %d is not supported ", audioCodecId);
            }
        }
    }

    if (m_audioFormat == FRAME_FORMAT_UNKNOWN && m_videoFormat == FRAME_FORMAT_UNKNOWN)
        return false;

    av_init_packet(&m_avPacket);
    return true;
}

void RtspIn::receiveLoop()
{
    bool ret = connect();
    std::string message;
    if (ret)
        message = "success";
    else
        message = "opening input url error";

    if (m_statusListener)
        m_statusListener->notifyStatus(message);

    if (!ret) return;

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
                if (m_statusListener)
                    m_statusListener->notifyStatus("reopening input url error");

                return;
            }
            av_read_play(m_context);
            continue;
        }

        if (m_avPacket.stream_index == m_videoStreamIndex) { //packet is video
            //ELOG_DEBUG("Receive video frame packet with size %d ", m_avPacket.size);
            Frame frame = {};
            memset(&frame, 0, sizeof(frame));
            frame.format = m_videoFormat;
            frame.payload = reinterpret_cast<uint8_t*>(m_avPacket.data);
            frame.length = m_avPacket.size;
            frame.timeStamp = m_avPacket.dts;
            frame.additionalInfo.video.width = 1920; //FIXME: should be fetched previously when connecting.
            frame.additionalInfo.video.height = 1080; //FIXME: should be fetched previously when connecting.
            deliverFrame(frame);
        } else if (m_avPacket.stream_index == m_audioStreamIndex) { //packet is audio
            //ELOG_DEBUG("Receive audio frame packet with size %d ", m_avPacket.size);
            Frame frame = {};
            memset(&frame, 0, sizeof(frame));
            frame.format = m_audioFormat;
            frame.payload = reinterpret_cast<uint8_t*>(m_avPacket.data);
            frame.length = m_avPacket.size;
            frame.timeStamp = m_avPacket.dts;
            frame.additionalInfo.audio.isRtpPacket = 0;
            frame.additionalInfo.audio.nbSamples = m_avPacket.duration;
            frame.additionalInfo.audio.sampleRate = m_audioFormat == FRAME_FORMAT_OPUS ? 48000 : 8000;
            frame.additionalInfo.audio.channels = m_audioFormat == FRAME_FORMAT_OPUS ? 2 : 1;
            deliverFrame(frame);
        }
        av_free_packet(&m_avPacket);
        av_init_packet(&m_avPacket);
    }
    m_running = false;
    av_read_pause(m_context);
}

void RtspIn::notifyStatus(const std::string& msg) {
    if (m_statusListener)
        m_statusListener->notifyStatus(msg);
}

}
