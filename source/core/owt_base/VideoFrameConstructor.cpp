// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoFrameConstructor.h"

#include "WebRTCTaskRunner.h"
#include <rtputils.h>
#include <webrtc/common_types.h>
#include <webrtc/modules/video_coding/timing.h>
#include <webrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_single_stream.h>

using namespace webrtc;

namespace owt_base {

DEFINE_LOGGER(VideoFrameConstructor, "owt.VideoFrameConstructor");

VideoFrameConstructor::VideoFrameConstructor(VideoInfoListener* vil)
    : m_enabled(true)
    , m_enableDump(false)
    , m_format(FRAME_FORMAT_UNKNOWN)
    , m_width(0)
    , m_height(0)
    , m_ssrc(0)
    , m_video_receiver(nullptr)
    , m_transport(nullptr)
    , m_pendingKeyFrameRequests(0)
    , m_videoInfoListener(vil)
{
    m_videoTransport.reset(new WebRTCTransport<erizoExtra::VIDEO>(nullptr, nullptr));
    sink_fb_source_ = m_videoTransport.get();
    m_taskRunner.reset(new owt_base::WebRTCTaskRunner("VideoFrameConstructor"));
    m_taskRunner->Start();
    m_feedbackTimer.reset(new JobTimer(1, this));
    init();
}

VideoFrameConstructor::~VideoFrameConstructor()
{
    m_feedbackTimer->stop();
    m_remoteBitrateObserver->RemoveRembSender(m_rtpRtcp.get());
    m_videoReceiver->StopReceive();

    unbindTransport();

    if (m_taskRunner) {
        m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
        m_taskRunner->DeRegisterModule(m_video_receiver.get());;
        m_taskRunner->DeRegisterModule(m_remoteBitrateEstimator.get());
    }
    m_taskRunner->Stop();
    boost::unique_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
}

bool VideoFrameConstructor::init()
{
    // TODO: move to new jitter buffer implemetation(not ready yet).
    m_video_receiver.reset(new webrtc::vcm::VideoReceiver(Clock::GetRealTimeClock(), nullptr, nullptr, new VCMTiming(Clock::GetRealTimeClock(), nullptr), nullptr, nullptr));

    m_packetRouter.reset(new PacketRouter());
    m_remoteBitrateObserver.reset(new VieRemb(Clock::GetRealTimeClock()));
    m_remoteBitrateEstimator.reset(new RemoteEstimatorProxy(Clock::GetRealTimeClock(), m_packetRouter.get()));
    m_videoReceiver.reset(new ViEReceiver(m_video_receiver.get(), m_remoteBitrateEstimator.get(), this));
    m_videoReceiver->SetReceiveTransportSequenceNumberStatus(true, 5);

    RtpRtcp::Configuration configuration;
    configuration.audio = false;  // Video.
    configuration.outgoing_transport = m_videoTransport.get(); // For sending RTCP feedback to the publisher
    configuration.remote_bitrate_estimator = m_remoteBitrateEstimator.get();
    configuration.receive_statistics = m_videoReceiver->GetReceiveStatistics();
    m_rtpRtcp.reset(RtpRtcp::CreateRtpRtcp(configuration));
    m_rtpRtcp->SetRTCPStatus(webrtc::RtcpMode::kCompound);
    // Since currently our MCU only claims FIR support in SDP, we choose FirRtcp for now.
    m_rtpRtcp->SetKeyFrameRequestMethod(kKeyFrameReqFirRtcp);
    m_rtpRtcp->RegisterSendRtpHeaderExtension(RTPExtensionType::kRtpExtensionTransportSequenceNumber, 5);
    m_rtpRtcp->SetREMBStatus(false);
    m_videoReceiver->SetRtpRtcpModule(m_rtpRtcp.get());
    m_remoteBitrateObserver->AddRembSender(m_rtpRtcp.get());
    m_packetRouter->AddReceiveRtpModule(m_rtpRtcp.get());

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

    VideoCodingModule::Codec(webrtc::kVideoCodecH265, &video_codec);
    video_codec.plType = H265_90000_PT;
    if (m_video_receiver->RegisterReceiveCodec(&video_codec, 1, true) != VCM_OK)
        assert(!"register H265 decoder failed");
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

    m_taskRunner->RegisterModule(m_remoteBitrateEstimator.get());
    m_taskRunner->RegisterModule(m_video_receiver.get());
    m_taskRunner->RegisterModule(m_rtpRtcp.get());
    m_videoReceiver->StartReceive();
    return true;
}

void VideoFrameConstructor::bindTransport(erizo::MediaSource* source, erizo::FeedbackSink* fbSink)
{
    // ELOG_INFO("bindTransport source %p fbsink %p this %p", source, fbSink, this);
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    m_transport = source;
    m_transport->setVideoSink(this);
    m_transport->setEventSink(this);
    m_videoTransport->setFeedbackSink(fbSink);
}

void VideoFrameConstructor::unbindTransport()
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (m_transport) {
        m_videoTransport->setFeedbackSink(nullptr);
        m_transport = nullptr;
    }
}

