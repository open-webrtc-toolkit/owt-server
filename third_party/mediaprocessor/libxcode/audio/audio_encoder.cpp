/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/
#ifdef ENABLE_AUDIO_CODEC
#include "audio_encoder.h"
#include "umc_mp3_encoder.h"
#include "base/trace.h"
#ifdef SUPPORT_SMTA
#include "aaccmn_adts.h"
#endif

DEFINE_MLOGINSTANCE_CLASS(AudioEncoder, "AudioEncoder");
AudioEncoder::AudioEncoder(Stream* st, unsigned char* name) : m_pStream(st), m_pUMCEncoder(NULL), m_pInMediaBuffer(NULL),
                               m_pOutMediaBuffer(NULL), m_pAudioCodecParams(NULL), m_pOut(NULL)
{
    m_pMediaBufferParams = NULL;
    m_EncodedFrameSize = 0;

    memset(&m_StreamInfo, 0, sizeof(m_StreamInfo));
    m_StreamInfo.codec_id = -1;

    m_bInitDone = false;
}

AudioEncoder::~AudioEncoder()
{
    Release();
}

void AudioEncoder::Release()
{
    if (m_pUMCEncoder) {
        delete m_pUMCEncoder;
        m_pUMCEncoder = NULL;
    }

    if (m_pMediaBufferParams) {
        delete m_pMediaBufferParams;
        m_pMediaBufferParams = NULL;
    }

    if (m_pOut) {
        delete m_pOut;
        m_pOut = NULL;
    }

    if (m_pInMediaBuffer) {
        delete m_pInMediaBuffer;
        m_pInMediaBuffer = NULL;
    }

    if (m_pOutMediaBuffer) {
        delete m_pOutMediaBuffer;
        m_pOutMediaBuffer = NULL;
    }

}

bool AudioEncoder::Init(void* cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    sAudioParams* audio_params = reinterpret_cast<sAudioParams *> (cfg);
    m_CodecType = audio_params->nCodecType;
    audio_params->nBitRate = 128;//TODO: check the bitrate configuration for the mp3.
    if (m_CodecType == STREAM_TYPE_AUDIO_MP3) {
        m_StreamInfo.codec_id = 1;
        m_pAudioCodecParams = reinterpret_cast<UMC::AudioEncoderParams*>(&m_mp3enc_params);
    } else if (m_CodecType == STREAM_TYPE_AUDIO_AAC) {
        m_StreamInfo.codec_id = 2;
        m_pAudioCodecParams = reinterpret_cast<UMC::AudioEncoderParams*>(&m_aacenc_params);
    } else {
        MLOG_ERROR(" Not support the audio format\n");
    }

    m_pAudioCodecParams->m_info.iBitrate = audio_params->nBitRate * 1000;
    APP_TRACE_INFO("bit rate is %d\n", m_pAudioCodecParams->m_info.iBitrate);

    if (audio_params->nSampleRate > 0) {
        m_pAudioCodecParams->m_info.audioInfo.m_iSampleFrequency = getConfigAudioSampleRate(audio_params->nSampleRate);
        APP_TRACE_INFO("sample rate is %d\n", m_pAudioCodecParams->m_info.audioInfo.m_iSampleFrequency);
        if (m_pAudioCodecParams->m_info.audioInfo.m_iSampleFrequency <= 0) {
            MLOG_ERROR("Invalid Audio Sample Rate (%d)\n", audio_params->nSampleRate);
        }
    }

    switch (m_StreamInfo.codec_id) {
    case 1:
        //enc_EncodedFrameSize = 1; // MP3 in RTP uses constant frame size. // But Live555 output takes care of that.
        m_EncodedFrameSize = 0; // Not needed when writing to Muxer
        m_mp3enc_params.mode = 0;  // CBR
        m_mp3enc_params.stereo_mode = UMC_MPA_LR_STEREO;
        break;
    case 2:
        m_EncodedFrameSize = 0; // Is purely dynamic for AAC
        break;
    default:
        break;
    }

    return true;
}

int AudioEncoder::Recycle(MediaBuf &buf)
{
    //We don't have any SID resource, directly return
    return 0;
}

