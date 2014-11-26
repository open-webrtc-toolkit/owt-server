/* <COPYRIGHT_TAG> */

/**
 *\file AudioTranscoder.cpp
 *\brief Implementation for AudioTranscoder class
 */

#include <assert.h>
#include <unistd.h>
#include "app.h"
#include "audio_pcm_reader.h"
#include "audio_pcm_writer.h"

#ifdef ENABLE_AUDIO_CODEC
#include "audio_decoder.h"
#include "audio_encoder.h"
#endif

#include "audio_transcoder.h"
#include "base/trace.h"

typedef std::list<sAudioParams *> LIST_AUDDECOPT;

AudioTranscoder::AudioTranscoder():
    is_running_(false),
    is_from_file_(false)
{
    mAPP_ = NULL;
    done_init_ = false;

    pthread_mutex_init(&xcoder_mutex_, NULL);
}

AudioTranscoder::~AudioTranscoder()
{
    Cleanup();
    if (pthread_mutex_destroy(&xcoder_mutex_)) {
        printf("Error in pthread_mutex_destroy.\n");
        assert(0);
    }
}

MemPool &AudioTranscoder::OpenMemPool()
{
    WaitMutex();
    MemPool *mempool = new MemPool();
    mempool->init();
    mMemPool_list_.push_back(mempool);
    ReleaseMutex();
    return *mempool;
}

MemPool &AudioTranscoder::OpenInput(const char *filename)
{
    WaitMutex();
    Stream *infile = new Stream;
    bool status = false;

    if (infile) {
        status = infile->Open(filename);
    }

    if (!status) {
        if (infile) {
            delete infile;
        }

        printf("Failed to open input file %s\n", filename);
        ReleaseMutex();
        return *((MemPool*)NULL);
    }

    MemPool *input_mempool = new MemPool(filename);

    if (MEM_POOL_RETVAL_SUCCESS != input_mempool->init()) {
        delete input_mempool;
        input_mempool = NULL;
        delete infile;
        ReleaseMutex();
        return *((MemPool*)NULL);
    }

    mMemPool_list_.push_back(input_mempool);

    mInputStream_list_.push_back(infile);
    is_from_file_ = true;
    ReleaseMutex();
    return *input_mempool;
}

Stream &AudioTranscoder::OpenOutput(const char *filename)
{
    WaitMutex();
    Stream *outfile = new Stream;
    bool status = false;

    if (outfile) {
        status = outfile->Open(filename, true);
    }

    if (!status) {
        if (outfile) {
            delete outfile;
        }

        printf("Failed to open output file %s\n", filename);
        ReleaseMutex();
        return *((Stream*)NULL);
    }

    mOutputStream_list_.push_back(outfile);
    ReleaseMutex();
    return (*outfile);
}

Stream &AudioTranscoder::OpenOutput(string name)
{
    WaitMutex();
    Stream *output = new Stream;
    bool status = false;

    if (output) {
        status = output->Open(name);
    }

    if (!status) {
        if (output) {
            delete output;
        }

        printf("Failed to open output file\n");
        ReleaseMutex();
        return *((Stream*)NULL);
    }

    mOutputStream_list_.push_back(output);
    ReleaseMutex();
    return (*output);
}

void* AudioTranscoder::AttachInputStream(void *decCfg)
{
    BaseElement *element_dec = NULL;
    sAudioParams *audio_params = reinterpret_cast<sAudioParams *> (decCfg);
    printf("Attach input stream in AudioTrancoder %p\n", this);
    WaitMutex();
    //TODO prevent call this func before Init()

    if (!done_init_) {
        ReleaseMutex();
        printf("Attach new audio input before initialization\n");
        return NULL;
    }
    if (audio_params->nCodecType == STREAM_TYPE_AUDIO_PCM) {
        element_dec = new AudioPCMReader(audio_params->input_stream, NULL);
    } else {
#ifdef ENABLE_AUDIO_CODEC
        element_dec = new AudioDecoder(audio_params->input_stream, NULL);
#endif
    }

    if (!element_dec) {
        ReleaseMutex();
        printf("Failed to new Decoder\n");
        return NULL;
    }

    if (!element_dec->Init(audio_params, ELEMENT_MODE_ACTIVE)) {
        delete element_dec;
        element_dec = NULL;
        ReleaseMutex();
        printf("Failed to initialize Decoder\n");
        return NULL;
    }

    mDecoder_list_.push_back(element_dec);
    element_dec->LinkNextElement(mAPP_);
    if (is_running_) {
        element_dec->Start();
    }
    printf("Link decoder %p to APP %p...dec list size %d\n", element_dec, mAPP_, (int)mDecoder_list_.size());
    ReleaseMutex();
    return element_dec;
}

