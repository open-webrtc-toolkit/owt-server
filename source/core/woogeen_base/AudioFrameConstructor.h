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

#ifndef AudioFrameConstructor_h
#define AudioFrameConstructor_h

#include "MediaFramePipeline.h"

#include <logger.h>
#include <MediaDefinitions.h>

namespace woogeen_base {

/**
 * A class to process the incoming streams by leveraging video coding module from
 * webrtc engine, which will framize and decode the frames.
 */
class AudioFrameConstructor : public erizo::MediaSink,
                              public erizo::FeedbackSource,
                              public FrameSource {
    DECLARE_LOGGER();

public:
    AudioFrameConstructor(erizo::FeedbackSink* fbSink);
    virtual ~AudioFrameConstructor();

    // Implements the MediaSink interfaces.
    int deliverAudioData(char*, int len);
    int deliverVideoData(char*, int len);

    // Implements the FrameSource interfaces.
    void onFeedback(const FeedbackMsg& msg);
};

}
#endif /* AudioFrameConstructor_h */