int AudioEncoder::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    AudioPayload pIn;
    pIn.payload = buf.payload;
    pIn.payload_length = buf.payload_length;
    pIn.isFirstPacket = buf.isFirstPacket;

    APP_TRACE_INFO("encoder will encode the payload %p\n", &pIn);
    if (NULL == pIn.payload) {
        APP_TRACE_INFO("input audio payload is empty, audio encoder get eof\n");
        m_pStream->SetEndOfStream();
        return 0;
    }

    if (Encode(&pIn) != 0) {
        ReturnBuf(buf);
        return -1;
    }

    return 0;
}

////
// This method is intended to be called several times, each time it should receive 1 wav frame via the input
// PayloadInfo. It provides the encoded output, in the form of UMC::MediaData, to its configured AudioOutput.
//
// Note: Contrary to what some variable names could make think, this method DOES NOT READ
// OR ACCEPT file data directly, but it is expected that the input frame data will be in wav format.
//
int AudioEncoder::Encode(AudioPayload* pIn)
{
    UMC::Status     enc_sts;
    Ipp32s          enc_err = 0;
    int             outPackets = 0;
#ifdef AUDIO_ENC_TIMING
    vm_tick         t_start, t_end, t_dur;
#endif

    if (pIn->isFirstPacket) {
        m_firstPacketPending = true; // Need to remember because with some codecs, we'll need multiple input frames before we can get a first encoded frame
        int ret = Reset();
        if (ret != 0) {
            MLOG_ERROR("Reset() failed (%d)\n", ret);
            return ret;
        }
        MLOG_INFO("is first packet arrived\n");
    }

    // Lets populate the inputWaveInfo object with some parameters from the wav input data (remember this is not receiving a wav file JUST WAV FORMATTED DATA!!!!)
    WaveHeader inputWaveHeader;
    Info inputWaveInfo;
    int dataOffset = inputWaveHeader.Interpret(pIn->payload, &inputWaveInfo, pIn->payload_length);
    if (dataOffset <= 0) {
        MLOG_ERROR("Encode: Invalid or unsupported header format!\n");
        return -1;
    }

    unsigned int frameSize;
    if (pIn->payload_length > dataOffset) {
        frameSize = pIn->payload_length - dataOffset;
    } else {
        MLOG_WARNING(" input payload size (%d) is lower than data dataOffset(%d)\n",
                     pIn->payload_length, dataOffset);
        frameSize = 0;
    }
    APP_TRACE_INFO("%s: frameSize=%d (payload_size=%d) WAV dataOffset=%d isFirstPacket=%d\n",
                __FUNCTION__, frameSize, pIn->payload_length, dataOffset, pIn->isFirstPacket);

#ifdef DUMP_ENC_INPUT_FRAME
    if (m_pDumpInFile) {
        fwrite(pIn->payload + dataOffset, 1, frameSize, m_pDumpInFile);
        fflush(m_pDumpInFile);
    }
#endif

    APP_TRACE_DEBUG("%s: inputWaveInfo chans=%d res=%d s_rate=%d\n",
                    __FUNCTION__,
                    inputWaveInfo.channels_number,
                    inputWaveInfo.resolution,
                    inputWaveInfo.sample_rate);
    if (m_bInitDone == false) {
        m_pAudioCodecParams->m_info.streamType = UMC::PCM_AUDIO;  // Input is wav so lets set the codec input stream type
        m_pAudioCodecParams->m_info.audioInfo.m_iSampleFrequency = inputWaveInfo.sample_rate;
        m_pAudioCodecParams->m_info.audioInfo.m_iBitPerSample = inputWaveInfo.resolution;
        m_pAudioCodecParams->m_info.audioInfo.m_iChannels = inputWaveInfo.channels_number;
        m_pAudioCodecParams->m_info.audioInfo.m_iChannelMask = inputWaveInfo.channel_mask;
        m_pAudioCodecParams->m_pData = NULL;
        APP_TRACE_INFO("%s: Initializing Encoder with chans=%d res=%d s_rate=%d chan_mask=0x%x\n", __FUNCTION__,
                                m_pAudioCodecParams->m_info.audioInfo.m_iChannels,
                                m_pAudioCodecParams->m_info.audioInfo.m_iBitPerSample,
                                m_pAudioCodecParams->m_info.audioInfo.m_iSampleFrequency,
                                m_pAudioCodecParams->m_info.audioInfo.m_iChannelMask);
        enc_sts = m_pUMCEncoder->Init(m_pAudioCodecParams); // Check the encoder init failed.
        if (enc_sts != UMC::UMC_OK) {
            MLOG_ERROR("ERROR - AudioEncoder audio codec Init failed\n");
            return UMC::UMC_ERR_FAILED;
        }
        enc_sts = m_pUMCEncoder->GetInfo(m_pAudioCodecParams);
        if (enc_sts < UMC::UMC_OK) {
            MLOG_ERROR("ERROR - AudioEncoder audio codec GetInfo failed\n");
            return UMC::UMC_ERR_FAILED;
        }

        if (m_EncodedFrameSize && m_pOutMediaBuffer) {
            m_pMediaBufferParams->m_numberOfFrames = 10;
            m_pMediaBufferParams->m_prefInputBufferSize = m_pAudioCodecParams->m_iSuggestedOutputSize;
            m_pMediaBufferParams->m_prefOutputBufferSize = m_pAudioCodecParams->m_iSuggestedOutputSize;

            APP_TRACE_INFO("%s: Allocating OutMediaBuffer with prefInputBufferSize=%u prefOutputBufferSize=%u\n",
                         __FUNCTION__,
                         (unsigned int)m_pMediaBufferParams->m_prefInputBufferSize,
                         (unsigned int)m_pMediaBufferParams->m_prefOutputBufferSize );
            enc_sts = m_pOutMediaBuffer->Init(m_pMediaBufferParams);
            if (enc_sts != UMC::UMC_OK) {
                MLOG_ERROR("ERROR - AudioEncoder OutMediaBuffer Init failed\n");
                return UMC::UMC_ERR_FAILED;
            }
            m_pOut = new UMC::MediaData();
        } else {
            // Assign m_pOut a new memory block to hold the transcode output data
            m_pOut = new UMC::MediaData();
            m_pOut->Alloc(m_pAudioCodecParams->m_iSuggestedOutputSize);
        }
        if (NULL == m_pOut) {
            MLOG_ERROR("ERROR - AudioEncoder Output buffer memory allocation error\n");
            return -1;
        }
        APP_TRACE_INFO("%s: Output MediaData allocated (buffer=%p Size=%d)\n",
                    __FUNCTION__, m_pOut->GetBufferPointer(), (int)m_pOut->GetBufferSize());

        m_pMediaBufferParams->m_numberOfFrames = 10;
        if (frameSize > m_pAudioCodecParams->m_iSuggestedInputSize) {
            m_pMediaBufferParams->m_prefInputBufferSize = frameSize;
            m_pMediaBufferParams->m_prefOutputBufferSize = frameSize;
        } else {
            m_pMediaBufferParams->m_prefInputBufferSize = m_pAudioCodecParams->m_iSuggestedInputSize;
            m_pMediaBufferParams->m_prefOutputBufferSize = m_pAudioCodecParams->m_iSuggestedInputSize;
        }
        APP_TRACE_INFO("%s: Allocating InMediaBuffer with prefInputBufferSize=%u prefOutputBufferSize=%u\n",
                     __FUNCTION__,
                     (unsigned int)m_pMediaBufferParams->m_prefInputBufferSize,
                     (unsigned int)m_pMediaBufferParams->m_prefOutputBufferSize);
        enc_sts = m_pInMediaBuffer->Init(m_pMediaBufferParams);
        if (enc_sts != UMC::UMC_OK) {
            MLOG_ERROR("ERROR - AudioEncoder InMediaBuffer Init failed\n");
            return UMC::UMC_ERR_FAILED;
        }

#ifdef AUDIO_ENC_TIMING
        m_TimeFreq = vm_time_get_frequency();
#endif
    }

#ifdef AUDIO_ENC_TIMING
    t_end = 0;
    t_dur = 0;
#endif

    enc_sts = m_pInMediaBuffer->LockInputBuffer(&m_InMediaData);
    APP_TRACE_DEBUG("%s: InMediaBuffer.LockInputBuffer -> sts=%d data.size=%d buf.size=%d buf.ptr=%p data.ptr=%p\n",
                __FUNCTION__, enc_sts, (int)m_InMediaData.GetDataSize(), (int)m_InMediaData.GetBufferSize(),
                m_InMediaData.GetBufferPointer(), m_InMediaData.GetDataPointer() );
    if (enc_sts == UMC::UMC_OK) {
        size_t needSize = m_pMediaBufferParams->m_prefOutputBufferSize; // enc_inMediaBuffer.GetBufferSize();

        if (inputWaveInfo.resolution == 16) {
            needSize = needSize & (~1);
        } else if (inputWaveInfo.resolution == 24) {
            needSize = (needSize / 3) * 3;
        } else if (inputWaveInfo.resolution == 32) {
            needSize = needSize & (~3);
        } else {
            MLOG_WARNING("Audio Encoder doesn't support %dbit input.\n", inputWaveInfo.resolution);
        }
        APP_TRACE_INFO("%s: needSize=%d\n", __FUNCTION__, (int)needSize);

        // Transform 24 and 32 bits input to 16 bits one

        Ipp16s *dest16 = (Ipp16s*)m_InMediaData.GetDataPointer();
        unsigned int ii;

        if (inputWaveInfo.resolution == 24) {
            Ipp8u *in8 = (Ipp8u*)(pIn->payload + dataOffset);

            for (ii = 0; ii < frameSize/3; ii++) {

                #ifdef _BIG_ENDIAN_
                Ipp32s tmp = ( SHIFT_RIGHT(SHIFT_LEFT(((Ipp32s)in8[0]), 24), 8) +
                SHIFT_LEFT(((Ipp32s)in8[1]), 8) +
                ((Ipp32s)in8[2]));
                # else
                Ipp32s tmp = (SHIFT_RIGHT(SHIFT_LEFT(((Ipp32s)in8[2]), 24), 8) +
                    SHIFT_LEFT(((Ipp32s)in8[1]), 8) +
                    ((Ipp32s)in8[0]));
                #endif

                dest16[ii] = (Ipp16s)(SHIFT_RIGHT((tmp + 128),8));
                in8 += 3;
            }

            m_InMediaData.SetDataSize(2*(frameSize/3));
        } else if (inputWaveInfo.resolution == 32) {
            Ipp32s *in32 = (Ipp32s*)(pIn->payload + dataOffset);
            for (ii = 0; ii < frameSize/4; ii++) {
                dest16[ii] = (Ipp16s)(SHIFT_RIGHT((in32[ii] + 32768),16));
            }

            m_InMediaData.SetDataSize(2*(frameSize/4));
        } else if (inputWaveInfo.resolution == 16) {
            memcpy(dest16, pIn->payload + dataOffset, frameSize);

            m_InMediaData.SetDataSize(frameSize);
        }

        m_InMediaData.m_fPTSStart = -1;  // This will let the encoder derive the PTS from the number of incoming sample of raw PCM

        UMC::Status in_sts;
        size_t data_sz = m_InMediaData.GetDataSize();
        in_sts = m_pInMediaBuffer->UnLockInputBuffer(&m_InMediaData, UMC::UMC_OK  /* UMC::UMC_ERR_END_OF_STREAM */);
        APP_TRACE_INFO("%s: InMediaBuffer.UnLockInputBuffer() adding dataSize=%d -> sts=%d\n",
                    __FUNCTION__, (int)data_sz, in_sts);
    }

    m_bInitDone = true;

    while (enc_sts == UMC::UMC_OK) {

        // Lock output of InMediaBuffer (for data into codec)
        UMC::Status in_sts = m_pInMediaBuffer->LockOutputBuffer(&m_InMediaData);
        APP_TRACE_INFO("%s: InMediaBuffer.LockOutputBuffer -> sts=%d inData.size=%d inData.bufSz=%d\n",
                    __FUNCTION__, in_sts, (int)m_InMediaData.GetDataSize(), (int)m_InMediaData.GetBufferSize());

        // And - if applicable - lock input of OutMediaBuffer (for data out of codec)
        UMC::Status out_sts = UMC::UMC_OK;
        if (m_pOutMediaBuffer) {
            out_sts = m_pOutMediaBuffer->LockInputBuffer(m_pOut);
            APP_TRACE_DEBUG("%s: OutMediaBuffer.LockInputBuffer() -> sts=%d Buffer(%p:%d) Data(%p:%d)\n", 
                        __FUNCTION__, out_sts, m_pOut->GetBufferPointer(), (int)m_pOut->GetBufferSize(),
                        m_pOut->GetDataPointer(), (int)m_pOut->GetDataSize());
            if (out_sts != UMC::UMC_OK) {
                APP_TRACE_INFO("%s: OutMediaBuffer.LockInputBuffer -> sts=%d\n", __FUNCTION__, out_sts);
            }
        }

        if (out_sts == UMC::UMC_OK) {
            if (in_sts != UMC::UMC_OK) {
                if (frameSize == 0) {
                    APP_TRACE_INFO("%s: InMediaBuffer.LockOutputBuffer() -> sts=%d and frameSize=0. Last frames...\n",                             __FUNCTION__, in_sts);
                    // Last frames of encoder
#ifdef AUDIO_ENC_TIMING
                    t_start = vm_time_get_tick();
#endif
                    enc_sts = m_pUMCEncoder->GetFrame(NULL, m_pOut);
#ifdef AUDIO_ENC_TIMING
                    t_dur = (vm_time_get_tick() - t_start);
#endif
                } else {
                    APP_TRACE_INFO("%s: InMediaBuffer.LockOutputBuffer() -> sts=%d - Partial input\n",
                                __FUNCTION__, in_sts);
                    // Partial input packet. Need to read more!
                    enc_sts = in_sts;
                    enc_err = 0;
                }
            } else {
                APP_TRACE_INFO("%s: m_pOut before GetFrame(): buf=%p data=%p size=%d\n", __FUNCTION__,
                            m_pOut->GetBufferPointer(), m_pOut->GetDataPointer(), (int)m_pOut->GetDataSize());

                size_t dataSz = m_InMediaData.GetDataSize(); // Total data available

#ifdef AUDIO_ENC_TIMING
                t_start = vm_time_get_tick();
#endif
                enc_sts = m_pUMCEncoder->GetFrame(&m_InMediaData, m_pOut);
#ifdef AUDIO_ENC_TIMING
                t_dur = (vm_time_get_tick() - t_start);
#endif

                size_t usedData = dataSz - m_InMediaData.GetDataSize();
                APP_TRACE_INFO("%s: Encoder used %d bytes from inData (remains %d)\n", __FUNCTION__,
                            (int)usedData, (int)m_InMediaData.GetDataSize() );

#ifdef DUMP_ENC_INPUT_FRAME
                void *data = m_InMediaData.GetDataPointer();
                if (enc_sts == UMC::UMC_OK && m_pDumpInFile) {
                    fwrite(data, 1, usedData, m_pDumpInFile);
                    fflush(m_pDumpInFile);
                }
#endif

                dataSz = m_InMediaData.GetDataSize();
                in_sts = m_pInMediaBuffer->UnLockOutputBuffer(&m_InMediaData);
                APP_TRACE_INFO("%s: InMediaBuffer.UnLockOutputBuffer with remaining inData.size=%d -> sts=%d\n",
                            __FUNCTION__, (int)dataSz, in_sts);
                if (in_sts != UMC::UMC_OK) {
                    APP_TRACE_INFO("%s: InMediaBuffer.UnLockOutputBuffer -> sts=%d\n", __FUNCTION__, in_sts);
                }
            }

            if ((enc_sts == UMC::UMC_OK) && (m_StreamInfo.sample_rate == 0)) {
                // Save stream info in m_StreamInfo so we can later provide it...
                m_StreamInfo.sample_rate    = m_pAudioCodecParams->m_info.audioInfo.m_iSampleFrequency;
                m_StreamInfo.channels       = m_pAudioCodecParams->m_info.audioInfo.m_iChannels;
                m_StreamInfo.bit_rate       = m_pAudioCodecParams->m_info.iBitrate;
                m_StreamInfo.sample_fmt     = 1;//TODO: to define AV_SAMPLE_FMT_S16; //AV_SAMPLE_FMT_NONE; // AV_SAMPLE_FMT_FLTP; //AV_SAMPLE_FMT_S16;
                m_StreamInfo.channel_mask   = m_pAudioCodecParams->m_info.audioInfo.m_iChannelMask;
            }

            if (m_pOutMediaBuffer) {
                out_sts = m_pOutMediaBuffer->UnLockInputBuffer(m_pOut);
                APP_TRACE_INFO("%s: OutMediaBuffer.UnLockInputBuffer() -> sts=%d Buffer(%p:%d) Data(%p:%d)\n",
                            __FUNCTION__, out_sts, m_pOut->GetBufferPointer(), (int)m_pOut->GetBufferSize(),
                            m_pOut->GetDataPointer(), (int)m_pOut->GetDataSize());
                if (out_sts != UMC::UMC_OK) {
                    APP_TRACE_INFO("%s: OutMediaBuffer.LockInputBuffer -> sts=%d\n", __FUNCTION__, out_sts);
                }
            }
        }

        out_sts = UMC::UMC_OK;
        if (m_pOutMediaBuffer) {
            out_sts = m_pOutMediaBuffer->LockOutputBuffer(m_pOut);
            APP_TRACE_DEBUG("%s: OutMediaBuffer.LockOutputBuffer() -> sts=%d Buffer(%p:%d) Data(%p:%d)\n",
                        __FUNCTION__, out_sts, m_pOut->GetBufferPointer(), (int)m_pOut->GetBufferSize(),
                        m_pOut->GetDataPointer(), (int)m_pOut->GetDataSize());
            if (out_sts != UMC::UMC_OK) {
                APP_TRACE_ERROR("%s: OutMediaBuffer.LockOutputBuffer -> sts=%d\n", __FUNCTION__, out_sts);
            }
        }
        APP_TRACE_INFO("%s: enc_sts=%d output data.size=%d (buf.size=%d)\n",
                   __FUNCTION__, enc_sts, (int)m_pOut->GetDataSize(), (int)m_pOut->GetBufferSize());

        if (out_sts == UMC::UMC_OK) {
            if ( (enc_sts == UMC::UMC_OK) && (m_pOut->GetDataSize() > 0) ) {  // Got encoded data
#ifdef AUDIO_ENC_TIMING
                t_end += t_dur;
#endif
                m_pStream->WriteBlock(m_pOut->GetDataPointer(), m_pOut->GetDataSize());
                outPackets++;
            } else if (enc_sts == UMC::UMC_ERR_NOT_ENOUGH_BUFFER) {
                if (m_pOutMediaBuffer) {
                    enc_err = -1;
                } else {
                    delete m_pOut;

                    enc_sts = m_pUMCEncoder->GetInfo(m_pAudioCodecParams);
                    APP_TRACE_INFO("%s: ADJUSTING OUTPUT BUFFER SIZE TO %d bytes\n",
                                __FUNCTION__, m_pAudioCodecParams->m_iSuggestedOutputSize);
                    m_pOut = new UMC::MediaData();
                    if (NULL == m_pOut) {
                        MLOG_ERROR("ERROR - AudioEncoder output buffer memory allocation error\n");
                        return -1;
                    }
                    enc_sts = m_pOut->Alloc(m_pAudioCodecParams->m_iSuggestedOutputSize);
                    if (UMC::UMC_OK != enc_sts) {
                        MLOG_ERROR("ERROR - AudioEncoder output buffer allocation error\n");
                        return -1;
                    }
                }
            } else if (enc_sts == UMC::UMC_ERR_NOT_ENOUGH_DATA) {
                APP_TRACE_INFO("%s: enc_sts=UMC_ERR_NOT_ENOUGH_DATA && outPackets=%d\n", __FUNCTION__, outPackets);
                enc_err = (outPackets > 0) ? 0 : -1;
                // Will exit as enc_sts != UMC_OK
            }

            if (m_pOutMediaBuffer) {
                out_sts = m_pOutMediaBuffer->UnLockOutputBuffer(m_pOut);
                APP_TRACE_DEBUG("%s: OutMediaBuffer.UnLockOutputBuffer() -> sts=%d Buffer(%p:%d) Data(%p:%d)\n",
                            __FUNCTION__, out_sts, m_pOut->GetBufferPointer(), (int)m_pOut->GetBufferSize(),
                            m_pOut->GetDataPointer(), (int)m_pOut->GetDataSize());
            }
        }

    } // while (enc_sts == UMC::UMC_OK) {

    return enc_err;
}
int AudioEncoder::getConfigAudioSampleRate(int idx)
{
    //Sampling Frequencies
    //http://wiki.multimedia.cx/index.php?title=MPEG-4_Audio#Sampling_Frequencies
    const unsigned int t_srMap[] = {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025,  8000,
        7350
    };
    const int numValidSrValues = sizeof(t_srMap)/sizeof(t_srMap[0]);

    if ( (idx < 0) || (idx >= numValidSrValues) ) {
        MLOG_ERROR("### ERROR: sample_rate %d is out of range (0-%d)\n", idx, (numValidSrValues-1));
        return -1;
    }

    return t_srMap[idx];
}

