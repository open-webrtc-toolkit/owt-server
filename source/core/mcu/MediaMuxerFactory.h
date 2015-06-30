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

#ifndef MediaMuxerFactory_h
#define MediaMuxerFactory_h

#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <MediaMuxer.h>

namespace mcu {

class MediaMuxerFactory {
    DECLARE_LOGGER();

public:
    static woogeen_base::MediaMuxer* createMediaMuxer(const std::string& id, const std::string& configParam);
    static woogeen_base::MediaMuxer* findMediaMuxer(const std::string& id);
    static bool recycleMediaMuxer(const std::string& id);

private:
    static std::map<std::string, boost::shared_ptr<woogeen_base::MediaMuxer>> s_muxers;
};

}

#endif
