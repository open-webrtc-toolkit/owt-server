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

#include "AudioMixer.h"

#include <rtputils.h>
#include <webrtc/modules/interface/module_common_types.h>
#include <webrtc/voice_engine/include/voe_codec.h>
#include <webrtc/voice_engine/include/voe_external_media.h>
#include <webrtc/voice_engine/include/voe_network.h>
#include <webrtc/voice_engine/include/voe_rtp_rtcp.h>

using namespace erizo;
using namespace webrtc;

namespace mcu {

DEFINE_LOGGER(AudioMixer, "mcu.media.AudioMixer");

AudioMixer::AudioMixer(erizo::RTPDataReceiver* receiver, AudioMixerVADCallback* callback, bool enableVAD)
    : m_dataReceiver(receiver)
    , m_vadEnabled(enableVAD)
    , m_vadCallback(callback)
    , m_jitterHoldCount(0)
{
    m_voiceEngine = VoiceEngine::Create();
    m_adm.reset(new webrtc::FakeAudioDeviceModule);

    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    voe->Init(m_adm.get());

    // FIXME: hard coded timer interval.
    m_jobTimer.reset(new woogeen_base::JobTimer(100, this));
}

AudioMixer::~AudioMixer()
{
    m_jobTimer->stop();

    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);

    boost::unique_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, VoiceChannel>::iterator it = m_sourceChannels.begin();
    for (; it != m_sourceChannels.end(); ++it) {
        int channel = it->second.id;
        voe->StopPlayout(channel);
        voe->StopReceive(channel);
        network->DeRegisterExternalTransport(channel);
        voe->DeleteChannel(channel);
    }
    m_sourceChannels.clear();
    lock.unlock();

    boost::unique_lock<boost::shared_mutex> outputLock(m_outputMutex);
    std::map<std::string, VoiceChannel>::iterator outputIt = m_outputChannels.begin();
    for (; outputIt != m_outputChannels.end(); ++outputIt) {
        int channel = outputIt->second.id;
        voe->StopSend(channel);
        network->DeRegisterExternalTransport(channel);
        voe->DeleteChannel(channel);
    }
    m_outputChannels.clear();
    outputLock.unlock();

    voe->Terminate();
    VoiceEngine::Delete(m_voiceEngine);
}

erizo::MediaSink* AudioMixer::addSource(uint32_t from,
                                        bool isAudio,
                                        erizo::DataContentType,
                                        const std::string& codecName,
                                        erizo::FeedbackSink* feedback,
                                        const std::string& participantId)
{
    assert(isAudio);

    boost::upgrade_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, VoiceChannel>::iterator it = m_sourceChannels.find(from);
    if (it != m_sourceChannels.end())
        return nullptr;

    int channel = -1;
    boost::shared_ptr<woogeen_base::WebRTCTransport<erizo::AUDIO>> transport;
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    bool existingParticipant = false;

    boost::upgrade_lock<boost::shared_mutex> outputLock(m_outputMutex);
    std::map<std::string, VoiceChannel>::iterator outputIt = m_outputChannels.find(participantId);
    if (outputIt != m_outputChannels.end()) {
        channel = outputIt->second.id;
        transport = outputIt->second.transport;
        existingParticipant = true;
        outputLock.unlock();
    } else
        channel = voe->CreateChannel();

    if (channel != -1) {
        if (existingParticipant) {
            assert(transport);
            // Set the Feedback sink for the transport, because this channel is going to be a source channel.
            transport->setFeedbackSink(feedback);
            if (voe->StartReceive(channel) == -1 || voe->StartPlayout(channel) == -1)
                return nullptr;
        } else {
            VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);
            transport.reset(new woogeen_base::WebRTCTransport<erizo::AUDIO>(m_dataReceiver, feedback));
            if (network->RegisterExternalTransport(channel, *(transport.get())) == -1
                || voe->StartReceive(channel) == -1
                || voe->StartPlayout(channel) == -1) {
                voe->DeleteChannel(channel);
                return nullptr;
            }
            boost::upgrade_to_unique_lock<boost::shared_mutex> uniquePartLock(outputLock);
            m_outputChannels[participantId] = {channel, transport};
        }

        // TODO: Another option is that we can implement
        // an External mixer. We may need to investigate whether it's
        // better than the current approach.
        // VoEExternalMedia* externalMedia = VoEExternalMedia::GetInterface(m_voiceEngine);
        // externalMedia->SetExternalMixing(channel, true);

        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_sourceChannels[from] = {channel, transport};
    }
    return this;
}

