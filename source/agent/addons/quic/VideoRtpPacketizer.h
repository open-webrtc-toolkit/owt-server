/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_VIDEOPRTPACKETIZER_H_
#define QUIC_VIDEOPRTPACKETIZER_H_

#include "../../core/rtc_adapter/RtcAdapter.h"
#include <logger.h>

// RTP packetizer for video.
class VideoRtpPacketizer : public owt_base::FrameSource, public owt_base::FrameDestination, public rtc_adapter::AdapterFeedbackListener, public rtc_adapter::AdapterStatsListener, public rtc_adapter::AdapterDataListener {
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

private:
    uint32_t m_ssrc;
    std::unique_ptr<rtc_adapter::RtcAdapter> m_rtcAdapter;
    std::unique_ptr<rtc_adapter::VideoSendAdapter> m_videoSend;
};

#endif