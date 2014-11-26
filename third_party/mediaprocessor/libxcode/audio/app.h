/* <COPYRIGHT_TAG> */

#ifndef APP_H
#define APP_H

#include <signal.h>
#include <time.h>
#include <speex/speex_preprocess.h>
#include <speex/speex_resampler.h>
#include "base/media_common.h"
#include "base/base_element.h"
#include "base/media_types.h"
#include "base/media_pad.h"
#include "base/ring_buffer.h"

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
    AudioPayload audio_payload[MAX_APP_INPUT];
    short *audio_data_in[MAX_APP_INPUT];
    int audio_payload_length[MAX_APP_INPUT];
    int audio_data_offset[MAX_APP_INPUT];
    int audio_frame_size_in[MAX_APP_INPUT];
    int audio_frame_size_mix[MAX_APP_INPUT];
    int audio_frame_size_out[MAX_APP_INPUT];
    unsigned int audio_input_id[MAX_APP_INPUT];
    unsigned int audio_input_running_status[MAX_APP_INPUT];
    unsigned int audio_input_active_status[MAX_APP_INPUT];
    unsigned int audio_param_inited[MAX_APP_INPUT];
    unsigned int audio_sample_rate_in[MAX_APP_INPUT];
    unsigned int audio_sample_rate_mix[MAX_APP_INPUT];
    unsigned int audio_vad_prob[MAX_APP_INPUT];
    unsigned int audio_channel_number_in[MAX_APP_INPUT];
    unsigned int audio_channel_number[MAX_APP_INPUT];
} APPContext;

typedef struct {
    SpeexPreprocessState *m_st;
    WaveHeader m_hdr;
    Info m_info;
    int data_offset;
    int frame_size;
} CheckContext;

typedef struct {
    BaseElement *element;
    const char *input_name;
    MediaPad *pad;
    MEDIA_INPUT_STATUS status;
    CheckContext *ctx;
    SpeexPreprocessState *front_end_processor;
    SpeexPreprocessState *back_end_processor;
    SpeexResamplerState *front_end_resampler;
    SpeexResamplerState *back_end_resampler;
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
    AudioPostProcessing();
    virtual ~AudioPostProcessing();

    virtual bool Init(void *cfg, ElementMode element_mode);

    virtual int Recycle(MediaBuf &buf);

    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);

    virtual void NewPadAdded(MediaPad *pad);

    virtual void PadRemoved(MediaPad *pad);

    bool Destroy();

    int EnableVAD(bool enable, unsigned int vad_prob_start_value);

    int EnableAGC(bool enable, unsigned int agc_level_value);

    int EnableFrontEndDenoise(bool enable);
    int EnableBackEndDenoise(bool enable);

    int EnableFrontEndResample(bool enable, unsigned int new_sample_rate_value);
    int EnableBackEndResample(bool enable, unsigned int new_sample_rate_value);

    int EnableMixing(bool enable);

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
    int FrontEndProcessRun(APPMediaInput *media_input, unsigned int input_id, AudioPayload *payload);
    int BackEndProcessRun(APPMediaInput *media_input, unsigned int input_id, AudioPayload *payload);
    int FrontEndResample(APPMediaInput *media_input, unsigned int input_id, AudioPayload *payload);
    int BackEndResample(APPMediaInput *media_input, unsigned int input_id, AudioPayload *payload);
    int ChannelNumberConvert(APPMediaInput *mediainput, unsigned int input_id, AudioPayload *payload);
    int AudioMix(MediaBuf &buffer, int input_id);
    int UpdateMediaInput(APPMediaInput *mediainput, MediaBuf &buf);
    int PrepareParameter(MediaBuf &buffer, unsigned int input_id);
    void ReleaseBuffer(MediaBuf *out);
    int SpeechCheck(CheckContext *ctx, MediaBuf &buffer);
    void VADSort();

    APPContext app_context_;
    std::list<APPMediaInput *> media_input_;
    APPMediaInput *host_input_;
    int num_input_;
    int num_running_input_;
    pthread_mutex_t mutex_;
    VADSortContext vad_sort_ctx_[MAX_APP_INPUT];

    bool m_bFirstPacket[MAX_APP_INPUT];
    bool m_bOutParamInited;
    int m_nActiveInputID;

    WaveHeader m_inputWaveHeader[MAX_APP_INPUT];
    Info m_inputWaveInfo[MAX_APP_INPUT];
    WaveHeader m_outputWaveHeader[MAX_APP_INPUT];
    Info m_outputWaveInfo[MAX_APP_INPUT];
    int m_nDataOffset[MAX_APP_INPUT];
    unsigned long long m_nFrameCount;
    unsigned char *m_pBufSilence;

    short *m_pBufResample[MAX_APP_INPUT];
    short *m_pBufBKResample[MAX_APP_INPUT];
    short *m_pBufMix[MAX_APP_INPUT];
    int m_nMixBufSize[MAX_APP_INPUT];
    short *m_pBufChannelNumConvert[MAX_APP_INPUT];
};

#endif

