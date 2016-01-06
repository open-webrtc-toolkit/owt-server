/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#ifndef MediaUtilities_h
#define MediaUtilities_h

#include <Compiler.h>

namespace woogeen_base {

static int partial_linear_bitrate[][2] = {
    {0, 0}, {76800, 400}, {307200, 800}, {921600, 2000}, {2073600, 4000}, {8294400, 16000}
};

inline unsigned int calcBitrate(unsigned int width, unsigned int height, float framerate = 30) {
    unsigned int bitrate = 0;
    unsigned int prev = 0;
    unsigned int next = 0;
    float portion = 0.0;
    unsigned int def = width * height * framerate / 30;
    int lines = sizeof(partial_linear_bitrate) / sizeof(partial_linear_bitrate[0][0]) / 2;

    // find the partial linear section and calculate bitrate
    for (int i = 0; i < lines - 1; i++) {
        prev = partial_linear_bitrate[i][0];
        next = partial_linear_bitrate[i+1][0];
        if (def > prev && def <= next) {
            portion = static_cast<float>(def - prev) / (next - prev);
            bitrate = partial_linear_bitrate[i][1] + (partial_linear_bitrate[i+1][1] - partial_linear_bitrate[i][1]) * portion;
            break;
        }
    }

    // set default bitrate for over large resolution
    if (0 == bitrate)
        bitrate = 8000;

    return bitrate;
}

inline int findNALU(uint8_t* buf, int size, int* nal_start, int* nal_end)
{
    int i = 0;
    *nal_start = 0;
    *nal_end = 0;

    while (true) {
        if (size < i + 3)
            return -1; /* Did not find NAL start */

        /* ( next_bits( 24 ) == {0, 0, 1} ) */
        if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1) {
            i += 3;
            break;
        }

        /* ( next_bits( 32 ) == {0, 0, 0, 1} ) */
        if (size > i + 3 && buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 0 && buf[i + 3] == 1) {
            i += 4;
            break;
        }

        ++i;
    }

    // assert(buf[i - 1] == 1);
    *nal_start = i;

    /*( next_bits( 24 ) != {0, 0, 1} )*/
    while ((size > i + 2) &&
           (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 1))
        ++i;

    if (size <= i + 2)
        *nal_end = size;
    else if (buf[i - 1] == 0)
        *nal_end = i - 1;
    else
        *nal_end = i;

    return (*nal_end - *nal_start);
}

class VP8Dumper {
public:
    VP8Dumper(const char* filePath, uint32_t timeS, uint16_t width_ = 0, uint16_t height_ = 0, uint32_t frameRate = 30) : m_file(nullptr), m_index(0), m_frameCount(timeS * frameRate), m_frameRate(frameRate) {
        m_file = fopen(filePath, "wb");
        assert(m_file);

        char ivfFileHdr[32];
        uint16_t ivfFileHdrLen = 32;
        uint16_t width = width_ ? width_ : 640;
        uint16_t height = height_ ? height_ : 480;
        uint32_t framerate = m_frameRate;
        uint32_t timescale = 1;

        ivfFileHdr[0] = 'D';
        ivfFileHdr[1] = 'K';
        ivfFileHdr[2] = 'I';
        ivfFileHdr[3] = 'F';
        ivfFileHdr[4] = '\0';
        ivfFileHdr[5] = '\0';
        mem_put_le16(ivfFileHdr + 6, ivfFileHdrLen);
        ivfFileHdr[8] = 'V';
        ivfFileHdr[9] = 'P';
        ivfFileHdr[10] = '8';
        ivfFileHdr[11] = '0';

        mem_put_le16(ivfFileHdr + 12, width);
        mem_put_le16(ivfFileHdr + 14, height);
        mem_put_le32(ivfFileHdr + 16, framerate);
        mem_put_le32(ivfFileHdr + 20, timescale);
        mem_put_le32(ivfFileHdr + 24, m_frameCount);     /* length */
        fwrite(ivfFileHdr, sizeof(ivfFileHdr), 1, m_file);
    }

    virtual ~VP8Dumper() {
        if (m_file != nullptr) {
            fclose(m_file);
            m_file = nullptr;
        }
        m_index = 0;
    }

    void onFrame(char* buf, int len) {
        if (m_file) {
            int64_t cs = m_frameRate * m_index++;
            char ivfFrameHdr[12];
            mem_put_le32(ivfFrameHdr, len);
            mem_put_le32(ivfFrameHdr+ 4, (int)(cs & 0xFFFFFFFF));
            mem_put_le32(ivfFrameHdr + 8, (int)(cs >> 32));
            fwrite(ivfFrameHdr, sizeof(ivfFrameHdr), 1, m_file);

            fwrite(buf, len, 1, m_file);

            if (m_index == m_frameCount) {
                fclose(m_file);
                m_file = nullptr;
            }
        }
    }

private:
    void mem_put_le32(void *vmem, int val) {
        unsigned char *mem = (unsigned char *)vmem;

        mem[0] = (val >>  0) & 0xff;
        mem[1] = (val >>  8) & 0xff;
        mem[2] = (val >> 16) & 0xff;
        mem[3] = (val >> 24) & 0xff;
    }

    void mem_put_le16(void *vmem, int val) {
        unsigned char  *mem = (unsigned char  *)vmem;

        mem[0] = (val >>  0) & 0xff;
        mem[1] = (val >>  8) & 0xff;
    }


private:
    FILE*  m_file;
    uint32_t m_index;
    uint32_t m_frameCount;
    uint32_t m_frameRate;
};

}

#endif // MediaUtilities_h
