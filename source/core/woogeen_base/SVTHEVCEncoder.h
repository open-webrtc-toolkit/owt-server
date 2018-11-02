/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

    bool convert2BufferHeader(const Frame& frame, EB_BUFFERHEADERTYPE *bufferHeader);

    void fillPacketDone(EB_BUFFERHEADERTYPE* pBufferHeader);

    void dump(uint8_t *buf, int len);

private:
    bool                        m_ready;
    FrameDestination            *m_dest;

    EB_COMPONENTTYPE            *m_handle;
    EB_H265_ENC_CONFIGURATION   m_encParameters;

    std::vector<EB_BUFFERHEADERTYPE> m_inputBufferPool;
    std::queue<EB_BUFFERHEADERTYPE *> m_freeInputBuffers;
    std::vector<EB_BUFFERHEADERTYPE> m_streamBufferPool;

    bool m_forceIDR;
    uint32_t m_frameCount;

    bool m_enableBsDump;
    FILE *m_bsDumpfp;
};

} /* namespace woogeen_base */
#endif /* SVTHEVCEncoder_h */
