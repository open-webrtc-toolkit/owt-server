/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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
#include "MediaUtilities.h"

#define KEYFRAME_REQ_INTERVAL (2 * 1000) // 2 seconds

static inline const char* getShortName(std::string& url)
{
    if (url.compare(0, 7, "rtsp://") == 0)
        return "rtsp";
    else if (url.compare(0, 7, "rtmp://") == 0)
        return "flv";
    else if (url.compare(0, 7, "http://") == 0)
        return "hls";
    return nullptr;
}

namespace woogeen_base {

DEFINE_LOGGER(RtspOut, "woogeen.RtspOut");

RtspOut::RtspOut(const std::string& url, const AVOptions* audio, const AVOptions* video, EventRegistry* handle)
    : AVStreamOut{ handle }
    , m_uri{ url }
    , m_audioReceived(false)
    , m_videoReceived(false)
    , m_context{ nullptr }
    , m_audioStream{ nullptr }
    , m_videoStream{ nullptr }
{
    if (!audio && !video) {
        ELOG_ERROR("NULL a/v AVOptions params");
        notifyAsyncEvent("init", "NULL a/v AVOptions params");
        return;
    }

    if (audio)
        m_audioOptions = *audio;
    if (video)
        m_videoOptions = *video;

    ELOG_TRACE("url %s, acodec %s, vcodec %s", m_uri.c_str(), m_audioOptions.codec.c_str(), m_videoOptions.codec.c_str());

    m_frameQueue.reset(new woogeen_base::MediaFrameQueue());

    if (ELOG_IS_TRACE_ENABLED())
        av_log_set_level(AV_LOG_DEBUG);
    else if (ELOG_IS_DEBUG_ENABLED())
        av_log_set_level(AV_LOG_INFO);
    else
        av_log_set_level(AV_LOG_WARNING);

    avcodec_register_all();

    if(!connect()) {
        notifyAsyncEvent("init", "Cannot open connection");
        return;
    }

    m_status = Context_INITIALIZING;
    notifyAsyncEvent("init", "");
    m_thread = boost::thread(&RtspOut::sendLoop, this);
}

RtspOut::~RtspOut()
{
    close();
}

void RtspOut::close()
{
    ELOG_INFO("Closing %s", m_uri.c_str());

    m_status = AVStreamOut::Context_CLOSED;
    m_frameQueue->cancel();
    m_thread.join();

    // output context
    if (m_context) {
        av_write_trailer(m_context);
        if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
            avio_close(m_context->pb);
        }
        avformat_free_context(m_context);
        m_context = NULL;
    }
}

void RtspOut::sendLoop()
{
    int ret;
    int i = 0;

    while ((hasAudio() && !m_audioReceived) || (hasVideo() && !m_videoReceived)) {
        if (m_status == AVStreamOut::Context_CLOSED)
            goto exit;

        if (i++ >= 100) {
            ELOG_ERROR("No a/v frames");
            notifyAsyncEvent("fatal", "No a/v frames");
            goto exit;
        }
        ELOG_DEBUG("Wait for av options available, hasAudio %d(rcv %d), hasVideo %d(rcv %d), retry %d"
                , hasAudio(), m_audioReceived, hasAudio(), m_videoReceived, i);
        usleep(20000);
    }

    if (hasAudio() && !addAudioStream(m_audioOptions)) {
        notifyAsyncEvent("fatal", "Cannot add audio stream");
        goto exit;
    }
    if (hasVideo() && !addVideoStream(m_videoOptions)) {
        notifyAsyncEvent("fatal", "Cannot add video stream");
        goto exit;
    }

    if (!writeHeader()) {
        notifyAsyncEvent("fatal", "Cannot write header");
        goto exit;
    }

    m_status = AVStreamOut::Context_READY;

    ELOG_DEBUG("Request video key frame");
    deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME});

    ELOG_DEBUG("Start sending");
    while (m_status == AVStreamOut::Context_READY) {
        boost::shared_ptr<woogeen_base::EncodedFrame> frame = m_frameQueue->popFrame(1000);
        if (!frame) {
            ELOG_WARN("No input frames available");
            notifyAsyncEvent("fatal", "No input frames available");
            goto exit;
        }

        int retry = 1;
        while(true) {
            ret = writeAVFrame(frame->m_format == FRAME_FORMAT_H264? m_videoStream : m_audioStream, *frame);
            if (ret == 0)
                break;

            if (retry-- <= 0) {
                ELOG_ERROR("Cannot write frame after reconnection");
                notifyAsyncEvent("fatal", "Cannot write frame after reconnection");
                goto exit;
            }

            if (!reconnect()){
                ELOG_ERROR("Cannot reconnect");
                notifyAsyncEvent("fatal", "Cannot reconnect");
                goto exit;
            }
        }
    }

exit:
    m_status = AVStreamOut::Context_CLOSED;
    ELOG_DEBUG("Thread exited!");
}

