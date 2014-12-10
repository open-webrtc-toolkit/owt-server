/* <COPYRIGHT_TAG> */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "app.h"
#include "base/trace.h"
#include "base/measurement.h"

//#define DUMP_APP_OUT_PCM
#define VAD_PROB_START  80
#define VAD_PROB_CONT   60
#define MIXER_TIMER_INT 30000
#define MIXER_SLEEP_INT 5000
#define BUF_DEPTH_LOW   5
#define BUF_DEPTH_HIGH  20
#define BUF_DEPTH_OVER  30

#define MIX2(a, b, nScale ) \
(((a) < 0) && ((b) < 0)) \
? ( (a) + (b) + ( ((a) * (b)) / ((1<<(nScale))-1 ) ) ) \
: ( (a) + (b) - ( ((a) * (b)) / ((1<<(nScale))-1 ) ) )

static int ComparePowerSpect( const void *a, const void *b)
{
    return (*(VADSortContext *)b).audio_power_spect - (*(VADSortContext *)a).audio_power_spect;
}

static void StereoToMono(short *output, short *input, int number)
{
    while (number >= 4) {
        output[0] = (input[0] + input[1]) >> 1;
        output[1] = (input[2] + input[3]) >> 1;
        output[2] = (input[4] + input[5]) >> 1;
        output[3] = (input[6] + input[7]) >> 1;
        output += 4;
        input += 8;
        number -= 4;
    }
    while (number > 0) {
        output[0] = (input[0] + input[1]) >> 1;
        output++;
        input += 2;
        number--;
    }
}

static void MonoToStereo(short *output, short *input, int number)
{
    int tmp;

    while (number >= 4) {
        tmp = input[0]; output[0] = tmp; output[1] = tmp;
        tmp = input[1]; output[2] = tmp; output[3] = tmp;
        tmp = input[2]; output[4] = tmp; output[5] = tmp;
        tmp = input[3]; output[6] = tmp; output[7] = tmp;
        output += 8;
        input += 4;
        number -= 4;
    }
    while (number > 0) {
        tmp = input[0]; output[0] = tmp; output[1] = tmp;
        output += 2;
        input += 1;
        number--;
    }
}

bool AudioPostProcessing::Destroy()
{
    int i;
    std::list<APPMediaInput *>::iterator it_mediainput;
    APPMediaInput *input = NULL;
    if (pthread_mutex_destroy(&mutex_)) {
        printf("Error in pthread_mutex_destroy.\n");
        assert(0);
    }

    if (buf_silence_) {
        free(buf_silence_);
        buf_silence_ = NULL;
    }

#ifdef DUMP_APP_OUT_PCM
    if (file_dump_app_) {
        input = *(media_input_list_.begin());
        for (i = 0; i < MAX_APP_INPUT; i++) {
            if (file_dump_app_[i]) {
                input->input_ctx->wav_info_out.channels_number = 2;
                input->input_ctx->wav_header_out.Populate(&(input->input_ctx->wav_info_out), 0xFFFFFFFF);
                fseek(file_dump_app_[i], 0, SEEK_SET);
                fwrite(&(input->input_ctx->wav_header_out),
                       sizeof(unsigned char),
                       input->input_ctx->wav_header_out.GetHeaderSize(),
                file_dump_app_[i]);
                fclose(file_dump_app_[i]);
                file_dump_app_[i] = NULL;
            }
            delete file_dump_app_;
            file_dump_app_ = NULL;
        }
    }
#endif

    // destroy the media input list
    for (it_mediainput = media_input_list_.begin(), i = 0;
         it_mediainput != media_input_list_.end();
         it_mediainput++, i++) {
        if (*it_mediainput != NULL) {
            input = *it_mediainput;
            if (input->check_ctx) {
                if (input->check_ctx->processor_state) {
                    speex_preprocess_state_destroy(input->check_ctx->processor_state);
                    input->check_ctx->processor_state = NULL;
                }
                free(input->check_ctx);
                input->check_ctx = NULL;
            }
            if (input->input_ctx) {
                free(input->input_ctx);
                input->input_ctx = NULL;
            }
            if (input->buffers) {
                if (input->buffers->buf_front_resample) {
                    free(input->buffers->buf_front_resample);
                    input->buffers->buf_front_resample = NULL;
                }
                if (input->buffers->buf_back_resample) {
                    free(input->buffers->buf_back_resample);
                    input->buffers->buf_back_resample = NULL;
                }
                if (input->buffers->buf_mix) {
                    free(input->buffers->buf_mix);
                    input->buffers->buf_mix = NULL;
                }
                if (input->buffers->buf_channel_convert) {
                    free(input->buffers->buf_channel_convert);
                    input->buffers->buf_channel_convert = NULL;
                }
                if (input->buffers->buf_psd) {
                    free(input->buffers->buf_psd);
                    input->buffers->buf_psd = NULL;
                }
                free(input->buffers);
                input->buffers = NULL;
            }
            if (input->front_end_processor) {
                speex_preprocess_state_destroy(input->front_end_processor);
                input->front_end_processor = NULL;
            }
            if (input->back_end_processor) {
                speex_preprocess_state_destroy(input->back_end_processor);
                input->back_end_processor = NULL;
            }
            if (input->front_end_resampler) {
                speex_resampler_destroy(input->front_end_resampler);
                input->front_end_resampler = NULL;
            }
            if (input->back_end_resampler) {
                speex_resampler_destroy(input->back_end_resampler);
                input->back_end_resampler = NULL;
            }
            if (input->echo_state) {
                speex_echo_state_destroy(input->echo_state);
                input->echo_state = NULL;
            }
            delete input;
        }
    }

    return true;
}

AudioPostProcessing::AudioPostProcessing() :
    host_input_(NULL),
    num_input_(0),
    num_running_input_(0),
    out_param_inited_(false),
    active_input_id_(-1),
    total_frame_count_(0),
    buf_silence_(NULL),
    file_dump_app_(NULL)
{
    int i = 0;
    memset(vad_sort_ctx_, 0, sizeof(vad_sort_ctx_));

    for (i = 0; i < MAX_APP_INPUT; i++) {
        vad_sort_ctx_[i].audio_input_id = i;
    }

    pthread_mutex_init(&mutex_, NULL);
}

AudioPostProcessing::~AudioPostProcessing()
{
    Destroy();
}

// Init APP
bool AudioPostProcessing::Init(void *cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    APP_TRACE_INFO("APP: Init APP\n");
    if (NULL == cfg) {
        APP_TRACE_INFO("No config for APP, just pass through\n");
        return true;
    }

    APPFilterParameters *app_cfg = static_cast<APPFilterParameters *>(cfg);
    app_parameter_ = *app_cfg;

#ifdef DUMP_APP_OUT_PCM
    file_dump_app_ = new FILE*[MAX_APP_INPUT];
    memset(file_dump_app_, 0, sizeof(FILE *) * MAX_APP_INPUT);
#endif

    return true;
}

int AudioPostProcessing::EnableVAD(bool enable, unsigned int vad_prob_start_value)
{
    app_parameter_.vad_enable = enable;
    app_parameter_.vad_prob_start_value = vad_prob_start_value;

    return 0;
}

int AudioPostProcessing::EnableAGC(bool enable, unsigned int agc_level_value)
{
    app_parameter_.agc_enable = enable;
    app_parameter_.agc_level_value = agc_level_value;

    return 0;
}

int AudioPostProcessing::EnableFrontEndDenoise(bool enable)
{
    app_parameter_.front_end_denoise_enable = enable;
    return 0;
}

int AudioPostProcessing::EnableBackEndDenoise(bool enable)
{
    app_parameter_.back_end_denoise_enable = enable;
    return 0;
}

int AudioPostProcessing::EnableFrontEndResample(bool enable, unsigned int new_sample_rate_value)
{
    app_parameter_.front_end_resample_enable = enable;
    app_parameter_.front_end_sample_rate = new_sample_rate_value;

    return 0;
}

int AudioPostProcessing::EnableBackEndResample(bool enable, unsigned int new_sample_rate_value)
{
    app_parameter_.back_end_resample_enable = enable;
    app_parameter_.back_end_sample_rate = new_sample_rate_value;

    return 0;
}

int AudioPostProcessing::EnableMixing(bool enable)
{
    app_parameter_.nn_mixing_enable = enable;
    return 0;
}

int AudioPostProcessing::Recycle(MediaBuf &buf)
{
    return 0;
}

