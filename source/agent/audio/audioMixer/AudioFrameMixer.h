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

    virtual bool addInput(const std::string& participant, const woogeen_base::FrameFormat format, woogeen_base::FrameSource* source) = 0;
    virtual void removeInput(const std::string& participant) = 0;
    virtual bool addOutput(const std::string& participant, const woogeen_base::FrameFormat format, woogeen_base::FrameDestination* destination) = 0;
    virtual void removeOutput(const std::string& participant) = 0;

    virtual void setEventRegistry(EventRegistry* handle) = 0;
};

} /* namespace mcu */

#endif /* AudioFrameMixer_h */