errors_t AudioTranscoder::DetachInputStream(void *input_handle)
{
    bool dec_found = false;
    if (NULL == input_handle) {
        return ERR_INVALID_INPUT_PARAMETER;
    }

    WaitMutex();
    if (!done_init_) {
        ReleaseMutex();
        printf("Detach audio input before initialization\n");
        return ERR_INVALID_INPUT_PARAMETER;
    }

    BaseElement *dec = static_cast<BaseElement*>(input_handle);
    for(std::list<BaseElement*>::iterator it_dec = mDecoder_list_.begin();
            it_dec != mDecoder_list_.end();
            ++it_dec) {
        if (dec == *it_dec) {
            mDecoder_list_.erase(it_dec);
            dec_found = true;
            break;
        }
    }

    if (!dec_found) {
        ReleaseMutex();
        return ERR_INVALID_INPUT_PARAMETER;
    }
    printf("Found dec, about to stop decoder %p...\n", dec);
    dec->Stop();
    printf("Unlinking element ...\n");
    dec->UnlinkNextElement();
    printf("Deleting dec %p...\n", dec);
    delete dec;
    dec = NULL;
    printf("DetachInputStream Done\n");

    ReleaseMutex();
    return ERR_NONE;
}

void* AudioTranscoder::AttachOutputStream(void *encCfg)
{
    BaseElement *element_enc = NULL;
    sAudioParams *audio_params = reinterpret_cast<sAudioParams *> (encCfg);
    printf("Attach output stream in AudioTrancoder %p\n", this);
    WaitMutex();
    //TODO prevent call this func before Init()

    if (!done_init_) {
        ReleaseMutex();
        printf("Attach new audio output before initialization\n");
        return NULL;
    }

    if (audio_params->nCodecType == STREAM_TYPE_AUDIO_PCM) {
       element_enc = new AudioPCMWriter(audio_params->output_stream, NULL);
    } else {
#ifdef ENABLE_AUDIO_CODEC
       element_enc = new AudioEncoder(audio_params->output_stream, NULL);
#endif
    }

    if (!element_enc) {
        ReleaseMutex();
        printf("Failed to new Encoder\n");
        return NULL;
    }

    if (!element_enc->Init(audio_params, ELEMENT_MODE_PASSIVE)) {
        delete element_enc;
        element_enc = NULL;
        ReleaseMutex();
        printf("Failed to initialise Encoder\n");
        return NULL;
    }

    mEncoder_list_.push_back(element_enc);
    mAPP_->LinkNextElement(element_enc);
    printf("Link APP %p to encoder %p...enc list size %d\n", mAPP_, element_enc, (int)mEncoder_list_.size());
    if (is_running_) {
        element_enc->Start();
    }
    ReleaseMutex();
    return element_enc;

}

errors_t AudioTranscoder::DetachOutputStream(void *output_handle)
{
    bool enc_found = false;
    if (NULL == output_handle) {
        return ERR_INVALID_INPUT_PARAMETER;
    }

    WaitMutex();
    if (!done_init_) {
        ReleaseMutex();
        printf("Detach audio output before initialization\n");
        return ERR_INVALID_INPUT_PARAMETER;
    }

    BaseElement *enc = static_cast<BaseElement*>(output_handle);
    for(std::list<BaseElement*>::iterator it_enc = mEncoder_list_.begin();
            it_enc != mEncoder_list_.end();
            ++it_enc) {
        if (enc == *it_enc) {
            mEncoder_list_.erase(it_enc);
            enc_found = true;
            break;
        }
    }

    if (!enc_found) {
        ReleaseMutex();
        return ERR_INVALID_INPUT_PARAMETER;
    }
    printf("Found enc, about to stop encoder %p...\n", enc);
    enc->Stop();
    printf("Unlinking element ...\n");
    enc->UnlinkNextElement();
    printf("Deleting enc %p...\n", enc);
    delete enc;
    enc = NULL;
    printf("DetachOutputStream Done\n");

    ReleaseMutex();
    return ERR_NONE;
}

void* AudioTranscoder::GetActiveInputHandle()
{
    if (IsAlive() == false) {
        printf("AudioTranscoder is not alive.\n");
        return NULL;
    }

    void *active_input = static_cast<AudioPostProcessing *>(mAPP_)->GetActiveInputHandle();

    return active_input;
}

