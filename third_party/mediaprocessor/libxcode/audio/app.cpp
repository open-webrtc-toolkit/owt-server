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
    }

    if (buf_silence_) {
        free(buf_silence_);
        buf_silence_ = NULL;
    }

    if (vad_sort_ctx_) {
        delete[] vad_sort_ctx_;
        vad_sort_ctx_ = NULL;
    }

    if (out_buf_) {
        delete[] out_buf_;
        out_buf_ = NULL;
    }
    // destroy the media input list
    for (it_mediainput = media_input_list_.begin(), i = 0;
         it_mediainput != media_input_list_.end();
         it_mediainput++, i++) {
        if (*it_mediainput != NULL) {
            input = *it_mediainput;
            ReleaseMediaInput(input);
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
    vad_sort_ctx_(NULL),
    out_buf_(NULL)
{
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
    if (NULL == cfg) {
        APP_TRACE_INFO("No config for APP, just pass through\n");
        return true;
    }

    APPFilterParameters *app_cfg = static_cast<APPFilterParameters *>(cfg);
    app_parameter_ = *app_cfg;

    return true;
}

int AudioPostProcessing::Recycle(MediaBuf &buf)
{
    return 0;
}

int AudioPostProcessing::FrontEndProcessRun(APPMediaInput *mediainput, AudioPayload *payload)
{
    APPInputContext *ctx = mediainput->input_ctx;
    unsigned char *audio_data = payload->payload + ctx->wav_data_offset;
    int ret = 0;
    int i = 0;
    int input_id = ctx->audio_input_id;

    if((ctx->audio_input_running_status == 0) ||
       (ctx->audio_frame_size_in <= 0) ||
       (ctx->audio_sample_rate_in <= 0)) {
        return 0;
    }

    if (!(mediainput->front_end_processor)) {
        // Init speex front-end process status
        printf("APP[%p]: %s[%d], frame_size_in = %d, sample_rate_in = %d\n",
               this,
               __FUNCTION__,
               input_id,
               ctx->audio_frame_size_in,
               ctx->audio_sample_rate_in);

        mediainput->front_end_processor = speex_preprocess_state_init(ctx->audio_frame_size_in >> 1,
                                                                      ctx->audio_sample_rate_in);
        if (app_parameter_.front_end_denoise_enable) {
            int noise_suppress_value = app_parameter_.front_end_denoise_level;

            if (noise_suppress_value > 100) {
                printf("APP Warning: front_end_denoise_level > 100!");
            }

            speex_preprocess_ctl(mediainput->front_end_processor,
                                 SPEEX_PREPROCESS_SET_DENOISE,
                                 &(app_parameter_.front_end_denoise_enable));

            speex_preprocess_ctl(mediainput->front_end_processor,
                                 SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,
                                 &(noise_suppress_value));
        }

        if (app_parameter_.agc_enable) {
            // Only support float type agc level currently.
            float agc_level = app_parameter_.agc_level_value * 327.68;

            speex_preprocess_ctl(mediainput->front_end_processor,
                                 SPEEX_PREPROCESS_SET_AGC,
                                 &(app_parameter_.agc_enable));

            speex_preprocess_ctl(mediainput->front_end_processor,
                                 SPEEX_PREPROCESS_SET_AGC_LEVEL,
                                 &(agc_level));
        }

        if (app_parameter_.vad_enable) {
            speex_preprocess_ctl(mediainput->front_end_processor,
                                 SPEEX_PREPROCESS_SET_VAD,
                                 &(app_parameter_.vad_enable));

            speex_preprocess_ctl(mediainput->front_end_processor,
                                 SPEEX_PREPROCESS_SET_PROB_START,
                                 &(app_parameter_.vad_prob_start_value));

            speex_preprocess_ctl(mediainput->front_end_processor,
                                 SPEEX_PREPROCESS_SET_PROB_CONTINUE,
                                 &(app_parameter_.vad_prob_cont_value));
        }
    }

    if (mediainput->front_end_processor) {
        ret = speex_preprocess_run(mediainput->front_end_processor, (short *)(audio_data));

        if (app_parameter_.vad_enable) {
            speex_preprocess_ctl(mediainput->front_end_processor,
                                 SPEEX_PREPROCESS_GET_PROB,
                                 &(vad_sort_ctx_[input_id].audio_speech_prob));
            ctx->audio_vad_prob = vad_sort_ctx_[input_id].audio_speech_prob;

            if (ret) {
                ctx->audio_input_active_status = 1;
                // Get power spectrum
                int psd_size = 0;
                float psd_avg = 0.0;
                speex_preprocess_ctl(mediainput->front_end_processor,
                                     SPEEX_PREPROCESS_GET_PSD_SIZE,
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
                int *psd_buf = mediainput->buffers->buf_psd;

                speex_preprocess_ctl(mediainput->front_end_processor,
                                     SPEEX_PREPROCESS_GET_PSD,
                                     mediainput->buffers->buf_psd);
                // Calculate average power spectrum
                for (i = 0; i < psd_size; i++) {
                    psd_avg += (float)(psd_buf[i]) / psd_size;
                }
#ifdef USD_PSD_MSD
                float psd_squared_sum = 0.0;
                float psd_deviation = 0.0;
                float psd_msd = 0.0;
                for (i = 0; i < psd_size; i++) {
                    psd_deviation = psd_buf[i] - psd_avg;
                    psd_squared_sum += psd_deviation * psd_deviation;
                }
                psd_msd = sqrt(psd_squared_sum / psd_size);
                vad_sort_ctx_[input_id].audio_power_spect = (int)psd_msd;
#else
                vad_sort_ctx_[input_id].audio_power_spect = (int)psd_avg;
#endif
            } else {
                ctx->audio_input_active_status = 0;
                vad_sort_ctx_[input_id].audio_power_spect = 0;
            }

            vad_sort_ctx_[input_id].audio_input_id = input_id;
            vad_sort_ctx_[input_id].media_input = mediainput;
        }
    }

    return ret;
}

int AudioPostProcessing::BackEndProcessRun(APPMediaInput *mediainput, AudioPayload *payload)
{
    APPInputContext *ctx = mediainput->input_ctx;
    unsigned char *audio_data = payload->payload + ctx->wav_data_offset;
    int ret = 0;

    if((ctx->audio_frame_size_mix <= 0) ||
       (ctx->audio_sample_rate_mix <= 0)) {
        return 0;
    }

    if (!(mediainput->back_end_processor)) {
        // Init speex back-end process status
        printf("APP[%p]: %s[%d]: frame_size_mix = %d, sample_rate_mix = %d\n",
               this,
               __FUNCTION__,
               ctx->audio_input_id,
               ctx->audio_frame_size_mix,
               ctx->audio_sample_rate_mix);

        mediainput->back_end_processor = speex_preprocess_state_init(ctx->audio_frame_size_mix >> 1,
                                                                     ctx->audio_sample_rate_mix);
        if (NULL == mediainput->back_end_processor) {
            printf("APP [%p]: Back-end processor init failed!\n", this);
            return -1;
        }
        if (app_parameter_.back_end_denoise_enable) {
            int noise_suppress_value = app_parameter_.back_end_denoise_level;
            if (noise_suppress_value > 100) {
                printf("APP Warning: back_end_denoise_level > 100!");
            }
            speex_preprocess_ctl(mediainput->back_end_processor,
                                 SPEEX_PREPROCESS_SET_DENOISE,
                                 &(app_parameter_.back_end_denoise_enable));

            speex_preprocess_ctl(mediainput->back_end_processor,
                                 SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,
                                 &(noise_suppress_value));
        }
        if (app_parameter_.aec_enable) {
            //Init speex preprocess status
            int echo_tail = 1 + (ctx->audio_frame_size_mix * app_parameter_.aec_tail_level);
            mediainput->echo_state = speex_echo_state_init(ctx->audio_frame_size_mix >> 1,
                                                           echo_tail);
            if (NULL == mediainput->echo_state) {
                printf("APP [%p]: Failed to initialize echo state!\n", this);
                return -2;
            } else {
                printf("APP [%p]: Echo state initialized successfully. tail = %d\n", this, echo_tail);
            }
            speex_echo_ctl(mediainput->echo_state,
                           SPEEX_ECHO_SET_SAMPLING_RATE,
                           &(ctx->audio_sample_rate_mix));
            speex_preprocess_ctl(mediainput->back_end_processor,
                                 SPEEX_PREPROCESS_SET_ECHO_STATE,
                                 mediainput->echo_state);
        }
    }

    if (mediainput->back_end_processor) {
        if (app_parameter_.aec_enable &&
            mediainput->echo_state &&
            (ctx->audio_input_running_status != 0)) {
            //echo cancellation process
            speex_echo_cancellation(mediainput->echo_state,
                                    (short *)(audio_data),
                                    ctx->audio_data_in,
                                    (short *)(audio_data));
        }
        ret = speex_preprocess_run(mediainput->back_end_processor, (short *)(audio_data));
    }

    return ret;
}

int AudioPostProcessing::FrontEndResample(APPMediaInput *mediainput, AudioPayload *payload)
{
    int ret = 0;
    unsigned int size_to_write, size_write;
    unsigned char *buf_dst_data = NULL;
    APPInputContext *ctx = mediainput->input_ctx;
    unsigned int size_to_read = (payload->payload_length - ctx->wav_data_offset) >> 1;
    unsigned char *audio_data_in = payload->payload + ctx->wav_data_offset;

    if((ctx->audio_input_running_status == 0) ||
       (ctx->audio_sample_rate_in <= 0) ||
       (app_parameter_.front_end_sample_rate <= 0)) {
        APP_TRACE_INFO("APP: %s[%d]: sample_rate_in(%d) -> front_end_sample_rate (%d), return.\n",
                       __FUNCTION__,
                       ctx->audio_input_id,
                       ctx->audio_sample_rate_in,
                       app_parameter_.front_end_sample_rate);
        return 0;
    }

    if (!(mediainput->front_end_resampler)) {
        printf("APP: %s[%d]: sampe_rate_in(%d) -> front_end_sample_rate(%d), frame_size_in = %d\n",
               __FUNCTION__,
               ctx->audio_input_id,
               ctx->audio_sample_rate_in,
               app_parameter_.front_end_sample_rate,
               ctx->audio_frame_size_in);

        // Init speex resampler status
        mediainput->front_end_resampler = speex_resampler_init(ctx->audio_channel_number_in,
                                                               ctx->audio_sample_rate_in,
                                                               app_parameter_.front_end_sample_rate,
                                                               app_parameter_.resample_quality_level,
                                                               &ret);

        if (!(mediainput->front_end_resampler)) {
            printf("APP: [Error]: Resampler Init failed ret = %d!\n", ret);
        }
    }

    if (mediainput->front_end_resampler) {
        if (app_parameter_.front_end_sample_rate <= ctx->audio_sample_rate_in) {
            size_to_write = size_to_read;
        } else {
            size_to_write = size_to_read *
                           (1 + app_parameter_.front_end_sample_rate / ctx->audio_sample_rate_in);
        }

        int size = (size_to_write << 1) + ctx->wav_data_offset;
        if (size <= 0) {
            printf("APP: [Error]: Resampler size = %d\n", size);
            return -1;
        }

        if (!(mediainput->buffers->buf_front_resample)) {
            mediainput->buffers->buf_front_resample = (short *)malloc(sizeof(short) * size);
            if (!(mediainput->buffers->buf_front_resample)) {
                printf("APP: [Error]: Failed to allocate memory! size_to_write = %d\n", size_to_write);
                return -1;
            }
        }
        buf_dst_data = (unsigned char *)(mediainput->buffers->buf_front_resample) +
                      ctx->wav_data_offset;

        size_write = size_to_write;
        APP_TRACE_INFO("APP: input %d, size_to_read = %d, size_to_write = %d\n",
                       ctx->audio_input_id,
                       size_to_read,
                       size_to_write);

        // re-sampling process
        ret = speex_resampler_process_int(mediainput->front_end_resampler,
                                          0,
                                          (short *)audio_data_in,
                                          &size_to_read,
                                          (short *)buf_dst_data,
                                          &size_write);
        if (!ret) {
            APP_TRACE_INFO("APP: [Info]: size_read = %d, size_write = %d, size_to_write = %d\n",
                            ctx->audio_frame_size_in,
                            size_write,
                            size_to_write);

#ifdef DUMP_FR_RESAMPLE_PCM
            if (ctx->audio_input_id == 0) {
                FILE *fp;
                fp = fopen("dump_fr_resample.pcm", "ab+");
                fwrite(buf_dst_data, sizeof(short), size_write, fp);
                fclose(fp);
            }
#endif

            ctx->wav_info_out.sample_rate = app_parameter_.front_end_sample_rate;
            ctx->wav_header_out.freq = app_parameter_.front_end_sample_rate;
            ctx->wav_header_out.Populate(&(ctx->wav_info_out), size_write << 1);
            memcpy(mediainput->buffers->buf_front_resample,
                   (unsigned char*) &(ctx->wav_header_out),
                   ctx->wav_header_out.GetHeaderSize());
            payload->payload_length = (size_write << 1) + ctx->wav_header_out.GetHeaderSize();
            payload->payload = (unsigned char*)(mediainput->buffers->buf_front_resample);

            ctx->audio_payload = *payload;
            ctx->audio_data_in = mediainput->buffers->buf_front_resample + (ctx->wav_data_offset >> 1);
            ctx->audio_payload_length = payload->payload_length;
            ctx->audio_frame_size_mix = size_write << 1;
            APP_TRACE_INFO("APP: Front-end Resample [%d], frame_size_mix = %d, sample_rate_mix = %d\n",
                           ctx->audio_input_id,
                           ctx->audio_frame_size_mix,
                           ctx->audio_sample_rate_mix);

        } else {
            APP_TRACE_INFO("APP: [Error]: speex_resampler_process_int() returns %d\n", ret);
        }
    }
    return ret;
}

int AudioPostProcessing::BackEndResample(APPMediaInput *mediainput, AudioPayload *payload)
{
    int ret = 0;
    unsigned int size_to_write, size_write;
    unsigned char *buf_dst_data = NULL;
    APPInputContext *ctx = mediainput->input_ctx;
    unsigned int size_to_read = (payload->payload_length - ctx->wav_data_offset) >> 1;
    unsigned char *audio_data_in = payload->payload + ctx->wav_data_offset;

    if ((ctx->audio_sample_rate_mix <= 0) ||
        (app_parameter_.back_end_sample_rate <= 0)) {
        printf("APP: %s[%d], sample_rate_mix(%d) -> back_end_sample_rate(%d), return\n",
                       __FUNCTION__,
                       ctx->audio_input_id,
                       ctx->audio_sample_rate_mix,
                       app_parameter_.back_end_sample_rate);
        return 0;
    }

    if (!(mediainput->back_end_resampler)) {
        printf("APP: %s[%d], sample_rate_mix(%d) -> back_end_sample_rate(%d), frame_size_mix = %d\n",
               __FUNCTION__,
               ctx->audio_input_id,
               ctx->audio_sample_rate_mix,
               app_parameter_.back_end_sample_rate,
               ctx->audio_frame_size_mix);

        // Init speex resampler status
        mediainput->back_end_resampler = speex_resampler_init(ctx->audio_channel_number,
                                                              ctx->audio_sample_rate_mix,
                                                              app_parameter_.back_end_sample_rate,
                                                              app_parameter_.resample_quality_level,
                                                              &ret);

        if (!(mediainput->back_end_resampler)) {
            printf("APP: [Error]: Resampler Init failed ret = %d!\n", ret);
        }
    }

    if (mediainput->back_end_resampler) {
        if (app_parameter_.back_end_sample_rate <= ctx->audio_sample_rate_mix) {
            size_to_write = size_to_read;
        } else {
            size_to_write = size_to_read *
                           (1 + app_parameter_.back_end_sample_rate / ctx->audio_sample_rate_mix);
        }

        int size = (size_to_write << 1) + ctx->wav_data_offset;
        if (size <= 0) {
            printf("APP: [Error]: Resampler size = %d\n", size);
            return -1;
        }

        if (!(mediainput->buffers->buf_back_resample)) {
            printf("APP: Allocate memory for %s, size = %d, read_size_16b = %d\n",
                   __FUNCTION__,
                   size,
                   size_to_read);
            mediainput->buffers->buf_back_resample = (short *)malloc(sizeof(short) * size);
            if (!(mediainput->buffers->buf_back_resample)) {
                printf("APP: [Error]: Failed to allocate memory! size_write = %d\n", size_to_write);
                return -1;
            }
        }
        buf_dst_data = (unsigned char *)(mediainput->buffers->buf_back_resample) +
                      ctx->wav_data_offset;

        size_write = size_to_write;
        APP_TRACE_INFO("APP: size_to_read = %d, size_to_write = %d\n", size_to_read, size_to_write);

        // re-sampling process
        ret = speex_resampler_process_int(mediainput->back_end_resampler,
                                              0,
                                              (short *)audio_data_in,
                                              &size_to_read,
                                              (short *)buf_dst_data,
                                              &size_write);

        if (!ret) {
            APP_TRACE_INFO("APP: [Info]: size_read = %d, size_write = %d, size_to_write = %d\n",
                           ctx->audio_frame_size_mix, size_write, size_to_write);

#ifdef DUMP_BK_RESAMPLE_PCM
            if (ctx->audio_input_id == 0) {
                FILE *fp;
                fp = fopen("dump_bk_resample.pcm", "ab+");
                fwrite(buf_dst_data, sizeof(short), size_write, fp);
                fclose(fp);
            }
#endif

            ctx->wav_info_out.sample_rate = app_parameter_.back_end_sample_rate;
            ctx->wav_header_out.freq = app_parameter_.back_end_sample_rate;
            ctx->wav_header_out.Populate(&(ctx->wav_info_out), size_write << 1);
            memcpy(mediainput->buffers->buf_back_resample,
                   (unsigned char*) &(ctx->wav_header_out),
                   ctx->wav_header_out.GetHeaderSize());
            payload->payload_length = (size_write << 1) +
                                      ctx->wav_header_out.GetHeaderSize();
            payload->payload = (unsigned char*)(mediainput->buffers->buf_back_resample);
            ctx->audio_frame_size_out = size_write << 1;
            APP_TRACE_INFO("APP: Back-end Resample: input %d, frame_size_out = %d, sample_rate_mix = %d\n",
                            ctx->audio_input_id,
                            ctx->audio_frame_size_out,
                            ctx->audio_sample_rate_mix);
        } else {
            APP_TRACE_INFO("APP: [Error]: speex_resampler_process_int() returns %d\n", ret);
        }
    }

    return ret;
}

int AudioPostProcessing::ChannelNumberConvert(APPMediaInput *mediainput, AudioPayload *payload)
{
    APPInputContext *ctx = mediainput->input_ctx;
    unsigned int write_size_16b = 0;
    unsigned char *audio_data_out = NULL;
    unsigned int read_size_16b = (payload->payload_length - ctx->wav_data_offset) >> 1;
    unsigned char *audio_data_in = payload->payload + ctx->wav_data_offset;

    if ((ctx->audio_input_running_status == 0) ||
        (ctx->audio_channel_number_in == app_parameter_.channel_number)) {
        return 0;
    }

    if (ctx->audio_channel_number_in == 1) {  //mono to stereo
        write_size_16b = read_size_16b << 1;
    } else if (ctx->audio_channel_number_in == 2) { //stereo to mono
        write_size_16b = read_size_16b >> 1;
    } else {
        printf("APP: Error: Only support conversion between mono and stereo!");
        return 0;
    }

    int convert_buf_size = (write_size_16b << 1) + ctx->wav_data_offset;
    // Allocate buffer for channel number conversion
    if (!(mediainput->buffers->buf_channel_convert)) {
        printf("APP: Allocate memory for %s[%d], conv_buf_size16b = %d, read_size_16b = %d\n",
               __FUNCTION__,
               ctx->audio_input_id,
               convert_buf_size, read_size_16b);
        mediainput->buffers->buf_channel_convert = (short *)malloc(sizeof(short) * convert_buf_size);
        if (!(mediainput->buffers->buf_channel_convert)) {
            printf("APP: [Error]: Failed to allocate memory! write_size_16b = %d\n", write_size_16b);
            return -1;
        }
    }
    audio_data_out = (unsigned char *)(mediainput->buffers->buf_channel_convert) + ctx->wav_data_offset;

    // Channel number conversion
    if (ctx->audio_channel_number_in == 1) {
        MonoToStereo((short *)audio_data_out, (short *)audio_data_in, read_size_16b);
    } else if (ctx->audio_channel_number_in == 2) {
        StereoToMono((short *)audio_data_out, (short *)audio_data_in, read_size_16b);
    }

#ifdef DUMP_CHANNEL_NUM_CONVERT_PCM
        if (ctx->audio_input_id == 0) {
            FILE *fp;
            fp = fopen("dump_chnum_convert.pcm", "ab+");
            fwrite(audio_data_out, sizeof(short), write_size_16b, fp);
            fclose(fp);
        }
#endif

    // Build the output payload packet
    APP_TRACE_DEBUG("read_size_16b = %d, write_size_16b = %d\n", read_size_16b, write_size_16b);
    ctx->wav_info_out.channels_number = app_parameter_.channel_number;
    ctx->wav_header_out.Populate(&(ctx->wav_info_out), write_size_16b << 1);
    memcpy(mediainput->buffers->buf_channel_convert,
           (unsigned char*) &(ctx->wav_header_out),
           ctx->wav_header_out.GetHeaderSize());
    payload->payload_length = (write_size_16b << 1) + ctx->wav_header_out.GetHeaderSize();
    payload->payload = (unsigned char*)(mediainput->buffers->buf_channel_convert);

    // Update APP context variable value.
    ctx->audio_payload = *payload;
    ctx->audio_data_in = mediainput->buffers->buf_channel_convert +
                                           (ctx->wav_data_offset >> 1);
    ctx->audio_payload_length = payload->payload_length;
    ctx->audio_channel_number = app_parameter_.channel_number;
    ctx->audio_frame_size_mix = write_size_16b << 1;
    APP_TRACE_DEBUG("APP: input %d, frame_size_mix = %d, channel_number[input_id] = %d\n",
                    ctx->audio_input_id,
                    ctx->audio_frame_size_mix,
                    ctx->audio_channel_number);

    return 0;
}

int AudioPostProcessing::FrontEndBitDepthConvert(APPMediaInput *mediainput, AudioPayload *payload)
{
    APPInputContext *ctx = mediainput->input_ctx;
    unsigned int i = 0;
    unsigned int write_size_16b = 0;
    unsigned char *audio_data_out8b = NULL;
    short *audio_data_out16b = NULL;
    unsigned int read_size_16b = (payload->payload_length - ctx->wav_data_offset) >> 1;
    unsigned char *audio_data_in8b = payload->payload + ctx->wav_data_offset;
    short *audio_data_in16b = (short *)audio_data_in8b;

    if ((ctx->audio_input_running_status == 0) ||
        (ctx->audio_bit_depth_in == 16)) {    // Only support 16-bit AudioMix currently
        return 0;
    }

    if (ctx->audio_bit_depth_in == 8) {    // 8bit to 16bit
        write_size_16b = read_size_16b << 1;
    } else if (ctx->audio_bit_depth_in == 16) {   // 16bit to 8bit
        write_size_16b = read_size_16b >> 1;
    } else {
        printf("APP: Error: Only support conversion between 8bit and 16bit! audio_bit_depth_in = %d\n",
               ctx->audio_bit_depth_in);
        return 0;
    }
    int convert_buf_size = (write_size_16b << 1) + ctx->wav_data_offset;

    // Allocate buffer for bit depth conversion
    if (!(mediainput->buffers->buf_bit_depth_conv_front)) {
        printf("APP: Allocate memory for %s, convert_buf_size = %d, read_size_16b = %d\n",
               __FUNCTION__,
               convert_buf_size,
               read_size_16b);
        mediainput->buffers->buf_bit_depth_conv_front = (short *)malloc(sizeof(short) * convert_buf_size);
        if (!(mediainput->buffers->buf_bit_depth_conv_front)) {
            printf("APP: [Error]: Failed to allocate memory! write_size_16b = %d\n", write_size_16b);
            return -1;
        }
    }
    audio_data_out8b = (unsigned char *)(mediainput->buffers->buf_bit_depth_conv_front) +
                       ctx->wav_data_offset;
    audio_data_out16b = (short *)audio_data_out8b;

    // bit depth conversion
    if (ctx->audio_bit_depth_in == 8) {    // 8bit to 16bit
        for (i = 0; i < write_size_16b; i++) {
            *audio_data_out16b = (*audio_data_in8b - 0x80) << 8;
            audio_data_out16b++;
            audio_data_in8b++;
        }
    } else if (ctx->audio_bit_depth_in == 16) {   // 16bit to 8bit
        for (i = 0; i < read_size_16b; i++) {
            *audio_data_out8b = (*audio_data_in16b >> 8) + 0x80;
            audio_data_out8b++;
            audio_data_in16b++;
        }
    }

    // Build the output payload packet
    ctx->wav_info_out.resolution = app_parameter_.bit_depth;
    ctx->wav_header_out.Populate(&(ctx->wav_info_out), write_size_16b << 1);
    memcpy(mediainput->buffers->buf_bit_depth_conv_front,
           (unsigned char*) &(ctx->wav_header_out),
           ctx->wav_header_out.GetHeaderSize());
    payload->payload_length = (write_size_16b << 1) + ctx->wav_header_out.GetHeaderSize();
    payload->payload = (unsigned char*)(mediainput->buffers->buf_bit_depth_conv_front);

    // Update APP context variable value.
    ctx->audio_payload = *payload;
    ctx->audio_data_in = mediainput->buffers->buf_bit_depth_conv_front +
                                           (ctx->wav_data_offset >> 1);
    ctx->audio_payload_length = payload->payload_length;
    ctx->audio_frame_size_in = write_size_16b << 1;

    return 0;
}

int AudioPostProcessing::BackEndBitDepthConvert(APPMediaInput *mediainput, AudioPayload *payload)
{
    APPInputContext *ctx = mediainput->input_ctx;
    unsigned int i = 0;
    unsigned int write_size_16b = 0;
    unsigned char *audio_data_out8b = NULL;
    short *audio_data_out16b = NULL;
    unsigned int read_size_16b = (payload->payload_length - ctx->wav_data_offset) >> 1;
    unsigned char *audio_data_in8b = payload->payload + ctx->wav_data_offset;
    short *audio_data_in16b = (short *)audio_data_in8b;

    if (ctx->audio_bit_depth_mix == app_parameter_.bit_depth) {
        return 0;
    }

    if (ctx->audio_bit_depth_mix == 8) {    // 8bit to 16bit
        write_size_16b = read_size_16b << 1;
    } else if (ctx->audio_bit_depth_mix == 16) {   // 16bit to 8bit
        write_size_16b = read_size_16b >> 1;
    } else {
        printf("APP(%s):Error: Only support conversion between 8bit and 16bit!\n", __FUNCTION__);
        return 0;
    }
    int convert_buf_size = (write_size_16b << 1) + ctx->wav_data_offset;

    // Allocate buffer for bit depth conversion
    if (!(mediainput->buffers->buf_bit_depth_conv_back)) {
        printf("APP: Allocate memory for %s[%d], convert_buf_size = %d, read_size_16b = %d\n",
               __FUNCTION__,
               ctx->audio_input_id,
               convert_buf_size,
               read_size_16b);
        mediainput->buffers->buf_bit_depth_conv_back = (short *)malloc(sizeof(short) * convert_buf_size);
        if (!(mediainput->buffers->buf_bit_depth_conv_back)) {
            printf("APP: [Error]: Failed to allocate memory! write_size_16b = %d\n", write_size_16b);
            return -1;
        }
    }
    audio_data_out8b = (unsigned char *)(mediainput->buffers->buf_bit_depth_conv_back) +
                       ctx->wav_data_offset;
    audio_data_out16b = (short *)audio_data_out8b;

    // Bit depth conversion
    if (ctx->audio_bit_depth_mix == 8) {    // 8bit to 16bit
        for (i = 0; i < write_size_16b; i++) {
            *audio_data_out16b = (*audio_data_in8b - 0x80) << 8;
            audio_data_out16b++;
            audio_data_in8b++;
        }
    } else if (ctx->audio_bit_depth_mix == 16) {    // 16bit to 8bit
        for (i = 0; i < read_size_16b; i++) {
            *audio_data_out8b = (*audio_data_in16b >> 8) + 0x80;
            audio_data_out8b++;
            audio_data_in16b++;
        }
    }

    // Build the output payload packet
    ctx->wav_info_out.resolution = app_parameter_.bit_depth;
    ctx->wav_header_out.bits = app_parameter_.bit_depth;
    ctx->wav_header_out.Populate(&(ctx->wav_info_out), write_size_16b << 1);
    memcpy(mediainput->buffers->buf_bit_depth_conv_back,
           (unsigned char*) &(ctx->wav_header_out),
           ctx->wav_header_out.GetHeaderSize());
    payload->payload_length = (write_size_16b << 1) + ctx->wav_header_out.GetHeaderSize();
    payload->payload = (unsigned char*)(mediainput->buffers->buf_bit_depth_conv_back);

    // Update APP context variable value.
    ctx->audio_frame_size_out = write_size_16b << 1;

    return 0;
}

void AudioPostProcessing::VADSort()
{
    qsort(vad_sort_ctx_, num_input_, sizeof(VADSortContext), ComparePowerSpect);
    return;
}

void* AudioPostProcessing::GetActiveInputHandle()
{
    if (!(app_parameter_.vad_enable)) {
        printf("APP[%p]: VAD option is not enabled!\n", this);
        return NULL;
    }

    std::list<APPMediaInput *>::iterator it_mediainput;
    APPMediaInput *mediainput = NULL;
    BaseElement *element_dec = NULL;

    mediainput = vad_sort_ctx_[0].media_input;
    if (!mediainput) {
        printf("APP: %s, mediainput (%p), return NULL!\n", __FUNCTION__, mediainput);
        return NULL;
    }
    element_dec = mediainput->pad->get_peer_pad()->get_parent();
    printf("APP: Active dec is %p, input_id = %d\n",
           element_dec, vad_sort_ctx_[0].audio_input_id);

    return element_dec;
}

int AudioPostProcessing::GetInputActiveStatus(void *input_handle)
{
    if (!(app_parameter_.vad_enable)) {
        printf("APP[%p]: VAD option is not enabled!\n", this);
        return -1;
    }

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
            APP_TRACE_DEBUG("APP: Found dec(%p), id = %d, active status = %d\n",
                            element_dec, id, status);
            break;
        }
    }

    return status;
}

int AudioPostProcessing::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    AudioPayload audio_payload;
    audio_payload.payload = buf.payload;
    audio_payload.payload_length = buf.payload_length;
    audio_payload.isFirstPacket = buf.isFirstPacket;

    APP_TRACE_INFO("APP will process the payload %p\n", &audio_payload);

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
            if (NULL == audio_payload.payload) {
                mediainput->status = STOP;
                UpdateMediaInput(mediainput, buf);
            }
            if (mediainput->pad->GetBufQueueSize() > 0) {
                // Update the media input status
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
            APP_TRACE_INFO("APP: host_input_->pad->GetBufQueueSize() = %d\n",
                           host_input_->pad->GetBufQueueSize());
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
    APP_TRACE_INFO("APP[%p]: total_frame_count_ = %lld\n", this, total_frame_count_++);
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
            APP_TRACE_INFO("APP mediainput->status = %d, input_id = %d\n",
                           mediainput->status, input_id);
            continue;
        }
        APP_TRACE_INFO("APP: pad %p->GetBufQueueSize() = %d\n",
                       mediainput->pad, mediainput->pad->GetBufQueueSize());

        // Pick the buffer from the queue.
        pad = mediainput->pad;
        ret = pad->PickBufData(buffer);
        if (ret != 0) {
            APP_TRACE_INFO("APP: Input %d PickBufData Failed\n", input_id);
            continue;
        }

        AudioPayload audio_payload;
        audio_payload.payload = buffer.payload;
        audio_payload.payload_length = buffer.payload_length;
        audio_payload.isFirstPacket = buffer.isFirstPacket;
        APP_TRACE_DEBUG("APP PickBufData: payload %p, mediainput %p, pad %p, num_running_input_ = %d\n",
                    audio_payload.payload, mediainput, pad, num_running_input_);

        if (NULL == audio_payload.payload) {
            printf("APP: Audio payload is NULL. Input %d get EOF!\n", input_id);
            mediainput->status = STOP;
        } else {
            eof = false;
            // Retrieve the raw audio parameters
            APP_TRACE_INFO("input_id = %d, audio_payload.payload = %p\n",
                           input_id, audio_payload.payload);

            mediainput->input_ctx->wav_data_offset = mediainput->input_ctx->wav_header_in.Interpret(
                                                         audio_payload.payload,
                                                         &(mediainput->input_ctx->wav_info_in),
                                                         audio_payload.payload_length);
            if ((audio_payload.payload_length <= mediainput->input_ctx->wav_data_offset) ||
                (mediainput->input_ctx->wav_data_offset <= 0)) {
                printf("APP: %s: input payload size (%d) is lower than dataOffset[%d](%d)\n",
                       __FUNCTION__,
                       audio_payload.payload_length,
                       input_id,
                       mediainput->input_ctx->wav_data_offset);

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

    if ((!vad_sort_ctx_) && app_parameter_.vad_enable) {
        // Prepare vad_sort_ctx_ for VAD feature
        vad_sort_ctx_ = new VADSortContext[num_input_];
        if (!vad_sort_ctx_) {
            printf("APP[%p]: Fail to allocate vad_sort_ctx_!\n", this);

            if (pthread_mutex_unlock(&mutex_) != 0) {
                printf("APP: pthread_mutex_unlock error!\n");
            }

            return true;
        }
        memset(vad_sort_ctx_, 0, num_input_ * sizeof(VADSortContext));
        for (int i = 0; i < num_input_; i++) {
            vad_sort_ctx_[i].audio_input_id = i;
        }
    }

    ComposeAudioFrame();

    if (pthread_mutex_unlock(&mutex_) != 0) {
        printf("APP: pthread_mutex_unlock error!\n");
    }
    return false;
}

int AudioPostProcessing::ComposeAudioFrame()
{
    int ret = 0;
    int i = 0;
    int num_output = num_input_;
    APPMediaInput *mediainput = NULL;
    std::list<APPMediaInput *>::iterator it_mediainput;
    APPInputContext *ctx = NULL;

    if (!out_buf_) {
        out_buf_ = new MediaBuf[num_input_];
        if (!out_buf_) {
            printf("APP[%p]: Error: Fail to allocate out_buf_\n", this);
            return -1;
        }
    }
    // Front-end process, including de-noise, VAD, AGC.
    for (it_mediainput = media_input_list_.begin();
         it_mediainput != media_input_list_.end();
         it_mediainput++, i++) {
        mediainput = *it_mediainput;
        ctx = mediainput->input_ctx;

        // Automatically enable bit depth conversion for 8-bit input case
        if (ctx->audio_bit_depth_in != ctx->audio_bit_depth_mix) {
             FrontEndBitDepthConvert(mediainput, &(ctx->audio_payload));
        }
        if (app_parameter_.vad_enable ||
            app_parameter_.agc_enable ||
            app_parameter_.front_end_denoise_enable) {
             ret = FrontEndProcessRun(mediainput, &(ctx->audio_payload));
        }

        // Front-end re-sampling
        if (app_parameter_.front_end_resample_enable) {
            ret = FrontEndResample(mediainput, &(ctx->audio_payload));
        }

        // Channel number conversion
        if (app_parameter_.channel_number_convert_enable) {
            ChannelNumberConvert(mediainput, &(ctx->audio_payload));
        }

        out_buf_[i].payload = ctx->audio_payload.payload;
        out_buf_[i].payload_length = ctx->audio_payload.payload_length;
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
        ctx = mediainput->input_ctx;
        // Process audio mixing
        ret = AudioMix(mediainput, out_buf_[i], i);
        if (ret) {
            out_buf_[i].payload_length = 0;
            out_buf_[i].payload = NULL;
            APP_TRACE_INFO("APP: %s AudioMix ret %d, num_input_ = %d, num_output = %d\n",
                           __FUNCTION__,
                           ret,
                           num_input_,
                           num_output);
            continue;
        }

        AudioPayload pOut;
        pOut.payload = out_buf_[i].payload;
        pOut.payload_length = out_buf_[i].payload_length;

        // Back-end process, including de-noise and AEC.
        if (app_parameter_.back_end_denoise_enable ||
            app_parameter_.aec_enable) {
             BackEndProcessRun(mediainput, &pOut);
        }

        // Back-end re-sampling
        if (app_parameter_.back_end_resample_enable) {
            ret = BackEndResample(mediainput, &pOut);
        }

        // Back-end bit depth conversion
        if (app_parameter_.bit_depth_convert_enable) {
             BackEndBitDepthConvert(mediainput, &pOut);
        }

        // Prepare the output buffer
        out_buf_[i].isFirstPacket = mediainput->first_packet;
#ifdef DUMP_APP_OUT_PCM
        if (!(mediainput->dump_file)) {
            char file_name[FILENAME_MAX] = {0};
            sprintf(file_name, "dump_app_out%d_%p.wav", i, this);
            mediainput->dump_file = fopen(file_name, "wb+");
            printf("APP[%p]: Dump APP output to %s, file_dump_app[%d] = %p\n",
                   this,
                   file_name,
                   i,
                   mediainput->dump_file);
        }
        if (mediainput->dump_file) {
            if (mediainput->first_packet) {
                ctx->wav_header_out.Populate(&(ctx->wav_info_out), 0xFFFFFFFF);
                fwrite(&(ctx->wav_header_out),
                       sizeof(unsigned char),
                       ctx->wav_header_out.GetHeaderSize(),
                       mediainput->dump_file);
            }
            fwrite(pOut.payload + ctx->wav_data_offset,
                       sizeof(unsigned char),
                       ctx->audio_frame_size_in,
                       mediainput->dump_file);
        }
#endif
        if (mediainput->first_packet) {
            mediainput->first_packet = false;
        }
        out_buf_[i].payload = pOut.payload;
        out_buf_[i].payload_length = pOut.payload_length;

        APP_TRACE_DEBUG("APP: push the payload %p to next element, length = %d\n",
                        pOut.payload,
                        pOut.payload_length);
    }

    for (it_mediainput = media_input_list_.begin(), i = 0;
         it_mediainput != media_input_list_.end();
         it_mediainput++, i++) {
        mediainput = *it_mediainput;
        ctx = mediainput->input_ctx;
        ctx->audio_input_running_status = 0;
        if (app_parameter_.vad_enable) {
            ctx->audio_vad_prob = 0;
            vad_sort_ctx_[i].audio_power_spect = 0;
            vad_sort_ctx_[i].audio_speech_prob = 0;
        }
    }

    ReleaseBuffer(out_buf_);

    return 0;
}

int AudioPostProcessing::PrepareParameter(APPMediaInput *mediainput, MediaBuf &buffer, unsigned int input_id)
{
    AudioPayload audio_payload;
    audio_payload.payload = buffer.payload;
    audio_payload.payload_length = buffer.payload_length;
    audio_payload.isFirstPacket = buffer.isFirstPacket;
    APPInputContext *ctx = mediainput->input_ctx;

    ctx->audio_payload = audio_payload;
    ctx->audio_data_in = (short *)(audio_payload.payload + ctx->wav_data_offset);
    ctx->audio_payload_length = audio_payload.payload_length;
    ctx->audio_frame_size_in = audio_payload.payload_length - ctx->wav_data_offset;
    ctx->audio_input_id = input_id;
    ctx->audio_input_running_status = 1;

    // Init input and mixed related parameters for each channel
    if (!(ctx->audio_param_inited)) {
        ctx->wav_data_offset = ctx->wav_header_in.Interpret(audio_payload.payload,
                                                            &(ctx->wav_info_in),
                                                            audio_payload.payload_length);
        ctx->audio_sample_rate_in = ctx->wav_info_in.sample_rate;
        ctx->audio_channel_number_in = ctx->wav_info_in.channels_number;
        ctx->audio_bit_depth_in = ctx->wav_info_in.resolution;
        ctx->audio_bit_depth_mix = 16;  // Only support 16-bit AudioMix currently

        if (app_parameter_.front_end_sample_rate == 0) {
            app_parameter_.front_end_sample_rate = ctx->audio_sample_rate_in;
        }
        // Automatically enable re-sample for different sample rate inputs case
        if (ctx->audio_sample_rate_in != app_parameter_.front_end_sample_rate) {
            if (!app_parameter_.front_end_resample_enable) {
                app_parameter_.front_end_resample_enable = true;
            }
            printf("APP: [%d]: Enable front-end re-sample to support inputs with different sample rate!\n",\
                   input_id);
        }
        ctx->audio_sample_rate_mix = app_parameter_.front_end_sample_rate;

        if (app_parameter_.channel_number == 0) {
            app_parameter_.channel_number = ctx->audio_channel_number_in;
        }
        // Automatically enable channel num conversion for different channel num inputs case
        if (ctx->audio_channel_number_in != app_parameter_.channel_number) {
            if (!app_parameter_.channel_number_convert_enable) {
                app_parameter_.channel_number_convert_enable = true;
            }
            printf("APP: [%d]: Enable channel num conversion to support inputs with different channel num!\n",
                   input_id);
        }
        ctx->audio_channel_number = app_parameter_.channel_number;

        // Calculate the mixed frame size according to the audio mixed parameters. (60ms as one frame)
        // Note: The calculation of frame size should be sychronous with PCM reader or decoder.
        ctx->audio_frame_size_mix = ctx->audio_channel_number *
                                    ctx->audio_sample_rate_mix * 3 *
                                    (ctx->audio_bit_depth_mix >> 3) / 50;

        // Calculate the mixed buffer size
        mediainput->buffers->mix_buf_size = ctx->audio_frame_size_mix
                                            + ctx->wav_data_offset;

        ctx->audio_param_inited = true;
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
            input->input_ctx->wav_data_offset = input->input_ctx->wav_header_in.Interpret(
                                                       audio_payload.payload,
                                                       &(input->input_ctx->wav_info_in),
                                                       audio_payload.payload_length);
            input->input_ctx->wav_header_out.Interpret(audio_payload.payload,
                                                       &(input->input_ctx->wav_info_out),
                                                       audio_payload.payload_length);
            input->input_ctx->audio_sample_rate_mix = ctx->audio_sample_rate_mix;
            input->input_ctx->audio_frame_size_mix = ctx->audio_frame_size_mix;
            input->input_ctx->audio_channel_number = ctx->audio_channel_number;
            input->input_ctx->audio_bit_depth_mix = ctx->audio_bit_depth_mix;
            input->input_ctx->wav_info_out.channels_number = ctx->audio_channel_number;
            input->input_ctx->wav_info_out.resolution = ctx->audio_bit_depth_mix;
            input->buffers->mix_buf_size = mediainput->buffers->mix_buf_size;
        }
        out_param_inited_ = true;
    }

    return 0;
}

int AudioPostProcessing::AudioMix(APPMediaInput *mediainput, MediaBuf &media_buf, int input_id)
{
    int i, k, input_m, size_read;
    short *audio_data_in, *audio_data_out;
    APPMediaInput *mediainput_m = NULL;
    std::list<APPMediaInput *>::iterator it_mediainput;
    APPMediaInput *input = NULL;
    APPInputContext *ctx = NULL;

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
                   mediainput->buffers->mix_buf_size,
                   mediainput_m->input_ctx->audio_payload_length);
            free(mediainput->buffers->buf_mix);
            mediainput->buffers->buf_mix = NULL;
        }
        mediainput->buffers->mix_buf_size = mediainput_m->input_ctx->audio_payload_length;
    }
    // Allocate mixer buffer
    if (!(mediainput->buffers->buf_mix)) {
        if (mediainput->buffers->mix_buf_size > 0) {
            mediainput->buffers->buf_mix = (short *)malloc(sizeof(short) *
                                           (mediainput->buffers->mix_buf_size >> 1));
            if (!(mediainput->buffers->buf_mix)) {
                printf("APP: [Error]: Failed to allocate memory! size = %d\n",
                       mediainput->buffers->mix_buf_size >> 1);
                return -2;
            }
        } else {
            printf("APP: Invalid size = %d\n", mediainput->buffers->mix_buf_size >> 1);
            return -2;
        }
    }

    short *buf_dst_data = mediainput->buffers->buf_mix +
                         (mediainput_m->input_ctx->wav_data_offset >> 1);
    memset(mediainput->buffers->buf_mix,
           0,
           mediainput->buffers->mix_buf_size >> 1);
    memcpy(mediainput->buffers->buf_mix,
           mediainput_m->input_ctx->audio_payload.payload,
           mediainput_m->input_ctx->audio_payload_length);
    media_buf.payload = (unsigned char*)(mediainput->buffers->buf_mix);
    audio_data_out = buf_dst_data;

    // No need to mix for only one input case.
    if (num_input_ < 2 || num_running_input_ < 2) {
        media_buf.payload_length = mediainput_m->input_ctx->audio_payload_length;
        return 0;
    }

    size_read = mediainput_m->input_ctx->audio_frame_size_mix;

    for (it_mediainput = media_input_list_.begin(), i = 0;
         it_mediainput != media_input_list_.end();
         it_mediainput++, i++) {
        input = *it_mediainput;
        ctx = input->input_ctx;
        if ((i != input_m) && (ctx->audio_input_running_status == 1)) {
            // If N:N mixer is enabled, exclude the input itself. Only mix other inputs.
            if (app_parameter_.nn_mixing_enable && (i == input_id)) {
                continue;
            }
            // If VAD is enabled, exclude the input without active voice.
            // Only mix inputs with active voice.
            if (app_parameter_.vad_enable && (ctx->audio_input_active_status == 0)) {
                continue;
            }

            audio_data_in = ctx->audio_data_in;

            // If mixer buffer size is smaller than payload length of current inputs,
            // report error message and reduce frame size to avoid overstepping the bundary.
            if (mediainput->buffers->mix_buf_size < ctx->audio_payload_length) {
                APP_TRACE_ERROR("APP: [Error]: MixBufSize(%d) < payload_length(%d)\n",
                                mediainput->buffers->mix_buf_size,
                                ctx->audio_payload_length);
                ctx->audio_frame_size_mix = mediainput->buffers->mix_buf_size -
                                            ctx->wav_data_offset;
            }

            if (size_read < ctx->audio_frame_size_mix) {
                size_read = ctx->audio_frame_size_mix;
            }

            for (k = 0; k < (size_read >> 1); k++) {
                // only support 16bit case currently
                audio_data_out[k] = MIX2(audio_data_out[k], audio_data_in[k], 15);
            }
        }
    }

    media_buf.payload_length = size_read + mediainput_m->input_ctx->wav_data_offset;

    return 0;
}

