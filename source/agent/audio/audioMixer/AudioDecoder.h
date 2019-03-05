// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioDecoder_h
#define AudioDecoder_h

#include <webrtc/modules/include/module_common_types.h>
#include "MediaFramePipeline.h"

namespace mcu {

class AudioDecoder : public owt_base::FrameDestination {
public:
    virtual ~AudioDecoder() { }

    virtual bool init() = 0;
    virtual bool getAudioFrame(webrtc::AudioFrame *audioFrame) = 0;

    // Implements owt_base::FrameDestination
    virtual void onFrame(const owt_base::Frame& frame) = 0;
};

} /* namespace mcu */

#endif /* AudioDecoder_h */