int AudioEncoder::HandleProcess()
{
    return 0;
}

int AudioEncoder::Reset()
{
    m_bInitDone = false;

    Release();

    if (m_StreamInfo.codec_id == 1) {
        m_pUMCEncoder = new UMC::MP3Encoder();
    } else if (m_StreamInfo.codec_id == 2) {
        m_pUMCEncoder = new UMC::AACEncoder();
    } else {
        MLOG_ERROR("ERROR - Unknown codec value (%d)\n", m_StreamInfo.codec_id);
        return -1;
    }

    if (m_pUMCEncoder == NULL) {
        MLOG_ERROR("ERROR - Can't create audio codec object\n");
        return -1;
    }

    // NOTE: A LinearBuffer is meant to be used from different threads for input and output.
    //       It has necessary synchronization constructs for this.
    //       Hence is possibly overhead when all is done from one thread...
    m_pInMediaBuffer = DynamicCast < UMC::MediaBuffer > (new UMC::LinearBuffer);
    if (NULL == m_pInMediaBuffer) {
        MLOG_ERROR("ERROR - AudioEncoder InMediaBuffer memory allocation error\n");
        return -1;
    }

    if (m_EncodedFrameSize) {  // Forcing fixed size output frames => need to use a real MediaBuffer at output as well
        MLOG_INFO(" Encoder using an output LinearBuffer for FrameSize=%d\n", (int)m_EncodedFrameSize);
        m_pOutMediaBuffer = DynamicCast < UMC::MediaBuffer > (new UMC::LinearBuffer);
        if (NULL == m_pOutMediaBuffer) {
            MLOG_ERROR("ERROR - AudioEncoder OutMediaBuffer memory allocation error\n");
            return -1;
        }
    }

    m_pMediaBufferParams = DynamicCast < UMC::MediaBufferParams > (new UMC::MediaBufferParams);
    if (NULL == m_pMediaBufferParams) {
        APP_TRACE_INFO("ERROR - AudioEncoder media buffer params memory allocation error\n");
        return -1;
    }

#ifdef DUMP_ENC_INPUT_FRAME
    static const char* outputFileName = "audio_enc_input.raw";
    m_pDumpInFile = fopen(outputFileName, "wb");
    if (NULL == m_pDumpInFile) {
        MLOG_ERROR("Can't create output file '%s'", outputFileName);
    }
#endif

    return 0;
}

