// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoFramePacketizer.h"
#include "MediaUtilities.h"
#include <rtputils.h>
#include <api/video_codecs/video_codec.h>
#include <api/video/video_codec_type.h>
#include <api/rtc_event_log/rtc_event_log.h>
#include <modules/rtp_rtcp/source/rtp_video_header.h>
#include <modules/include/module_common_types.h>
#include <rtc_base/logging.h>

namespace owt_base {

// To make it consistent with the webrtc library, we allow packets to be transmitted
// in up to 2 times max video bitrate if the bandwidth estimate allows it.
static const int TRANSMISSION_MAXBITRATE_MULTIPLIER = 2;

DEFINE_LOGGER(VideoFramePacketizer, "owt.VideoFramePacketizer");

VideoFramePacketizer::VideoFramePacketizer(
    bool enableRed,
    bool enableUlpfec,
    bool enableTransportcc,
    bool selfRequestKeyframe,
    uint32_t transportccExtId)
    : m_enabled(true)
    , m_enableDump(false)
    , m_keyFrameArrived(false)
    , m_selfRequestKeyframe(selfRequestKeyframe)
    , m_frameFormat(FRAME_FORMAT_UNKNOWN)
    , m_frameWidth(0)
    , m_frameHeight(0)
    , m_random(rtc::TimeMicros())
    , m_ssrc(0)
    , m_ssrcGenerator(SsrcGenerator::GetSsrcGenerator())
    , m_sendFrameCount(0)
    , m_clock(nullptr)
    , m_timeStampOffset(0)
{
    video_sink_ = nullptr;
    m_ssrc = m_ssrcGenerator->CreateSsrc();
    m_ssrcGenerator->RegisterSsrc(m_ssrc);
    m_videoTransport.reset(new WebRTCTransport<erizoExtra::VIDEO>(this, nullptr));
    m_taskRunner.reset(new owt_base::WebRTCTaskRunner("VideoFramePacketizer"));
    m_taskRunner->Start();
    init(enableRed, enableUlpfec, enableTransportcc, transportccExtId);
}

VideoFramePacketizer::~VideoFramePacketizer()
{
    close();
    m_taskRunner->Stop();
    m_ssrcGenerator->ReturnSsrc(m_ssrc);
    boost::unique_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
}

void VideoFramePacketizer::bindTransport(erizo::MediaSink* sink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_transportMutex);
    video_sink_ = sink;
    video_sink_->setVideoSinkSSRC(m_rtpRtcp->SSRC());
    erizo::FeedbackSource* fbSource = video_sink_->getFeedbackSource();
    if (fbSource)
        fbSource->setFeedbackSink(this);
}

void VideoFramePacketizer::unbindTransport()
{
    boost::unique_lock<boost::shared_mutex> lock(m_transportMutex);
    if (video_sink_) {
        video_sink_ = nullptr;
    }
}

void VideoFramePacketizer::enable(bool enabled)
{
    m_enabled = enabled;
    if (m_enabled) {
        FeedbackMsg feedback = {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
        deliverFeedbackMsg(feedback);
        m_keyFrameArrived = false;
        m_sendFrameCount = 0;
        m_timeStampOffset = 0;
    }
}

void VideoFramePacketizer::OnReceivedIntraFrameRequest(uint32_t ssrc)
{
    // ELOG_WARN("onReceivedIntraFrameRequest.");
    FeedbackMsg feedback = {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
    deliverFeedbackMsg(feedback);
}

int VideoFramePacketizer::deliverFeedback_(std::shared_ptr<erizo::DataPacket> data_packet)
{
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    if (m_rtpRtcp) {
        m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(data_packet->data), data_packet->length);
        return data_packet->length;
    }
    return 0;
}

void VideoFramePacketizer::receiveRtpData(char* buf, int len, erizoExtra::DataType type, uint32_t channelId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_transportMutex);
    if (!video_sink_) {
        return;
    }

    assert(type == erizoExtra::VIDEO);
    // ELOG_WARN("receiveRtpData %p", buf);
    video_sink_->deliverVideoData(std::make_shared<erizo::DataPacket>(0, buf, len, erizo::VIDEO_PACKET));
}

