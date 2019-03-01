// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioFramePacketizer.h"
#include "AudioUtilities.h"

#include "WebRTCTaskRunner.h"

using namespace webrtc;

namespace woogeen_base {

DEFINE_LOGGER(AudioFramePacketizer, "woogeen.AudioFramePacketizer");

AudioFramePacketizer::AudioFramePacketizer()
    : m_enabled(true)
    , m_frameFormat(FRAME_FORMAT_UNKNOWN)
    , m_lastOriginSeqNo(0)
    , m_seqNo(0)
    , m_ssrc(0)
    , m_ssrc_generator(SsrcGenerator::GetSsrcGenerator())
{
    audio_sink_ = nullptr;
    m_ssrc = m_ssrc_generator->CreateSsrc();
    m_ssrc_generator->RegisterSsrc(m_ssrc);
    m_audioTransport.reset(new WebRTCTransport<erizoExtra::AUDIO>(this, nullptr));
    m_taskRunner.reset(new woogeen_base::WebRTCTaskRunner("AudioFramePacketizer"));
    m_taskRunner->Start();
    init();
}

AudioFramePacketizer::~AudioFramePacketizer()
{
    close();
    m_taskRunner->Stop();
    m_ssrc_generator->ReturnSsrc(m_ssrc);
    SsrcGenerator::ReturnSsrcGenerator();
    boost::unique_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
}

void AudioFramePacketizer::bindTransport(erizo::MediaSink* sink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    audio_sink_ = sink;
    audio_sink_->setAudioSinkSSRC(m_rtpRtcp->SSRC());
    erizo::FeedbackSource* fbSource = audio_sink_->getFeedbackSource();
    if (fbSource)
        fbSource->setFeedbackSink(this);
}

void AudioFramePacketizer::unbindTransport()
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (audio_sink_) {
        audio_sink_ = nullptr;
    }
}

int AudioFramePacketizer::deliverFeedback_(std::shared_ptr<erizo::DataPacket> data_packet)
{
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    return m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(data_packet->data), data_packet->length) == -1 ? 0 : data_packet->length;
}

void AudioFramePacketizer::receiveRtpData(char* buf, int len, erizoExtra::DataType type, uint32_t channelId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (!audio_sink_) {
        return;
    }

    assert(type == erizoExtra::AUDIO);
    audio_sink_->deliverAudioData(std::make_shared<erizo::DataPacket>(0, buf, len, erizo::AUDIO_PACKET));
}


void AudioFramePacketizer::onFrame(const Frame& frame)
{
    if (!m_enabled) {
        return;
    }

    boost::shared_lock<boost::shared_mutex> lock1(m_transport_mutex);
    if (!audio_sink_) {
        return;
    }

    if (frame.length <= 0)
        return;

    if (frame.format != m_frameFormat) {
        m_frameFormat = frame.format;
        setSendCodec(m_frameFormat);
    }

    if (frame.additionalInfo.audio.isRtpPacket) { // FIXME: Temporarily use Frame to carry rtp-packets due to the premature AudioFrameConstructor implementation.
        updateSeqNo(frame.payload);
        audio_sink_->deliverAudioData(std::make_shared<erizo::DataPacket>(0, reinterpret_cast<char*>(frame.payload), frame.length, erizo::AUDIO_PACKET));
        return;
    }
    lock1.unlock();

    int payloadType = getAudioPltype(frame.format);
    if (payloadType == INVALID_PT)
        return;

    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    // FIXME: The frame type information is lost. We treat every frame a kAudioFrameSpeech frame for now.
    m_rtpRtcp->SendOutgoingData(webrtc::kAudioFrameSpeech, payloadType, frame.timeStamp, -1, frame.payload, frame.length, nullptr, nullptr, nullptr);
}

bool AudioFramePacketizer::init()
{
    RtpRtcp::Configuration configuration;
    configuration.outgoing_transport = m_audioTransport.get();
    configuration.audio = true;  // Audio.
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));
    m_rtpRtcp->SetSSRC(m_ssrc);

    // Enable NACK.
    // TODO: the parameters should be dynamically adjustable.
    m_rtpRtcp->SetStorePacketsStatus(true, 600);

    m_taskRunner->RegisterModule(m_rtpRtcp.get());

    return true;
}

bool AudioFramePacketizer::setSendCodec(FrameFormat format)
{
    webrtc::CodecInst codec;

    if (!getAudioCodecInst(m_frameFormat, codec))
        return false;

    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    m_rtpRtcp->RegisterSendPayload(codec);
    return true;
}

void AudioFramePacketizer::close()
{
    unbindTransport();
    m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
}

void AudioFramePacketizer::updateSeqNo(uint8_t* rtp) {
    uint16_t originSeqNo = *(reinterpret_cast<uint16_t*>(&rtp[2]));
    if ((m_lastOriginSeqNo == m_seqNo) && (m_seqNo == 0)) {
    } else {
        uint16_t step = ((originSeqNo < m_lastOriginSeqNo) ? (UINT16_MAX - m_lastOriginSeqNo + originSeqNo + 1) : (originSeqNo - m_lastOriginSeqNo));
        if ((step == 1) || (step > 10)) {
          m_seqNo += 1;
        } else {
          m_seqNo += step;
        }
    }
    m_lastOriginSeqNo = originSeqNo;
    *(reinterpret_cast<uint16_t*>(&rtp[2])) = htons(m_seqNo);
}

int AudioFramePacketizer::sendPLI() {
    return 0;
}

}
