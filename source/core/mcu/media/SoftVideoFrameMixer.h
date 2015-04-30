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
#include "VideoFramePipeline.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <map>

namespace mcu {

class SoftVideoFrameMixer : public VideoFrameMixer {
public:
    SoftVideoFrameMixer(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor);
    ~SoftVideoFrameMixer();

    bool activateInput(int input, woogeen_base::FrameFormat, woogeen_base::VideoFrameProvider*);
    void deActivateInput(int input);
    void pushInput(int input, unsigned char* payload, int len);

    void setBitrate(unsigned short bitrate, int id);
    void requestKeyFrame(int id);
    bool activateOutput(int id, woogeen_base::FrameFormat, unsigned int framerate, unsigned short bitrate, woogeen_base::VideoFrameConsumer*);
    void deActivateOutput(int id);

    void updateRootSize(VideoSize& rootSize);
    void updateBackgroundColor(YUVColor& bgColor);
    void updateLayoutSolution(LayoutSolution& solution);

private:
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameDecoder>> m_decoders;
    boost::shared_ptr<VideoFrameCompositor> m_compositor;
    boost::scoped_ptr<FakedVideoFrameEncoder> m_encoder;
};

SoftVideoFrameMixer::SoftVideoFrameMixer(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor)
{
    m_compositor.reset(new SoftVideoCompositor(maxInput, rootSize, bgColor));
    m_encoder.reset(new FakedVideoFrameEncoder(m_compositor));
}

SoftVideoFrameMixer::~SoftVideoFrameMixer()
{
    m_decoders.clear();
}

inline bool SoftVideoFrameMixer::activateInput(int input, woogeen_base::FrameFormat format, woogeen_base::VideoFrameProvider* provider)
{
    assert(format == woogeen_base::FRAME_FORMAT_I420);
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameDecoder>>::iterator it = m_decoders.find(input);
    if (it != m_decoders.end())
        return false;

    I420VideoFrameDecoder* decoder = new I420VideoFrameDecoder(input, m_compositor);
    decoder->setInput(format, provider);
    m_decoders[input].reset(decoder);
    return true;
}

inline void SoftVideoFrameMixer::deActivateInput(int input)
{
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameDecoder>>::iterator it = m_decoders.find(input);
    if (it != m_decoders.end()) {
        it->second->unsetInput();
        m_decoders.erase(it);
    }
}

inline void SoftVideoFrameMixer::pushInput(int input, unsigned char* payload, int len)
{
    std::map<int, boost::shared_ptr<woogeen_base::VideoFrameDecoder>>::iterator it = m_decoders.find(input);
    if (it != m_decoders.end())
        it->second->onFrame(woogeen_base::FRAME_FORMAT_I420, payload, len, 0);
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

inline void SoftVideoFrameMixer::setBitrate(unsigned short bitrate, int id)
{
    m_encoder->setBitrate(bitrate, id);
}

inline void SoftVideoFrameMixer::requestKeyFrame(int id)
{
    m_encoder->requestKeyFrame(id);
}

inline bool SoftVideoFrameMixer::activateOutput(int id, woogeen_base::FrameFormat format, unsigned int framerate, unsigned short bitrate, woogeen_base::VideoFrameConsumer* consumer)
{
    return m_encoder->activateOutput(id, format, framerate, bitrate, consumer);
}

inline void SoftVideoFrameMixer::deActivateOutput(int id)
{
    m_encoder->deActivateOutput(id);
}

}
#endif
