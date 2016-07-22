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

#include "VideoFrameConstructor.h"

#include "WebRTCTaskRunner.h"
#include <rtputils.h>

using namespace webrtc;
using namespace erizo;

namespace woogeen_base {

DEFINE_LOGGER(VideoFrameConstructor, "woogeen.VideoFrameConstructor");

VideoFrameConstructor::VideoFrameConstructor(erizo::FeedbackSink* feedback)
    : m_format(FRAME_FORMAT_UNKNOWN)
    , m_width(0)
    , m_height(0)
    , m_ssrc(0)
    , m_vcm(nullptr)
{
    m_videoTransport.reset(new WebRTCTransport<erizo::VIDEO>(nullptr, feedback));
    m_taskRunner.reset(new woogeen_base::WebRTCTaskRunner());
    m_taskRunner->Start();
    init();
}

VideoFrameConstructor::~VideoFrameConstructor()
{
    m_videoReceiver->StopReceive();
    m_recorder->Stop();

    if (m_taskRunner) {
        m_taskRunner->DeRegisterModule(m_avSync.get());
        m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
        m_taskRunner->DeRegisterModule(m_vcm);
    }
    m_taskRunner->Stop();

    if (m_vcm) {
        VideoCodingModule::Destroy(m_vcm);
        m_vcm = nullptr;
    }
}

bool VideoFrameConstructor::init()
{
    m_vcm = VideoCodingModule::Create();
    if (m_vcm)
        m_vcm->InitializeReceiver();
    else
        return false;

    // FIXME: Now we just provide a DummyRemoteBitrateObserver to satisfy the requirement of our components.
    // We need to investigate whether this is necessary or not in our case, so that we can decide whether to
    // provide a real RemoteBitrateObserver.
    m_remoteBitrateObserver.reset(new DummyRemoteBitrateObserver());
    m_remoteBitrateEstimator.reset(RemoteBitrateEstimatorFactory().Create(m_remoteBitrateObserver.get(), Clock::GetRealTimeClock(), kMimdControl, 0));
    m_videoReceiver.reset(new ViEReceiver(0, m_vcm, m_remoteBitrateEstimator.get(), this));

    RtpRtcp::Configuration configuration;
    configuration.id = 0;
    configuration.audio = false;  // Video.
    configuration.outgoing_transport = m_videoTransport.get(); // For sending RTCP feedback to the publisher
    configuration.remote_bitrate_estimator = m_remoteBitrateEstimator.get();
    configuration.receive_statistics = m_videoReceiver->GetReceiveStatistics();
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));
    m_rtpRtcp->SetRTCPStatus(webrtc::kRtcpCompound);
    // There're 3 options of Intra frame requests: PLI, FIR in RTCP and FIR in RTP (RFC 2032).
    // Since currently our MCU only claims FIR support in SDP, we choose FirRtcp for now.
    m_rtpRtcp->SetKeyFrameRequestMethod(kKeyFrameReqFirRtcp);

    m_videoReceiver->SetRtpRtcpModule(m_rtpRtcp.get());

    // Register codec.
    VideoCodec video_codec;
    if (VideoCodingModule::Codec(webrtc::kVideoCodecVP8, &video_codec) != VCM_OK
        || m_vcm->RegisterReceiveCodec(&video_codec, 1, true) != VCM_OK)
        assert(!"register VP8 decoder failed");
    m_videoReceiver->SetReceiveCodec(video_codec);

    if (VideoCodingModule::Codec(webrtc::kVideoCodecH264, &video_codec) != VCM_OK
        || m_vcm->RegisterReceiveCodec(&video_codec, 1, true) != VCM_OK)
        assert(!"register H264 decoder failed");
    m_videoReceiver->SetReceiveCodec(video_codec);

    memset(&video_codec, 0, sizeof(VideoCodec));
    video_codec.codecType = webrtc::kVideoCodecRED;
    strcpy(video_codec.plName, "red");
    video_codec.plType = RED_90000_PT;
    m_videoReceiver->SetReceiveCodec(video_codec);

    memset(&video_codec, 0, sizeof(VideoCodec));
    video_codec.codecType = webrtc::kVideoCodecULPFEC;
    strcpy(video_codec.plName, "ulpfec");
    video_codec.plType = ULP_90000_PT;
    m_videoReceiver->SetReceiveCodec(video_codec);

    // Enable NACK.
    // TODO: the parameters should be dynamically adjustable.
    m_videoReceiver->SetNackStatus(true, webrtc::kMaxPacketAgeToNack);
    m_vcm->SetVideoProtection(webrtc::kProtectionNackReceiver, true);
    m_vcm->SetNackSettings(webrtc::kMaxNackListSize, webrtc::kMaxPacketAgeToNack, 0);
    m_vcm->RegisterPacketRequestCallback(this);

    // Register the key frame request callback.
    m_vcm->RegisterFrameTypeCallback(this);

    m_avSync.reset(new ViESyncModule(m_vcm, 0));
    m_recorder.reset(new DebugRecorder());
//    m_recorder->Start("webrtc.frame.i420");

    m_taskRunner->RegisterModule(m_vcm);
    m_taskRunner->RegisterModule(m_rtpRtcp.get());
    m_videoReceiver->StartReceive();
    return true;
}

