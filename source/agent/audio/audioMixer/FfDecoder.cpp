// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <rtputils.h>

#include "AudioUtilities.h"
#include "FfDecoder.h"

namespace mcu {

using namespace webrtc;
using namespace owt_base;

DEFINE_LOGGER(FfDecoder, "mcu.media.FfDecoder");

FfDecoder::FfDecoder(const FrameFormat format)
    : m_format(format)
    , m_valid(false)
    , m_decCtx(NULL)
    , m_decFrame(NULL)
    , m_needResample(false)
    , m_swrCtx(nullptr)
    , m_swrSamplesData(nullptr)
    , m_swrSamplesLinesize(0)
    , m_swrSamplesCount(0)
    , m_swrInitialised(false)
    , m_audioFifo(NULL)
    , m_audioFrame(NULL)
    , m_inSampleFormat(AV_SAMPLE_FMT_NONE)
    , m_inSampleRate(0)
    , m_inChannels(0)
    , m_outSampleFormat(AV_SAMPLE_FMT_S16)
    , m_outSampleRate(0)
    , m_outChannels(0)
    , m_timestamp(0)
#if 1
    , m_outFormat(FRAME_FORMAT_PCM_48000_2)
#else
    , m_outFormat(FRAME_FORMAT_OPUS)
#endif
{
    if (ELOG_IS_TRACE_ENABLED())
        av_log_set_level(AV_LOG_DEBUG);
    else if (ELOG_IS_DEBUG_ENABLED())
        av_log_set_level(AV_LOG_INFO);
    else if (ELOG_IS_INFO_ENABLED())
        av_log_set_level(AV_LOG_WARNING);
    else
        av_log_set_level(AV_LOG_QUIET);

    m_outSampleRate = getAudioSampleRate(m_outFormat);
    m_outChannels = getAudioChannels(m_outFormat);
}

FfDecoder::~FfDecoder()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    if (m_audioFifo) {
        av_audio_fifo_free(m_audioFifo);
        m_audioFifo = NULL;
    }
    if (m_audioFrame) {
        av_frame_free(&m_audioFrame );
        m_audioFrame = NULL;
    }

    if (m_swrCtx) {
        swr_free(&m_swrCtx);
        m_swrCtx = NULL;
    }
    if (m_swrSamplesData) {
        av_freep(&m_swrSamplesData[0]);
        av_freep(&m_swrSamplesData);
        m_swrSamplesData = NULL;

        m_swrSamplesLinesize = 0;
    }
    m_swrSamplesCount = 0;

    if (m_decFrame) {
        av_frame_free(&m_decFrame);
        m_decFrame = NULL;
    }

    if (m_decCtx) {
        avcodec_free_context(&m_decCtx);
        m_decCtx = NULL;
    }

    if (m_input) {
        this->removeAudioDestination(m_input.get());
        m_input.reset();
    }
}

bool FfDecoder::init()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    if (m_outFormat != FRAME_FORMAT_PCM_48000_2) {
        m_input.reset(new AcmDecoder(m_outFormat));
        if (!m_input->init()) {
            m_input.reset();
            return false;
        }

        //m_output.reset(new AcmEncoder(m_outFormat));
        m_output.reset(new FfEncoder(m_outFormat));
        if (!m_output->init()) {
            m_output.reset();
            return false;
        }

        m_output->addAudioDestination(m_input.get());
    } else {
        m_input.reset(new AcmDecoder(m_outFormat));
        if (!m_input->init()) {
            m_input.reset();
            return false;
        }

        this->addAudioDestination(m_input.get());
    }

    m_valid = true;
    return true;
}

