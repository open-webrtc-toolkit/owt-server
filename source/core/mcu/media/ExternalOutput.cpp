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

#include "ExternalOutput.h"

namespace mcu {

DEFINE_LOGGER(ExternalOutput, "mcu.media.ExternalOutput");

ExternalOutput::ExternalOutput(woogeen_base::MediaMuxer* muxer) : m_muxer(muxer)
{
    m_videoExternalOutput.reset(new VideoExternalOutput());
    m_audioExternalOutput.reset(new AudioExternalOutput());

    if (m_muxer)
        m_muxer->setMediaSource(m_videoExternalOutput.get(), m_audioExternalOutput.get());
}

ExternalOutput::~ExternalOutput()
{
    if (m_muxer)
        m_muxer->unsetMediaSource();
}

int ExternalOutput::deliverAudioData(char* buf, int len)
{
    return m_audioExternalOutput->deliverAudioData(buf, len);
}

int ExternalOutput::deliverVideoData(char* buf, int len)
{
    return m_videoExternalOutput->deliverVideoData(buf, len);
}

} /* namespace mcu */
