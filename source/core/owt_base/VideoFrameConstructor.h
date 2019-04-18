// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameConstructor_h
#define VideoFrameConstructor_h

#include "WebRTCTaskRunner.h"
#include "WebRTCTransport.h"
#include "MediaFramePipeline.h"
#include "VieReceiver.h"
#include "VieRemb.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <MediaDefinitionExtra.h>

#include <JobTimer.h>
#include <webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h>
#include <webrtc/modules/remote_bitrate_estimator/remote_estimator_proxy.h>
#include <webrtc/modules/rtp_rtcp/include/rtp_rtcp.h>
#include <webrtc/modules/pacing/packet_router.h>
#include <webrtc/modules/video_coding/include/video_codec_interface.h>
#include <webrtc/modules/video_coding/include/video_coding.h>
#include <webrtc/modules/video_coding/include/video_coding_defines.h>
#include <webrtc/modules/video_coding/video_coding_impl.h>


namespace owt_base {

class VideoInfoListener {
public:
    virtual ~VideoInfoListener(){};
    virtual void onVideoInfo(const std::string& videoInfoJSON) = 0;
};


/**
 * A class to process the incoming streams by leveraging video coding module from
 * webrtc engine, which will framize and decode the frames.
 */
class VideoFrameConstructor : public erizo::MediaSink,
                              public FrameSource,
                              public JobTimerListener,
                              public webrtc::VideoDecoder,
                              public webrtc::RtpFeedback,
                              public webrtc::VCMReceiveCallback,
                              public webrtc::VCMFrameTypeCallback,
                              public webrtc::VCMPacketRequestCallback {
    DECLARE_LOGGER();

public:
    VideoFrameConstructor(VideoInfoListener*);
    virtual ~VideoFrameConstructor();

    void bindTransport(erizo::MediaSource* source, erizo::FeedbackSink* fbSink);
    void unbindTransport();
    void enable(bool enabled);

    // Implements the JobTimerListener.
    void onTimeout();

    // Implements the webrtc::VCMPacketRequestCallback interface.
    virtual int32_t ResendPackets(const uint16_t* sequenceNumbers, uint16_t length);

    // Implements the webrtc::VCMFrameTypeCallback interface.
    virtual int32_t RequestKeyFrame();

    // Implements the webrtc::VCMReceiveCallback interface.
    virtual int32_t FrameToRender(webrtc::VideoFrame& videoFrame,
                                  rtc::Optional<uint8_t> qp) { return 0; }

    // Implements the webrtc::RtpFeedback interface.
    virtual int32_t OnInitializeDecoder(
        const int8_t payload_type,
        const char payload_name[RTP_PAYLOAD_NAME_SIZE],
        const int frequency,
        const size_t channels,
        const uint32_t rate);
    virtual void OnIncomingSSRCChanged( const uint32_t ssrc);
    virtual void OnIncomingCSRCChanged( const uint32_t CSRC, const bool added) { }
    virtual void ResetStatistics(uint32_t ssrc);

    // Implements the webrtc::VideoDecoder interface.
    virtual int32_t InitDecode(const webrtc::VideoCodec* codecSettings, int32_t numberOfCores);
    virtual int32_t Decode(const webrtc::EncodedImage& inputImage,
                           bool missingFrames,
                           const webrtc::RTPFragmentationHeader* fragmentation,
                           const webrtc::CodecSpecificInfo* codecSpecificInfo = NULL,
                           int64_t renderTimeMs = -1);
    virtual int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) { return 0; }
    virtual int32_t Release() { return 0; }
    virtual int32_t Reset() { return 0; }


    // Implements the MediaSink interfaces.
    // int deliverAudioData(char*, int len);
    // int deliverVideoData(char*, int len);

    // Implements the FrameSource interfaces.
    void onFeedback(const FeedbackMsg& msg);

    bool setBitrate(uint32_t kbps);

private:
    bool init();

    bool m_enabled;
    bool m_enableDump;
    FrameFormat m_format;
    uint16_t m_width;
    uint16_t m_height;
    uint32_t m_ssrc;

    boost::scoped_ptr<webrtc::vcm::VideoReceiver> m_video_receiver;
    boost::scoped_ptr<webrtc::VieRemb> m_remoteBitrateObserver;
    boost::scoped_ptr<webrtc::RemoteBitrateEstimator> m_remoteBitrateEstimator;
    boost::scoped_ptr<webrtc::ViEReceiver> m_videoReceiver;
    boost::scoped_ptr<webrtc::RtpRtcp> m_rtpRtcp;
    boost::scoped_ptr<webrtc::PacketRouter> m_packetRouter;
    boost::shared_mutex m_rtpRtcpMutex;
    boost::shared_ptr<WebRTCTransport<erizoExtra::VIDEO>> m_videoTransport;

    boost::shared_ptr<WebRTCTaskRunner> m_taskRunner;

    erizo::MediaSource* m_transport;
    boost::shared_mutex m_transport_mutex;
    boost::scoped_ptr<JobTimer> m_feedbackTimer;
    uint32_t m_pendingKeyFrameRequests;

    ////////////// NEW INTERFACE ///////////
    int deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet) override;
    int deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet) override;
    int deliverEvent_(erizo::MediaEventPtr event) override;
    void close();

    char buf[1500];

    VideoInfoListener* m_videoInfoListener;
};

class DummyRemoteBitrateObserver : public webrtc::RemoteBitrateObserver {
public:
    virtual void OnReceiveBitrateChanged(const std::vector<unsigned int>& ssrcs, unsigned int bitrate) { }
};

}
#endif /* VideoFrameConstructor_h */