bool FfDecoder::initDecoder(FrameFormat format, uint32_t sampleRate, uint32_t channels)
{
    int ret;
    AVCodecID decId;
    AVCodec *dec = NULL;

    switch(format) {
        case FRAME_FORMAT_AAC:
        case FRAME_FORMAT_AAC_48000_2:
            decId = AV_CODEC_ID_AAC;
            break;
        case FRAME_FORMAT_AC3:
            decId = AV_CODEC_ID_AC3;
            break;
        case FRAME_FORMAT_NELLYMOSER:
            decId = AV_CODEC_ID_NELLYMOSER;
            break;
        default:
            ELOG_WARN_T("Invalid format(%s)", getFormatStr(format));
            return false;
    }

    dec = avcodec_find_decoder(decId);
    if (!dec) {
        ELOG_ERROR_T("Could not find ffmpeg decoder %s", avcodec_get_name(decId));
        return false;
    }

    m_decCtx = avcodec_alloc_context3(dec);
    if (!m_decCtx ) {
        ELOG_ERROR_T("Could not alloc ffmpeg decoder context");
        return false;
    }

    m_decCtx->sample_fmt        = AV_SAMPLE_FMT_FLT;
    m_decCtx->sample_rate       = sampleRate;
    m_decCtx->channels          = channels;
    m_decCtx->channel_layout    = av_get_default_channel_layout(m_decCtx->channels);

    ret = avcodec_open2(m_decCtx, dec , NULL);
    if (ret < 0) {
        ELOG_ERROR_T("Could not open ffmpeg decoder context, %s", ff_err2str(ret));
        return false;
    }

    m_decFrame = av_frame_alloc();
    if (!m_decFrame) {
        ELOG_ERROR_T("Could not allocate dec frame");
        return false;
    }

    memset(&m_packet, 0, sizeof(m_packet));

    m_inSampleFormat    = m_decCtx->sample_fmt;
    m_inSampleRate      = m_decCtx->sample_rate;
    m_inChannels        = m_decCtx->channels;

    return true;
}

bool FfDecoder::initResampler(enum AVSampleFormat inSampleFormat, int inSampleRate, int inChannels,
        enum AVSampleFormat outSampleFormat, int outSampleRate, int outChannels)
{
    int ret;

    if (inSampleFormat == outSampleFormat && inSampleRate == outSampleRate && inChannels == outChannels) {
        m_needResample = false;
        return true;
    }

    m_needResample = true;

    ELOG_INFO_T("Init resampler %s-%d-%d -> %s-%d-%d"
            , av_get_sample_fmt_name(inSampleFormat)
            , inSampleRate
            , inChannels
            , av_get_sample_fmt_name(outSampleFormat)
            , outSampleRate
            , outChannels
            );

    m_swrCtx = swr_alloc();
    if (!m_swrCtx) {
        ELOG_ERROR_T("Could not allocate resampler context");
        return false;
    }

    /* set options */
    av_opt_set_sample_fmt(m_swrCtx, "in_sample_fmt",      inSampleFormat,       0);
    av_opt_set_int       (m_swrCtx, "in_sample_rate",     inSampleRate,         0);
    av_opt_set_int       (m_swrCtx, "in_channel_count",   inChannels,           0);
    av_opt_set_sample_fmt(m_swrCtx, "out_sample_fmt",     outSampleFormat,    0);
    av_opt_set_int       (m_swrCtx, "out_sample_rate",    outSampleRate,      0);
    av_opt_set_int       (m_swrCtx, "out_channel_count",  outChannels,        0);

    ret = swr_init(m_swrCtx);
    if (ret < 0) {
        ELOG_ERROR_T("Fail to initialize the resampling context, %s", ff_err2str(ret));

        swr_free(&m_swrCtx);
        m_swrCtx = NULL;
        return false;
    }

    m_swrSamplesCount = 2048;
    ret = av_samples_alloc_array_and_samples(&m_swrSamplesData, &m_swrSamplesLinesize, outChannels,
            m_swrSamplesCount, outSampleFormat, 0);
    if (ret < 0) {
        ELOG_ERROR_T("Could not allocate swr samples data, %s", ff_err2str(ret));

        swr_free(&m_swrCtx);
        m_swrCtx = NULL;
        return false;
    }

