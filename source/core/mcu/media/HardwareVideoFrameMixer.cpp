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

static int H264_STARTCODE_BYTES = 4;
/*static int H264_AUD_BYTES = 6;*/

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
            if (m_outFormat == FRAME_FORMAT_H264) {
                if ((len - H264_STARTCODE_BYTES) > 0) {
                    m_outCount++;
                    m_receiver->onFrame(m_outFormat, (unsigned char *)&m_esBuffer + H264_STARTCODE_BYTES, len - H264_STARTCODE_BYTES, m_outCount * (1000 / m_frameRate));
                } else
                    break;
            } else {
                m_outCount++;
                m_receiver->onFrame(m_outFormat, (unsigned char *)&m_esBuffer, len, m_outCount * (1000 / m_frameRate));
            }
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
        std::map<int, boost::shared_ptr<HardwareVideoFrameMixerInput>>::iterator it2 = m_inputs.find(it->input);
        if (it2 != m_inputs.end()) {
            RegionInfo region = {it->region.left, it->region.top, it->region.relativeSize, it->region.relativeSize};
            layout[it2->second->index()] = region;
        }
    }
    m_engine->setLayout(&layout);
}

bool HardwareVideoFrameMixer::activateInput(int input, FrameFormat format, VideoFrameProvider* provider)
{
    if (m_inputs.find(input) != m_inputs.end()) {
        ELOG_WARN("activateInput failed, input is in use.");
        return false;
    }

    m_inputs[input].reset(new HardwareVideoFrameMixerInput(m_engine, format, provider));
    ELOG_DEBUG("activateInput OK, input: %d", input);
    return true;
}

void HardwareVideoFrameMixer::deActivateInput(int input)
{
    m_inputs.erase(input);
}

void HardwareVideoFrameMixer::pushInput(int input, unsigned char* payload, int len)
{
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerInput>>::iterator it = m_inputs.find(input);
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

}
