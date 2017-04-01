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

#ifndef MsdkFrameProcesser_h
#define MsdkFrameProcesser_h

#ifdef ENABLE_MSDK

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>

#include <webrtc/common_video/interface/i420_video_frame.h>

#include "MediaFramePipeline.h"

#include "MsdkFrame.h"
#include "MsdkBase.h"

namespace woogeen_base {

class MsdkFrameProcesser : public VideoFrameProcesser {
    DECLARE_LOGGER();

public:
    MsdkFrameProcesser();
    ~MsdkFrameProcesser();

    void onFrame(const Frame&);
    bool init(FrameFormat format);

protected:

private:
    uint32_t m_lastWidth;
    uint32_t m_lastHeight;
    FrameFormat m_format;

    boost::shared_ptr<mfxFrameAllocator> m_allocator;
    boost::shared_ptr<woogeen_base::MsdkFrame> m_msdkFrame;

    boost::shared_ptr<webrtc::I420VideoFrame> m_i420Frame;
};

} /* namespace woogeen_base */

#endif /* ENABLE_MSDK */
#endif /* MsdkFrameProcesser_h */

