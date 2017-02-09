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

#ifndef AudioUtilities_h
#define AudioUtilities_h

#include <webrtc/common_types.h>

#include "MediaFramePipeline.h"

using namespace webrtc;

namespace mcu {

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

static inline int frameFormat2PTC(woogeen_base::FrameFormat format)
{
    switch (format) {
        case woogeen_base::FRAME_FORMAT_PCMU:
            return PCMU_8000_PT;
        case woogeen_base::FRAME_FORMAT_PCMA:
            return PCMA_8000_PT;
        case woogeen_base::FRAME_FORMAT_ISAC16:
            return ISAC_16000_PT;
        case woogeen_base::FRAME_FORMAT_ISAC32:
            return ISAC_32000_PT;
        case woogeen_base::FRAME_FORMAT_OPUS:
            return OPUS_48000_PT;
        default:
            return INVALID_PT;
    }
}

static inline bool fillAudioCodec(woogeen_base::FrameFormat format, CodecInst& audioCodec)
{
    bool ret = true;
    audioCodec.pltype = frameFormat2PTC(format);
    switch (format) {
    case woogeen_base::FRAME_FORMAT_PCMU:
        strcpy(audioCodec.plname, "PCMU");
        audioCodec.plfreq = 8000;
        audioCodec.pacsize = 160;
        audioCodec.channels = 1;
        audioCodec.rate = 64000;
        break;
    case woogeen_base::FRAME_FORMAT_PCMA:
        strcpy(audioCodec.plname, "PCMA");
        audioCodec.plfreq = 8000;
        audioCodec.pacsize = 160;
        audioCodec.channels = 1;
        audioCodec.rate = 64000;
        break;
    case woogeen_base::FRAME_FORMAT_ISAC16:
        strcpy(audioCodec.plname, "ISAC");
        audioCodec.plfreq = 16000;
        audioCodec.pacsize = 480;
        audioCodec.channels = 1;
        audioCodec.rate = 32000;
        break;
    case woogeen_base::FRAME_FORMAT_ISAC32:
        strcpy(audioCodec.plname, "ISAC");
        audioCodec.plfreq = 32000;
        audioCodec.pacsize = 960;
        audioCodec.channels = 1;
        audioCodec.rate = 56000;
        break;
    case woogeen_base::FRAME_FORMAT_OPUS:
        strcpy(audioCodec.plname, "opus");
        audioCodec.plfreq = 48000;
        audioCodec.pacsize = 960;
        audioCodec.channels = 2;
        audioCodec.rate = 64000;
        break;
    case woogeen_base::FRAME_FORMAT_PCM_RAW:
        audioCodec.pltype = OPUS_48000_PT;
        strcpy(audioCodec.plname, "opus");
        audioCodec.plfreq = 48000;
        audioCodec.pacsize = 960;
        audioCodec.channels = 2;
        audioCodec.rate = 64000;
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

static inline int32_t getSampleRate(const woogeen_base::FrameFormat format) {
    switch (format) {
        case woogeen_base::FRAME_FORMAT_PCM_RAW:
            return 48000;
        case woogeen_base::FRAME_FORMAT_PCMU:
        case woogeen_base::FRAME_FORMAT_PCMA:
            return 8000;
        case woogeen_base::FRAME_FORMAT_ISAC16:
            return 16000;
        case woogeen_base::FRAME_FORMAT_ISAC32:
            return 32000;
        case woogeen_base::FRAME_FORMAT_OPUS:
            return 48000;
        default:
            return 8000;
    }
}

static inline int32_t getChannels(const woogeen_base::FrameFormat format) {
    switch (format) {
        case woogeen_base::FRAME_FORMAT_PCM_RAW:
        case woogeen_base::FRAME_FORMAT_OPUS:
            return 2;
        case woogeen_base::FRAME_FORMAT_PCMU:
        case woogeen_base::FRAME_FORMAT_PCMA:
        case woogeen_base::FRAME_FORMAT_ISAC16:
        case woogeen_base::FRAME_FORMAT_ISAC32:
            return 1;
        default:
            return 0;
    }
}

} /* namespace mcu */

#endif /* AudioUtilities_h */
