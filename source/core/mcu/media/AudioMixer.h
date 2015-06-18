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

#ifndef AudioMixer_h
#define AudioMixer_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <JobTimer.h>
#include <logger.h>
#include <MediaDefinitions.h>
#include <MediaMuxer.h>
#include <MediaSourceConsumer.h>
#include <WebRTCTransport.h>
#include <webrtc/modules/audio_device/include/fake_audio_device.h>
#include <webrtc/voice_engine/include/voe_external_media.h>
#include <webrtc/voice_engine/include/voe_video_sync.h>

namespace mcu {

class AudioMixerVADCallback {
public:
    virtual void onPositiveAudioSources(std::vector<uint32_t>& sources) = 0;
};

class RawAudioOutput : public webrtc::VoEMediaProcess {
public:
    RawAudioOutput(woogeen_base::FrameConsumer* consumer) : m_frameConsumer(consumer) { }
    ~RawAudioOutput() { }
    void Process(int channelId, webrtc::ProcessingTypes type, int16_t data[], int nbSamples, int sampleRate, bool isStereo);
private:
    woogeen_base::FrameConsumer* m_frameConsumer;
};

class AudioMixer : public woogeen_base::MediaSourceConsumer, public woogeen_base::FrameDispatcher, public erizo::MediaSink, public erizo::FeedbackSink, public woogeen_base::JobTimerListener {
    DECLARE_LOGGER();

public:
    AudioMixer(erizo::RTPDataReceiver*, AudioMixerVADCallback* callback, bool enableVAD = false);
    virtual ~AudioMixer();

    // Implements MediaSourceConsumer.
    erizo::MediaSink* addSource(uint32_t id,
                                bool isAudio,
                                erizo::DataContentType,
                                int payloadType,
                                erizo::FeedbackSink*,
                                const std::string& participantId);
    void removeSource(uint32_t id, bool isAudio);

    // Implements MediaSink.
    int deliverAudioData(char* buf, int len);
    int deliverVideoData(char* buf, int len);

    // Implements FeedbackSink.
    int deliverFeedback(char* buf, int len);

    // Implements JobTimerListener.
    void onTimeout();

    int32_t addOutput(const std::string& participant, int payloadType);
    int32_t removeOutput(const std::string& particpant);
    uint32_t getSendSSRC(int32_t channelId);

    int32_t getChannelId(uint32_t sourceId);
    webrtc::VoEVideoSync* avSyncInterface();

    // Implements FrameDispatcher
    int32_t addFrameConsumer(const std::string& name, int payloadType, woogeen_base::FrameConsumer*);
    void removeFrameConsumer(int32_t id);
    bool getVideoSize(unsigned int& width, unsigned int& height) const { return true; } // not-used

private:
    int32_t performMix();
    int32_t updateAudioFrame();
    void detectActiveSources();
    void notifyActiveSources();

    struct VoiceChannel {
        int32_t id;
        boost::shared_ptr<woogeen_base::WebRTCTransport<erizo::AUDIO>> transport;
    };

    webrtc::VoiceEngine* m_voiceEngine;
    boost::scoped_ptr<webrtc::FakeAudioDeviceModule> m_adm;

    erizo::RTPDataReceiver* m_dataReceiver;
    bool m_vadEnabled;
    AudioMixerVADCallback* m_vadCallback;
    uint32_t m_jitterHoldCount;
    std::vector<int32_t> m_activeChannels;
    std::map<uint32_t, VoiceChannel> m_sourceChannels; // TODO: revisit here - shall we use ChannelManager?
    boost::shared_mutex m_sourceMutex;
    std::map<std::string, VoiceChannel> m_outputChannels;
    boost::shared_mutex m_outputMutex;

    boost::scoped_ptr<woogeen_base::AudioEncodedFrameCallbackAdapter> m_encodedFrameCallback;
    boost::scoped_ptr<woogeen_base::JobTimer> m_jobTimer;
    boost::scoped_ptr<RawAudioOutput> m_rawAudioOutput;
};

inline webrtc::VoEVideoSync* AudioMixer::avSyncInterface()
{
    return m_voiceEngine ? webrtc::VoEVideoSync::GetInterface(m_voiceEngine) : nullptr;
}

} /* namespace mcu */

#endif /* AudioMixer_h */
