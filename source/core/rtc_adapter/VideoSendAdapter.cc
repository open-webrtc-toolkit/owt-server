// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoSendAdapter.h"
#include "MediaUtilities.h"
#include "TaskRunnerPool.h"

#include <api/rtc_event_log/rtc_event_log.h>
#include <api/video/video_codec_type.h>
#include <api/video_codecs/video_codec.h>
#include <modules/include/module_common_types.h>
#include <modules/rtp_rtcp/source/rtp_video_header.h>
#include <rtc_base/logging.h>
#include <rtputils.h>

using namespace owt_base;

namespace rtc_adapter {

// To make it consistent with the webrtc library, we allow packets to be transmitted
// in up to 2 times max video bitrate if the bandwidth estimate allows it.
static const int TRANSMISSION_MAXBITRATE_MULTIPLIER = 2;
static const int kMaxRtpPacketSize = 1200;

static int getNextNaluPosition(uint8_t* buffer, int buffer_size, bool& is_aud_or_sei, int& sc_len)
{
    if (buffer_size < 3) {
        return -1;
    }
    is_aud_or_sei = false;
    uint8_t* head = buffer;
    uint8_t* end = buffer + buffer_size - 3;
    while (head < end) {
        if (head[0]) {
            head++;
            continue;
        }
        if (head[1]) {
            head += 2;
            continue;
        }
        if (head[2]) {
            if (head[2] == 0x01) {
                if (((head[3] & 0x1F) == 9) || ((head[3] & 0x1F) == 6)) {
                    is_aud_or_sei = true;
                }
                sc_len = 3;
                return static_cast<int>(head - buffer);
            }
            head += 3;
            continue;
        }
        if (head[3] != 0x01) {
            head++;
            continue;
        }
        if (head + 1 == end) {
            break;
        }
        if (((head[4] & 0x1F) == 9) || ((head[4] & 0x1F) == 6)) {
            is_aud_or_sei = true;
        }
        sc_len = 4;
        return static_cast<int>(head - buffer);
    }
    return -1;
}

#define MAX_NALS_PER_FRAME 128
static int dropAUDandSEI(uint8_t* framePayload, int frameLength)
{
    uint8_t* origin_pkt_data = framePayload;
    int origin_pkt_length = frameLength;
    uint8_t* head = origin_pkt_data;

    std::vector<int> nal_offset;
    std::vector<bool> nal_type_is_aud_or_sei;
    std::vector<int> nal_size;
    bool is_aud_or_sei = false, has_aud_or_sei = false;

    int sc_positions_length = 0;
    int sc_position = 0;
    int sc_len = 4;
    while (sc_positions_length < MAX_NALS_PER_FRAME) {
        int nalu_position = getNextNaluPosition(origin_pkt_data + sc_position,
            origin_pkt_length - sc_position, is_aud_or_sei, sc_len);
        if (nalu_position < 0) {
            break;
        }
        sc_position += nalu_position;
        nal_offset.push_back(sc_position); //include start code.
        sc_position += sc_len;
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
            nal_size.push_back(nal_offset[count + 1] - nal_offset[count]);
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

VideoSendAdapterImpl::VideoSendAdapterImpl(
    CallOwner* owner,
    const RtcAdapter::Config& config)
    : m_enableDump(false)
    , m_config(config)
    , m_keyFrameArrived(false)
    , m_frameFormat(FRAME_FORMAT_UNKNOWN)
    , m_frameWidth(0)
    , m_frameHeight(0)
    , m_random(rtc::TimeMicros())
    , m_ssrc(0)
    , m_ssrcGenerator(SsrcGenerator::GetSsrcGenerator())
    , m_clock(nullptr)
    , m_timeStampOffset(0)
    , m_feedbackListener(config.feedback_listener)
    , m_rtpListener(config.rtp_listener)
    , m_statsListener(config.stats_listener)
{
    m_ssrc = m_ssrcGenerator->CreateSsrc();
    m_ssrcGenerator->RegisterSsrc(m_ssrc);
    m_taskRunner = TaskRunnerPool::GetInstance().GetTaskRunner();
    init();
}

VideoSendAdapterImpl::~VideoSendAdapterImpl()
{
    m_taskRunner->DeRegisterModule(m_rtpRtcp.get());
    m_ssrcGenerator->ReturnSsrc(m_ssrc);
    boost::unique_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
}

bool VideoSendAdapterImpl::init()
{
    m_clock = webrtc::Clock::GetRealTimeClock();
    m_retransmissionRateLimiter.reset(
        new webrtc::RateLimiter(webrtc::Clock::GetRealTimeClock(), 1000));

    m_eventLog = std::make_unique<webrtc::RtcEventLogNull>();
    webrtc::RtpRtcp::Configuration configuration;
    configuration.clock = m_clock;
    configuration.audio = false;
    configuration.receiver_only = false;
    configuration.outgoing_transport = this;
    configuration.intra_frame_callback = this;
    configuration.event_log = m_eventLog.get();
    configuration.retransmission_rate_limiter = m_retransmissionRateLimiter.get();
    configuration.local_media_ssrc = m_ssrc; //rtp_config.ssrcs[i];
    // TODO: enable UlpfecGenerator
    /*
    std::make_unique<UlpfecGenerator>(
        rtp.ulpfec.red_payload_type, rtp.ulpfec.ulpfec_payload_type, clock);
    configuration.fec_generator = fec_generator.get();
    */

    m_rtpRtcp = webrtc::RtpRtcp::Create(configuration);
    m_rtpRtcp->SetSendingStatus(true);
    m_rtpRtcp->SetSendingMediaStatus(true);
    m_rtpRtcp->SetRTCPStatus(webrtc::RtcpMode::kReducedSize);
    // Set NACK.
    m_rtpRtcp->SetStorePacketsStatus(true, 600);
    if (m_config.transport_cc) {
        m_rtpRtcp->RegisterRtpHeaderExtension(
            webrtc::RtpExtension::kTransportSequenceNumberUri, m_config.transport_cc);
    }
    if (m_config.mid_ext) {
        m_config.mid[sizeof(m_config.mid) - 1] = '\0';
        std::string mid(m_config.mid);
        // Register MID extension
        m_rtpRtcp->RegisterRtpHeaderExtension(
            webrtc::RtpExtension::kMidUri, m_config.mid_ext);
        m_rtpRtcp->SetMid(mid);
    }

    m_rtpRtcp->SetMaxRtpPacketSize(kMaxRtpPacketSize);

    webrtc::RTPSenderVideo::Config video_config;
    m_fieldTrialConfig = std::make_unique<webrtc::FieldTrialBasedConfig>();
    video_config.clock = configuration.clock;
    video_config.rtp_sender = m_rtpRtcp->RtpSender();
    video_config.field_trials = m_fieldTrialConfig.get();
    if (m_config.red_payload) {
        video_config.red_payload_type = m_config.red_payload;
    }

    m_senderVideo = std::make_unique<webrtc::RTPSenderVideo>(video_config);
    m_taskRunner->RegisterModule(m_rtpRtcp.get());

    return true;
}

void VideoSendAdapterImpl::reset()
{
    m_keyFrameArrived = false;
    m_timeStampOffset = 0;
}

void VideoSendAdapterImpl::onFrame(const Frame& frame)
{
    using namespace webrtc;

    if (!m_keyFrameArrived) {
        if (!frame.additionalInfo.video.isKeyFrame) {
            RTC_DLOG(LS_INFO) << "Key frame has not arrived, send key-frame-request.";
            if (m_feedbackListener) {
                FeedbackMsg feedback = {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME };
                m_feedbackListener->onFeedback(feedback);
            }
            return;
        } else {
            // Recalculate timestamp offset
            const uint32_t kMsToRtpTimestamp = 90;
            m_timeStampOffset = kMsToRtpTimestamp * m_clock->TimeInMilliseconds() - frame.timeStamp;
            m_keyFrameArrived = true;
        }
    }

    // Recalculate timestamp for stream substitution
    uint32_t timeStamp = frame.timeStamp + m_timeStampOffset; //kMsToRtpTimestamp * m_clock->TimeInMilliseconds();
    webrtc::RTPVideoHeader h;
    memset(&h, 0, sizeof(webrtc::RTPVideoHeader));

    if (frame.format != m_frameFormat
        || frame.additionalInfo.video.width != m_frameWidth
        || frame.additionalInfo.video.height != m_frameHeight) {
        m_frameFormat = frame.format;
        m_frameWidth = frame.additionalInfo.video.width;
        m_frameHeight = frame.additionalInfo.video.height;
    }

    h.frame_type = frame.additionalInfo.video.isKeyFrame ? VideoFrameType::kVideoFrameKey : VideoFrameType::kVideoFrameDelta;
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
            h,
            m_rtpRtcp->ExpectedRetransmissionTimeMs(),
            0);
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
            h,
            m_rtpRtcp->ExpectedRetransmissionTimeMs(),
            0);

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

        h.codec = (frame.format == FRAME_FORMAT_H264) ?
            webrtc::VideoCodecType::kVideoCodecH264 :
            webrtc::VideoCodecType::kVideoCodecH265;

        boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
        if (frame.format == FRAME_FORMAT_H264) {
            h.video_type_header.emplace<RTPVideoHeaderH264>();
            m_senderVideo->SendVideo(
                H264_90000_PT,
                webrtc::kVideoCodecH264,
                timeStamp,
                timeStamp,
                rtc::ArrayView<const uint8_t>(frame.payload, frame.length),
                h,
                m_rtpRtcp->ExpectedRetransmissionTimeMs(),
                0);
        } else {
            h.video_type_header.emplace<RTPVideoHeaderH265>();
            m_senderVideo->SendVideo(
                H265_90000_PT,
                webrtc::kVideoCodecH265,
                timeStamp,
                timeStamp,
                rtc::ArrayView<const uint8_t>(frame.payload, frame.length),
                h,
                m_rtpRtcp->ExpectedRetransmissionTimeMs(),
                0);
        }
    }
}