void AudioPostProcessing::ReleaseBuffer(MediaBuf *out)
{
    WaitSrcMutex();
    std::list<MediaPad *>::iterator srcpad;
    int i = 0;
    for (srcpad = (srcpads_.begin());
         srcpad != (srcpads_.end());
         srcpad++, i++) {
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
            printf("ERROR: fail to malloc CheckContext\n");
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
#ifdef DUMP_APP_OUT_PCM
        mediainput->dump_file = NULL;
#endif
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
                ReleaseMediaInput(mediainput);
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

    AudioPayload audio_payload;
    audio_payload.payload = buffer.payload;
    audio_payload.payload_length = buffer.payload_length;
    audio_payload.isFirstPacket = buffer.isFirstPacket;

    if (!ctx) {
        printf("APP:[%s] Invalid input param\n", __FUNCTION__);
        return ret;
    }

    if (!ctx->processor_state) {
        if (audio_payload.payload) {
            ctx->data_offset = ctx->wav_header.Interpret(audio_payload.payload,
                                                         &(ctx->wav_info),
                                                         audio_payload.payload_length);
            if (audio_payload.payload_length <= ctx->data_offset || ctx->data_offset <= 0) {
                printf("APP:[%s] Input payload size (%d) is lower than dataOffset:%d\n",
                       __FUNCTION__, audio_payload.payload_length, ctx->data_offset);
                return ret;
            } else {
                ctx->frame_size = audio_payload.payload_length - ctx->data_offset;

                int vad = 1;
                ctx->processor_state = speex_preprocess_state_init(ctx->frame_size >> 1,
                                                                   ctx->wav_info.sample_rate);
                printf("APP:[%s] frame_size = %d, sample rate = %d\n",
                       __FUNCTION__, ctx->frame_size, ctx->wav_info.sample_rate);
                speex_preprocess_ctl(ctx->processor_state,
                                     SPEEX_PREPROCESS_SET_VAD,
                                     &vad);
                //set prob start and prob continue
                speex_preprocess_ctl(ctx->processor_state,
                                     SPEEX_PREPROCESS_SET_PROB_START,
                                     &vad_prob_start);
                speex_preprocess_ctl(ctx->processor_state,
                                     SPEEX_PREPROCESS_SET_PROB_CONTINUE,
                                     &vad_prob_continue);
            }
        } else {
            printf("APP: [%s] input payload NULL.\n", __FUNCTION__);
            return ret;
        }
    }
    if (ctx->processor_state) {
        audio_data = audio_payload.payload + ctx->data_offset;
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

            if ((mediainput->pad->GetBufQueueSize() < 1) &&
                (mediainput->status == RUNNING)) {
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
        for (it_mediainput = media_input_list_.begin();
             it_mediainput != media_input_list_.end();
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
                out_buf_[i++].payload = NULL;
            }
            ReleaseBuffer(out_buf_);
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
                    out_buf_[0].payload = buf_silence_;
                    out_buf_[0].payload_length = mediainput->input_ctx->audio_frame_size_out;
                    ReleaseBuffer(out_buf_);
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
                out_buf_[i++].payload = NULL;
            }
            ReleaseBuffer(out_buf_);
            ReleaseSinkMutex();
            break;
        }
        ReleaseSinkMutex();
    }

    is_running_ = 0;
    return 0;
}

void AudioPostProcessing::ReleaseMediaInput(APPMediaInput *mediainput)
{
#ifdef DUMP_APP_OUT_PCM
    if (mediainput->dump_file) {
        fseek(mediainput->dump_file, 0L, SEEK_END);
        int file_length = ftell(mediainput->dump_file);
        fseek(mediainput->dump_file, 0, SEEK_SET);
        mediainput->input_ctx->wav_header_out.Populate(&(mediainput->input_ctx->wav_info_out),
                                                       file_length);
        fwrite(&(mediainput->input_ctx->wav_header_out),
               sizeof(unsigned char),
               mediainput->input_ctx->wav_header_out.GetHeaderSize(),
               mediainput->dump_file);
        fclose(mediainput->dump_file);
        mediainput->dump_file = NULL;
    }
#endif
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
        if (mediainput->buffers->buf_bit_depth_conv_front) {
            free(mediainput->buffers->buf_bit_depth_conv_front);
            mediainput->buffers->buf_bit_depth_conv_front = NULL;
        }
        if (mediainput->buffers->buf_bit_depth_conv_back) {
            free(mediainput->buffers->buf_bit_depth_conv_back);
            mediainput->buffers->buf_bit_depth_conv_back = NULL;
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

