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

#include "VideoMixer.h"

#include "VideoFrameMixerImpl.h"
#include "VideoLayoutProcessor.h"
#include "VideoFrameMixer.h"
#include <webrtc/system_wrappers/interface/trace.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace webrtc;
using namespace woogeen_base;

namespace mcu {

DEFINE_LOGGER(VideoMixer, "mcu.media.VideoMixer");

VideoMixer::VideoMixer(const std::string& configStr)
    : m_nextOutputIndex(0)
    , m_inputCount(0)
    , m_maxInputCount(0)
{
    boost::property_tree::ptree config;
    std::istringstream is(configStr);
    boost::property_tree::read_json(is, config);

    m_maxInputCount = config.get<uint32_t>("maxinput", 16);
    m_freeInputIndexes.reserve(m_maxInputCount);
    for (size_t i = 0; i < m_maxInputCount; ++i)
        m_freeInputIndexes.push_back(true);

    bool cropVideo = config.get<bool>("crop");

#ifdef ENABLE_MSDK
    bool useGacc = config.get<bool>("gaccplugin", false);
    bool enableBgColorSurface = config.get<bool>("enableMsdkBackgroundColorSurface", false);

    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase != NULL) {
        msdkBase->setConfigHevcEncoderGaccPlugin(useGacc);
        msdkBase->setConfigEnableBackgroundColorSurface(enableBgColorSurface);
    }
#endif

    m_layoutProcessor.reset(new VideoLayoutProcessor(config));
    VideoSize rootSize;
    m_layoutProcessor->getRootSize(rootSize);
    YUVColor bgColor;
    m_layoutProcessor->getBgColor(bgColor);

    ELOG_DEBUG("Init maxInput(%u), rootSize(%u, %u), bgColor(%u, %u, %u)", m_maxInputCount, rootSize.width, rootSize.height, bgColor.y, bgColor.cb, bgColor.cr);

    m_taskRunner.reset(new woogeen_base::WebRTCTaskRunner());

    m_frameMixer.reset(new VideoFrameMixerImpl(m_maxInputCount, rootSize, bgColor, m_taskRunner, true, cropVideo));
    m_layoutProcessor->registerConsumer(m_frameMixer);

    m_taskRunner->Start();

    if (ELOG_IS_TRACE_ENABLED()) {
        webrtc::Trace::CreateTrace();
        webrtc::Trace::SetTraceFile("webrtc_trace_videoMixer.txt");
        webrtc::Trace::set_level_filter(webrtc::kTraceAll);
    }
}

VideoMixer::~VideoMixer()
{
    closeAll();

    m_taskRunner->Stop();
    m_layoutProcessor->deregisterConsumer(m_frameMixer);

    if (ELOG_IS_TRACE_ENABLED()) {
        webrtc::Trace::ReturnTrace();
    }
}

bool VideoMixer::addInput(const std::string& inStreamID, const std::string& codec, woogeen_base::FrameSource* source, const std::string& avatar)
{
    if (m_inputCount == m_maxInputCount) {
        ELOG_WARN("Exceeding maximum number of sources (%u), ignoring the addSource request", m_maxInputCount);
        return false;
    }

    woogeen_base::FrameFormat format = getFormat(codec);

    boost::upgrade_lock<boost::shared_mutex> lock(m_inputsMutex);
    auto it = m_inputs.find(inStreamID);
    if (it == m_inputs.end() || !it->second) {
        int index = useAFreeInputIndex();
        ELOG_DEBUG("addInput - assigned input(%s) index: %d", inStreamID.c_str(), index);

        if (m_frameMixer->addInput(index, format, source, avatar)) {
            m_layoutProcessor->addInput(index);
            boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
            m_inputs[inStreamID] = index;
        }
        ++m_inputCount;
        return true;
    }

    ELOG_WARN("addInput for an existing inStreamID:%s", inStreamID.c_str());
    return false;
}

