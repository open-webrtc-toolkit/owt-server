/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
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

#ifndef VideoFrameSender_h
#define VideoFrameSender_h

#include "VideoFramePipeline.h"

#include <boost/shared_ptr.hpp>
#include <MediaDefinitions.h>
#include <WebRTCFeedbackProcessor.h>

namespace mcu {

class TaskRunner;

/**
 * This is the class to send out the video frame with a given format.
 */
class VideoFrameSender : public VideoFrameConsumer {
public:
    VideoFrameSender(int id)
        : m_id(id)
    {
    }

    virtual ~VideoFrameSender() { }

    virtual bool setSendCodec(FrameFormat, VideoSize) = 0;
    virtual bool updateVideoSize(VideoSize) = 0;
    virtual uint32_t sendSSRC() = 0;
    virtual woogeen_base::IntraFrameCallback* iFrameCallback() = 0;
    virtual erizo::FeedbackSink* feedbackSink() = 0;
    virtual int id() { return m_id; }

protected:
    int m_id;
};

}
#endif /* VideoFrameSender_h */
