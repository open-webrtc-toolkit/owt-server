/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#ifndef LiveStreamOut_h
#define LiveStreamOut_h

#include <string>

#include <logger.h>

#include "AVStreamOut.h"

namespace woogeen_base {

class LiveStreamOut : public AVStreamOut {
    DECLARE_LOGGER();

public:
    LiveStreamOut(const std::string& url, bool hasAudio, bool hasVideo, EventRegistry* handle, int streamingTimeout);
    ~LiveStreamOut();

protected:
    bool isAudioFormatSupported(FrameFormat format) override;
    bool isVideoFormatSupported(FrameFormat format) override;
    const char *getFormatName(std::string& url) override;
    bool writeHeader(void) override;

    uint32_t getKeyFrameInterval(void) override {return 2000;}
    uint32_t getReconnectCount(void) override {return 1;}
};

}

#endif // LiveStreamOut_h
