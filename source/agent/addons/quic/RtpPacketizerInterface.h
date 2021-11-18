/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_PRTPACKETIZERINTERFACE_H_
#define QUIC_PRTPACKETIZERINTERFACE_H_

#include "../../core/owt_base/MediaFramePipeline.h"
#include "../../core/rtc_adapter/RtcAdapter.h"
#include "../common/MediaFramePipelineWrapper.h"

class VideoRtpPacketizerInterface : public owt_base::FrameSource, public owt_base::FrameDestination, public rtc_adapter::AdapterFeedbackListener, public rtc_adapter::AdapterStatsListener, public rtc_adapter::AdapterDataListener {

public:
    explicit VideoRtpPacketizerInterface() = default;
    virtual ~VideoRtpPacketizerInterface() = default;

    // Overrides owt_base::FrameDestination.
    void onFrame(const owt_base::Frame&) override = 0;
    void onVideoSourceChanged() override = 0;

    // Overrides AdapterFeedbackListener.
    void onFeedback(const owt_base::FeedbackMsg& msg) override = 0;
    // Overrides AdapterStatsListener.
    void onAdapterStats(const rtc_adapter::AdapterStats& stats) override = 0;
    // Overrides AdapterDataListener.
    void onAdapterData(char* data, int len) override = 0;
};

#endif