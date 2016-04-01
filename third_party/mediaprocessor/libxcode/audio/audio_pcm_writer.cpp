/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/
#include "audio_pcm_writer.h"
#include "base/trace.h"

#ifdef ENABLE_AUDIO_CODEC
#include "pcm_tablegen.h"
#endif

//#define DUMP_AUDIO_PCM_OUTPUT    // Customized option for debugging in WebRTC scenario
AudioPCMWriter::AudioPCMWriter(Stream* st, unsigned char* name) :
    stream_writer_(st),
    dump_output_(NULL),
    data_size_(0),
    enc_type_(STREAM_TYPE_AUDIO_PCM),
    buf_enc_(NULL)
{
    frame_count_ = 0;
    cur_time_ = 0;
    start_time_ = 0;
}

AudioPCMWriter::~AudioPCMWriter()
{
    Release();
}

void AudioPCMWriter::Release()
{
    //write the final data length value to wav file header
    if (data_size_) {
        stream_writer_->SetStreamAttribute(FILE_HEAD);
#ifdef ENABLE_AUDIO_CODEC
        if (enc_type_ == STREAM_TYPE_AUDIO_ALAW) {
            wav_info_.format_tag = WAVE_FORMAT_ALAW;
            wav_info_.resolution = 8;
        } else if (enc_type_ == STREAM_TYPE_AUDIO_MULAW) {
            wav_info_.format_tag = WAVE_FORMAT_MULAW;
            wav_info_.resolution = 8;
        }
#endif
        wav_header_.Populate(&wav_info_, data_size_);
        stream_writer_->WriteBlock(&wav_header_, wav_header_.GetHeaderSize());
#ifdef DUMP_AUDIO_PCM_OUTPUT
    if (dump_output_) {
        dump_output_->SetStreamAttribute(FILE_HEAD);
        wav_info_.channels_number = 2;
        wav_header_.Populate(&wav_info_, data_size_);
        dump_output_->WriteBlock(&wav_header_, wav_header_.GetHeaderSize());
        delete dump_output_;
    }
#endif
    }
    stream_writer_->SetEndOfStream();

    if(buf_enc_) {
        free(buf_enc_);
    }
}

bool AudioPCMWriter::Init(void* cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    sAudioParams *audio_params = reinterpret_cast<sAudioParams *> (cfg);
    enc_type_ = audio_params->nCodecType;
#ifdef ENABLE_AUDIO_CODEC
    if (enc_type_ == STREAM_TYPE_AUDIO_ALAW ||
        enc_type_ == STREAM_TYPE_AUDIO_MULAW) {
        printf("AudioPCMWriter(%p): Init G.711 Table, CodecType = %d\n", this, enc_type_);
        pcm_alaw_tableinit();
        pcm_ulaw_tableinit();
    }
#endif
    measure_.GetCurTime(&cur_time_);
    start_time_ = cur_time_;
#ifdef DUMP_AUDIO_PCM_OUTPUT
    dump_output_ = new Stream;
    bool status = false;
    char file_name[FILENAME_MAX] = {0};
    sprintf(file_name, "dump_pcm_output_%p.wav", this);
    if (dump_output_) {
        status = dump_output_->Open(file_name, true);
    }

    if (!status) {
        if (dump_output_) {
            delete dump_output_;
            dump_output_ = NULL;
        }
    }
    printf("[%p]: Dump output pcm file to: %s\n", this, file_name);
#endif
    return true;
}

int AudioPCMWriter::Recycle(MediaBuf &buf)
{
    //We don't have any SID resource, directly return
    return 0;
}

int AudioPCMWriter::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    AudioPayload payload_in;
    payload_in.payload = buf.payload;
    payload_in.payload_length = buf.payload_length;
    payload_in.isFirstPacket = buf.isFirstPacket;

    if (NULL == payload_in.payload) {
        APP_TRACE_INFO("PCMWriter[%p]: input audio payload is empty, audio PCMWriter get eof\n", this);
        return 0;
    }

    if (PCMWrite(&payload_in) != 0) {
        ReturnBuf(buf);
        return -1;
    }

    frame_count_++;
    measure_.GetCurTime(&cur_time_);
    if (cur_time_ - start_time_ >= 1000000) {
        APP_TRACE_DEBUG("AudioPCMWriter[%p]: audio output frame rate = %d\n", this, frame_count_);
        start_time_ = cur_time_;
        frame_count_ = 0;
    }

    return 0;
}

