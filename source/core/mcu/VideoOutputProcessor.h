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

#ifndef VideoOutputProcessor_h
#define VideoOutputProcessor_h

#include "VideoFrameProcessor.h"

#include <boost/shared_ptr.hpp>
#include <MediaDefinitions.h>
#include <WoogeenTransport.h>

namespace mcu {

class TaskRunner;

/**
 * This is the class to send out the encoded frame via the given WoogeenTransport.
 */
class VideoOutputProcessor : public VideoFrameConsumer {
public:
    enum VideoCodecType {VCT_VP8, VCT_H264};

    VideoOutputProcessor(int id)
        : m_id(id)
    {
    }

    virtual ~VideoOutputProcessor() { }

    virtual bool setSendCodec(VideoCodecType, VideoSize) = 0;
    virtual uint32_t sendSSRC() = 0;
    virtual void onRequestIFrame() = 0;
    virtual erizo::FeedbackSink* feedbackSink() = 0;

protected:
    int m_id;
};

}
#endif /* VideoOutputProcessor_h */