int AudioPostProcessing::FrontEndProcessRun(APPMediaInput *mediainput, unsigned int input_id, AudioPayload *payload)
{
    unsigned char *audio_data = payload->payload + mediainput->input_ctx->wav_data_offset;
    int ret = 0;
    int vad_prob_start = 95;
    int vad_prob_continue = 80;
    int i = 0;

    if((mediainput->input_ctx->audio_input_running_status == 0) ||
       (mediainput->input_ctx->audio_frame_size_in <= 0) ||
       (mediainput->input_ctx->audio_sample_rate_in <= 0)) {
        return 0;
    }

    if (!(mediainput->front_end_processor)) {
        // Init speex front-end process status
        printf("APP[%p]: %s[%d], frame_size_in = %d, sample_rate_in = %d\n",
               this,
               __FUNCTION__,
               input_id,
               mediainput->input_ctx->audio_frame_size_in,
               mediainput->input_ctx->audio_sample_rate_in);

        mediainput->front_end_processor = speex_preprocess_state_init(mediainput->input_ctx->audio_frame_size_in >> 1,
                                                                      mediainput->input_ctx->audio_sample_rate_in);
        if (app_parameter_.front_end_denoise_enable) {
            int noise_suppress_value = app_parameter_.front_end_denoise_level;

            if (noise_suppress_value > 100) {
                printf("APP Warning: front_end_denoise_level > 100!");
            }

            speex_preprocess_ctl(mediainput->front_end_processor,\
                                 SPEEX_PREPROCESS_SET_DENOISE,\
                                 &(app_parameter_.front_end_denoise_enable));

            speex_preprocess_ctl(mediainput->front_end_processor,\
                                 SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,\
                                 &(noise_suppress_value));
        }

        if (app_parameter_.agc_enable) {
            // Only support float type agc level currently.
            float agc_level = app_parameter_.agc_level_value * 327.68;

            speex_preprocess_ctl(mediainput->front_end_processor,\
                                 SPEEX_PREPROCESS_SET_AGC,\
                                 &(app_parameter_.agc_enable));

            speex_preprocess_ctl(mediainput->front_end_processor,\
                                 SPEEX_PREPROCESS_SET_AGC_LEVEL,\
                                 &(agc_level));
        }

        if (app_parameter_.vad_enable) {
            speex_preprocess_ctl(mediainput->front_end_processor,\
                                 SPEEX_PREPROCESS_SET_VAD,\
                                 &(app_parameter_.vad_enable));

            speex_preprocess_ctl(mediainput->front_end_processor,\
                                 SPEEX_PREPROCESS_SET_PROB_START,\
                                 &vad_prob_start);

            speex_preprocess_ctl(mediainput->front_end_processor,\
                                 SPEEX_PREPROCESS_SET_PROB_CONTINUE,\
                                 &vad_prob_continue);
        }
    }

    if (mediainput->front_end_processor) {
        ret = speex_preprocess_run(mediainput->front_end_processor, (short *)(audio_data));

        if (app_parameter_.vad_enable) {
            speex_preprocess_ctl(mediainput->front_end_processor,\
                                 SPEEX_PREPROCESS_GET_PROB,\
                                 &(vad_sort_ctx_[input_id].audio_speech_prob));
            mediainput->input_ctx->audio_vad_prob = vad_sort_ctx_[input_id].audio_speech_prob;

            if (ret) {
                mediainput->input_ctx->audio_input_active_status = 1;
                // Get power spectrum
                int psd_size = 0;
                float psd_avg = 0.0;
                speex_preprocess_ctl(mediainput->front_end_processor,\
                                     SPEEX_PREPROCESS_GET_PSD_SIZE,\
                                     &(psd_size));
                if (mediainput->buffers->psd_buf_size < psd_size) {
                    printf("APP[%p]: PsdSize[%d](%d) < psd_size(%d)\n",
                           this, input_id, mediainput->buffers->psd_buf_size, psd_size);
                    if (mediainput->buffers->buf_psd) {
                        free(mediainput->buffers->buf_psd);
                        mediainput->buffers->buf_psd = NULL;
                    }
                    mediainput->buffers->psd_buf_size = psd_size;
                }
                if (!(mediainput->buffers->buf_psd)) {
                    mediainput->buffers->buf_psd = (int *)malloc(mediainput->buffers->psd_buf_size * sizeof(int));
                }
                if (!(mediainput->buffers->buf_psd)) {
                    printf("APP[%p]: ERROR: Fail to malloc psd_array\n", this);
                    return ret;
                }
                speex_preprocess_ctl(mediainput->front_end_processor,\
                                     SPEEX_PREPROCESS_GET_PSD,\
                                     mediainput->buffers->buf_psd);
                // Calculate average power spectrum
                for (i = 0; i < psd_size; i++) {
                    psd_avg += (float)(mediainput->buffers->buf_psd[i]) / psd_size;
                }
#ifdef USD_PSD_MSD
                float psd_squared_sum = 0.0;
                float psd_deviation = 0.0;
                float psd_msd = 0.0;
                for (i = 0; i < psd_size; i++) {
                    psd_deviation = mediainput->buffers->buf_psd[i] - psd_avg;
                    psd_squared_sum += psd_deviation * psd_deviation;
                }
                psd_msd = sqrt(psd_squared_sum / psd_size);
                vad_sort_ctx_[input_id].audio_power_spect = (int)psd_msd;
#else
                vad_sort_ctx_[input_id].audio_power_spect = (int)psd_avg;
#endif
            } else {
                mediainput->input_ctx->audio_input_active_status = 0;
                vad_sort_ctx_[input_id].audio_power_spect = 0;
            }

            vad_sort_ctx_[input_id].audio_input_id = input_id;
            vad_sort_ctx_[input_id].media_input = mediainput;
        }
    }

    return ret;
}

int AudioPostProcessing::BackEndProcessRun(APPMediaInput *mediainput, unsigned int input_id, AudioPayload *payload)
{
    unsigned char *audio_data = payload->payload + mediainput->input_ctx->wav_data_offset;
    int ret = 0;

    if((mediainput->input_ctx->audio_frame_size_mix <= 0) ||
       (mediainput->input_ctx->audio_sample_rate_mix <= 0)) {
        return 0;
    }

    if (!(mediainput->back_end_processor)) {
        // Init speex back-end process status
        printf("APP[%p]: %s[%d]: frame_size_mix = %d, sample_rate_mix = %d\n",
               this,
               __FUNCTION__,
               input_id,
               mediainput->input_ctx->audio_frame_size_mix,
               mediainput->input_ctx->audio_sample_rate_mix);

        mediainput->back_end_processor = speex_preprocess_state_init(mediainput->input_ctx->audio_frame_size_mix >> 1,
                                                                     mediainput->input_ctx->audio_sample_rate_mix);
        if (NULL == mediainput->back_end_processor) {
            printf("APP [%p]: Back-end processor init failed!\n", this);
            return -1;
        }
        if (app_parameter_.back_end_denoise_enable) {
            int noise_suppress_value = app_parameter_.back_end_denoise_level;
            if (noise_suppress_value > 100) {
                printf("APP Warning: back_end_denoise_level > 100!");
            }
            speex_preprocess_ctl(mediainput->back_end_processor,\
                                 SPEEX_PREPROCESS_SET_DENOISE,\
                                 &(app_parameter_.back_end_denoise_enable));

            speex_preprocess_ctl(mediainput->back_end_processor,\
                                 SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,\
                                 &(noise_suppress_value));
        }
        if (app_parameter_.aec_enable) {
            //Init speex preprocess status
            int echo_tail = 1;    // between 1 and (mediainput->input_ctx->audio_frame_size_mix * 10)
            mediainput->echo_state = speex_echo_state_init(mediainput->input_ctx->audio_frame_size_mix >> 1,
                                                           echo_tail);
            if (NULL == mediainput->echo_state) {
                printf("APP [%p]: Failed to initialize echo state!\n", this);
                return -2;
            } else {
                printf("APP [%p]: Echo state initialized successfully. tail = %d\n", this, echo_tail);
            }
            speex_echo_ctl(mediainput->echo_state,
                           SPEEX_ECHO_SET_SAMPLING_RATE,
                           &(mediainput->input_ctx->audio_sample_rate_mix));
            speex_preprocess_ctl(mediainput->back_end_processor,
                                 SPEEX_PREPROCESS_SET_ECHO_STATE,
                                 mediainput->echo_state);
        }
    }

    if (mediainput->back_end_processor) {
        if (app_parameter_.aec_enable &&
            mediainput->echo_state &&
            (mediainput->input_ctx->audio_input_running_status != 0)) {
            //echo cancellation process
            speex_echo_cancellation(mediainput->echo_state,
                                    (short *)(audio_data),
                                    mediainput->input_ctx->audio_data_in,
                                    (short *)(audio_data));
        }
        ret = speex_preprocess_run(mediainput->back_end_processor, (short *)(audio_data));
    }

    return ret;
}

