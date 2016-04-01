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

private:
    void Release();
    AudioPCMReader();
    int ParseWAVHeader(AudioPayload *payload_in);
    int PCMRead(AudioPayload* payload_in, AudioPayload* payload_out, bool is_first_packet, unsigned int* data_consumed);

protected:
    MemPool*                   mem_pool_;
    Info                       wav_info_;
    WaveHeader                 wav_header_;
    int                        data_offset_;

    AudioPayload               output_payload_[AUDIO_OUTPUT_QUEUE_SIZE];
    RingBuffer<AudioPayload, AUDIO_OUTPUT_QUEUE_SIZE>   output_queue_;
    Stream*                    dump_out_file_;
    unsigned long long         total_data_size_;
    short                      g711_table_[256];
    StreamType                 dec_type_;
};

#endif // __AUDIOPCMREADER_H__
