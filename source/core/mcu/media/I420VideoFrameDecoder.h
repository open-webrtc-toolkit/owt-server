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

#ifndef I420VideoFrameDecoder_h
#define I420VideoFrameDecoder_h

#include "VideoFrameMixer.h"

#include <boost/shared_ptr.hpp>
#include <VideoFramePipeline.h>

namespace mcu {

class I420VideoFrameDecoder : public woogeen_base::VideoFrameDecoder {
public:
    I420VideoFrameDecoder(int input, boost::shared_ptr<VideoFrameCompositor>);
    ~I420VideoFrameDecoder();

    bool setInput(woogeen_base::FrameFormat, woogeen_base::VideoFrameProvider*);
    void unsetInput();
    void onFrame(woogeen_base::FrameFormat, unsigned char* payload, int len, unsigned int ts);

private:
    int m_input;
    boost::shared_ptr<VideoFrameCompositor> m_compositor;
};

I420VideoFrameDecoder::I420VideoFrameDecoder(int input, boost::shared_ptr<VideoFrameCompositor> compositor)
    : m_input(input)
    , m_compositor(compositor)
{
    m_compositor->activateInput(m_input);
}

I420VideoFrameDecoder::~I420VideoFrameDecoder()
{
    m_compositor->deActivateInput(m_input);
}

inline void I420VideoFrameDecoder::onFrame(woogeen_base::FrameFormat format, unsigned char* payload, int len, unsigned int ts)
{
    assert(format == woogeen_base::FRAME_FORMAT_I420);
    webrtc::I420VideoFrame* frame = reinterpret_cast<webrtc::I420VideoFrame*>(payload);
    m_compositor->pushInput(m_input, frame);
}

inline bool I420VideoFrameDecoder::setInput(woogeen_base::FrameFormat format, woogeen_base::VideoFrameProvider*)
{
    assert(format == woogeen_base::FRAME_FORMAT_I420);
    return true;
}

inline void I420VideoFrameDecoder::unsetInput()
{
}

}
#endif