int AudioPostProcessing::FrontEndResample(APPMediaInput *mediainput, unsigned int input_id, AudioPayload *payload)
{
    int ret = 0;
    unsigned int nSizeToWrite, nSizeWrite;
    unsigned char *pBufDstData = NULL;
    unsigned int nSizeToRead = (payload->payload_length - mediainput->input_ctx->wav_data_offset) >> 1;
    unsigned char *audio_data_in = payload->payload + mediainput->input_ctx->wav_data_offset;

    if((mediainput->input_ctx->audio_input_running_status == 0) ||
       (mediainput->input_ctx->audio_sample_rate_in <= 0) ||
       (app_parameter_.front_end_sample_rate <= 0)) {
        APP_TRACE_INFO("APP: %s[%d]: sample_rate_in(%d) -> front_end_sample_rate (%d), return.\n",
                       __FUNCTION__,
                       input_id,
                       mediainput->input_ctx->audio_sample_rate_in,
                       app_parameter_.front_end_sample_rate);
        return 0;
    }

    if (!(mediainput->front_end_resampler)) {
        printf("APP: %s[%d]: sampe_rate_in(%d) -> front_end_sample_rate(%d), frame_size_in = %d\n",
               __FUNCTION__,
               input_id,
               mediainput->input_ctx->audio_sample_rate_in,
               app_parameter_.front_end_sample_rate,
               mediainput->input_ctx->audio_frame_size_in);

        // Init speex resampler status
        int resample_quality_level = 10;    // Between 0 and 10.
        mediainput->front_end_resampler = speex_resampler_init(mediainput->input_ctx->audio_channel_number_in,
                                                               mediainput->input_ctx->audio_sample_rate_in,
                                                               app_parameter_.front_end_sample_rate,
                                                               resample_quality_level,
                                                               &ret);

        if (!(mediainput->front_end_resampler)) {
            printf("APP: [Error]: Resampler Init failed ret = %d!\n", ret);
        }
    }

    if (mediainput->front_end_resampler) {
        if (app_parameter_.front_end_sample_rate <= mediainput->input_ctx->audio_sample_rate_in) {
            nSizeToWrite = nSizeToRead;
        } else {
            nSizeToWrite = nSizeToRead *
                           (1 + app_parameter_.front_end_sample_rate / mediainput->input_ctx->audio_sample_rate_in);
        }

        int size = (nSizeToWrite << 1) + mediainput->input_ctx->wav_data_offset;
        if (size <= 0) {
            printf("APP: [Error]: Resampler size = %d\n", size);
            return -1;
        }

        if (!(mediainput->buffers->buf_front_resample)) {
            mediainput->buffers->buf_front_resample = (short *)malloc(sizeof(short) * size);
            if (!(mediainput->buffers->buf_front_resample)) {
                printf("APP: [Error]: Failed to allocate memory! nSizeToWrite = %d\n", nSizeToWrite);
                return -1;
            }
        }
        pBufDstData = (unsigned char *)(mediainput->buffers->buf_front_resample) +
                      mediainput->input_ctx->wav_data_offset;

        nSizeWrite = nSizeToWrite;
        APP_TRACE_INFO("APP: input %d, nSizeToRead = %d, nSizeToWrite = %d\n", input_id, nSizeToRead, nSizeToWrite);

        // re-sampling process
        ret = speex_resampler_process_int(mediainput->front_end_resampler,
                                          0,
                                          (short *)audio_data_in,
                                          &nSizeToRead,
                                          (short *)pBufDstData,
                                          &nSizeWrite);
        if (!ret) {
            APP_TRACE_INFO("APP: [Info]: nSizeRead = %d, nSizeWrite = %d, nSizeToWrite = %d\n",
                            mediainput->input_ctx->audio_frame_size_in,
                            nSizeWrite,
                            nSizeToWrite);

#ifdef DUMP_FR_RESAMPLE_PCM
            if (input_id == 0) {
                FILE *fp;
                fp = fopen("dump_fr_resample.pcm", "ab+");
                fwrite(pBufDstData, sizeof(short), nSizeWrite, fp);
                fclose(fp);
            }
#endif

            mediainput->input_ctx->wav_info_out.sample_rate = app_parameter_.front_end_sample_rate;
            mediainput->input_ctx->wav_header_out.freq = app_parameter_.front_end_sample_rate;
            mediainput->input_ctx->wav_header_out.Populate(&(mediainput->input_ctx->wav_info_out),
                                                           nSizeWrite << 1);
            memcpy(mediainput->buffers->buf_front_resample,
                   (unsigned char*) &(mediainput->input_ctx->wav_header_out),
                   mediainput->input_ctx->wav_header_out.GetHeaderSize());
            payload->payload_length = (nSizeWrite << 1) + mediainput->input_ctx->wav_header_out.GetHeaderSize();
            payload->payload = (unsigned char*)(mediainput->buffers->buf_front_resample);

            mediainput->input_ctx->audio_payload = *payload;
            mediainput->input_ctx->audio_data_in = mediainput->buffers->buf_front_resample +
                                                   (mediainput->input_ctx->wav_data_offset >> 1);
            mediainput->input_ctx->audio_payload_length = payload->payload_length;
            mediainput->input_ctx->audio_frame_size_mix = nSizeWrite << 1;
            APP_TRACE_INFO("APP: Front-end Resample [%d], frame_size_mix = %d, sample_rate_mix = %d\n",
                           input_id,
                           mediainput->input_ctx->audio_frame_size_mix,
                           mediainput->input_ctx->audio_sample_rate_mix);

        } else {
            APP_TRACE_INFO("APP: [Error]: speex_resampler_process_int() returns %d\n", ret);
        }
    }
    return ret;
}

int AudioPostProcessing::BackEndResample(APPMediaInput *mediainput, unsigned int input_id, AudioPayload *payload)
{
    int ret = 0;
    unsigned int nSizeToWrite, nSizeWrite;
    unsigned char *pBufDstData = NULL;
    unsigned int nSizeToRead = (payload->payload_length - mediainput->input_ctx->wav_data_offset) >> 1;
    unsigned char *audio_data_in = payload->payload + mediainput->input_ctx->wav_data_offset;

    if ((mediainput->input_ctx->audio_sample_rate_mix <= 0) ||
        (app_parameter_.back_end_sample_rate <= 0)) {
        printf("APP: %s[%d], sample_rate_mix(%d) -> back_end_sample_rate(%d), return\n",
                       __FUNCTION__,
                       input_id,
                       mediainput->input_ctx->audio_sample_rate_mix,
                       app_parameter_.back_end_sample_rate);
        return 0;
    }

    if (!(mediainput->back_end_resampler)) {
        printf("APP: %s[%d], sample_rate_mix(%d) -> back_end_sample_rate(%d), frame_size_mix = %d\n",
               __FUNCTION__,
               input_id,
               mediainput->input_ctx->audio_sample_rate_mix,
               app_parameter_.back_end_sample_rate,
               mediainput->input_ctx->audio_frame_size_mix);

        // Init speex resampler status
        int resample_quality_level = 5;    // Between 0 and 10.
        mediainput->back_end_resampler = speex_resampler_init(mediainput->input_ctx->audio_channel_number,
                                                              mediainput->input_ctx->audio_sample_rate_mix,
                                                              app_parameter_.back_end_sample_rate,
                                                              resample_quality_level,
                                                              &ret);

        if (!(mediainput->back_end_resampler)) {
            printf("APP: [Error]: Resampler Init failed ret = %d!\n", ret);
        }
    }

    if (mediainput->back_end_resampler) {
        if (app_parameter_.back_end_sample_rate <= mediainput->input_ctx->audio_sample_rate_mix) {
            nSizeToWrite = nSizeToRead;
        } else {
            nSizeToWrite = nSizeToRead *
                           (1 + app_parameter_.back_end_sample_rate / mediainput->input_ctx->audio_sample_rate_mix);
        }

        int size = (nSizeToWrite << 1) + mediainput->input_ctx->wav_data_offset;
        if (size <= 0) {
            printf("APP: [Error]: Resampler size = %d\n", size);
            return -1;
        }

        if (!(mediainput->buffers->buf_back_resample)) {
            mediainput->buffers->buf_back_resample = (short *)malloc(sizeof(short) * size);
            if (!(mediainput->buffers->buf_back_resample)) {
                printf("APP: [Error]: Failed to allocate memory! nSizeWrite = %d\n", nSizeToWrite);
                return -1;
            }
        }
        pBufDstData = (unsigned char *)(mediainput->buffers->buf_back_resample) +
                      mediainput->input_ctx->wav_data_offset;

        nSizeWrite = nSizeToWrite;
        APP_TRACE_INFO("APP: nSizeToRead = %d, nSizeToWrite = %d\n", nSizeToRead, nSizeToWrite);

        // re-sampling process
        ret = speex_resampler_process_int(mediainput->back_end_resampler,
                                              0,
                                              (short *)audio_data_in,
                                              &nSizeToRead,
                                              (short *)pBufDstData,
                                              &nSizeWrite);

        if (!ret) {
            APP_TRACE_INFO("APP: [Info]: nSizeRead = %d, nSizeWrite = %d, nSizeToWrite = %d\n",
                           mediainput->input_ctx->audio_frame_size_mix, nSizeWrite, nSizeToWrite);

#ifdef DUMP_BK_RESAMPLE_PCM
            if (input_id == 0) {
                FILE *fp;
                fp = fopen("dump_bk_resample.pcm", "ab+");
                fwrite(pBufDstData, sizeof(short), nSizeWrite, fp);
                fclose(fp);
            }
#endif

            mediainput->input_ctx->wav_info_out.sample_rate = app_parameter_.back_end_sample_rate;
            mediainput->input_ctx->wav_header_out.freq = app_parameter_.back_end_sample_rate;
            mediainput->input_ctx->wav_header_out.Populate(&(mediainput->input_ctx->wav_info_out),
                                                           nSizeWrite << 1);
            memcpy(mediainput->buffers->buf_back_resample,
                   (unsigned char*) &(mediainput->input_ctx->wav_header_out),
                   mediainput->input_ctx->wav_header_out.GetHeaderSize());
            payload->payload_length = (nSizeWrite << 1) +
                                      mediainput->input_ctx->wav_header_out.GetHeaderSize();
            payload->payload = (unsigned char*)(mediainput->buffers->buf_back_resample);
            mediainput->input_ctx->audio_frame_size_out = nSizeWrite << 1;
            APP_TRACE_INFO("APP: Back-end Resample: input %d, frame_size_out = %d, sample_rate_mix = %d\n",
                            input_id,
                            mediainput->input_ctx->audio_frame_size_out,
                            mediainput->input_ctx->audio_sample_rate_mix);
        } else {
            APP_TRACE_INFO("APP: [Error]: speex_resampler_process_int() returns %d\n", ret);
        }
    }

    return ret;
}

