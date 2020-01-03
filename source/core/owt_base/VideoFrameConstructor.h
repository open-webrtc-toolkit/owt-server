// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameConstructor_h
#define VideoFrameConstructor_h

#include "MediaFramePipeline.h"
#include "WebRTCTransport.h"

#include <boost/scoped_ptr.hpp>
#include <MediaDefinitions.h>
#include <MediaDefinitionExtra.h>

#include <JobTimer.h>
#include <modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h>
#include <modules/remote_bitrate_estimator/remote_estimator_proxy.h>
#include <modules/rtp_rtcp/include/rtp_rtcp.h>
#include <modules/pacing/packet_router.h>
#include <modules/video_coding/include/video_codec_interface.h>
#include <modules/video_coding/include/video_coding.h>
#include <modules/video_coding/include/video_coding_defines.h>
#include <modules/video_coding/video_coding_impl.h>

#include <rtc_base/task_queue.h>
#include <call/call.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <api/video_codecs/video_codec.h>
#include <api/video_codecs/video_decoder.h>
#include <api/video_codecs/video_decoder_factory.h>


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
                              public rtc::VideoSinkInterface<webrtc::VideoFrame>,
                              public webrtc::VideoDecoderFactory/*,
                              public webrtc::RtpFeedback,
                              public webrtc::VCMReceiveCallback,
                              public webrtc::VCMFrameTypeCallback,
                              public webrtc::VCMPacketRequestCallback*/ {

public:
    VideoFrameConstructor(VideoInfoListener*, uint32_t transportccExtId = 0);
    virtual ~VideoFrameConstructor();

    void bindTransport(erizo::MediaSource* source, erizo::FeedbackSink* fbSink);
    void unbindTransport();
    void enable(bool enabled);

    // Implements the JobTimerListener.
    void onTimeout();

    // Implements the webrtc::VideoDecoderFactory interface.
    std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
    std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(
      const webrtc::SdpVideoFormat& format) override;

    // Implements rtc::VideoSinkInterface<VideoFrame>.
    void OnFrame(const webrtc::VideoFrame& video_frame) override {};

    // Implements the FrameSource interfaces.
    void onFeedback(const FeedbackMsg& msg);

    int32_t RequestKeyFrame();

    bool setBitrate(uint32_t kbps);

private:
    class AdaptorDecoder : public webrtc::VideoDecoder {
      public:
        AdaptorDecoder(VideoFrameConstructor* src): m_src(src) {}

        int32_t InitDecode(const webrtc::VideoCodec* config,
                           int32_t number_of_cores) override;

        int32_t Decode(const webrtc::EncodedImage& input,
                       bool missing_frames,
                       int64_t render_time_ms) override;

        int32_t RegisterDecodeCompleteCallback(
            webrtc::DecodedImageCallback* callback) override { return 0; }

        int32_t Release() override { return 0; }
      private:
        VideoFrameConstructor* m_src;
        webrtc::VideoCodecType m_codec;
        uint16_t m_width;
        uint16_t m_height;
        std::unique_ptr<uint8_t[]> m_frameBuffer;
        uint32_t m_bufferSize;
    };

    void maybeCreateReceiveVideo(uint32_t ssrc);

    // Implement erizo::MediaSink
    int deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet) override;
    int deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet) override;
    int deliverEvent_(erizo::MediaEventPtr event) override;
    void close();

    bool m_enabled;
    bool m_enableDump;
    FrameFormat m_format;
    uint32_t m_ssrc;

    erizo::MediaSource* m_transport;
    boost::shared_mutex m_transportMutex;
    boost::scoped_ptr<JobTimer> m_feedbackTimer;
    uint32_t m_pendingKeyFrameRequests;
    std::atomic<bool> m_isRequestingKeyFrame;

    VideoInfoListener* m_videoInfoListener;
    std::unique_ptr<WebRTCTransport<erizoExtra::VIDEO>> m_videoTransport;

    // Temporary buffer
    char buf[1500];

    std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory;
    std::unique_ptr<webrtc::RtcEventLog> event_log;
    std::unique_ptr<webrtc::Call> call;
    webrtc::VideoReceiveStream* video_recv_stream = nullptr;
    rtc::TaskQueue task_queue;
};

} // namespace owt_base

#endif /* VideoFrameConstructor_h */
