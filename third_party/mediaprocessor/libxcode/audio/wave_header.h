/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/

#ifndef __WAVEHEADER_H__
#define __WAVEHEADER_H__
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct Info
{
    unsigned int format_tag;      // 1 = PCM
    unsigned int sample_rate;     // f.e. 44100
    unsigned int resolution;      //  8, 16, 32 bits, etc
    unsigned int channels_number;
    unsigned int channel_mask;
};

struct WaveHeader
{
    unsigned int id_riff;
    unsigned int len_riff;

    unsigned int id_chuck;
    unsigned int fmt;
    unsigned int len_chuck;

    unsigned short type;
    unsigned short channels;
    unsigned int freq;
    unsigned int bytes;
    unsigned short align;
    unsigned short bits;

    unsigned int id_data;
    unsigned int len_data;

    unsigned int GetHeaderSize() { return sizeof(WaveHeader); }
    bool Populate(Info* m_info, unsigned long long m_data_size);
    int Interpret(unsigned char *wavInputData, Info* info, unsigned int data_size);
};

#endif // __WAVEHEADER_H__
