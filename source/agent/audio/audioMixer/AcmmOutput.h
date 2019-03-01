// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AcmmOutput_h
#define AcmmOutput_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <webrtc/modules/audio_conference_mixer/include/audio_conference_mixer_defines.h>

#include <logger.h>

#include "MediaFramePipeline.h"

#include "AudioEncoder.h"

namespace mcu {

using namespace woogeen_base;
using namespace webrtc;

class AcmmOutput {
    DECLARE_LOGGER();

public:
    AcmmOutput(int32_t id);
    ~AcmmOutput();

    int32_t id() {return m_id;}

    bool addDest(FrameFormat format, FrameDestination* destination);
    void removeDest(FrameDestination* destination);

    bool hasDest() {return m_destinations.size() > 0;}

    int32_t NeededFrequency();
    bool newAudioFrame(const webrtc::AudioFrame *audioFrame);

private:
    int32_t m_id;

    FrameFormat m_dstFormat;
    std::list<FrameDestination *> m_destinations;

    boost::shared_ptr<AudioEncoder> m_encoder;
};

} /* namespace mcu */

#endif /* AcmmOutput_h */
