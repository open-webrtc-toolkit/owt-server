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

#include "AudioFramePacketizer.h"
#include "MediaUtilities.h"

#include "WebRTCTaskRunner.h"

using namespace webrtc;
using namespace erizo;

namespace woogeen_base {

AudioFramePacketizer::AudioFramePacketizer()
    : m_frameFormat(FRAME_FORMAT_UNKNOWN)
{
    audioSink_ = nullptr;
    m_audioTransport.reset(new WebRTCTransport<erizo::AUDIO>(this, nullptr));
    m_taskRunner.reset(new woogeen_base::WebRTCTaskRunner());
    m_taskRunner->Start();
    init();
}

AudioFramePacketizer::~AudioFramePacketizer()
{
    close();
    m_taskRunner->Stop();
    boost::unique_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
}

void AudioFramePacketizer::bindTransport(erizo::MediaSink* sink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    audioSink_ = sink;
    audioSink_->setAudioSinkSSRC(m_rtpRtcp->SSRC());
    erizo::FeedbackSource* fbSource = audioSink_->getFeedbackSource();
    if (fbSource)
        fbSource->setFeedbackSink(this);
}

void AudioFramePacketizer::unbindTransport()
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (audioSink_) {
        erizo::FeedbackSource* fbSource = audioSink_->getFeedbackSource();
        if (fbSource)
            fbSource->setFeedbackSink(nullptr);
        audioSink_ = nullptr;
    }
}

int AudioFramePacketizer::deliverFeedback(char* buf, int len)
{
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    return m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(buf), len) == -1 ? 0 : len;
}

void AudioFramePacketizer::receiveRtpData(char* buf, int len, erizo::DataType type, uint32_t channelId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (!audioSink_) {
        return;
    }

    assert(type == erizo::AUDIO);
    audioSink_->deliverAudioData(buf, len);
}


void AudioFramePacketizer::onFrame(const Frame& frame)
{
    boost::shared_lock<boost::shared_mutex> lock1(m_transport_mutex);
    if (!audioSink_) {
        return;
    }

    if (frame.length <= 0)
        return;

    if (frame.format != m_frameFormat) {
        m_frameFormat = frame.format;
        setSendCodec(m_frameFormat);
    }

    if (frame.additionalInfo.audio.isRtpPacket) { // FIXME: Temporarily use Frame to carry rtp-packets due to the premature AudioFrameConstructor implementation.
        audioSink_->deliverAudioData(reinterpret_cast<char*>(frame.payload), frame.length);
        return;
    }
    lock1.unlock();

    int payloadType = INVALID_PT;

    // TODO: A static map between the FrameFormat and the payload type.
    switch (frame.format) {
    case FRAME_FORMAT_PCMU:
        payloadType = PCMU_8000_PT;
        break;
    case FRAME_FORMAT_PCMA:
        payloadType = PCMA_8000_PT;
        break;
    case FRAME_FORMAT_OPUS:
        payloadType = OPUS_48000_PT;
        break;
    default:
        return;
    }
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    // FIXME: The frame type information is lost. We treat every frame a kAudioFrameSpeech frame for now.
    m_rtpRtcp->SendOutgoingData(webrtc::kAudioFrameSpeech, payloadType, frame.timeStamp, -1, frame.payload, frame.length /*, RTPFragementationHeader* */);
}

bool AudioFramePacketizer::init()
{
    RtpRtcp::Configuration configuration;
    // configuration.id = m_id;
    configuration.outgoing_transport = m_audioTransport.get();
    configuration.audio = true;  // Audio.
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));

    // Enable NACK.
    // TODO: the parameters should be dynamically adjustable.
    m_rtpRtcp->SetStorePacketsStatus(true, 600);

    m_taskRunner->RegisterModule(m_rtpRtcp.get());

    return true;
}

bool AudioFramePacketizer::setSendCodec(FrameFormat format)
{
    webrtc::CodecInst codec;

    // TODO: A static map between the FrameFormat and the payload type.
    switch (m_frameFormat) {
    case FRAME_FORMAT_PCMU:
        strcpy(codec.plname, "PCMU");
        codec.pltype = PCMU_8000_PT;
        codec.plfreq = 8000;
        codec.pacsize = 160;
        codec.channels = 1;
        codec.rate = 64000;
        break;
    case FRAME_FORMAT_OPUS:
        strcpy(codec.plname, "opus");
        codec.pltype = OPUS_48000_PT;
        codec.plfreq = 48000;
        codec.pacsize = 960;
        codec.channels = 2;
        codec.rate = 64000;
        break;
    default:
        return false;
    }

    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    m_rtpRtcp->RegisterSendPayload(codec);
    return true;
}

void AudioFramePacketizer::close()
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (audioSink_) {
        erizo::FeedbackSource* fbSource = audioSink_->getFeedbackSource();
        if (fbSource)
            fbSource->setFeedbackSink(nullptr);
        audioSink_ = nullptr;
    }
    lock.unlock();
    m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
}

}