int AudioPostProcessing::ChannelNumberConvert(APPMediaInput *mediainput, unsigned int input_id, AudioPayload *payload)
{
    unsigned int write_size_16b = 0;
    unsigned char *audio_data_out = NULL;
    unsigned int read_size_16b = (payload->payload_length - mediainput->input_ctx->wav_data_offset) >> 1;
    unsigned char *audio_data_in = payload->payload + mediainput->input_ctx->wav_data_offset;

    if ((mediainput->input_ctx->audio_input_running_status == 0) ||
        (mediainput->input_ctx->audio_channel_number_in == app_parameter_.channel_number)) {
        return 0;
    }

    if (mediainput->input_ctx->audio_channel_number_in == 1) {  //mono to stereo
        write_size_16b = read_size_16b << 1;
    } else if (mediainput->input_ctx->audio_channel_number_in == 2) { //stereo to mono
        write_size_16b = read_size_16b >> 1;
    } else {
        printf("APP: Error: Only support conversion between mono and stereo!");
        return 0;
    }
    int convert_buf_size = (write_size_16b << 1) + mediainput->input_ctx->wav_data_offset;

    // Allocate buffer for channel number conversion
    if (!(mediainput->buffers->buf_channel_convert)) {
        mediainput->buffers->buf_channel_convert = (short *)malloc(sizeof(short) * convert_buf_size);
        printf("APP: Allocate memory for %s[%d]\n", __FUNCTION__, input_id);
        if (!(mediainput->buffers->buf_channel_convert)) {
            printf("APP: [Error]: Failed to allocate memory! write_size_16b = %d\n", write_size_16b);
            return -1;
        }
    }
    audio_data_out = (unsigned char *)(mediainput->buffers->buf_channel_convert) + mediainput->input_ctx->wav_data_offset;

    // Channel number conversion
    if (mediainput->input_ctx->audio_channel_number_in == 1) {
        MonoToStereo((short *)audio_data_out, (short *)audio_data_in, read_size_16b);
    } else if (mediainput->input_ctx->audio_channel_number_in == 2) {
        StereoToMono((short *)audio_data_out, (short *)audio_data_in, read_size_16b);
    }

#ifdef DUMP_CHANNEL_NUM_CONVERT_PCM
        if (input_id == 0) {
            FILE *fp;
            fp = fopen("dump_chnum_convert.pcm", "ab+");
            fwrite(audio_data_out, sizeof(short), write_size_16b, fp);
            fclose(fp);
        }
#endif

    // Build the output payload packet
    APP_TRACE_DEBUG("read_size_16b = %d, write_size_16b = %d\n", read_size_16b, write_size_16b);
    mediainput->input_ctx->wav_info_out.channels_number = app_parameter_.channel_number;
    mediainput->input_ctx->wav_header_out.Populate(&(mediainput->input_ctx->wav_info_out), write_size_16b << 1);
    memcpy(mediainput->buffers->buf_channel_convert,
           (unsigned char*) &(mediainput->input_ctx->wav_header_out),
           mediainput->input_ctx->wav_header_out.GetHeaderSize());
    payload->payload_length = (write_size_16b << 1) + mediainput->input_ctx->wav_header_out.GetHeaderSize();
    payload->payload = (unsigned char*)(mediainput->buffers->buf_channel_convert);

    // Update APP context variable value.
    mediainput->input_ctx->audio_payload = *payload;
    mediainput->input_ctx->audio_data_in = mediainput->buffers->buf_channel_convert +
                                           (mediainput->input_ctx->wav_data_offset >> 1);
    mediainput->input_ctx->audio_payload_length = payload->payload_length;
    mediainput->input_ctx->audio_frame_size_mix = write_size_16b << 1;
    mediainput->input_ctx->audio_channel_number = app_parameter_.channel_number;
    APP_TRACE_DEBUG("APP: input %d, frame_size_mix = %d, channel_number[input_id] = %d\n",
                    input_id,
                    mediainput->input_ctx->audio_frame_size_mix,
                    mediainput->input_ctx->audio_channel_number);

    return 0;
}

void AudioPostProcessing::VADSort()
{
    qsort(vad_sort_ctx_, num_input_, sizeof(vad_sort_ctx_[0]), ComparePowerSpect);
    return;
}

void* AudioPostProcessing::GetActiveInputHandle()
{
    std::list<APPMediaInput *>::iterator it_mediainput;
    APPMediaInput *mediainput = NULL;
    BaseElement *element_dec = NULL;

    mediainput = vad_sort_ctx_[0].media_input;
    if (!mediainput) {
        printf("APP: %s, mediainput (%p), return NULL!\n", __FUNCTION__, mediainput);
        return NULL;
    }
    element_dec = mediainput->pad->get_peer_pad()->get_parent();
    printf("APP: Active dec is %p, input_id = %d\n", element_dec, vad_sort_ctx_[0].audio_input_id);

    return element_dec;
}

int AudioPostProcessing::GetInputActiveStatus(void *input_handle)
{
    std::list<APPMediaInput *>::iterator it_mediainput;
    APPMediaInput *mediainput = NULL;
    int id = 0;
    int status = -1;

    BaseElement *element_dec = static_cast<BaseElement*>(input_handle);

    for (it_mediainput = media_input_list_.begin();
         it_mediainput != media_input_list_.end();
         it_mediainput++, id++) {
        mediainput = *it_mediainput;
        if (element_dec == mediainput->pad->get_peer_pad()->get_parent()) {
            status = mediainput->input_ctx->audio_input_active_status;
            APP_TRACE_DEBUG("APP: Found dec(%p), id = %d, active status = %d\n", element_dec, id, status);
            break;
        }
    }

    return status;
}

int AudioPostProcessing::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    AudioPayload pIn;
    pIn.payload = buf.payload;
    pIn.payload_length = buf.payload_length;
    pIn.isFirstPacket = buf.isFirstPacket;

    APP_TRACE_INFO("APP will process the payload %p\n", &pIn);

    int ret;
    ret = pad->PushBufData(buf);
    APP_TRACE_INFO("APP: pad %p PushBufData\n", pad);
    if (ret != 0) {
        APP_TRACE_INFO("PushBufData(buf) error!\n");
        return -1;
    }

    // Step 2: find the relative input handle.
    APPMediaInput *mediainput = NULL;
    std::list<APPMediaInput *>::iterator it_mediainput;

    for (it_mediainput = media_input_list_.begin();
         it_mediainput != media_input_list_.end();
         it_mediainput++) {
        if ((*it_mediainput)->pad == pad) {
            mediainput = *it_mediainput;
            // If EOF, set input status to STOP
            if (NULL == pIn.payload) {
                mediainput->status = STOP;
                UpdateMediaInput(mediainput, buf);
            }
            if (mediainput->pad->GetBufQueueSize() > 0) {
                // update the media input status
                if (mediainput->status == INITIALIZED) {
                    mediainput->status = RUNNING;
                }
            }
            break;
        }
    }

    // Step 3: process all input when the host input buffer is not empty.
    if (mediainput == host_input_) {
        while (host_input_->pad->GetBufQueueSize() > 0) {
            APP_TRACE_INFO("APP: host_input_->pad->GetBufQueueSize() = %d\n", host_input_->pad->GetBufQueueSize());
            ProcessInput();
            // Step 4: release the outdated surface.
            for (it_mediainput = media_input_list_.begin();
                 it_mediainput != media_input_list_.end();
                 it_mediainput++) {
                MediaPad* pad_i;
                int remaining;
                MediaBuf buffer;
                mediainput = *it_mediainput;
                pad_i = mediainput->pad;
                if (mediainput != host_input_) {
                    remaining = 1;
                } else {
                    remaining = 0;
                }
                APP_TRACE_DEBUG("APP: pad_i(%p)->GetBufQueueSize() = %d, remaining = %d\n",
                            pad_i, pad_i->GetBufQueueSize(), remaining);
                if (pad_i->GetBufQueueSize() > remaining) {
                    pad_i->GetBufData(buffer);
                    pad_i->ReturnBufToPeerPad(buffer);
                }
            }
        }
    }
    return 1;
}

