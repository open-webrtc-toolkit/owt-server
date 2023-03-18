// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoFramePacketizer.h"
#include "MediaUtilities.h"
#include <rtputils.h>

using namespace rtc_adapter;

namespace owt_base {

// To make it consistent with the webrtc library, we allow packets to be transmitted
// in up to 2 times max video bitrate if the bandwidth estimate allows it.
static const int TRANSMISSION_MAXBITRATE_MULTIPLIER = 2;

// Interval for getting send-side estimated bandwidth
static const int kBitrateEstimationInterval = 5;

DEFINE_LOGGER(VideoFramePacketizer, "owt.VideoFramePacketizer");

VideoFramePacketizer::VideoFramePacketizer(VideoFramePacketizer::Config& config)
    : m_enabled(true)
    , m_frameFormat(FRAME_FORMAT_UNKNOWN)
    , m_frameWidth(0)
    , m_frameHeight(0)
    , m_ssrc(0)
    , m_sendFrameCount(0)
    , m_rtcAdapter(config.rtcAdapter)
    , m_videoSend(nullptr)
{
    video_sink_ = nullptr;
    if (!m_rtcAdapter) {
        ELOG_DEBUG("Create RtcAdapter");
        m_rtcAdapter.reset(RtcAdapterFactory::CreateRtcAdapter());
    }
    if (config.enableBandwidthEstimation) {
        m_feedbackTimer = SharedJobTimer::GetSharedFrequencyTimer(
            kBitrateEstimationInterval);
        m_feedbackTimer->addListener(this);
    }
    init(config);
}

VideoFramePacketizer::~VideoFramePacketizer()
{
    if (m_feedbackTimer) {
        m_feedbackTimer->removeListener(this);
        m_feedbackTimer.reset();
    }
    close();
    if (m_videoSend) {
        m_rtcAdapter->destoryVideoSender(m_videoSend);
        m_rtcAdapter.reset();
        m_videoSend = nullptr;
    }
}

bool VideoFramePacketizer::init(VideoFramePacketizer::Config& config)
{
    if (!m_videoSend) {
        // Create Send Video Stream
        rtc_adapter::RtcAdapter::Config sendConfig;
        if (config.enableTransportcc) {
            sendConfig.transport_cc = config.transportccExt;
        }
        if (config.enableRed) {
            sendConfig.red_payload = RED_90000_PT;
        }
        if (config.enableUlpfec) {
            sendConfig.ulpfec_payload = ULP_90000_PT;
        }
        if (!config.mid.empty()) {
            memset(sendConfig.mid, 0, sizeof(sendConfig.mid));
            strncat(sendConfig.mid, config.mid.c_str(), sizeof(sendConfig.mid) - 1);
            sendConfig.mid_ext = config.midExtId;
        }
        if (config.enableBandwidthEstimation) {
            sendConfig.bandwidth_estimation = true;
        }
        sendConfig.feedback_listener = this;
        sendConfig.rtp_listener = this;
        sendConfig.stats_listener = this;
        m_videoSend = m_rtcAdapter->createVideoSender(sendConfig);
        m_ssrc = m_videoSend->ssrc();
        return true;
    }

    return false;
}

void VideoFramePacketizer::bindTransport(erizo::MediaSink* sink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_transportMutex);
    video_sink_ = sink;
    video_sink_->setVideoSinkSSRC(m_videoSend->ssrc());
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
        m_sendFrameCount = 0;
        if (m_videoSend) {
            m_videoSend->reset();
        }
    }
}

uint32_t VideoFramePacketizer::getTotalBitrate()
{
    uint32_t totalBitrateBps = 0;
    if (m_videoSend) {
        totalBitrateBps = m_videoSend->getStats().total_bitrate_bps;
    }
    return totalBitrateBps;
}

uint32_t VideoFramePacketizer::getRetransmitBitrate()
{
    uint32_t retransmitBitrateBps = 0;
    if (m_videoSend) {
        retransmitBitrateBps = m_videoSend->getStats().retransmit_bitrate_bps;
    }
    return retransmitBitrateBps;
}

uint32_t VideoFramePacketizer::getEstimatedBandwidth()
{
    uint32_t estimatedBandwidth = 0;
    if (m_videoSend) {
        estimatedBandwidth = m_videoSend->getStats().estimated_bandwidth;
    }
    return estimatedBandwidth;
}

void VideoFramePacketizer::onTimeout()
{
    if (m_videoSend) {
        auto stats = m_videoSend->getStats();
        uint32_t bandwidthBps = stats.estimated_bandwidth;
        ELOG_DEBUG("Estimated bandwidth: %u", bandwidthBps);

        FeedbackMsg msg(VIDEO_FEEDBACK, SET_BITRATE);
        msg.data.kbps = bandwidthBps / 1000;
        deliverFeedbackMsg(msg);
    }
}

void VideoFramePacketizer::onFeedback(const FeedbackMsg& msg)
{
    deliverFeedbackMsg(msg);
}

void VideoFramePacketizer::onAdapterStats(const rtc_adapter::AdapterStats& stats) {
    // if (stats.estimatedBandwidth) {
    //     ELOG_DEBUG("updated estimatedBandwidth %d", stats.estimatedBandwidth);
    //     m_estimatedBitrateBps = stats.estimatedBandwidth;
    // }
}

void VideoFramePacketizer::onAdapterData(char* data, int len)
{
    boost::shared_lock<boost::shared_mutex> lock(m_transportMutex);
    if (!video_sink_) {
        return;
    }

    video_sink_->deliverVideoData(std::make_shared<erizo::DataPacket>(0, data, len, erizo::VIDEO_PACKET));
}

void VideoFramePacketizer::onFrame(const Frame& frame)
{
    if (!m_enabled) {
        return;
    }

    if (m_selfRequestKeyframe) {
        //FIXME: This is a workround for peer client not send key-frame-request
        if (m_sendFrameCount < 151) {
            if ((m_sendFrameCount == 10)
                || (m_sendFrameCount == 30)
                || (m_sendFrameCount == 60)
                || (m_sendFrameCount == 150)) {
                // ELOG_DEBUG("Self generated key-frame-request.");
                FeedbackMsg feedback = {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME };
                deliverFeedbackMsg(feedback);
            }
            m_sendFrameCount += 1;
        }
    }

    if (m_videoSend) {
        m_videoSend->onFrame(frame);
    }
}

void VideoFramePacketizer::onVideoSourceChanged()
{
    if (m_videoSend) {
        m_videoSend->reset();
    }
}

int VideoFramePacketizer::sendFirPacket()
{
    FeedbackMsg feedback = {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME };
    deliverFeedbackMsg(feedback);
    return 0;
}

void VideoFramePacketizer::close()
{
    unbindTransport();
}

int VideoFramePacketizer::deliverFeedback_(std::shared_ptr<erizo::DataPacket> data_packet)
{
    if (m_videoSend) {
        m_videoSend->onRtcpData(data_packet->data, data_packet->length);
        return data_packet->length;
    }
    return 0;
}

int VideoFramePacketizer::sendPLI()
{
    return 0;
}
}