bool RtspOut::connect()
{
    int ret;

    avformat_alloc_output_context2(&m_context, nullptr, getShortName(m_uri), m_uri.c_str());
    if (!m_context) {
        ELOG_ERROR("Cannot allocate output context");
        goto fail;
    }

    if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_context->pb, m_context->filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            ELOG_ERROR("Cannot open connection(%s), %s", m_uri.c_str(), ff_err2str(ret));
            goto fail;
        }
    }

    return true;

fail:
    if (m_context) {
        if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
            avio_close(m_context->pb);
        }
        avformat_free_context(m_context);
        m_context = NULL;
    }
    return false;
}

bool RtspOut::reconnect()
{
    av_write_trailer(m_context);
    if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
        avio_close(m_context->pb);
    }
    avformat_free_context(m_context);

    m_context = NULL;
    m_audioStream = NULL;
    m_videoStream = NULL;

    ELOG_WARN("Write output data failed, trying to reopen output from url %s", m_uri.c_str());
    usleep(5000000);

    if (!connect())
        return false;

    if (hasAudio() && !addAudioStream(m_audioOptions)) {
        return false;
    }
    if (hasVideo() && !addVideoStream(m_videoOptions)) {
        return false;
    }

    if (!writeHeader()) {
        return false;
    }

    ELOG_DEBUG("Request video key frame");
    deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME});

    return true;
}

bool RtspOut::addAudioStream(AVOptions &options)
{
    enum AVCodecID codec_id = AV_CODEC_ID_AAC;

    if (options.codec.compare("aac") != 0) {
        ELOG_ERROR("Invalid audio codec, %s", options.codec.c_str());
        return false;
    }

    m_audioStream = avformat_new_stream(m_context, NULL);
    if (!m_audioStream) {
        ELOG_ERROR("Cannot add audio stream");
        return false;
    }

    AVCodecParameters *par = m_audioStream->codecpar;
    par->codec_type     = AVMEDIA_TYPE_AUDIO;
    par->codec_id       = codec_id;
    par->sample_rate    = m_audioOptions.spec.audio.sampleRate;
    par->channels       = m_audioOptions.spec.audio.channels;
    par->channel_layout = av_get_default_channel_layout(par->channels);
    if (codec_id == AV_CODEC_ID_AAC) { //AudioSpecificConfig 48000-2
        par->extradata_size = 2;
        par->extradata      = (uint8_t *)av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        par->extradata[0]   = 0x11;
        par->extradata[1]   = 0x90;
    }

    ELOG_DEBUG("Audio stream added");
    return true;
}

bool RtspOut::addVideoStream(AVOptions &options)
{
    enum AVCodecID codec_id = AV_CODEC_ID_H264;
    int width = options.spec.video.width;
    int height = options.spec.video.height;
    AVCodecParserContext *parser;

    if (options.codec.compare("h264") != 0) {
        ELOG_ERROR("Invalid video codec, %s", options.codec.c_str());
        return false;
    }

    m_videoStream = avformat_new_stream(m_context, NULL);
    if (!m_videoStream) {
        ELOG_ERROR("Cannot add video stream");
        return false;
    }

    AVCodecParameters *par = m_videoStream->codecpar;
    par->codec_type = AVMEDIA_TYPE_VIDEO;
    par->codec_id = codec_id;
    par->width = width;
    par->height = height;

    //extradata
    parser = av_parser_init(codec_id);
    if (!parser) {
        ELOG_ERROR("Cannot find video parser");
        return false;
    }

    int size = parser->parser->split(NULL, m_videoKeyFrame->m_payloadData, m_videoKeyFrame->m_payloadSize);
    if (size > 0) {
        par->extradata_size = size;
        par->extradata      = (uint8_t *)av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        memcpy(par->extradata, m_videoKeyFrame->m_payloadData, par->extradata_size);
    } else {
        ELOG_WARN("Cannot find video extradata");
    }

    av_parser_close(parser);
    ELOG_DEBUG("Video stream added: %dx%d", width, height);
    return true;
}

