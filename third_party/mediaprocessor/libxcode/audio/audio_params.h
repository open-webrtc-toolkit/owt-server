/* <COPYRIGHT_TAG> */

#ifndef __AUDIO_PARAMS_H__
#define __AUDIO_PARAMS_H__
#include "base/media_common.h"
#include "base/mem_pool.h"
#include "base/stream.h"

#define AAC_FRAME_SIZE (1024*2*12+32)
#define AUDIO_OUTPUT_QUEUE_SIZE 32
#define VAD_PROB_START  95
#define VAD_PROB_CONT   80

#define WAVE_FORMAT_PCM    1
#define WAVE_FORMAT_ALAW   6
#define WAVE_FORMAT_MULAW  7

struct sAudioParams
{
    StreamType nCodecType;
    int nSampleRate;
    unsigned int nBitRate;
    MemPool* input_stream;
    Stream* output_stream;
    union {
        void *dec_handle;
        void *enc_handle;
    };
};

struct AudioStreamInfo
{
    int codec_id;
    int sample_rate;
    int channels;
    int bit_rate;
    int sample_fmt;
    unsigned long channel_mask;
};

struct AudioPayload
{
    unsigned char* payload;
    int payload_length;
    bool isFirstPacket;
};

typedef struct APPFilterParameters {
    bool vad_enable;
    unsigned int vad_prob_start_value;   // Between 0 to 100
    unsigned int vad_prob_cont_value;   // Between 0 to 100
    bool agc_enable;
    unsigned int agc_level_value;
    bool front_end_denoise_enable;
    int front_end_denoise_level;
    bool back_end_denoise_enable;
    int back_end_denoise_level;
    bool aec_enable;
    unsigned int aec_tail_level;    // Between 0 to 10
    unsigned int resample_quality_level;    // Between 0 to 10
    bool front_end_resample_enable;
    unsigned int front_end_sample_rate;
    bool back_end_resample_enable;
    unsigned int back_end_sample_rate;
    bool channel_number_convert_enable;
    unsigned int channel_number;
    bool bit_depth_convert_enable;
    unsigned int bit_depth;
    bool nn_mixing_enable;
    bool file_mode_app;

    APPFilterParameters():
        vad_enable(0),
        vad_prob_start_value(VAD_PROB_START),
        vad_prob_cont_value(VAD_PROB_CONT),
        agc_enable(0),
        agc_level_value(8000),
        front_end_denoise_enable(0),
        front_end_denoise_level(15),
        back_end_denoise_enable(0),
        back_end_denoise_level(15),
        aec_enable(0),
        resample_quality_level(1),
        front_end_resample_enable(0),
        front_end_sample_rate(0),
        back_end_resample_enable(0),
        back_end_sample_rate(16000),
        channel_number_convert_enable(0),
        channel_number(0),
        bit_depth_convert_enable(0),
        bit_depth(16),
        nn_mixing_enable(0),
        file_mode_app(0) {
    };
} APPFilterParameters;

#endif