static int getNextNaluPosition(uint8_t *buffer, int buffer_size, bool &is_aud_or_sei) {
    if (buffer_size < 4) {
        return -1;
    }
    is_aud_or_sei = false;
    uint8_t *head = buffer;
    uint8_t *end = buffer + buffer_size - 4;
    while (head < end) {
        if (head[0]) {
            head++;
            continue;
        }
        if (head[1]) {
            head +=2;
            continue;
        }
        if (head[2]) {
            head +=3;
            continue;
        }
        if (head[3] != 0x01) {
            head++;
            continue;
        }
        if (((head[4] & 0x1F) == 9) || ((head[4] & 0x1F) == 6)) {
            is_aud_or_sei = true;
        }

        return static_cast<int>(head - buffer);
    }
    return -1;
}

#define MAX_NALS_PER_FRAME 128
static int dropAUDandSEI(uint8_t* framePayload, int frameLength) {
    uint8_t *origin_pkt_data = framePayload;
    int origin_pkt_length = frameLength;
    uint8_t *head = origin_pkt_data;

    std::vector<int> nal_offset;
    std::vector<bool> nal_type_is_aud_or_sei;
    std::vector<int> nal_size;
    bool is_aud_or_sei = false, has_aud_or_sei = false;

    int sc_positions_length = 0;
    int sc_position = 0;
    while (sc_positions_length < MAX_NALS_PER_FRAME) {
         int nalu_position = getNextNaluPosition(origin_pkt_data + sc_position,
              origin_pkt_length - sc_position, is_aud_or_sei);
         if (nalu_position < 0) {
             break;
         }
         sc_position += nalu_position;
         nal_offset.push_back(sc_position); //include start code.
         sc_position += 4;
         sc_positions_length++;
         if (is_aud_or_sei) {
             has_aud_or_sei = true;
             nal_type_is_aud_or_sei.push_back(true);
         } else {
             nal_type_is_aud_or_sei.push_back(false);
         }
    }
    if (sc_positions_length == 0 || !has_aud_or_sei)
        return frameLength;
    // Calculate size of each NALs
    for (unsigned int count = 0; count < nal_offset.size(); count++) {
       if (count + 1 == nal_offset.size()) {
           nal_size.push_back(origin_pkt_length - nal_offset[count]);
       } else {
           nal_size.push_back(nal_offset[count+1] - nal_offset[count]);
       }
    }
    // remove in place the AUD NALs
    int new_size = 0;
    for (unsigned int i = 0; i < nal_offset.size(); i++) {
       if (!nal_type_is_aud_or_sei[i]) {
           memmove(head + new_size, head + nal_offset[i], nal_size[i]);
           new_size += nal_size[i];
       }
    }
    return new_size;
}

static void dump(void* index, FrameFormat format, uint8_t* buf, int len)
{
    char dumpFileName[128];

    snprintf(dumpFileName, 128, "/tmp/prePacketizer-%p.%s", index, getFormatStr(format));
    FILE* bsDumpfp = fopen(dumpFileName, "ab");
    if (bsDumpfp) {
        fwrite(buf, 1, len, bsDumpfp);
        fclose(bsDumpfp);
    }
}

