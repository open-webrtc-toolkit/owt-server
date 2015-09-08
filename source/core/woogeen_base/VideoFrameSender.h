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

#include "MediaFramePipeline.h"
#include "WebRTCFeedbackProcessor.h"

#include <MediaDefinitions.h>

namespace woogeen_base {

/**
 * This is the class to send out the video frame with a given format.
 */
class VideoFrameSender : public VideoFrameConsumer {
public:
    virtual ~VideoFrameSender() { }

    virtual bool updateVideoSize(unsigned int width, unsigned int height) = 0;
    virtual bool startSend(bool nack, bool fec) = 0;
    virtual bool stopSend(bool nack, bool fec) = 0;
    virtual uint32_t sendSSRC(bool nack, bool fec) = 0;
    virtual IntraFrameCallback* iFrameCallback() = 0;
    virtual erizo::FeedbackSink* feedbackSink() = 0;
    virtual int streamId() = 0;

    virtual void registerPreSendFrameCallback(FrameConsumer*) { }
    virtual void deRegisterPreSendFrameCallback() { }

protected:
};

}
#endif /* VideoFrameSender_h */
