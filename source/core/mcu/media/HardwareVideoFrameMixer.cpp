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

#include "HardwareVideoFrameMixer.h"

namespace mcu {

VideoMixCodecType Frameformat2CodecType(FrameFormat format)
{
    switch (format) {
        case FRAME_FORMAT_H264:
            return VCT_MIX_H264;
        case FRAME_FORMAT_VP8:
            return VCT_MIX_VP8;
        default:
            return VCT_MIX_UNKNOWN;
    }
}

HardwareVideoFrameMixerInput::HardwareVideoFrameMixerInput(boost::shared_ptr<VideoMixEngine> engine,
                                                 FrameFormat inFormat,
                                                 VideoFrameProvider* provider)
    : m_index(INVALID_INPUT_INDEX)
    , m_provider(provider)
    , m_engine(engine)
{
    assert((inFormat == FRAME_FORMAT_VP8 || inFormat == FRAME_FORMAT_H264) && m_provider);

    m_index = m_engine->enableInput(Frameformat2CodecType(inFormat), this);
}

HardwareVideoFrameMixerInput::~HardwareVideoFrameMixerInput()
{
    if (m_index != INVALID_INPUT_INDEX && m_engine.get()) {
        m_engine->disableInput(m_index);
        m_index = INVALID_INPUT_INDEX;
    }
    m_provider = nullptr;
}

void HardwareVideoFrameMixerInput::push(unsigned char* payload, int len)
{
    m_engine->pushInput(m_index, payload, len);
}

void HardwareVideoFrameMixerInput::requestKeyFrame(InputIndex index) {
    if (index == m_index)
        m_provider->requestKeyFrame();
}

HardwareVideoFrameMixerOutput::HardwareVideoFrameMixerOutput(boost::shared_ptr<VideoMixEngine> engine,
                                                    FrameFormat outFormat,
                                                    unsigned int framerate,
                                                    unsigned short bitrate,
                                                    VideoFrameConsumer* receiver)
    : m_outFormat(outFormat)
    , m_index(INVALID_OUTPUT_INDEX)
    , m_engine(engine)
    , m_receiver(receiver)
    , m_frameRate(framerate)
    , m_outCount(0)
{
    assert((m_outFormat == FRAME_FORMAT_VP8 || m_outFormat == FRAME_FORMAT_H264) && m_receiver);

    m_index = m_engine->enableOutput(Frameformat2CodecType(m_outFormat), bitrate, this);
    assert(m_index != INVALID_OUTPUT_INDEX);
    m_jobTimer.reset(new JobTimer(m_frameRate, this));
}

HardwareVideoFrameMixerOutput::~HardwareVideoFrameMixerOutput()
{
    if(m_index != INVALID_OUTPUT_INDEX && m_engine.get()) {
        m_engine->disableOutput(m_index);
        m_index = INVALID_OUTPUT_INDEX;
    }
}

void HardwareVideoFrameMixerOutput::setBitrate(unsigned short bitrate)
{
    m_engine->setBitrate(m_index, bitrate);
}

void HardwareVideoFrameMixerOutput::requestKeyFrame()
{
    m_engine->forceKeyFrame(m_index);
}

void HardwareVideoFrameMixerOutput::onTimeout()
{   
    do {
        int len = m_engine->pullOutput(m_index, (unsigned char*)&m_esBuffer);
        if (len > 0) {
            m_outCount++;
            m_receiver->onFrame(m_outFormat, (unsigned char *)&m_esBuffer, len, m_outCount * m_frameRate);
        } else
            break;
    } while (1);
}

void HardwareVideoFrameMixerOutput::notifyFrameReady(OutputIndex index)
{
    if (index == m_index) {
        // TODO: Pulling stratege (onTigger) is used now, but if mix-engine support notify mechanism, we
        // can receive output frame by this method.
    }
}

DEFINE_LOGGER(HardwareVideoFrameMixer, "mcu.media.HardwareVideoFrameMixer");

HardwareVideoFrameMixer::HardwareVideoFrameMixer(const std::string& layoutType, const std::string& defaultRootSize, const std::string& backgroundColor, const std::string& customLayout)
{
    m_engine.reset(new VideoMixEngine());

    m_configListenerId = Config::get()->registerListener(this);
    Config::get()->initVideoLayout(m_configListenerId, layoutType, defaultRootSize, backgroundColor, customLayout);

    const VideoLayout& layout = Config::get()->getVideoLayout(m_configListenerId);

    // Fetch video size and background color.
    VideoSize rootSize = VideoLayoutHelper::getVideoSize(layout.rootSize);
    YUVColor rootColor = VideoLayoutHelper::getVideoYUVColor(layout.rootColor);
    BackgroundColor bgColor = {rootColor.y, rootColor.cb, rootColor.cr};
    FrameSize frameSize = {rootSize.width, rootSize.height};
    bool result = m_engine->init(bgColor, frameSize);
    assert(result);
    if (!result) {
        ELOG_ERROR("Init video mixing engine failed!");
    }

    setLayout(layout);
}

HardwareVideoFrameMixer::~HardwareVideoFrameMixer()
{
    Config::get()->unregisterListener(m_configListenerId);
}

void HardwareVideoFrameMixer::setLayout(const VideoLayout& layout)
{
    // Initialize the layout mapping with custom video layout
    if (!layout.regions.empty()) {
        // Set the layout video size and background color
        VideoSize rootSize = VideoLayoutHelper::getVideoSize(layout.rootSize);
        YUVColor rootColor = VideoLayoutHelper::getVideoYUVColor(layout.rootColor);
        m_currentLayout.rootSize = {rootSize.width, rootSize.height};
        m_currentLayout.rootColor = {rootColor.y, rootColor.cb, rootColor.cr};

        // Clear the current video layout
        m_currentLayout.layoutMapping.clear();
        m_currentLayout.candidateRegions.clear();

        // Set the layout information to hardware engine
        std::vector<Region>::const_iterator regionIt = layout.regions.begin();
        for (std::map<int, boost::shared_ptr<HardwareVideoFrameMixerInput>>::iterator it=m_inputs.begin(); it!=m_inputs.end(); ++it) {
            if (regionIt != layout.regions.end()) {
                RegionInfo regionInfo = {regionIt->left, regionIt->top,
                    regionIt->relativeSize, regionIt->relativeSize};

                // TODO: Currently, map the input to region sequentially.
                // Some enhancement like VAD will change this logic in the future.
                InputIndex index = it->second->index();
                if (index != INVALID_INPUT_INDEX)
                    m_currentLayout.layoutMapping[index] = regionInfo;
                ++regionIt;
            } else {
                ELOG_WARN("HardwareVideoFrameMixer::setLayout failed. Not enough regions defined.");
            }
        }

        while (regionIt != layout.regions.end()) {
            RegionInfo regionInfo = {regionIt->left, regionIt->top,
                regionIt->relativeSize, regionIt->relativeSize};
            m_currentLayout.candidateRegions.push_back(regionInfo);

            ++regionIt;
        }

        m_engine->setLayout(&m_currentLayout);
    }
}

bool HardwareVideoFrameMixer::activateInput(int slot, FrameFormat format, VideoFrameProvider* provider)
{
    if (m_inputs.find(slot) != m_inputs.end()) {
        ELOG_WARN("activateInput failed, slot is in use.");
        return false;
    }

    m_inputs[slot].reset(new HardwareVideoFrameMixerInput(m_engine, format, provider));
    ELOG_DEBUG("activateInput OK, slot: %d", slot);

    // Adjust the mapping of input and layout region
    if (!onSlotNumberChanged(m_inputs.size()) && !m_currentLayout.candidateRegions.empty()) {
        // Pop up an existing one from candidateRegions
        RegionInfo regionInfo = m_currentLayout.candidateRegions.back();
        m_currentLayout.candidateRegions.pop_back();

        // Add a new mapping item
        InputIndex index = m_inputs[slot]->index();
        if (index != INVALID_INPUT_INDEX)
            m_currentLayout.layoutMapping[index] = regionInfo;

        m_engine->setLayout(&m_currentLayout);
    }

    return true;
}

void HardwareVideoFrameMixer::deActivateInput(int slot)
{
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerInput>>::iterator inputIt = m_inputs.find(slot);
    if (inputIt == m_inputs.end())
        return;

    // Get the index first
    InputIndex index = inputIt->second->index();

    // Remove the input entry
    m_inputs.erase(slot);

    // Adjust the mapping of input and layout region
    if (!onSlotNumberChanged(m_inputs.size())) {
        std::map<InputIndex, RegionInfo>::iterator it = m_currentLayout.layoutMapping.find(index);
        if (it != m_currentLayout.layoutMapping.end()) {
            // Add the spared region to candidateRegions
            std::map<InputIndex, RegionInfo>::reverse_iterator rit = m_currentLayout.layoutMapping.rbegin();
            m_currentLayout.candidateRegions.push_back(rit->second);

            // Move the following regions forward
            while (rit != m_currentLayout.layoutMapping.rend()) {
                if (rit->first == it->first)
                    break;

                rit->second = (++rit)->second;
            }

            // Remove the existing item from mapping
            m_currentLayout.layoutMapping.erase(index);

            // Set the new layout with updated mapping items
            m_engine->setLayout(&m_currentLayout);
        }
    }
}

void HardwareVideoFrameMixer::pushInput(int slot, unsigned char* payload, int len)
{
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerInput>>::iterator it = m_inputs.find(slot);
    if (it != m_inputs.end())
        it->second->push(payload, len);
}

void HardwareVideoFrameMixer::setBitrate(int id, unsigned short bitrate)
{
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerOutput>>::iterator it = m_outputs.find(id);
    if (it != m_outputs.end())
        it->second->setBitrate(bitrate);
}

void HardwareVideoFrameMixer::requestKeyFrame(int id)
{
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerOutput>>::iterator it = m_outputs.find(id);
    if (it != m_outputs.end())
        it->second->requestKeyFrame();
}

bool HardwareVideoFrameMixer::activateOutput(int id, FrameFormat format, unsigned int framerate, unsigned short bitrate, VideoFrameConsumer* receiver)
{
    if (m_outputs.find(id) != m_outputs.end()) {
        ELOG_WARN("activateOutput failed, format is in use.");
        return false;
    }
    ELOG_DEBUG("activateOutput OK, format: %s", ((format == FRAME_FORMAT_VP8)? "VP8" : "H264"));
    m_outputs[id].reset(new HardwareVideoFrameMixerOutput(m_engine, format, framerate, bitrate, receiver));
    return true;
}

void HardwareVideoFrameMixer::deActivateOutput(int id)
{
    m_outputs.erase(id);
}

void HardwareVideoFrameMixer::onConfigChanged()
{
    ELOG_DEBUG("onConfigChanged");
    setLayout(Config::get()->getVideoLayout(m_configListenerId));
}

bool HardwareVideoFrameMixer::onSlotNumberChanged(uint32_t newSlotNum)
{
    // Update the video layout according to the new input number
    if (Config::get()->updateVideoLayout(m_configListenerId, newSlotNum)) {
        ELOG_DEBUG("Video layout updated with new slot number changed to %d", newSlotNum);
        return true;
    }

    return false;
}

}
