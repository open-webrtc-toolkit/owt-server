/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VideoRtpPacketizer.h"

DEFINE_LOGGER(VideoRtpPacketizer, "VideoRtpPacketizer");

VideoRtpPacketizer::VideoRtpPacketizer()
    : m_ssrc(0)
    , m_rtcAdapter(std::unique_ptr<rtc_adapter::RtcAdapter>(rtc_adapter::RtcAdapterFactory::CreateRtcAdapter()))
    , m_videoSend(nullptr)
{
    if (!m_videoSend) {
        // Create Send Video Stream
        rtc_adapter::RtcAdapter::Config sendConfig;
        sendConfig.transport_cc = false;
        sendConfig.feedback_listener = this;
        sendConfig.rtp_listener = this;
        sendConfig.stats_listener = this;
        m_videoSend = std::unique_ptr<rtc_adapter::VideoSendAdapter>(m_rtcAdapter->createVideoSender(sendConfig));
        m_ssrc = m_videoSend->ssrc();
    }
}

VideoRtpPacketizer::~VideoRtpPacketizer() { }

void VideoRtpPacketizer::onFrame(const owt_base::Frame& frame)
{
    if (m_videoSend) {
        m_videoSend->onFrame(frame);
    }
}

void VideoRtpPacketizer::onVideoSourceChanged() { }

void VideoRtpPacketizer::onFeedback(const owt_base::FeedbackMsg& msg)
{
    if (msg.cmd == owt_base::RTCP_PACKET) {
        if (m_videoSend) {
            m_videoSend->onRtcpData(msg.buffer.data, msg.buffer.len);
        }
    } else {
        ELOG_WARN("Only RTCP feedbacks can be handled.")
    }
}

void VideoRtpPacketizer::onAdapterStats(const rtc_adapter::AdapterStats& stats)
{
    ELOG_DEBUG("Received adapter stats, do nothing.");
}

void VideoRtpPacketizer::onAdapterData(char* data, int len)
{
    owt_base::Frame frame;
    frame.format = owt_base::FRAME_FORMAT_RTP;
    frame.length = len;
    frame.payload = reinterpret_cast<uint8_t*>(data);
    deliverFrame(frame);
}