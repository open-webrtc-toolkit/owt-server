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

#ifndef AcmOutput_h
#define AcmOutput_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <webrtc/modules/audio_coding/include/audio_coding_module.h>

#include <logger.h>

#include "MediaFramePipeline.h"
#include "AudioOutput.h"

namespace mcu {
using namespace woogeen_base;
using namespace webrtc;

class AcmOutput : public AudioOutput,
                       public AudioPacketizationCallback {
    DECLARE_LOGGER();

public:
    AcmOutput(const FrameFormat format, FrameDestination *destination);
    ~AcmOutput();

    bool init() override;
    bool addAudioFrame(const AudioFrame *audioFrame) override;

    // Implements AudioPacketizationCallback
    int32_t SendData(FrameType frame_type,
            uint8_t payload_type,
            uint32_t timestamp,
            const uint8_t* payload_data,
            size_t payload_len_bytes,
            const RTPFragmentationHeader* fragmentation) override;

private:
    boost::shared_ptr<AudioCodingModule> m_audioCodingModule;
    FrameFormat m_format;
    FrameDestination *m_destination;

    uint32_t m_timestampOffset;
    bool m_valid;
};

} /* namespace mcu */

#endif /* AcmOutput_h */
