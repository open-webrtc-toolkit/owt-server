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

//#define DUMP_AUDIO_PCM_INPUT    // Customized option for debugging in WebRTC scenario

AudioPCMReader::AudioPCMReader(MemPool* mp, char* input) :
    m_pMempool(mp),
    m_pDumpOutFile(NULL),
    m_nDataSize(0)
{
    for (int i = 0; i < AUDIO_OUTPUT_QUEUE_SIZE; i++) {
        m_Output[i].payload = NULL;
    }
}

AudioPCMReader::~AudioPCMReader()
{
    Release();
}

void AudioPCMReader::Release()
{
    for (int i = 0; i < AUDIO_OUTPUT_QUEUE_SIZE; i++) {
        if(m_Output[i].payload) {
            free(m_Output[i].payload);
            m_Output[i].payload_length = 0;
        }
    }
#ifdef DUMP_AUDIO_PCM_INPUT
    if (m_pDumpOutFile) {
        m_pDumpOutFile->SetStreamAttribute(FILE_HEAD);
        m_WaveInfoOut.channels_number = 2;
        m_WaveHeader.Populate(&m_WaveInfoOut, m_nDataSize);
        m_pDumpOutFile->WriteBlock(&m_WaveHeader, m_WaveHeader.GetHeaderSize());
        delete m_pDumpOutFile;
    }
#endif
}

bool AudioPCMReader::Init(void* cfg, ElementMode element_mode)
{
    sAudioParams* audio_params;
    audio_params = static_cast<sAudioParams*>(cfg);
    element_mode_ = element_mode;

#ifdef DUMP_AUDIO_PCM_INPUT
    m_pDumpOutFile = new Stream;
    bool status = false;
    char file_name[FILENAME_MAX] = {0};
    sprintf(file_name, "dump_input_%p.wav", this);
    if (m_pDumpOutFile) {
        status = m_pDumpOutFile->Open(file_name, true);
    }

    if (!status) {
        if (m_pDumpOutFile) {
            delete m_pDumpOutFile;
            m_pDumpOutFile = NULL;
        }
    }
    printf("[%p]: Dump input pcm file to: %s\n", this, file_name);
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
        m_output_queue.Push(payload);
    } else {
        return -1;
    }

    return 0;
}

