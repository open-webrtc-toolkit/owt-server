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

#include "AudioChannel.h"

#include <rtputils.h>
#include <webrtc/modules/interface/module_common_types.h>
#include <webrtc/voice_engine/include/voe_codec.h>
#include <webrtc/voice_engine/include/voe_network.h>
#include <webrtc/voice_engine/include/voe_audio_processing.h>
#include <webrtc/voice_engine/include/voe_rtp_rtcp.h>

using namespace erizo;
using namespace webrtc;

namespace mcu {

AudioChannel::AudioChannel(webrtc::VoiceEngine* engine)
    : m_channel(-1)
    , m_engine(engine)
    , m_source(nullptr)
    , m_destination(nullptr)
    , m_outFormat(FRAME_FORMAT_OPUS)
{}

bool AudioChannel::init()
{
    VoEBase* voe = VoEBase::GetInterface(m_engine);
    m_channel = voe->CreateChannel();
    VoENetwork* network = VoENetwork::GetInterface(m_engine);

    VoEAudioProcessing* apm = VoEAudioProcessing::GetInterface(m_engine);
    apm->SetNsStatus(true, kNsHighSuppression);

    m_transport.reset(new woogeen_base::WebRTCTransport<erizo::AUDIO>(nullptr, nullptr));
    network->RegisterExternalTransport(m_channel, *(m_transport.get()));

    return m_channel >= 0;
}

AudioChannel::~AudioChannel()
{
    if (m_channel >= 0) {
        unsetInput();
        unsetOutput();

        VoEBase* voe = VoEBase::GetInterface(m_engine);
        voe->StopPlayout(m_channel);
        voe->StopReceive(m_channel);
        voe->StopSend(m_channel);

        VoENetwork* network = VoENetwork::GetInterface(m_engine);
        network->DeRegisterExternalTransport(m_channel);
        voe->DeleteChannel(m_channel);
        m_channel = -1;
    }

    boost::unique_lock<boost::shared_mutex> lock1(m_sourceMutex);
    m_source = nullptr;
    lock1.unlock();

    boost::unique_lock<boost::shared_mutex> lock2(m_destinationMutex);
    m_destination = nullptr;
    lock2.unlock();
}

int32_t AudioChannel::Encoded(webrtc::FrameType frameType, uint8_t payloadType,
                            uint32_t timeStamp, const uint8_t* payloadData,
                            uint16_t payloadSize)
{
    if (payloadSize > 0 && m_destination) {
        FrameFormat format = FRAME_FORMAT_UNKNOWN;
        woogeen_base::Frame frame;
        memset(&frame, 0, sizeof(frame));
        switch (payloadType) {
        case PCMU_8000_PT:
            format = FRAME_FORMAT_PCMU;
            frame.additionalInfo.audio.channels = 1; // FIXME: retrieve codec info from VOE?
            frame.additionalInfo.audio.sampleRate = 8000;
            break;
        case PCMA_8000_PT:
            format = FRAME_FORMAT_PCMA;
            frame.additionalInfo.audio.channels = 1;
            frame.additionalInfo.audio.sampleRate = 8000;
            break;
        case ISAC_16000_PT:
            format = FRAME_FORMAT_ISAC16;
            frame.additionalInfo.audio.channels = 1; // FIXME: retrieve codec info from VOE?
            frame.additionalInfo.audio.sampleRate = 16000;
            break;
        case ISAC_32000_PT:
            format = FRAME_FORMAT_ISAC32;
            frame.additionalInfo.audio.channels = 1;
            frame.additionalInfo.audio.sampleRate = 32000;
            break;
        case OPUS_48000_PT:
            format = FRAME_FORMAT_OPUS;
            frame.additionalInfo.audio.channels = 2;
            frame.additionalInfo.audio.sampleRate = 48000;
            break;
        default:
            break;
        }

        frame.format = format;
        frame.payload = const_cast<uint8_t*>(payloadData);
        frame.length = payloadSize;
        frame.timeStamp = timeStamp;
        deliverFrame(frame);
    }

    return 0;
}

void AudioChannel::Process(int channelId, webrtc::ProcessingTypes type,
    int16_t data[], size_t length,
    int sampleRate, bool isStereo)
{
    if (m_destination) {
        int channels = isStereo ? 2 : 1;

        woogeen_base::Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = woogeen_base::FRAME_FORMAT_PCM_RAW;
        frame.payload = reinterpret_cast<uint8_t*>(data);
        frame.length = length * channels * sizeof(int16_t);
        frame.timeStamp = 0; // unused.
        frame.additionalInfo.audio.nbSamples = length;
        frame.additionalInfo.audio.sampleRate = sampleRate;
        frame.additionalInfo.audio.channels = channels;

        deliverFrame(frame);
    }
}

void AudioChannel::onFeedback(const woogeen_base::FeedbackMsg& msg)
{
    //TODO: complete this logic later.
}

static int frameFormat2PTC(woogeen_base::FrameFormat format) {
    switch (format) {
    case woogeen_base::FRAME_FORMAT_PCMU:
        return PCMU_8000_PT;
    case woogeen_base::FRAME_FORMAT_PCMA:
        return PCMA_8000_PT;
    case woogeen_base::FRAME_FORMAT_ISAC16:
        return ISAC_16000_PT;
    case woogeen_base::FRAME_FORMAT_ISAC32:
        return ISAC_32000_PT;
    case woogeen_base::FRAME_FORMAT_OPUS:
        return OPUS_48000_PT;
    default:
        return INVALID_PT;
    }
}

void AudioChannel::onFrame(const Frame& frame)
{
    // FIXME: Temporarily use Frame to carry rtp-packets due to the premature AudioFrameConstructor implementation.
    if (frame.additionalInfo.audio.isRtpPacket) {

        RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(frame.payload);
        uint8_t packetType = chead->getPacketType();

        VoENetwork* network = VoENetwork::GetInterface(m_engine);

        if (packetType == RTCP_Sender_PT) {
            network->ReceivedRTCPPacket(m_channel, frame.payload, frame.length);
            return;
        }

        network->ReceivedRTPPacket(m_channel, frame.payload, frame.length);
        return;
    }

    // FIXME: Temporarily convert audio frame to rtp-packets due to the premature AudioFrameConstructor implementation.
    char buf[MAX_DATA_PACKET_SIZE];
    uint32_t len = m_converter.convert(frame, buf);
    VoENetwork* network = VoENetwork::GetInterface(m_engine);
    network->ReceivedRTPPacket(m_channel, buf, len);
}

static bool fillAudioCodec(FrameFormat format, CodecInst& audioCodec)
{
    bool ret = true;
    audioCodec.pltype = frameFormat2PTC(format);
    switch (format) {
    case woogeen_base::FRAME_FORMAT_PCMU:
        strcpy(audioCodec.plname, "PCMU");
        audioCodec.plfreq = 8000;
        audioCodec.pacsize = 160;
        audioCodec.channels = 1;
        audioCodec.rate = 64000;
        break;
    case woogeen_base::FRAME_FORMAT_PCMA:
        strcpy(audioCodec.plname, "PCMA");
        audioCodec.plfreq = 8000;
        audioCodec.pacsize = 160;
        audioCodec.channels = 1;
        audioCodec.rate = 64000;
        break;
    case woogeen_base::FRAME_FORMAT_ISAC16:
        strcpy(audioCodec.plname, "ISAC");
        audioCodec.plfreq = 16000;
        audioCodec.pacsize = 480;
        audioCodec.channels = 1;
        audioCodec.rate = 32000;
        break;
    case woogeen_base::FRAME_FORMAT_ISAC32:
        strcpy(audioCodec.plname, "ISAC");
        audioCodec.plfreq = 32000;
        audioCodec.pacsize = 960;
        audioCodec.channels = 1;
        audioCodec.rate = 56000;
        break;
    case woogeen_base::FRAME_FORMAT_OPUS:
        strcpy(audioCodec.plname, "opus");
        audioCodec.plfreq = 48000;
        audioCodec.pacsize = 960;
        audioCodec.channels = 2;
        audioCodec.rate = 64000;
        break;
    case woogeen_base::FRAME_FORMAT_PCM_RAW:
        audioCodec.pltype = OPUS_48000_PT;
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

#define global_ns

bool AudioChannel::setOutput(FrameFormat format, woogeen_base::FrameDestination* destination)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_destinationMutex);
    if (m_destination) {
        //TODO: log warning here.
        return false;
    }

    VoEBase* voe = VoEBase::GetInterface(m_engine);
    VoECodec* codec = VoECodec::GetInterface(m_engine);
    VoEExternalMedia* externalMedia = VoEExternalMedia::GetInterface(m_engine);

    VoEAudioProcessing* apm = VoEAudioProcessing::GetInterface(m_engine);
    apm->SetRxNsStatus(m_channel, true, kNsHighSuppression);

    CodecInst audioCodec;
    bool validCodec = fillAudioCodec(format, audioCodec);

    if (!validCodec || codec->SetSendCodec(m_channel, audioCodec) == -1) {
        //TODO: log error here.
        return false;
    }

    if (format != woogeen_base::FRAME_FORMAT_PCM_RAW) {
        voe->RegisterPostEncodeFrameCallback(m_channel, this);
    } else {
        externalMedia->RegisterExternalMediaProcessing(m_channel, kRecordingPerChannel, *this);
    }

    if (voe->StartSend(m_channel) != -1){
        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
        m_outFormat = format;
        this->addAudioDestination(destination);
        m_destination = destination;
        return true;
    }

    externalMedia->DeRegisterExternalMediaProcessing(m_channel, kRecordingPerChannel);
    voe->DeRegisterPostEncodeFrameCallback(m_channel);
    return false;
}

void AudioChannel::unsetOutput()
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_destinationMutex);
    if (m_destination) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
        VoEExternalMedia* externalMedia = VoEExternalMedia::GetInterface(m_engine);
        externalMedia->DeRegisterExternalMediaProcessing(m_channel, kRecordingPerChannel);
        VoEBase* voe = VoEBase::GetInterface(m_engine);
        voe->DeRegisterPostEncodeFrameCallback(m_channel);
        voe->StopSend(m_channel);
        this->removeAudioDestination(m_destination);
        m_destination = nullptr;
    }
}

