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

#ifndef AudioDecoder_h
#define AudioDecoder_h

#include <webrtc/modules/include/module_common_types.h>
#include "MediaFramePipeline.h"

namespace mcu {

class AudioDecoder : public woogeen_base::FrameDestination {
public:
    virtual ~AudioDecoder() { }

    virtual bool init() = 0;
    virtual bool getAudioFrame(webrtc::AudioFrame *audioFrame) = 0;

    // Implements woogeen_base::FrameDestination
    virtual void onFrame(const woogeen_base::Frame& frame) = 0;
};

} /* namespace mcu */

#endif /* AudioDecoder_h */
