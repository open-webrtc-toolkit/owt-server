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

#include <cstdio>
#include <rtputils.h>
#include <sys/time.h>

#include "AudioFrame2RtpPacketConverter.h"

namespace mcu {

AudioFrame2RtpPacketConverter::AudioFrame2RtpPacketConverter()
    : m_ssrc(0)
    , m_seqNumber(0)
{
    srand((unsigned)time(0));
    m_ssrc = rand();
}

static int frameFormat2PTC(woogeen_base::FrameFormat format) {
    switch (format) {
    case woogeen_base::FRAME_FORMAT_PCMU:
        return PCMU_8000_PT;
    case woogeen_base::FRAME_FORMAT_PCMA:
        return PCMA_8000_PT;
    case woogeen_base::FRAME_FORMAT_ISAC16:
        return ISAC_16000_PT;
    case woogeen_base::FRAME_FORMAT_ISAC32:
        return ISAC_32000_PT;
    case woogeen_base::FRAME_FORMAT_OPUS:
        return OPUS_48000_PT;
    default:
        return INVALID_PT;
    }
}

uint32_t AudioFrame2RtpPacketConverter::convert(const woogeen_base::Frame& frame, char* buf)
{
    RTPHeader* head = reinterpret_cast<RTPHeader*>(buf);
    memset(head, 0, sizeof(RTPHeader));
    head->setVersion(2);
    head->setSSRC(m_ssrc);
    head->setPayloadType(frameFormat2PTC(frame.format));
    head->setTimestamp(frame.timeStamp);
    head->setSeqNumber(m_seqNumber++);
    head->setMarker(false); // not used.
    memcpy(&buf[head->getHeaderLength()], frame.payload, frame.length);

    return frame.length + head->getHeaderLength();
}

}