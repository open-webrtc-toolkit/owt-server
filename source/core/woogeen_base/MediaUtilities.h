/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

inline int findNALU(uint8_t* buf, int size, int* nal_start, int* nal_end, int* sc_len)
{
    int i = 0;
    *nal_start = 0;
    *nal_end = 0;
    *sc_len = 0;

    while (true) {
        if (size < i + 3)
            return -1; /* Did not find NAL start */

        /* ( next_bits( 24 ) == {0, 0, 1} ) */
        if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1) {
            i += 3;
            *sc_len = 3;
            break;
        }

        /* ( next_bits( 32 ) == {0, 0, 0, 1} ) */
        if (size > i + 3 && buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 0 && buf[i + 3] == 1) {
            i += 4;
            *sc_len = 4;
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

#if 0
inline bool isH264KeyFrame(uint8_t *data, size_t len)
{
    if (len < 5) {
        //printf("Invalid len %ld\n", len);
        return false;
    }

    if (data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 1) {
        //printf("Invalid start code %d-%d-%d-%d\n", data[0], data[1], data[2], data[3]);
        return false;
    }

    int nal_unit_type = data[4] & 0x1f;

    if (nal_unit_type == 5 || nal_unit_type == 7) {
        //printf("nal_unit_type %d, key_frame %d, len %ld\n", nal_unit_type, true, len);
        return true;
    } else if (nal_unit_type == 9) {
        if (len < 6) {
            //printf("Invalid len %ld\n", len);
            return false;
        }

        int primary_pic_type = (data[5] >> 5) & 0x7;

        //printf("nal_unit_type %d, primary_pic_type %d, key_frame %d, len %ld\n", nal_unit_type, primary_pic_type, (primary_pic_type == 0), len);
        return (primary_pic_type == 0);
    } else {
        //printf("nal_unit_type %d, key_frame %d, len %ld\n", nal_unit_type, false, len);
        return false;
    }
}

inline bool isVp8KeyFrame(uint8_t *data, size_t len)
{
    if (len < 3) {
        //printf("Invalid len %ld\n", len);
        return false;
    }

    unsigned char *c = data;
    unsigned int tmp = (c[2] << 16) | (c[1] << 8) | c[0];

    int key_frame = tmp & 0x1;
    //int version = (tmp >> 1) & 0x7;
    //int show_frame = (tmp >> 4) & 0x1;
    //int first_part_size = (tmp >> 5) & 0x7FFFF;

    //printf("key_frame %d, len %ld\n", (key_frame == 0), len);
    return (key_frame == 0);
}
#endif

}

#endif // MediaUtilities_h
