// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AcmDecoder_h
#define AcmDecoder_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <webrtc/modules/audio_coding/include/audio_coding_module.h>

#include <logger.h>

#include "MediaFramePipeline.h"
#include "AudioDecoder.h"

namespace mcu {
using namespace woogeen_base;
using namespace webrtc;

class AcmDecoder : public AudioDecoder {
    DECLARE_LOGGER();

public:
    AcmDecoder(const FrameFormat format);
    ~AcmDecoder();

    bool init() override;
    bool getAudioFrame(AudioFrame *audioFrame) override;

    // Implements woogeen_base::FrameDestination
    void onFrame(const Frame& frame) override;

private:
    boost::shared_ptr<AudioCodingModule> m_audioCodingModule;
    FrameFormat m_format;

    unsigned int m_ssrc;
    uint32_t m_seqNumber;
    bool m_valid;
    boost::shared_mutex m_mutex;
};

} /* namespace mcu */

#endif /* AcmDecoder_h */