bool AudioPostProcessing::ProcessInput()
{
    APP_TRACE_INFO("APP: %s: line %d, total_frame_count_ = %lld\n", __FUNCTION__, __LINE__, total_frame_count_++);
    APPMediaInput *mediainput = NULL;
    MediaBuf buffer;
    int ret = 0;
    int input_id = 0;
    MediaPad *pad;
    std::list<APPMediaInput *>::iterator it_mediainput;
    bool eof = true;

    num_running_input_ = 0;
    // Get every buffer from every input and prepare the input parameters.
    if (pthread_mutex_lock(&mutex_)) {
        printf("Error in pthread_mutex_lock.\n");
        assert(0);
    }

    for (it_mediainput = media_input_list_.begin();
         it_mediainput != media_input_list_.end();
         it_mediainput++, input_id++) {
        mediainput = *it_mediainput;
        if (mediainput->status != RUNNING) {
            APP_TRACE_INFO("APP mediainput->status = %d, input_id = %d\n", mediainput->status, input_id);
            continue;
        }
        APP_TRACE_INFO("APP: pad %p->GetBufQueueSize() = %d\n", mediainput->pad, mediainput->pad->GetBufQueueSize());

        // Pick the buffer from the queue.
        pad = mediainput->pad;
        ret = pad->PickBufData(buffer);
        if (ret != 0) {
            APP_TRACE_INFO("APP: Input %d PickBufData Failed\n", input_id);
            continue;
        }

        AudioPayload pIn;
        pIn.payload = buffer.payload;
        pIn.payload_length = buffer.payload_length;
        pIn.isFirstPacket = buffer.isFirstPacket;
        APP_TRACE_INFO("APP PickBufData: the input payload %p, mediainput = %p, pad = %p, num_running_input_ = %d\n",
                    pIn.payload, mediainput, pad, num_running_input_);

        if (NULL == pIn.payload) {
            printf("APP: Audio payload is NULL. Input %d get EOF!\n", input_id);
            mediainput->status = STOP;
        } else {
            eof = false;
            // Retrieve the raw audio parameters
            APP_TRACE_INFO("input_id = %d, pIn.payload = %p\n", input_id, pIn.payload);

            mediainput->input_ctx->wav_data_offset = mediainput->input_ctx->wav_header_in.Interpret(pIn.payload,
                                                                            &(mediainput->input_ctx->wav_info_in),
                                                                            pIn.payload_length);
            if ((pIn.payload_length <= mediainput->input_ctx->wav_data_offset) ||
                (mediainput->input_ctx->wav_data_offset <= 0)) {
                printf("APP: %s: input payload size (%d) is lower than data dataOffset[%d](%d)\n",
                            __FUNCTION__, pIn.payload_length, input_id, mediainput->input_ctx->wav_data_offset);

            } else {
                ret = PrepareParameter(mediainput, buffer, input_id);
                num_running_input_++;
            }
        }
    }
    num_input_ = input_id;

    if (num_running_input_ == 0 || ret != 0) {
        if (pthread_mutex_unlock(&mutex_) != 0) {
            printf("APP: pthread_mutex_unlock error!\n");
        }
        return eof;
    }

    ComposeAudioFrame();

    if (pthread_mutex_unlock(&mutex_) != 0) {
        printf("APP: pthread_mutex_unlock error!\n");
    }
    return false;
}

int AudioPostProcessing::ComposeAudioFrame()
{
    MediaBuf OutBuf[MAX_APP_INPUT];
    int ret = 0;
    int i = 0;
    int num_output = num_input_;
    APPMediaInput *mediainput = NULL;
    std::list<APPMediaInput *>::iterator it_mediainput;

    // Front-end process, including de-noise, VAD, AGC.
    for (it_mediainput = media_input_list_.begin();
         it_mediainput != media_input_list_.end();
         it_mediainput++, i++) {
        mediainput = *it_mediainput;
        if (app_parameter_.vad_enable ||
            app_parameter_.agc_enable ||
            app_parameter_.front_end_denoise_enable) {
             ret = FrontEndProcessRun(mediainput, i, &(mediainput->input_ctx->audio_payload));
        }

        // Front-end re-sampling
        if (app_parameter_.front_end_resample_enable) {
            ret = FrontEndResample(mediainput, i, &(mediainput->input_ctx->audio_payload));
        }

        // Channel number conversion
        if (app_parameter_.channel_number_convert_enable) {
            ChannelNumberConvert(mediainput, i, &(mediainput->input_ctx->audio_payload));
        }

        OutBuf[i].payload = mediainput->input_ctx->audio_payload.payload;
        OutBuf[i].payload_length = mediainput->input_ctx->audio_payload.payload_length;
    }

    // This is for VAD internal test. Check the VAD status every 100frames.
    if ((app_parameter_.vad_enable) && ((total_frame_count_ - 15) % 100 == 0)) {
        // Sort power spectrum and get the input with most higher probability
        VADSort();
        active_input_id_ = vad_sort_ctx_[0].audio_input_id;

        APP_TRACE_DEBUG("\nFrame %3lld: ", total_frame_count_);
        for (i = 0; i < num_input_; i++) {
            APP_TRACE_DEBUG("power_spect[%d] = %4d\t", vad_sort_ctx_[i].audio_input_id,
                                                       vad_sort_ctx_[i].audio_power_spect);
            APP_TRACE_DEBUG("speech_prob[%d] = %2d\t", vad_sort_ctx_[i].audio_input_id,
                                                       vad_sort_ctx_[i].audio_speech_prob);
        }

        if (vad_sort_ctx_[0].audio_speech_prob > VAD_PROB_START) {
            printf("APP: Input %d is active.\n", vad_sort_ctx_[0].audio_input_id);
            APP_TRACE_DEBUG("APP: Speech Probability is %d\n", vad_sort_ctx_[0].audio_speech_prob);
        }
    }

    if(!app_parameter_.nn_mixing_enable) {
        num_output = 1;
    }

    for (it_mediainput = media_input_list_.begin(), i = 0;
         it_mediainput != media_input_list_.end() && i < num_output;
         it_mediainput++, i++) {
        mediainput = *it_mediainput;
        // Process audio mixing
        ret = AudioMix(mediainput, OutBuf[i], i);
        if (ret) {
            OutBuf[i].payload_length = 0;
            OutBuf[i].payload = NULL;
            APP_TRACE_INFO("APP: %s AudioMix ret %d, num_input_ = %d, num_output = %d\n",
                           __FUNCTION__,
                           ret,
                           num_input_,
                           num_output);
            continue;
        }

        AudioPayload pOut;
        pOut.payload = OutBuf[i].payload;
        pOut.payload_length = OutBuf[i].payload_length;

        // Back-end process, including de-noise and AEC.
        if (app_parameter_.back_end_denoise_enable ||
            app_parameter_.aec_enable) {
             BackEndProcessRun(mediainput, i, &pOut);
        }

        // Back-end re-sampling
        if (app_parameter_.back_end_resample_enable) {
            ret = BackEndResample(mediainput, i, &pOut);
        }

        // Prepare the output buffer
        OutBuf[i].isFirstPacket = mediainput->first_packet;
#ifdef DUMP_APP_OUT_PCM
        if (!(file_dump_app_[i])) {
            char file_name[FILENAME_MAX] = {0};
            sprintf(file_name, "dump_app_out%d_%p.wav", i, this);
            file_dump_app_[i] = fopen(file_name, "wb+");
            printf("APP[%p]: Dump APP output to %s, file_dump_app_[%d]  = %p\n", this, file_name, i, file_dump_app_[i]);
        }
        if (file_dump_app_[i]) {
            if (mediainput->first_packet) {
                mediainput->input_ctx->wav_info_out.channels_number = 2;    // Special case for debugging in WebRTC scenario
                mediainput->input_ctx->wav_header_out.Populate(&(mediainput->input_ctx->wav_info_out), 0xFFFFFFFF);
                mediainput->input_ctx->wav_info_out.channels_number = 1;    // Special case for debugging in WebRTC scenario
                fwrite(&(mediainput->input_ctx->wav_header_out),
                       sizeof(unsigned char),
                       mediainput->input_ctx->wav_header_out.GetHeaderSize(),
                       file_dump_app_[i]);
            }
            fwrite(pOut.payload + mediainput->input_ctx->wav_data_offset,
                       sizeof(unsigned char),
                       mediainput->input_ctx->audio_frame_size_in,
                       file_dump_app_[i]);
        }
#endif
        if (mediainput->first_packet) {
            mediainput->first_packet = false;
        }
        OutBuf[i].payload = pOut.payload;
        OutBuf[i].payload_length = pOut.payload_length;

        APP_TRACE_DEBUG("APP: push the payload %p to next element, length = %d\n",
                        pOut.payload,
                        pOut.payload_length);
    }

    for (it_mediainput = media_input_list_.begin(), i = 0;
         it_mediainput != media_input_list_.end();
         it_mediainput++, i++) {
        mediainput = *it_mediainput;
        mediainput->input_ctx->audio_input_running_status = 0;
        mediainput->input_ctx->audio_vad_prob = 0;
        vad_sort_ctx_[i].audio_power_spect = 0;
        vad_sort_ctx_[i].audio_speech_prob = 0;
    }

    ReleaseBuffer(OutBuf);

    return 0;
}

