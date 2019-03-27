// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
#include "AVStreamOut.h"

namespace owt_base {

inline AVCodecID frameFormat2AVCodecID(int frameFormat)
{
    switch (frameFormat) {
        case FRAME_FORMAT_VP8:
            return AV_CODEC_ID_VP8;
        case FRAME_FORMAT_VP9:
            return AV_CODEC_ID_VP9;
        case FRAME_FORMAT_H264:
            return AV_CODEC_ID_H264;
        case FRAME_FORMAT_H265:
            return AV_CODEC_ID_H265;
        case FRAME_FORMAT_PCMU:
            return AV_CODEC_ID_PCM_MULAW;
        case FRAME_FORMAT_PCMA:
            return AV_CODEC_ID_PCM_ALAW;
        case FRAME_FORMAT_OPUS:
            return AV_CODEC_ID_OPUS;
        case FRAME_FORMAT_AAC:
        case FRAME_FORMAT_AAC_48000_2:
            return AV_CODEC_ID_AAC;
        default:
            return AV_CODEC_ID_NONE;
    }
}

DEFINE_LOGGER(AVStreamOut, "owt.AVStreamOut");

AVStreamOut::AVStreamOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry *handle, int timeout)
    : m_status(Context_EMPTY)
    , m_url(url)
    , m_hasAudio(hasAudio)
    , m_hasVideo(hasVideo)
    , m_asyncHandle(handle)
    , m_timeOutMs(timeout)
    , m_audioFormat(FRAME_FORMAT_UNKNOWN)
    , m_sampleRate(0)
    , m_channels(0)
    , m_videoFormat(FRAME_FORMAT_UNKNOWN)
    , m_width(0)
    , m_height(0)
    , m_videoSourceChanged(true)
    , m_context(NULL)
    , m_audioStream(NULL)
    , m_videoStream(NULL)
    , m_lastKeyFrameTimestamp(0)
{
    ELOG_INFO("url %s, audio %d, video %d, timeOut %d", m_url.c_str(), m_hasAudio, m_hasVideo, m_timeOutMs);

    if (!m_hasAudio && !m_hasVideo) {
        ELOG_ERROR("Audio/Video not enabled");
        notifyAsyncEvent("init", "Audio/Video not enabled");
        return;
    }

    if (ELOG_IS_TRACE_ENABLED())
        av_log_set_level(AV_LOG_DEBUG);
    else if (ELOG_IS_DEBUG_ENABLED())
        av_log_set_level(AV_LOG_INFO);
    else if (ELOG_IS_INFO_ENABLED())
        av_log_set_level(AV_LOG_WARNING);
    else
        av_log_set_level(AV_LOG_QUIET);

    m_status = Context_INITIALIZING;
    notifyAsyncEvent("init", "");

    m_thread = boost::thread(&AVStreamOut::sendLoop, this);
}

AVStreamOut::~AVStreamOut()
{
}

