/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/

#ifndef __AUDIODECODER_H__
#define __AUDIODECODER_H__
#include "base/base_element.h"
#include "base/media_pad.h"
#include "base/mem_pool.h"
#include "base/ring_buffer.h"
#include "wave_header.h"
#include "audio_params.h"
#include "umc_media_data.h"
#include "umc_aac_decoder.h"
#include "umc_mp3_decoder.h"

class AudioDecoder : public BaseElement {
public:
    AudioDecoder(MemPool* mp, char* input);
    virtual ~AudioDecoder();
    virtual bool Init(void* cfg, ElementMode element_mode);
    virtual int Recycle(MediaBuf &buf);
    virtual int HandleProcess();
    virtual int ProcessChain(MediaPad* pad, MediaBuf &buf);
    void Release(void);
private:
    AudioDecoder(const AudioDecoder &dec);

    AudioDecoder& operator = (const AudioDecoder&) {
        return *this;
    }

    int Decode(AudioPayload* pIn, bool isFirstPacket, unsigned int* DataConsumed);
    bool FindAACFrame(unsigned char* buf, unsigned int buf_size, unsigned int* frame_size);

protected:
    MemPool*                   m_pMempool;
    MemPool*                   m_pRawDataMP;
    UMC::MediaData*            m_pOut;
    UMC::MediaData             m_inMediaBuffer;
    UMC::AudioDecoder*         m_pUMCDecoder;
    UMC::AudioDecoderParams*   m_pAudioDecoderParams;

    char*                      m_DecoderName;
    int                        m_codec_id;
    UMC::AACDecoderParams      m_aacdec_params;
    UMC::MP3DecoderParams      m_mp3dec_params;
    int                        m_NumFramesToProcess;
    Info                       m_WaveInfoOut;

    AudioPayload               m_Output[AUDIO_OUTPUT_QUEUE_SIZE];
    RingBuffer<AudioPayload, AUDIO_OUTPUT_QUEUE_SIZE>   m_output_queue;
    bool                       m_dump;
    FILE*                      m_pDumpOutFile;
    bool                       m_bHaveDecoderValuesBeenInitialised;  // flag that can be used to control the initialisation of the values in Method2

    bool                       m_bResetFlag;
    bool                       m_bFirstPass;
};


#endif // __AUDIODECODERIMPL_H__
