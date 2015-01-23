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

#include "JobTimer.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <MediaSourceConsumer.h>
#include <WoogeenTransport.h>
#include <webrtc/voice_engine/include/voe_base.h>
#include <webrtc/voice_engine/include/voe_video_sync.h>

namespace mcu {

class AudioMixerVADCallback {
public:
    virtual void onPositiveAudioSources(std::vector<uint32_t>& sources) = 0;
};

class AudioMixer : public woogeen_base::MediaSourceConsumer, public erizo::MediaSink, public erizo::FeedbackSink, public JobTimerListener {
    DECLARE_LOGGER();

public:
    AudioMixer(erizo::RTPDataReceiver*, AudioMixerVADCallback* callback, bool enableVAD = false);
    virtual ~AudioMixer();

    // Implements MediaSourceConsumer.
    int32_t addSource(uint32_t id, bool isAudio, erizo::FeedbackSink*, const std::string& participantId);
    int32_t removeSource(uint32_t id, bool isAudio);
    erizo::MediaSink* mediaSink() { return this; }

    // Implements MediaSink.
    int deliverAudioData(char*, int len);
    int deliverVideoData(char*, int len);

    // Implements FeedbackSink.
    int deliverFeedback(char* buf, int len);

    // Implements JobTimerListener.
    void onTimeout();

    int32_t sharedChannelId() { return m_sharedChannel.id; }
    int32_t addOutput(const std::string& participant);
    int32_t removeOutput(const std::string& particpant);
    uint32_t getSendSSRC(int32_t channelId);

    int32_t getChannelId(uint32_t sourceId);
    webrtc::VoEVideoSync* avSyncInterface();

private:
    int32_t performMix();
    int32_t updateAudioFrame();
    void detectActiveSources();
    void notifyActiveSources();

    struct VoiceChannel {
        int32_t id;
        boost::shared_ptr<woogeen_base::WoogeenTransport<erizo::AUDIO>> transport;
    };

    webrtc::VoiceEngine* m_voiceEngine;

    VoiceChannel m_sharedChannel;
    erizo::RTPDataReceiver* m_dataReceiver;
    bool m_vadEnabled;
    AudioMixerVADCallback* m_vadCallback;
    uint32_t m_jitterHoldCount;
    std::vector<int32_t> m_activeChannels;
    std::map<uint32_t, VoiceChannel> m_sourceChannels; // TODO: revisit here - shall we use ChannelManager?
    boost::shared_mutex m_sourceMutex;
    std::map<std::string, VoiceChannel> m_outputChannels;
    boost::shared_mutex m_outputMutex;

    boost::scoped_ptr<JobTimer> m_jobTimer;
};

inline webrtc::VoEVideoSync* AudioMixer::avSyncInterface()
{
    return m_voiceEngine ? webrtc::VoEVideoSync::GetInterface(m_voiceEngine) : nullptr;
}

} /* namespace mcu */

#endif /* AudioMixer_h */
