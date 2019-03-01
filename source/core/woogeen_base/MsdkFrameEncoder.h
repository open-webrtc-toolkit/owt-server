// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MsdkFrameEncoder_h
#define MsdkFrameEncoder_h

#ifdef ENABLE_MSDK

#include <boost/thread.hpp>
#include <logger.h>

#include "MediaFramePipeline.h"

#include "MsdkFrame.h"

namespace woogeen_base {

class StreamEncoder;
/**
 * This is the class to accept the raw frame and encode it to the given format.
 */
class MsdkFrameEncoder : public VideoFrameEncoder {
    DECLARE_LOGGER();

public:
    MsdkFrameEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast = false);
    ~MsdkFrameEncoder();

    FrameFormat getInputFormat() {return FRAME_FORMAT_MSDK;}

    static bool supportFormat(FrameFormat format);

    // Implements VideoFrameEncoder.
    int32_t generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, FrameDestination* dest);
    void degenerateStream(int32_t streamId);
    bool canSimulcast(FrameFormat format, uint32_t width, uint32_t height);
    bool isIdle();
    void onFrame(const Frame&);
    void setBitrate(unsigned short kbps, int id = 0);
    void requestKeyFrame(int id = 0);

private:
    FrameFormat m_encodeFormat;
    VideoCodecProfile m_profile;
    std::map<int, boost::shared_ptr<StreamEncoder> > m_streams;
    boost::shared_mutex m_mutex;
    bool m_useSimulcast;
    int m_id;
};

} /* namespace woogeen_base */
#endif /* ENABLE_MSDK */
#endif /* MsdkFrameEncoder_h */

