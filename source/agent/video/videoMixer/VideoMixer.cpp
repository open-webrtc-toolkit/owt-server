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

#include <webrtc/base/logging.h>
#include <webrtc/system_wrappers/include/trace.h>

#include "VideoMixer.h"
#include "VideoFrameMixerImpl.h"
#include "VideoFrameMixer.h"

using namespace webrtc;
using namespace woogeen_base;

namespace mcu {

DEFINE_LOGGER(VideoMixer, "mcu.media.VideoMixer");

VideoMixer::VideoMixer(const VideoMixerConfig& config)
    : m_nextOutputIndex(0)
    , m_maxInputCount(16)
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

    m_maxInputCount = config.maxInput;

    VideoSize rootSize;
    if (!VideoResolutionHelper::getVideoSize(config.resolution, rootSize)) {
        ELOG_WARN("configured resolution is invalid!");
        rootSize = DEFAULT_VIDEO_SIZE;
    }

    YUVColor bgColor;
    if (!VideoColorHelper::getVideoColor(config.bgColor.r, config.bgColor.g, config.bgColor.b, bgColor)) {
        ELOG_WARN("configured background color is invalid!");
        bgColor = DEFAULT_VIDEO_BG_COLOR;
    }

#ifdef ENABLE_MSDK
    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase != NULL) {
        msdkBase->setConfigHevcEncoderGaccPlugin(config.useGacc);
        msdkBase->setConfigMFETimeout(config.MFE_timeout);
    }
#endif

    ELOG_INFO("Init maxInput(%u), rootSize(%u, %u), bgColor(%u, %u, %u)", m_maxInputCount, rootSize.width, rootSize.height, bgColor.y, bgColor.cb, bgColor.cr);

    m_frameMixer.reset(new VideoFrameMixerImpl(m_maxInputCount, rootSize, bgColor, true, config.crop));
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

bool VideoMixer::addOutput(
    const std::string& outStreamID
    , const std::string& codec
    , const woogeen_base::VideoCodecProfile profile
    , const std::string& resolution
    , const unsigned int framerateFPS
    , const unsigned int bitrateKbps
    , const unsigned int keyFrameIntervalSeconds
    , woogeen_base::FrameDestination* dest)
{
    woogeen_base::FrameFormat format = getFormat(codec);
    VideoSize vSize;
    VideoResolutionHelper::getVideoSize(resolution, vSize);

    if (m_frameMixer->addOutput(m_nextOutputIndex, format, profile, vSize, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest)) {
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

void VideoMixer::forceKeyFrame(const std::string& outStreamID)
{
    int32_t index = -1;
    boost::shared_lock<boost::shared_mutex> lock(m_outputsMutex);
    auto it = m_outputs.find(outStreamID);
    if (it != m_outputs.end()) {
        index = it->second;
    }
    lock.unlock();

    if (index != -1) {
        m_frameMixer->requestKeyFrame(index);
    }
}

void VideoMixer::updateLayoutSolution(LayoutSolution& solution) {
    ELOG_DEBUG("updateLayoutSolution, size(%ld)", solution.size());

    for (auto& l : solution) {
        Region *pRegion = &l.region;

        ELOG_DEBUG("input(%d): shape(%s), left(%d/%d), top(%d/%d), width(%d/%d), height(%d/%d)"
                , l.input
                , pRegion->shape.c_str()
                , pRegion->area.rect.left.numerator, pRegion->area.rect.left.denominator
                , pRegion->area.rect.top.numerator, pRegion->area.rect.top.denominator
                , pRegion->area.rect.width.numerator, pRegion->area.rect.width.denominator
                , pRegion->area.rect.height.numerator, pRegion->area.rect.height.denominator);

        assert(pRegion->shape.compare("rectangle") == 0);
        assert(pRegion->area.rect.left.denominator != 0 && pRegion->area.rect.left.denominator >= pRegion->area.rect.left.numerator);
        assert(pRegion->area.rect.top.denominator != 0 && pRegion->area.rect.top.denominator >= pRegion->area.rect.top.numerator);
        assert(pRegion->area.rect.width.denominator != 0 && pRegion->area.rect.width.denominator >= pRegion->area.rect.width.numerator);
        assert(pRegion->area.rect.height.denominator != 0 && pRegion->area.rect.height.denominator >= pRegion->area.rect.height.numerator);

        ELOG_TRACE("input(%d): left(%.2f), top(%.2f), width(%.2f), height(%.2f)"
                , l.input
                , (float)pRegion->area.rect.left.numerator / pRegion->area.rect.left.denominator
                , (float)pRegion->area.rect.top.numerator / pRegion->area.rect.top.denominator
                , (float)pRegion->area.rect.width.numerator / pRegion->area.rect.width.denominator
                , (float)pRegion->area.rect.height.numerator / pRegion->area.rect.height.denominator);
    }

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