int AudioPCMWriter::PCMWrite(AudioPayload* payload_in)
{
    int data_offset = wav_header_.Interpret(payload_in->payload, &wav_info_, payload_in->payload_length);
    if (data_offset <= 0) {
        printf("AudioPCMWriter[%p]: Invalid or unsupported format! data_offset = %d, payload_length = %d\n",
               this, data_offset, payload_in->payload_length);
        return -1;
    }

#ifdef ENABLE_AUDIO_CODEC
    if (wav_info_.resolution != 16 && enc_type_ != STREAM_TYPE_AUDIO_PCM) {
        printf("G.711 Encoder doesn't support %dbit input.\n", wav_info_.resolution);
        return -1;
    }
#endif

    unsigned int frameSize;
    if (payload_in->payload_length > data_offset) {
        frameSize = payload_in->payload_length - data_offset;
    }
    else {
        printf("AudioPCMWriter[%p]: input payload size(%d) is lower than data_offset(%d)\n",
               this,
               payload_in->payload_length,
               data_offset);
        frameSize = 0;
    }

    if (payload_in->isFirstPacket) {
        printf("AudioPCMWriter[%p]: channel:sample_rate:bit_depth:channel_mask is %d:%d:%d:%d\n",
               this,
               wav_info_.channels_number,
               wav_info_.sample_rate,
               wav_info_.resolution,
               wav_info_.channel_mask);
#ifdef ENABLE_AUDIO_CODEC
        if (enc_type_ == STREAM_TYPE_AUDIO_ALAW) {
            wav_info_.format_tag = WAVE_FORMAT_ALAW;
            wav_info_.resolution = 8;
            buf_enc_ = (unsigned char *)malloc(sizeof(unsigned char) * (frameSize >> 1));
        } else if (enc_type_ == STREAM_TYPE_AUDIO_MULAW) {
            wav_info_.format_tag = WAVE_FORMAT_MULAW;
            wav_info_.resolution = 8;
            buf_enc_ = (unsigned char *)malloc(sizeof(unsigned char) * (frameSize >> 1));
        }
#endif
#ifdef DUMP_AUDIO_PCM_OUTPUT
        wav_info_.channels_number = 2;    // Special case for debugging in WebRTC scenario
        wav_header_.Populate(&wav_info_, 0xFFFFFFFF);
        dump_output_->WriteBlock(&wav_header_, wav_header_.GetHeaderSize());
        wav_info_.channels_number = 1;    // Special case for debugging in WebRTC scenario
#endif
        wav_header_.Populate(&wav_info_, 0xFFFFFFFF);
        stream_writer_->WriteBlock(&wav_header_, wav_header_.GetHeaderSize());
    }

#ifdef ENABLE_AUDIO_CODEC
    int num, value;
    short *sample_in = (short *)(payload_in->payload + data_offset);
    unsigned char *sample_out = buf_enc_;
#endif
    unsigned char *buf_dst;

    switch (enc_type_) {
#ifdef ENABLE_AUDIO_CODEC
    case STREAM_TYPE_AUDIO_ALAW:
        buf_dst = buf_enc_;
        frameSize = frameSize >> 1;
        num = frameSize;
        for (; num > 0; num--) {
            value = *sample_in++;
            *sample_out++ = linear_to_alaw[(value + 32768) >> 2];
        }
        break;
    case STREAM_TYPE_AUDIO_MULAW:
        buf_dst = buf_enc_;
        frameSize = frameSize >> 1;
        num = frameSize;
        for (; num > 0; num--) {
            value = *sample_in++;
            *sample_out++ = linear_to_ulaw[(value + 32768) >> 2];
        }
        break;
#endif
    case STREAM_TYPE_AUDIO_PCM:
    default:
        buf_dst = payload_in->payload + data_offset;
        break;
    }

    stream_writer_->WriteBlock(buf_dst, frameSize);
#ifdef DUMP_AUDIO_PCM_OUTPUT
    dump_output_->WriteBlock(buf_dst, frameSize);
#endif
    data_size_ += frameSize;

    return 0;
}

int AudioPCMWriter::HandleProcess()
{
    return 0;
}

