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
#include <MediaFramePipeline.h>

namespace mcu {

class I420VideoFrameDecoder : public woogeen_base::VideoFrameDecoder {
public:
    I420VideoFrameDecoder();
    ~I420VideoFrameDecoder();

    bool init(woogeen_base::FrameFormat format) {return format == woogeen_base::FRAME_FORMAT_I420;}

    void onFrame(const woogeen_base::Frame& frame) {deliverFrame(frame);}
};

}
#endif
