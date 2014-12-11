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

#ifndef VideoFramePipeline_h
#define VideoFramePipeline_h

#include "VideoLayout.h"

#include <webrtc/common_video/interface/i420_video_frame.h>

namespace mcu {

enum FrameFormat {
    FRAME_FORMAT_UNKNOWN,
    FRAME_FORMAT_I420,
    FRAME_FORMAT_VP8,
    FRAME_FORMAT_H264
};

class VideoFrameProvider {
public:
    virtual void requestKeyFrame() = 0;
};

class VideoFrameConsumer {
public:
    virtual void onFrame(FrameFormat, unsigned char* payload, int len, unsigned int ts) = 0;
};

// VideoFrameDecoder accepts the input data from exactly one VideoFrameProvider
// and decodes it into raw I420VideoFrame.
class VideoFrameDecoder : public VideoFrameConsumer {
public:
    virtual bool setInput(FrameFormat, VideoFrameProvider*) = 0;
    virtual void unsetInput() = 0;
};

// VideoFrameCompositor accepts the raw I420VideoFrame from multiple inputs and
// composites them into one I420VideoFrame with the given VideoLayout.
// The composited I420VideoFrame will be handed over to one VideoFrameConsumer.
class VideoFrameCompositor {
public:
    virtual bool activateInput(int slot) = 0;
    virtual void deActivateInput(int slot) = 0;
    virtual void pushInput(int slot, webrtc::I420VideoFrame*) = 0;

    virtual void setLayout(const VideoLayout&) = 0;

    virtual bool setOutput(VideoFrameConsumer*) = 0;
    virtual void unsetOutput() = 0;
};

// VideoFrameEncoder consumes the I420VideoFrame and encodes it into the
// given FrameFormat. It can have multiple outputs with different FrameFormat
// or framerate/bitrate settings.
class VideoFrameEncoder : public VideoFrameConsumer {
public:
    virtual bool activateOutput(int id, FrameFormat, unsigned int framerate, unsigned short bitrate, VideoFrameConsumer*) = 0;
    virtual void deActivateOutput(int id) = 0;

    virtual void setBitrate(int id, unsigned short bitrate) = 0;
    virtual void requestKeyFrame(int id) = 0;
};

// VideoFrameMixer accepts frames from multiple inputs and mixes them.
// It can have multiple outputs with different FrameFormat or framerate/bitrate settings.
class VideoFrameMixer {
public:
    virtual bool activateInput(int slot, FrameFormat, VideoFrameProvider*) = 0;
    virtual void deActivateInput(int slot) = 0;
    virtual void pushInput(int slot, unsigned char* payload, int len) = 0;

    virtual void setLayout(const VideoLayout&) = 0;

    virtual void setBitrate(int id, unsigned short bitrate) = 0;
    virtual void requestKeyFrame(int id) = 0;
    virtual bool activateOutput(int id, FrameFormat, unsigned int framerate, unsigned short bitrate, VideoFrameConsumer*) = 0;
    virtual void deActivateOutput(int id) = 0;
};

}
#endif