void AudioMixer::removeSource(uint32_t from, bool isAudio)
{
    assert(isAudio);

    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);

    boost::upgrade_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, VoiceChannel>::iterator it = m_sourceChannels.find(from);
    if (it != m_sourceChannels.end()) {
        int channel = it->second.id;

        voe->StopPlayout(channel);
        voe->StopReceive(channel);

        bool outputExisted = false;
        boost::shared_lock<boost::shared_mutex> outputLock(m_outputMutex);
        std::map<std::string, VoiceChannel>::iterator outputIt = m_outputChannels.begin();
        for (; outputIt != m_outputChannels.end(); ++outputIt) {
            if (outputIt->second.id == channel) {
                outputIt->second.transport->setFeedbackSink(nullptr);
                outputExisted = true;
                break;
            }
        }
        outputLock.unlock();

        if (!outputExisted) {
            network->DeRegisterExternalTransport(channel);
            voe->DeleteChannel(channel);
        }

        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_sourceChannels.erase(it);
    }
}

#define global_ns

int AudioMixer::deliverAudioData(char* buf, int len)
{
    uint32_t id = 0;
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT)
        id = chead->getSSRC();
    else {
        global_ns::RTPHeader* head = reinterpret_cast<global_ns::RTPHeader*>(buf);
        id = head->getSSRC();
    }

    boost::shared_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, VoiceChannel>::iterator it = m_sourceChannels.find(id);
    if (it == m_sourceChannels.end())
        return 0;

    int channel = it->second.id;
    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);

    if (packetType == RTCP_Sender_PT)
        return network->ReceivedRTCPPacket(channel, buf, len) == -1 ? 0 : len;

    return network->ReceivedRTPPacket(channel, buf, len) == -1 ? 0 : len;
}

int AudioMixer::deliverVideoData(char* buf, int len)
{
    assert(false);
    return 0;
}

int AudioMixer::deliverFeedback(char* buf, int len)
{
    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);

    // TODO: For now we just send the feedback to all of the output channels.
    // The output channel will filter out the feedback which does not belong
    // to it. In the future we may do the filtering at a higher level?
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<std::string, VoiceChannel>::iterator it = m_outputChannels.begin();
    for (; it != m_outputChannels.end(); ++it)
        network->ReceivedRTCPPacket(it->second.id, buf, len);

    return len;
}

void AudioMixer::onTimeout()
{
    performMix();
}

static bool fillAudioCodec(int payloadType, CodecInst& audioCodec)
{
    bool ret = true;
    audioCodec.pltype = payloadType;
    switch (payloadType) {
    case PCMU_8000_PT:
        strcpy(audioCodec.plname, "PCMU");
        audioCodec.plfreq = 8000;
        audioCodec.pacsize = 160;
        audioCodec.channels = 1;
        audioCodec.rate = 64000;
        break;
    case ISAC_16000_PT:
        strcpy(audioCodec.plname, "ISAC");
        audioCodec.plfreq = 16000;
        audioCodec.pacsize = 480;
        audioCodec.channels = 1;
        audioCodec.rate = 32000;
        break;
    case ISAC_32000_PT:
        strcpy(audioCodec.plname, "ISAC");
        audioCodec.plfreq = 32000;
        audioCodec.pacsize = 960;
        audioCodec.channels = 1;
        audioCodec.rate = 56000;
        break;
    case OPUS_48000_PT:
        strcpy(audioCodec.plname, "opus");
        audioCodec.plfreq = 48000;
        audioCodec.pacsize = 960;
        audioCodec.channels = 2;
        audioCodec.rate = 64000;
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

int32_t AudioMixer::addOutput(const std::string& participant, int payloadType)
{
    int channel = -1;
    bool existingParticipant = false;
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);

    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<std::string, VoiceChannel>::iterator it = m_outputChannels.find(participant);
    if (it != m_outputChannels.end()) {
        channel = it->second.id;
        existingParticipant = true;
        lock.unlock();
    } else
        channel = voe->CreateChannel();

    if (channel != -1) {
        VoECodec* codec = VoECodec::GetInterface(m_voiceEngine);
        CodecInst audioCodec;
        bool validCodec = fillAudioCodec(payloadType, audioCodec);

        if (!existingParticipant) {
            boost::shared_ptr<woogeen_base::WebRTCTransport<erizo::AUDIO>> transport(new woogeen_base::WebRTCTransport<erizo::AUDIO>(m_dataReceiver, nullptr));
            VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);
            if (!validCodec || codec->SetSendCodec(channel, audioCodec) == -1
                || network->RegisterExternalTransport(channel, *(transport.get())) == -1
                || voe->StartSend(channel) == -1) {
                voe->DeleteChannel(channel);
                return -1;
            }
            boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
            m_outputChannels[participant] = {channel, transport};
        } else if (!validCodec || codec->SetSendCodec(channel, audioCodec) == -1
                   || voe->StartSend(channel) == -1)
            return -1;
    }

    return channel;
}

