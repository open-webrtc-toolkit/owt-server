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

#ifndef VCMFrameEncoderAdapter_h
#define VCMFrameEncoderAdapter_h

#include "MediaFramePipeline.h"
#include "WebRTCTaskRunner.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>

#include "VCMFrameEncoder.h"

namespace woogeen_base {

class VCMFrameEncoderAdapter : public VideoFrameEncoder {
    DECLARE_LOGGER();

public:
    VCMFrameEncoderAdapter(FrameFormat format, boost::shared_ptr<WebRTCTaskRunner>);
    ~VCMFrameEncoderAdapter();

    FrameFormat getInputFormat() {return FRAME_FORMAT_I420;}

    // Implements VideoFrameEncoder.
    void onFrame(const Frame&);
    bool canSimulcast(FrameFormat format, uint32_t width, uint32_t height);
    bool isIdle();
    int32_t generateStream(uint32_t width, uint32_t height, uint32_t bitrateKbps, FrameDestination* dest);
    void degenerateStream(int32_t streamId);
    void setBitrate(unsigned short kbps, int32_t streamId);
    void requestKeyFrame(int32_t streamId);

private:
    boost::scoped_ptr<VCMFrameEncoder> m_encoder;
    int32_t m_streamId;
    int32_t m_encoderStreamId;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_kbps;
    FrameDestination *m_dest;
};

} /* namespace woogeen_base */
#endif /* VCMFrameEncoderAdapter_h */
