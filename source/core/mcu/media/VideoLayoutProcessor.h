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

#ifndef VideoLayoutProcessor_h
#define VideoLayoutProcessor_h

#include <boost/property_tree/ptree.hpp>
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <logger.h>

#include "VideoLayout.h"

namespace mcu {

class VideoLayoutProcessor{
    DECLARE_LOGGER();
public:
    VideoLayoutProcessor(LayoutConsumer* consumer, boost::property_tree::ptree& layoutConfig);
    virtual ~VideoLayoutProcessor();

    bool getRootSize(VideoSize& rootSize);
    bool setRootSize(const std::string& resolution);

    bool getBgColor(YUVColor& bgColor);
    bool setBgColor(const std::string& color);

    void addInput(int input, bool toFront = false);
    void addInput(int input, std::string& specifiedRegionID);
    void removeInput(int input);
    void promoteInput(int input, size_t magnitude);
    void specifyInputRegion(int input, std::string& regionID);

private:
    void updateInputPositions();
    void chooseRegions();

private:
    LayoutConsumer* m_consumer;
    VideoSize m_rootSize;
    YUVColor m_bgColor;
    std::vector<int> m_inputPositions;
    std::vector<Region>* m_currentRegions;
    std::map<size_t/*MaxInputCount*/, std::vector<Region>> m_templates;
};


}
#endif
