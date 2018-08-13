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

#ifndef MsdkFrameEncoder_h
#define MsdkFrameEncoder_h

#ifdef ENABLE_MSDK

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
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

    static bool supportFormat(FrameFormat format) {return (format == FRAME_FORMAT_H264 || format == FRAME_FORMAT_H265);}

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

