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
    bool addOutput(const std::string& endpoint, const std::string& outStreamId, const std::string& codec, woogeen_base::FrameDestination* dest);
    void removeOutput(const std::string& endpoint, const std::string& outStreamId);

    void setEventRegistry(EventRegistry* handle);

private:
    boost::shared_ptr<AudioFrameMixer> m_mixer;
};

} /* namespace mcu */

#endif /* AudioMixer_h */