bool AudioChannel::setInput(woogeen_base::FrameSource* source)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_sourceMutex);
    if (m_source) {
        //TODO: log warning here.
        return false;
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
    VoEBase* voe = VoEBase::GetInterface(m_engine);

    voe->StartPlayout(m_channel);
    voe->StartReceive(m_channel);
    m_source = source;
    source->addAudioDestination(this);
    return true;
}

void AudioChannel::unsetInput()
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_sourceMutex);
    if (m_source) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
        m_transport->setFeedbackSink(nullptr);
        VoEBase* voe = VoEBase::GetInterface(m_engine);

        voe->StopPlayout(m_channel);
        voe->StopReceive(m_channel);
        m_source->removeAudioDestination(this);
        m_source = nullptr;
    }
}

bool AudioChannel::isIdle()
{
    bool noInput = false, noOutput = false;

    boost::shared_lock<boost::shared_mutex> lock1(m_sourceMutex);
    noInput = ! m_source;
    lock1.unlock();

    boost::shared_lock<boost::shared_mutex> lock2(m_destinationMutex);
    noOutput = ! m_destination;
    lock2.unlock();

    return noInput && noOutput;
}

void AudioChannel::performMix(bool isCommon)
{
    VoECodec* codec = VoECodec::GetInterface(m_engine);
    CodecInst audioCodec;
    VoEBase* voe = VoEBase::GetInterface(m_engine);
    AudioTransport* audioTransport = voe->audio_transport();
    int16_t data[AudioFrame::kMaxDataSizeSamples];
    uint32_t nSamplesOut = 0;

    int64_t elapsedTimeInMs, ntpTimeInMs;
    if (codec->GetSendCodec(m_channel, audioCodec) != -1) {
        if (audioTransport->NeedMorePlayData(
            audioCodec.plfreq / 1000 * 10, // samples per channel in a 10 ms period. FIXME: hard coded timer interval.
            0,
            audioCodec.channels,
            audioCodec.plfreq,
            data,
            nSamplesOut,
            &elapsedTimeInMs,
            &ntpTimeInMs,
            isCommon,
            m_channel) == 0)
            audioTransport->OnData(m_channel, data, 0, audioCodec.plfreq, audioCodec.channels, nSamplesOut);
    }
}

} /* namespace mcu */
