/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#ifndef VCMFrameEncoder_h
#define VCMFrameEncoder_h

#include <map>
#include <atomic>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

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

namespace woogeen_base {

class EncodeOut : public FrameSource {
public:
    EncodeOut(int32_t streamId, woogeen_base::VideoFrameEncoder* owner, woogeen_base::FrameDestination* dest)
        : m_streamId(streamId), m_owner(owner), m_out(dest) {
        addVideoDestination(dest);
    }
    virtual ~EncodeOut() {
        removeVideoDestination(m_out);
    }

    void onFeedback(const woogeen_base::FeedbackMsg& msg) {
        if (msg.type == woogeen_base::VIDEO_FEEDBACK) {
            if (msg.cmd == REQUEST_KEY_FRAME) {
                m_owner->requestKeyFrame(m_streamId);
            } else if (msg.cmd == SET_BITRATE) {
                m_owner->setBitrate(msg.data.kbps, m_streamId);
            }
        }
    }

    void onEncoded(const woogeen_base::Frame& frame) {
        deliverFrame(frame);
    }

private:
    int32_t m_streamId;
    woogeen_base::VideoFrameEncoder* m_owner;
    woogeen_base::FrameDestination* m_out;
};

/**
 * This is the class to accept the raw frame and encode it to the given format.
 */
class VCMFrameEncoder : public VideoFrameEncoder, public webrtc::EncodedImageCallback {
    DECLARE_LOGGER();

public:
    VCMFrameEncoder(FrameFormat format, bool useSimulcast = false);
    ~VCMFrameEncoder();

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
    void doEncoding();
    void encodeLoop();
    void encodeOneFrame();
    void dump(uint8_t *buf, int len);

private:
    struct OutStream {
        uint32_t width;
        uint32_t height;
        int32_t  simulcastId;
        boost::shared_ptr<EncodeOut> encodeOut;
    };

    int32_t m_streamId;

    FrameFormat m_encodeFormat;
    boost::scoped_ptr<webrtc::VideoEncoder> m_encoder;
    webrtc::TemporalLayersFactory tl_factory_;

    boost::scoped_ptr<I420BufferManager> m_bufferManager;
    std::map<int32_t/*streamId*/, OutStream> m_streams;
    boost::shared_mutex m_mutex;

    bool m_running;
    boost::thread m_thread;
    boost::mutex m_encMutex;
    boost::condition_variable m_encCond;

    uint32_t m_incomingFrameCount;

    std::atomic<bool> m_requestKeyFrame;
    std::atomic<uint32_t> m_updateBitrateKbps;

    bool m_isAdaptiveMode;
    int32_t m_width;
    int32_t m_height;
    uint32_t m_frameRate;
    uint32_t m_bitrateKbps;

    boost::scoped_ptr<FrameConverter> m_converter;

    boost::shared_ptr<webrtc::VideoFrame> m_busyFrame;

    bool m_enableBsDump;
    FILE *m_bsDumpfp;
};

} /* namespace woogeen_base */
#endif /* VCMFrameEncoder_h */
