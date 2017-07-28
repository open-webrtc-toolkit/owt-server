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

#include "VideoFrameConstructor.h"

#include "WebRTCTaskRunner.h"
#include <rtputils.h>
#include <webrtc/common_types.h>
#include <webrtc/modules/video_coding/timing.h>
#include "webrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_single_stream.h"

using namespace webrtc;
using namespace erizo;

namespace woogeen_base {

DEFINE_LOGGER(VideoFrameConstructor, "woogeen.VideoFrameConstructor");

VideoFrameConstructor::VideoFrameConstructor()
    : m_enabled(true)
    , m_format(FRAME_FORMAT_UNKNOWN)
    , m_width(0)
    , m_height(0)
    , m_ssrc(0)
    , m_video_receiver(nullptr)
    , m_transport(nullptr)
{
    m_videoTransport.reset(new WebRTCTransport<erizo::VIDEO>(nullptr, nullptr));
    sinkfbSource_ = m_videoTransport.get();
    m_taskRunner.reset(new woogeen_base::WebRTCTaskRunner("VideoFrameConstructor"));
    m_taskRunner->Start();
    init();
}

VideoFrameConstructor::~VideoFrameConstructor()
{
    m_remoteBitrateObserver->RemoveRembSender(m_rtpRtcp.get());
    m_videoReceiver->StopReceive();

    unbindTransport();

    if (m_taskRunner) {
        m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
        m_taskRunner->DeRegisterModule(m_video_receiver.get());;
    }
    m_taskRunner->Stop();
}

bool VideoFrameConstructor::init()
{
    // TODO: move to new jitter buffer implemetation(not ready yet).
    m_video_receiver.reset(new webrtc::vcm::VideoReceiver(Clock::GetRealTimeClock(), nullptr, nullptr, new VCMTiming(Clock::GetRealTimeClock(), nullptr), nullptr, nullptr));

    // FIXME: Now we just provide a DummyRemoteBitrateObserver to satisfy the requirement of our components.
    // We need to investigate whether this is necessary or not in our case, so that we can decide whether to
    // provide a real RemoteBitrateObserver.
    //m_remoteBitrateObserver.reset(new DummyRemoteBitrateObserver());
    m_remoteBitrateObserver.reset(new VieRemb(Clock::GetRealTimeClock()));
    m_remoteBitrateEstimator.reset(new RemoteBitrateEstimatorSingleStream(m_remoteBitrateObserver.get(), Clock::GetRealTimeClock()));
    m_videoReceiver.reset(new ViEReceiver(m_video_receiver.get(), m_remoteBitrateEstimator.get(), this));

    RtpRtcp::Configuration configuration;
    configuration.audio = false;  // Video.
    configuration.outgoing_transport = m_videoTransport.get(); // For sending RTCP feedback to the publisher
    configuration.remote_bitrate_estimator = m_remoteBitrateEstimator.get();
    configuration.receive_statistics = m_videoReceiver->GetReceiveStatistics();
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));
    m_rtpRtcp->SetRTCPStatus(webrtc::RtcpMode::kCompound);
    // Since currently our MCU only claims FIR support in SDP, we choose FirRtcp for now.
    m_rtpRtcp->SetKeyFrameRequestMethod(kKeyFrameReqFirRtcp);
    m_rtpRtcp->SetREMBStatus(true);
    m_videoReceiver->SetRtpRtcpModule(m_rtpRtcp.get());
    m_remoteBitrateObserver->AddRembSender(m_rtpRtcp.get());

    // Register codec.
    VideoCodec video_codec;
    VideoCodingModule::Codec(webrtc::kVideoCodecVP8, &video_codec);
    video_codec.plType = VP8_90000_PT;
    if (m_video_receiver->RegisterReceiveCodec(&video_codec, 1, true) != VCM_OK)
        assert(!"register VP8 decoder failed");
    m_videoReceiver->SetReceiveCodec(video_codec);

    VideoCodingModule::Codec(webrtc::kVideoCodecVP9, &video_codec);
    video_codec.plType = VP9_90000_PT;
    if (m_video_receiver->RegisterReceiveCodec(&video_codec, 1, true) != VCM_OK)
        assert(!"register VP9 decoder failed");
    m_videoReceiver->SetReceiveCodec(video_codec);

    VideoCodingModule::Codec(webrtc::kVideoCodecH264, &video_codec);
    video_codec.plType = H264_90000_PT;
    if (m_video_receiver->RegisterReceiveCodec(&video_codec, 1, true) != VCM_OK)
        assert(!"register H264 decoder failed");
    m_videoReceiver->SetReceiveCodec(video_codec);

#if 0
    memset(&video_codec, 0, sizeof(VideoCodec));
    video_codec.codecType = webrtc::kVideoCodecH265;
    strcpy(video_codec.plName, "H265");
    video_codec.plType = H265_90000_PT;
    m_vcm->RegisterReceiveCodec(&video_codec, 1, true);
    m_videoReceiver->SetReceiveCodec(video_codec);
