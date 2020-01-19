// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioFramePacketizer.h"
#include "AudioUtilities.h"
#include "WebRTCTaskRunner.h"

#include <rtc_base/logging.h>

using namespace webrtc;

namespace owt_base {

DEFINE_LOGGER(AudioFramePacketizer, "owt.AudioFramePacketizer");

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
    m_taskRunner.reset(new owt_base::WebRTCTaskRunner("AudioFramePacketizer"));
    m_taskRunner->Start();
    init();
}

AudioFramePacketizer::~AudioFramePacketizer()
{
    close();
    m_taskRunner->Stop();
    m_ssrc_generator->ReturnSsrc(m_ssrc);
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
    m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(data_packet->data), data_packet->length);
    return data_packet->length;
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
    if (!m_rtpRtcp->OnSendingRtpFrame(frame.timeStamp, -1, payloadType, false)) {
        RTC_DLOG(LS_WARNING) << "OnSendingRtpFrame return false";
        return;
    }
    const uint32_t rtp_timestamp = frame.timeStamp + m_rtpRtcp->StartTimestamp();
    // TODO: The frame type information is lost. We treat every frame a kAudioFrameSpeech frame for now.
    if (!m_senderAudio->SendAudio(webrtc::AudioFrameType::kAudioFrameSpeech, payloadType, rtp_timestamp,
                                  frame.payload, frame.length)) {
        RTC_DLOG(LS_ERROR) << "ChannelSend::SendData() failed to send data to RTP/RTCP module";
    }
}

bool AudioFramePacketizer::init()
{
    m_clock = Clock::GetRealTimeClock();
    m_event_log = std::make_unique<webrtc::RtcEventLogNull>();

    RtpRtcp::Configuration configuration;
    configuration.clock = m_clock;
    configuration.audio = true;  // Audio.
    configuration.receiver_only = false;
    configuration.outgoing_transport = m_audioTransport.get();
    configuration.event_log = m_event_log.get();
    configuration.local_media_ssrc = m_ssrc;//rtp_config.ssrcs[i];

    m_rtpRtcp = RtpRtcp::Create(configuration);
    m_rtpRtcp->SetSendingStatus(true);
    m_rtpRtcp->SetSendingMediaStatus(true);
    m_rtpRtcp->SetRTCPStatus(RtcpMode::kReducedSize);
    // Set NACK.
    m_rtpRtcp->SetStorePacketsStatus(true, 600);

    m_senderAudio = std::make_unique<RTPSenderAudio>(
        configuration.clock, m_rtpRtcp->RtpSender());
    m_taskRunner->RegisterModule(m_rtpRtcp.get());

    return true;
}

bool AudioFramePacketizer::setSendCodec(FrameFormat format)
{
    CodecInst codec;

    if (!getAudioCodecInst(m_frameFormat, codec))
        return false;

    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    m_rtpRtcp->RegisterSendPayloadFrequency(codec.pltype, codec.plfreq);
    m_senderAudio->RegisterAudioPayload("audio", codec.pltype,
                                        codec.plfreq, codec.channels, 0);
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
