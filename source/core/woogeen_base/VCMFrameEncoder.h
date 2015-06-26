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
#include <list>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>

namespace woogeen_base {

/**
 * This is the class to accept the raw frame and encode it to the given format.
 */
class VCMFrameEncoder : public VideoFrameEncoder, public webrtc::VCMPacketizationCallback {
public:
    VCMFrameEncoder(boost::shared_ptr<WebRTCTaskRunner>);
    ~VCMFrameEncoder();

    // Implements VideoFrameEncoder.
    int32_t addFrameConsumer(const std::string& name, FrameFormat, FrameConsumer*);
    void removeFrameConsumer(int32_t id);
    void onFrame(const Frame&);
    void setBitrate(unsigned short kbps, int id = 0);
    void requestKeyFrame(int id = 0);

    // Implements VCMPacketizationCallback.
    virtual int32_t SendData(
        uint8_t payloadType,
        const webrtc::EncodedImage& encoded_image,
        const webrtc::RTPFragmentationHeader& fragmentationHeader,
        const webrtc::RTPVideoHeader* rtpVideoHdr);

private:
    bool initializeEncoder(uint32_t width, uint32_t height);

    webrtc::VideoCodingModule* m_vcm;
    bool m_encoderInitialized;
    FrameFormat m_encodeFormat;
    boost::shared_ptr<WebRTCTaskRunner> m_taskRunner;
    std::list<VideoFrameConsumer*> m_consumers;
    boost::shared_mutex m_mutex;
};

} /* namespace woogeen_base */
#endif /* VCMFrameEncoder_h */
