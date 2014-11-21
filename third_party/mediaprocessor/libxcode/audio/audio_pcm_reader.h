/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/

#ifndef __AUDIOPCMREADER_H__
#define __AUDIOPCMREADER_H__
#include "base/base_element.h"
#include "base/media_pad.h"
#include "base/mem_pool.h"
#include "base/ring_buffer.h"

#include "wave_header.h"
#include "audio_params.h"

#define AUDIO_OUTPUT_QUEUE_SIZE 32
#define INITIAL_PCM_FRAME_SIZE 960

class AudioPCMReader : public BaseElement {
public:
    AudioPCMReader(MemPool* mp, char* input);
    virtual ~AudioPCMReader();
    virtual bool Init(void* cfg, ElementMode element_mode);
    virtual int Recycle(MediaBuf &buf);
    virtual int HandleProcess();
    virtual int ProcessChain(MediaPad* pad, MediaBuf &buf);
    void Release(void);

private:
    AudioPCMReader();
    int ParseWAVHeader(AudioPayload *pIn);
    int PCMRead(AudioPayload* pIn, AudioPayload* pOut, bool isFirstPacket, unsigned int* DataConsumed);

protected:
    MemPool*                   m_pMempool;
    char*                      m_DecoderName;
    Info                       m_WaveInfoOut;
    WaveHeader                 m_WaveHeader;
    int                        m_nDataOffset;

    AudioPayload               m_Output[AUDIO_OUTPUT_QUEUE_SIZE];
    RingBuffer<AudioPayload, AUDIO_OUTPUT_QUEUE_SIZE>   m_output_queue;
    Stream*                    m_pDumpOutFile;
    unsigned long long         m_nDataSize;
};

#endif // __AUDIOPCMREADER_H__
