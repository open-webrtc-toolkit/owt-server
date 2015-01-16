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

#include "VideoLayoutProcessor.h"
#include <boost/foreach.hpp>

namespace mcu {

DEFINE_LOGGER(VideoLayoutProcessor, "mcu.media.VideoLayoutProcessor");

VideoLayoutProcessor::VideoLayoutProcessor(boost::property_tree::ptree& layoutConfig)
    : m_currentRegions(nullptr)
{
    std::string resolution = layoutConfig.get<std::string>("resolution");
    if (!VideoResolutionHelper::getVideoSize(resolution, m_rootSize)) {
        ELOG_WARN("configured resolution is invalid!");
        VideoResolutionHelper::getVideoSize("vga", m_rootSize);
    }

    std::string color = layoutConfig.get<std::string>("backgroundcolor");
    if (!VideoColorHelper::getVideoColor(color, m_bgColor)) {
        ELOG_WARN("configured background color is invalid!");
        VideoColorHelper::getVideoColor("black", m_bgColor);
    }

    BOOST_FOREACH (boost::property_tree::ptree::value_type& layoutTemplate, layoutConfig.get_child("templates")) {
        std::vector<Region> regions;

        BOOST_FOREACH (boost::property_tree::ptree::value_type& regionPair, layoutTemplate.second.get_child("region")) {
            Region region;
            region.id = regionPair.second.get<std::string>("id");
            region.left = regionPair.second.get<float>("left");
            region.top = regionPair.second.get<float>("top");
            region.relativeSize = regionPair.second.get<float>("relativesize");
            if (region.left < 0.0 || region.left > 1.0
                || region.top < 0.0 || region.top > 1.0
                || region.relativeSize < 0.0 || region.relativeSize > 1.0) {
                ELOG_WARN("Abandoning improper region configuration with id: %s in dynamic video layout template.", region.id.c_str());
            } else
                regions.push_back(region);
        }

        if (regions.size() > 0)
            m_templates[regions.size()] = regions;
    }

    ELOG_DEBUG("Init OK, resolution:%s, bgcolor:%s, layout templates count:%zu", resolution.c_str(), color.c_str(), m_templates.size());

    m_currentRegions = &(m_templates.begin()->second);
}

VideoLayoutProcessor::~VideoLayoutProcessor()
{
}

void VideoLayoutProcessor::registerConsumer(boost::shared_ptr<LayoutConsumer> consumer)
{
    std::list<boost::shared_ptr<LayoutConsumer>>::iterator it = m_consumers.begin();
    for (; it != m_consumers.end(); ++it) {
        if (*it == consumer)
            break;
    }

    if (it == m_consumers.end())
        m_consumers.push_back(consumer);
}

void VideoLayoutProcessor::deregisterConsumer(boost::shared_ptr<LayoutConsumer> consumer)
{
    std::list<boost::shared_ptr<LayoutConsumer>>::iterator it = m_consumers.begin();
    for (; it != m_consumers.end(); ++it) {
        if (*it == consumer) {
            m_consumers.erase(it);
            break;
        }
    }
}

bool VideoLayoutProcessor::getRootSize(VideoSize& rootSize)
{
    rootSize = m_rootSize;
    return true;
}

bool VideoLayoutProcessor::setRootSize(const std::string& resolution)
{
    VideoSize newSize;
    if (VideoResolutionHelper::getVideoSize(resolution, newSize)) {
        if (!(m_rootSize.width == newSize.width && m_rootSize.height == newSize.height)) {
            m_rootSize = newSize;
            std::list<boost::shared_ptr<LayoutConsumer>>::iterator it = m_consumers.begin();
            for (; it != m_consumers.end(); ++it)
                (*it)->updateRootSize(m_rootSize);
        }
        return true;
    }
    return false;
}

bool VideoLayoutProcessor::getBgColor(YUVColor& bgColor)
{
    bgColor = m_bgColor;
    return true;
}

bool VideoLayoutProcessor::setBgColor(const std::string& color)
{
    YUVColor newColor;
    if (VideoColorHelper::getVideoColor(color, newColor)) {
        if (!(m_bgColor.y == newColor.y && m_bgColor.cb == newColor.cb && m_bgColor.cr == newColor.cr)) {
            m_bgColor = newColor;
            std::list<boost::shared_ptr<LayoutConsumer>>::iterator it = m_consumers.begin();
            for (; it != m_consumers.end(); ++it)
                (*it)->updateBackgroundColor(m_bgColor);
        }
        return true;
    }
    return false;
}

void VideoLayoutProcessor::addInput(int input, bool toFront)
{
    ELOG_DEBUG("addInput, input: %d", input);
    if (toFront)
        m_inputPositions.insert(m_inputPositions.begin(), input);
    m_inputPositions.push_back(input);
    updateInputPositions();
}

void VideoLayoutProcessor::addInput(int input, std::string& specifiedRegionID)
{
    std::vector<int>::iterator pos = m_inputPositions.begin();
    std::vector<Region>::iterator itRegion = m_currentRegions->begin();
    for (; itRegion != m_currentRegions->end(); ++itRegion) {
        if (pos != m_inputPositions.end())
            pos++;
        if (itRegion->id == specifiedRegionID)
            break;
    }

    if (itRegion != m_currentRegions->end())
        m_inputPositions.insert(pos, input);
    else {
        ELOG_WARN("There are empty positions previous the specified region, append to the end.");
        m_inputPositions.push_back(input);
    }

    updateInputPositions();
}

void VideoLayoutProcessor::removeInput(int input)
{
    std::vector<int>::iterator it = std::find(m_inputPositions.begin(), m_inputPositions.end(), input);
    if (it != m_inputPositions.end()) {
        m_inputPositions.erase(it);
        updateInputPositions();
    }
}

void VideoLayoutProcessor::promoteInput(int input, size_t magnitude)
{
    std::vector<int>::iterator it = std::find(m_inputPositions.begin(), m_inputPositions.end(), input);
    if (it != m_inputPositions.end()) {
        m_inputPositions.erase(it);

        std::vector<int>::iterator newIt = m_inputPositions.begin();
        if (m_inputPositions.begin() + magnitude < it) {
            newIt = it - magnitude;
        }
        m_inputPositions.insert(newIt, input);
        updateInputPositions();
    }
}

void VideoLayoutProcessor::promoteInputs(std::vector<int>& inputs)
{
    // FIXME: We just promote the highest input in inputs to the head of m_inputPositions.
    // this is a temporary strategy due to the current layout template having only one main sub-image.
    if (inputs.size() > 0) {
        int highestInput = inputs.front();
        if (!m_inputPositions.empty()) {
            int old = m_inputPositions.front();
            if (highestInput != old) {
                std::vector<int>::iterator found = std::find(m_inputPositions.begin(), m_inputPositions.end(), highestInput);
                if (found != m_inputPositions.end()) {
                    // m_inputPositions.insert(found, old);
                    // std::vector<int>::iterator found = std::find(m_inputPositions.begin(), m_inputPositions.end(), highestInput);
                    // m_inputPositions.erase(found);
                    *found = old;
                    m_inputPositions.erase(m_inputPositions.begin());
                }
                m_inputPositions.insert(m_inputPositions.begin(), highestInput);
                updateInputPositions();
            }
        } else {
            m_inputPositions.insert(m_inputPositions.begin(), highestInput);
            updateInputPositions();
        }
    }
}

void VideoLayoutProcessor::specifyInputRegion(int input, std::string& regionID)
{
    std::vector<int>::iterator itOldPosition = std::find(m_inputPositions.begin(), m_inputPositions.end(), input);
    std::vector<int>::iterator itNewPosition = m_inputPositions.begin();

    std::vector<Region>::iterator itRegion = m_currentRegions->begin();
    for (; itRegion != m_currentRegions->end(); ++itRegion) {
        if (itNewPosition != m_inputPositions.end())
            itNewPosition++;
        if (itRegion->id == regionID)
            break;
    }

    if (itOldPosition != m_inputPositions.end() && itRegion != m_currentRegions->end()) {
        m_inputPositions.insert(itNewPosition, input);
        m_inputPositions.erase(itOldPosition);
        updateInputPositions();
    }
}

void VideoLayoutProcessor::updateInputPositions()
{
    if (!m_currentRegions) {
        ELOG_ERROR("no proper template.");
        return;
    }

    if (m_currentRegions->size() < 1) {
        ELOG_ERROR("current template is empty.");
        return;
    }

    chooseRegions();

    LayoutSolution solution;
    std::vector<Region>::iterator itReg = m_currentRegions->begin();
    for (std::vector<int>::iterator it = m_inputPositions.begin(); it != m_inputPositions.end(); ++it)
        solution[*it] = *itReg++;

    std::list<boost::shared_ptr<LayoutConsumer>>::iterator it = m_consumers.begin();
    for (; it != m_consumers.end(); ++it)
        (*it)->updateLayoutSolution(solution);
}

void VideoLayoutProcessor::chooseRegions()
{
    size_t inputCount = m_inputPositions.size();

    std::map<size_t, std::vector<Region>>::reverse_iterator lastCover, it = m_templates.rbegin();
    lastCover = it;
    for (; it != m_templates.rend(); ++it) {
        if (inputCount > it->first)
            break;
        else
            lastCover = it;
    }
    m_currentRegions = &(lastCover->second);
}

}

