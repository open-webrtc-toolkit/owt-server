/* <COPYRIGHT_TAG> */

#ifndef APP_H
#define APP_H

#include <signal.h>
#include <time.h>
#include <speex/speex_preprocess.h>
#include <speex/speex_resampler.h>
#include <speex/speex_echo.h>
#include "base/media_common.h"
#include "base/base_element.h"
#include "base/media_types.h"
#include "base/media_pad.h"
#include "base/ring_buffer.h"
#include "base/logger.h"

#ifdef FAST_COPY
#include "base/fast_copy.h"
#endif

#include "audio_params.h"
#include "wave_header.h"

enum MEDIA_INPUT_STATUS {
    INITIALIZED = 0,
    RUNNING,
    STOP
};

typedef struct {
    AudioPayload audio_payload;
    short *audio_data_in;
    int audio_payload_length;
    int audio_data_offset;
    int audio_frame_size_in;
    int audio_frame_size_mix;
    int audio_frame_size_out;
    unsigned int audio_input_id;
    unsigned int audio_input_running_status;
    unsigned int audio_input_active_status;
    unsigned int audio_param_inited;
    unsigned int audio_sample_rate_in;
    unsigned int audio_sample_rate_mix;
    unsigned int audio_vad_prob;
    unsigned int audio_channel_number_in;
    unsigned int audio_channel_number;
    unsigned int audio_bit_depth_in;
    unsigned int audio_bit_depth_mix;
    WaveHeader wav_header_in;
    Info wav_info_in;
    WaveHeader wav_header_out;
    Info wav_info_out;
    int wav_data_offset;
} APPInputContext;

typedef struct {
    short *buf_front_resample;
    short *buf_back_resample;
    short *buf_mix;
    short *buf_channel_convert;
    short *buf_bit_depth_conv_front;
    short *buf_bit_depth_conv_back;
    int *buf_psd;
    int psd_buf_size;
    int mix_buf_size;
} APPInputBuffers;

typedef struct {
    SpeexPreprocessState *processor_state;
    WaveHeader wav_header;
    Info wav_info;
    int data_offset;
    int frame_size;
} CheckContext;

typedef struct {
    BaseElement *element;
    const char *input_name;
    MediaPad *pad;
    MEDIA_INPUT_STATUS status;
    CheckContext *check_ctx;
    APPInputContext *input_ctx;
    APPInputBuffers *buffers;
    bool first_packet;
    SpeexPreprocessState *front_end_processor;
    SpeexPreprocessState *back_end_processor;
    SpeexResamplerState *front_end_resampler;
    SpeexResamplerState *back_end_resampler;
    SpeexEchoState *echo_state;
#ifdef DUMP_APP_OUT_PCM
    FILE *dump_file;
#endif
} APPMediaInput;

typedef struct {
    APPMediaInput *media_input;
    unsigned int audio_input_id;
    int audio_speech_prob;
    int audio_power_spect;
} VADSortContext;

class AudioPostProcessing: public BaseElement
{
public:
    DECLARE_MLOGINSTANCE();
    AudioPostProcessing();
    virtual ~AudioPostProcessing();

    virtual bool Init(void *cfg, ElementMode element_mode);

    virtual int Recycle(MediaBuf &buf);

    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);

    virtual void NewPadAdded(MediaPad *pad);

    virtual void PadRemoved(MediaPad *pad);

    bool Destroy();

    void* GetActiveInputHandle();

    int GetInputActiveStatus(void *input_handle);

    APPFilterParameters app_parameter_;
private:
    AudioPostProcessing(const AudioPostProcessing &app);

    AudioPostProcessing& operator = (const AudioPostProcessing&) {
        return *this;
    }

    virtual int HandleProcess();
    int FileModeHandleProcess();
    int StreamingModeHandleProcess();
    bool ProcessInput();
    int ComposeAudioFrame();
    int FrontEndProcessRun(APPMediaInput *media_input, AudioPayload *payload);
    int BackEndProcessRun(APPMediaInput *media_input, AudioPayload *payload);
    int FrontEndResample(APPMediaInput *media_input, AudioPayload *payload);
    int BackEndResample(APPMediaInput *media_input, AudioPayload *payload);
    int ChannelNumberConvert(APPMediaInput *media_input, AudioPayload *payload);
    int FrontEndBitDepthConvert(APPMediaInput *media_input, AudioPayload *payload);
    int BackEndBitDepthConvert(APPMediaInput *media_input, AudioPayload *payload);
    int AudioMix(APPMediaInput *media_input, MediaBuf &buffer, int input_id);
    int UpdateMediaInput(APPMediaInput *media_input, MediaBuf &buf);
    int PrepareParameter(APPMediaInput *media_input, MediaBuf &buffer, unsigned int input_id);
    void ReleaseBuffer(MediaBuf *out);
    int SpeechCheck(CheckContext *ctx, MediaBuf &buffer);
    void VADSort();
    void ReleaseMediaInput(APPMediaInput *media_input);

    std::list<APPMediaInput *> media_input_list_;
    APPMediaInput *host_input_;
    int num_input_;
    int num_running_input_;
    pthread_mutex_t mutex_;
    bool out_param_inited_;
    int active_input_id_;
    unsigned long long total_frame_count_;
    unsigned char *buf_silence_;
    VADSortContext *vad_sort_ctx_;
    MediaBuf *out_buf_;
};

#endif

