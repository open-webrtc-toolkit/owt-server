// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_ADAPTER_VIDEO_RECEIVE_ADAPTER_
#define RTC_ADAPTER_VIDEO_RECEIVE_ADAPTER_

#include <AdapterInternalDefinitions.h>
#include <RtcAdapter.h>

#include <api/video_codecs/video_codec.h>
#include <api/video_codecs/video_decoder.h>
#include <api/video_codecs/video_decoder_factory.h>
#include <call/call.h>
#include <rtc_base/task_queue.h>

namespace rtc_adapter {

class VideoReceiveAdapterImpl : public VideoReceiveAdapter,
                                public rtc::VideoSinkInterface<webrtc::VideoFrame>,
                                public webrtc::VideoDecoderFactory,
                                public webrtc::Transport {
public:
    VideoReceiveAdapterImpl(CallOwner* owner, const RtcAdapter::Config& config);
    virtual ~VideoReceiveAdapterImpl();
    // Implement VideoReceiveAdapter
    int onRtpData(char* data, int len) override;
    void requestKeyFrame() override;

    // Implements rtc::VideoSinkInterface<VideoFrame>.
    void OnFrame(const webrtc::VideoFrame& video_frame) override;

    // Implements the webrtc::VideoDecoderFactory interface.
    std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
    std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(
        const webrtc::SdpVideoFormat& format) override;

    // Implements webrtc::Transport
    bool SendRtp(const uint8_t* packet,
        size_t length,
        const webrtc::PacketOptions& options) override;
    bool SendRtcp(const uint8_t* packet, size_t length) override;

private:
    class AdapterDecoder : public webrtc::VideoDecoder {
    public:
        AdapterDecoder(VideoReceiveAdapterImpl* parent)
            : m_parent(parent)
        {
        }

        int32_t InitDecode(const webrtc::VideoCodec* config,
            int32_t number_of_cores) override;

        int32_t Decode(const webrtc::EncodedImage& input,
            bool missing_frames,
            int64_t render_time_ms) override;

        int32_t RegisterDecodeCompleteCallback(
            webrtc::DecodedImageCallback* callback) override { return 0; }

        int32_t Release() override { return 0; }
    private:
        VideoReceiveAdapterImpl* m_parent;
        webrtc::VideoCodecType m_codec;
        uint16_t m_width;
        uint16_t m_height;
        std::unique_ptr<uint8_t[]> m_frameBuffer;
        uint32_t m_bufferSize;
    };

    void CreateReceiveVideo();

    std::shared_ptr<webrtc::Call> call()
    {
        return m_owner ? m_owner->call() : nullptr;
    }
    std::shared_ptr<rtc::TaskQueue> taskQueue()
    {
        return m_owner ? m_owner->taskQueue() : nullptr;
    }

    bool m_enableDump;
    RtcAdapter::Config m_config;
    // Video Statistics collected in decoder thread
    owt_base::FrameFormat m_format;
    uint16_t m_width;
    uint16_t m_height;
    // Listeners
    AdapterFrameListener* m_frameListener;
    AdapterDataListener* m_rtcpListener;
    AdapterStatsListener* m_statsListener;

    std::atomic<bool> m_isWaitingKeyFrame;
    CallOwner* m_owner;

    webrtc::VideoReceiveStream* m_videoRecvStream = nullptr;
};

} // namespace rtc_adapter

#endif /* RTC_ADAPTER_VIDEO_RECEIVE_ADAPTER_ */
