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

VideoMixCodecType Frameformat2CodecType(woogeen_base::FrameFormat format)
{
    switch (format) {
        case woogeen_base::FRAME_FORMAT_H264:
            return VCT_MIX_H264;
        case woogeen_base::FRAME_FORMAT_VP8:
            return VCT_MIX_VP8;
        default:
            return VCT_MIX_UNKNOWN;
    }
}

HardwareVideoFrameMixerInput::HardwareVideoFrameMixerInput(boost::shared_ptr<VideoMixEngine> engine,
                                                           woogeen_base::FrameFormat inFormat,
                                                           woogeen_base::VideoFrameProvider* provider)
    : m_index(INVALID_INPUT_INDEX)
    , m_provider(provider)
    , m_engine(engine)
{
    assert((inFormat == woogeen_base::FRAME_FORMAT_VP8 || inFormat == woogeen_base::FRAME_FORMAT_H264) && m_provider);

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
                                                             woogeen_base::FrameFormat outFormat,
                                                             unsigned int framerate,
                                                             unsigned short bitrate,
                                                             woogeen_base::VideoFrameConsumer* receiver)
    : m_outFormat(outFormat)
    , m_index(INVALID_OUTPUT_INDEX)
    , m_engine(engine)
    , m_receiver(receiver)
    , m_frameRate(framerate)
    , m_outCount(0)
{
    assert((m_outFormat == woogeen_base::FRAME_FORMAT_VP8 || m_outFormat == woogeen_base::FRAME_FORMAT_H264) && m_receiver);

    m_index = m_engine->enableOutput(Frameformat2CodecType(m_outFormat), bitrate, this);
    assert(m_index != INVALID_OUTPUT_INDEX);
    m_jobTimer.reset(new woogeen_base::JobTimer(m_frameRate, this));
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

            woogeen_base::Frame frame;
            memset(&frame, 0, sizeof(frame));
            frame.format = m_outFormat;
            frame.payload = reinterpret_cast<uint8_t*>(&m_esBuffer);
            frame.length = len;
            frame.timeStamp = m_outCount * (1000 / m_frameRate) * 90;

            m_receiver->onFrame(frame);
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

HardwareVideoFrameMixer::HardwareVideoFrameMixer(VideoSize rootSize, YUVColor bgColor)
{
    m_engine.reset(new VideoMixEngine());

    // Fetch video size and background color.
    BackgroundColor backgroundColor = {bgColor.y, bgColor.cb, bgColor.cr};
    FrameSize frameSize = {rootSize.width, rootSize.height};
    bool result = m_engine->init(backgroundColor, frameSize);
    assert(result);
    if (!result) {
        ELOG_ERROR("Init video mixing engine failed!");
    }
}

HardwareVideoFrameMixer::~HardwareVideoFrameMixer()
{
}

void HardwareVideoFrameMixer::updateRootSize(VideoSize& rootSize)
{
    m_engine->setResolution(rootSize.width, rootSize.height);
}

void HardwareVideoFrameMixer::updateBackgroundColor(YUVColor& bgColor)
{
    BackgroundColor backgroundColor = {bgColor.y, bgColor.cb, bgColor.cr};
    m_engine->setBackgroundColor(&backgroundColor);
}

void HardwareVideoFrameMixer::updateLayoutSolution(LayoutSolution& solution)
{
    CustomLayoutInfo layout;
    for (LayoutSolution::iterator it = solution.begin(); it != solution.end(); ++it) {
        boost::shared_lock<boost::shared_mutex> lock(m_inputMutex);
        std::map<int, boost::shared_ptr<HardwareVideoFrameMixerInput>>::iterator it2 = m_inputs.find(it->input);
        if (it2 != m_inputs.end()) {
            RegionInfo region = {it->region.left, it->region.top, it->region.relativeSize, it->region.relativeSize};
            layout[it2->second->index()] = region;
        }
    }
    m_engine->setLayout(&layout);
}

bool HardwareVideoFrameMixer::activateInput(int input, woogeen_base::FrameFormat format, woogeen_base::VideoFrameProvider* provider)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_inputMutex);
    if (m_inputs.find(input) != m_inputs.end()) {
        ELOG_WARN("activateInput failed, input is in use.");
        return false;
    }

    HardwareVideoFrameMixerInput* newInput = new HardwareVideoFrameMixerInput(m_engine, format, provider);

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_inputs[input].reset(newInput);
    ELOG_DEBUG("activateInput OK, input: %d", input);
    return true;
}

void HardwareVideoFrameMixer::deActivateInput(int input)
{
    boost::unique_lock<boost::shared_mutex> lock(m_inputMutex);
    m_inputs.erase(input);
}

void HardwareVideoFrameMixer::pushInput(int input, unsigned char* payload, int len)
{
    boost::shared_lock<boost::shared_mutex> lock(m_inputMutex);
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerInput>>::iterator it = m_inputs.find(input);
    if (it != m_inputs.end())
        it->second->push(payload, len);
}

void HardwareVideoFrameMixer::setBitrate(unsigned short kbps, int id)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerOutput>>::iterator it = m_outputs.find(id);
    if (it != m_outputs.end())
        it->second->setBitrate(kbps);
}

void HardwareVideoFrameMixer::requestKeyFrame(int id)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerOutput>>::iterator it = m_outputs.find(id);
    if (it != m_outputs.end())
        it->second->requestKeyFrame();
}

bool HardwareVideoFrameMixer::activateOutput(int id, woogeen_base::FrameFormat format, unsigned int framerate, unsigned short kbps, woogeen_base::VideoFrameConsumer* receiver)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    if (m_outputs.find(id) != m_outputs.end()) {
        ELOG_WARN("activateOutput failed, format is in use.");
        return false;
    }
    ELOG_DEBUG("activateOutput OK, format: %s", ((format == woogeen_base::FRAME_FORMAT_VP8)? "VP8" : "H264"));
    HardwareVideoFrameMixerOutput* newOutput = new HardwareVideoFrameMixerOutput(m_engine, format, framerate, kbps, receiver);

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_outputs[id].reset(newOutput);
    return true;
}

void HardwareVideoFrameMixer::deActivateOutput(int id)
{
    boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
    m_outputs.erase(id);
}

}
