// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioFramePacketizer_h
#define AudioFramePacketizer_h

#include "MediaFramePipeline.h"
#include "WebRTCTransport.h"
#include "SsrcGenerator.h"

#include <logger.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <MediaDefinitions.h>
#include <MediaDefinitionExtra.h>
#include <webrtc/modules/rtp_rtcp/include/rtp_rtcp.h>


namespace owt_base {

class WebRTCTaskRunner;

/**
 * This is the class to send out the audio frame with a given format.
 */
class AudioFramePacketizer : public FrameDestination,
                             public erizo::MediaSource,
                             public erizo::FeedbackSink,
                             public erizoExtra::RTPDataReceiver {
    DECLARE_LOGGER();

public:
    AudioFramePacketizer();
    ~AudioFramePacketizer();

    void bindTransport(erizo::MediaSink* sink);
    void unbindTransport();
    void enable(bool enabled) {m_enabled = enabled;}

    // Implements FrameDestination.
    void onFrame(const Frame&);

    // Implements RTPDataReceiver.
    void receiveRtpData(char*, int len, erizoExtra::DataType, uint32_t channelId);

private:
    bool init();
    bool setSendCodec(FrameFormat format);
    void close();
    void updateSeqNo(uint8_t* rtp);

    bool m_enabled;
    boost::scoped_ptr<webrtc::RtpRtcp> m_rtpRtcp;
    boost::shared_mutex m_rtpRtcpMutex;

    boost::shared_ptr<webrtc::Transport> m_audioTransport;
    boost::shared_ptr<WebRTCTaskRunner> m_taskRunner;
    FrameFormat m_frameFormat;
    boost::shared_mutex m_transport_mutex;

    uint16_t m_lastOriginSeqNo;
    uint16_t m_seqNo;
    uint32_t m_ssrc;
    SsrcGenerator* const m_ssrc_generator;

    ///// NEW INTERFACE ///////////
    int deliverFeedback_(std::shared_ptr<erizo::DataPacket> data_packet);
    int sendPLI();
};

}
#endif /* AudioFramePacketizer_h */