#endif

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
    // The basic idea is to adjust according to bitrate.
    static const int kMaxPacketAgeToNack = 450;
    static const int kMaxNackListSize = 250;
    m_videoReceiver->SetNackStatus(true, kMaxPacketAgeToNack);
    m_video_receiver->SetVideoProtection(webrtc::kProtectionNack, true);
    m_video_receiver->SetNackSettings(kMaxNackListSize, kMaxPacketAgeToNack, 0);
    m_video_receiver->RegisterPacketRequestCallback(this);

    // Register the key frame request callback.
    m_video_receiver->RegisterFrameTypeCallback(this);
    m_video_receiver->RegisterReceiveCallback(this);

    m_taskRunner->RegisterModule(m_video_receiver.get());
    m_taskRunner->RegisterModule(m_rtpRtcp.get());
    m_videoReceiver->StartReceive();
    return true;
}

void VideoFrameConstructor::bindTransport(erizo::MediaSource* source, erizo::FeedbackSink* fbSink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    m_transport = source;
    m_transport->setVideoSink(this);
    m_videoTransport->setFeedbackSink(fbSink);
}

void VideoFrameConstructor::unbindTransport()
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (m_transport) {
        m_transport->setVideoSink(nullptr);
        m_videoTransport->setFeedbackSink(nullptr);
        m_transport = nullptr;
    }
}

void VideoFrameConstructor::enable(bool enabled)
{
    m_enabled = enabled;
    if (m_enabled) {
        m_rtpRtcp->RequestKeyFrame();
    }
}

bool VideoFrameConstructor::setBitrate(uint32_t kbps)
{
    // At present we do not react on this request
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
    const int8_t payload_type,
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const int frequency,
    const size_t channels,
    const uint32_t rate)
{
    // TODO: In M59 rtp_video_receiver will never invoke OnInitializeDecoder
    // from RtpFeedback. So moving the external decoder register to
    // somewhere else and remove the workaround in rtp_rtcp module.
    m_video_receiver->RegisterExternalDecoder(this, payload_type);
    return 0;
}

void VideoFrameConstructor::OnIncomingSSRCChanged(const uint32_t ssrc)
{
    m_rtpRtcp->SetRemoteSSRC(ssrc);
    m_ssrc = ssrc;
}

void VideoFrameConstructor::ResetStatistics(uint32_t ssrc)
{
    //Not supported on M59
}

int32_t VideoFrameConstructor::InitDecode(const webrtc::VideoCodec* codecSettings, int32_t numberOfCores)
{
    assert(codecSettings->codecType == webrtc::kVideoCodecVP8 || codecSettings->codecType == webrtc::kVideoCodecH264
           /*|| codecSettings->codecType == webrtc::kVideoCodecH265*/ || codecSettings->codecType == webrtc::kVideoCodecVP9);
    if (codecSettings->codecType == webrtc::kVideoCodecVP8)
        m_format = woogeen_base::FRAME_FORMAT_VP8;
    else if (codecSettings->codecType == webrtc::kVideoCodecVP9)
        m_format = woogeen_base::FRAME_FORMAT_VP9;
    else if (codecSettings->codecType == webrtc::kVideoCodecH264)
        m_format = woogeen_base::FRAME_FORMAT_H264;
/*
    else if (codecSettings->codecType == webrtc::kVideoCodecH265)
        m_format = woogeen_base::FRAME_FORMAT_H265;
*/

    return 0;
}

static void dump(void* index, FrameFormat format, uint8_t* buf, int len)
{
    char dumpFileName[128];

    snprintf(dumpFileName, 128, "/tmp/postConstructor-%p.%s", index, getFormatStr(format));
    FILE* bsDumpfp = fopen(dumpFileName, "ab");
    if (bsDumpfp) {
        fwrite(buf, 1, len, bsDumpfp);
        fclose(bsDumpfp);
    }
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
        case webrtc::kVideoCodecVP9:
            format = FRAME_FORMAT_VP9;
            break;
        case webrtc::kVideoCodecH264:
            format = FRAME_FORMAT_H264;
            break;
/*
        case webrtc::kVideoCodecH265:
            format = FRAME_FORMAT_H265;
            break;
*/
        default:
            ELOG_ERROR("Unknown format");
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
            ELOG_DEBUG("received video resolution has changed to %ux%u", m_width, m_height);
            //TODO: Notify the controlling layer about the resolution change.
        }

        if (encodedImage._frameType == webrtc::kVideoFrameKey) {
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
        frame.additionalInfo.video.isKeyFrame = (encodedImage._frameType == webrtc::kVideoFrameKey);

        if (m_enabled) {
            deliverFrame(frame);
        }

        //if (frame.format == FRAME_FORMAT_H264 || frame.format == FRAME_FORMAT_H265) {
        //    dump(this, frame.format, frame.payload, frame.length);
        //}
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
        // a dedicated thread to keep on invoking vcm::VideoReceiver::Decode, and, with a wait time other than 0.
        // (Default wait time used by webrtc vie engine is 50ms).
        uint16_t maxWaitInterval = 0;
        int32_t ret = m_video_receiver->Decode(maxWaitInterval);
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