void RtspOut::onFrame(const woogeen_base::Frame& frame)
{
    switch (frame.format) {
        case woogeen_base::FRAME_FORMAT_AAC:
        case woogeen_base::FRAME_FORMAT_AAC_48000_2:
            if (!hasAudio())
                return;

            if (!m_audioReceived) {
                ELOG_INFO("Initial audio options format(%s), sample rate(%d), channels(%d)"
                        , getFormatStr(frame.format)
                        , frame.additionalInfo.audio.sampleRate
                        , frame.additionalInfo.audio.channels);

                m_audioOptions.spec.audio.sampleRate = frame.additionalInfo.audio.sampleRate;
                m_audioOptions.spec.audio.channels = frame.additionalInfo.audio.channels;
                m_audioReceived = true;
            }

            if (m_status != AVStreamOut::Context_READY)
                return;

            if (frame.additionalInfo.audio.channels != m_audioOptions.spec.audio.channels
                    || frame.additionalInfo.audio.sampleRate != m_audioOptions.spec.audio.sampleRate) {
                ELOG_ERROR("Invalid audio frame channels %d, or sample rate: %d"
                        , frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);

                notifyAsyncEvent("fatal", "Invalid audio frame channels or sample rate");
                return;
            }
            m_frameQueue->pushFrame(frame.payload, frame.length);

            break;

        case woogeen_base::FRAME_FORMAT_H264:
            if (!hasVideo())
                return;

            if (!m_videoReceived) {
                if (!isH264KeyFrame(frame.payload, frame.length)) {
                    ELOG_DEBUG("Not video key frame, %d", frame.length);

                    ELOG_DEBUG("Request video key frame");
                    deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME});
                    return;
                }

                ELOG_INFO("Initial video options: format(%s), %dx%d",
                        getFormatStr(frame.format),
                        frame.additionalInfo.video.width, frame.additionalInfo.video.height);

                m_videoKeyFrame.reset(new EncodedFrame(frame.payload, frame.length, 0, frame.format));

                m_videoOptions.spec.video.width = frame.additionalInfo.video.width;
                m_videoOptions.spec.video.height = frame.additionalInfo.video.height;
                m_videoReceived = true;
            }

            if (m_status != AVStreamOut::Context_READY)
                return;

            if (frame.additionalInfo.video.width != m_videoOptions.spec.video.width
                    || frame.additionalInfo.video.height != m_videoOptions.spec.video.height) {
                ELOG_ERROR("Invalid video frame resolution: %dx%d",
                        frame.additionalInfo.video.width, frame.additionalInfo.video.height);

                notifyAsyncEvent("fatal", "Invalid video frame resolution");
                return;
            }
            m_frameQueue->pushFrame(frame.payload, frame.length, frame.format);

            break;

        default:
            ELOG_ERROR("Unsupported frame format: %d", frame.format);

            notifyAsyncEvent("fatal", "Unsupported frame format");
            return;
    }
}

bool RtspOut::writeHeader()
{
    int ret;
    AVDictionary *options = NULL;

    if (isHls(m_uri)) {
        std::string::size_type pos1 = m_uri.rfind('/');
        if (pos1 == std::string::npos) {
            ELOG_ERROR("Cant not find base url %s", m_uri.c_str());
            return false;
        }

        std::string::size_type pos2 = m_uri.rfind('.');
        if (pos2 == std::string::npos) {
            ELOG_ERROR("Cant not find base url %s", m_uri.c_str());
            return false;
        }

        if (pos2 <= pos1) {
            ELOG_ERROR("Cant not find base url %s", m_uri.c_str());
            return false;
        }

        std::string segment_uri(m_uri.substr(0, pos1));
        segment_uri.append("/intel_");
        segment_uri.append(m_uri.substr(pos1 + 1, pos2 - pos1 - 1));
        segment_uri.append("_%09d.ts");

        ELOG_TRACE("index url %s", m_uri.c_str());
        ELOG_TRACE("segment url %s", segment_uri.c_str());

        av_dict_set(&options, "hls_segment_filename", segment_uri.c_str(), 0);
        av_dict_set(&options, "hls_time", "10", 0);
        av_dict_set(&options, "hls_list_size", "4", 0);
        av_dict_set(&options, "hls_flags", "delete_segments", 0);
        av_dict_set(&options, "method", "PUT", 0);
    }

    ret = avformat_write_header(m_context, &options);
    if (ret < 0) {
        ELOG_ERROR("Cannot write header, %s", ff_err2str(ret));
        return false;
    }

    av_dump_format(m_context, 0, m_context->filename, 1);
    return true;
}

int RtspOut::writeAVFrame(AVStream* stream, const EncodedFrame& frame)
{
    int ret;
    AVPacket pkt;

    av_init_packet(&pkt);
    pkt.data = frame.m_payloadData;
    pkt.size = frame.m_payloadSize;
    pkt.dts = (int64_t)(frame.m_timeStamp / (av_q2d(stream->time_base) * 1000));
    pkt.pts = pkt.dts;
    pkt.stream_index = stream->index;

    if (stream == m_videoStream) {
        pkt.flags = isH264KeyFrame(frame.m_payloadData, frame.m_payloadSize) ? AV_PKT_FLAG_KEY : 0;

        if (pkt.flags == AV_PKT_FLAG_KEY) {
            m_lastKeyFrameReqTime = frame.m_timeStamp;
        }

        if (frame.m_timeStamp - m_lastKeyFrameReqTime > KEYFRAME_REQ_INTERVAL) {
            m_lastKeyFrameReqTime = frame.m_timeStamp;

            ELOG_DEBUG("Request video key frame");
            deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME});
        }
    }

    ELOG_DEBUG("Send %s frame, timestamp %ld, size %4d%s"
            , stream == m_audioStream ? "audio" : "video"
            , pkt.dts
            , pkt.size
            , pkt.flags == AV_PKT_FLAG_KEY ? " - key" : ""
            );

    ret = av_interleaved_write_frame(m_context, &pkt);
    if (ret < 0)
        ELOG_ERROR("Cannot write frame, %s", ff_err2str(ret));

    return ret;
}

char *RtspOut::ff_err2str(int errRet)
{
    av_strerror(errRet, (char*)(&m_errbuff), 500);
    return m_errbuff;
}

} /* namespace mcu */
