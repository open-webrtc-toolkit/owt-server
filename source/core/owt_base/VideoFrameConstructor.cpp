// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoFrameConstructor.h"

#include <rtputils.h>
#include <common_types.h>
#include <modules/video_coding/timing.h>
#include <modules/remote_bitrate_estimator/remote_bitrate_estimator_single_stream.h>
#include <rtc_base/time_utils.h>
#include <random>
#include <future>

// using namespace webrtc;
using std::cout;
using std::endl;

namespace owt_base {

const uint32_t kBufferSize = 8192;
// Local SSRC has no meaning for receive stream here
const uint32_t kLocalSsrc = 152320;

constexpr char kNameVp8[] = "VP8";
constexpr char kNameVp9[] = "VP9";
constexpr char kNameH264[] = "H264";
constexpr char kNameH265[] = "H265";

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

class FakeTransport : public webrtc::Transport{
public:
    virtual bool SendRtp(const uint8_t* data, size_t len, const webrtc::PacketOptions& options) override {
        return true;
    }
    virtual bool SendRtcp(const uint8_t* data, size_t len) override {
        return true;
    }
// private:
//     erizoExtra::RTPDataReceiver* m_rtpReceiver;
};

int32_t VideoFrameConstructor::AdaptorDecoder::InitDecode(const webrtc::VideoCodec* config,
                                                          int32_t number_of_cores) {
    RTC_DLOG(LS_INFO) << "AdaptorDecoder InitDecode";
    if (config) {
        m_codec = config->codecType;
    }
    m_width = 0;
    m_height = 0;
    if (!m_frameBuffer) {
        m_bufferSize = kBufferSize;
        m_frameBuffer.reset(new uint8_t[m_bufferSize]);
    }
    return 0;
}

int32_t VideoFrameConstructor::AdaptorDecoder::Decode(const webrtc::EncodedImage& encodedImage,
                                                      bool missing_frames,
                                                      int64_t render_time_ms) {
    RTC_DLOG(LS_INFO) << "AdaptorDecoder Decode";
    FrameFormat format = FRAME_FORMAT_UNKNOWN;
    bool resolutionChanged = false;

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

    if (encodedImage._encodedWidth != 0 && m_width != encodedImage._encodedWidth) {
        m_width = encodedImage._encodedWidth;
        resolutionChanged = true;
    };

    if (encodedImage._encodedHeight != 0 && m_height != encodedImage._encodedHeight) {
        m_height = encodedImage._encodedHeight;
        resolutionChanged = true;
    };

    if (resolutionChanged) {
        // Notify resolution change
    }
    if (encodedImage.size() > m_bufferSize) {
        m_bufferSize = encodedImage.size() * 2;
        m_frameBuffer.reset(new uint8_t[m_bufferSize]);
        RTC_DLOG(LS_INFO) << "AdaptorDecoder increase buffer size: " << m_bufferSize;
    }

    memcpy(m_frameBuffer.get(), encodedImage.data(), encodedImage.size());
    Frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.format = format;
    frame.payload = m_frameBuffer.get();
    frame.length = encodedImage.size();
    frame.timeStamp = encodedImage.Timestamp();
    frame.additionalInfo.video.width = encodedImage._encodedWidth;
    frame.additionalInfo.video.height = encodedImage._encodedHeight;
    frame.additionalInfo.video.isKeyFrame = (encodedImage._frameType == webrtc::VideoFrameType::kVideoFrameKey);

    if (m_src) {
        m_src->deliverFrame(frame);
        // Dump for debug use
        if (m_src->m_enableDump && (frame.format == FRAME_FORMAT_H264 || frame.format == FRAME_FORMAT_H265)) {
            dump(this, frame.format, frame.payload, frame.length);
        }
        // Request key frame
        if (m_src->m_isRequestingKeyFrame) {
            return WEBRTC_VIDEO_CODEC_OK_REQUEST_KEYFRAME;
        }
    }
    return 0;
}

VideoFrameConstructor::VideoFrameConstructor(VideoInfoListener* vil, uint32_t transportccExtId)
    : m_enabled(true)
    , m_enableDump(false)
    , m_format(FRAME_FORMAT_UNKNOWN)
    , m_ssrc(0)
    , m_transport(nullptr)
    , m_pendingKeyFrameRequests(0)
    , m_isRequestingKeyFrame(false)
    , m_videoInfoListener(vil)
    , task_queue_factory(webrtc::CreateDefaultTaskQueueFactory())
    , task_queue(task_queue_factory->CreateTaskQueue(
        "CallTaskQueue",
        webrtc::TaskQueueFactory::Priority::NORMAL))
{
    m_videoTransport.reset(new WebRTCTransport<erizoExtra::VIDEO>(nullptr, nullptr));
    sink_fb_source_ = m_videoTransport.get();
    // m_feedbackTimer.reset(new JobTimer(1, this));
    task_queue.PostTask([this]() {
        // Initialize call
        event_log = std::make_unique<webrtc::RtcEventLogNull>();
        webrtc::Call::Config call_config(event_log.get());
        call_config.task_queue_factory = task_queue_factory.get();
        call.reset(webrtc::Call::Create(call_config));
    });
}

VideoFrameConstructor::~VideoFrameConstructor()
{
    // m_feedbackTimer->stop();
    unbindTransport();
    std::promise<int> p;
    std::future<int> f = p.get_future();
    task_queue.PostTask([this, &p]() {
        if (call) {
            if (video_recv_stream) {
                video_recv_stream->Stop();
                call->DestroyVideoReceiveStream(video_recv_stream);
                video_recv_stream = nullptr;
            }
            call.reset();
        }
        p.set_value(0);
    });
    f.wait();
}

void VideoFrameConstructor::maybeCreateReceiveVideo(uint32_t ssrc) {
    task_queue.PostTask([this, ssrc]() {
        if (call && !video_recv_stream) {
            RTC_DLOG(LS_INFO) << "Create VideoReceiveStream with SSRC: " << ssrc;
            // Create Receive Video Stream
            webrtc::VideoReceiveStream::Config default_config(m_videoTransport.get());
            default_config.rtp.transport_cc = true;
            default_config.rtp.local_ssrc = kLocalSsrc;
            default_config.rtp.rtcp_mode = webrtc::RtcpMode::kCompound;
            default_config.rtp.remote_ssrc = m_ssrc;
            default_config.rtp.red_payload_type = RED_90000_PT;
            char ext_url[] = "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01";
            default_config.rtp.extensions.emplace_back(ext_url, 3);
            // default_config.rtp.extensions.push_back(extension);
            // default_config.rtp.nack.rtp_history_ms = rtp_history_ms;
            // Enable RTT calculation so NTP time estimator will work.
            // default_config.rtp.rtcp_xr.receiver_reference_time_report =
            //     receiver_reference_time_report;
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
            decoder.video_format = webrtc::SdpVideoFormat(kNameVp8);
            RTC_DLOG(LS_INFO) <<  "Config add decoder:" << decoder.ToString();
            video_recv_config.decoders.push_back(decoder);
            // Add VP9 decoder
            decoder.payload_type = VP9_90000_PT;
            decoder.video_format = webrtc::SdpVideoFormat(kNameVp9);
            RTC_DLOG(LS_INFO) <<  "Config add decoder:" << decoder.ToString();
            video_recv_config.decoders.push_back(decoder);
            // Add H264 decoder
            decoder.payload_type = H264_90000_PT;
            decoder.video_format = webrtc::SdpVideoFormat(kNameH264);
            RTC_DLOG(LS_INFO) <<  "Config add decoder:" << decoder.ToString();
            video_recv_config.decoders.push_back(decoder);
            // Add H265 decoder
            decoder.payload_type = H265_90000_PT;
            decoder.video_format = webrtc::SdpVideoFormat(kNameH265);
            RTC_DLOG(LS_INFO) <<  "Config add decoder:" << decoder.ToString();
            video_recv_config.decoders.push_back(decoder);

            video_recv_stream = call->CreateVideoReceiveStream(std::move(video_recv_config));
            video_recv_stream->Start();
            call->SignalChannelNetworkState(webrtc::MediaType::VIDEO, webrtc::NetworkState::kNetworkUp);
        }
    });
}

void VideoFrameConstructor::bindTransport(erizo::MediaSource* source, erizo::FeedbackSink* fbSink)
{
    // ELOG_INFO("bindTransport source %p fbsink %p this %p", source, fbSink, this);
    boost::unique_lock<boost::shared_mutex> lock(m_transportMutex);
    m_transport = source;
    m_transport->setVideoSink(this);
    m_transport->setEventSink(this);
    m_videoTransport->setFeedbackSink(fbSink);
}

void VideoFrameConstructor::unbindTransport()
{
    boost::unique_lock<boost::shared_mutex> lock(m_transportMutex);
    if (m_transport) {
        m_videoTransport->setFeedbackSink(nullptr);
        m_transport = nullptr;
    }
}

void VideoFrameConstructor::enable(bool enabled)
{
    m_enabled = enabled;
    RequestKeyFrame();
}

int32_t VideoFrameConstructor::RequestKeyFrame() {
    if (!m_enabled) {
        return 0;
    }
    // video_recv_stream->GenerateKeyFrame();
    m_isRequestingKeyFrame = true;
    return 0;
}

bool VideoFrameConstructor::setBitrate(uint32_t kbps)
{
    // At present we do not react on this request
    return true;
}

std::vector<webrtc::SdpVideoFormat> VideoFrameConstructor::GetSupportedFormats() const {
    return std::vector<webrtc::SdpVideoFormat>{
        webrtc::SdpVideoFormat(kNameVp8),
        webrtc::SdpVideoFormat(kNameVp9),
        webrtc::SdpVideoFormat(kNameH264),
        webrtc::SdpVideoFormat(kNameH265)
    };
}

std::unique_ptr<webrtc::VideoDecoder> VideoFrameConstructor::CreateVideoDecoder(
    const webrtc::SdpVideoFormat& format) {
    return std::make_unique<AdaptorDecoder>(this);
}

int VideoFrameConstructor::deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet)
{
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(video_packet->data);
    uint8_t packetType = chead->getPacketType();

    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT)
        return 0;

    const uint8_t rtcpMinPt = 194, rtcpMaxPt = 223;
    if (packetType >= rtcpMinPt && packetType <= rtcpMaxPt) {
        return 0;
    }

    RTPHeader* head = reinterpret_cast<RTPHeader*>(video_packet->data);
    if (!m_ssrc && head->getSSRC()) {
        m_ssrc = head->getSSRC();
        maybeCreateReceiveVideo(m_ssrc);
    }
    task_queue.PostTask([this, video_packet]() {
        if (call) {
            call->Receiver()->DeliverPacket(
                webrtc::MediaType::VIDEO,
                rtc::CopyOnWriteBuffer(video_packet->data, video_packet->length),
                rtc::TimeUTCMicros());
        }
    });
    return video_packet->length;
}

int VideoFrameConstructor::deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet)
{
    assert(false);
    return 0;
}

void VideoFrameConstructor::onTimeout()
{
    if (m_pendingKeyFrameRequests > 1) {
        RequestKeyFrame();
    }
    m_pendingKeyFrameRequests = 0;
}

void VideoFrameConstructor::onFeedback(const FeedbackMsg& msg)
{
    if (msg.type == owt_base::VIDEO_FEEDBACK) {
        if (msg.cmd == REQUEST_KEY_FRAME) {
            if (!m_pendingKeyFrameRequests) {
                RequestKeyFrame();
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