void VideoFrameConstructor::syncWithAudio(int32_t voiceChannelId, VoEVideoSync* voe)
{
    if (m_avSync) {
        m_avSync->ConfigureSync(voiceChannelId, voe, m_rtpRtcp.get(), m_videoReceiver->GetRtpReceiver());
        m_taskRunner->RegisterModule(m_avSync.get());
    }
}

bool VideoFrameConstructor::setBitrate(uint32_t kbps)
{
    if (!m_ssrc)
        return false;

    m_rtpRtcp->SetREMBStatus(true);
    std::vector<uint32_t> ssrcs;
    ssrcs.push_back(m_ssrc);
    m_rtpRtcp->SetREMBData(kbps * 1000, ssrcs);
    return true;
}

int32_t VideoFrameConstructor::ResendPackets(const uint16_t* sequenceNumbers, uint16_t length)
{
    return m_rtpRtcp->SendNACK(sequenceNumbers, length);
}

int32_t VideoFrameConstructor::RequestKeyFrame()
{
    return m_rtpRtcp->RequestKeyFrame();
}

int32_t VideoFrameConstructor::OnInitializeDecoder(
    const int32_t id,
    const int8_t payload_type,
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const int frequency,
    const uint8_t channels,
    const uint32_t rate)
{
    m_vcm->RegisterExternalDecoder(this, payload_type, true);
    m_vcm->ResetDecoder();
    return 0;
}

void VideoFrameConstructor::OnIncomingSSRCChanged(const int32_t id, const uint32_t ssrc)
{
    m_rtpRtcp->SetRemoteSSRC(ssrc);
    m_ssrc = ssrc;
}

void VideoFrameConstructor::ResetStatistics(uint32_t ssrc)
{
    StreamStatistician* statistician = m_videoReceiver->GetReceiveStatistics()->GetStatistician(ssrc);
    if (statistician)
        statistician->ResetStatistics();
}

int32_t VideoFrameConstructor::InitDecode(const webrtc::VideoCodec* codecSettings, int32_t numberOfCores)
{
    assert(codecSettings->codecType == webrtc::kVideoCodecVP8 || codecSettings->codecType == webrtc::kVideoCodecH264);
    if (codecSettings->codecType == webrtc::kVideoCodecVP8)
        m_format = woogeen_base::FRAME_FORMAT_VP8;
    else if (codecSettings->codecType == webrtc::kVideoCodecH264)
        m_format = woogeen_base::FRAME_FORMAT_H264;

    return 0;
}

int32_t VideoFrameConstructor::Decode(const webrtc::EncodedImage& encodedImage,
                       bool missingFrames,
                       const webrtc::RTPFragmentationHeader* fragmentation,
                       const webrtc::CodecSpecificInfo* codecSpecificInfo,
                       int64_t renderTimeMs)
{
    FrameFormat format = FRAME_FORMAT_UNKNOWN;
    bool resolutionChanged = false;

    if (encodedImage._length > 0) {
        switch (codecSpecificInfo->codecType) {
        case webrtc::kVideoCodecVP8:
            format = FRAME_FORMAT_VP8;
            break;
        case webrtc::kVideoCodecH264:
            format = FRAME_FORMAT_H264;
            break;
        default:
            return 0;
        }

        if (encodedImage._encodedWidth != 0 && m_width != encodedImage._encodedWidth) {
            m_width = encodedImage._encodedWidth;
            resolutionChanged = true;
        };

        if (encodedImage._encodedHeight != 0 && m_height != encodedImage._encodedHeight) {
            m_height = encodedImage._encodedHeight;
            resolutionChanged = true;
        };

        if (resolutionChanged) {
            ELOG_INFO("received video resolution has changed to %ux%u", m_width, m_height);
            //TODO: Notify the controlling layer about the resolution change.
        }

        if (encodedImage._frameType == webrtc::kKeyFrame) {
            //ELOG_INFO("got a key frame");
        }

        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = format;
        frame.payload = encodedImage._buffer;
        frame.length = encodedImage._length;
        frame.timeStamp = encodedImage._timeStamp;
        frame.additionalInfo.video.width = m_width;
        frame.additionalInfo.video.height = m_height;

        deliverFrame(frame);
    }
    return 0;
}

int VideoFrameConstructor::deliverVideoData(char* buf, int len)
{
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT)
        return m_videoReceiver->ReceivedRTCPPacket(buf, len) == -1 ? 0 : len;

    PacketTime current;
    if (m_videoReceiver->ReceivedRTPPacket(buf, len, current) != -1) {
        // FIXME: Decode should be invoked as often as possible.
        // To invoke it in current work thread is not a good idea. We may need to create
        // a dedicated thread to keep on invoking vcm Decode, and, with a wait time other than 0.
        // (Default wait time used by webrtc vie engine is 50ms).
        int32_t ret = m_vcm->Decode(0);
        ELOG_TRACE("receivedRtpData decode result = %d",  ret);
        return len;
    }

    return 0;
}

int VideoFrameConstructor::deliverAudioData(char* buf, int len)
{
    assert(false);
    return 0;
}

void VideoFrameConstructor::onFeedback(const FeedbackMsg& msg)
{
    if (msg.type == woogeen_base::VIDEO_FEEDBACK) {
        if (msg.cmd == REQUEST_KEY_FRAME) {
            this->RequestKeyFrame();
        } else if (msg.cmd == SET_BITRATE) {
            this->setBitrate(msg.data.kbps);
        }
    }
}

}
