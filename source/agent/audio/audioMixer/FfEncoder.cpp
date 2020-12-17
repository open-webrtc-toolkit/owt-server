// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioUtilities.h"
#include "FfEncoder.h"

#include "AudioTime.h"

namespace mcu {

using namespace webrtc;
using namespace owt_base;

static enum AVSampleFormat getCodecPreferedSampleFmt(AVCodec *codec, enum AVSampleFormat PreferedSampleFmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == PreferedSampleFmt)
            return PreferedSampleFmt;
        p++;
    }
    return codec->sample_fmts[0];
}

DEFINE_LOGGER(FfEncoder, "mcu.media.FfEncoder");

FfEncoder::FfEncoder(const FrameFormat format)
    : m_format(format)
    , m_timestampOffset(0)
    , m_valid(false)
    , m_channels(0)
    , m_sampleRate(0)
    , m_audioEnc(NULL)
    , m_audioFifo(NULL)
    , m_audioFrame(NULL)
{
    if (ELOG_IS_TRACE_ENABLED())
        av_log_set_level(AV_LOG_DEBUG);
    else if (ELOG_IS_DEBUG_ENABLED())
        av_log_set_level(AV_LOG_INFO);
    else if (ELOG_IS_INFO_ENABLED())
        av_log_set_level(AV_LOG_WARNING);
    else
        av_log_set_level(AV_LOG_QUIET);

    m_sampleRate = getAudioSampleRate(format);
    m_channels = getAudioChannels(format);
}

FfEncoder::~FfEncoder()
{
    if (!m_valid)
        return;

    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
        m_audioFrame = NULL;
    }

    if (m_audioFifo) {
        av_audio_fifo_free(m_audioFifo);
        m_audioFifo = NULL;
    }

    if (m_audioEnc) {
        avcodec_free_context(&m_audioEnc);
        m_audioEnc = NULL;
    }

    m_format = FRAME_FORMAT_UNKNOWN;
}

bool FfEncoder::init()
{
    if(!initEncoder(m_format)) {
        return false;
    }

    //m_timestampOffset = currentTimeMs();
    m_valid = true;

    return true;
}

bool FfEncoder::initEncoder(const FrameFormat format)
{
    int ret;
    AVCodec* codec = NULL;

    switch(format) {
        case FRAME_FORMAT_AAC_48000_2:
            codec = avcodec_find_encoder_by_name("libfdk_aac");
            if (!codec) {
                ELOG_ERROR_T("Can not find audio encoder %s, please check if ffmpeg/libfdk_aac installed", "libfdk_aac");
                return false;
            }
            break;
        case FRAME_FORMAT_OPUS:
            codec = avcodec_find_encoder(AV_CODEC_ID_OPUS);
            if (!codec) {
                ELOG_ERROR_T("Could not find audio encoder %s", avcodec_get_name(AV_CODEC_ID_OPUS));
                return false;
            }
            break;
        default:
            ELOG_ERROR_T("Encoder %s is not supported", getFormatStr(format));
            return false;
    }

    // context
    m_audioEnc = avcodec_alloc_context3(codec);
    if (!m_audioEnc) {
        ELOG_ERROR_T("Can not alloc avcodec context");
        return false;
    }

    m_audioEnc->channels        = m_channels;
    m_audioEnc->channel_layout  = av_get_default_channel_layout(m_audioEnc->channels);
    m_audioEnc->sample_rate     = m_sampleRate;
    m_audioEnc->sample_fmt      = getCodecPreferedSampleFmt(codec, AV_SAMPLE_FMT_S16);
    m_audioEnc->flags           |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(m_audioEnc, codec, nullptr);
    if (ret < 0) {
        ELOG_ERROR_T("Cannot open output audio codec, %s", ff_err2str(ret));
        goto fail;
    }

    // fifo
    if (m_audioFifo) {
        av_audio_fifo_free(m_audioFifo);
        m_audioFifo = NULL;
    }

    m_audioFifo = av_audio_fifo_alloc(m_audioEnc->sample_fmt, m_audioEnc->channels, 1);
    if (!m_audioFifo) {
        ELOG_ERROR_T("Cannot allocate audio fifo");
        goto fail;
    }

    // frame
    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
        m_audioFrame = NULL;
    }

    m_audioFrame  = av_frame_alloc();
    if (!m_audioFrame) {
        ELOG_ERROR_T("Cannot allocate audio frame");
        goto fail;
    }

    m_audioFrame->nb_samples        = m_audioEnc->frame_size;
    m_audioFrame->format            = m_audioEnc->sample_fmt;
    m_audioFrame->channel_layout    = m_audioEnc->channel_layout;
    m_audioFrame->sample_rate       = m_audioEnc->sample_rate;

    ret = av_frame_get_buffer(m_audioFrame, 0);
    if (ret < 0) {
        ELOG_ERROR_T("Cannot get audio frame buffer, %s", ff_err2str(ret));
        goto fail;
    }

    ELOG_DEBUG_T("Audio encoder frame_size %d, sample_rate %d, channels %d",
            m_audioEnc->frame_size, m_audioEnc->sample_rate, m_audioEnc->channels);
    return true;

