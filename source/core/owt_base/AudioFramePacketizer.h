// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioFramePacketizer_h
#define AudioFramePacketizer_h

#include "MediaFramePipeline.h"

#include <logger.h>

#include <MediaDefinitionExtra.h>
#include <MediaDefinitions.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <RtcAdapter.h>

namespace owt_base {

/**
 * This is the class to send out the audio frame with a given format.
 */
class AudioFramePacketizer : public FrameDestination,
                             public erizo::MediaSource,
                             public erizo::FeedbackSink,
                             public erizoExtra::RTPDataReceiver,
                             public rtc_adapter::AdapterStatsListener,
                             public rtc_adapter::AdapterDataListener {
    DECLARE_LOGGER();

public:
    struct Config {
        std::string mid = "";
        uint32_t midExtId = 0;
    };
    AudioFramePacketizer(Config& config);
    ~AudioFramePacketizer();

    void bindTransport(erizo::MediaSink* sink);
    void unbindTransport();
    void enable(bool enabled) { m_enabled = enabled; }
    uint32_t getSsrc() { return m_ssrc; }
    void setOwner(std::string owner);

    // Implements FrameDestination.
    void onFrame(const Frame&);
    void onMetaData(const MetaData&);

    // Implements RTPDataReceiver.
    void receiveRtpData(char*, int len, erizoExtra::DataType, uint32_t channelId);

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

    FrameFormat m_frameFormat;
    boost::shared_mutex m_transport_mutex;

    uint16_t m_lastOriginSeqNo;
    uint16_t m_seqNo;
    uint32_t m_ssrc;

    std::shared_ptr<rtc_adapter::RtcAdapter> m_rtcAdapter;
    rtc_adapter::AudioSendAdapter* m_audioSend;
    std::string m_owner;
    std::string m_sourceOwner;
    bool m_firstFrame;
};
}
#endif /* AudioFramePacketizer_h */
