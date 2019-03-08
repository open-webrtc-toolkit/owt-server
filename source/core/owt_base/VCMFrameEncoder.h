// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VCMFrameEncoder_h
#define VCMFrameEncoder_h

#include <map>
#include <atomic>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <webrtc/modules/video_coding/codecs/h264/include/h264.h>
#include <webrtc/modules/video_coding/codecs/vp8/include/vp8.h>
#include <webrtc/modules/video_coding/codecs/vp8/temporal_layers.h>
#include <webrtc/modules/video_coding/codecs/vp9/include/vp9.h>
#include <webrtc/modules/video_coding/codecs/i420/include/i420.h>

#include "logger.h"
#include "I420BufferManager.h"
#include "MediaFramePipeline.h"
#include "FrameConverter.h"

using namespace webrtc;

namespace owt_base {

class EncodeOut : public FrameSource {
public:
    EncodeOut(int32_t streamId, owt_base::VideoFrameEncoder* owner, owt_base::FrameDestination* dest)
        : m_streamId(streamId), m_owner(owner), m_out(dest) {
        addVideoDestination(dest);
    }
    virtual ~EncodeOut() {
        removeVideoDestination(m_out);
    }

    void onFeedback(const owt_base::FeedbackMsg& msg) {
        if (msg.type == owt_base::VIDEO_FEEDBACK) {
            if (msg.cmd == REQUEST_KEY_FRAME) {
                m_owner->requestKeyFrame(m_streamId);
            } else if (msg.cmd == SET_BITRATE) {
                m_owner->setBitrate(msg.data.kbps, m_streamId);
            }
        }
    }

    void onEncoded(const owt_base::Frame& frame) {
        deliverFrame(frame);
    }

private:
    int32_t m_streamId;
    owt_base::VideoFrameEncoder* m_owner;
    owt_base::FrameDestination* m_out;
};

/**
 * This is the class to accept the raw frame and encode it to the given format.
 */
class VCMFrameEncoder : public VideoFrameEncoder, public webrtc::EncodedImageCallback {
    DECLARE_LOGGER();

public:
    VCMFrameEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast = false);
    ~VCMFrameEncoder();

    static bool supportFormat(FrameFormat format) {
        return (format == FRAME_FORMAT_VP8
                || format == FRAME_FORMAT_VP9
                || format == FRAME_FORMAT_H264);
    }

    FrameFormat getInputFormat() {return FRAME_FORMAT_I420;}

    // Implements DecodedImageCallback.
    webrtc::EncodedImageCallback::Result OnEncodedImage(const EncodedImage& encoded_frame,
            const CodecSpecificInfo* codec_specific_info,
            const RTPFragmentationHeader* fragmentation) override;

    // Implements VideoFrameEncoder.
    void onFrame(const Frame&);
    bool canSimulcast(FrameFormat format, uint32_t width, uint32_t height);
    bool isIdle();
    int32_t generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, FrameDestination* dest);
    void degenerateStream(int32_t streamId);
    void setBitrate(unsigned short kbps, int32_t streamId);
    void requestKeyFrame(int32_t streamId);

protected:
    static void Encode(VCMFrameEncoder *This, boost::shared_ptr<webrtc::VideoFrame> videoFrame) {This->encode(videoFrame);};
    void encode(boost::shared_ptr<webrtc::VideoFrame> videoFrame);

    boost::shared_ptr<webrtc::VideoFrame> frameConvert(const Frame& frame);

    void dump(uint8_t *buf, int len);

private:
    struct OutStream {
        uint32_t width;
        uint32_t height;
        int32_t  simulcastId;
        boost::shared_ptr<EncodeOut> encodeOut;
    };

    int32_t m_streamId;
    std::map<int32_t/*streamId*/, OutStream> m_streams;

    FrameFormat m_encodeFormat;
    VideoCodecProfile m_profile;
    boost::scoped_ptr<webrtc::VideoEncoder> m_encoder;
    webrtc::TemporalLayersFactory tl_factory_;

    boost::scoped_ptr<I420BufferManager> m_bufferManager;

    boost::shared_mutex m_mutex;

    boost::shared_ptr<boost::asio::io_service> m_srv;
    boost::shared_ptr<boost::asio::io_service::work> m_srvWork;
    boost::shared_ptr<boost::thread> m_thread;

    std::atomic<bool> m_requestKeyFrame;
    std::atomic<uint32_t> m_updateBitrateKbps;

    bool m_isAdaptiveMode;
    int32_t m_width;
    int32_t m_height;
    uint32_t m_frameRate;
    uint32_t m_bitrateKbps;

    boost::scoped_ptr<FrameConverter> m_converter;

    bool m_enableBsDump;
    FILE *m_bsDumpfp;
};

} /* namespace owt_base */
#endif /* VCMFrameEncoder_h */
