// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_ADAPTER_AUDIO_SEND_ADAPTER_
#define RTC_ADAPTER_AUDIO_SEND_ADAPTER_

#include <AdapterInternalDefinitions.h>
#include <RtcAdapter.h>

#include "MediaFramePipeline.h"
#include "SsrcGenerator.h"
#include "WebRTCTaskRunner.h"

#include <api/rtc_event_log/rtc_event_log.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <modules/rtp_rtcp/include/rtp_rtcp.h>
#include <modules/rtp_rtcp/source/rtp_sender_audio.h>

namespace rtc_adapter {

/**
 * This is the class to send out the audio frame with a given format.
 */
class AudioSendAdapterImpl : public AudioSendAdapter,
                             public webrtc::Transport {
public:
    AudioSendAdapterImpl(CallOwner* owner, const RtcAdapter::Config& config);
    ~AudioSendAdapterImpl();

    // Implement AudioSendAdapter
    void onFrame(const owt_base::Frame&) override;
    int onRtcpData(char* data, int len) override;
    uint32_t ssrc() override { return m_ssrc; }

    // Implement webrtc::Transport
    bool SendRtp(const uint8_t* packet,
        size_t length,
        const webrtc::PacketOptions& options) override;
    bool SendRtcp(const uint8_t* packet, size_t length) override;

private:
    bool init();
    bool setSendCodec(owt_base::FrameFormat format);
    void close();
    void updateSeqNo(uint8_t* rtp);

    boost::shared_mutex m_rtpRtcpMutex;
    std::unique_ptr<webrtc::RtpRtcp> m_rtpRtcp;

    boost::shared_ptr<owt_base::WebRTCTaskRunner> m_taskRunner;
    owt_base::FrameFormat m_frameFormat;
    boost::shared_mutex m_transport_mutex;

    uint16_t m_lastOriginSeqNo;
    uint16_t m_seqNo;
    uint32_t m_ssrc;
    owt_base::SsrcGenerator* const m_ssrc_generator;

    webrtc::Clock* m_clock;
    std::unique_ptr<webrtc::RtcEventLog> m_eventLog;
    std::unique_ptr<webrtc::RTPSenderAudio> m_senderAudio;

    RtcAdapter::Config m_config;
    // Listeners
    AdapterDataListener* m_rtpListener;
    AdapterStatsListener* m_statsListener;
    // TODO: remove extensionMap and mid if frames do not carry rtp packets
    webrtc::RtpHeaderExtensionMap m_extensions;
    std::string m_mid;
};
}
#endif /* RTC_ADAPTER_AUDIO_SEND_ADAPTER_ */
