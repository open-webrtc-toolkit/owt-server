// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioMixer_h
#define AudioMixer_h

#include <EventRegistry.h>
#include <logger.h>

#include "MediaFramePipeline.h"
#include "AudioFrameMixer.h"

namespace mcu {

class AudioMixer {
    DECLARE_LOGGER();

public:
    AudioMixer(const std::string& configStr);
    virtual ~AudioMixer();

    void enableVAD(uint32_t period);
    void disableVAD();
    void resetVAD();

    bool addInput(const std::string& endpoint, const std::string& inStreamId, const std::string& codec, woogeen_base::FrameSource* source);
    void removeInput(const std::string& endpoint, const std::string& inStreamId);
    void setInputActive(const std::string& endpoint, const std::string& inStreamId, bool active);

    bool addOutput(const std::string& endpoint, const std::string& outStreamId, const std::string& codec, woogeen_base::FrameDestination* dest);
    void removeOutput(const std::string& endpoint, const std::string& outStreamId);

    void setEventRegistry(EventRegistry* handle);

private:
    boost::shared_ptr<AudioFrameMixer> m_mixer;
};

} /* namespace mcu */

#endif /* AudioMixer_h */
