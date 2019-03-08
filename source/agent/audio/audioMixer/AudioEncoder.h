// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioEncoder_h
#define AudioEncoder_h

#include <webrtc/modules/include/module_common_types.h>
#include "MediaFramePipeline.h"

namespace mcu {

class AudioEncoder : public owt_base::FrameSource {
public:
    virtual ~AudioEncoder() { }

    virtual bool init() = 0;
    virtual bool addAudioFrame(const webrtc::AudioFrame *audioFrame) = 0;
};

} /* namespace mcu */

#endif /* AudioEncoder_h */
