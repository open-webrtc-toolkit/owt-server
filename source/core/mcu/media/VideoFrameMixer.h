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

#ifndef VideoFrameMixer_h
#define VideoFrameMixer_h

#include "VideoLayout.h"

#include <MediaFramePipeline.h>
#include <webrtc/common_video/interface/i420_video_frame.h>

namespace mcu {

// VideoFrameCompositor accepts the raw I420VideoFrame from multiple inputs and
// composites them into one I420VideoFrame with the given VideoLayout.
// The composited I420VideoFrame will be handed over to one VideoFrameConsumer.
class VideoFrameCompositor : public LayoutConsumer {
public:
    virtual bool activateInput(int input) = 0;
    virtual void deActivateInput(int input) = 0;
    virtual void pushInput(int input, webrtc::I420VideoFrame*) = 0;

    virtual bool setOutput(woogeen_base::VideoFrameConsumer*) = 0;
    virtual void unsetOutput() = 0;
};

// VideoFrameMixer accepts frames from multiple inputs and mixes them.
// It can have multiple outputs with different FrameFormat or framerate/bitrate settings.
class VideoFrameMixer : public LayoutConsumer, public woogeen_base::VideoFrameProvider {
public:
    virtual bool activateInput(int input, woogeen_base::FrameFormat, woogeen_base::VideoFrameProvider*) = 0;
    virtual void deActivateInput(int input) = 0;
    virtual void pushInput(int input, unsigned char* payload, int len) = 0;
};

}
#endif
