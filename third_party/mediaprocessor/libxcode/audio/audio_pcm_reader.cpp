/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/
#include <unistd.h>
#include "audio_pcm_reader.h"
#include "audio_params.h"
#include "base/trace.h"
#include "base/measurement.h"

#ifdef ENABLE_AUDIO_CODEC
extern "C" {
#include "libavcodec/pcm_tablegen.h"
}
#endif

//#define DUMP_AUDIO_PCM_INPUT    // Customized option for debugging in WebRTC scenario

DEFINE_MLOGINSTANCE_CLASS(AudioPCMReader, "AudioPCMReader");
AudioPCMReader::AudioPCMReader(MemPool* mp, char* input) :
    mem_pool_(mp),
    dump_out_file_(NULL),
    total_data_size_(0)
{
    for (int i = 0; i < AUDIO_OUTPUT_QUEUE_SIZE; i++) {
        output_payload_[i].payload = NULL;
    }
}

AudioPCMReader::~AudioPCMReader()
{
    Release();
}

void AudioPCMReader::Release()
{
    for (int i = 0; i < AUDIO_OUTPUT_QUEUE_SIZE; i++) {
        if(output_payload_[i].payload) {
            free(output_payload_[i].payload);
            output_payload_[i].payload_length = 0;
        }
    }
#ifdef DUMP_AUDIO_PCM_INPUT
    if (dump_out_file_) {
        dump_out_file_->SetStreamAttribute(FILE_HEAD);
        wav_info_.channels_number = 2;
        wav_header_.Populate(&wav_info_, total_data_size_);
        dump_out_file_->WriteBlock(&wav_header_, wav_header_.GetHeaderSize());
        delete dump_out_file_;
    }
#endif
}

bool AudioPCMReader::Init(void* cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;

#ifdef DUMP_AUDIO_PCM_INPUT
    dump_out_file_ = new Stream;
    bool status = false;
    char file_name[FILENAME_MAX] = {0};
    sprintf(file_name, "dump_input_%p.wav", this);
    if (dump_out_file_) {
        status = dump_out_file_->Open(file_name, true);
    }

    if (!status) {
        if (dump_out_file_) {
            delete dump_out_file_;
            dump_out_file_ = NULL;
        }
    }
    MLOG_DEBUG("[%p]: Dump input pcm file to: %s\n", this, file_name);
#endif

    return true;
}

int AudioPCMReader::ProcessChain(MediaPad* pad, MediaBuf &buf)
{
    return -1;
}

int AudioPCMReader::Recycle(MediaBuf &buf)
{
    AudioPayload payload;
    payload.payload = buf.payload;
    payload.payload_length = buf.payload_length;
    payload.isFirstPacket = buf.isFirstPacket;

    if (payload.payload) {
        output_queue_.Push(payload);
    } else {
        return -1;
    }

    return 0;
}