int AudioPostProcessing::PrepareParameter(APPMediaInput *mediainput, MediaBuf &buffer, unsigned int input_id)
{
    AudioPayload pIn;
    pIn.payload = buffer.payload;
    pIn.payload_length = buffer.payload_length;
    pIn.isFirstPacket = buffer.isFirstPacket;

    mediainput->input_ctx->audio_payload = pIn;
    mediainput->input_ctx->audio_data_in = (short *)(pIn.payload + mediainput->input_ctx->wav_data_offset);
    mediainput->input_ctx->audio_payload_length = pIn.payload_length;
    mediainput->input_ctx->audio_frame_size_in = pIn.payload_length - mediainput->input_ctx->wav_data_offset;
    mediainput->input_ctx->audio_input_id = input_id;
    mediainput->input_ctx->audio_input_running_status = 1;

    // Init input and mixed related parameters for each channel
    if (!(mediainput->input_ctx->audio_param_inited)) {
        mediainput->input_ctx->wav_data_offset = mediainput->input_ctx->wav_header_in.Interpret(pIn.payload,
                                                                        &(mediainput->input_ctx->wav_info_in),
                                                                        pIn.payload_length);
        mediainput->input_ctx->audio_sample_rate_in = mediainput->input_ctx->wav_info_in.sample_rate;
        mediainput->input_ctx->audio_channel_number_in = mediainput->input_ctx->wav_info_in.channels_number;

        if (app_parameter_.front_end_sample_rate == 0) {
            app_parameter_.front_end_sample_rate = mediainput->input_ctx->audio_sample_rate_in;
        }
        // Automatically enable re-sample for different sample rate inputs case
        if (mediainput->input_ctx->audio_sample_rate_in != app_parameter_.front_end_sample_rate) {
            if (!app_parameter_.front_end_resample_enable) {
                app_parameter_.front_end_resample_enable = true;
            }
            printf("APP: [%d]: Enable front-end re-sample to support inputs with different sample rate!\n",\
                   input_id);
        }
        mediainput->input_ctx->audio_sample_rate_mix = app_parameter_.front_end_sample_rate;

        if (app_parameter_.channel_number == 0) {
            app_parameter_.channel_number = mediainput->input_ctx->audio_channel_number_in;
        }
        // Automatically enable channel num conversion for different channel num inputs case
        if (mediainput->input_ctx->audio_channel_number_in != app_parameter_.channel_number) {
            if (!app_parameter_.channel_number_convert_enable) {
                app_parameter_.channel_number_convert_enable = true;
            }
            printf("APP: [%d]: Enable channel num conversion to support inputs with different channel num!\n",
                   input_id);
        }
        mediainput->input_ctx->audio_channel_number = app_parameter_.channel_number;

        // Calculate the mixed frame size according to the audio mixed parameters. (60ms as one frame)
        // Note: The calculation of frame size should be sychronous with PCM reader or decoder.
        mediainput->input_ctx->audio_frame_size_mix = mediainput->input_ctx->audio_channel_number *
                                  mediainput->input_ctx->audio_sample_rate_mix * 3 *
                                  (mediainput->input_ctx->wav_info_in.resolution >> 3) / 50;

        // Calculate the mixed buffer size
        mediainput->buffers->mix_buf_size = mediainput->input_ctx->audio_frame_size_mix
                                            + mediainput->input_ctx->wav_data_offset;

        mediainput->input_ctx->audio_param_inited = true;
    }

    // Init mixed and output related parameters for all channels
    APPMediaInput *input = NULL;
    std::list<APPMediaInput *>::iterator it_mediainput;
    int i = 0;
    if (!out_param_inited_) {
        for (it_mediainput = media_input_list_.begin();
             it_mediainput != media_input_list_.end();
             it_mediainput++, i++) {
            input = *it_mediainput;
            input->input_ctx->wav_data_offset = input->input_ctx->wav_header_in.Interpret(pIn.payload,
                                                                              &(input->input_ctx->wav_info_in),
                                                                              pIn.payload_length);
            input->input_ctx->wav_header_out.Interpret(pIn.payload, &(input->input_ctx->wav_info_out), pIn.payload_length);
            input->input_ctx->audio_sample_rate_mix = mediainput->input_ctx->audio_sample_rate_mix;
            input->input_ctx->audio_frame_size_mix = mediainput->input_ctx->audio_frame_size_mix;
            input->input_ctx->audio_channel_number = mediainput->input_ctx->audio_channel_number;
            input->input_ctx->wav_info_out.channels_number = mediainput->input_ctx->audio_channel_number;
            input->buffers->mix_buf_size = mediainput->buffers->mix_buf_size;
        }
        out_param_inited_ = true;
    }

    return 0;
}

int AudioPostProcessing::AudioMix(APPMediaInput *mediainput, MediaBuf &out_buf, int input_id)
{
    int i, k, input_m, nSizeRead;
    short *pAudioDataIn, *pAudioDataOut;
    APPMediaInput *mediainput_m = NULL;
    std::list<APPMediaInput *>::iterator it_mediainput;

    // Find the first valid input.
    input_m = -1;
    for (it_mediainput = media_input_list_.begin(), i = 0;
         it_mediainput != media_input_list_.end();
         it_mediainput++, i++) {
        mediainput_m = *it_mediainput;
        if ((app_parameter_.nn_mixing_enable && (i == input_id)) ||
            (mediainput_m->input_ctx->audio_input_running_status == 0)) {
            continue;
        } else {
            input_m = i;
            break;
        }
    }
    if (input_m < 0) {
        APP_TRACE_INFO("APP: No input data needed to mix.\n");
        return -1;
    }

    // Check mixer buffer size. If it's smaller than the payload length, change
    // the buffer size and re-allocate the buffer.
    if (mediainput->buffers->mix_buf_size < mediainput_m->input_ctx->audio_payload_length) {
        if (mediainput->buffers->buf_mix) {
            printf("APP: Warning: mixer buffer size(%d) is smaller than payload length(%d)!\n",
                   mediainput->buffers->mix_buf_size, mediainput_m->input_ctx->audio_payload_length);
            free(mediainput->buffers->buf_mix);
            mediainput->buffers->buf_mix = NULL;
        }
        mediainput->buffers->mix_buf_size = mediainput_m->input_ctx->audio_payload_length;
    }
    // Allocate mixer buffer
    if (!(mediainput->buffers->buf_mix)) {
        if (mediainput->buffers->mix_buf_size > 0) {
            mediainput->buffers->buf_mix = (short *)malloc(sizeof(short) * (mediainput->buffers->mix_buf_size >> 1));
            if (!(mediainput->buffers->buf_mix)) {
                printf("APP: [Error]: Failed to allocate memory! size = %d\n", mediainput->buffers->mix_buf_size >> 1);
                return -2;
            }
        } else {
            printf("APP: Invalid size = %d\n", mediainput->buffers->mix_buf_size >> 1);
            return -2;
        }
    }

    short *pBufDstData = mediainput->buffers->buf_mix + (mediainput_m->input_ctx->wav_data_offset >> 1);
    memset(mediainput->buffers->buf_mix, 0, mediainput->buffers->mix_buf_size >> 1);
    memcpy(mediainput->buffers->buf_mix,
           mediainput_m->input_ctx->audio_payload.payload,
           mediainput_m->input_ctx->audio_payload_length);
    out_buf.payload = (unsigned char*)(mediainput->buffers->buf_mix);
    pAudioDataOut = pBufDstData;

    // No need to mix for only one input case.
    if (num_input_ < 2 || num_running_input_ < 2) {
        out_buf.payload_length = mediainput_m->input_ctx->audio_payload_length;
        return 0;
    }

    nSizeRead = mediainput_m->input_ctx->audio_frame_size_mix;
    for (it_mediainput = media_input_list_.begin(), i = 0;
         it_mediainput != media_input_list_.end();
         it_mediainput++, i++) {
        mediainput = *it_mediainput;
        if ((i != input_m) && (mediainput->input_ctx->audio_input_running_status == 1)) {
            // If N:N mixer is enabled, exclude the input itself. Only mix other inputs.
            if (app_parameter_.nn_mixing_enable && (i == input_id)) {
                continue;
            }
            // If VAD is enabled, exclude the input without active voice. Only mix inputs with active voice.
            if (app_parameter_.vad_enable && (mediainput->input_ctx->audio_input_active_status == 0)) {
                continue;
            }

            pAudioDataIn = mediainput->input_ctx->audio_data_in;

            // If mixer buffer size is smaller than payload length of current inputs,
            // report error message and reduce frame size to avoid overstepping the bundary
            if (mediainput->buffers->mix_buf_size < mediainput->input_ctx->audio_payload_length) {
                APP_TRACE_ERROR("APP: [Error]: MixBufSize(%d) < payload_length(%d)\n",
                                mediainput->buffers->mix_buf_size,
                                mediainput->input_ctx->audio_payload_length);
                mediainput->input_ctx->audio_frame_size_mix = mediainput->buffers->mix_buf_size
                                                              - mediainput->input_ctx->wav_data_offset;
            }

            if (nSizeRead < mediainput->input_ctx->audio_frame_size_mix) {
                nSizeRead = mediainput->input_ctx->audio_frame_size_mix;
            }

            for (k = 0; k < (nSizeRead >> 1); k++) {
                pAudioDataOut[k] = MIX2(pAudioDataOut[k], pAudioDataIn[k], 15); // only for 16bit case currently
            }
        }
    }

    out_buf.payload_length = nSizeRead + mediainput_m->input_ctx->wav_data_offset;

    return 0;
}

