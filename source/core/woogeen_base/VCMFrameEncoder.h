/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#include "MediaFramePipeline.h"
#include "WebRTCTaskRunner.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <map>
#include <tuple>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>

#ifdef ENABLE_YAMI
#include "VideoDisplay.h"
#include "YamiVideoFrame.h"
#endif

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
class VCMFrameEncoder : public VideoFrameEncoder, public webrtc::VCMPacketizationCallback {
    DECLARE_LOGGER();

public:
    VCMFrameEncoder(FrameFormat format, boost::shared_ptr<WebRTCTaskRunner>, bool useSimulcast = false);
    ~VCMFrameEncoder();

    // Implements VideoFrameEncoder.
    void onFrame(const Frame&);
    bool canSimulcast(FrameFormat format, uint32_t width, uint32_t height);
    bool isIdle();
    int32_t generateStream(uint32_t width, uint32_t height, FrameDestination* dest);
    void degenerateStream(int32_t streamId);
    void setBitrate(unsigned short kbps, int32_t streamId);
    void requestKeyFrame(int32_t streamId);

    // Implements VCMPacketizationCallback.
    virtual int32_t SendData(
        uint8_t payloadType,
        const webrtc::EncodedImage& encoded_image,
        const webrtc::RTPFragmentationHeader& fragmentationHeader,
        const webrtc::RTPVideoHeader* rtpVideoHdr);

private:
#ifdef ENABLE_YAMI
    uint8_t* mapVASurfaceToVAImage(intptr_t surface, VAImage&);
    void unmapVAImage(const VAImage&);
    bool convertYamiVideoFrameToI420VideoFrame(YamiVideoFrame&, webrtc::I420VideoFrame&);
    boost::shared_ptr<VADisplay> m_vaDisplay;
#endif

    struct OutStream {
        uint32_t width;
        uint32_t height;
        int32_t  simulcastId;
        boost::shared_ptr<EncodeOut> encodeOut;
    };

    int32_t m_streamId;
    webrtc::VideoCodingModule* m_vcm;
    FrameFormat m_encodeFormat;
    boost::shared_ptr<WebRTCTaskRunner> m_taskRunner;

    std::map<int32_t/*streamId*/, OutStream> m_streams;
    boost::shared_mutex m_mutex;
};

} /* namespace woogeen_base */
#endif /* VCMFrameEncoder_h */
