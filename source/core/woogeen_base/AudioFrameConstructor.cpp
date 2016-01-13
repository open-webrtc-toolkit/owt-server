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

#include "AudioFrameConstructor.h"

#include <rtputils.h>

using namespace erizo;

namespace woogeen_base {

DEFINE_LOGGER(AudioFrameConstructor, "woogeen.AudioFrameConstructor");

AudioFrameConstructor::AudioFrameConstructor(erizo::FeedbackSink* fbSink)
    : m_fbSink(fbSink)

{
    assert(fbSink);
}

AudioFrameConstructor::~AudioFrameConstructor()
{
}

int AudioFrameConstructor::deliverVideoData(char* packet, int len)
{
    assert(false);
    return 0;
}

int AudioFrameConstructor::deliverAudioData(char* buf, int len)
{
    FrameFormat frameFormat;
    Frame frame {};
    memset(&frame, 0, sizeof(frame));
    RTPHeader* head = (RTPHeader*) buf;
    switch (head->getPayloadType()) {
    case PCMU_8000_PT:
        frameFormat = FRAME_FORMAT_PCMU;
        frame.additionalInfo.audio.channels = 1;
        frame.additionalInfo.audio.sampleRate = 8000;
        break;
    case PCMA_8000_PT:
        frameFormat = FRAME_FORMAT_PCMA;
        frame.additionalInfo.audio.channels = 1;
        frame.additionalInfo.audio.sampleRate = 8000;
        break;
    case ISAC_16000_PT:
        frameFormat = FRAME_FORMAT_ISAC16;
        frame.additionalInfo.audio.channels = 1;
        frame.additionalInfo.audio.sampleRate = 16000;
        break;
    case ISAC_32000_PT:
        frameFormat = FRAME_FORMAT_ISAC32;
        frame.additionalInfo.audio.channels = 1;
        frame.additionalInfo.audio.sampleRate = 32000;
        break;
    case OPUS_48000_PT:
        frameFormat = FRAME_FORMAT_OPUS;
        frame.additionalInfo.audio.channels = 2;
        frame.additionalInfo.audio.sampleRate = 48000;
        break;
    default:
        return 0;
    }

    frame.format = frameFormat;
    frame.payload = reinterpret_cast<uint8_t*>(buf);
    frame.length = len;
    frame.timeStamp = head->getTimestamp();
    frame.additionalInfo.audio.isRtpPacket = 1;

    deliverFrame(frame);
    return len;
}

void AudioFrameConstructor::onFeedback(const FeedbackMsg& msg)
{
    if (msg.type == woogeen_base::AUDIO_FEEDBACK) {
        if (msg.cmd == RTCP_PACKET) {
            m_fbSink->deliverFeedback(const_cast<char*>(msg.data.rtcp.buf), msg.data.rtcp.len);
        }
    }
}


}