void AVStreamOut::onFrame(const owt_base::Frame& frame)
{
    if (isAudioFrame(frame)) {
        if (!m_hasAudio) {
            ELOG_ERROR("Audio is not enabled");
            notifyAsyncEvent("fatal", "Audio is not enabled");
            return;
        }

        if (!isAudioFormatSupported(frame.format)) {
            ELOG_ERROR("Unsupported audio frame format: %s(%d)", getFormatStr(frame.format), frame.format);
            notifyAsyncEvent("fatal", "Unsupported audio frame format");
            return;
        }

        if (m_audioFormat == FRAME_FORMAT_UNKNOWN) {
            ELOG_INFO("Initial audio options format(%s), sample rate(%d), channels(%d), isRtpPacket(%d)"
                    , getFormatStr(frame.format)
                    , frame.additionalInfo.audio.sampleRate
                    , frame.additionalInfo.audio.channels
                    , frame.additionalInfo.audio.isRtpPacket
                    );

            m_sampleRate    = frame.additionalInfo.audio.sampleRate;
            m_channels      = frame.additionalInfo.audio.channels;
            m_audioFormat   = frame.format;
        }

        if (m_audioFormat != frame.format) {
            ELOG_ERROR("Expected codec(%s), got(%s)", getFormatStr(m_audioFormat), getFormatStr(frame.format));
            notifyAsyncEvent("fatal", "Unexpected audio codec");
            return;
        }

        if (m_status != AVStreamOut::Context_READY)
            return;

        if (m_channels != frame.additionalInfo.audio.channels
                || m_sampleRate != frame.additionalInfo.audio.sampleRate) {
            ELOG_ERROR("Invalid audio frame channels %d, or sample rate: %d"
                    , frame.additionalInfo.audio.channels, frame.additionalInfo.audio.sampleRate);

            notifyAsyncEvent("fatal", "Invalid audio frame channels or sample rate");
            return;
        }
        m_frameQueue.pushFrame(frame);
    } else if (isVideoFrame(frame)) {
        if (!m_hasVideo) {
            ELOG_ERROR("Video is not enabled");
            notifyAsyncEvent("fatal", "Video is not enabled");
            return;
        }

        if (!isVideoFormatSupported(frame.format)) {
            ELOG_ERROR("Unsupported video frame format: %s(%d)", getFormatStr(frame.format), frame.format);
            notifyAsyncEvent("fatal", "Unsupported video frame format");
            return;
        }

        if (m_videoFormat == FRAME_FORMAT_UNKNOWN) {
            if (!frame.additionalInfo.video.isKeyFrame) {
                ELOG_DEBUG("Request video key frame for initialization");
                deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME});
                return;
            }

            ELOG_INFO("Initial video options: format(%s), %dx%d",
                    getFormatStr(frame.format),
                    frame.additionalInfo.video.width, frame.additionalInfo.video.height);

            m_videoSourceChanged = false;
            m_videoKeyFrame.reset(new MediaFrame(frame));

            m_width         = frame.additionalInfo.video.width;
            m_height        = frame.additionalInfo.video.height;
            m_videoFormat   = frame.format;
        }

        if (m_videoFormat != frame.format) {
            ELOG_ERROR("Expected codec(%s), got(%s)", getFormatStr(m_videoFormat), getFormatStr(frame.format));
            notifyAsyncEvent("fatal", "Unexpected video codec");
            return;
        }

        if (!m_videoSourceChanged
                && (m_width != frame.additionalInfo.video.width || m_height != frame.additionalInfo.video.height)) {

            ELOG_DEBUG("Video resolution changed %dx%d -> %dx%d",
                    m_width, m_height,
                    frame.additionalInfo.video.width, frame.additionalInfo.video.height
                    );

            m_videoSourceChanged = true;
        }

        if (m_videoSourceChanged) {
            if (!frame.additionalInfo.video.isKeyFrame) {
                ELOG_DEBUG("Request video key frame for video changed");
                deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME});
                return;
            }

            ELOG_DEBUG("Ready after video changed: format(%s), %dx%d",
                    getFormatStr(frame.format),
                    frame.additionalInfo.video.width, frame.additionalInfo.video.height);

            m_videoSourceChanged = false;
            m_videoKeyFrame.reset(new MediaFrame(frame));

            m_width         = frame.additionalInfo.video.width;
            m_height        = frame.additionalInfo.video.height;
            m_videoFormat   = frame.format;
        }

#if 0 // dont drop key frame
        if (m_status != AVStreamOut::Context_READY)
            return;
#endif

        m_frameQueue.pushFrame(frame);
    } else {
        ELOG_WARN("Unsupported frame format: %s(%d)", getFormatStr(frame.format), frame.format);
        notifyAsyncEvent("fatal", "Unsupported frame format");
    }
}