int AudioTranscoder::GetInputActiveStatus(void *input_handle)
{
    if (NULL == input_handle) {
        printf("AudioTranscoder: input_handle is NULL.\n");
        return -1;
    }

    int status = static_cast<AudioPostProcessing *>(mAPP_)->GetInputActiveStatus(input_handle);
    return status;
}

errors_t AudioTranscoder::CreateEncoder(void *encCfg_list)
{
    bool res = true;
    std::list<Stream *>::iterator output_i;
    BaseElement *element_enc = NULL;
    std::list<sAudioParams *>::iterator encOpt_i;
    LIST_AUDDECOPT *encOpt_list = (LIST_AUDDECOPT *)encCfg_list;

    output_i = mOutputStream_list_.begin();

    for (encOpt_i = (*encOpt_list).begin();
         encOpt_i != (*encOpt_list).end() && output_i != mOutputStream_list_.end();
         encOpt_i++, output_i++) {
        if ((*encOpt_i)->nCodecType == STREAM_TYPE_AUDIO_PCM) {
            element_enc = new AudioPCMWriter(*output_i, NULL);
        } else {
#ifdef ENABLE_AUDIO_CODEC
            element_enc = new AudioEncoder(*output_i, NULL);
#endif
        }

        if (!element_enc) {
            ReleaseMutex();
            printf("Failed to new Encoder\n");
            return ERR_ALLOCATION_FAIL;
        }

        printf("enter AudioTranscoder::CreateEncoder\n");
        res = element_enc->Init((*encOpt_i), ELEMENT_MODE_PASSIVE);

        if (!res) {
            delete element_enc;
            element_enc = NULL;
            ReleaseMutex();
            printf("Failed to initialise Encoder\n");
            return ERR_ENCODER_INITIALIZATION_FAIL;
        }

       mEncoder_list_.push_back(element_enc);
    }

    return ERR_NONE;
}

void* AudioTranscoder::CreateDecoder(void *decCfg_list)
{
    BaseElement *element_dec = NULL;
    std::list<sAudioParams *>::iterator decOpt_i;
    std::list<MemPool *>::iterator input_i;
    LIST_AUDDECOPT *decOpt_list = (LIST_AUDDECOPT *)decCfg_list;

    input_i = mMemPool_list_.begin();

    for (decOpt_i = (*decOpt_list).begin();
         decOpt_i != (*decOpt_list).end() && input_i != mMemPool_list_.end();
         decOpt_i++, input_i++) {
        if ((*decOpt_i)->nCodecType == STREAM_TYPE_AUDIO_PCM) {
            element_dec = new AudioPCMReader((*decOpt_i)->input_stream, NULL);
        } else {
#ifdef ENABLE_AUDIO_CODEC
            element_dec = new AudioDecoder((*decOpt_i)->input_stream, NULL);
#endif
        }

        if (!element_dec) {
            ReleaseMutex();
            printf("Failed to new Decoder\n");
            return NULL;
        }

        if (!element_dec->Init((*decOpt_i), ELEMENT_MODE_ACTIVE)) {
            delete element_dec;
            element_dec = NULL;
            ReleaseMutex();
            printf("Failed to initialize Decoder\n");
            return NULL;
        }

        mDecoder_list_.push_back(element_dec);
    }

    return element_dec;
}

