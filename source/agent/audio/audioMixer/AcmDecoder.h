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
