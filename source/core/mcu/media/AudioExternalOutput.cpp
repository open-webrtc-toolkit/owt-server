/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#include "AudioExternalOutput.h"

#include <rtputils.h>
#include <webrtc/voice_engine/include/voe_codec.h>
#include <webrtc/voice_engine/include/voe_network.h>

using namespace webrtc;

namespace mcu {

AudioExternalOutput::AudioExternalOutput()
    : m_inputChannelId(-1)
{
    init();
}

AudioExternalOutput::~AudioExternalOutput()
{
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);

    if (m_inputChannelId != -1) {
        voe->StopPlayout(m_inputChannelId);
        voe->StopReceive(m_inputChannelId);
        network->DeRegisterExternalTransport(m_inputChannelId);
        voe->DeleteChannel(m_inputChannelId);
    }

    boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int32_t, boost::shared_ptr<woogeen_base::AudioEncodedFrameCallbackAdapter>>::iterator outputIt = m_outputTransports.begin();
    for (; outputIt != m_outputTransports.end(); ++outputIt) {
        int channel = outputIt->first;
        voe->DeRegisterPostEncodeFrameCallback(channel);

        voe->StopSend(channel);
        network->DeRegisterExternalTransport(channel);
        voe->DeleteChannel(channel);
    }
    m_outputTransports.clear();
    lock.unlock();

    voe->Terminate();
    VoiceEngine::Delete(m_voiceEngine);

    m_jobTimer->stop();
}

void AudioExternalOutput::init()
{
    // FIXME: hard coded timer interval.
    m_jobTimer.reset(new woogeen_base::JobTimer(100, this));

    m_voiceEngine = VoiceEngine::Create();
    m_adm.reset(new FakeAudioDeviceModule);
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    voe->Init(m_adm.get());

    m_inputChannelId = voe->CreateChannel();
    if (m_inputChannelId != -1) {
        VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);
        m_inputTransport.reset(new woogeen_base::WebRTCTransport<erizo::AUDIO>(nullptr, this));

        if (network->RegisterExternalTransport(m_inputChannelId, *(m_inputTransport.get())) == -1
            || voe->StartReceive(m_inputChannelId) == -1 || voe->StartPlayout(m_inputChannelId) == -1) {
            voe->DeleteChannel(m_inputChannelId);
            return;
        }
    }
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

int AudioExternalOutput::deliverVideoData(char* buf, int len)
{
    assert(false);
    return 0;
}

int AudioExternalOutput::deliverAudioData(char* buf, int len)
{
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
    uint8_t packetType = chead->getPacketType();

    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);

    if (packetType == RTCP_Sender_PT)
        return network->ReceivedRTCPPacket(m_inputChannelId, buf, len) == -1 ? 0 : len;

    return network->ReceivedRTPPacket(m_inputChannelId, buf, len) == -1 ? 0 : len;
}

int AudioExternalOutput::deliverFeedback(char* buf, int len)
{
    // The incoming feedback might be useful for the future external output
    return -1;
}

int32_t AudioExternalOutput::addFrameConsumer(const std::string& name, int payloadType, woogeen_base::FrameConsumer* consumer)
{
    return addOutput(payloadType, consumer);
}

void AudioExternalOutput::removeFrameConsumer(int32_t id)
{
    if (id != -1)
        removeOutput(id);
}

bool AudioExternalOutput::getVideoSize(unsigned int&, unsigned int&) const
{
    assert(false);
    return false;
}

void AudioExternalOutput::onTimeout() {
    VoECodec* codec = VoECodec::GetInterface(m_voiceEngine);
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    AudioTransport* audioTransport = voe->audio_transport();
    int16_t data[AudioFrame::kMaxDataSizeSamples];
    uint32_t nSamplesOut = 0;
    CodecInst audioCodec;

    // Audio external output channels
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int32_t, boost::shared_ptr<woogeen_base::AudioEncodedFrameCallbackAdapter>>::iterator it = m_outputTransports.begin();
    for (; it != m_outputTransports.end(); ++it) {
        int64_t elapsedTimeInMs, ntpTimeInMs;
        int32_t channelId = it->first;
        if (codec->GetSendCodec(channelId, audioCodec) != -1) {
            if (audioTransport->NeedMorePlayData(
                audioCodec.plfreq / 1000 * 10, // samples per channel in a 10 ms period. FIXME: hard coded timer interval.
                0,
                audioCodec.channels,
                audioCodec.plfreq,
                data,
                nSamplesOut,
                &elapsedTimeInMs,
                &ntpTimeInMs,
                it == m_outputTransports.begin(),
                channelId) == 0)
                audioTransport->OnData(channelId, data, 0, audioCodec.plfreq, audioCodec.channels, nSamplesOut);
        }
    }
}

int32_t AudioExternalOutput::addOutput(int payloadType, woogeen_base::FrameConsumer* consumer)
{
    int32_t outputChannelId = -1;
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    outputChannelId = voe->CreateChannel();
    if (outputChannelId != -1) {
        VoECodec* voECodec = VoECodec::GetInterface(m_voiceEngine);
        CodecInst audioCodec;
        bool validCodec = fillAudioCodec(payloadType, audioCodec);
        if (!validCodec || voECodec->SetSendCodec(outputChannelId, audioCodec) == -1
            || voe->StartSend(outputChannelId) == -1) {
            voe->DeleteChannel(outputChannelId);
            return -1;
        }

        boost::shared_ptr<woogeen_base::AudioEncodedFrameCallbackAdapter> audioEncodedFrameCallback(new woogeen_base::AudioEncodedFrameCallbackAdapter(consumer));
        voe->RegisterPostEncodeFrameCallback(outputChannelId, audioEncodedFrameCallback.get());

        boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
        m_outputTransports[outputChannelId] = audioEncodedFrameCallback;
    }

    return outputChannelId;
}

void AudioExternalOutput::removeOutput(int32_t channelId)
{
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);

    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int32_t, boost::shared_ptr<woogeen_base::AudioEncodedFrameCallbackAdapter>>::iterator it = m_outputTransports.find(channelId);
    if (it != m_outputTransports.end()) {
        voe->DeRegisterPostEncodeFrameCallback(channelId);

        voe->StopSend(channelId);
        network->DeRegisterExternalTransport(channelId);
        voe->DeleteChannel(channelId);
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_outputTransports.erase(it);
    }
}

} /* namespace mcu */
