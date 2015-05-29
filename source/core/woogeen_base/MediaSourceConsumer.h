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

#ifndef MediaSourceConsumer_h
#define MediaSourceConsumer_h

#include <MediaDefinitions.h>

namespace woogeen_base {

class MediaSourceConsumer {
public:
    DLL_PUBLIC static MediaSourceConsumer* createMediaSourceConsumerInstance();
    DLL_PUBLIC virtual ~MediaSourceConsumer() { }

    virtual erizo::MediaSink* addSource(uint32_t id,
                                        bool isAudio,
                                        erizo::DataContentType,
                                        const std::string& codecName,
                                        erizo::FeedbackSink*,
                                        const std::string& participantId) = 0;
    virtual void removeSource(uint32_t id, bool isAudio) = 0;
    virtual int32_t bindAV(uint32_t audioSource, uint32_t videoSource) { return -1; }
};

} /* namespace woogeen_base */

#endif /* MediaSourceConsumer_h */