void* AudioTranscoder::Init(void *decCfg, void *appCfg, void *encCfg)
{
    FRMW_TRACE_INFO("mem pool size %d, input stream list size %d\n", (int)mMemPool_list_.size(),
                                                                     (int)mInputStream_list_.size());

    //check input parameters
    if (!decCfg || !appCfg || !encCfg) {
        printf("Input parameter can not be NULL\n");
        return NULL;
    }

    WaitMutex();

    //create decoder object
    BaseElement *element_dec = NULL;
    sAudioParams *dec_params = reinterpret_cast<sAudioParams *> (decCfg);

    if (dec_params->nCodecType == STREAM_TYPE_AUDIO_PCM) {
        element_dec = new AudioPCMReader(dec_params->input_stream, NULL);
    } else {
#ifdef ENABLE_AUDIO_CODEC
        element_dec = new AudioDecoder(dec_params->input_stream, NULL);
#endif
    }

    if (!element_dec) {
        ReleaseMutex();
        printf("Failed to new Decoder\n");
        return NULL;
    }

    if (!element_dec->Init(dec_params, ELEMENT_MODE_ACTIVE)) {
        delete element_dec;
        element_dec = NULL;
        ReleaseMutex();
        printf("Failed to initialize Decoder\n");
        return NULL;
    }

    //create APP object
    printf("create APP object\n");
    mAPP_ = new AudioPostProcessing();
    printf("[%p] ****new APP [%p]\n", this, mAPP_);
    if (!mAPP_) {
        delete element_dec;
        element_dec = NULL;
        ReleaseMutex();
        printf("Failed to create APP\n");
        return NULL;
    }

    if (!mAPP_->Init(appCfg, ELEMENT_MODE_ACTIVE)) {
        delete element_dec;
        element_dec = NULL;
        delete mAPP_;
        mAPP_ = NULL;
        ReleaseMutex();
        printf("Failed to initialize APP\n");
        return NULL;
    }

    //create encoder objects
    BaseElement *element_enc = NULL;
    sAudioParams *enc_params = reinterpret_cast<sAudioParams *> (encCfg);

    if (enc_params->nCodecType == STREAM_TYPE_AUDIO_PCM) {
       element_enc = new AudioPCMWriter(enc_params->output_stream, NULL);
    } else {
#ifdef ENABLE_AUDIO_CODEC
       element_enc = new AudioEncoder(enc_params->output_stream, NULL);
#endif
    }

    if (!element_enc) {
        delete element_dec;
        element_dec = NULL;
        delete mAPP_;
        mAPP_ = NULL;
        ReleaseMutex();
        printf("Failed to new Encoder\n");
        return NULL;
    }

    if (!element_enc->Init(enc_params, ELEMENT_MODE_PASSIVE)) {
        delete element_dec;
        element_dec = NULL;
        delete mAPP_;
        mAPP_ = NULL;
        delete element_enc;
        element_enc = NULL;
        ReleaseMutex();
        printf("Failed to initialise Encoder\n");
        return NULL;
    }

    element_dec->LinkNextElement(mAPP_);
    printf("Link decoder %p to APP %p...\n", element_dec, mAPP_);

    mAPP_->LinkNextElement(element_enc);
    printf("Link APP %p to encoder %p...\n", mAPP_, element_enc);


    mDecoder_list_.push_back(element_dec);
    mEncoder_list_.push_back(element_enc);
    done_init_ = true;

    printf("Create pipeline done\n");
    ReleaseMutex();

    return element_dec;
}

bool AudioTranscoder::IsAlive()
{
    bool alive = false;

    std::list<BaseElement *>::iterator dec_i;

    //check decoder alive
    for (dec_i = mDecoder_list_.begin(); dec_i != mDecoder_list_.end(); dec_i++) {
        alive |= (*dec_i)->IsAlive();
    }

    return alive;
}

bool AudioTranscoder::Cleanup()
{
    std::list<BaseElement *>::iterator item;
    if (is_running_ == true) {
        Stop();
    }

    WaitMutex();

    //cleanup encoder
    item = mEncoder_list_.begin();

    while (item != mEncoder_list_.end()) {
        if (*item) {
            delete (*item);
        }

        mEncoder_list_.pop_front();
        item = mEncoder_list_.begin();
    }

    //cleanup APP
    if (mAPP_) {
        delete mAPP_;
        mAPP_ = NULL;
    }

    //cleanup decoder
    item = mDecoder_list_.begin();

    while (item != mDecoder_list_.end()) {
        if (*item) {
            delete (*item);
        }

        mDecoder_list_.pop_front();
        item = mDecoder_list_.begin();
    }

    //cleanup output streams
    std::list<Stream *>::iterator str;

    str = mOutputStream_list_.begin();
    while (str != mOutputStream_list_.end()) {
        if (*str) {
            delete (*str);
        }

        mOutputStream_list_.pop_front();
        str = mOutputStream_list_.begin();
    }

    //cleanup input streams
    str = mInputStream_list_.begin();

    while (str != mInputStream_list_.end()) {
        if (*str) {
            delete (*str);
        }

        mInputStream_list_.pop_front();
        str = mInputStream_list_.begin();
    }

    //cleanup mem pool
    std::list<MemPool *>::iterator mem;
    mem = mMemPool_list_.begin();

    while (mem != mMemPool_list_.end()) {
        if (*mem) {
            delete (*mem);
        }

        mMemPool_list_.pop_front();
        mem = mMemPool_list_.begin();
    }

    ReleaseMutex();
    return true;
}

