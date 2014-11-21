/****************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 ****************************************************************************/
#include "wave_header.h"

#ifdef _BIG_ENDIAN_
#define SWAP16(x) \
    (uint16_t) ((x) >> 8 | (x) << 8)

#define SWAP32(x) \
    (unsigned int)(((x) << 24) + \
    (((x)&0xff00) << 8) + \
    (((x) >> 8)&0xff00) + \
    ((unsigned int)(x) >> 24))

#define BIG_ENDIAN_SWAP16(x) SWAP16(x)
#define BIG_ENDIAN_SWAP32(x) SWAP32(x)
#else
#define BIG_ENDIAN_SWAP16(x) (x)
#define BIG_ENDIAN_SWAP32(x) (x)
#endif

const unsigned int ctRiff = 0x46464952;
const unsigned int ctWave = 0x45564157;
const unsigned int ctFmt  = 0x20746D66;
const unsigned int ctData = 0x61746164;

struct t_chunk {
    unsigned int m_ulId;
    unsigned int m_ulSize;
};

const unsigned int ctFmtSize = (2 + 2 + 4 + 4 + 2 + 2);

struct wav_header_ex {
    t_chunk riff_chunk;
    unsigned int wave_signature;
    t_chunk fmt_chunk;

    struct {
        uint16_t nFormatTag;
        uint16_t nChannels;
        unsigned int nSamplesPerSec;
        unsigned int nAvgBytesPerSec;
        uint16_t nBlockAlign;
        uint16_t wBitPerSample;
        uint16_t cbSize;
    } fmt;

    union {
        uint16_t wValidBitsPerSample; /* bits of precision */
        uint16_t wSamplesPerBlock;    /* valid if wBitsPerSample==0 */
        uint16_t wReserved;           /* If neither applies, set to zero. */
    };

    unsigned int dwChannelMask;
    uint16_t guid[16];

    t_chunk data_chunk;
};


bool WaveHeader::Populate(Info* m_info, unsigned long long m_data_size)
{
    int bits = 16;
    int nCh = m_info->channels_number;
    int CntFrameMulLenFrame = m_data_size / nCh / 2;

    this->id_riff = BIG_ENDIAN_SWAP32(0x46464952);
    this->len_riff = BIG_ENDIAN_SWAP32(sizeof(WaveHeader) + ((CntFrameMulLenFrame) << 1) * nCh - 8);

    this->id_chuck = BIG_ENDIAN_SWAP32(0x45564157);
    this->fmt = BIG_ENDIAN_SWAP32(0x20746D66);
    this->len_chuck = BIG_ENDIAN_SWAP32(0x00000010);

    this->type = BIG_ENDIAN_SWAP16(0x0001);
    this->channels = BIG_ENDIAN_SWAP16((uint16_t)nCh);
    this->freq = BIG_ENDIAN_SWAP32((unsigned int)(m_info->sample_rate));
    this->bytes = BIG_ENDIAN_SWAP32((m_info->sample_rate << 1) * nCh);
    this->align = BIG_ENDIAN_SWAP16((uint16_t)((nCh * bits) / 8));
    this->bits = BIG_ENDIAN_SWAP16((uint16_t)bits);

    this->id_data = BIG_ENDIAN_SWAP32(0x61746164);
    this->len_data = BIG_ENDIAN_SWAP32(((CntFrameMulLenFrame) << 1) * nCh);

    return true;
}

int WaveHeader::Interpret(unsigned char *wavInputData, Info* info, unsigned int data_size)
{
    if (!wavInputData) {
        return -1;
    }

    wav_header_ex header;
    t_chunk xChunk;
    unsigned int ulId = 0;

    unsigned int ulTemp = 0;
    unsigned int ulOffset = 0;
    int iFmtOk = 0;
    int iDataOk = 0;

    header.fmt.nFormatTag = 0;
    header.fmt.nSamplesPerSec = 0;
    header.fmt.wBitPerSample = 0;
    header.fmt.nChannels = 0;

    unsigned int *fishingPtr_u;
    unsigned char  *fishingPtr_8;

    unsigned int numberOfFourByteBlocks = 0;
    fishingPtr_8 = (unsigned char *) wavInputData;
    fishingPtr_u = (unsigned int *) wavInputData;
    xChunk.m_ulId = *(fishingPtr_u + numberOfFourByteBlocks++);
    xChunk.m_ulSize = *(fishingPtr_u + numberOfFourByteBlocks++);

    ulId = BIG_ENDIAN_SWAP32(xChunk.m_ulId);
    if (ulId != ctRiff) {
        return -2; // Data does not contain 'Riff' chunk !
    }

    ulTemp = *(fishingPtr_u + numberOfFourByteBlocks++);

    if (ulTemp != BIG_ENDIAN_SWAP32(ctWave)) {
        return -3; // Data does not contain 'Wave' signature !
    }

    ulOffset = numberOfFourByteBlocks * 4;

    for (;;) {
        if (iFmtOk && iDataOk)
            break;

        if (ulOffset >= data_size - 8) {
            return -4;
        }

        xChunk.m_ulId = *((unsigned int *)(fishingPtr_8 + ulOffset));
        ulOffset += 4;
        xChunk.m_ulSize = *((unsigned int *)(fishingPtr_8 + ulOffset));
        ulOffset += 4;

        ulId = BIG_ENDIAN_SWAP32(xChunk.m_ulId);

        switch (ulId) {
        case ctFmt://16 or 18
            memcpy(&header.fmt, fishingPtr_8 + ulOffset, ctFmtSize);
            ulOffset += ctFmtSize;
            if (18 == xChunk.m_ulSize) {
                ulOffset += 2;
            }

            iFmtOk = 1;
            break;
        case ctData:
            iDataOk = 1;
            break;
        default:
            break;
        }
    }

    info->format_tag = BIG_ENDIAN_SWAP16(header.fmt.nFormatTag);
    info->sample_rate = BIG_ENDIAN_SWAP32(header.fmt.nSamplesPerSec);//problems with these values
    info->resolution = BIG_ENDIAN_SWAP16(header.fmt.wBitPerSample);
    info->channels_number = BIG_ENDIAN_SWAP16(header.fmt.nChannels);
    info->channel_mask = 0;

    return ulOffset; // returns the offset into the data where the header ends
}
