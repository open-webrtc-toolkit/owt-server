/*
 * Copyright 2014 Intel Corporation All Rights Reserved.
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

#ifndef AudioChannel_h
#define AudioChannel_h

#include <EventRegistry.h>
#include <utility>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <webrtc/modules/audio_device/include/fake_audio_device.h>
#include <webrtc/voice_engine/include/voe_external_media.h>
#include <webrtc/voice_engine/include/voe_base.h>

#include <logger.h>
#include "MediaFramePipeline.h"
#include "WebRTCTransport.h"
#include "AudioFrame2RtpPacketConverter.h"

namespace woogeen_base {
    struct Frame;
    enum FrameFormat;
}

namespace mcu {

using namespace woogeen_base;

class AudioChannel : public webrtc::AudioEncodedFrameCallback,
                     public webrtc::VoEMediaProcess,
                     public woogeen_base::FrameSource,
                     public woogeen_base::FrameDestination {
public:
    AudioChannel(webrtc::VoiceEngine* engine);
    virtual ~AudioChannel();

    bool init();
    int32_t id() {return m_channel;}

    // Implements webrtc::AudioEncodedFrameCallback.
    int32_t Encoded(webrtc::FrameType frameType, uint8_t payloadType,
                    uint32_t timeStamp, const uint8_t* payloadData,
                    uint16_t payloadSize);

    // Implements webrtc::VoEMediaProcess.
    void Process(int channelId, webrtc::ProcessingTypes type, int16_t data[], size_t length, int sampleRate, bool isStereo);

    // Implements woogeen_base::FrameSource.
    void onFeedback(const woogeen_base::FeedbackMsg& msg);

    // Implements woogeen_base::FrameDestination.
    void onFrame(const Frame& frame);

    bool setOutput(FrameFormat format, woogeen_base::FrameDestination* destination);
    void unsetOutput();
    bool setInput(woogeen_base::FrameSource* source);
    void unsetInput();

    bool isIdle();

    void performMix(bool isCommon);

private:
    int32_t m_channel;
    webrtc::VoiceEngine* m_engine;
    boost::shared_ptr<woogeen_base::WebRTCTransport<erizo::AUDIO>> m_transport;
    woogeen_base::FrameSource* m_source;
    boost::shared_mutex m_sourceMutex;
    woogeen_base::FrameDestination* m_destination;
    boost::shared_mutex m_destinationMutex;
    FrameFormat m_outFormat;
    AudioFrame2RtpPacketConverter m_converter; //FIXME: Temporarily convert audio frame to rtp-packets due to the premature AudioFrameConstructor implementation.
};

} /* namespace mcu */

#endif /* AudioChannel_h */