int32_t AudioMixer::removeOutput(const std::string& participant)
{
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);

    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<std::string, VoiceChannel>::iterator it = m_outputChannels.find(participant);
    if (it != m_outputChannels.end()) {
        int channel = it->second.id;

        voe->StopSend(channel);

        bool sourceExisted = false;
        boost::shared_lock<boost::shared_mutex> sourceLock(m_sourceMutex);
        std::map<uint32_t, VoiceChannel>::iterator sourceIt = m_sourceChannels.begin();
        for (; sourceIt != m_sourceChannels.end(); ++sourceIt) {
            if (sourceIt->second.id == channel) {
                sourceExisted = true;
                break;
            }
        }
        sourceLock.unlock();

        if (!sourceExisted) {
            network->DeRegisterExternalTransport(channel);
            voe->DeleteChannel(channel);
            boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
            m_outputChannels.erase(it);
        }
        // We keep the output channel if the participant is still there.
        // This is to make it consistent with the addSource/Output behavior.
        // i.e., if there's a source channel for this participant, there's an
        // output channel for it as well, but the output channel will only
        // start sending out data if addOutput is invoked, and stop sending
        // if removeOutput is invoked.

        return 0;
    }

    return -1;
}

int32_t AudioMixer::startMuxing(const std::string& participant, int codec, woogeen_base::MediaFrameQueue& audioQueue)
{
    int32_t id = addOutput(participant, codec);
    if (id != -1) {
        m_encodedFrameCallback.reset(new woogeen_base::AudioEncodedFrameCallbackAdapter(&audioQueue));
        VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
        voe->RegisterPostEncodeFrameCallback(id, m_encodedFrameCallback.get());
    }
    return id;
}

void AudioMixer::stopMuxing(int32_t chanId)
{
    if (chanId == -1)
        return;
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<std::string, VoiceChannel>::iterator it = m_outputChannels.begin();
    for (; it != m_outputChannels.end(); ++it) {
        if (it->second.id == chanId) {
            lock.unlock();
            VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
            voe->DeRegisterPostEncodeFrameCallback(chanId);
            removeOutput(it->first);
            break;
        }
    }
}

int32_t AudioMixer::getChannelId(uint32_t sourceId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, VoiceChannel>::iterator it = m_sourceChannels.find(sourceId);
    if (it != m_sourceChannels.end())
        return it->second.id;

    return -1;
}

uint32_t AudioMixer::getSendSSRC(int32_t channelId)
{
    VoERTP_RTCP* rtpRtcp = VoERTP_RTCP::GetInterface(m_voiceEngine);
    uint32_t ssrc = 0;
    rtpRtcp->GetLocalSSRC(channelId, ssrc);
    return ssrc;
}

int32_t AudioMixer::performMix()
{
    VoECodec* codec = VoECodec::GetInterface(m_voiceEngine);
    CodecInst audioCodec;
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    AudioTransport* audioTransport = voe->audio_transport();
    int16_t data[AudioFrame::kMaxDataSizeSamples];
    uint32_t nSamplesOut = 0;
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    for (std::map<std::string, VoiceChannel>::iterator it = m_outputChannels.begin();
        it != m_outputChannels.end();
        ++it) {
        int64_t elapsedTimeInMs, ntpTimeInMs;
        if (codec->GetSendCodec(it->second.id, audioCodec) != -1) {
            if (audioTransport->NeedMorePlayData(
                audioCodec.plfreq / 1000 * 10, // samples per channel in a 10 ms period. FIXME: hard coded timer interval.
                0,
                audioCodec.channels,
                audioCodec.plfreq,
                data,
                nSamplesOut,
                &elapsedTimeInMs,
                &ntpTimeInMs,
                it == m_outputChannels.begin(),
                it->second.id) == 0)
                audioTransport->OnData(it->second.id, data, 0, audioCodec.plfreq, audioCodec.channels, nSamplesOut);
        }
    }

    if (m_vadEnabled)
        detectActiveSources();

    return 0;
}

void AudioMixer::detectActiveSources()
{
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    std::vector<int32_t> activeChannels;

    if (m_jitterHoldCount > 0) {
        m_jitterHoldCount--;
        return;
    }

    if (!voe->GetActiveMixedInChannels(activeChannels)) {
        if (activeChannels != m_activeChannels) {
            m_activeChannels = activeChannels;
            notifyActiveSources();
            m_jitterHoldCount = 200; /* 200 * 10ms = 2 seconds */
        }
    }
}

void AudioMixer::notifyActiveSources()
{
    std::vector<uint32_t> sources;

    boost::shared_lock<boost::shared_mutex> sourceLock(m_sourceMutex);
    std::map<uint32_t, VoiceChannel>::iterator sourceIt = m_sourceChannels.begin();
    for (; sourceIt != m_sourceChannels.end(); ++sourceIt) {
        if (std::find(m_activeChannels.begin(), m_activeChannels.end(), sourceIt->second.id) != m_activeChannels.end())
            sources.push_back(sourceIt->first);
    }
    sourceLock.unlock();

    if (m_vadCallback)
        m_vadCallback->onPositiveAudioSources(sources);
}

} /* namespace mcu */
