// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AcmEncoder_h
#define AcmEncoder_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <webrtc/modules/audio_coding/include/audio_coding_module.h>

#include <logger.h>

#include "MediaFramePipeline.h"
#include "AudioEncoder.h"

namespace mcu {
using namespace owt_base;
using namespace webrtc;

class AcmEncoder : public AudioEncoder,
                       public AudioPacketizationCallback {
    DECLARE_LOGGER();

public:
    AcmEncoder(const FrameFormat format);
    ~AcmEncoder();

    bool init() override;
    bool addAudioFrame(const AudioFrame *audioFrame) override;

    // Implements AudioPacketizationCallback
    int32_t SendData(FrameType frame_type,
            uint8_t payload_type,
            uint32_t timestamp,
            const uint8_t* payload_data,
            size_t payload_len_bytes,
            const RTPFragmentationHeader* fragmentation) override;

protected:
    void encodeLoop();

private:
    boost::shared_ptr<AudioCodingModule> m_audioCodingModule;
    FrameFormat m_format;
    uint32_t m_rtpSampleRate;

    uint32_t m_timestampOffset;
    bool m_valid;

    bool m_running;
    boost::thread m_thread;
    boost::mutex m_mutex;
    boost::condition_variable m_cond;

    uint32_t m_incomingFrameCount;
    boost::shared_ptr<AudioFrame> m_frame;
};

} /* namespace mcu */

#endif /* AcmEncoder_h */
