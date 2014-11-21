////testdenoise.c
//

#include <speex/speex_preprocess.h>
#include <stdio.h>

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
 
int main(int argc,char *argv[])
{
    SpeexPreprocessState *st; 
    FILE *fpSrc, *fpDst;
    WAVFLIEHEAD SrcWAVHeader;
    int nDenoiseEnable, nSizeRead, nSizeToRead;
    char *pBufSrc, *pBufDst;
    int nFrameNum = -1;
 
    if (argc != 3) {
        printf("Usage: testdenoise <input_file_name> <output_file_name>\n");
        printf("Note: Only 16bit audio supported currently.\n");
        return -1;
    }

    fpSrc=fopen(argv[1], "rb");
    if(!fpSrc) {
        printf("[Error]: Failed to open src wav file!\n");
        return -2;
    }
    fread(&SrcWAVHeader, sizeof(WAVFLIEHEAD), 1, fpSrc);
    printf("[Info]: src_file %s: channel_num %d, bits_per_sample %d, sample_rate %d\n", \
            argv[1], SrcWAVHeader.nChannelNumber, SrcWAVHeader.nBitsPerSample, SrcWAVHeader.nSampleRate);

    //PCM FrameSize = SampleRate * 0.02
    int nFrameSize = SrcWAVHeader.nSampleRate / 50;
    printf("[Info]: nFrameSize = %d\n", nFrameSize);
 
    fpDst = fopen(argv[2], "wb");
    if(!fpDst) {
        printf("[Error]: Failed to open dst wav file!\n");
        fclose(fpSrc);
        return -3;
    }
 
    //Write an initial wav hearder with default parameters to the destination file
    int nReturn = WriteWavHeader(fpDst, SrcWAVHeader.nBitsPerSample, SrcWAVHeader.nSampleRate, SrcWAVHeader.nChannelNumber, 0xFF);
    if (nReturn != 0) {
        fclose(fpSrc);
        fclose(fpDst);
        return -4;
    }

    //Don't change nSizeToRead. Otherwise, it may result in bad quality or segmentation fault in lib.
    nSizeToRead = nFrameSize * (SrcWAVHeader.nBitsPerSample >> 3);	
    printf("[Info]: nSizeToRead = %d\n", nSizeToRead);

    //Allocate buffer
    pBufSrc = (char *)malloc(sizeof(char) * nSizeToRead);
	if (!pBufSrc) {
        printf("[Error]: Failed to allocate memory!\n");
        fclose(fpSrc);
        fclose(fpDst);
        return -4;
    }

    //Init speex preprocess ctl
    st = speex_preprocess_state_init(nFrameSize, SrcWAVHeader.nSampleRate);
    nDenoiseEnable = 1;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DENOISE, &nDenoiseEnable);

    while(!feof(fpSrc)) {
        nSizeRead = fread(pBufSrc, sizeof(char), nSizeToRead, fpSrc);
        nFrameNum++;
        //De-noise process
        speex_preprocess_run(st, (short*)pBufSrc);
        fwrite(pBufSrc, sizeof(char), nSizeRead, fpDst);
    }

    speex_preprocess_state_destroy(st);

    free(pBufSrc);
    fclose(fpSrc);

    //Rewrite wav hearder to the destination file with actual data length parameter
    fseek(fpDst, 0L, SEEK_END);
    int nFileLen = ftell(fpDst);
    fseek(fpDst, 0L, SEEK_SET);
    nReturn = WriteWavHeader(fpDst, SrcWAVHeader.nBitsPerSample, SrcWAVHeader.nSampleRate, SrcWAVHeader.nChannelNumber, nFileLen);
    if (nReturn != 0) {
        fclose(fpDst);
        return -5;
    }

    fclose(fpDst);

    return 0;
}