void VideoFramePacketizer::onFrame(const Frame& frame)
{
    // ELOG_DEBUG("onFrame, format:%d, length:%d", frame.format, frame.length);
    using namespace webrtc;
    if (!m_enabled) {
        return;
    }

    if (!m_keyFrameArrived) {
        if (!frame.additionalInfo.video.isKeyFrame) {
            // ELOG_DEBUG("Key frame has not arrived, send key-frame-request.");
            FeedbackMsg feedback = {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
            deliverFeedbackMsg(feedback);
            return;
        } else {
            // Recalculate timestamp offset
            const uint32_t kMsToRtpTimestamp = 90;
            m_timeStampOffset = kMsToRtpTimestamp * m_clock->TimeInMilliseconds() - frame.timeStamp;
            m_keyFrameArrived = true;
        }
    }

    if (m_selfRequestKeyframe) {
        //FIXME: This is a workround for peer client not send key-frame-request
        if (m_sendFrameCount < 151) {
            if ((m_sendFrameCount == 10)
                || (m_sendFrameCount == 30)
                || (m_sendFrameCount == 60)
                || (m_sendFrameCount == 150)) {
                // ELOG_DEBUG("Self generated key-frame-request.");
                FeedbackMsg feedback = {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
                deliverFeedbackMsg(feedback);
            }
            m_sendFrameCount += 1;
        }
    }

    // Recalculate timestamp for stream substitution
    uint32_t timeStamp = frame.timeStamp + m_timeStampOffset;//kMsToRtpTimestamp * m_clock->TimeInMilliseconds();
    webrtc::RTPVideoHeader h;
    memset(&h, 0, sizeof(webrtc::RTPVideoHeader));

    if (frame.format != m_frameFormat
        || frame.additionalInfo.video.width != m_frameWidth
        || frame.additionalInfo.video.height != m_frameHeight) {
        m_frameFormat = frame.format;
        m_frameWidth = frame.additionalInfo.video.width;
        m_frameHeight = frame.additionalInfo.video.height;
    }

    h.frame_type = frame.additionalInfo.video.isKeyFrame ?
        VideoFrameType::kVideoFrameKey : VideoFrameType::kVideoFrameDelta;
    // h.rotation = image.rotation_;
    // h.content_type = image.content_type_;
    // h.playout_delay = image.playout_delay_;
    h.width = m_frameWidth;
    h.height = m_frameHeight;

    if (frame.format == FRAME_FORMAT_VP8) {
        h.codec = webrtc::VideoCodecType::kVideoCodecVP8;
        auto& vp8_header = h.video_type_header.emplace<RTPVideoHeaderVP8>();
        vp8_header.InitRTPVideoHeaderVP8();
        boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
        m_senderVideo->SendVideo(
            VP8_90000_PT,
            webrtc::kVideoCodecVP8,
            timeStamp,
            timeStamp,
            rtc::ArrayView<const uint8_t>(frame.payload, frame.length),
            nullptr,
            h,
            m_rtpRtcp->ExpectedRetransmissionTimeMs());
    } else if (frame.format == FRAME_FORMAT_VP9) {
        h.codec = webrtc::VideoCodecType::kVideoCodecVP9;
        auto& vp9_header = h.video_type_header.emplace<RTPVideoHeaderVP9>();
        vp9_header.InitRTPVideoHeaderVP9();
        vp9_header.inter_pic_predicted = !frame.additionalInfo.video.isKeyFrame;
        boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
        m_senderVideo->SendVideo(
            VP9_90000_PT,
            webrtc::kVideoCodecVP9,
            timeStamp,
            timeStamp,
            rtc::ArrayView<const uint8_t>(frame.payload, frame.length),
            nullptr,
            h,
            m_rtpRtcp->ExpectedRetransmissionTimeMs());
    } else if (frame.format == FRAME_FORMAT_H264 || frame.format == FRAME_FORMAT_H265) {
        int frame_length = frame.length;
        if (m_enableDump) {
            dump(this, frame.format, frame.payload, frame_length);
        }

        //FIXME: temporarily filter out AUD because chrome M59 could NOT handle it correctly.
        //FIXME: temporarily filter out SEI because safari could NOT handle it correctly.
        if (frame.format == FRAME_FORMAT_H264) {
            frame_length = dropAUDandSEI(frame.payload, frame_length);
        }

        int nalu_found_length = 0;
        uint8_t* buffer_start = frame.payload;
        int buffer_length = frame_length;
        int nalu_start_offset = 0;
        int nalu_end_offset = 0;
        int sc_len = 0;
        RTPFragmentationHeader frag_info;

        h.codec = (frame.format == FRAME_FORMAT_H264)?
            webrtc::VideoCodecType::kVideoCodecH264 : webrtc::VideoCodecType::kVideoCodecH265;
        while (buffer_length > 0) {
            nalu_found_length = findNALU(buffer_start, buffer_length, &nalu_start_offset, &nalu_end_offset, &sc_len);
            if (nalu_found_length < 0) {
                /* Error, should never happen */
                break;
            } else {
                /* SPS, PPS, I, P*/
                uint16_t last = frag_info.fragmentationVectorSize;
                frag_info.VerifyAndAllocateFragmentationHeader(last + 1);
                frag_info.fragmentationOffset[last] = nalu_start_offset + (buffer_start - frame.payload);
                frag_info.fragmentationLength[last] = nalu_found_length;
                buffer_start += (nalu_start_offset + nalu_found_length);
                buffer_length -= (nalu_start_offset + nalu_found_length);
            }
        }

        boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
        if (frame.format == FRAME_FORMAT_H264) {
            h.video_type_header.emplace<RTPVideoHeaderH264>();
            m_senderVideo->SendVideo(
                H264_90000_PT,
                webrtc::kVideoCodecH264,
                timeStamp,
                timeStamp,
                rtc::ArrayView<const uint8_t>(frame.payload, frame.length),
                &frag_info,
                h,
                m_rtpRtcp->ExpectedRetransmissionTimeMs());
        } else {
            h.video_type_header.emplace<RTPVideoHeaderH265>();
            m_senderVideo->SendVideo(
                H265_90000_PT,
                webrtc::kVideoCodecH265,
                timeStamp,
                timeStamp,
                rtc::ArrayView<const uint8_t>(frame.payload, frame.length),
                &frag_info,
                h,
                m_rtpRtcp->ExpectedRetransmissionTimeMs());
        }
    }
}

void VideoFramePacketizer::onVideoSourceChanged()
{
    m_keyFrameArrived = false;
}

int VideoFramePacketizer::sendFirPacket()
{
    FeedbackMsg feedback = {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
    deliverFeedbackMsg(feedback);
    return 0;
}

bool VideoFramePacketizer::init(bool enableRed, bool enableUlpfec, bool enableTransportcc, uint32_t transportccExtId)
{
    using namespace webrtc;
    m_clock = Clock::GetRealTimeClock();
    m_retransmissionRateLimiter.reset(new webrtc::RateLimiter(Clock::GetRealTimeClock(), 1000));

    m_eventLog = std::make_unique<webrtc::RtcEventLogNull>();
    RtpRtcp::Configuration configuration;
    configuration.clock = m_clock;
    configuration.audio = false;
    configuration.receiver_only = false;
    configuration.outgoing_transport = m_videoTransport.get();
    configuration.intra_frame_callback = this;
    configuration.event_log = m_eventLog.get();
    configuration.retransmission_rate_limiter = m_retransmissionRateLimiter.get();
    configuration.local_media_ssrc = m_ssrc;//rtp_config.ssrcs[i];


    m_rtpRtcp = RtpRtcp::Create(configuration);
    m_rtpRtcp->SetSendingStatus(true);
    m_rtpRtcp->SetSendingMediaStatus(true);
    m_rtpRtcp->SetRTCPStatus(RtcpMode::kReducedSize);
    // Set NACK.
    m_rtpRtcp->SetStorePacketsStatus(true, 600);
    if (transportccExtId > 0) {
        m_rtpRtcp->RegisterRtpHeaderExtension(
            webrtc::RtpExtension::kTransportSequenceNumberUri, transportccExtId);
    }

    webrtc::RTPSenderVideo::Config video_config;
    m_playoutDelayOracle = std::make_unique<PlayoutDelayOracle>();
    m_fieldTrialConfig = std::make_unique<FieldTrialBasedConfig>();
    video_config.clock = configuration.clock;
    video_config.rtp_sender = m_rtpRtcp->RtpSender();
    video_config.field_trials = m_fieldTrialConfig.get();
    video_config.playout_delay_oracle = m_playoutDelayOracle.get();
    if (enableRed) {
        video_config.red_payload_type = RED_90000_PT;
    }
    if (enableUlpfec) {
        video_config.ulpfec_payload_type = ULP_90000_PT;
    }
    m_senderVideo = std::make_unique<RTPSenderVideo>(video_config);
    // m_params = std::make_unique<RtpPayloadParams>(m_ssrc, nullptr);
    m_taskRunner->RegisterModule(m_rtpRtcp.get());

    return true;
}

void VideoFramePacketizer::close()
{
    unbindTransport();
    m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
}

int VideoFramePacketizer::sendPLI() {
    return 0;
}

}
