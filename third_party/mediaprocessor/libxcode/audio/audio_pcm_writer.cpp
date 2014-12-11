/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/
#include "audio_pcm_writer.h"
#include "base/trace.h"

//#define DUMP_AUDIO_PCM_OUTPUT    // Customized option for debugging in WebRTC scenario
AudioPCMWriter::AudioPCMWriter(Stream* st, unsigned char* name) :
    m_pStream(st),
    m_nDataSize(0)
{
    m_nFrameCount = 0;
    m_nCurTime = 0;
    m_nLastTime = 0;
}

AudioPCMWriter::~AudioPCMWriter()
{
    Release();
}

void AudioPCMWriter::Release()
{
    //write the final data length value to wav file header
    if (m_nDataSize) {
        m_pStream->SetStreamAttribute(FILE_HEAD);
        m_WaveHeader.Populate(&m_WaveInfo, m_nDataSize);
        m_pStream->WriteBlock(&m_WaveHeader, m_WaveHeader.GetHeaderSize());
#ifdef DUMP_AUDIO_PCM_OUTPUT
    if (m_pDumpOutFile) {
        m_pDumpOutFile->SetStreamAttribute(FILE_HEAD);
        m_WaveInfo.channels_number = 2;
        m_WaveHeader.Populate(&m_WaveInfo, m_nDataSize);
        m_pDumpOutFile->WriteBlock(&m_WaveHeader, m_WaveHeader.GetHeaderSize());
        delete m_pDumpOutFile;
    }
#endif
    }
    m_pStream->SetEndOfStream();
}

bool AudioPCMWriter::Init(void* cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    sAudioParams* audio_params;
    audio_params = reinterpret_cast<sAudioParams *> (cfg);

    m_Measure.GetCurTime(&m_nCurTime);
    m_nLastTime = m_nCurTime;
#ifdef DUMP_AUDIO_PCM_OUTPUT
    m_pDumpOutFile = new Stream;
    bool status = false;
    char file_name[FILENAME_MAX] = {0};
    sprintf(file_name, "dump_pcm_output_%p.wav", this);
    if (m_pDumpOutFile) {
        status = m_pDumpOutFile->Open(file_name, true);
    }

    if (!status) {
        if (m_pDumpOutFile) {
            delete m_pDumpOutFile;
            m_pDumpOutFile = NULL;
        }
    }
    printf("[%p]: Dump output pcm file to: %s\n", this, file_name);
#endif
    return true;
}

int AudioPCMWriter::Recycle(MediaBuf &buf)
{
    //We don't have any SID resource, directly return
    return 0;
}

int AudioPCMWriter::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    AudioPayload pIn;
    pIn.payload = buf.payload;
    pIn.payload_length = buf.payload_length;
    pIn.isFirstPacket = buf.isFirstPacket;

    if (NULL == pIn.payload) {
        APP_TRACE_INFO("PCMWriter: %s: input audio payload is empty, audio PCMWriter get eof\n", __FUNCTION__);
        return 0;
    }

    if (PCMWrite(&pIn) != 0) {
        ReturnBuf(buf);
        return -1;
    }

    m_nFrameCount++;
    m_Measure.GetCurTime(&m_nCurTime);
    if (m_nCurTime - m_nLastTime >= 1000000) {
        APP_TRACE_DEBUG("AudioPCMWriter[%p]: audio output frame rate = %d\n", this, m_nFrameCount);
        m_nLastTime = m_nCurTime;
        m_nFrameCount = 0;
    }
    return 0;
}


int AudioPCMWriter::PCMWrite(AudioPayload* pIn)
{
    int dataOffset = m_WaveHeader.Interpret(pIn->payload, &m_WaveInfo, pIn->payload_length);
    if (dataOffset <= 0) {
        printf("AudioPCMWriter: Invalid or unsupported wav format!\n");
        return -1;
    }

    unsigned int frameSize;
    if (pIn->payload_length > dataOffset) {
        frameSize = pIn->payload_length - dataOffset;
    }
    else {
        printf("AudioPCMWriter[%p]: input payload size (%d) is lower than data dataOffset(%d)\n",
               this,
               pIn->payload_length,
               dataOffset);
        frameSize = 0;
    }

    if (pIn->isFirstPacket) {
#ifdef DUMP_AUDIO_PCM_OUTPUT
        m_WaveInfo.channels_number = 2;    // Special case for debugging in WebRTC scenario
        m_WaveHeader.Populate(&m_WaveInfo, 0xFFFFFFFF);
        m_pDumpOutFile->WriteBlock(&m_WaveHeader, m_WaveHeader.GetHeaderSize());
        m_WaveInfo.channels_number = 1;    // Special case for debugging in WebRTC scenario
#endif
        m_WaveHeader.Populate(&m_WaveInfo, 0xFFFFFFFF);
        m_pStream->WriteBlock(&m_WaveHeader, m_WaveHeader.GetHeaderSize());
    }
    m_pStream->WriteBlock(pIn->payload + dataOffset, frameSize);
#ifdef DUMP_AUDIO_PCM_OUTPUT
        m_pDumpOutFile->WriteBlock(pIn->payload + dataOffset, frameSize);
#endif
    m_nDataSize += frameSize;

    return 0;
}

int AudioPCMWriter::HandleProcess()
{
    return 0;
}

