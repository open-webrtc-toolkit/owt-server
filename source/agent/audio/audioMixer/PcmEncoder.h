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
