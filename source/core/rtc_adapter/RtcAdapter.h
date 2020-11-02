// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_ADAPTER_RTC_ADAPTER_H_
#define RTC_ADAPTER_RTC_ADAPTER_H_

#include <MediaFramePipeline.h>

namespace rtc_adapter {

class AdapterDataListener {
public:
    virtual void onAdapterData(char* data, int len) = 0;
};

class AdapterFrameListener {
public:
    virtual void onAdapterFrame(const owt_base::Frame& frame) = 0;
};

class AdapterFeedbackListener {
public:
    virtual void onFeedback(const owt_base::FeedbackMsg& msg) = 0;
};

struct AdapterStats {
    int width = 0;
    int height = 0;
    owt_base::FrameFormat format = owt_base::FRAME_FORMAT_UNKNOWN;
    int estimatedBandwidth = 0;
};

class AdapterStatsListener {
public:
    virtual void onAdapterStats(const AdapterStats& stat) = 0;
};

class VideoReceiveAdapter {
public:
    virtual int onRtpData(char* data, int len) = 0;
    virtual void requestKeyFrame() = 0;
};

class VideoSendAdapter {
public:
    virtual void onFrame(const owt_base::Frame&) = 0;
    virtual int onRtcpData(char* data, int len) = 0;
    virtual uint32_t ssrc() = 0;
    virtual void reset() = 0;
};

class AudioReceiveAdapter {
public:
    virtual int onRtpData(char* data, int len) = 0;
};

class AudioSendAdapter {
public:
    virtual void onFrame(const owt_base::Frame&) = 0;
    virtual int onRtcpData(char* data, int len) = 0;
    virtual uint32_t ssrc() = 0;
};

class RtcAdapter {
public:
    struct Config {
        // SSRC of target stream
        uint32_t ssrc = 0;
        // Transport-cc extension ID
        int transport_cc = 0;
        int red_payload = 0;
        int ulpfec_payload = 0;
        // MID of target stream
        char mid[32];
        // MID extension ID
        int mid_ext = 0;
        AdapterDataListener* rtp_listener = nullptr;
        AdapterStatsListener* stats_listener = nullptr;
        AdapterFrameListener* frame_listener = nullptr;
        AdapterFeedbackListener* feedback_listener = nullptr;
    };
    virtual VideoReceiveAdapter* createVideoReceiver(const Config&) = 0;
    virtual void destoryVideoReceiver(VideoReceiveAdapter*) = 0;
    virtual VideoSendAdapter* createVideoSender(const Config&) = 0;
    virtual void destoryVideoSender(VideoSendAdapter*) = 0;

    virtual AudioReceiveAdapter* createAudioReceiver(const Config&) = 0;
    virtual void destoryAudioReceiver(AudioReceiveAdapter*) = 0;
    virtual AudioSendAdapter* createAudioSender(const Config&) = 0;
    virtual void destoryAudioSender(AudioSendAdapter*) = 0;
    virtual ~RtcAdapter(){}
};

class RtcAdapterFactory {
public:
    static RtcAdapter* CreateRtcAdapter();
    // Use delete instead of this function
    static void DestroyRtcAdapter(RtcAdapter*);
};

} // namespace rtc_adapter

#endif
