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

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <webrtc/base/logging.h>
#include <webrtc/system_wrappers/include/trace.h>

#include "VideoMixer.h"
#include "VideoFrameMixerImpl.h"
#include "VideoFrameMixer.h"

using namespace webrtc;
using namespace woogeen_base;

namespace mcu {

DEFINE_LOGGER(VideoMixer, "mcu.media.VideoMixer");

VideoMixer::VideoMixer(const std::string& configStr)
    : m_nextOutputIndex(0)
    , m_maxInputCount(0)
{
    if (ELOG_IS_TRACE_ENABLED()) {
        rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
        rtc::LogMessage::LogTimestamps(true);

        webrtc::Trace::CreateTrace();
        webrtc::Trace::SetTraceFile(NULL, false);
        webrtc::Trace::set_level_filter(webrtc::kTraceAll);
    } else if (ELOG_IS_DEBUG_ENABLED()) {
        rtc::LogMessage::LogToDebug(rtc::LS_INFO);
        rtc::LogMessage::LogTimestamps(true);

        const int kTraceFilter = webrtc::kTraceNone | webrtc::kTraceTerseInfo |
            webrtc::kTraceWarning | webrtc::kTraceError |
            webrtc::kTraceCritical | webrtc::kTraceDebug |
            webrtc::kTraceInfo;

        webrtc::Trace::CreateTrace();
        webrtc::Trace::SetTraceFile(NULL, false);
        webrtc::Trace::set_level_filter(kTraceFilter);
    }

    boost::property_tree::ptree config;
    std::istringstream is(configStr);
    boost::property_tree::read_json(is, config);

    m_maxInputCount = config.get<uint32_t>("maxinput", 16);

    bool cropVideo = config.get<bool>("crop");

#ifdef ENABLE_MSDK
    bool useGacc = config.get<bool>("gaccplugin", false);

    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase != NULL) {
        msdkBase->setConfigHevcEncoderGaccPlugin(useGacc);
    }
#endif

    VideoSize rootSize;
    std::string resolution = config.get<std::string>("resolution");
    if (!VideoResolutionHelper::getVideoSize(resolution, rootSize)) {
        ELOG_WARN("configured resolution is invalid!");
        VideoResolutionHelper::getVideoSize("vga", rootSize);
    }

    YUVColor bgColor;
    std::string color = config.get<std::string>("backgroundcolor");
    if (!VideoColorHelper::getVideoColor(color, bgColor)) {
        // Try the RGB configuration mode
        boost::property_tree::ptree pt = config.get_child("backgroundcolor");
        int r = pt.get<int>("r", -1);
        int g = pt.get<int>("g", -1);
        int b = pt.get<int>("b", -1);

        if (!VideoColorHelper::getVideoColor(r, g, b, bgColor)) {
            ELOG_WARN("configured background color is invalid!");
            VideoColorHelper::getVideoColor("black", bgColor);
        }
    }

    ELOG_DEBUG("Init maxInput(%u), rootSize(%u, %u), bgColor(%u, %u, %u)", m_maxInputCount, rootSize.width, rootSize.height, bgColor.y, bgColor.cb, bgColor.cr);

    m_frameMixer.reset(new VideoFrameMixerImpl(m_maxInputCount, rootSize, bgColor, true, cropVideo));
}

VideoMixer::~VideoMixer()
{
    closeAll();
}

bool VideoMixer::addInput(const int inputIndex, const std::string& codec, woogeen_base::FrameSource* source, const std::string& avatar)
{
    if (m_inputs.find(inputIndex) != m_inputs.end()) {
        ELOG_WARN("addInput already exist:%d", inputIndex);
        return false;
    }
    if (m_inputs.size() >= m_maxInputCount) {
        ELOG_WARN("addInput reach max input");
        return false;
    }

    woogeen_base::FrameFormat format = getFormat(codec);

    if (m_frameMixer->addInput(inputIndex, format, source, avatar)) {
        m_inputs.insert(inputIndex);
        return true;
    }
    ELOG_WARN("addInput fail for inputIndex:%d", inputIndex);
    return false;
}

void VideoMixer::removeInput(const int inputIndex)
{
    if (m_inputs.find(inputIndex) == m_inputs.end()) {
        ELOG_WARN("removeInput no such input:%d", inputIndex);
        return;
    }

    m_inputs.erase(inputIndex);
    ELOG_DEBUG("removeInput - recycle input(%d)", inputIndex);
    m_frameMixer->removeInput(inputIndex);
}

void VideoMixer::setInputActive(const int inputIndex, bool active)
{
    if (m_inputs.find(inputIndex) != m_inputs.end()) {
        m_frameMixer->setInputActive(inputIndex, active);
    } else {
        ELOG_WARN("setInputActive no such input:%d", inputIndex);
    }
}

bool VideoMixer::addOutput(const std::string& outStreamID, const std::string& codec, const std::string& resolution, woogeen_base::QualityLevel qualityLevel, woogeen_base::FrameDestination* dest)
{
    woogeen_base::FrameFormat format = getFormat(codec);
    VideoSize vSize;
    VideoResolutionHelper::getVideoSize(resolution, vSize);
    if (m_frameMixer->addOutput(m_nextOutputIndex, format, vSize, qualityLevel, dest)) {
        boost::unique_lock<boost::shared_mutex> lock(m_outputsMutex);
        m_outputs[outStreamID] = m_nextOutputIndex++;
        return true;
    }
    return false;
}

void VideoMixer::removeOutput(const std::string& outStreamID)
{
    int32_t index = -1;
    boost::unique_lock<boost::shared_mutex> lock(m_outputsMutex);
    auto it = m_outputs.find(outStreamID);
    if (it != m_outputs.end()) {
        index = it->second;
        m_outputs.erase(it);
    }
    lock.unlock();

    if (index != -1) {
        m_frameMixer->removeOutput(index);
    }
}

void VideoMixer::updateLayoutSolution(LayoutSolution& solution) {
    m_frameMixer->updateLayoutSolution(solution);
}

void VideoMixer::closeAll()
{
    ELOG_DEBUG("closeAll");
    auto it = m_inputs.begin();
    while (it != m_inputs.end()) {
        m_frameMixer->removeInput(*it);
    }
    m_inputs.clear();

    {
        boost::upgrade_lock<boost::shared_mutex> outputLock(m_outputsMutex);
        auto it = m_outputs.begin();
        while (it != m_outputs.end()) {
            int32_t index = it->second;
            m_frameMixer->removeOutput(index);
        }
        boost::upgrade_to_unique_lock<boost::shared_mutex> outputLock1(outputLock);
        m_outputs.clear();
    }

    ELOG_DEBUG("Closed all media in this Mixer");
}


}/* namespace mcu */
