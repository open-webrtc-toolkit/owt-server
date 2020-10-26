// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoReceiveAdapter.h"

#include <common_types.h>
#include <future>
#include <modules/video_coding/include/video_error_codes.h>
#include <modules/video_coding/timing.h>
#include <rtc_base/time_utils.h>
#include <rtputils.h>

// using namespace webrtc;
using namespace owt_base;

namespace rtc_adapter {

const uint32_t kBufferSize = 8192;
// Local SSRC has no meaning for receive stream here
const uint32_t kLocalSsrc = 1;

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

int32_t VideoReceiveAdapterImpl::AdapterDecoder::InitDecode(const webrtc::VideoCodec* config,
    int32_t number_of_cores)
{
    RTC_DLOG(LS_INFO) << "AdapterDecoder InitDecode";
    if (config) {
        m_codec = config->codecType;
    }
    if (!m_frameBuffer) {
        m_bufferSize = kBufferSize;
        m_frameBuffer.reset(new uint8_t[m_bufferSize]);
    }
    return 0;
}

struct DataPacket {
    DataPacket() = default;
    DataPacket(const char* data_, int length_)
        : length{ length_ }
    {
        memcpy(data, data_, length_);
    }

    char data[1500];
    int length;
};

int32_t VideoReceiveAdapterImpl::AdapterDecoder::Decode(const webrtc::EncodedImage& encodedImage,
    bool missing_frames,
    int64_t render_time_ms)
{
    RTC_DLOG(LS_VERBOSE) << "AdapterDecoder Decode";
    owt_base::FrameFormat format = FRAME_FORMAT_UNKNOWN;

    switch (m_codec) {
    case webrtc::VideoCodecType::kVideoCodecVP8:
        format = FRAME_FORMAT_VP8;
        break;
    case webrtc::VideoCodecType::kVideoCodecVP9:
        format = FRAME_FORMAT_VP9;
        break;
    case webrtc::VideoCodecType::kVideoCodecH264:
        format = FRAME_FORMAT_H264;
        break;
    case webrtc::VideoCodecType::kVideoCodecH265:
        format = FRAME_FORMAT_H265;
        break;
    default:
        RTC_LOG(LS_WARNING) << "Unknown FORMAT";
        return 0;
    }

    if (encodedImage.size() > m_bufferSize) {
        m_bufferSize = encodedImage.size() * 2;
        m_frameBuffer.reset(new uint8_t[m_bufferSize]);
        RTC_DLOG(LS_INFO) << "AdapterDecoder increase buffer size: " << m_bufferSize;
    }

    if (encodedImage._encodedWidth > 0 && encodedImage._encodedHeight > 0) {
        m_width = encodedImage._encodedWidth;
        m_height = encodedImage._encodedHeight;
    }

    memcpy(m_frameBuffer.get(), encodedImage.data(), encodedImage.size());
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = format;
    frame.payload = m_frameBuffer.get();
    frame.length = encodedImage.size();
    frame.timeStamp = encodedImage.Timestamp();
    frame.additionalInfo.video.width = m_width;
    frame.additionalInfo.video.height = m_height;
    frame.additionalInfo.video.isKeyFrame = (encodedImage._frameType == webrtc::VideoFrameType::kVideoFrameKey);

    if (m_parent) {
        if (m_parent->m_frameListener) {
            m_parent->m_frameListener->onAdapterFrame(frame);
        }
        // Check video update
        if (m_parent->m_statsListener) {
            bool statsChanged = false;
            if (format != m_parent->m_format) {
                // Update format
                m_parent->m_format = format;
                statsChanged = true;
            }
            if ((m_parent->m_width != m_width) || (m_parent->m_height != m_height)) {
                // Update width and height
                m_parent->m_width = m_width;
                m_parent->m_height = m_height;
                statsChanged = true;
            }
            if (statsChanged) {
                // Notify the stats
                AdapterStats stats = {
                    m_parent->m_width,
                    m_parent->m_height,
                    m_parent->m_format,
                    0
                };
                m_parent->m_statsListener->onAdapterStats(stats);
            }
        }
        // Dump for debug use
        if (m_parent->m_enableDump && (frame.format == FRAME_FORMAT_H264 || frame.format == FRAME_FORMAT_H265)) {
            dump(this, frame.format, frame.payload, frame.length);
        }
        // Request key frame
        if (m_parent->m_isWaitingKeyFrame) {
            return WEBRTC_VIDEO_CODEC_OK_REQUEST_KEYFRAME;
        }
    }
    return 0;
}

VideoReceiveAdapterImpl::VideoReceiveAdapterImpl(CallOwner* owner, const RtcAdapter::Config& config)
    : m_enableDump(false)
    , m_config(config)
    , m_format(owt_base::FRAME_FORMAT_UNKNOWN)
    , m_width(0)
    , m_height(0)
    , m_frameListener(config.frame_listener)
    , m_rtcpListener(config.rtp_listener)
    , m_statsListener(config.stats_listener)
    , m_isWaitingKeyFrame(false)
    , m_owner(owner)
{
    assert(m_owner != nullptr);
    CreateReceiveVideo();
}

VideoReceiveAdapterImpl::~VideoReceiveAdapterImpl()
{
    std::promise<int> p;
    std::future<int> f = p.get_future();
    taskQueue()->PostTask([this, &p]() {
        if (m_videoRecvStream) {
            RTC_DLOG(LS_INFO) << "Destroy VideoReceiveStream with SSRC: " << m_config.ssrc;
            m_videoRecvStream->Stop();
            call()->DestroyVideoReceiveStream(m_videoRecvStream);
            m_videoRecvStream = nullptr;
        }
        p.set_value(0);
    });
    f.wait();
}

void VideoReceiveAdapterImpl::OnFrame(const webrtc::VideoFrame& video_frame) {}

void VideoReceiveAdapterImpl::CreateReceiveVideo()
{
    taskQueue()->PostTask([this]() {
        if (!m_videoRecvStream) {
            RTC_LOG(LS_INFO) << "Create VideoReceiveStream with SSRC: " << m_config.ssrc;
            // Create Receive Video Stream
            webrtc::VideoReceiveStream::Config default_config(this);
            default_config.rtp.local_ssrc = kLocalSsrc;
            default_config.rtp.rtcp_mode = webrtc::RtcpMode::kReducedSize;
            default_config.rtp.remote_ssrc = m_config.ssrc;
            default_config.rtp.red_payload_type = RED_90000_PT;
            if (m_config.transport_cc) {
                RTC_LOG(LS_INFO) << "TransportSequenceNumber Extension Enabled";
                default_config.rtp.transport_cc = true;
                default_config.rtp.extensions.emplace_back(
                    webrtc::RtpExtension::kTransportSequenceNumberUri, m_config.transport_cc);
            } else {
                default_config.rtp.transport_cc = false;
            }
            /*
            default_config.rtp.nack.rtp_history_ms = rtp_history_ms;
            // Enable RTT calculation so NTP time estimator will work.
            default_config.rtp.rtcp_xr.receiver_reference_time_report =
                receiver_reference_time_report;
            */
            default_config.renderer = this;

            webrtc::VideoReceiveStream::Config video_recv_config(default_config.Copy());
            video_recv_config.decoders.clear();
            /*
            if (!video_send_config.rtp.rtx.ssrcs.empty()) {
              video_recv_config.rtp.rtx_ssrc = video_send_config.rtp.rtx.ssrcs[i];
              video_recv_config.rtp.rtx_associated_payload_types[kSendRtxPayloadType] =
                  video_send_config.rtp.payload_type;
            }
            */
            webrtc::VideoReceiveStream::Decoder decoder;
            decoder.decoder_factory = this;
            // Add VP8 decoder
            decoder.payload_type = VP8_90000_PT;
            decoder.video_format = webrtc::SdpVideoFormat(
                webrtc::CodecTypeToPayloadString(webrtc::VideoCodecType::kVideoCodecVP8));
            RTC_DLOG(LS_INFO) << "Config add decoder:" << decoder.ToString();
            video_recv_config.decoders.push_back(decoder);
            // Add VP9 decoder
            decoder.payload_type = VP9_90000_PT;
            decoder.video_format = webrtc::SdpVideoFormat(
                webrtc::CodecTypeToPayloadString(webrtc::VideoCodecType::kVideoCodecVP9));
            RTC_DLOG(LS_INFO) << "Config add decoder:" << decoder.ToString();
            video_recv_config.decoders.push_back(decoder);
            // Add H264 decoder
            decoder.payload_type = H264_90000_PT;
            decoder.video_format = webrtc::SdpVideoFormat(
                webrtc::CodecTypeToPayloadString(webrtc::VideoCodecType::kVideoCodecH264));
            RTC_DLOG(LS_INFO) << "Config add decoder:" << decoder.ToString();
            video_recv_config.decoders.push_back(decoder);
            // Add H265 decoder
            decoder.payload_type = H265_90000_PT;
            decoder.video_format = webrtc::SdpVideoFormat(
                webrtc::CodecTypeToPayloadString(webrtc::VideoCodecType::kVideoCodecH265));
            RTC_DLOG(LS_INFO) << "Config add decoder:" << decoder.ToString();
            video_recv_config.decoders.push_back(decoder);

            RTC_DLOG(LS_INFO) << "VideoReceiveStream::Config " << video_recv_config.ToString();
            m_videoRecvStream = call()->CreateVideoReceiveStream(std::move(video_recv_config));
            m_videoRecvStream->Start();
            call()->SignalChannelNetworkState(webrtc::MediaType::VIDEO, webrtc::NetworkState::kNetworkUp);
        }
    });
}

void VideoReceiveAdapterImpl::requestKeyFrame()
{
    // m_videoRecvStream->GenerateKeyFrame();
    m_isWaitingKeyFrame = true;
}

std::vector<webrtc::SdpVideoFormat> VideoReceiveAdapterImpl::GetSupportedFormats() const
{
    return std::vector<webrtc::SdpVideoFormat>{
        webrtc::SdpVideoFormat(
            webrtc::CodecTypeToPayloadString(webrtc::VideoCodecType::kVideoCodecVP8)),
        webrtc::SdpVideoFormat(
            webrtc::CodecTypeToPayloadString(webrtc::VideoCodecType::kVideoCodecVP9)),
        webrtc::SdpVideoFormat(
            webrtc::CodecTypeToPayloadString(webrtc::VideoCodecType::kVideoCodecH264)),
        webrtc::SdpVideoFormat(
            webrtc::CodecTypeToPayloadString(webrtc::VideoCodecType::kVideoCodecH265))
    };
}

std::unique_ptr<webrtc::VideoDecoder> VideoReceiveAdapterImpl::CreateVideoDecoder(
    const webrtc::SdpVideoFormat& format)
{
    return std::make_unique<AdapterDecoder>(this);
}

int VideoReceiveAdapterImpl::onRtpData(char* data, int len)
{
    std::shared_ptr<DataPacket> wp = std::make_shared<DataPacket>(data, len);
    taskQueue()->PostTask([this, wp]() {
        call()->Receiver()->DeliverPacket(
            webrtc::MediaType::VIDEO,
            rtc::CopyOnWriteBuffer(wp->data, wp->length),
            rtc::TimeUTCMicros());
    });
    return len;
}

bool VideoReceiveAdapterImpl::SendRtp(const uint8_t* data, size_t len, const webrtc::PacketOptions& options)
{
    RTC_LOG(LS_WARNING) << "VideoReceiveAdapterImpl SendRtp called";
    return true;
}

bool VideoReceiveAdapterImpl::SendRtcp(const uint8_t* data, size_t len)
{
    if (m_rtcpListener) {
        m_rtcpListener->onAdapterData(
            reinterpret_cast<char*>(const_cast<uint8_t*>(data)), len);
        return true;
    }
    return false;
}

} // namespace rtc_adapter
