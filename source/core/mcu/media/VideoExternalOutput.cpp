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

#include "VideoExternalOutput.h"

namespace mcu {

VideoExternalOutput::VideoExternalOutput()
{
    init();
}

VideoExternalOutput::~VideoExternalOutput()
{
    m_taskRunner->Stop();
}

void VideoExternalOutput::init()
{
    // FIXME: Chunbo to set this index value - 0 as default currently
    m_frameConstructor.reset(new woogeen_base::VCMFrameConstructor(0));
    m_taskRunner.reset(new woogeen_base::WebRTCTaskRunner());
    m_taskRunner->Start();

    m_frameConstructor->initialize(nullptr, m_taskRunner);
}

int VideoExternalOutput::deliverVideoData(char* buf, int len)
{
    return m_frameConstructor->deliverVideoData(buf, len);
}

int VideoExternalOutput::deliverAudioData(char* buf, int len)
{
    assert(false);
    return 0;
}

int32_t VideoExternalOutput::addFrameConsumer(const std::string& name, int payloadType, woogeen_base::FrameConsumer* frameConsumer)
{
    // Start the recording of video frames
    m_encodedFrameCallback.reset(new woogeen_base::VideoEncodedFrameCallbackAdapter(frameConsumer));
    m_frameConstructor->registerPreDecodeImageCallback(m_encodedFrameCallback);
    return 0;
}

void VideoExternalOutput::removeFrameConsumer(int32_t id)
{
    // Stop the recording of video frames
    m_frameConstructor->deRegisterPreDecodeImageCallback();
}

} /* namespace mcu */
