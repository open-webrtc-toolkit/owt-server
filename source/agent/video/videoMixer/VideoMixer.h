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

#ifndef VideoMixer_h
#define VideoMixer_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <map>
#include <set>

#include "MediaFramePipeline.h"
#include "VideoLayout.h"

namespace mcu {

class VideoFrameMixer;

class VideoMixer {
    DECLARE_LOGGER();

public:
    VideoMixer(const std::string& config);
    virtual ~VideoMixer();

    bool addInput(const int inputIndex, const std::string& codec, woogeen_base::FrameSource* source, const std::string& avatar);
    void removeInput(const int inputIndex);
    void setInputActive(const int inputIndex, bool active);
    bool addOutput(const std::string& outStreamID
            , const std::string& codec
            , const std::string& resolution
            , const unsigned int framerateFPS
            , const unsigned int bitrateKbps
            , const unsigned int keyFrameIntervalSeconds
            , woogeen_base::FrameDestination* dest);
    void removeOutput(const std::string& outStreamID);

    // Update Layout solution
    void updateLayoutSolution(LayoutSolution& solution);

private:
    void closeAll();

    int m_nextOutputIndex;

    boost::shared_ptr<VideoFrameMixer> m_frameMixer;

    uint32_t m_maxInputCount;
    std::set<int> m_inputs;

    boost::shared_mutex m_outputsMutex;
    std::map<std::string, int32_t> m_outputs;
};

} /* namespace mcu */
#endif /* VideoMixer_h */
