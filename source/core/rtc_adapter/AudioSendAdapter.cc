// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioSendAdapter.h"
#include "AudioUtilitiesNew.h"
#include "TaskRunnerPool.h"

#include <rtc_base/logging.h>
#include <modules/rtp_rtcp/source/rtp_packet.h>

#include <rtputils.h>

using namespace webrtc;
using namespace owt_base;

namespace rtc_adapter {

const uint32_t kSeqNoStep = 10;

AudioSendAdapterImpl::AudioSendAdapterImpl(CallOwner* owner, const RtcAdapter::Config& config)
    : m_frameFormat(FRAME_FORMAT_UNKNOWN)
    , m_lastOriginSeqNo(0)
    , m_seqNo(0)
    , m_ssrc(0)
    , m_ssrc_generator(SsrcGenerator::GetSsrcGenerator())
    , m_config(config)
    , m_rtpListener(config.rtp_listener)
    , m_statsListener(config.stats_listener)
{
    m_ssrc = m_ssrc_generator->CreateSsrc();
    m_ssrc_generator->RegisterSsrc(m_ssrc);
    m_taskRunner = TaskRunnerPool::GetInstance().GetTaskRunner();
    init();
}

AudioSendAdapterImpl::~AudioSendAdapterImpl()
{
    close();
    m_ssrc_generator->ReturnSsrc(m_ssrc);
    boost::unique_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
}

int AudioSendAdapterImpl::onRtcpData(char* data, int len)
{
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(data), len);
    return len;
}

void AudioSendAdapterImpl::onFrame(const Frame& frame)
{
    if (frame.length <= 0)
        return;

    if (frame.format != m_frameFormat) {
        m_frameFormat = frame.format;
        setSendCodec(m_frameFormat);
    }

    if (frame.additionalInfo.audio.isRtpPacket) {
        // FIXME: Temporarily use Frame to carry rtp-packets
        // due to the premature AudioFrameConstructor implementation.
        updateSeqNo(frame.payload);
        if (m_rtpListener) {
            if (!m_mid.empty()) {
                webrtc::RtpPacket packet(&m_extensions);
                packet.Parse(frame.payload, frame.length);
                packet.SetExtension<webrtc::RtpMid>(m_mid);
                m_rtpListener->onAdapterData(
                    reinterpret_cast<char*>(const_cast<uint8_t*>(packet.data())), packet.size());
            } else {
                m_rtpListener->onAdapterData(
                    reinterpret_cast<char*>(frame.payload), frame.length);
            }
        }

    } else {
        int payloadType = getAudioPltype(frame.format);
        if (payloadType != INVALID_PT) {
            boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
            if (m_rtpRtcp->OnSendingRtpFrame(frame.timeStamp, -1, payloadType, false)) {
                const uint32_t rtp_timestamp = frame.timeStamp + m_rtpRtcp->StartTimestamp();
                // TODO: The frame type information is lost.
                // We treat every frame a kAudioFrameSpeech frame for now.
                if (!m_senderAudio->SendAudio(webrtc::AudioFrameType::kAudioFrameSpeech,
                        payloadType, rtp_timestamp,
                        frame.payload, frame.length)) {
                    RTC_DLOG(LS_ERROR) << "ChannelSend failed to send data to RTP/RTCP module";
                }
            } else {
                RTC_DLOG(LS_WARNING) << "OnSendingRtpFrame return false";
            }
        }
    }
}

bool AudioSendAdapterImpl::init()
{
    m_clock = Clock::GetRealTimeClock();
    m_eventLog = std::make_unique<webrtc::RtcEventLogNull>();

    RtpRtcp::Configuration configuration;
    configuration.clock = m_clock;
    configuration.audio = true; // Audio.
    configuration.receiver_only = false;
    configuration.outgoing_transport = this;
    configuration.event_log = m_eventLog.get();
    configuration.local_media_ssrc = m_ssrc; //rtp_config.ssrcs[i];

    m_rtpRtcp = RtpRtcp::Create(configuration);
    m_rtpRtcp->SetSendingStatus(true);
    m_rtpRtcp->SetSendingMediaStatus(true);
    m_rtpRtcp->SetRTCPStatus(RtcpMode::kReducedSize);
    // Set NACK.
    m_rtpRtcp->SetStorePacketsStatus(true, 600);

    if (m_config.mid_ext) {
        m_config.mid[sizeof(m_config.mid) - 1] = '\0';
        std::string mid(m_config.mid);
        // Register MID extension
        m_rtpRtcp->RegisterRtpHeaderExtension(
            webrtc::RtpExtension::kMidUri, m_config.mid_ext);
        m_rtpRtcp->SetMid(mid);
        // TODO: Remove m_extensions if frames do not carry rtp
        m_extensions.Register<webrtc::RtpMid>(m_config.mid_ext);
        m_mid = mid;
    }
    m_senderAudio = std::make_unique<RTPSenderAudio>(
        configuration.clock, m_rtpRtcp->RtpSender());
    m_taskRunner->RegisterModule(m_rtpRtcp.get());

    return true;
}

bool AudioSendAdapterImpl::setSendCodec(FrameFormat format)
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

void AudioSendAdapterImpl::close()
{
    m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
}

bool AudioSendAdapterImpl::SendRtp(const uint8_t* packet,
    size_t length,
    const webrtc::PacketOptions& options)
{
    if (m_rtpListener) {
        m_rtpListener->onAdapterData(
            reinterpret_cast<char*>(const_cast<uint8_t*>(packet)), length);
        return true;
    }
    return false;
}

bool AudioSendAdapterImpl::SendRtcp(const uint8_t* packet, size_t length)
{
    const RTCPHeader* chead = reinterpret_cast<const RTCPHeader*>(packet);
    uint8_t packetType = chead->getPacketType();
    if (packetType == RTCP_Sender_PT) {
        if (m_rtpListener) {
            m_rtpListener->onAdapterData(
                reinterpret_cast<char*>(const_cast<uint8_t*>(packet)), length);
            return true;
        }
    }
    return false;
}

void AudioSendAdapterImpl::updateSeqNo(uint8_t* rtp)
{
    uint16_t originSeqNo = *(reinterpret_cast<uint16_t*>(&rtp[2]));
    if ((m_lastOriginSeqNo == m_seqNo) && (m_seqNo == 0)) {
    } else {
        uint16_t step = ((originSeqNo < m_lastOriginSeqNo) ? (UINT16_MAX - m_lastOriginSeqNo + originSeqNo + 1) : (originSeqNo - m_lastOriginSeqNo));
        if ((step == 1) || (step > kSeqNoStep)) {
            m_seqNo += 1;
        } else {
            m_seqNo += step;
        }
    }
    m_lastOriginSeqNo = originSeqNo;
    *(reinterpret_cast<uint16_t*>(&rtp[2])) = htons(m_seqNo);
}
}
