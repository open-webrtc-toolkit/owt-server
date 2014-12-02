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
#include "BufferManager.h"

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <Compiler.h>
#include <sstream>

namespace mcu {

DEFINE_LOGGER(Config, "mcu.Config");

Config* Config::m_config = nullptr;

Config* Config::get()
{
    if (!m_config)
        m_config = new Config();

    return m_config;
}

inline void Config::signalConfigChanged()
{
    for (std::list<ConfigListener*>::iterator listenerIter = m_configListeners.begin();
            listenerIter != m_configListeners.end();
            listenerIter++)
        (*listenerIter)->onConfigChanged();
}

VideoLayout& Config::getVideoLayout()
{
    return m_currentVideoLayout;
}

void Config::initVideoLayout(const std::string& type, const std::string& defaultRootSize,
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
    m_currentVideoLayout.rootSize = defaultSize;

    VideoBackgroundColor defaultColor = VideoBackgroundColor::black;
    for (std::map<std::string, VideoBackgroundColor>::const_iterator it=VideoColors.begin();
        it!=VideoColors.end(); ++it) {
        if (!defaultBackgroundColor.compare(it->first)) {
            defaultColor = it->second;
            break;
        }
    }
    m_currentVideoLayout.rootColor = defaultColor;

    // Load the configuration
    if (type.compare("custom")) {
        // The default fluid video layout
        m_currentVideoLayout.divFactor = 1;
    } else {
        // Customized video layout
        int initIndex = BufferManager::SLOT_SIZE;
        boost::property_tree::ptree pt;
        std::istringstream is (customLayout);
        boost::property_tree::read_json (is, pt);

        // Parsing all the customized layout candidates
        BOOST_FOREACH (boost::property_tree::ptree::value_type& layoutPair, pt.get_child("videolayout")) {
            int maxInput = layoutPair.second.get<int>("maxinput");
            int validMaxInput = (maxInput > 0 && maxInput <= BufferManager::SLOT_SIZE) ? maxInput : 0;
            if (validMaxInput < 1)
                continue;

            VideoLayout& targetLayout = m_customVideoLayouts[maxInput - 1];
            targetLayout.rootSize = defaultSize;
            targetLayout.rootColor = defaultColor;
            targetLayout.maxInput = validMaxInput;

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
        if (initIndex < BufferManager::SLOT_SIZE)
            m_currentVideoLayout = m_customVideoLayouts[initIndex];
    }

    signalConfigChanged();
}

bool Config::updateVideoLayout(uint32_t slotNumber)
{
    if (m_currentVideoLayout.regions.empty()) {
        // fluid layout
        uint32_t currentDivFactor = m_currentVideoLayout.divFactor;
        bool updateDivFactor = false;
        if (slotNumber <= 1) {
            if (currentDivFactor != 1) {
                m_currentVideoLayout.divFactor = 1;
                updateDivFactor = true;
            }
        } else if (slotNumber <= 4) {
            if (currentDivFactor != 2) {
                m_currentVideoLayout.divFactor = 2;
                updateDivFactor = true;
            }
        } else if (slotNumber <= 9) {
            if (currentDivFactor != 3) {
                m_currentVideoLayout.divFactor = 3;
                updateDivFactor = true;
            }
        } else if (currentDivFactor != 4) {
            m_currentVideoLayout.divFactor = 4;
            updateDivFactor = true;
        }

        if (updateDivFactor) {
            signalConfigChanged();
            return true;
        }

        return false;
    }

    // custom layout
    uint32_t currentRegionNum = m_currentVideoLayout.regions.size();
    if (slotNumber != currentRegionNum) {
        for (int i = slotNumber - 1; i < BufferManager::SLOT_SIZE; ++i) {
            if (m_customVideoLayouts[i].regions.empty())
                continue;

            if (m_customVideoLayouts[i].regions.size() != currentRegionNum) {
                m_currentVideoLayout = m_customVideoLayouts[i];
                signalConfigChanged();
                return true;
            }

            break;
        }
    }

    return false;
}

void Config::registerListener(ConfigListener* listener)
{
    m_configListeners.push_back(listener);
}

void Config::unregisterListener(ConfigListener* listener)
{
    m_configListeners.remove(listener);
}

Config::Config()
{
    m_customVideoLayouts.reserve(BufferManager::SLOT_SIZE);
}

} /* namespace mcu */
