// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioFrameMixer_h
#define AudioFrameMixer_h

#include <EventRegistry.h>

#include "MediaFramePipeline.h"

namespace mcu {

class AudioFrameMixer {
public:
    virtual ~AudioFrameMixer() {}

    virtual void enableVAD(uint32_t period) = 0;
    virtual void disableVAD() = 0;
    virtual void resetVAD() = 0;

    virtual bool addInput(const std::string& group, const std::string& inStream, const woogeen_base::FrameFormat format, woogeen_base::FrameSource* source) = 0;
    virtual void removeInput(const std::string& group, const std::string& inStream) = 0;

    virtual void setInputActive(const std::string& group, const std::string& inStream, bool active) = 0;

    virtual bool addOutput(const std::string& group, const std::string& outStream, const woogeen_base::FrameFormat format, woogeen_base::FrameDestination* destination) = 0;
    virtual void removeOutput(const std::string& group, const std::string& outStream) = 0;

    virtual void setEventRegistry(EventRegistry* handle) = 0;
};

} /* namespace mcu */

#endif /* AudioFrameMixer_h */
