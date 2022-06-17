/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_VIDEOPRTPACKETIZER_H_
#define QUIC_VIDEOPRTPACKETIZER_H_

#include "RtpPacketizerInterface.h"
#include <logger.h>

// RTP packetizer for video.
class VideoRtpPacketizer : public VideoRtpPacketizerInterface {
    DECLARE_LOGGER();

public:
    explicit VideoRtpPacketizer();
    virtual ~VideoRtpPacketizer();

    // Overrides owt_base::FrameDestination.
    void onFrame(const owt_base::Frame&) override;
    void onVideoSourceChanged() override;

    // Overrides AdapterFeedbackListener.
    void onFeedback(const owt_base::FeedbackMsg& msg) override;
    // Overrides AdapterStatsListener.
    void onAdapterStats(const rtc_adapter::AdapterStats& stats) override;
    // Overrides AdapterDataListener.
    void onAdapterData(char* data, int len) override;

    RtpConfig getRtpConfig() override;

private:
    uint32_t m_ssrc;
    std::unique_ptr<rtc_adapter::RtcAdapter> m_rtcAdapter;
    std::unique_ptr<rtc_adapter::VideoSendAdapter> m_videoSend;
};

#endif