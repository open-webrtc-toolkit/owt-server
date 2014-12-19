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

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <Compiler.h>
#include <sstream>

namespace mcu {

DEFINE_LOGGER(Config, "mcu.media.Config");

Config* Config::m_config = nullptr;

Config* Config::get()
{
    if (!m_config)
        m_config = new Config();

    return m_config;
}

Config::Config()
    : m_idAccumulator(0)
{
}

inline void Config::signalConfigChanged(uint32_t id)
{
    m_configListeners[id]->onConfigChanged();
}

const VideoLayout& Config::getVideoLayout(uint32_t id)
{
    return m_currentVideoLayouts[id];
}

void Config::initVideoLayout(uint32_t id, const std::string& type, const std::string& defaultRootSize,
    const std::string& defaultBackgroundColor, const std::string& customLayout)
{
    ELOG_DEBUG("initVideoLayout configuration");

    // Set the default value for root size, background color
    VideoResolutionType defaultSize = VideoResolutionType::vga;
    for (std::map<std::string, VideoResolutionType>::const_iterator it=VideoResolutions.begin();
        it!=VideoResolutions.end(); ++it) {
        if (!defaultRootSize.compare(it->first)) {
            defaultSize = it->second;
            break;
        }
    }
    VideoLayout& currentVideoLayout = m_currentVideoLayouts[id];

    currentVideoLayout.rootSize = defaultSize;

    VideoBackgroundColor defaultColor = VideoBackgroundColor::black;
    for (std::map<std::string, VideoBackgroundColor>::const_iterator it=VideoColors.begin();
        it!=VideoColors.end(); ++it) {
        if (!defaultBackgroundColor.compare(it->first)) {
            defaultColor = it->second;
            break;
        }
    }
    currentVideoLayout.rootColor = defaultColor;

    // Load the configuration
    if (type.compare("custom")) {
        // The default fluid video layout
        currentVideoLayout.divFactor = 1;
    } else {
        // Customized video layout
        uint32_t initIndex = MAX_VIDEO_SLOT_NUMBER;
        boost::property_tree::ptree pt;
        std::istringstream is (customLayout);
        boost::property_tree::read_json (is, pt);

        // Parsing all the customized layout candidates
        BOOST_FOREACH (boost::property_tree::ptree::value_type& layoutPair, pt.get_child("videolayout")) {
            uint32_t maxInput = layoutPair.second.get<int>("maxinput");
            if (maxInput == 0 || maxInput > MAX_VIDEO_SLOT_NUMBER)
                continue;

            VideoLayout& targetLayout = m_customVideoLayouts[maxInput - 1];
            targetLayout.rootSize = defaultSize;
            targetLayout.rootColor = defaultColor;
            targetLayout.maxInput = maxInput;

            std::string size = layoutPair.second.get<std::string> ("root.size");
            targetLayout.rootSize = VideoResolutionType::vga;
            for (std::map<std::string, VideoResolutionType>::const_iterator it=VideoResolutions.begin();
                it!=VideoResolutions.end(); ++it) {
                if (!size.compare(it->first)) {
                    targetLayout.rootSize = it->second;
                    break;
                }
            }

            std::string color = layoutPair.second.get<std::string> ("root.backgroundcolor");
            targetLayout.rootColor = VideoBackgroundColor::black;
            for (std::map<std::string, VideoBackgroundColor>::const_iterator it=VideoColors.begin();
                it!=VideoColors.end(); ++it) {
                if (!color.compare(it->first)) {
                    targetLayout.rootColor = it->second;
                    break;
                }
            }

            BOOST_FOREACH (boost::property_tree::ptree::value_type& regionPair, layoutPair.second.get_child("region")) {
                Region region;
                region.id = regionPair.second.get<std::string>("id");
                region.left = regionPair.second.get<float>("left");
                region.top = regionPair.second.get<float>("top");
                region.relativeSize = regionPair.second.get<float>("relativesize");
                targetLayout.regions.push_back(region);
            }

            if (initIndex > maxInput - 1)
                initIndex = maxInput - 1;
        }

        // Find the minimum customized layout for initialization purpose
        if (initIndex < MAX_VIDEO_SLOT_NUMBER)
            currentVideoLayout = m_customVideoLayouts[initIndex];
    }

    signalConfigChanged(id);
}

bool Config::updateVideoLayout(uint32_t id, uint32_t slotNumber)
{
    VideoLayout& currentVideoLayout = m_currentVideoLayouts[id];
    if (currentVideoLayout.regions.empty()) {
        // fluid layout
        uint32_t newDivFactor = sqrt(slotNumber > 0 ? slotNumber - 1 : 0) + 1;
        if (newDivFactor != currentVideoLayout.divFactor) {
            currentVideoLayout.divFactor = newDivFactor;
            signalConfigChanged(id);
            return true;
        }

        return false;
    }

    // custom layout
    uint32_t currentRegionNum = currentVideoLayout.regions.size();
    if (slotNumber != currentRegionNum) {
        for (uint32_t i = slotNumber - 1; i < MAX_VIDEO_SLOT_NUMBER; ++i) {
            if (m_customVideoLayouts[i].regions.empty())
                continue;

            if (m_customVideoLayouts[i].regions.size() != currentRegionNum) {
                currentVideoLayout = m_customVideoLayouts[i];
                signalConfigChanged(id);
                return true;
            }

            break;
        }
    }

    return false;
}

uint32_t Config::registerListener(ConfigListener* listener)
{
    // Default video layout definition
    VideoLayout layout;
    layout.rootSize = vga; // Default to VGA
    layout.rootColor = black; // Default to Black
    layout.divFactor = 1;

    uint32_t id = m_idAccumulator++;
    m_configListeners[id] = listener;
    m_currentVideoLayouts[id] = layout;

    return id;
}

void Config::unregisterListener(uint32_t id)
{
    m_currentVideoLayouts.erase(id);
    m_configListeners.erase(id);
}

} /* namespace mcu */
