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

#include "I420VideoFrameDecoder.h"
#include "SoftVideoCompositor.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <map>
#include <MediaFramePipeline.h>
#include <VCMFrameDecoder.h>
#include <VCMFrameEncoder.h>

namespace mcu {

class CompositeIn : public woogeen_base::FrameDestination
{
public:
    CompositeIn(int index, boost::shared_ptr<VideoFrameCompositor> compositor) : m_index(index), m_compositor(compositor) {
        m_compositor->activateInput(m_index);
    }

    virtual ~CompositeIn() {
        m_compositor->deActivateInput(m_index);
    }

    void onFrame(const woogeen_base::Frame& frame) {
        assert(frame.format == woogeen_base::FRAME_FORMAT_I420);
        webrtc::I420VideoFrame* i420Frame = reinterpret_cast<webrtc::I420VideoFrame*>(frame.payload);
        m_compositor->pushInput(m_index, i420Frame);
    }

private:
    int m_index;
    boost::shared_ptr<VideoFrameCompositor> m_compositor;
};

class SoftVideoFrameMixer : public VideoFrameMixer {
public:
    SoftVideoFrameMixer(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, boost::shared_ptr<woogeen_base::WebRTCTaskRunner>, bool useSimulcast, bool crop);
    ~SoftVideoFrameMixer();

    bool addInput(int input, woogeen_base::FrameFormat, woogeen_base::FrameSource*);
    void removeInput(int input);

    bool addOutput(int output, woogeen_base::FrameFormat, const VideoSize&, woogeen_base::FrameDestination*);
    void removeOutput(int output);
    void setBitrate(unsigned short kbps, int output);
    void requestKeyFrame(int output);

    void updateLayoutSolution(LayoutSolution& solution);

private:
    struct Input {
        woogeen_base::FrameSource* source;
        boost::shared_ptr<woogeen_base::VideoFrameDecoder> decoder;
        boost::shared_ptr<CompositeIn> compositorIn;
    };

    struct Output {
        boost::shared_ptr<woogeen_base::VideoFrameEncoder> encoder;
        int streamId;
    };

    std::map<int, Input> m_inputs;
    boost::shared_mutex m_inputMutex;

    boost::shared_ptr<VideoFrameCompositor> m_compositor;

    std::map<int, Output> m_outputs;
    boost::shared_mutex m_outputMutex;

    boost::shared_ptr<woogeen_base::WebRTCTaskRunner> m_taskRunner;
    bool m_useSimulcast;
};

SoftVideoFrameMixer::SoftVideoFrameMixer(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, boost::shared_ptr<woogeen_base::WebRTCTaskRunner> taskRunner, bool useSimulcast, bool crop)
    : m_taskRunner(taskRunner)
    , m_useSimulcast(useSimulcast)
{
    m_compositor.reset(new SoftVideoCompositor(maxInput, rootSize, bgColor, crop));
}

SoftVideoFrameMixer::~SoftVideoFrameMixer()
{
    {
        boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
        for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it) {
            it->second.encoder->degenerateStream(it->second.streamId);
            m_compositor->removeVideoDestination(it->second.encoder.get());
        }
        m_outputs.clear();
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(m_inputMutex);
        for (auto it = m_inputs.begin(); it != m_inputs.end(); ++it) {
            it->second.source->removeVideoDestination(it->second.decoder.get());
            m_inputs.erase(it);
        }
        m_inputs.clear();
    }

    m_compositor.reset();
}

inline bool SoftVideoFrameMixer::addInput(int input, woogeen_base::FrameFormat format, woogeen_base::FrameSource* source)
{
    assert(source);

    boost::upgrade_lock<boost::shared_mutex> lock(m_inputMutex);
    auto it = m_inputs.find(input);
    if (it != m_inputs.end())
        return false;

    boost::shared_ptr<woogeen_base::VideoFrameDecoder> decoder;

    if (format == woogeen_base::FRAME_FORMAT_I420) {
        decoder.reset(new I420VideoFrameDecoder());
    } else {
        decoder.reset(new woogeen_base::VCMFrameDecoder(format));
    }

    if (decoder->init(format)) {
        boost::shared_ptr<CompositeIn> compositorIn(new CompositeIn(input, m_compositor));
    
        source->addVideoDestination(decoder.get());
        decoder->addVideoDestination(compositorIn.get());

        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        Input in{.source = source, .decoder = decoder, .compositorIn = compositorIn};
        m_inputs[input] = in;
        return true;
    } else {
        return false;
    }
}

inline void SoftVideoFrameMixer::removeInput(int input)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_inputMutex);
    auto it = m_inputs.find(input);
    if (it != m_inputs.end()) {
        it->second.source->removeVideoDestination(it->second.decoder.get());
        it->second.decoder->removeVideoDestination(it->second.compositorIn.get());
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_inputs.erase(it);
    }
}

inline void SoftVideoFrameMixer::updateLayoutSolution(LayoutSolution& solution)
{
    m_compositor->updateLayoutSolution(solution);
}

inline void SoftVideoFrameMixer::setBitrate(unsigned short kbps, int output)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end()) {
        it->second.encoder->setBitrate(kbps, it->second.streamId);
    }
}

inline void SoftVideoFrameMixer::requestKeyFrame(int output)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end()) {
        it->second.encoder->requestKeyFrame(it->second.streamId);
    }
}

inline bool SoftVideoFrameMixer::addOutput(int output,
                                           woogeen_base::FrameFormat format,
                                           const VideoSize& rootSize,
                                           woogeen_base::FrameDestination* dest)
{
    boost::shared_ptr<woogeen_base::VideoFrameEncoder> encoder;
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);

    // find a reusable encoder.
    auto it = m_outputs.begin();
    for (; it != m_outputs.end(); ++it) {
        if (it->second.encoder->canSimulcastFor(format, rootSize.width, rootSize.height)) {
            break;
        }
    }

    int32_t streamId = -1;
    if (it != m_outputs.end()) { //Found a reusable encoder
        encoder = it->second.encoder;
        streamId = encoder->generateStream(rootSize.width, rootSize.height, dest);
        if (streamId < 0){
            return false;
        }
    } else { //Never found a reusable encoder.
        encoder.reset(new woogeen_base::VCMFrameEncoder(format, m_taskRunner, m_useSimulcast));
        streamId = encoder->generateStream(rootSize.width, rootSize.height, dest);
        if (streamId < 0){
            return false;
        }
        m_compositor->addVideoDestination(encoder.get());
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    Output out{.encoder = encoder, .streamId = streamId};
    m_outputs[output] = out;
    return true;
}

inline void SoftVideoFrameMixer::removeOutput(int32_t output)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end()) {
        it->second.encoder->degenerateStream(it->second.streamId);
        if (it->second.encoder->isIdle()) {
            m_compositor->removeVideoDestination(it->second.encoder.get());
        }
        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
        m_outputs.erase(output);
    }
}

}
#endif
