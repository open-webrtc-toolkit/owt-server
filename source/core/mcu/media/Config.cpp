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

Config::Config() : m_idAccumulator(0)
{
}

inline void Config::signalConfigChanged(uint32_t id)
{
    m_configListeners[id]->onConfigChanged();
}

bool Config::validateVideoRegion(const Region& region)
{
    // region.left, region.top and region.relativeSize should be between 0-1.0
    if (region.left < 0.0 || region.left > 1.0
        || region.top < 0.0 || region.top > 1.0
        || region.relativeSize < 0.0 || region.relativeSize > 1.0) {
        return false;
    }

    return true;
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
    VideoLayout& currentVideoLayout = m_currentVideoLayouts[id];
    VideoResolutionType defaultSize = VideoLayoutHelper::getVideoResolution(defaultRootSize);
    currentVideoLayout.rootSize = defaultSize;
    currentVideoLayout.rootColor = VideoLayoutHelper::getVideoBackgroundColor(defaultBackgroundColor);

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
            targetLayout.maxInput = maxInput;
            targetLayout.rootSize = defaultSize;

            std::string color = layoutPair.second.get<std::string> ("root.backgroundcolor");
            targetLayout.rootColor = VideoLayoutHelper::getVideoBackgroundColor(color);

            BOOST_FOREACH (boost::property_tree::ptree::value_type& regionPair, layoutPair.second.get_child("region")) {
                Region region;
                region.id = regionPair.second.get<std::string>("id");
                region.left = regionPair.second.get<float>("left");
                region.top = regionPair.second.get<float>("top");
                region.relativeSize = regionPair.second.get<float>("relativesize");
                if (validateVideoRegion(region))
                    targetLayout.regions.push_back(region);
                else {
                    ELOG_WARN("Abandoning improper region configuration with id: %s in the video layout with maxInput: %u",
                        region.id.c_str(), maxInput);
                }
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
    if (slotNumber != currentVideoLayout.maxInput) {
        for (uint32_t i = slotNumber - 1; i < MAX_VIDEO_SLOT_NUMBER; ++i) {
            if (m_customVideoLayouts[i].regions.empty())
                continue;

            if (m_customVideoLayouts[i].maxInput != currentVideoLayout.maxInput) {
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