#ifdef SUPPORT_SMTA
extern "C" {
void enc_adts_header(sAdts_fixed_header *pFixedHeader,
                    sAdts_variable_header *pVarHeader,
                    sBitsreamBuffer *pBS);
}
void AudioEncoder::GenADTSHeader(Ipp8u *outPointer, int len,
       int profile, int sampling_freq_id, int ch_num)
{
    sAdts_fixed_header    adts_fixed_header;
    sAdts_variable_header adts_variable_header;
    sBitsreamBuffer       BS;
    sBitsreamBuffer*      pBS = &BS;

    INIT_BITSTREAM(pBS, outPointer)
    // Put to bistream ADTS header !
    // Fixed header.
    adts_fixed_header.ID                        = profile >= 3? 0:1;
    adts_fixed_header.Layer                     = 0;
    adts_fixed_header.protection_absent         = 1;
    adts_fixed_header.Profile                   = profile;
    adts_fixed_header.sampling_frequency_index  = sampling_freq_id;
    adts_fixed_header.private_bit               = 0;
    adts_fixed_header.channel_configuration     = ch_num;
    adts_fixed_header.original_copy             = 0;
    adts_fixed_header.Home                      = 0;
    if (8 == ch_num)
        adts_fixed_header.channel_configuration = 7;

    // Variable header !
    adts_variable_header.copyright_identification_bit   = 0;
    adts_variable_header.copyright_identification_start = 0;
    adts_variable_header.aac_frame_length               = len;
    adts_variable_header.adts_buffer_fullness           = 0x7FF;
    adts_variable_header.no_raw_data_blocks_in_frame    = 0;

    enc_adts_header(&adts_fixed_header, &adts_variable_header, pBS);
    if (adts_fixed_header.protection_absent == 0)
    {
        PUT_BITS(pBS,0,16); /* for CRC */
    }

    SAVE_BITSTREAM(pBS)
    Byte_alignment(pBS);

    Ipp32s  headerBytes = 0;
    GET_BITS_COUNT(pBS, headerBytes)
    headerBytes >>= 3;

    adts_variable_header.aac_frame_length = headerBytes + len;

    enc_adts_header(&adts_fixed_header, &adts_variable_header, pBS);
    SAVE_BITSTREAM(pBS)
}
#endif
#endif