void AudioPostProcessing::ReleaseBuffer(MediaBuf *out)
{
    WaitSrcMutex();
    std::list<MediaPad *>::iterator srcpad;
    int i = 0;
    for (srcpad = (srcpads_.begin()); srcpad != (srcpads_.end()); srcpad++, i++) {
        if ((*srcpad)->get_pad_status() == MEDIA_PAD_LINKED) {
                (*srcpad)->PushBufToPeerPad(out[i]);
        }
    }
    ReleaseSrcMutex();
}

void AudioPostProcessing::NewPadAdded(MediaPad *pad)
{
    APP_TRACE_INFO("APP Add a new pad %p, Direction is %d, Status is %d\n",
                   pad, pad->get_pad_direction(),
                   pad->get_pad_status());
    APPMediaInput *mediainput;

    MediaPadDirection direction = pad->get_pad_direction();

    if (direction == MEDIA_PAD_SINK) {
        mediainput = new APPMediaInput;
        memset(mediainput, 0, sizeof(APPMediaInput));
        mediainput->element = this;
        mediainput->pad = pad;
        mediainput->status = INITIALIZED;
        mediainput->check_ctx = (CheckContext *)malloc(sizeof(CheckContext));
        if (!(mediainput->check_ctx)) {
            printf("ERROR: fail to malloc\n");
        } else {
            memset(mediainput->check_ctx, 0, sizeof(CheckContext));
        }
        mediainput->input_ctx = (APPInputContext *)malloc(sizeof(APPInputContext));
        if (!(mediainput->input_ctx)) {
            printf("ERROR: fail to malloc APPInputContext\n");
        } else {
            memset(mediainput->input_ctx, 0, sizeof(APPInputContext));
        }

        mediainput->buffers = (APPInputBuffers *)malloc(sizeof(APPInputBuffers));
        if (!(mediainput->buffers)) {
            printf("ERROR: fail to malloc APPInputBuffers\n");
        } else {
            memset(mediainput->buffers, 0, sizeof(APPInputBuffers));
        }
        mediainput->first_packet = true;
        mediainput->front_end_processor = NULL;
        mediainput->back_end_processor = NULL;
        mediainput->front_end_resampler = NULL;
        mediainput->back_end_resampler = NULL;
        mediainput->echo_state = NULL;
        media_input_list_.push_back(mediainput);

        if (media_input_list_.size() == 1) {
            host_input_ = mediainput;
        }
    }
}

void AudioPostProcessing::PadRemoved(MediaPad *pad)
{
    std::list<APPMediaInput *>::iterator it_mediainput;
    APPMediaInput *mediainput = NULL;

    MediaPadDirection direction = pad->get_pad_direction();
    if (direction == MEDIA_PAD_SINK) {
        for (it_mediainput = media_input_list_.begin();
             it_mediainput != media_input_list_.end();
             it_mediainput++) {
            mediainput = *it_mediainput;
            if (mediainput->pad == pad) {
                mediainput->status = STOP;
                if (mediainput->check_ctx) {
                    if (mediainput->check_ctx->processor_state) {
                        speex_preprocess_state_destroy(mediainput->check_ctx->processor_state);
                        mediainput->check_ctx->processor_state = NULL;
                    }
                    free(mediainput->check_ctx);
                    mediainput->check_ctx = NULL;
                }
                if (mediainput->input_ctx) {
                    free(mediainput->input_ctx);
                    mediainput->input_ctx = NULL;
                }
                if (mediainput->buffers) {
                    if (mediainput->buffers->buf_front_resample) {
                        free(mediainput->buffers->buf_front_resample);
                        mediainput->buffers->buf_front_resample = NULL;
                    }
                    if (mediainput->buffers->buf_back_resample) {
                        free(mediainput->buffers->buf_back_resample);
                        mediainput->buffers->buf_back_resample = NULL;
                    }
                    if (mediainput->buffers->buf_mix) {
                        free(mediainput->buffers->buf_mix);
                        mediainput->buffers->buf_mix = NULL;
                        mediainput->buffers->mix_buf_size = 0;
                    }
                    if (mediainput->buffers->buf_channel_convert) {
                        free(mediainput->buffers->buf_channel_convert);
                        mediainput->buffers->buf_channel_convert = NULL;
                    }
                    if (mediainput->buffers->buf_psd) {
                        free(mediainput->buffers->buf_psd);
                        mediainput->buffers->buf_psd = NULL;
                        mediainput->buffers->psd_buf_size = 0;
                    }
                    free(mediainput->buffers);
                    mediainput->buffers = NULL;
                }
                if (mediainput->front_end_processor) {
                    speex_preprocess_state_destroy(mediainput->front_end_processor);
                    mediainput->front_end_processor = NULL;
                }
                if (mediainput->back_end_processor) {
                    speex_preprocess_state_destroy(mediainput->back_end_processor);
                    mediainput->back_end_processor = NULL;
                }
                if (mediainput->front_end_resampler) {
                    speex_resampler_destroy(mediainput->front_end_resampler);
                    mediainput->front_end_resampler = NULL;
                }
                if (mediainput->back_end_resampler) {
                    speex_resampler_destroy(mediainput->back_end_resampler);
                    mediainput->back_end_resampler = NULL;
                }
                if (mediainput->echo_state) {
                    speex_echo_state_destroy(mediainput->echo_state);
                    mediainput->echo_state = NULL;
                }
                media_input_list_.erase(it_mediainput);
                delete mediainput;
                break;
            }
        }
    }
}
//return value: 0:non-speech 1:speech
int AudioPostProcessing::SpeechCheck(CheckContext *ctx, MediaBuf &buffer)
{
    int ret= -1;
    int vad_prob_start = VAD_PROB_START;
    int vad_prob_continue = VAD_PROB_CONT;
    unsigned char *audio_data = NULL;

    AudioPayload pIn;
    pIn.payload = buffer.payload;
    pIn.payload_length = buffer.payload_length;
    pIn.isFirstPacket = buffer.isFirstPacket;

    if (!ctx) {
        printf("APP:[%s] Invalid input param\n", __FUNCTION__);
        return ret;
    }

    if (!ctx->processor_state) {
        if (pIn.payload) {
            ctx->data_offset = ctx->wav_header.Interpret(pIn.payload, &(ctx->wav_info), pIn.payload_length);
            if (pIn.payload_length <= ctx->data_offset || ctx->data_offset <= 0) {
                printf("APP:[%s] Input payload size (%d) is lower than data dataOffset:%d\n",
                   __FUNCTION__, pIn.payload_length, ctx->data_offset);
                return ret;
            } else {
                ctx->frame_size = pIn.payload_length - ctx->data_offset;

                int vad = 1;
                ctx->processor_state = speex_preprocess_state_init(ctx->frame_size >> 1, ctx->wav_info.sample_rate);
                printf("APP:[%s] frame_size = %d, sample rate = %d\n",
                   __FUNCTION__, ctx->frame_size, ctx->wav_info.sample_rate);
                speex_preprocess_ctl(ctx->processor_state, SPEEX_PREPROCESS_SET_VAD, &vad);
                //set prob start and prob continue
                speex_preprocess_ctl(ctx->processor_state, SPEEX_PREPROCESS_SET_PROB_START, &vad_prob_start);
                speex_preprocess_ctl(ctx->processor_state, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &vad_prob_continue);
            }
        } else {
            printf("APP: [%s] input payload NULL.\n", __FUNCTION__);
            return ret;
        }
    }
    if (ctx->processor_state) {
        audio_data = pIn.payload + ctx->data_offset;
        if (ctx->processor_state) {
            ret = speex_preprocess_run(ctx->processor_state, (short *)(audio_data));
        }
    }

    return ret;
}

