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

#include "VCMInputProcessor.h"

#include <rtputils.h>
#include <WebRTCTaskRunner.h>

using namespace webrtc;
using namespace erizo;

namespace mcu {

VCMInputProcessor::VCMInputProcessor(int index, bool externalDecoding)
    : m_index(index)
    , m_externalDecoding(externalDecoding)
{
    m_frameConstructor.reset(new woogeen_base::VCMFrameConstructor(index));
}

VCMInputProcessor::~VCMInputProcessor()
{
}

bool VCMInputProcessor::init(woogeen_base::WebRTCTransport<erizo::VIDEO>* transport, boost::shared_ptr<VideoFrameMixer> frameReceiver, boost::shared_ptr<woogeen_base::WebRTCTaskRunner> taskRunner, VCMInputProcessorCallback* initCallback)
{
    if (!m_frameConstructor->initialize(transport, taskRunner))
        return false;

    if (m_externalDecoding) {
        boost::shared_ptr<webrtc::VideoDecoder> decoder(new ExternalDecoder(m_index, frameReceiver, this, initCallback));
        return m_frameConstructor->registerExternalDecoder(decoder);
    }

    boost::shared_ptr<webrtc::VCMReceiveCallback> renderer(new ExternalRenderer(m_index, frameReceiver, this, initCallback));
    return m_frameConstructor->registerDecodedFrameReceiver(renderer);
}

void VCMInputProcessor::bindAudioForSync(int32_t voiceChannelId, webrtc::VoEVideoSync* voe)
{
    m_frameConstructor->syncWithAudio(voiceChannelId, voe);
}

int VCMInputProcessor::deliverVideoData(char* buf, int len)
{
    return m_frameConstructor->deliverVideoData(buf, len);
}

int VCMInputProcessor::deliverAudioData(char* buf, int len)
{
    return m_frameConstructor->deliverAudioData(buf, len);
}

void VCMInputProcessor::setBitrate(unsigned short kbps, int)
{
    m_frameConstructor->setBitrate(kbps);
}

void VCMInputProcessor::requestKeyFrame(int)
{
    m_frameConstructor->RequestKeyFrame();
}

}