bool AudioTranscoder::Start()
{
    WaitMutex();
    //create pipeline dec->vpp->enc
    printf("Starting pipeline... \n ");
    std::list<BaseElement *>::iterator dec_i;
    std::list<BaseElement *>::iterator enc_i;

    //start the pipeline
    bool res = true;

    //start encoder
    for (enc_i = mEncoder_list_.begin(); enc_i != mEncoder_list_.end(); enc_i++) {
        if (*enc_i) {
            res &= (*enc_i)->Start();
        }
    }

    //start APP
    if (mAPP_) {
        mAPP_->Start();
    }

    //start decoder
    for (dec_i = mDecoder_list_.begin(); dec_i != mDecoder_list_.end(); dec_i++) {
        if (*dec_i) {
            res &= (*dec_i)->Start();
        }
    }

    //set running status
    is_running_ = true;

    if (is_from_file_) {
        while (is_running_) {
            unsigned int read_finish_nums = 0;
            std::list<MemPool *>::iterator mempool_i = mMemPool_list_.begin();
            std::list<Stream *>::iterator input_i = mInputStream_list_.begin();

            for (; mempool_i != mMemPool_list_.end() && input_i != mInputStream_list_.end();
                   mempool_i++, input_i++) {
                int read_size = 0;
                int mempool_freeflat = (*mempool_i)->GetFreeFlatBufSize();
                unsigned char *mempool_wrptr = (*mempool_i)->GetWritePtr();

                bool is_alive = IsAlive();
                if(!is_alive) {
                    ReleaseMutex();
                    return res;
                }

                if (mempool_freeflat == 0) {
                    FRMW_TRACE_DEBUG("Mempool %p is still full, waiting...\n",*mempool_i);
                    usleep(100 * 1000);
                    continue;
                }

                if ((*input_i)->GetEndOfStream()) {
                    (*mempool_i)->SetDataEof(true);
                    read_finish_nums++;
                    continue;
                }

                read_size = (*input_i)->ReadBlock(mempool_wrptr, mempool_freeflat);
                (*mempool_i)->UpdateWritePtrCopyData(read_size);
                FRMW_TRACE_DEBUG("read_size = %d input_i %p mempool_i %p\n", read_size, *input_i, *mempool_i);
            }

            if (mInputStream_list_.size() == read_finish_nums) {
                printf("Read all files(%d) done.\n", read_finish_nums);
                break;
            }

        }
    }

    ReleaseMutex();
    return res;
}

bool AudioTranscoder::Join()
{
    printf("Processing join...\n");
    std::list<BaseElement *>::iterator dec_i;
    std::list<BaseElement *>::iterator enc_i;

    bool res  = true;

    //join encoder
    for (enc_i = mEncoder_list_.begin(); enc_i != mEncoder_list_.end(); enc_i++) {
        if (*enc_i) {
            res &= (*enc_i)->Join();
        }
    }

    //join APP 
    if (mAPP_) {
        mAPP_->Join();
    }

    //join decoder
    for (dec_i = mDecoder_list_.begin(); dec_i != mDecoder_list_.end(); dec_i++) {
        if (*dec_i) {
            res &= (*dec_i)->Join();
        }
    }

    printf("AudioTranscoder Join: Done processing\n");
    return true;
}

bool AudioTranscoder::Stop()
{
    printf("Stopping transcoder...\n");
    WaitMutex();
    std::list<BaseElement *>::iterator dec_i;
    std::list<BaseElement *>::iterator enc_i;
    bool res  = true;

    //stop encoder
    for (enc_i = mEncoder_list_.begin(); enc_i != mEncoder_list_.end(); enc_i++) {
        if (*enc_i) {
            res &= (*enc_i)->Stop();
        }
    }

    //stop APP
    if (mAPP_) {
        mAPP_->Stop();
    }

    //stop dec
    for (dec_i = mDecoder_list_.begin(); dec_i != mDecoder_list_.end(); dec_i++) {
        if (*dec_i) {
            res &= (*dec_i)->Stop();
        }
    }

    printf("AudioTranscoder Stop: Done processing!\n");
    //set running status
    is_running_ = false;
    ReleaseMutex();
    return true;
}

bool AudioTranscoder::WaitMutex()
{
    if (pthread_mutex_lock(&xcoder_mutex_)) {
        printf("Error in pthread_mutex_lock.\n");
        assert(0);
    }
    return true;
}

bool AudioTranscoder::ReleaseMutex()
{
    if (pthread_mutex_unlock(&xcoder_mutex_)) {
        printf("Error in pthread_mutex_unlock.\n");
        assert(0);
    }
    return true;
}