int AudioPostProcessing::HandleProcess()
{
    int ret = -1;

    if (app_parameter_.file_mode_app) {
        ret = FileModeHandleProcess();
    } else {
        ret = StreamingModeHandleProcess();
    }

    return ret;
}

int AudioPostProcessing::FileModeHandleProcess()
{
    APP_TRACE_INFO("APP: %s\n", __FUNCTION__);
    std::list<APPMediaInput *>::iterator it_mediainput;
    APPMediaInput *mediainput = NULL;
    MediaPad *pad = NULL;
    MediaBuf buffer;
    bool eof = false;
    bool empty;
    bool underflow;
    MediaBuf OutBuf[MAX_APP_INPUT];
    int i = 0;

    while (is_running_) {
        // Step 1: check the inputs
        empty = true;
        underflow = false;

        for (it_mediainput = media_input_list_.begin();
             it_mediainput != media_input_list_.end();
             it_mediainput++) {
            mediainput = *it_mediainput;
            // check if there is valid input data inside the APP.
            if (mediainput->pad->GetBufQueueSize() > 0) {
                empty = false;
                // update the media input status
                if (mediainput->status == INITIALIZED) {
                    mediainput->status = RUNNING;
                }
            }

            if ((mediainput->pad->GetBufQueueSize() < 1) && (mediainput->status == RUNNING)) {
                underflow = true;
            }
        }

        if ((underflow == true) || (empty == true)) {
            APP_TRACE_INFO("APP underflow %d , empty = %d\n", underflow, empty);
            usleep(10000);
            continue;
        }

        // Step 2: process the inputs.
        eof = ProcessInput();

        // Step 3: release the outdated buffer.
        for (it_mediainput = media_input_list_.begin(); it_mediainput != media_input_list_.end();
             it_mediainput++) {
                mediainput = *it_mediainput;
                pad = mediainput->pad;

                if (pad->GetBufQueueSize() > 0) {
                    pad->GetBufData(buffer);
                    pad->ReturnBufToPeerPad(buffer);
                }
        }
        // Step 4: all inputs get eof, finish processing.
        if (eof) {
            for (it_mediainput = media_input_list_.begin();
                 it_mediainput != media_input_list_.end();
                 it_mediainput++) {
                OutBuf[i++].payload = NULL;
            }
            ReleaseBuffer(OutBuf);
            break;
        }
    }

    is_running_ = 0;
    return 0;
}

int AudioPostProcessing::StreamingModeHandleProcess()
{
    APP_TRACE_INFO("APP: %s\n", __FUNCTION__);
    std::list<APPMediaInput *>::iterator it_mediainput;
    APPMediaInput *mediainput = NULL;
    MediaPad *pad = NULL;
    MediaBuf buffer;
    bool eof = false;
    bool empty;
    bool underflow;
    MediaBuf OutBuf[MAX_APP_INPUT];
    int i = 0;
    int input_num = 0;
    Measurement measure;
    unsigned long last_time = 0;
    unsigned long cur_time = 0;

    while (is_running_) {
        WaitSinkMutex();
        // Step 1: check the inputs
        empty = true;
        underflow = false;

        for (it_mediainput = media_input_list_.begin(), input_num = 0;
             it_mediainput != media_input_list_.end();
             it_mediainput++, input_num++) {
            mediainput = *it_mediainput;

            // check if there is valid input data inside the APP.
            if (mediainput->pad->GetBufQueueSize() > 0) {
                empty = false;
                // update the media input status
                if (mediainput->status == INITIALIZED) {
                    mediainput->status = RUNNING;
                }
            }

            // cache data for de-jitter buffer
            // only for the beginning of mix
            if ((mediainput->pad->GetBufQueueSize() < BUF_DEPTH_LOW) &&
                (mediainput->status == RUNNING)) {
                underflow = true;
            }
        }

        measure.GetCurTime(&cur_time);
        if (empty == true) {
            if ((cur_time - last_time) >= MIXER_TIMER_INT) {
                measure.GetCurTime(&last_time);
                // padding silence data for network jitter
                mediainput = *(media_input_list_.begin());
                if (mediainput->input_ctx->audio_frame_size_out != 0 && input_num != 0) {
                    printf("[%p] APP underflow %d , empty = %d\n", this, underflow, empty);
#ifdef PADDING_SILENCE_FRAME    //Don't enable this option for current status
                    // Build the silence payload
                    if (!buf_silence_) {
                        printf("[%p] APP building silence packet\n", this);
                        buf_silence_ = (unsigned char *)malloc(sizeof(unsigned char) *
                                         mediainput->input_ctx->audio_frame_size_out);
                        if (!buf_silence_) {
                            printf("APP: [Error]: Failed to allocate memory! size = %d\n",
                                    mediainput->input_ctx->audio_frame_size_out);
                            ReleaseSinkMutex();
                            continue;
                        }
                        memset(buf_silence_, 0, mediainput->input_ctx->audio_frame_size_out);
                        mediainput->input_ctx->wav_header_out.Populate(&(mediainput->input_ctx->wav_info_out),
                                                        mediainput->input_ctx->audio_frame_size_out);
                        memcpy(buf_silence_,
                                (unsigned char*) &(mediainput->input_ctx->wav_header_out),
                                mediainput->input_ctx->wav_header_out.GetHeaderSize());
                    }
                    OutBuf[0].payload = buf_silence_;
                    OutBuf[0].payload_length = mediainput->input_ctx->audio_frame_size_out;
                    ReleaseBuffer(OutBuf);
#endif
                }
            }
            ReleaseSinkMutex();
            // sleep for next round
            usleep(MIXER_SLEEP_INT);
            continue;
        } else {
            if (underflow == true) {
                if ((cur_time - last_time) < MIXER_TIMER_INT) {
                    ReleaseSinkMutex();
                    usleep(MIXER_SLEEP_INT);
                    continue;
                }
            }
        }

        // Step 2: process the inputs.
        eof = ProcessInput();
        measure.GetCurTime(&last_time);

        // Step 3: release the outdated buffer.
        for (it_mediainput = media_input_list_.begin();
             it_mediainput != media_input_list_.end();
             it_mediainput++) {
            mediainput = *it_mediainput;
            pad = mediainput->pad;

            if (pad->GetBufQueueSize() > 0) {
                pad->GetBufData(buffer);
                pad->ReturnBufToPeerPad(buffer);
                if (pad->GetBufQueueSize() == BUF_DEPTH_OVER) {
                    pad->GetBufData(buffer);
                    pad->ReturnBufToPeerPad(buffer);
                    printf("[%p]****Pad [%p] cache buffer overflow\n", this, pad);
#ifdef AUDIO_SPEECH_DETECTION
                } else if (pad->GetBufQueueSize() >= BUF_DEPTH_HIGH &&
                           pad->GetBufQueueSize() < BUF_DEPTH_OVER) {
                    pad->PickBufData(buffer);
                    if (!(SpeechCheck(mediainput->check_ctx, buffer))) { //non-speech
                        pad->GetBufData(buffer);
                        pad->ReturnBufToPeerPad(buffer);
                        printf("[%p]****Pad [%p] drop non-speech frame\n", this, pad);
                    }
#endif
                }
            }
        }
        // Step 4: all inputs get eof, finish processing.
        if (eof) {
            for (it_mediainput = media_input_list_.begin();
                 it_mediainput != media_input_list_.end();
                 it_mediainput++) {
                OutBuf[i++].payload = NULL;
            }
            ReleaseBuffer(OutBuf);
            ReleaseSinkMutex();
            break;
        }
        ReleaseSinkMutex();
    }

    is_running_ = 0;
    return 0;
}

int AudioPostProcessing::UpdateMediaInput(APPMediaInput *mediainput, MediaBuf &buf)
{
    if ((mediainput == host_input_) && (mediainput->status == STOP)) {
        std::list<APPMediaInput *>::iterator it_mediainput;
        for (it_mediainput = media_input_list_.begin();
             it_mediainput != media_input_list_.end();
             it_mediainput++) {
            mediainput = *it_mediainput;
            if (mediainput->status == RUNNING) {
                host_input_ = mediainput;
                APP_TRACE_INFO("APP: change the host input to %p\n", host_input_);
                break;
            }
        }
    }
    return 0;
}