void AVStreamOut::sendLoop()
{
    uint32_t connectRetry;

    const uint32_t waitMs = 20;
    uint32_t timeOut = 0;
    while ((m_hasAudio && m_audioFormat == FRAME_FORMAT_UNKNOWN) || (m_hasVideo && m_videoFormat == FRAME_FORMAT_UNKNOWN)) {
        if (m_status == AVStreamOut::Context_CLOSED)
            goto exit;

        if (timeOut >= m_timeOutMs) {
            ELOG_ERROR("No a/v frames, hasAudio(%d) - ready(%d), hasVideo(%d) - ready(%d), timeOutMs %d"
                    , m_hasAudio
                    , (m_audioFormat != FRAME_FORMAT_UNKNOWN)
                    , m_hasVideo
                    , (m_videoFormat != FRAME_FORMAT_UNKNOWN)
                    , m_timeOutMs);
            notifyAsyncEvent("fatal", "No a/v frames");
            goto exit;
        }

        timeOut += waitMs;
        usleep(waitMs * 1000);

        ELOG_DEBUG("Wait for av options available, hasAudio(%d) - ready(%d), hasVideo(%d) - ready(%d), waitMs %d"
                , m_hasAudio, m_audioFormat != FRAME_FORMAT_UNKNOWN, m_hasVideo, m_videoFormat != FRAME_FORMAT_UNKNOWN, timeOut);
    }

     connectRetry = getReconnectCount();
reconnect:
    if(!connect()) {
        notifyAsyncEvent("init", "Cannot open connection");
        goto exit;
    }

    if (m_hasAudio && !addAudioStream(m_audioFormat, m_sampleRate, m_channels)) {
        notifyAsyncEvent("fatal", "Cannot add audio stream");
        goto exit;
    }
    if (m_hasVideo && !addVideoStream(m_videoFormat, m_width, m_height)) {
        notifyAsyncEvent("fatal", "Cannot add video stream");
        goto exit;
    }
    if (!writeHeader()) {
        notifyAsyncEvent("fatal", "Cannot write header");
        goto exit;
    }
    av_dump_format(m_context, 0, m_context->url, 1);

    m_status = AVStreamOut::Context_READY;

    ELOG_DEBUG("Start");
    while (m_status == AVStreamOut::Context_READY) {
        boost::shared_ptr<owt_base::MediaFrame> mediaFrame = m_frameQueue.popFrame(2000);
        if (!mediaFrame) {
            if (m_status == AVStreamOut::Context_READY) {
                ELOG_WARN("No input frames available");
                notifyAsyncEvent("fatal", "No input frames available");
            }
            break;
        }

        bool ret = writeFrame(isVideoFrame(mediaFrame->m_frame) ? m_videoStream : m_audioStream, mediaFrame);
        if (!ret) {
            if (connectRetry-- > 0) {
                ELOG_WARN("Try to reconnect");
                av_write_trailer(m_context);
                disconnect();
                goto reconnect;
            } else {
                notifyAsyncEvent("fatal", "Cannot write frame");
                break;
            }
        }
    }
    av_write_trailer(m_context);

exit:
    m_status = AVStreamOut::Context_CLOSED;
    ELOG_DEBUG("Thread exited!");
}

bool AVStreamOut::connect()
{
    const char *formatName = getFormatName(m_url);

    avformat_alloc_output_context2(&m_context, NULL, formatName, m_url.c_str());
    if (!m_context) {
        ELOG_ERROR("Cannot allocate output context, format(%s), url(%s)", formatName ? formatName : "", m_url.c_str());
        return false;
    }

    if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&m_context->pb, m_context->url, AVIO_FLAG_WRITE);
        if (ret < 0) {
            ELOG_ERROR("Cannot open avio, %s", ff_err2str(ret));

            avformat_free_context(m_context);
            m_context = NULL;
            return false;
        }
    }

    return true;
}

void AVStreamOut::disconnect()
{
    if (m_context) {
        if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
            avio_close(m_context->pb);
        }
        avformat_free_context(m_context);
        m_context = NULL;
    }
}

void AVStreamOut::close()
{
    ELOG_INFO("Close %s", m_url.c_str());

    m_status = AVStreamOut::Context_CLOSED;
    m_frameQueue.cancel();
    m_thread.join();

    disconnect();
}

bool AVStreamOut::addAudioStream(FrameFormat format, uint32_t sampleRate, uint32_t channels)
{
    enum AVCodecID codec_id = frameFormat2AVCodecID(format);
    m_audioStream = avformat_new_stream(m_context, NULL);
    if (!m_audioStream) {
        ELOG_ERROR("Cannot add audio stream");
        return false;
    }

    AVCodecParameters *par = m_audioStream->codecpar;
    par->codec_type     = AVMEDIA_TYPE_AUDIO;
    par->codec_id       = codec_id;
    par->sample_rate    = sampleRate;
    par->channels       = channels;
    par->channel_layout = av_get_default_channel_layout(par->channels);
    switch(par->codec_id) {
        case AV_CODEC_ID_AAC: //AudioSpecificConfig 48000-2
            par->extradata_size = 2;
            par->extradata      = (uint8_t *)av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            par->extradata[0]   = 0x11;
            par->extradata[1]   = 0x90;
            break;
        case AV_CODEC_ID_OPUS: //OpusHead 48000-2
            par->extradata_size = 19;
            par->extradata      = (uint8_t *)av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            par->extradata[0]   = 'O';
            par->extradata[1]   = 'p';
            par->extradata[2]   = 'u';
            par->extradata[3]   = 's';
            par->extradata[4]   = 'H';
            par->extradata[5]   = 'e';
            par->extradata[6]   = 'a';
            par->extradata[7]   = 'd';
            //Version
            par->extradata[8]   = 1;
            //Channel Count
            par->extradata[9]   = 2;
            //Pre-skip
            par->extradata[10]  = 0x38;
            par->extradata[11]  = 0x1;
            //Input Sample Rate (Hz)
            par->extradata[12]  = 0x80;
            par->extradata[13]  = 0xbb;
            par->extradata[14]  = 0;
            par->extradata[15]  = 0;
            //Output Gain (Q7.8 in dB)
            par->extradata[16]  = 0;
            par->extradata[17]  = 0;
            //Mapping Family
            par->extradata[18]  = 0;
            break;
        default:
            break;
    }

    return true;
}

