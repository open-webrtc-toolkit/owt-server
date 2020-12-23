// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFramePacketizer_h
#define VideoFramePacketizer_h

#include "MediaFramePipeline.h"

#include <MediaDefinitionExtra.h>
#include <MediaDefinitions.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>

#include <RtcAdapter.h>

namespace owt_base {
/**
 * This is the class to accept the encoded frame with the given format,
 * packetize the frame and send them out via the given WebRTCTransport.
 * It also gives the feedback to the encoder based on the feedback from the remote.
 */
class VideoFramePacketizer : public FrameDestination,
                             public erizo::MediaSource,
                             public erizo::FeedbackSink,
                             public rtc_adapter::AdapterFeedbackListener,
                             public rtc_adapter::AdapterStatsListener,
                             public rtc_adapter::AdapterDataListener {
    DECLARE_LOGGER();

public:
    struct Config {
        bool enableRed = false;
        bool enableUlpfec = false;
        bool enableTransportcc = true;
        bool selfRequestKeyframe = false;
        uint32_t transportccExt = 0;
        std::string mid = "";
        uint32_t midExtId = 0;
    };
    VideoFramePacketizer(Config& config);
    ~VideoFramePacketizer();

    void bindTransport(erizo::MediaSink* sink);
    void unbindTransport();
    void enable(bool enabled);
    uint32_t getSsrc() { return m_ssrc; }

    // Implements FrameDestination.
    void onFrame(const Frame&);
    void onVideoSourceChanged() override;

    // Implements erizo::MediaSource.
    int sendFirPacket();

    // Implements the AdapterFeedbackListener interfaces.
    void onFeedback(const FeedbackMsg& msg) override;
    // Implements the AdapterStatsListener interfaces.
    void onAdapterStats(const rtc_adapter::AdapterStats& stats) override;
    // Implements the AdapterDataListener interfaces.
    void onAdapterData(char* data, int len) override;

private:
    bool init(Config& config);
    void close();

    // Implement erizo::FeedbackSink
    int deliverFeedback_(std::shared_ptr<erizo::DataPacket> data_packet);
    // Implement erizo::MediaSource
    int sendPLI();

    bool m_enabled;
    bool m_selfRequestKeyframe;

    FrameFormat m_frameFormat;
    uint16_t m_frameWidth;
    uint16_t m_frameHeight;
    uint32_t m_ssrc;

    boost::shared_mutex m_transportMutex;

    uint16_t m_sendFrameCount;
    std::shared_ptr<rtc_adapter::RtcAdapter> m_rtcAdapter;
    rtc_adapter::VideoSendAdapter* m_videoSend;
};
}
#endif /* EncodedVideoFrameSender_h */
