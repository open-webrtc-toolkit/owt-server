/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/

#ifndef __AUDIOENCODER_H__
#define __AUDIOENCODER_H__

#include "base/base_element.h"
#include "base/media_common.h"
#include "base/stream.h"
#include "base/logger.h"
#include "wave_header.h"
#include "audio_params.h"
#include "umc_config.h"
#include "umc_aac_encoder.h"
#include "umc_mp3_encoder.h"
#include "umc_defs.h"
#include "umc_linear_buffer.h"

#define SHIFT_RIGHT(VALUE, SHIFT_AMOUNT)         (VALUE >> SHIFT_AMOUNT)
#define SHIFT_LEFT(VALUE, SHIFT_AMOUNT)          (VALUE << SHIFT_AMOUNT)

class AudioEncoder : public BaseElement
{
public:
    DECLARE_MLOGINSTANCE();
    AudioEncoder(Stream* st, unsigned char* name);
    virtual ~AudioEncoder();
    virtual bool Init(void *cfg, ElementMode element_mode);
    virtual int Recycle(MediaBuf &buf);
    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);
    virtual int HandleProcess();
#ifdef SUPPORT_SMTA
    AudioStreamInfo& getAudioStreamInfo() { return m_StreamInfo; }
    static void GenADTSHeader(Ipp8u *outPointer, int len,
               int profile, int sampling_freq_id, int ch_num);
#endif
private:
    AudioEncoder(const AudioEncoder &enc);

    AudioEncoder& operator = (const AudioEncoder&) {
        return *this;
    }

    void Release();
    int getConfigAudioSampleRate(int idx);
    int Encode(AudioPayload* pIn);
    int Reset();
protected:
    Stream*                       m_pStream;
    UMC::MediaData*               m_pOut;
    UMC::MediaData                m_InMediaData;
    UMC::AudioEncoder*            m_pUMCEncoder;
    UMC::MediaBuffer*             m_pInMediaBuffer;
    UMC::MediaBuffer*             m_pOutMediaBuffer;

    UMC::AudioEncoderParams*      m_pAudioCodecParams;
    UMC::MediaBufferParams*       m_pMediaBufferParams;

    size_t                        m_EncodedFrameSize;   // Size of an encoded frame, if encoder produces fixed size frames. 0 otherwise

    unsigned char*                m_Name;
    unsigned int                  m_CodecType;
    UMC::MP3EncoderParams         m_mp3enc_params;
    UMC::AACEncoderParams         m_aacenc_params;
    AudioStreamInfo               m_StreamInfo;
    bool                          m_bInitDone;
    bool                          m_firstPacketPending;
};

#endif // __AUDIOENCODERIMPL_H__
