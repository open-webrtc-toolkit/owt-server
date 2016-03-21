/* <COPYRIGHT_TAG> */

#ifndef AUDIO_TRANSCODER_H_
#define AUDIO_TRANSCODER_H_

#include <pthread.h>
#include "base/measurement.h"
#include "base/media_common.h"
#include "base/mem_pool.h"
#include "base/logger.h"
#include "base/stream.h"
#include "audio_params.h"
#include "wave_header.h"


class BaseElement;

/**
 * \brief AudioTranscoder class.
 * \details Manages decoder and encoder objects.
 */
class AudioTranscoder
{
public:
    DECLARE_MLOGINSTANCE();
    AudioTranscoder();
    virtual ~AudioTranscoder();

    /**
     * \brief Open file input.
     * \param filename input file name.
     */
    MemPool &OpenInput(const char *filename);

    MemPool &OpenMemPool();
    /**
     * \brief Open file output.
     * \param filename output file name.
     * \returns ERR_NONE if successful
     */
    Stream &OpenOutput(const char *filename);

    /**
     * \brief Open non-file output.
     * \returns a reference to the opened stream.
     */
    Stream &OpenOutput(string name);

    /**
     * \brief Initialise transcoder object.
     * \param dec_cfg optional pointer that will pe passed to Decoder
     * initialisation function. Can contain decoder configuration.
     * \param enc_cfg optional pointer that will pe passed to Encoder
     *  initialisation function. Can contain encoder configuration.
     * \param vppCfg optional pointer that will pe passed to VPP
     *  initialisation function. Can contain vpp configuration.
     */
    void* Init(void *dec_cfg, void *app_cfg, void *enc_cfg);

    /**
     * \brief attach input function
     * returns decoder handle
     */
    void* AttachInputStream(void *dec_cfg);

    /**
     * \brief detach input function
     */
    errors_t DetachInputStream(void *input_handle);

    /**
     * \brief attach output function
     * returns encoder handle
     */
    void* AttachOutputStream(void *enc_cfg);

    /**
     * \brief detach output function
     */
    errors_t DetachOutputStream(void *output_handle);

    void* GetActiveInputHandle();

    int GetInputActiveStatus(void *input_handle);

    /**
     * \brief Cleanup function, it will be called automatically by the destructor
     * or can be called manually.
     */
    bool Cleanup();

    /**
     * \brief Starts processing.
     */
    bool Start();

    bool Stop();

    bool Join();
    /**
     * \brief Returns true if either encoder or decoder are active.
     */
    bool IsAlive();

    /**
     * \brief Returns true if got xcoder mutex.
     */
    bool WaitMutex();
    /**
     * \brief Returns true if released xcoder mutex.
     */
    bool ReleaseMutex();

    StreamType input_codec_type_;
    StreamType output_codec_type_;

    const char *Input_Name;
private:
    AudioTranscoder(const AudioTranscoder &trans);

    AudioTranscoder& operator = (const AudioTranscoder&) {
        return *this;
    }

    std::list<Stream *>      input_stream_list_;
    std::list<MemPool *>     mem_pool_list_;
    std::list<BaseElement *> decoder_list_;
    std::list<Stream *>      output_stream_list_;
    std::list<BaseElement *> encoder_list_;
    BaseElement              *element_app_;
    BaseElement              *element_dec_;
    pthread_mutex_t          xcoder_mutex_;  /**< \brief API entry mutex*/
    bool                     is_running_;    /**< \brief xcoder running status*/
    bool                     is_from_file_;
    bool                     done_init_;
};

#endif /* AUDIO_TRANSCODER_H_ */
