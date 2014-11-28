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

#include "Config.h"
#include "VideoLayout.h"

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <Compiler.h>
#include <sstream>

namespace mcu {

Config* Config::m_config = nullptr;

Config* Config::get()
{
    if (!m_config)
        m_config = new Config();

    return m_config;
}

inline void Config::signalConfigChanged()
{
    for (std::list<ConfigListener*>::iterator listenerIter = m_configListener.begin();
            listenerIter != m_configListener.end();
            listenerIter++)
        (*listenerIter)->onConfigChanged();
}

VideoLayout* Config::getVideoLayout()
{
    return m_videoLayout.get();
}

void Config::setVideoLayout(const std::string& layoutStr)
{
    boost::property_tree::ptree pt;
    std::istringstream is (layoutStr);
    boost::property_tree::read_json (is, pt);
    VideoLayout layout;
    std::string size = pt.get<std::string> ("root.size");
    layout.rootsize = VideoResolutionType::vga;
    for(int i = 0; i < VideoResolutionType::total; i++) {
        if (!size.compare(VideoResString[i])) {
            layout.rootsize = (VideoResolutionType)i;
            break;
        }
    }

    BOOST_FOREACH(boost::property_tree::ptree::value_type &regionlist, pt.get_child("region"))
    {
        Region region;
        region.id = regionlist.second.get<std::string>("id");
        region.left = regionlist.second.get<float>("left");
        region.top = regionlist.second.get<float>("top");
        region.relativesize = regionlist.second.get<float>("relativesize");
        layout.regions.push_back(region);
    }

    setVideoLayout(layout);
}

void Config::setVideoLayout(VideoLayout& layout)
{
    m_videoLayout.reset(new VideoLayout(layout));
    signalConfigChanged();
}

void Config::registerListener(ConfigListener* listener)
{
    m_configListener.push_back(listener);
}

void Config::unregisterListener(ConfigListener* listener)
{
    m_configListener.remove(listener);
}

Config::Config()
{
}

} /* namespace mcu */
