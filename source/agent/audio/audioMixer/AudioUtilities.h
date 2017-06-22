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

bool getAudioCodecInst(woogeen_base::FrameFormat format, CodecInst& audioCodec);
int getAudioPltype(woogeen_base::FrameFormat format);
int32_t getAudioSampleRate(const woogeen_base::FrameFormat format);
uint32_t getAudioChannels(const woogeen_base::FrameFormat format);

} /* namespace mcu */

#endif /* AudioUtilities_h */
