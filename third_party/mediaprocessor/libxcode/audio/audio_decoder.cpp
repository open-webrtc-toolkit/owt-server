/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/
#ifdef ENABLE_AUDIO_CODEC
#include "base/media_pad.h"
#include "base/trace.h"
#include "audio_decoder.h"
#include "audio_params.h"

AudioDecoder::AudioDecoder(MemPool* mp, char* input) :
    m_pMempool(mp),
    m_pRawDataMP(NULL),
    m_pOut(NULL),
    m_pUMCDecoder(NULL),
    m_pAudioDecoderParams(NULL),
    m_codec_id(0),
    m_dump(false),
    m_bHaveDecoderValuesBeenInitialised(false),
    m_bResetFlag(false)
{
    m_DecoderName = input;
    m_bFirstPass = true;
    for (int i = 0; i < AUDIO_OUTPUT_QUEUE_SIZE; i++) {
        m_Output[i].payload = NULL;
    }
}

AudioDecoder::~AudioDecoder()
{
    Release();
}

void AudioDecoder::Release()
{
    if (m_pUMCDecoder) {
        delete m_pUMCDecoder;
        m_pUMCDecoder = NULL;
    }

    for (int i = 0; i < AUDIO_OUTPUT_QUEUE_SIZE; i++) {
        if(m_Output[i].payload) {
            free(m_Output[i].payload);
            m_Output[i].payload = NULL;
            m_Output[i].payload_length = 0;
        }
    }

    if (m_pOut) {
        delete m_pOut;
        m_pOut = NULL;
    }

    if (m_pRawDataMP) {
        delete m_pRawDataMP;
        m_pRawDataMP = NULL;
    }
}

bool AudioDecoder::Init(void* cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;

    if (NULL == m_pUMCDecoder) {
        m_pUMCDecoder = new UMC::AACDecoder();
    }

    m_aacdec_params.ModeDecodeHEAACprofile = HEAAC_HQ_MODE;
    m_aacdec_params.ModeDwnsmplHEAACprofile= HEAAC_DWNSMPL_ON;
    m_aacdec_params.layer = -1;

    m_pAudioDecoderParams = static_cast<UMC::AudioDecoderParams*>(&m_aacdec_params);
    m_pAudioDecoderParams->m_info.streamType = UMC::UNDEF_AUDIO;
    m_pAudioDecoderParams->m_pData = NULL;

    if (m_dump) {
#ifdef DUMP_AUDIO_DEC_OUTPUT
        char* outputFileName = "audio_dec_out.raw";
        m_pDumpOutFile = fopen(outputFileName, "wb");
        APP_TRACE_INFO("%s: Opening %s\n", __FUNCTION__, outputFileName);
        if (NULL == m_pDumpOutFile) {
            printf("Can't create output file %s\n", outputFileName);
        }
#endif
    }

    m_pRawDataMP = new MemPool();
    m_pRawDataMP->init();

    return true;
}

int AudioDecoder::ProcessChain(MediaPad* pad, MediaBuf &buf)
{
    return -1;
}

int AudioDecoder::Recycle(MediaBuf &buf)
{
    AudioPayload payload;
    payload.payload = buf.payload;
    payload.payload_length = buf.payload_length;
    payload.isFirstPacket = buf.isFirstPacket;

    if (payload.payload) {
        m_output_queue.Push(payload);
    } else {
        return -1;
    }

    return 0;
}