int AudioPCMReader::HandleProcess()
{
    bool is_first_packet = true;
    int ret;
    AudioPayload payload_out;
    AudioPayload payload_in;
    MediaBuf buf;
    MediaPad *srcpad = *(this->srcpads_.begin());
    unsigned int data_consumed = 0;
    unsigned int header_size = sizeof(WaveHeader);
    unsigned int frame_size = INITIAL_PCM_FRAME_SIZE;
    unsigned int buffer_size = frame_size;
    unsigned int frame_count = 0;
    int i = 0;
    Measurement measure;
    unsigned long cur_time = 0;
    unsigned long last_time = 0;

    measure.GetCurTime(&cur_time);
    last_time = cur_time;

    while (is_running_) {

        // step 1: check if there is available coded stream
        unsigned int mem_size = mem_pool_->GetFlatBufFullness();
        buffer_size = (mem_size < frame_size) ? mem_size : frame_size;

        unsigned int eof = mem_pool_->GetDataEof();
        if ((buffer_size == 0) && (eof == true)) {
            MLOG_INFO("PCMReader: [%p] Got the end of stream.\n", this);
            buf.payload = NULL;
            buf.payload_length = 0;
            srcpad->PushBufToPeerPad(buf);
            break;
        }

        if ((buffer_size < header_size) && (eof != true)) {
            usleep(10 * 1000);
            APP_TRACE_DEBUG("[%p] data not ready. buffer_size = %d\n", this, buffer_size);
            continue;
        }

        // step 2: parse wav header.
        if (is_first_packet) {
            payload_in.payload = mem_pool_->GetReadPtr();
            payload_in.payload_length = 1024;
            ret = ParseWAVHeader(&payload_in);
            if (ret == -1) {
                MLOG_ERROR("ParsePCMHeader error, return.\n");
                buf.payload = NULL;
                buf.payload_length = 0;
                srcpad->PushBufToPeerPad(buf);
                break;
            }
#ifdef DUMP_AUDIO_PCM_INPUT
            if (dump_out_file_) {
                wav_info_.channels_number = 2;    // Special case for debugging in WebRTC scenario
                wav_header_.Populate(&wav_info_, 0xFFFFFFFF);
                dump_out_file_->WriteBlock(&wav_header_, wav_header_.GetHeaderSize());
                wav_info_.channels_number = 1;    // Special case for debugging in WebRTC scenario
                wav_header_.Populate(&wav_info_, 0xFFFFFFFF);
            }
#endif
            mem_pool_->UpdateReadPtr(data_offset_);
            total_data_size_ += data_offset_;
            // 30ms as one audio frame
            frame_size = wav_info_.channels_number *
                         wav_info_.sample_rate * 3 *
                         (wav_info_.resolution >> 3) / 50;
            APP_TRACE_DEBUG("AudioPCMReader[%p]: frame_size = %d\n", this, frame_size);
            buffer_size = (mem_size < frame_size) ? mem_size : frame_size;
            // allocate memory for audio payload.
            for (i = 0; i < AUDIO_OUTPUT_QUEUE_SIZE; i++) {
                // For G.711 inputs, the output frame size should be (frame_size * 2) after decoded.
                switch (dec_type_) {
#ifdef ENABLE_AUDIO_CODEC
                case STREAM_TYPE_AUDIO_ALAW:
                case STREAM_TYPE_AUDIO_MULAW:
                    output_payload_[i].payload = reinterpret_cast<unsigned char*>
                                                 (malloc(frame_size * 2 + sizeof(WaveHeader)));
                    break;
#endif
                case STREAM_TYPE_AUDIO_PCM:
                default:
                    output_payload_[i].payload = reinterpret_cast<unsigned char*>
                                                 (malloc(frame_size + sizeof(WaveHeader)));
                    break;
                }
                if (!output_payload_[i].payload) {
                    MLOG_ERROR("AudioPCMReader[%p]: Failed to allocate memory for payload!\n", this);
                    continue;
                }
                output_payload_[i].payload_length  = 0;
                output_queue_.Push(output_payload_[i]);
            }
        }

        // step 3: check if there is free buffer for decoded wav.
        if (output_queue_.Pop(payload_out) == false || (NULL == payload_out.payload)) {
            APP_TRACE_INFO("There is no empty buffer for PCM reader(%p).\n", this);
            usleep(10 * 1000);
            continue;
        }

        // step 4: get PCM frame data.
        payload_in.payload = mem_pool_->GetReadPtr();
        payload_in.payload_length = buffer_size;
        data_consumed = 0;
        ret = PCMRead(&payload_out, &payload_in, is_first_packet, &data_consumed);
        if (ret == -1) {
            MLOG_ERROR("AudioPCMReader[%p]: PCMRead error, return.\n", this);
            buf.payload = NULL;
            buf.payload_length = 0;
            srcpad->PushBufToPeerPad(buf);
            break;
        }
#ifdef DUMP_AUDIO_PCM_INPUT
        if (dump_out_file_) {
            dump_out_file_->WriteBlock(mem_pool_->GetReadPtr(), buffer_size);
        }
#endif
        frame_count++;
        measure.GetCurTime(&cur_time);
        if (cur_time - last_time >= 1000000) {
            APP_TRACE_DEBUG("AudioPCMReader[%p]: audio frame rate = %d\n", this, frame_count);
            last_time = cur_time;
            frame_count = 0;
        }

        total_data_size_ += data_consumed;
        mem_pool_->UpdateReadPtr(data_consumed);
        is_first_packet = false;

        // step 4: send decoded stream to next element
        buf.payload = payload_out.payload;
        buf.payload_length = payload_out.payload_length;
        buf.isFirstPacket = payload_out.isFirstPacket;

        srcpad->PushBufToPeerPad(buf);
    }
    is_running_ = false;
    return 0;
}