bool AVStreamOut::addVideoStream(FrameFormat format, uint32_t width, uint32_t height)
{
    enum AVCodecID codec_id = frameFormat2AVCodecID(format);
    m_videoStream = avformat_new_stream(m_context, NULL);
    if (!m_videoStream) {
        ELOG_ERROR("Cannot add video stream");
        return false;
    }

    AVCodecParameters *par = m_videoStream->codecpar;
    par->codec_type = AVMEDIA_TYPE_VIDEO;
    par->codec_id   = codec_id;
    par->width      = width;
    par->height     = height;
    if (codec_id == AV_CODEC_ID_H264 || codec_id == AV_CODEC_ID_H265) { //extradata
        AVCodecParserContext *parser = av_parser_init(codec_id);
        if (!parser) {
            ELOG_ERROR("Cannot find video parser");
            return false;
        }

        int size = parser->parser->split(NULL, m_videoKeyFrame->m_frame.payload, m_videoKeyFrame->m_frame.length);
        if (size > 0) {
            par->extradata_size = size;
            par->extradata      = (uint8_t *)av_malloc(par->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            memcpy(par->extradata, m_videoKeyFrame->m_frame.payload, par->extradata_size);
        } else {
            ELOG_WARN("Cannot find video extradata");
        }

        av_parser_close(parser);
    }

    if (codec_id == AV_CODEC_ID_H265) {
        par->codec_tag = 0x31637668; //hvc1
    }

    return true;
}

bool AVStreamOut::writeHeader()
{
    int ret;
    AVDictionary *options = NULL;

    ret = getHeaderOpt(m_url, &options);
    if (!ret) {
        ELOG_ERROR("Cannot get header options");
        return false;
    }

    ret = avformat_write_header(m_context, options != NULL ? &options : NULL);
    if (ret < 0) {
        ELOG_ERROR("Cannot write header, %s", ff_err2str(ret));
        return false;
    }

    return true;
}

bool AVStreamOut::writeFrame(AVStream *stream, boost::shared_ptr<MediaFrame> mediaFrame)
{
    int ret;
    AVPacket pkt;

    if (stream == NULL || mediaFrame == NULL)
        return false;

    av_init_packet(&pkt);
    pkt.data = mediaFrame->m_frame.payload;
    pkt.size = mediaFrame->m_frame.length;
    pkt.dts = (int64_t)(mediaFrame->m_timeStamp / (av_q2d(stream->time_base) * 1000));
    pkt.pts = pkt.dts;
    pkt.duration =  (int64_t)(mediaFrame->m_duration / (av_q2d(stream->time_base) * 1000));
    pkt.stream_index = stream->index;

    if (isVideoFrame(mediaFrame->m_frame)) {
        if (m_lastKeyFrameTimestamp == 0)
            m_lastKeyFrameTimestamp = currentTimeMs();

        if (mediaFrame->m_frame.additionalInfo.video.isKeyFrame) {
            pkt.flags |= AV_PKT_FLAG_KEY;
            m_lastKeyFrameTimestamp = currentTimeMs();
        }

        if (m_lastKeyFrameTimestamp + 1.1 * getKeyFrameInterval() < currentTimeMs()) {
            ELOG_DEBUG("Request video key frame");

            deliverFeedbackMsg(FeedbackMsg{.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME});
            m_lastKeyFrameTimestamp = currentTimeMs();
        }
    }

    ELOG_TRACE("Send %s frame, timestamp %ld, duration %ld, size %4d%s"
            , isVideoFrame(mediaFrame->m_frame) ? "video" : "audio"
            , pkt.dts
            , pkt.duration
            , pkt.size
            , (pkt.flags & AV_PKT_FLAG_KEY) ? " - key" : ""
            );

    ret = av_interleaved_write_frame(m_context, &pkt);
    if (ret < 0)
        ELOG_ERROR("Cannot write frame, %s", ff_err2str(ret));

    return ret >= 0 ? true : false;
}

char *AVStreamOut::ff_err2str(int errRet)
{
    av_strerror(errRet, (char*)(&m_errbuff), 500);
    return m_errbuff;
}

}
