/*
 * Copyright 2015 Intel Corporation All Rights Reserved.
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

#ifndef VideoFrameTranscoder_h
#define VideoFrameTranscoder_h

#include "VCMFrameDecoder.h"
#include "VCMFrameEncoder.h"
#include "WebRTCTaskRunner.h"

#include <boost/shared_ptr.hpp>

namespace woogeen_base {

class VideoFrameTranscoder : public VideoFrameConsumer,
                             public VideoFrameProvider {
public:
    VideoFrameTranscoder(boost::shared_ptr<WebRTCTaskRunner>);
    ~VideoFrameTranscoder();

    bool setInput(FrameFormat, VideoFrameProvider*);
    void unsetInput();

    // Implements VideoFrameConsumer.
    void onFrame(const Frame&);
    bool acceptRawFrame() { return false; }

    // Implements VideoFrameProvider.
    int32_t addFrameConsumer(const std::string& name, FrameFormat, FrameConsumer*);
    void removeFrameConsumer(int32_t id);
    void setBitrate(unsigned short kbps, int id = 0);
    void requestKeyFrame(int id = 0);

private:
    boost::shared_ptr<VideoFrameEncoder> m_encoder;
    boost::shared_ptr<VideoFrameDecoder> m_decoder;
};

inline VideoFrameTranscoder::VideoFrameTranscoder(boost::shared_ptr<WebRTCTaskRunner> taskRunner)
{
    m_encoder.reset(new VCMFrameEncoder(taskRunner));
    m_decoder.reset(new VCMFrameDecoder(m_encoder));
}

inline VideoFrameTranscoder::~VideoFrameTranscoder()
{
}

inline bool VideoFrameTranscoder::setInput(FrameFormat format, VideoFrameProvider* provider)
{
    return m_decoder->setInput(format, provider);
}

inline void VideoFrameTranscoder::unsetInput()
{
    m_decoder->unsetInput();
}

inline void VideoFrameTranscoder::onFrame(const Frame& frame)
{
    m_decoder->onFrame(frame);
}

inline void VideoFrameTranscoder::setBitrate(unsigned short kbps, int id)
{
    m_encoder->setBitrate(kbps, id);
}

inline void VideoFrameTranscoder::requestKeyFrame(int id)
{
    m_encoder->requestKeyFrame(id);
}

inline int32_t VideoFrameTranscoder::addFrameConsumer(const std::string& name, FrameFormat format, VideoFrameConsumer* consumer)
{
    return m_encoder->addFrameConsumer(name, format, consumer);
}

inline void VideoFrameTranscoder::removeFrameConsumer(int32_t id)
{
    m_encoder->removeFrameConsumer(id);
}

} /* namespace woogeen_base */

#endif /* VideoFrameTranscoder_h */