int VideoSendAdapterImpl::onRtcpData(char* data, int len)
{
    boost::shared_lock<boost::shared_mutex> lock(m_rtpRtcpMutex);
    if (m_rtpRtcp) {
        m_rtpRtcp->IncomingRtcpPacket(reinterpret_cast<uint8_t*>(data), len);
        return len;
    }
    return 0;
}

bool VideoSendAdapterImpl::SendRtp(const uint8_t* data,
    size_t length,
    const webrtc::PacketOptions& options)
{
    if (m_rtpListener) {
        m_rtpListener->onAdapterData(
            reinterpret_cast<char*>(const_cast<uint8_t*>(data)), length);
        return true;
    }
    return false;
}

bool VideoSendAdapterImpl::SendRtcp(const uint8_t* data, size_t length)
{
    const RTCPHeader* chead = reinterpret_cast<const RTCPHeader*>(data);
    uint8_t packetType = chead->getPacketType();
    if (packetType == RTCP_Sender_PT) {
        if (m_rtpListener) {
            m_rtpListener->onAdapterData(
                reinterpret_cast<char*>(const_cast<uint8_t*>(data)), length);
            return true;
        }
    }
    return false;
}

void VideoSendAdapterImpl::OnReceivedIntraFrameRequest(uint32_t ssrc)
{
    RTC_DLOG(LS_INFO) << "onReceivedIntraFrameRequest.";
    if (m_feedbackListener) {
        FeedbackMsg feedback = {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME };
        m_feedbackListener->onFeedback(feedback);
    }
}

} // namespace rtc_adapter
