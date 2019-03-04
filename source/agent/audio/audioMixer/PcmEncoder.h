// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef PcmEncoder_h
#define PcmEncoder_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <webrtc/common_types.h>

#include <logger.h>

#include "MediaFramePipeline.h"
#include "AudioEncoder.h"

namespace mcu {
using namespace woogeen_base;
using namespace webrtc;

class PcmEncoder : public AudioEncoder {
    DECLARE_LOGGER();

public:
    PcmEncoder(const FrameFormat format);
    ~PcmEncoder();

    bool init() override;
    bool addAudioFrame(const AudioFrame *audioFrame) override;

private:
    FrameFormat m_format;

    uint32_t m_timestampOffset;
    bool m_valid;
};

} /* namespace mcu */

#endif /* PcmEncoder_h */
