// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameConstructor_h
#define VideoFrameConstructor_h

#include "MediaFramePipeline.h"

#include <MediaDefinitionExtra.h>
#include <MediaDefinitions.h>
#include <boost/scoped_ptr.hpp>
#include <logger.h>

#include <JobTimer.h>

#include <RtcAdapter.h>

namespace owt_base {

class VideoInfoListener {
public:
    virtual ~VideoInfoListener(){};
    virtual void onVideoInfo(const std::string& videoInfoJSON) = 0;
};

class KeyFrameRequester {
public:
    virtual ~KeyFrameRequester(){}
    virtual int32_t RequestKeyFrame() = 0;
};

/**
 * A class to process the incoming streams by leveraging video coding module from
 * webrtc engine, which will framize the frames.
 */
class VideoFrameConstructor : public erizo::MediaSink,
                              public erizo::FeedbackSource,
                              public FrameSource,
                              public JobTimerListener,
                              public KeyFrameRequester,
                              public rtc_adapter::AdapterFrameListener,
                              public rtc_adapter::AdapterStatsListener,
                              public rtc_adapter::AdapterDataListener {
    DECLARE_LOGGER();

public:
    struct Config {
        uint32_t transport_cc = 0;
    };

    VideoFrameConstructor(VideoInfoListener*, uint32_t transportccExtId = 0);
    VideoFrameConstructor(std::shared_ptr<rtc_adapter::RtcAdapter>,
                          VideoInfoListener*,
                          uint32_t transportccExtId = 0);
    VideoFrameConstructor(KeyFrameRequester* requester);
    virtual ~VideoFrameConstructor();

    void bindTransport(erizo::MediaSource* source, erizo::FeedbackSink* fbSink);
    void unbindTransport();
    void enable(bool enabled);

    // Implements the JobTimerListener.
    void onTimeout();

    // Implements the FrameSource interfaces.
    void onFeedback(const FeedbackMsg& msg) override;

    // Implements the AdapterFrameListener interfaces.
    void onAdapterFrame(const Frame& frame) override;
    // Implements the AdapterStatsListener interfaces.
    void onAdapterStats(const rtc_adapter::AdapterStats& stats) override;
    // Implements the AdapterDataListener interfaces.
    void onAdapterData(char* data, int len) override;

    // Implements KeyFrameRequester
    int32_t RequestKeyFrame() override;

    bool setBitrate(uint32_t kbps);

    void setPreferredLayers(int spatialId, int temporalId);

    bool addChildProcessor(std::string id, erizo::MediaSink* sink);
    bool removeChildProcessor(std::string id);

private:
    Config m_config;

    void maybeCreateReceiveVideo(uint32_t ssrc);

    // Implement erizo::MediaSink
    int deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet) override;
    int deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet) override;
    int deliverEvent_(erizo::MediaEventPtr event) override;
    void close();

    bool m_enabled;
    uint32_t m_ssrc;

    erizo::MediaSource* m_transport;
    boost::shared_mutex m_transportMutex;
    std::shared_ptr<SharedJobTimer> m_feedbackTimer;
    uint32_t m_pendingKeyFrameRequests;

    VideoInfoListener* m_videoInfoListener;

    std::shared_ptr<rtc_adapter::RtcAdapter> m_rtcAdapter;
    rtc_adapter::VideoReceiveAdapter* m_videoReceive;

    std::map<std::string, erizo::MediaSink*> m_childProcessors;
    int m_currentSpatialLayer = -1;
    int m_currentTemporalLayer = -1;
    KeyFrameRequester* m_requester = nullptr;
};

} // namespace owt_base

#endif /* VideoFrameConstructor_h */