int AudioPCMReader::HandleProcess()
{
    bool isFirstPacket = true;
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
        unsigned int mem_size = m_pMempool->GetFlatBufFullness();
        buffer_size = (mem_size < frame_size) ? mem_size : frame_size;

        unsigned int eof = m_pMempool->GetDataEof();
        if ((buffer_size == 0) && (eof == true)) {
            printf("PCMReader: [%p] Got the end of stream.\n", this);
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
        if (isFirstPacket) {
            payload_in.payload = m_pMempool->GetReadPtr();
            payload_in.payload_length = 1024;
            ret = ParseWAVHeader(&payload_in);
            if (ret == -1) {
                printf("ParsePCMHeader error, return.\n");
                buf.payload = NULL;
                buf.payload_length = 0;
                srcpad->PushBufToPeerPad(buf);
                break;
            }
#ifdef DUMP_AUDIO_PCM_INPUT
            if (m_pDumpOutFile) {
                m_WaveInfoOut.channels_number = 2;
                m_WaveHeader.Populate(&m_WaveInfoOut, 0xFFFFFFFF);
                m_pDumpOutFile->WriteBlock(&m_WaveHeader, m_WaveHeader.GetHeaderSize());
                m_WaveInfoOut.channels_number = 1;
                m_WaveHeader.Populate(&m_WaveInfoOut, 0xFFFFFFFF);
            }
#endif
            m_pMempool->UpdateReadPtr(m_nDataOffset);
            m_nDataSize += m_nDataOffset;
            // 30ms as one audio frame
            frame_size = m_WaveInfoOut.channels_number *
                         m_WaveInfoOut.sample_rate * 3 *
                         (m_WaveInfoOut.resolution >> 3) / 50;
            printf("AudioPCMReader[%p]: frame_size = %d\n", this, frame_size);
            buffer_size = (mem_size < frame_size) ? mem_size : frame_size;
            // allocate memory for audio payload
            for (i = 0; i < AUDIO_OUTPUT_QUEUE_SIZE; i++) {
                m_Output[i].payload = reinterpret_cast<unsigned char*>
                                      (malloc(frame_size + sizeof(WaveHeader)));
                if (!m_Output[i].payload) {
                    printf("AudioPCMReader[%p]: Failed to allocate memory for payload!\n", this);
                    continue;
                }
                m_Output[i].payload_length  = 0;
                m_output_queue.Push(m_Output[i]);
            }
        }

        // step 3: check if there is free buffer for decoded wav.
        if (m_output_queue.Pop(payload_out) == false || (NULL == payload_out.payload)) {
            APP_TRACE_INFO("There is no empty buffer for PCM reader(%p).\n", this);
            usleep(10 * 1000);
            continue;
        }

        // step 4: get PCM frame data.
        payload_in.payload = m_pMempool->GetReadPtr();
        payload_in.payload_length = buffer_size;
        data_consumed = 0;
        ret = PCMRead(&payload_out, &payload_in, isFirstPacket, &data_consumed);
        if (ret == -1) {
            printf("PCMRead error, return.\n");
            buf.payload = NULL;
            buf.payload_length = 0;
            srcpad->PushBufToPeerPad(buf);
            break;
        }
#ifdef DUMP_AUDIO_PCM_INPUT
        if (m_pDumpOutFile) {
            m_pDumpOutFile->WriteBlock(m_pMempool->GetReadPtr(), buffer_size);
        }
#endif
        frame_count++;
        measure.GetCurTime(&cur_time);
        if (cur_time - last_time >= 1000000) {
            APP_TRACE_DEBUG("[%p]: audio frame rate = %d\n", this, frame_count);
            last_time = cur_time;
            frame_count = 0;
        }

        m_nDataSize += data_consumed;
        m_pMempool->UpdateReadPtr(data_consumed);
        isFirstPacket = false;

        // step 4: send decoded stream to next element
        buf.payload = payload_out.payload;
        buf.payload_length = payload_out.payload_length;
        buf.isFirstPacket = payload_out.isFirstPacket;

        srcpad->PushBufToPeerPad(buf);
    }
    is_running_ = false;
    return 0;
}

int AudioPCMReader::ParseWAVHeader(AudioPayload *pIn)
{
    m_nDataOffset = m_WaveHeader.Interpret(pIn->payload, &m_WaveInfoOut, pIn->payload_length);
    if (m_nDataOffset <= 0) {
        printf("PCMRead: Invalid or unsupported wav format!\n");
        return -1;
    }
    unsigned int inputDataSize = pIn->payload_length - m_nDataOffset;
    // Create a RIFF header to prepend to the PCM samples
    m_WaveHeader.Populate(&m_WaveInfoOut, inputDataSize);
    printf("%s: channel:sample_frequency:bit_per_sample: channel_mask is %d:%d:%d:%d\n",
           __FUNCTION__,
           m_WaveInfoOut.channels_number,
           m_WaveInfoOut.sample_rate,
           m_WaveInfoOut.resolution,
           m_WaveInfoOut.channel_mask);

    return 0;
}

// PCMRead: Generate PCM frame
int AudioPCMReader::PCMRead(AudioPayload *pOut, AudioPayload *pIn, bool isFirstPacket, unsigned int* DataConsumed)
{
    unsigned char *inputData;
    unsigned int inputDataSize;

    inputData = pIn->payload;
    inputDataSize = pIn->payload_length;

    if ((inputData) && (inputDataSize > 0)) {
        // Assemble RIFF header and data into a new PayloadInfo
        pOut->payload_length = m_WaveHeader.GetHeaderSize() + inputDataSize;
        memcpy(pOut->payload, (unsigned char *) &m_WaveHeader, m_WaveHeader.GetHeaderSize());
        memcpy(pOut->payload + m_WaveHeader.GetHeaderSize(), inputData, inputDataSize);
        APP_TRACE_INFO("%s: The wave %p header size is %d\n", __FUNCTION__, &m_WaveHeader, m_WaveHeader.GetHeaderSize());
        pOut->isFirstPacket = isFirstPacket;
    }

    *DataConsumed = pIn->payload_length;
    return 0;
}