int AudioDecoder::HandleProcess()
{
    bool isFirstPacket = true;
    bool is_first_packet_out = true;
    static int total_size = 0;
    int ret;
    AudioPayload payload_out;
    AudioPayload payload_in;
    MediaBuf buf;
    MediaPad *srcpad = *(this->srcpads_.begin());
    unsigned int data_consumed = 0;
    unsigned int raw_data_frame_size = 0;
    int i = 0;

    while (is_running_) {
        // step 1: check if there is available coded stream
        unsigned int buffer_size = m_pMempool->GetFlatBufFullness();
        unsigned int eof = m_pMempool->GetDataEof();
        if ((buffer_size == 0) && (eof == true)) {
            printf("Got the end of stream.\n");
            buf.payload = NULL;
            buf.payload_length = 0;
            srcpad->PushBufToPeerPad(buf);
            break;
        }
        if ((buffer_size < 10 * 1024) && (eof != true)) {
            usleep(10 * 1000);
            continue;
        }

        // step 2: decode the coded stream.
        payload_in.payload = m_pMempool->GetReadPtr();
        payload_in.payload_length = buffer_size;
        data_consumed = 0;
        ret = Decode(&payload_in, isFirstPacket, &data_consumed);
        if (ret == -1) {
            printf("Decode error, return.\n");
            buf.payload = NULL;
            buf.payload_length = 0;
            srcpad->PushBufToPeerPad(buf);
            break;
        }
        total_size = total_size + data_consumed;
        m_pMempool->UpdateReadPtr(data_consumed);

        if (isFirstPacket) {
            // 30ms as one audio frame
            raw_data_frame_size = m_WaveInfoOut.channels_number *
                                  m_WaveInfoOut.sample_rate * 3 *
                                  (m_WaveInfoOut.resolution >> 3) / 50;
            printf("AudioDecoder: raw_data_frame_size = %d\n", raw_data_frame_size);
            // allocate memory for audio payload
            for (i = 0; i < AUDIO_OUTPUT_QUEUE_SIZE; i++) {
                m_Output[i].payload = reinterpret_cast<unsigned char*>
                                      (malloc(raw_data_frame_size + sizeof(WaveHeader)));
                m_Output[i].payload_length  = 0;
                m_output_queue.Push(m_Output[i]);
            }
        }

        // step 3: build output payload
        while (m_pRawDataMP->GetFlatBufFullness() >= raw_data_frame_size) {
            unsigned int eof_raw_buf = m_pRawDataMP->GetDataEof();
            if (eof_raw_buf) {
                break;
            }

            // Check if there is free buffer for decoded wav.
            if (m_output_queue.Pop(payload_out) == false) {
                printf("There is no empty buffer for decoded wav.\n");
                usleep(10*1000);
                continue;
            }

            // Create a RIFF header to prepend to the decoded samples
            WaveHeader wave;
            wave.Populate(&m_WaveInfoOut, raw_data_frame_size);

            // Assemble RIFF header and data into a new PayloadInfo
            payload_out.payload_length = wave.GetHeaderSize() + raw_data_frame_size;
            memcpy(payload_out.payload, (Ipp8u *) &wave, wave.GetHeaderSize());
            memcpy(payload_out.payload + wave.GetHeaderSize(), m_pRawDataMP->GetReadPtr(), raw_data_frame_size);
            payload_out.isFirstPacket = is_first_packet_out;
            is_first_packet_out = false;

            // Push decoded frame to next element
            buf.payload = payload_out.payload;
            buf.payload_length = payload_out.payload_length;
            buf.isFirstPacket = payload_out.isFirstPacket;
            srcpad->PushBufToPeerPad(buf);

            m_pRawDataMP->UpdateReadPtr(raw_data_frame_size);
        }

        isFirstPacket = false;
    }
    is_running_ = false;
    return 0;
}

bool AudioDecoder::FindAACFrame(unsigned char* buf, unsigned int buf_size, unsigned int* frame_size)
{
    unsigned char* ptr = buf;
    unsigned int size;
    unsigned int sync_word = ptr[0] | ((ptr[1] >> 4) << 8);
    if (sync_word != 0xfff) {
        printf("invalid sync word is %x\n",sync_word);
    }

    size = ptr[5] | (ptr[4] << 8) | (ptr[3] << 16);
    size = ((size & 0x3ffe0) >> 5);
    *frame_size = size;
    return true;
}

