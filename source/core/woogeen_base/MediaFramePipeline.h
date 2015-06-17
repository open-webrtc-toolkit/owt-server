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

#ifndef MediaFramePipeline_h
#define MediaFramePipeline_h

#include <stdint.h>

namespace woogeen_base {

enum FrameFormat {
    FRAME_FORMAT_UNKNOWN,

    FRAME_FORMAT_I420,
    FRAME_FORMAT_VP8,
    FRAME_FORMAT_H264,

    FRAME_FORMAT_PCM_RAW,
    FRAME_FORMAT_PCMU,
    FRAME_FORMAT_OPUS
};

struct VideoFrameSpecificInfo {
    uint16_t width;
    uint16_t height;
};

struct AudioFrameSpecificInfo {
    uint32_t nbSamples;
    uint32_t sampleRate;
    uint8_t channels;
};

struct Frame {
    FrameFormat format;
    uint8_t* payload;
    uint32_t length;
    uint32_t timeStamp;
    union {
        VideoFrameSpecificInfo video;
        AudioFrameSpecificInfo audio;
    } additionalInfo;
};

class FrameProvider {
public:
    virtual ~FrameProvider() { }

    virtual void setBitrate(unsigned short kbps, int id = 0) = 0;
};

class FrameConsumer {
public:
    virtual ~FrameConsumer() { }

    virtual void onFrame(const Frame&) = 0;
    virtual bool acceptRawFrame() { return true; }
    virtual bool acceptEncodedFrame() { return true; }
};

class VideoFrameProvider : public FrameProvider {
public:
    virtual ~VideoFrameProvider() { }

    virtual void requestKeyFrame(int id = 0) = 0;
};

typedef FrameConsumer VideoFrameConsumer;

// VideoFrameDecoder accepts the input data from exactly one VideoFrameProvider
// and decodes it into raw I420VideoFrame.
class VideoFrameDecoder : public VideoFrameConsumer {
public:
    virtual bool setInput(FrameFormat, VideoFrameProvider*) = 0;
    virtual void unsetInput() = 0;
};

// VideoFrameEncoder consumes the I420 video frame and encodes it into the
// given FrameFormat. It can have multiple outputs with different FrameFormat
// or framerate/bitrate settings.
class VideoFrameEncoder : public VideoFrameConsumer, public VideoFrameProvider {
public:
    virtual bool activateOutput(int id, FrameFormat, unsigned int framerate, unsigned short kbps, VideoFrameConsumer*) = 0;
    virtual void deActivateOutput(int id) = 0;
};

}
#endif
