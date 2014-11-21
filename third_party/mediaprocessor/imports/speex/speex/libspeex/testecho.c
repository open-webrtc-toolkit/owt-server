#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"


#define NN 128
#define TAIL 1024

//Define WAV File Header
typedef struct tagWAVFLIEHEAD
{
    unsigned char    RIFFNAME[4];
    unsigned int    nRIFFLength;
    unsigned char    WAVNAME[4];
    unsigned char    FMTNAME[4];
    unsigned int    nFMTLength;
    unsigned short    nAudioFormat;
    unsigned short    nChannelNumber;
    unsigned int    nSampleRate;
    unsigned int    nBytesPerSecond;
    unsigned short    nBytesPerSample;
    unsigned short    nBitsPerSample;
    unsigned char    DATANAME[4];
    unsigned int    nDataLength;
} WAVFLIEHEAD;

//Write wav header to file
int WriteWavHeader(FILE *fpWAV, unsigned short nBitsPerSample, unsigned int nSampleRate, unsigned short nChannelNumber, int nFileLen)
{
    WAVFLIEHEAD WAVFileHeader;

    int nHeaderSize = sizeof(WAVFileHeader);

    WAVFileHeader.RIFFNAME[0] = 'R';
    WAVFileHeader.RIFFNAME[1] = 'I';
    WAVFileHeader.RIFFNAME[2] = 'F';
    WAVFileHeader.RIFFNAME[3] = 'F';

    WAVFileHeader.WAVNAME[0] = 'W';
    WAVFileHeader.WAVNAME[1] = 'A';
    WAVFileHeader.WAVNAME[2] = 'V';
    WAVFileHeader.WAVNAME[3] = 'E';

    WAVFileHeader.FMTNAME[0] = 'f';
    WAVFileHeader.FMTNAME[1] = 'm';
    WAVFileHeader.FMTNAME[2] = 't';
    WAVFileHeader.FMTNAME[3] = 0x20;
    WAVFileHeader.nFMTLength = 16;
    WAVFileHeader.nAudioFormat = 0x01; //pcm format

    WAVFileHeader.DATANAME[0] = 'd';
    WAVFileHeader.DATANAME[1] = 'a';
    WAVFileHeader.DATANAME[2] = 't';
    WAVFileHeader.DATANAME[3] = 'a';
    WAVFileHeader.nBitsPerSample = nBitsPerSample;
    WAVFileHeader.nBytesPerSample = nBitsPerSample >> 3;
    WAVFileHeader.nSampleRate = nSampleRate;
    WAVFileHeader.nBytesPerSecond = nSampleRate * WAVFileHeader.nBytesPerSample;
    WAVFileHeader.nChannelNumber = nChannelNumber;
    WAVFileHeader.nRIFFLength = nFileLen - 8;
    WAVFileHeader.nDataLength = nFileLen - nHeaderSize;

    int nWrite = fwrite(&WAVFileHeader, 1, nHeaderSize, fpWAV);
    if (nWrite != nHeaderSize) {
        printf("[Error]: Failed to write wav header!\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    FILE *echo_fd, *ref_fd, *e_fd;
    short echo_buf[NN], ref_buf[NN], e_buf[NN];
    SpeexEchoState *st;
    SpeexPreprocessState *den;
    int sampleRate = 8000;
    WAVFLIEHEAD SrcWAVHeader;

    if (argc != 3) {
        printf("Usage: testecho <mic_signal_file_name> <speaker_signal_file_name> <output_file_name>\n");
        printf("Note: Only 16bit audio with same sample frequency supported currently.\n");
        return -1;
    }
    echo_fd = fopen(argv[2], "rb");
    ref_fd  = fopen(argv[1],  "rb");
    e_fd    = fopen(argv[3], "wb");

    fread(&SrcWAVHeader, sizeof(WAVFLIEHEAD), 1, echo_fd);
    printf("[Info]: src_file %s: channel_num %d, bits_per_sample %d, sample_rate %d\n", \
            argv[2], SrcWAVHeader.nChannelNumber, SrcWAVHeader.nBitsPerSample, SrcWAVHeader.nSampleRate);

    fread(&SrcWAVHeader, sizeof(WAVFLIEHEAD), 1, ref_fd);
    printf("[Info]: src_file %s: channel_num %d, bits_per_sample %d, sample_rate %d\n", \
            argv[1], SrcWAVHeader.nChannelNumber, SrcWAVHeader.nBitsPerSample, SrcWAVHeader.nSampleRate);

    //Write an initial wav hearder with default parameters to the destination file
    int nReturn = WriteWavHeader(e_fd, SrcWAVHeader.nBitsPerSample, SrcWAVHeader.nSampleRate, SrcWAVHeader.nChannelNumber, 0xFF);

    st = speex_echo_state_init(NN, TAIL);
    den = speex_preprocess_state_init(NN, sampleRate);
    speex_echo_ctl(st, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_STATE, st);

    while (!feof(ref_fd) && !feof(echo_fd)) {
        fread(ref_buf, sizeof(short), NN, ref_fd);
        fread(echo_buf, sizeof(short), NN, echo_fd);
        speex_echo_cancellation(st, ref_buf, echo_buf, e_buf);
        speex_preprocess_run(den, e_buf);
        fwrite(e_buf, sizeof(short), NN, e_fd);
    }
    speex_echo_state_destroy(st);
    speex_preprocess_state_destroy(den);

    //Rewrite wav hearder to the destination file with actual data length parameter
    fseek(e_fd, 0L, SEEK_END);
    int nFileLen = ftell(e_fd);
    fseek(e_fd, 0L, SEEK_SET);
    nReturn = WriteWavHeader(e_fd, SrcWAVHeader.nBitsPerSample, SrcWAVHeader.nSampleRate, SrcWAVHeader.nChannelNumber, nFileLen);

    fclose(e_fd);
    fclose(echo_fd);
    fclose(ref_fd);
    return 0;
}