void VideoMixer::removeInput(const std::string& inStreamID)
{
    int index = -1;
    boost::unique_lock<boost::shared_mutex> lock(m_inputsMutex);
    auto it = m_inputs.find(inStreamID);
    if (it != m_inputs.end()) {
        index = it->second;
        m_inputs.erase(it);
    }
    lock.unlock();

    if (index >= 0) {
        ELOG_DEBUG("removeInput - recycle input(%s) index: %d", inStreamID.c_str(), index);
        m_frameMixer->removeInput(index);
        m_layoutProcessor->removeInput(index);
        m_freeInputIndexes[index] = true;
        --m_inputCount;
    }
}

void VideoMixer::setInputActive(const std::string& inStreamID, bool active)
{
    int index = -1;
    boost::unique_lock<boost::shared_mutex> lock(m_inputsMutex);
    auto it = m_inputs.find(inStreamID);
    if (it != m_inputs.end()) {
        index = it->second;
    }
    lock.unlock();

    if (index >= 0) {
        m_frameMixer->setInputActive(index, active);
    }
}

bool VideoMixer::setRegion(const std::string& inStreamID, const std::string& regionID)
{
    int index = -1;

    boost::shared_lock<boost::shared_mutex> lock(m_inputsMutex);
    auto it = m_inputs.find(inStreamID);
    if (it != m_inputs.end()) {
        index = it->second;
    }
    lock.unlock();

    if (index >= 0) {
        return m_layoutProcessor->specifyInputRegion(index, regionID);
    }

    ELOG_ERROR("specifySourceRegion - no such an input stream: %s", inStreamID.c_str());
    return false;
}

std::string VideoMixer::getRegion(const std::string& inStreamID)
{
    int index = -1;

    boost::shared_lock<boost::shared_mutex> lock(m_inputsMutex);
    auto it = m_inputs.find(inStreamID);
    if (it != m_inputs.end()) {
        index = it->second;
    }
    lock.unlock();

    if (index >= 0)
        return m_layoutProcessor->getInputRegion(index);

    ELOG_ERROR("getSourceRegion - no such an input stream: %s", inStreamID.c_str());
    return "";
}

void VideoMixer::setPrimary(const std::string& inStreamID)
{
    std::vector<int> inputs;

    boost::shared_lock<boost::shared_mutex> lock(m_inputsMutex);
    auto it = m_inputs.find(inStreamID);
    if (it != m_inputs.end()) {
        inputs.push_back(it->second);
    }
    lock.unlock();

    if (inputs.size() > 0)
        m_layoutProcessor->promoteInputs(inputs);
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

boost::shared_ptr<std::map<std::string, mcu::Region>> VideoMixer::getCurrentRegions()
{
    boost::shared_ptr<std::map<std::string, Region>> layoutMap(new std::map<std::string, Region>);

    for (auto it = m_inputs.begin(); it != m_inputs.end(); it++) {
        std::string streamID = it->first;
        int index = it->second;
        (*layoutMap)[streamID] = m_layoutProcessor->getRegionDetail(index);
    }

    return layoutMap;
}

int VideoMixer::useAFreeInputIndex()
{
    for (size_t i = 0; i < m_freeInputIndexes.size(); ++i) {
        if (m_freeInputIndexes[i]) {
            m_freeInputIndexes[i] = false;
            return i;
        }
    }

    return -1;
}

void VideoMixer::closeAll()
{
    ELOG_DEBUG("closeAll");
    {
        boost::upgrade_lock<boost::shared_mutex> inputLock(m_inputsMutex);
        auto it = m_inputs.begin();
        while (it != m_inputs.end()) {
            int index = it->second;
            m_frameMixer->removeInput(index);
            m_layoutProcessor->removeInput(index);
        }
        boost::upgrade_to_unique_lock<boost::shared_mutex> inputLock1(inputLock);
        m_inputs.clear();

        m_inputCount = 0;
    }

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
