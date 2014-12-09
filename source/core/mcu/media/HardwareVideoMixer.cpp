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

#include "HardwareVideoMixer.h"

#include "Config.h"

namespace mcu {

CodecType Frameformat2CodecType(FrameFormat format)
{
    switch (format) {
        case FRAME_FORMAT_H264:
            return ::CODEC_TYPE_VIDEO_AVC;
        case FRAME_FORMAT_VP8:
            return ::CODEC_TYPE_VIDEO_VP8;
        default:
            return ::CODEC_TYPE_INVALID;
    }
}

HardwareVideoMixerInput::HardwareVideoMixerInput(int slot, boost::shared_ptr<VideoFrameCompositor> compositor)
    : m_index(INVALID_INPUT_INDEX)
    , m_slot(slot)
    , m_provider(nullptr)
    , m_compositor(compositor)
{
    // FIXME: The dynamic cast is ugly... Get rid of it asap!
    m_engine = boost::dynamic_pointer_cast<HardwareVideoMixer>(compositor)->engine;
    m_compositor->activateInput(slot);
}

HardwareVideoMixerInput::~HardwareVideoMixerInput()
{
    unsetInput();
    m_compositor->deActivateInput(m_slot);
}

bool HardwareVideoMixerInput::setInput(FrameFormat inFormat, VideoFrameProvider* provider)
{
    if (m_index == INVALID_INPUT_INDEX) {
        assert((inFormat == FRAME_FORMAT_VP8 || inFormat == FRAME_FORMAT_H264) && provider);
        m_provider = provider;
        m_index = m_engine->enableInput(Frameformat2CodecType(inFormat), this);
        return true;
    }

    return false;
}

void HardwareVideoMixerInput::unsetInput()
{
    if (m_index != INVALID_INPUT_INDEX && m_engine.get()) {
        m_engine->disableInput(m_index);
        m_index = INVALID_INPUT_INDEX;
    }
    m_provider = nullptr;
}

void HardwareVideoMixerInput::onFrame(FrameFormat, unsigned char* payload, int len, unsigned int ts)
{
    m_engine->pushInput(m_index, payload, len);
}

void HardwareVideoMixerInput::requestKeyFrame(InputIndex index) {
    if (index == m_index)
        m_provider->requestKeyFrame();
}

HardwareVideoMixerOutput::HardwareVideoMixerOutput(boost::shared_ptr<VideoMixEngine> engine,
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

HardwareVideoMixerOutput::~HardwareVideoMixerOutput()
{
    if(m_index != INVALID_OUTPUT_INDEX && m_engine.get()) {
        m_engine->disableOutput(m_index);
        m_index = INVALID_OUTPUT_INDEX;
    }
}

void HardwareVideoMixerOutput::setBitrate(unsigned short bitrate)
{
    m_engine->setBitrate(m_index, bitrate);
}

void HardwareVideoMixerOutput::requestKeyFrame()
{
    m_engine->forceKeyFrame(m_index);
}

void HardwareVideoMixerOutput::onTimeout()
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

void HardwareVideoMixerOutput::notifyFrameReady(OutputIndex index)
{
    if (index == m_index) {
        // TODO: Pulling stratege (onTigger) is used now, but if mix-engine support notify mechanism, we
        // can receive output frame by this method.
    }
}

DEFINE_LOGGER(HardwareVideoMixer, "mcu.media.HardwareVideoMixer");

HardwareVideoMixer::HardwareVideoMixer(const VideoLayout& layout)
{
    engine.reset(new VideoMixEngine());

    // Fetch video size and background color.
    VideoSize rootSize = VideoLayoutHelper::getVideoSize(layout.rootSize);
    YUVColor rootColor = VideoLayoutHelper::getVideoBackgroundColor(layout.rootColor);
    BgColor bg = {rootColor.y, rootColor.cb, rootColor.cr};
    bool result = engine->init(bg, rootSize.width, rootSize.height);
    assert(result);
    if (!result) {
        ELOG_ERROR("Init video mixing engine failed!");
    }

    setLayout(layout);
}

HardwareVideoMixer::~HardwareVideoMixer()
{
}

void HardwareVideoMixer::setLayout(const VideoLayout& layout)
{
    // Initialize the layout mapping with custom video layout
    if (!layout.regions.empty()) {
        // Clear the current video layout
        m_currentLayout.layoutMapping.clear();
        m_currentLayout.candidateRegions.clear();

        // Set the layout information to hardware engine
        std::vector<Region>::const_iterator regionIt = layout.regions.begin();
        for (std::set<int>::iterator it=m_inputs.begin(); it!=m_inputs.end(); ++it) {
            if (regionIt != layout.regions.end()) {
                RegionInfo regionInfo;
                regionInfo.id = (*regionIt).id;
                regionInfo.left = (*regionIt).left;
                regionInfo.top = (*regionIt).top;
                regionInfo.relativeSize = (*regionIt).relativeSize;
                regionInfo.priority = (*regionIt).priority;

                // TODO: Currently, map the input to region sequentially.
                // Some enhancement like VAD will change this logic in the future.
                m_currentLayout.layoutMapping[*it] = regionInfo;

                ++regionIt;
            } else {
                ELOG_WARN("HardwareVideoMixer::setLayout failed. Not enough regions defined.");
            }
        }

        while (regionIt != layout.regions.end()) {
            RegionInfo regionInfo;
            regionInfo.id = (*regionIt).id;
            regionInfo.left = (*regionIt).left;
            regionInfo.top = (*regionIt).top;
            regionInfo.relativeSize = (*regionIt).relativeSize;
            regionInfo.priority = (*regionIt).priority;
            m_currentLayout.candidateRegions.push_back(regionInfo);

            ++regionIt;
        }

        engine->setLayout(m_currentLayout);
    }
}

bool HardwareVideoMixer::activateInput(int slot)
{
    if (m_inputs.find(slot) != m_inputs.end()) {
        ELOG_WARN("activateInput failed, slot is in use.");
        return false;
    }

    m_inputs.insert(slot);
    ELOG_DEBUG("activateInput OK, slot: %d", slot);

    // Adjust the mapping of input and layout region
    if (!onSlotNumberChanged(m_inputs.size()) && !m_currentLayout.candidateRegions.empty()) {
        // Pop up an existing one from candidateRegions
        RegionInfo regionInfo = m_currentLayout.candidateRegions.back();
        m_currentLayout.candidateRegions.pop_back();

        // Add a new mapping
        m_currentLayout.layoutMapping[slot] = regionInfo;

        engine->setLayout(m_currentLayout);
    }

    return true;
}

void HardwareVideoMixer::deActivateInput(int slot)
{
    m_inputs.erase(slot);

    // Adjust the mapping of input and layout region
    if (!onSlotNumberChanged(m_inputs.size())) {
        std::map<InputIndex, RegionInfo>::iterator it = m_currentLayout.layoutMapping.find(slot);
        if (it != m_currentLayout.layoutMapping.end()) {
            // Add it to candidateRegions
            m_currentLayout.candidateRegions.push_back(it->second);

            // Remove the existing one from mapping
            m_currentLayout.layoutMapping.erase(it);

            engine->setLayout(m_currentLayout);
        }
    }
}

bool HardwareVideoMixer::onSlotNumberChanged(uint32_t newSlotNum)
{
    // Update the video layout according to the new input number
    if (Config::get()->updateVideoLayout(newSlotNum)) {
        ELOG_DEBUG("Video layout updated with new slot number changed to %d", newSlotNum);
        return true;
    }

    return false;
}

// For current hardware mixer, the composition is hidden
// inside the hardware mix engine, and there won't be invocation
// to the below compositor interfaces.

void HardwareVideoMixer::pushInput(int slot, webrtc::I420VideoFrame*)
{
    assert(false);
}

bool HardwareVideoMixer::setOutput(VideoFrameConsumer* encoder)
{
    assert(false);
    return false;
}

void HardwareVideoMixer::unsetOutput()
{
    assert(false);
}

void HardwareVideoMixer::onFrame(FrameFormat, unsigned char* payload, int len, unsigned int ts)
{
    // We now don't support push mode encoder.
    assert(false);
}

void HardwareVideoMixer::setBitrate(int id, unsigned short bitrate)
{
    std::map<int, boost::shared_ptr<HardwareVideoMixerOutput>>::iterator it = m_outputs.find(id);
    if (it != m_outputs.end())
        it->second->setBitrate(bitrate);
}

void HardwareVideoMixer::requestKeyFrame(int id)
{
    std::map<int, boost::shared_ptr<HardwareVideoMixerOutput>>::iterator it = m_outputs.find(id);
    if (it != m_outputs.end())
        it->second->requestKeyFrame();
}

bool HardwareVideoMixer::activateOutput(int id, FrameFormat format, unsigned int framerate, unsigned short bitrate, VideoFrameConsumer* receiver)
{
    if (m_outputs.find(id) != m_outputs.end()) {
        ELOG_WARN("activateOutput failed, format is in use.");
        return false;
    }
    ELOG_DEBUG("activateOutput OK, format: %s", ((format == FRAME_FORMAT_VP8)? "VP8" : "H264"));
    m_outputs[id].reset(new HardwareVideoMixerOutput(engine, format, framerate, bitrate, receiver));
    return true;
}

void HardwareVideoMixer::deActivateOutput(int id)
{
    m_outputs.erase(id);
}

}
