// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioFrameWriter.h"

#include <rtputils.h>

using namespace webrtc;

namespace woogeen_base {

inline AVCodecID frameFormat2AVCodecID(int frameFormat)
{
    switch (frameFormat) {
        case FRAME_FORMAT_PCMU:
            return AV_CODEC_ID_PCM_MULAW;
        case FRAME_FORMAT_PCMA:
            return AV_CODEC_ID_PCM_ALAW;
        case FRAME_FORMAT_OPUS:
            return AV_CODEC_ID_OPUS;
        case FRAME_FORMAT_AAC_48000_2:
            return AV_CODEC_ID_AAC;
        case FRAME_FORMAT_G722_16000_1:
        case FRAME_FORMAT_G722_16000_2:
            return AV_CODEC_ID_ADPCM_G722;
        default:
            return AV_CODEC_ID_NONE;
    }
}

DEFINE_LOGGER(AudioFrameWriter, "woogeen.AudioFrameWriter");

AudioFrameWriter::AudioFrameWriter(const std::string& name)
    : m_name(name)
    , m_fp(NULL)
    , m_index(0)
    , m_sampleRate(0)
    , m_channels(0)
    , m_context(NULL)
    , m_audioStream(NULL)
{
}

AudioFrameWriter::~AudioFrameWriter()
{
    if (m_fp) {
        fclose(m_fp);
        m_fp = NULL;
    }

    if (m_context) {
        av_write_trailer(m_context);

        if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
            avio_close(m_context->pb);
        }
        avformat_free_context(m_context);
        m_context = NULL;
    }
}

bool AudioFrameWriter::addAudioStream(FrameFormat format, uint32_t sampleRate, uint32_t channels)
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

void AudioFrameWriter::writeCompressedFrame(const Frame& frame)
{
    if (!m_context) {
        char fileName[128];
        snprintf(fileName, 128, "/tmp/%s-%s.mkv", m_name.c_str(), getFormatStr(frame.format));

        avformat_alloc_output_context2(&m_context, NULL, "matroska", fileName);
        if (!m_context) {
            ELOG_ERROR_T("Cannot allocate output context, %s", fileName);
            return;
        }

        if (!(m_context->oformat->flags & AVFMT_NOFILE)) {
            int ret = avio_open(&m_context->pb, m_context->url, AVIO_FLAG_WRITE);
            if (ret < 0) {
                ELOG_ERROR_T("Cannot open avio");

                avformat_free_context(m_context);
                m_context = NULL;

                return;
            }
        }

        if (!addAudioStream(frame.format, frame.additionalInfo.audio.sampleRate, frame.additionalInfo.audio.channels)) {
            ELOG_ERROR_T("Cannot add audio stream, %s, %d, %d"
                    , getFormatStr(frame.format)
                    , frame.additionalInfo.audio.sampleRate
                    , frame.additionalInfo.audio.channels);

            avformat_free_context(m_context);
            m_context = NULL;
            m_audioStream = NULL;
            return;
        }

        int ret = avformat_write_header(m_context, NULL);
        if (ret < 0) {
            ELOG_ERROR_T("Cannot write header");

            avformat_free_context(m_context);
            m_context = NULL;
            m_audioStream = NULL;
            return;
        }

        av_dump_format(m_context, 0, m_context->url, 1);
        ELOG_INFO_T("Dump: %s", fileName);
    }

    int ret;
    AVPacket pkt;

    uint8_t *payload = frame.payload;
    uint32_t length = frame.length;

    if (frame.additionalInfo.audio.isRtpPacket) {
        ELOG_INFO_T("isRtpPacket");

        ::RTPHeader* rtp = reinterpret_cast<::RTPHeader*>(payload);
        uint32_t headerLength = rtp->getHeaderLength();
        assert(length >= headerLength);
        payload += headerLength;
        length -= headerLength;
    }

    av_init_packet(&pkt);

    pkt.data = payload;
    pkt.size = length;

    pkt.dts = (int64_t)(frame.timeStamp / (av_q2d(m_audioStream->time_base) * 1000));
    if (m_audioStream->codecpar->codec_id == AV_CODEC_ID_ADPCM_G722)
        pkt.dts *= 2;

    pkt.pts = pkt.dts;
    pkt.stream_index = m_audioStream->index;

    ret = av_interleaved_write_frame(m_context, &pkt);
    if (ret < 0)
        ELOG_ERROR_T("Cannot write frame");

    return;
}

void AudioFrameWriter::write(const Frame& frame)
{
    switch (frame.format) {
        case FRAME_FORMAT_PCM_48000_2:
            assert(false);
            break;

        case FRAME_FORMAT_PCMU:
        case FRAME_FORMAT_PCMA:
        case FRAME_FORMAT_OPUS:
        case FRAME_FORMAT_ISAC16:
        case FRAME_FORMAT_ISAC32:
        case FRAME_FORMAT_ILBC:
        case FRAME_FORMAT_G722_16000_1:
        case FRAME_FORMAT_G722_16000_2:
        case FRAME_FORMAT_AAC_48000_2:
            writeCompressedFrame(frame);
            break;

        default:
            assert(false);
            return;
    }
}

FILE *AudioFrameWriter::getAudioFp(const webrtc::AudioFrame *audioFrame)
{
    if (m_sampleRate != audioFrame->sample_rate_hz_
            || m_channels != (int32_t)audioFrame->num_channels_) {
        if (m_fp) {
            fclose(m_fp);
            m_fp = NULL;
        }

        m_index++;

        char fileName[128];
        snprintf(fileName, 128, "/tmp/%s-%d-%d-%ld.pcm16", m_name.c_str(), m_index, audioFrame->sample_rate_hz_, audioFrame->num_channels_);
        m_fp = fopen(fileName, "wb");
        if (!m_fp) {
            ELOG_ERROR_T("Can not open dump file, %s", fileName);
            return NULL;
        }

        ELOG_INFO_T("Dump: %s", fileName);

        m_sampleRate = audioFrame->sample_rate_hz_;
        m_channels = audioFrame->num_channels_;
    }

    return m_fp;
}

void AudioFrameWriter::write(const webrtc::AudioFrame *audioFrame)
{
    if (!audioFrame) {
        ELOG_ERROR_T("NULL pointer");
        return;
    }

    if (audioFrame->samples_per_channel_ == 0) {
        ELOG_ERROR_T("Invalid size(%ld)", audioFrame->samples_per_channel_);
        return;
    }

    FILE *fp = getAudioFp(audioFrame);
    if (!fp) {
        ELOG_ERROR_T("NULL fp");
        return;
    }

    fwrite(audioFrame->data(), 1, audioFrame->samples_per_channel_* audioFrame->num_channels_ * 2, fp);
}

} /* namespace mcu */
