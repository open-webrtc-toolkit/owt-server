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

#ifndef SoftVideoFrameMixer_h
#define SoftVideoFrameMixer_h

#include "FakedVideoFrameEncoder.h"
#include "I420VideoFrameDecoder.h"
#include "SoftVideoCompositor.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <map>
#include <MediaFramePipeline.h>
#include <VCMFrameEncoder.h>

namespace mcu {

class CompositedFrameDispatcher;

class SoftVideoFrameMixer : public VideoFrameMixer {
    friend class CompositedFrameDispatcher;

public:
    SoftVideoFrameMixer(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, boost::shared_ptr<woogeen_base::WebRTCTaskRunner>);
    ~SoftVideoFrameMixer();

    bool activateInput(int input, woogeen_base::FrameFormat, woogeen_base::VideoFrameProvider*);
    void deActivateInput(int input);
    void pushInput(int input, unsigned char* payload, int len);

    void setBitrate(unsigned short kbps, int id);
    void requestKeyFrame(int id);
    bool activateOutput(int id, woogeen_base::FrameFormat, unsigned int framerate, unsigned short kbps, woogeen_base::VideoFrameConsumer*);
    void deActivateOutput(int id);

    void updateRootSize(VideoSize& rootSize);
    void updateBackgroundColor(YUVColor& bgColor);
    void updateLayoutSolution(LayoutSolution& solution);

private:
    boost::shared_ptr<VideoFrameCompositor> m_compositor;
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameDecoder>> m_decoders;
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameEncoder>> m_encoders;
    boost::shared_mutex m_decoderMutex;
    boost::shared_mutex m_encoderMutex;
    boost::shared_ptr<woogeen_base::WebRTCTaskRunner> m_taskRunner;
    boost::scoped_ptr<CompositedFrameDispatcher> m_dispatcher;
};

class CompositedFrameDispatcher : public woogeen_base::VideoFrameConsumer {
public:
    CompositedFrameDispatcher(SoftVideoFrameMixer* mixer);
    ~CompositedFrameDispatcher();

    void onFrame(const woogeen_base::Frame&);

private:
    SoftVideoFrameMixer* m_mixer;
};

SoftVideoFrameMixer::SoftVideoFrameMixer(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, boost::shared_ptr<woogeen_base::WebRTCTaskRunner> taskRunner)
    : m_taskRunner(taskRunner)
{
    m_compositor.reset(new SoftVideoCompositor(maxInput, rootSize, bgColor));
    m_dispatcher.reset(new CompositedFrameDispatcher(this));
}

SoftVideoFrameMixer::~SoftVideoFrameMixer()
{
    {
        boost::unique_lock<boost::shared_mutex> lock(m_decoderMutex);
        m_decoders.clear();
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(m_encoderMutex);
        m_encoders.clear();
    }
}

inline bool SoftVideoFrameMixer::activateInput(int input, woogeen_base::FrameFormat format, woogeen_base::VideoFrameProvider* provider)
{
    assert(format == woogeen_base::FRAME_FORMAT_I420);
    boost::upgrade_lock<boost::shared_mutex> lock(m_decoderMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameDecoder>>::iterator it = m_decoders.find(input);
    if (it != m_decoders.end())
        return false;

    I420VideoFrameDecoder* decoder = new I420VideoFrameDecoder(input, m_compositor);
    decoder->setInput(format, provider);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_decoders[input].reset(decoder);
    return true;
}

inline void SoftVideoFrameMixer::deActivateInput(int input)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_decoderMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameDecoder>>::iterator it = m_decoders.find(input);
    if (it != m_decoders.end()) {
        it->second->unsetInput();
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_decoders.erase(it);
    }
}

inline void SoftVideoFrameMixer::pushInput(int input, unsigned char* payload, int len)
{
    boost::shared_lock<boost::shared_mutex> lock(m_decoderMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameDecoder>>::iterator it = m_decoders.find(input);
    if (it != m_decoders.end()) {
        woogeen_base::Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = woogeen_base::FRAME_FORMAT_I420;
        frame.payload = payload;
        frame.length = len;
        frame.timeStamp = 0; // unused.

        it->second->onFrame(frame);
    }
}

inline void SoftVideoFrameMixer::updateRootSize(VideoSize& rootSize)
{
    m_compositor->updateRootSize(rootSize);
}

inline void SoftVideoFrameMixer::updateBackgroundColor(YUVColor& bgColor)
{
    m_compositor->updateBackgroundColor(bgColor);
}

inline void SoftVideoFrameMixer::updateLayoutSolution(LayoutSolution& solution)
{
    m_compositor->updateLayoutSolution(solution);
}

inline void SoftVideoFrameMixer::setBitrate(unsigned short kbps, int id)
{
    boost::shared_lock<boost::shared_mutex> lock(m_encoderMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameEncoder>>::iterator it = m_encoders.find(id);
    if (it != m_encoders.end())
        it->second->setBitrate(kbps, id);
}

inline void SoftVideoFrameMixer::requestKeyFrame(int id)
{
    boost::shared_lock<boost::shared_mutex> lock(m_encoderMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameEncoder>>::iterator it = m_encoders.find(id);
    if (it != m_encoders.end())
        it->second->requestKeyFrame(id);
}

inline bool SoftVideoFrameMixer::activateOutput(int id, woogeen_base::FrameFormat format, unsigned int framerate, unsigned short kbps, woogeen_base::VideoFrameConsumer* consumer)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_encoderMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameEncoder>>::iterator it = m_encoders.find(id);
    if (it != m_encoders.end())
        return false;

    woogeen_base::VideoFrameEncoder* encoder = nullptr;
    if (consumer->acceptRawFrame())
        encoder = new FakedVideoFrameEncoder();
    else
        encoder = new woogeen_base::VCMFrameEncoder(m_taskRunner);

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_encoders[id].reset(encoder);
    return encoder->activateOutput(id, format, framerate, kbps, consumer);
}

inline void SoftVideoFrameMixer::deActivateOutput(int id)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_encoderMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameEncoder>>::iterator it = m_encoders.find(id);
    if (it != m_encoders.end()) {
        it->second->deActivateOutput(id);
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_encoders.erase(it);
    }
}

CompositedFrameDispatcher::CompositedFrameDispatcher(SoftVideoFrameMixer* mixer)
    : m_mixer(mixer)
{
    m_mixer->m_compositor->setOutput(this);
}

CompositedFrameDispatcher::~CompositedFrameDispatcher()
{
    m_mixer->m_compositor->unsetOutput();
}

inline void CompositedFrameDispatcher::onFrame(const woogeen_base::Frame& frame)
{
    assert(frame.format == woogeen_base::FRAME_FORMAT_I420);
    boost::shared_lock<boost::shared_mutex> lock(m_mixer->m_encoderMutex);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameEncoder>>::iterator it = m_mixer->m_encoders.begin();
    for (; it != m_mixer->m_encoders.end(); ++it)
        it->second->onFrame(frame);
}

}
#endif