// Decode: currently only supports AAC
int AudioDecoder::Decode(AudioPayload *pIn, bool isFirstPacket, unsigned int* DataConsumed)
{
    size_t dataSize = 0;
    UMC::Status dec_sts = 0;
    uint8_t *inputData = pIn->payload;
    size_t inputDataSize = pIn->payload_length;

    if (isFirstPacket == false) {
        m_bHaveDecoderValuesBeenInitialised = true;
    } else {
#ifdef DUMP_AUDIO_DEC_OUTPUT
        if (m_bFirstPass) {
            char* outputFileName = "audio_dec_out.raw";
            m_pDumpOutFile = fopen(outputFileName, "wb");
            APP_TRACE_INFO("%s: Opening %S\n", __FUNCTION__, outputFileName);
            if (NULL == m_pDumpOutFile) {
                printf("Can't create output file '%S'", outputFileName);
            }
        }
#endif

        if (m_bFirstPass == false) {
            m_bResetFlag = true;
        }
        m_bFirstPass = false;

        m_bHaveDecoderValuesBeenInitialised = false; // this is a first packet so we must re-initialize
    }

    if (m_bHaveDecoderValuesBeenInitialised == false) {
        if (m_bResetFlag == true) {
            if (m_pUMCDecoder) {
                delete m_pUMCDecoder;
                m_pUMCDecoder = NULL;
            }
            delete [] m_pOut;
        }

        // Initialization
        m_NumFramesToProcess = 0;
        m_pOut = NULL;

        m_pAudioDecoderParams = NULL;

        m_aacdec_params.ModeDecodeHEAACprofile = HEAAC_HQ_MODE;
        m_aacdec_params.ModeDwnsmplHEAACprofile= HEAAC_DWNSMPL_ON;
        m_aacdec_params.layer = -1;

        m_pAudioDecoderParams = DynamicCast<UMC::AudioDecoderParams>(&m_aacdec_params);
        if (NULL == m_pAudioDecoderParams) {
            printf("AudioDecoder Memory allocation error\n");
            return -1;
        }

    }//end if(dec_haveEncoderValuesBeenInitialised == false)

    m_pAudioDecoderParams->m_info.streamType = UMC::UNDEF_AUDIO;

    if (m_bHaveDecoderValuesBeenInitialised == false) {
        if (m_pUMCDecoder == NULL) {
            m_pUMCDecoder = new UMC::AACDecoder();
            if (m_pUMCDecoder == NULL) {
                printf("AudioDecoder can't create audio codec object \n");
                return -1;
            }
        }
    }

    m_pAudioDecoderParams->m_pData = NULL;

    if (m_bHaveDecoderValuesBeenInitialised == false) {
        UMC::MediaData codecData;

        dec_sts = m_pUMCDecoder->Init(m_pAudioDecoderParams); // Prepare the parameter here.
        if (dec_sts != UMC::UMC_OK) {
            printf("AudioDecoder audio codec Init failed\n");
            return -1;
        }

        dec_sts = m_pUMCDecoder->GetInfo(m_pAudioDecoderParams);
        if (dec_sts < UMC::UMC_OK) {
            printf("AudioDecoder audio codec GetInfo failed\n");
            return -1;
        }

        APP_TRACE_INFO("%s: m_pAudioDecoderParams->m_SuggestedOutputSize= %d\n", __FUNCTION__, m_pAudioDecoderParams->m_iSuggestedOutputSize);

        m_pOut = new UMC::MediaData();
        if (NULL == m_pOut) {
            printf("Decoder output buffer memory allocation error\n");
            return -1;
        }
        if (UMC::UMC_OK != m_pOut->Alloc(m_pAudioDecoderParams->m_iSuggestedOutputSize)) {
            printf("Decoder output buffer allocation error\n");
            return -1;
        }
    }

    // TODO: Make sure a frame in the buffer.
    unsigned int frame_size = 0;
    FindAACFrame(inputData, inputDataSize, &frame_size);

    m_inMediaBuffer.SetBufferPointer(inputData, inputDataSize);
    m_inMediaBuffer.SetDataSize(inputDataSize);

    dec_sts = m_pUMCDecoder->GetFrame(&m_inMediaBuffer, m_pOut);
    // Supply the data to be decoded via dec_inMediaBuffer
    // and the result of the operation will be placed in dec_pOut
    if (dec_sts != UMC::UMC_OK) {
        printf("%s: Unsupported or Invalid stream. GetFrame Error (%d)\n", __FUNCTION__, dec_sts);
        return -1;
    }

#ifdef DUMP_AUDIO_DEC_OUTPUT
    if (m_pDumpOutFile) {
        fwrite(m_pOut->GetDataPointer(), 1, m_pOut->GetDataSize(), m_pDumpOutFile);
    }
#endif
    if(NULL == m_pOut) {
        printf("%s: m_pOut NULL!\n", __FUNCTION__);
        return -1;
    }
    APP_TRACE_INFO("%s: decoded data: %d\n", __FUNCTION__, (int)m_pOut->GetDataSize());

    if (dec_sts == UMC::UMC_ERR_NOT_ENOUGH_DATA) {
        return -1;
    } else if (dec_sts == UMC::UMC_OK) {
        if (m_NumFramesToProcess == 0) {
            dec_sts = m_pUMCDecoder->GetInfo(m_pAudioDecoderParams);

            if (m_pAudioDecoderParams->m_info.streamSubtype == UMC::AAC_ALS_PROFILE)
            {
                /* No need to add wav header for ALS profile */
            }
        }

        dataSize += m_pOut->GetDataSize();

        if (inputDataSize != 0) {
            m_NumFramesToProcess++;
        }
    } else if (dec_sts == UMC::UMC_ERR_NOT_ENOUGH_BUFFER) {
        delete m_pOut;

        dec_sts = m_pUMCDecoder->GetInfo(m_pAudioDecoderParams);

        m_pOut = new UMC::MediaData();
        if (NULL == m_pOut) {
            printf("AudioDecoder output buffer memory allocation error\n");
            return -1;
        }
        dec_sts = m_pOut->Alloc(m_pAudioDecoderParams->m_iSuggestedOutputSize);
        if (UMC::UMC_OK == dec_sts) {
            printf("AudioDecoder output buffer allocation error\n");
            return -1;
        }
    }

    m_pUMCDecoder->GetInfo(m_pAudioDecoderParams);

    m_WaveInfoOut.format_tag = 1;
    m_WaveInfoOut.channels_number = m_pAudioDecoderParams->m_info.audioInfo.m_iChannels;
    m_WaveInfoOut.sample_rate = m_pAudioDecoderParams->m_info.audioInfo.m_iSampleFrequency;
    m_WaveInfoOut.resolution = m_pAudioDecoderParams->m_info.audioInfo.m_iBitPerSample;
    m_WaveInfoOut.channel_mask = m_pAudioDecoderParams->m_info.audioInfo.m_iChannelMask;

    APP_TRACE_INFO("%s: Output DataSize=%d\n", __FUNCTION__, (int)m_pOut->GetDataSize());

    APP_TRACE_INFO("%s: channel:sample_frequency:bit_per_sample: channel_mask is %d:%d:%d:%d\n",
                        __FUNCTION__,
                        m_WaveInfoOut.channels_number,
                        m_WaveInfoOut.sample_rate,
                        m_WaveInfoOut.resolution,
                        m_WaveInfoOut.channel_mask);

    if (dec_sts == UMC::UMC_OK) {
        if (m_pOut->GetDataSize()) {
            // Write decoded audio data to mempool
            unsigned int mempool_freeflat = m_pRawDataMP->GetFreeFlatBufSize();
            unsigned char *mempool_wrptr = m_pRawDataMP->GetWritePtr();
            if (mempool_freeflat >= m_pOut->GetDataSize()) {
                memcpy(mempool_wrptr, m_pOut->GetBufferPointer(), m_pOut->GetDataSize());
                m_pRawDataMP->UpdateWritePtrCopyData(m_pOut->GetDataSize());
            }

#ifdef DUMP_AUDIO_DEC_OUTPUT
            if (m_pDumpOutFile) {
                fwrite(m_pOut->GetBufferPointer(), 1, m_pOut->GetDataSize(), m_pDumpOutFile);
            }
#endif
            m_bResetFlag = false;
            m_bHaveDecoderValuesBeenInitialised = true; // Ensure initialisation code only runs once
        }

        *DataConsumed = frame_size;
    }
    return 0;
}
#endif