    return true;
}

bool FfDecoder::initFifo(enum AVSampleFormat sampleFmt, uint32_t sampleRate, uint32_t channels)
{
    int ret;

    m_audioFifo = av_audio_fifo_alloc(sampleFmt, channels, 1);
    if (!m_audioFifo) {
        ELOG_ERROR_T("Cannot allocate audio fifo");
        return false;
    }

    m_audioFrame  = av_frame_alloc();
    if (!m_audioFrame) {
        ELOG_ERROR_T("Cannot allocate audio frame");
        return false;
    }

    m_audioFrame->nb_samples        = sampleRate / 100; //10ms
    m_audioFrame->format            = sampleFmt;
    m_audioFrame->channel_layout    = av_get_default_channel_layout(channels);
    m_audioFrame->sample_rate       = sampleRate;

    ret = av_frame_get_buffer(m_audioFrame, 0);
    if (ret < 0) {
        ELOG_ERROR_T("Cannot get audio frame buffer, %s", ff_err2str(ret));
        return false;
    }

    return true;
}

bool FfDecoder::resampleFrame(AVFrame *frame, uint8_t **pOutData, int *pOutNbSamples)
{
    int ret;
    int dst_nb_samples;

    if (!m_swrCtx)
        return false;

    /* compute destination number of samples */
    dst_nb_samples = av_rescale_rnd(
            swr_get_delay(m_swrCtx, m_inSampleRate) + frame->nb_samples
            , m_outSampleRate
            , m_inSampleRate
            , AV_ROUND_UP);

    if (dst_nb_samples > m_swrSamplesCount) {
        int newSize = 2 * dst_nb_samples;

        ELOG_INFO_T("Realloc audio swr samples buffer %d -> %d", m_swrSamplesCount, newSize);

        av_freep(&m_swrSamplesData[0]);
        ret = av_samples_alloc(m_swrSamplesData, &m_swrSamplesLinesize, m_outChannels,
                newSize, m_outSampleFormat, 1);
        if (ret < 0) {
            ELOG_ERROR_T("Fail to realloc swr samples, %s", ff_err2str(ret));
            return false;
        }
        m_swrSamplesCount = newSize;
    }

    /* convert to destination format */
    ret = swr_convert(m_swrCtx, m_swrSamplesData, dst_nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
    if (ret < 0) {
        ELOG_ERROR_T("Error while converting, %s", ff_err2str(ret));
        return false;
    }

    *pOutData       = m_swrSamplesData[0];
    *pOutNbSamples  = ret;
    return true;
}

bool FfDecoder::addFrameToFifo(AVFrame *frame)
{
    uint8_t *data;
    int samples_per_channel;

    if (m_needResample) {
         if (!resampleFrame(frame, &data, &samples_per_channel))
            return false;
    } else {
        data = (uint8_t *)frame->data;
        samples_per_channel = frame->nb_samples;
    }

    int32_t n;
    n = av_audio_fifo_write(m_audioFifo, reinterpret_cast<void**>(&data), samples_per_channel);
    if (n < samples_per_channel) {
        ELOG_ERROR("Cannot not write data to fifo, bnSamples %d, writed %d", samples_per_channel, n);
        return false;
    }

    return true;
}

void FfDecoder::onFrame(const Frame& frame)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    if (!m_valid) {
        ELOG_ERROR_T("Not valid");
        return;
    }

    if (frame.format != m_format) {
        ELOG_ERROR_T("Invalid format(%s), wanted(%s)", getFormatStr(frame.format), getFormatStr(m_format));
        return;
    }

    if (!m_decCtx) {
        if (!initDecoder(frame.format, frame.additionalInfo.audio.sampleRate, frame.additionalInfo.audio.channels)) {
            m_valid = false;
            return;
        }
        if (!initResampler(m_inSampleFormat, m_inSampleRate, m_inChannels,
                m_outSampleFormat, m_outSampleRate, m_outChannels)) {
            m_valid = false;
            return;
        }
        if (!initFifo(m_outSampleFormat, m_outSampleRate, m_outChannels)) {
            m_valid = false;
            return;
        }
    }

    av_init_packet(&m_packet);
    m_packet.data = frame.payload;
    m_packet.size = frame.length;

    int ret;

    ret = avcodec_send_packet(m_decCtx, &m_packet);
    if (ret < 0) {
        ELOG_ERROR_T("Error while send packet, %s", ff_err2str(ret));
        return;
    }

    ret = avcodec_receive_frame(m_decCtx, m_decFrame);
    if (ret == AVERROR(EAGAIN)) {
        ELOG_DEBUG_T("Error while receive frame(%d), %s", ret, ff_err2str(ret));
        return;
    }else if (ret < 0) {
        ELOG_ERROR_T("Error while receive frame(%d), %s", ret, ff_err2str(ret));
        return;
    }

    if (!addFrameToFifo(m_decFrame)) {
        ELOG_ERROR_T("Error add frame to fifo");
        return;
    }

    while (av_audio_fifo_size(m_audioFifo) >= m_audioFrame->nb_samples) {
        int32_t n;

        n = av_audio_fifo_read(m_audioFifo, reinterpret_cast<void**>(m_audioFrame->data), m_audioFrame->nb_samples);
        if (n != m_audioFrame->nb_samples) {
            ELOG_ERROR("Cannot read enough data from fifo, needed %d, read %d", m_audioFrame->nb_samples, n);
            return;
        }

        if (m_outFormat == FRAME_FORMAT_PCM_48000_2) {
            Frame outFrame;
            memset(&outFrame, 0, sizeof(outFrame));
            outFrame.format = FRAME_FORMAT_PCM_48000_2;
            outFrame.payload = reinterpret_cast<uint8_t*>(m_audioFrame->data[0]);
            outFrame.length = m_audioFrame->nb_samples * m_outChannels * 2;
            outFrame.additionalInfo.audio.isRtpPacket = 0;
            outFrame.additionalInfo.audio.sampleRate = m_outSampleRate;
            outFrame.additionalInfo.audio.channels = m_outChannels;
            outFrame.additionalInfo.audio.nbSamples = m_audioFrame->nb_samples;
            outFrame.timeStamp = m_timestamp * outFrame.additionalInfo.audio.sampleRate / 1000;

            ELOG_TRACE_T("deliverFrame(%s), sampleRate(%d), channels(%d), timeStamp(%d), length(%d), %s",
                    getFormatStr(outFrame.format),
                    outFrame.additionalInfo.audio.sampleRate,
                    outFrame.additionalInfo.audio.channels,
                    outFrame.timeStamp * 1000 / outFrame.additionalInfo.audio.sampleRate,
                    outFrame.length,
                    outFrame.additionalInfo.audio.isRtpPacket ? "RtpPacket" : "NonRtpPacket"
                    );

            deliverFrame(outFrame);
        } else {
            AudioFrame audioFrame;
            audioFrame.UpdateFrame(
                    -1,
                    (uint32_t)m_timestamp * m_audioFrame->nb_samples / 1000,
                    (const int16_t*)m_audioFrame->data[0],
                    (size_t)m_audioFrame->nb_samples,
                    m_outSampleRate,
                    AudioFrame::kNormalSpeech,
                    AudioFrame::kVadPassive,
                    (size_t)m_outChannels
                    );
            m_output->addAudioFrame(&audioFrame);
        }
        m_timestamp += 1000 * m_audioFrame->nb_samples / m_outSampleRate;
    }
}

bool FfDecoder::getAudioFrame(AudioFrame* audioFrame)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    if (!m_input)
        return false;

    return m_input->getAudioFrame(audioFrame);
}

char *FfDecoder::ff_err2str(int errRet)
{
    av_strerror(errRet, (char*)(&m_errbuff), 500);
    return m_errbuff;
}

} /* namespace mcu */
