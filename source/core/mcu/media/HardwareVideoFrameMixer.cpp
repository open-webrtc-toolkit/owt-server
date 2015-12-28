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

#include "MediaUtilities.h"

namespace mcu {

using namespace woogeen_base;

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
                                                           woogeen_base::FrameSource* source)
    : m_index(INVALID_INPUT_INDEX)
    , m_source(source)
    , m_engine(engine)
{
    assert((inFormat == woogeen_base::FRAME_FORMAT_VP8 || inFormat == woogeen_base::FRAME_FORMAT_H264) && m_source);

    m_source->addVideoDestination(this);

    m_index = m_engine->enableInput(Frameformat2CodecType(inFormat), this);
}

HardwareVideoFrameMixerInput::~HardwareVideoFrameMixerInput()
{
    if (m_index != INVALID_INPUT_INDEX && m_engine.get()) {
        m_engine->disableInput(m_index);
        m_index = INVALID_INPUT_INDEX;
    }
    m_source = nullptr;
}

void HardwareVideoFrameMixerInput::onFrame(const woogeen_base::Frame& frame)
{
    m_engine->pushInput(m_index, frame.payload, frame.length);
}

void HardwareVideoFrameMixerInput::requestKeyFrame(InputIndex index) {
    if (index == m_index) {
        FeedbackMsg msg {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME};
        m_source->onFeedback(msg);
    }
}

HardwareVideoFrameMixerOutput::HardwareVideoFrameMixerOutput(boost::shared_ptr<VideoMixEngine> engine,
                                                             woogeen_base::FrameFormat outFormat,
                                                             FrameSize size,
                                                             unsigned int framerate,
                                                             unsigned short bitrate,
                                                             woogeen_base::FrameDestination* destination)
    : m_outFormat(outFormat)
    , m_index(INVALID_OUTPUT_INDEX)
    , m_engine(engine)
    , m_destination(destination)
    , m_frameRate(framerate)
    , m_outCount(0)
{
    assert((m_outFormat == woogeen_base::FRAME_FORMAT_VP8 || m_outFormat == woogeen_base::FRAME_FORMAT_H264) && m_destination);

    m_index = m_engine->enableOutput(Frameformat2CodecType(m_outFormat), bitrate, this, size);
    assert(m_index != INVALID_OUTPUT_INDEX);
    this->addVideoDestination(destination);
    m_jobTimer.reset(new woogeen_base::JobTimer(m_frameRate, this));
}

HardwareVideoFrameMixerOutput::~HardwareVideoFrameMixerOutput()
{
    if(m_index != INVALID_OUTPUT_INDEX && m_engine.get()) {
        m_engine->disableOutput(m_index);
        m_index = INVALID_OUTPUT_INDEX;
    }
    this->removeVideoDestination(m_destination);
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

            unsigned int width = 640;
            unsigned int height = 480;
            m_engine->getResolution(width, height);
            frame.additionalInfo.video.width = width;
            frame.additionalInfo.video.height = height;

            deliverFrame(frame);
        } else
            break;
    } while (1);
}

void HardwareVideoFrameMixerOutput::onFeedback(const woogeen_base::FeedbackMsg& msg)
{
    if (msg.type == woogeen_base::VIDEO_FEEDBACK) {
        if (msg.cmd == REQUEST_KEY_FRAME) {
            m_engine->forceKeyFrame(m_index);
        } else if (msg.cmd == SET_BITRATE) {
            m_engine->setBitrate(m_index, msg.data.kbps);
        }
    }
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

void HardwareVideoFrameMixer::updateLayoutSolution(LayoutSolution& solution)
{
    CustomLayoutInfo layout;
    for (LayoutSolution::iterator it = solution.begin(); it != solution.end(); ++it) {
        boost::shared_lock<boost::shared_mutex> lock(m_inputMutex);
        auto it2 = m_inputs.find(it->input);
        if (it2 != m_inputs.end()) {
            layout.push_back({it2->second->index(), {it->region.left, it->region.top, it->region.relativeSize, it->region.relativeSize}});
        }
    }
    m_engine->setLayout(&layout);
}

bool HardwareVideoFrameMixer::addInput(int input, woogeen_base::FrameFormat format, woogeen_base::FrameSource* source)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_inputMutex);
    if (m_inputs.find(input) != m_inputs.end()) {
        ELOG_WARN("activateInput failed, input is in use.");
        return false;
    }

    boost::shared_ptr<HardwareVideoFrameMixerInput> newInput(new HardwareVideoFrameMixerInput(m_engine, format, source));

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_inputs[input] = newInput;
    ELOG_DEBUG("activateInput OK, input: %d", input);
    return true;
}

void HardwareVideoFrameMixer::removeInput(int input)
{
    boost::unique_lock<boost::shared_mutex> lock(m_inputMutex);
    m_inputs.erase(input);
}

void HardwareVideoFrameMixer::setBitrate(unsigned short kbps, int output)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end()) {
        it->second->setBitrate(kbps);
    }
}

void HardwareVideoFrameMixer::requestKeyFrame(int output)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end()) {
        it->second->requestKeyFrame();
    }
}

bool HardwareVideoFrameMixer::addOutput(int output, woogeen_base::FrameFormat format, const VideoSize& rootSize, woogeen_base::FrameDestination* dest)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.begin();
    for (; it != m_outputs.end(); ++it) {
        if (it->second && it->second->destination() == dest) {
            ELOG_WARN("addOutput failed, destination exists.");
            return false;
        }
    }
    FrameSize size {rootSize.width, rootSize.height};
    unsigned short bitrate = woogeen_base::calcBitrate(rootSize.width, rootSize.height, 30) * (format == woogeen_base::FRAME_FORMAT_VP8 ? 0.9 : 1);
    ELOG_DEBUG("addOutput OK, format: %s, size: (%u, %u), bitrate: %u", ((format == woogeen_base::FRAME_FORMAT_VP8)? "VP8" : "H264"), rootSize.width, rootSize.height, bitrate);
    boost::shared_ptr<HardwareVideoFrameMixerOutput> newOutput(new HardwareVideoFrameMixerOutput(m_engine, format, size, 30, bitrate, dest));

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_outputs[output] = newOutput;
    return true;
}

void HardwareVideoFrameMixer::removeOutput(int32_t output)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_outputs.erase(output);
    }
}

}
