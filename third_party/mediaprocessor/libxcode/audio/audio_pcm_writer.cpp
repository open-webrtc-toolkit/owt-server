/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/
#include "audio_pcm_writer.h"
#include "base/trace.h"

AudioPCMWriter::AudioPCMWriter(Stream* st, unsigned char* name) :
    m_pStream(st),
    m_nDataSize(0)
{
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
    }
    m_pStream->SetEndOfStream();
}

bool AudioPCMWriter::Init(void* cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    sAudioParams* audio_params;
    audio_params = reinterpret_cast<sAudioParams *> (cfg);

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

    return 0;
}


int AudioPCMWriter::PCMWrite(AudioPayload* pIn)
{
    int dataOffset = m_WaveHeader.Interpret(pIn->payload, &m_WaveInfo, pIn->payload_length);
    if (dataOffset <= 0) {
        printf("PCMWrite: Invalid or unsupported wav format!\n");
        return -1;
    }

    unsigned int frameSize;
    if (pIn->payload_length > dataOffset) {
        frameSize = pIn->payload_length - dataOffset;
    }
    else {
        printf("%s: input payload size (%d) is lower than data dataOffset(%d)\n", __FUNCTION__, pIn->payload_length, dataOffset);
        frameSize = 0;
    }

    if (pIn->isFirstPacket) {
        m_WaveHeader.Populate(&m_WaveInfo, 0xFFFFFFFF);
        m_pStream->WriteBlock(&m_WaveHeader, m_WaveHeader.GetHeaderSize());
    }
    m_pStream->WriteBlock(pIn->payload + dataOffset, frameSize);
    m_nDataSize += frameSize;

    return 0;
}

int AudioPCMWriter::HandleProcess()
{
    return 0;
}

