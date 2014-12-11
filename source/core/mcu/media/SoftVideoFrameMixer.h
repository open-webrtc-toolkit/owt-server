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
    SoftVideoFrameMixer();
    ~SoftVideoFrameMixer();

    bool activateInput(int slot, FrameFormat, VideoFrameProvider*);
    void deActivateInput(int slot);
    void pushInput(int slot, unsigned char* payload, int len);

    void setLayout(const VideoLayout&);

    void setBitrate(int id, unsigned short bitrate);
    void requestKeyFrame(int id);
    bool activateOutput(int id, FrameFormat, unsigned int framerate, unsigned short bitrate, VideoFrameConsumer*);
    void deActivateOutput(int id);

private:
    std::map<int, boost::shared_ptr<VideoFrameDecoder>> m_decoders;
    boost::shared_ptr<VideoFrameCompositor> m_compositor;
    boost::scoped_ptr<VideoFrameEncoder> m_encoder;
};

SoftVideoFrameMixer::SoftVideoFrameMixer()
{
    const VideoLayout& layout = Config::get()->getVideoLayout();

    m_compositor.reset(new SoftVideoCompositor(layout));
    m_encoder.reset(new FakedVideoFrameEncoder(m_compositor));
}

SoftVideoFrameMixer::~SoftVideoFrameMixer()
{
    m_decoders.clear();
}

inline bool SoftVideoFrameMixer::activateInput(int slot, FrameFormat format, VideoFrameProvider*)
{
    assert(format == FRAME_FORMAT_I420);
    std::map<int, boost::shared_ptr<VideoFrameDecoder>>::iterator it = m_decoders.find(slot);
    if (it != m_decoders.end())
        return false;

    m_decoders[slot].reset(new I420VideoFrameDecoder(slot, m_compositor));
    return true;
}

inline void SoftVideoFrameMixer::deActivateInput(int slot)
{
    std::map<int, boost::shared_ptr<VideoFrameDecoder>>::iterator it = m_decoders.find(slot);
    if (it != m_decoders.end())
        m_decoders.erase(it);
}

inline void SoftVideoFrameMixer::pushInput(int slot, unsigned char* payload, int len)
{
    std::map<int, boost::shared_ptr<VideoFrameDecoder>>::iterator it = m_decoders.find(slot);
    if (it != m_decoders.end())
        it->second->onFrame(FRAME_FORMAT_I420, payload, len, 0);
}

inline void SoftVideoFrameMixer::setLayout(const VideoLayout& layout)
{
    m_compositor->setLayout(layout);
}

inline void SoftVideoFrameMixer::setBitrate(int id, unsigned short bitrate)
{
    m_encoder->setBitrate(id, bitrate);
}

inline void SoftVideoFrameMixer::requestKeyFrame(int id)
{
    m_encoder->requestKeyFrame(id);
}

inline bool SoftVideoFrameMixer::activateOutput(int id, FrameFormat format, unsigned int framerate, unsigned short bitrate, VideoFrameConsumer* consumer)
{
    return m_encoder->activateOutput(id, format, framerate, bitrate, consumer);
}

inline void SoftVideoFrameMixer::deActivateOutput(int id)
{
    m_encoder->deActivateOutput(id);
}

}
#endif