fail:
    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
        m_audioFrame = NULL;
    }

    if (m_audioFifo) {
        av_audio_fifo_free(m_audioFifo);
        m_audioFifo = NULL;
    }

    if (m_audioEnc) {
        avcodec_free_context(&m_audioEnc);
        m_audioEnc = NULL;
    }

    return false;
}

bool FfEncoder::addToFifo(const AudioFrame* audioFrame)
{
    uint32_t n;

    if (audioFrame->sample_rate_hz_ != m_sampleRate ||
            (int32_t)audioFrame->num_channels_ != m_channels) {

        ELOG_ERROR_T("Invalid audio frame, %s(%d-%ld), want(%d-%d)",
                getFormatStr(m_format),
                audioFrame->sample_rate_hz_,
                audioFrame->num_channels_,
                m_sampleRate,
                m_channels
                );
        return false;
    }

    void *data = reinterpret_cast<void*>((const_cast<int16_t *>(audioFrame->data_)));

    n = av_audio_fifo_write(m_audioFifo, &data, audioFrame->samples_per_channel_);
    if (n < audioFrame->samples_per_channel_) {
        ELOG_ERROR("Cannot not write data to fifo, bnSamples %ld, writed %d", audioFrame->samples_per_channel_, n);
        return false;
    }

    return true;
}

void FfEncoder::encode()
{
    AVPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    while (av_audio_fifo_size(m_audioFifo) >= m_audioEnc->frame_size) {
        int ret;
        int32_t n;

        n = av_audio_fifo_read(m_audioFifo, reinterpret_cast<void**>(m_audioFrame->data), m_audioEnc->frame_size);
        if (n != m_audioEnc->frame_size) {
            ELOG_ERROR("Cannot read enough data from fifo, needed %d, read %d", m_audioEnc->frame_size, n);
            return;
        }

        ret = avcodec_send_frame(m_audioEnc, m_audioFrame);
        if (ret < 0) {
            ELOG_ERROR("avcodec_send_frame, %s", ff_err2str(ret));
            return;
        }

        av_init_packet(&pkt);
        ret = avcodec_receive_packet(m_audioEnc, &pkt);
        if (ret < 0) {
            ELOG_ERROR("avcodec_receive_packet, %s", ff_err2str(ret));
            return;
        }

        sendOut(pkt);
        av_packet_unref(&pkt);
    }
}

void FfEncoder::sendOut(AVPacket &pkt)
{
    owt_base::Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = m_format;
    frame.payload = const_cast<uint8_t *>(pkt.data);
    frame.length = pkt.size;
    frame.additionalInfo.audio.nbSamples = m_audioEnc->frame_size;
    frame.additionalInfo.audio.sampleRate = m_audioEnc->sample_rate;
    frame.additionalInfo.audio.channels = m_audioEnc->channels;
    frame.timeStamp = (AudioTime::currentTime()) * frame.additionalInfo.audio.sampleRate / 1000;

    ELOG_TRACE_T("deliverFrame(%s), sampleRate(%d), channels(%d), timeStamp(%d), length(%d), %s",
            getFormatStr(frame.format),
            frame.additionalInfo.audio.sampleRate,
            frame.additionalInfo.audio.channels,
            frame.timeStamp * 1000 / frame.additionalInfo.audio.sampleRate,
            frame.length,
            frame.additionalInfo.audio.isRtpPacket ? "RtpPacket" : "NonRtpPacket"
            );

    deliverFrame(frame);
}

bool FfEncoder::addAudioFrame(const AudioFrame* audioFrame)
{
    if (!m_valid)
        return false;

    if (!audioFrame) {
        return false;
    }

    if (!addToFifo(audioFrame))
        return false;

    encode();
    return true;
}


char *FfEncoder::ff_err2str(int errRet)
{
    av_strerror(errRet, (char*)(&m_errbuff), 500);
    return m_errbuff;
}

} /* namespace mcu */