int AudioPCMReader::ParseWAVHeader(AudioPayload *payload_in)
{
    data_offset_ = wav_header_.Interpret(payload_in->payload, &wav_info_, payload_in->payload_length);
    if (data_offset_ <= 0) {
        MLOG_ERROR("AudioPCMReader[%p]: Invalid or unsupported wav format! DataOffset(%d)\n",
               this,
               data_offset_);
        return -1;
    }

    MLOG_INFO("AudioPCMReader[%p] %s: channel:sample_frequency:bit_per_sample:channel_mask is %d:%d:%d:%d\n",
           this,
           __FUNCTION__,
           wav_info_.channels_number,
           wav_info_.sample_rate,
           wav_info_.resolution,
           wav_info_.channel_mask);

    switch (wav_info_.format_tag) {
#ifdef ENABLE_AUDIO_CODEC
    case WAVE_FORMAT_ALAW:
        dec_type_ = STREAM_TYPE_AUDIO_ALAW;
        for (int i = 0; i < 256; i++) {
            g711_table_[i] = alaw2linear(i);
        }
        MLOG_INFO("AudioPCMReader[%p]: input format is G.711 A-Law\n", this);
        break;
    case WAVE_FORMAT_MULAW:
        dec_type_ = STREAM_TYPE_AUDIO_MULAW;
        for (int i = 0; i < 256; i++) {
            g711_table_[i] = ulaw2linear(i);
        }
        MLOG_INFO("AudioPCMReader[%p]: input format is G.711 Mu-Law\n", this);
        break;
#endif
    case WAVE_FORMAT_PCM:
    default:
        dec_type_ = STREAM_TYPE_AUDIO_PCM;
        MLOG_INFO("AudioPCMReader[%p]: input format is Linear PCM\n", this);
        break;
    }
    // Set the output wav format to PCM.
    wav_info_.format_tag = WAVE_FORMAT_PCM;
    wav_info_.resolution = 16;

    unsigned int input_data_size = payload_in->payload_length - data_offset_;
    // Create a RIFF header to prepend to the PCM samples
    wav_header_.Populate(&wav_info_, input_data_size);

    return 0;
}

// PCMRead: Generate PCM frame
int AudioPCMReader::PCMRead(AudioPayload *payload_out, AudioPayload *payload_in, bool is_first_packet, unsigned int* data_consumed)
{
    unsigned char *input_data;
    unsigned int input_data_size;
    short *output_data;

    input_data = payload_in->payload;
    input_data_size = payload_in->payload_length;
    output_data = (short *)(payload_out->payload + wav_header_.GetHeaderSize());

    if ((input_data) && (input_data_size > 0)) {
        // Put RIFF header to payload out.
        memcpy(payload_out->payload, (unsigned char *) &wav_header_, wav_header_.GetHeaderSize());
        switch (dec_type_) {
#ifdef ENABLE_AUDIO_CODEC
        case STREAM_TYPE_AUDIO_ALAW:
        case STREAM_TYPE_AUDIO_MULAW:
            unsigned char *sample_g711;
            short *sample_raw_pcm;
            int num, value;
            // Set the payload length.
            payload_out->payload_length = wav_header_.GetHeaderSize() + input_data_size * 2;
            // G.711 a-law and mu-law decoding.
            sample_g711 = input_data;
            sample_raw_pcm = output_data;
            num = input_data_size;
            for (; num > 0; num--) {
                value = *sample_g711++;
                *sample_raw_pcm++ = g711_table_[value];
            }
            break;
#endif
        case STREAM_TYPE_AUDIO_PCM:
        default:
            payload_out->payload_length = wav_header_.GetHeaderSize() + input_data_size;
            // Put input raw PCM data to payload out.
            memcpy(output_data, input_data, input_data_size);
            break;
        }
        APP_TRACE_INFO("%s: The wave %p header size is %d\n", __FUNCTION__, &wav_header_, wav_header_.GetHeaderSize());
        payload_out->isFirstPacket = is_first_packet;
    }

    *data_consumed = payload_in->payload_length;
    return 0;
}
