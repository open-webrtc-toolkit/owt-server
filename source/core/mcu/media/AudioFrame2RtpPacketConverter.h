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

#ifndef AudioFrame2RtpPacketConverter_h
#define AudioFrame2RtpPacketConverter_h

#include <utility>
#include "MediaFramePipeline.h"

namespace mcu {

class AudioFrame2RtpPacketConverter {

public:
    AudioFrame2RtpPacketConverter();

    uint32_t convert(const woogeen_base::Frame& frame, char* buf);

private:
	unsigned int m_ssrc;
    uint32_t m_seqNumber;
};

}

#endif