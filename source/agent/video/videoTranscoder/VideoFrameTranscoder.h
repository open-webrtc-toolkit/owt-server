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

#ifndef VideoFrameTranscoder_h
#define VideoFrameTranscoder_h

#include "VideoHelper.h"
#include <MediaFramePipeline.h>

namespace mcu {

class VideoFrameTranscoder {
public:
    virtual bool setInput(int input, woogeen_base::FrameFormat, woogeen_base::FrameSource*) = 0;
    virtual void unsetInput(int input) = 0;

    virtual bool addOutput(int output,
            woogeen_base::FrameFormat,
            const woogeen_base::VideoSize&,
            const unsigned int framerateFPS,
            const unsigned int bitrateKbps,
            const unsigned int keyFrameIntervalSeconds,
            woogeen_base::FrameDestination*) = 0;
    virtual void removeOutput(int output) = 0;

    virtual void requestKeyFrame(int output) = 0;
};

}
#endif
