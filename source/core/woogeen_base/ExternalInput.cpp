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

#include "ExternalInput.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cstdio>
#include <rtputils.h>
#include <sys/time.h>

using namespace erizo;

namespace woogeen_base {

const uint32_t BUFFER_SIZE = 2*1024*1024;
DEFINE_LOGGER(ExternalInput, "woogeen.ExternalInput");

/*
  options: {
    url: 'xxx',
    transport: 'tcp'/'udp',
    buffer_size: 1024*1024*4
  }
*/
ExternalInput::ExternalInput(const std::string& options)
    : m_transportOpts(nullptr)
    , m_running(false)
    , m_context(nullptr)
    , m_timeoutHandler(nullptr)
    , m_videoStreamIndex(-1)
    , m_audioStreamIndex(-1)
    , m_audioSeqNumber(0)
    , m_statusListener(nullptr)
{
    boost::property_tree::ptree pt;
    std::istringstream is(options);
    boost::property_tree::read_json(is, pt);
    m_url = pt.get<std::string>("url", "");
    m_needAudio = pt.get<bool>("audio", false);
    m_needVideo = pt.get<bool>("video", false);
    std::string transport = pt.get<std::string>("transport", "udp");
    if (transport.compare("tcp") == 0) {
        av_dict_set(&m_transportOpts, "rtsp_transport", "tcp", 0);
        ELOG_DEBUG("url: %s, audio: %d, video: %d, transport::tcp", m_url.c_str(), m_needAudio, m_needVideo);
    } else {
        char buf[256];
        uint32_t buffer_size = pt.get<uint32_t>("buffer_size", BUFFER_SIZE);
        snprintf(buf, sizeof(buf), "%u", buffer_size);
        av_dict_set(&m_transportOpts, "buffer_size", buf, 0);
        ELOG_DEBUG("url: %s, audio: %d, video: %d, transport::%s, buffer_size: %u",
                   m_url.c_str(), m_needAudio, m_needVideo, transport.c_str(), buffer_size);
    }
    videoDataType_ = DataContentType::ENCODED_FRAME;
    audioDataType_ = DataContentType::RTP;
}

ExternalInput::~ExternalInput()
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

void ExternalInput::setStatusListener(ExternalInputStatusListener* listener)
{
    m_statusListener = listener;
}

void ExternalInput::init()
{
    m_running = true;
    m_thread = boost::thread(&ExternalInput::receiveLoop, this);
}

bool ExternalInput::connect()
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
            if (videoCodecId == AV_CODEC_ID_VP8 || videoCodecId == AV_CODEC_ID_H264) {
                if (!videoSourceSSRC_) {
                    unsigned int videoSourceId = rand();
                    ELOG_DEBUG("Set video SSRC : %d ", videoSourceId);
                    setVideoSourceSSRC(videoSourceId);
                }

                if (videoCodecId == AV_CODEC_ID_VP8)
                    videoPayloadType_ = VP8_90000_PT;
                else if (videoCodecId == AV_CODEC_ID_H264)
                    videoPayloadType_ = H264_90000_PT;
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
                if (!audioSourceSSRC_) {
                    unsigned int audioSourceId = rand();
                    ELOG_DEBUG("Set audio SSRC : %d", audioSourceId);
                    setAudioSourceSSRC(audioSourceId);
                }

                if (audioCodecId == AV_CODEC_ID_PCM_MULAW)
                    audioPayloadType_ = PCMU_8000_PT;
                else if (audioCodecId == AV_CODEC_ID_PCM_ALAW)
                    audioPayloadType_ = PCMA_8000_PT;
                else if (audioCodecId == AV_CODEC_ID_OPUS)
                    audioPayloadType_ = OPUS_48000_PT;
            } else {
                ELOG_WARN("Audio codec %d is not supported ", audioCodecId);
            }
        }
    }

    if (!videoSourceSSRC_ && !audioSourceSSRC_)
        return false;

    av_init_packet(&m_avPacket);
    return true;
}

int ExternalInput::sendFirPacket()
{
    return 0;
}

void ExternalInput::receiveLoop()
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
            if (videoSourceSSRC_ && videoSink_)
                videoSink_->deliverVideoData((char*)m_avPacket.data, m_avPacket.size);
        } else if (m_avPacket.stream_index == m_audioStreamIndex) { //packet is audio
            //ELOG_DEBUG("Receive audio frame packet with size %d ", m_avPacket.size);
            if (audioSourceSSRC_ && audioSink_) {
                char buf[MAX_DATA_PACKET_SIZE];
                RTPHeader* head = reinterpret_cast<RTPHeader*>(buf);
                memset(head, 0, sizeof(RTPHeader));
                head->setVersion(2);
                head->setSSRC(audioSourceSSRC_);
                head->setPayloadType(audioPayloadType_);
                head->setTimestamp(m_avPacket.dts);
                head->setSeqNumber(m_audioSeqNumber++);
                head->setMarker(false); // not used.
                memcpy(&buf[head->getHeaderLength()], m_avPacket.data, m_avPacket.size);

                audioSink_->deliverAudioData(buf, m_avPacket.size + head->getHeaderLength());
            }
        }
        av_free_packet(&m_avPacket);
        av_init_packet(&m_avPacket);
    }
    m_running = false;
    av_read_pause(m_context);
}

}
