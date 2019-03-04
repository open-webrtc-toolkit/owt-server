// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SVTHEVCEncoder_h
#define SVTHEVCEncoder_h

#include <queue>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "logger.h"
#include "MediaFramePipeline.h"

#include "EbApi.h"

namespace woogeen_base {

class SVTHEVCEncoder : public VideoFrameEncoder {
    DECLARE_LOGGER();

public:
    SVTHEVCEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast = false);
    ~SVTHEVCEncoder();

    FrameFormat getInputFormat() {return FRAME_FORMAT_I420;}

    // Implements VideoFrameEncoder.
    void onFrame(const Frame&);
    bool canSimulcast(FrameFormat format, uint32_t width, uint32_t height);
    bool isIdle();
    int32_t generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, FrameDestination* dest);
    void degenerateStream(int32_t streamId);
    void setBitrate(unsigned short kbps, int32_t streamId);
    void requestKeyFrame(int32_t streamId);

protected:
    void initDefaultParameters();

    void updateParameters(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds);

    bool allocateBuffers();
    void deallocateBuffers();

    bool initEncoder(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds);

    bool convert2BufferHeader(const Frame& frame, EB_BUFFERHEADERTYPE *bufferHeader);

    void fillPacketDone(EB_BUFFERHEADERTYPE* pBufferHeader);

    void dump(uint8_t *buf, int len);

private:
    bool                        m_encoderReady;
    FrameDestination            *m_dest;

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_frameRate;
    uint32_t m_bitrateKbps;
    uint32_t m_keyFrameIntervalSeconds;

    EB_COMPONENTTYPE            *m_handle;
    EB_H265_ENC_CONFIGURATION   m_encParameters;

    std::vector<EB_BUFFERHEADERTYPE> m_inputBufferPool;
    std::queue<EB_BUFFERHEADERTYPE *> m_freeInputBuffers;
    std::vector<EB_BUFFERHEADERTYPE> m_streamBufferPool;

    bool m_forceIDR;
    uint32_t m_frameCount;

    boost::shared_mutex m_mutex;

    bool m_enableBsDump;
    FILE *m_bsDumpfp;
};

} /* namespace woogeen_base */
#endif /* SVTHEVCEncoder_h */