void VideoFrameConstructor::enable(bool enabled)
{
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    m_enabled = enabled;
    if (m_enabled && m_rtpRtcp) {
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
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    if (m_rtpRtcp) {
        return m_rtpRtcp->SendNACK(sequenceNumbers, length);
    }
    return 0;
}

int32_t VideoFrameConstructor::RequestKeyFrame()
{
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    if (m_rtpRtcp) {
        // ELOG_INFO("RequestKeyFrame");
        return m_rtpRtcp->RequestKeyFrame();
    }
    return 0;
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
    // ELOG_INFO("OnInitializeDecoder");
    m_video_receiver->RegisterExternalDecoder(this, payload_type);
    return 0;
}

void VideoFrameConstructor::OnIncomingSSRCChanged(const uint32_t ssrc)
{
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    if (m_rtpRtcp) {
        m_rtpRtcp->SetRemoteSSRC(ssrc);
        m_ssrc = ssrc;
    }
}

void VideoFrameConstructor::ResetStatistics(uint32_t ssrc)
{
    //Not supported on M59
}

int32_t VideoFrameConstructor::InitDecode(const webrtc::VideoCodec* codecSettings, int32_t numberOfCores)
{
    assert(codecSettings->codecType == webrtc::kVideoCodecVP8 || codecSettings->codecType == webrtc::kVideoCodecH264
           || codecSettings->codecType == webrtc::kVideoCodecH265 || codecSettings->codecType == webrtc::kVideoCodecVP9);
    if (codecSettings->codecType == webrtc::kVideoCodecVP8)
        m_format = owt_base::FRAME_FORMAT_VP8;
    else if (codecSettings->codecType == webrtc::kVideoCodecVP9)
        m_format = owt_base::FRAME_FORMAT_VP9;
    else if (codecSettings->codecType == webrtc::kVideoCodecH264)
        m_format = owt_base::FRAME_FORMAT_H264;
    else if (codecSettings->codecType == webrtc::kVideoCodecH265)
        m_format = owt_base::FRAME_FORMAT_H265;

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

    // ELOG_INFO("encodedImage._length %d", encodedImage._length);
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

        case webrtc::kVideoCodecH265:
            format = FRAME_FORMAT_H265;
            break;

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
            if (m_videoInfoListener) {
                std::ostringstream json_str;
                json_str.str("");
                json_str << "{\"video\": {\"parameters\": {\"resolution\": {"
                         << "\"width\":" << m_width << ", "
                         << "\"height\":" << m_height
                         << "}}}}";
                m_videoInfoListener->onVideoInfo(json_str.str().c_str());
            }
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
            // ELOG_INFO("DELIVER FRAME !!!");
            deliverFrame(frame);
        }

        if (m_enableDump && (frame.format == FRAME_FORMAT_H264 || frame.format == FRAME_FORMAT_H265)) {
            dump(this, frame.format, frame.payload, frame.length);
        }
    }
    return 0;
}

int VideoFrameConstructor::deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet)
{
    // Copy data
    memcpy(buf, video_packet->data, video_packet->length);

    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
    uint8_t packetType = chead->getPacketType();

    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT)
        return m_videoReceiver->ReceivedRTCPPacket(buf, video_packet->length) == -1 ? 0 : video_packet->length;

    PacketTime current;
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    if (m_rtpRtcp && m_videoReceiver->ReceivedRTPPacket(buf, video_packet->length, current) != -1) {
        // FIXME: Decode should be invoked as often as possible.
        // To invoke it in current work thread is not a good idea. We may need to create
        // a dedicated thread to keep on invoking vcm::VideoReceiver::Decode, and, with a wait time other than 0.
        // (Default wait time used by webrtc vie engine is 50ms).
        uint16_t maxWaitInterval = 0;
        int32_t ret = m_video_receiver->Decode(maxWaitInterval);
        ELOG_TRACE("receivedRtpData decode result = %d",  ret);
        return video_packet->length;
    }

    return 0;
}

int VideoFrameConstructor::deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet)
{
    assert(false);
    return 0;
}

void VideoFrameConstructor::onTimeout()
{
    if (m_pendingKeyFrameRequests > 1) {
        this->RequestKeyFrame();
    }
    m_pendingKeyFrameRequests = 0;
}

void VideoFrameConstructor::onFeedback(const FeedbackMsg& msg)
{
    if (msg.type == owt_base::VIDEO_FEEDBACK) {
        if (msg.cmd == REQUEST_KEY_FRAME) {
            if (!m_pendingKeyFrameRequests) {
                this->RequestKeyFrame();
            }
            ++m_pendingKeyFrameRequests;
        } else if (msg.cmd == SET_BITRATE) {
            this->setBitrate(msg.data.kbps);
        }
    }
}

int VideoFrameConstructor::deliverEvent_(erizo::MediaEventPtr event){
    return 0;
}

void VideoFrameConstructor::close(){
    unbindTransport();
}

}
