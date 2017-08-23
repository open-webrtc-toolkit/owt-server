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

#ifndef FrameConverter_h
#define FrameConverter_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <webrtc/api/video/i420_buffer.h>

#include <logger.h>

#include "I420BufferManager.h"

#ifdef ENABLE_MSDK
#include "MsdkBase.h"
#include "MsdkFrame.h"
#include "MsdkScaler.h"
#endif

namespace woogeen_base {

class FrameConverter {
    DECLARE_LOGGER();

public:
    FrameConverter(bool useMsdkVpp = true);
    ~FrameConverter();

#ifdef ENABLE_MSDK
    bool convert(MsdkFrame *srcMsdkFrame, webrtc::I420Buffer *dstI420Buffer);
    bool convert(MsdkFrame *srcMsdkFrame, MsdkFrame *dstMsdkFrame);

    bool convert(webrtc::VideoFrameBuffer *srcBuffer, MsdkFrame *dstMsdkFrame);
#endif
    bool convert(webrtc::VideoFrameBuffer *srcBuffer, webrtc::I420Buffer *dstI420Buffer);

protected:

private:
#ifdef ENABLE_MSDK
    boost::scoped_ptr<MsdkScaler> m_scaler;
#endif

    boost::scoped_ptr<I420BufferManager> m_bufferManager;
};

} /* namespace woogeen_base */

#endif /* FrameConverter_h */

