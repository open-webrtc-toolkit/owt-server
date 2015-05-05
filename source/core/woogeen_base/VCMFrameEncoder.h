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

#include "TaskRunner.h"
#include "VideoFramePipeline.h"

#include <boost/shared_ptr.hpp>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>

namespace woogeen_base {

/**
 * This is the class to accept the raw frame and encode it to the given format.
 */
class VCMFrameEncoder : public VideoFrameEncoder, public webrtc::VCMPacketizationCallback {
public:
    VCMFrameEncoder(boost::shared_ptr<TaskRunner>);
    ~VCMFrameEncoder();

    // Implements VideoFrameEncoder.
    bool activateOutput(int id, FrameFormat, unsigned int framerate, unsigned short bitrate, VideoFrameConsumer*);
    void deActivateOutput(int id);
    void onFrame(FrameFormat, unsigned char* payload, int len, unsigned int ts);
    void setBitrate(unsigned short bitrate, int id = 0);
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
    boost::shared_ptr<TaskRunner> m_taskRunner;
    VideoFrameConsumer* m_encodedFrameConsumer;
    int m_consumerId;
};

} /* namespace woogeen_base */
#endif /* VCMFrameEncoder_h */
