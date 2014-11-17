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
#include <webrtc/common_types.h>
#include <webrtc/modules/audio_device/include/audio_device_defines.h>
#include <webrtc/modules/interface/module_common_types.h>
#include <webrtc/voice_engine/include/voe_codec.h>
#include <webrtc/voice_engine/include/voe_external_media.h>
#include <webrtc/voice_engine/include/voe_network.h>
#include <webrtc/voice_engine/include/voe_rtp_rtcp.h>

using namespace erizo;
using namespace webrtc;

namespace mcu {

DEFINE_LOGGER(AudioMixer, "mcu.AudioMixer");

AudioMixer::AudioMixer(erizo::RTPDataReceiver* receiver)
    : m_isClosing(false)
{
    m_voiceEngine = VoiceEngine::Create();

    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    voe->Init();
    m_outChannel.id = voe->CreateChannel();

    VoEExternalMedia* externalMedia = VoEExternalMedia::GetInterface(m_voiceEngine);
    externalMedia->SetExternalRecordingStatus(true);
    externalMedia->SetExternalPlayoutStatus(true);

    m_outChannel.transport.reset(new woogeen_base::WoogeenTransport<erizo::AUDIO>(receiver, nullptr));
    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);
    network->RegisterExternalTransport(m_outChannel.id, *(m_outChannel.transport));

    // FIXME: hard coded timer interval.
    m_timer.reset(new boost::asio::deadline_timer(m_ioService, boost::posix_time::milliseconds(10)));
    m_timer->async_wait(boost::bind(&AudioMixer::performMix, this, boost::asio::placeholders::error));
    m_audioMixingThread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService)));
}

AudioMixer::~AudioMixer()
{
    // According to the boost document, if the timer has already expired when
    // cancel() is called, then the handlers for asynchronous wait operations
    // can no longer be cancelled, and therefore are passed an error code
    // that indicates the successful completion of the wait operation.
    // This means we cannot rely on the operation_aborted error code in the handlers
    // to know if the timer is being cancelled, thus an additional flag is provided.
    m_isClosing = true;
    if (m_timer)
        m_timer->cancel();
    if (m_audioMixingThread)
        m_audioMixingThread->join();

    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);

    voe->StopSend(m_outChannel.id);
    network->DeRegisterExternalTransport(m_outChannel.id);
    voe->DeleteChannel(m_outChannel.id);

    boost::unique_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, VoiceChannel>::iterator it = m_inChannels.begin();
    for (; it != m_inChannels.end(); ++it) {
        int channel = it->second.id;
        voe->StopPlayout(channel);
        voe->StopReceive(channel);
        network->DeRegisterExternalTransport(channel);
        voe->DeleteChannel(channel);
    }
    m_inChannels.clear();
    lock.unlock();

    voe->Terminate();
    VoiceEngine::Delete(m_voiceEngine);
}

int32_t AudioMixer::addSource(uint32_t from, bool isAudio, erizo::FeedbackSink* feedback)
{
    assert(isAudio);
    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    int channel = voe->CreateChannel();
    if (channel != -1) {
        woogeen_base::WoogeenTransport<erizo::AUDIO>* feedbackTransport = new woogeen_base::WoogeenTransport<erizo::AUDIO>(nullptr, feedback);
        VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);
        if (network->RegisterExternalTransport(channel, *feedbackTransport) == -1)
            return -1;

        if (voe->StartReceive(channel) == -1)
            return -1;
        if (voe->StartPlayout(channel) == -1)
            return -1;

        // TODO: Another option is that we can implement VoEMediaProcess and register
        // an External media processor for mixing. We may need to investigate whether it's
        // better than the current approach.
        // VoEExternalMedia* externalMedia = VoEExternalMedia::GetInterface(m_voiceEngine);
        // externalMedia->SetExternalMixing(channel, true);

        boost::unique_lock<boost::shared_mutex> lock(m_sourceMutex);
        if (m_inChannels.size() == 0)
            voe->StartSend(m_outChannel.id);

        m_inChannels[from] = {channel, boost::shared_ptr<woogeen_base::WoogeenTransport<erizo::AUDIO>>(feedbackTransport)};
    }
    return channel;
}

int32_t AudioMixer::removeSource(uint32_t from, bool isAudio)
{
    assert(isAudio);

    VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
    VoENetwork* network = VoENetwork::GetInterface(m_voiceEngine);

    boost::unique_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, VoiceChannel>::iterator it = m_inChannels.find(from);
    if (it != m_inChannels.end()) {
        int channel = it->second.id;

        voe->StopPlayout(channel);
        voe->StopReceive(channel);
        network->DeRegisterExternalTransport(channel);
        voe->DeleteChannel(channel);
        m_inChannels.erase(it);

        if (m_inChannels.size() == 0)
            voe->StopSend(m_outChannel.id);

        return 0;
    }

    return -1;
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
    std::map<uint32_t, VoiceChannel>::iterator it = m_inChannels.find(id);
    if (it == m_inChannels.end()) {
        // TODO: Add a flag to control whether to add source on demand.
        lock.unlock();
        addSource(id, true, nullptr);
        return 0;
    }

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
    return network->ReceivedRTCPPacket(m_outChannel.id, buf, len) == -1 ? 0 : len;
}

int32_t AudioMixer::channelId(uint32_t sourceId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, VoiceChannel>::iterator it = m_inChannels.find(sourceId);
    if (it != m_inChannels.end())
        return it->second.id;

    return -1;
}

uint32_t AudioMixer::sendSSRC()
{
    VoERTP_RTCP* rtpRtcp = VoERTP_RTCP::GetInterface(m_voiceEngine);
    uint32_t ssrc = 0;
    rtpRtcp->GetLocalSSRC(m_outChannel.id, ssrc);
    return ssrc;
}

int32_t AudioMixer::performMix(const boost::system::error_code& ec)
{
    if (!ec) {
        VoECodec* codec = VoECodec::GetInterface(m_voiceEngine);
        CodecInst audioCodec;
        if (codec->GetSendCodec(m_outChannel.id, audioCodec) != -1) {
            VoEBase* voe = VoEBase::GetInterface(m_voiceEngine);
            AudioTransport* audioTransport = voe->audio_transport();
            int16_t data[AudioFrame::kMaxDataSizeSamples];
            uint32_t nSamplesOut = 0;
            if (audioTransport->NeedMorePlayData(
                audioCodec.plfreq / 1000 * 10, // samples per channel in a 10 ms period. FIXME: hard coded timer interval.
                0,
                audioCodec.channels,
                audioCodec.plfreq,
                data,
                nSamplesOut) == 0) {
                audioTransport->OnData(m_outChannel.id, data, 0, audioCodec.plfreq, audioCodec.channels, nSamplesOut);
            }
        }

        if (!m_isClosing) {
            // FIXME: hard coded timer interval.
            m_timer->expires_at(m_timer->expires_at() + boost::posix_time::milliseconds(10));
            m_timer->async_wait(boost::bind(&AudioMixer::performMix, this, boost::asio::placeholders::error));
        }
    } else {
        ELOG_INFO("AudioMixer timer error: %s", ec.message().c_str());
    }
    return 0;
}

} /* namespace mcu */
