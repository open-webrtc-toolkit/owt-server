// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AcmmInput_h
#define AcmmInput_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <webrtc/modules/audio_conference_mixer/include/audio_conference_mixer_defines.h>

#include <logger.h>

#include "MediaFramePipeline.h"

#include "AudioDecoder.h"

namespace mcu {

using namespace owt_base;
using namespace webrtc;

class AcmmInput : public MixerParticipant {
    DECLARE_LOGGER();

public:
    AcmmInput(int32_t id, const std::string &name);
    ~AcmmInput();

    int32_t id() {return m_id;}
    const std::string &name() {return m_name;}

    bool isActive() {return m_active;}

    bool setSource(FrameFormat format, FrameSource* source);
    void unsetSource();

    void setActive(bool active);

    // Implements MixerParticipant
    int32_t GetAudioFrame(int32_t id, AudioFrame* audioFrame) override;
    int32_t NeededFrequency(int32_t id) const override;

private:
    int32_t m_id;
    const std::string m_name;

    bool m_active;

    FrameFormat m_srcFormat;
    FrameSource *m_source;

    boost::shared_ptr<AudioDecoder> m_decoder;
};

} /* namespace mcu */

#endif /* AcmmInput_h */
