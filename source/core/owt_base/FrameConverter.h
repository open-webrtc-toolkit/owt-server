// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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

namespace owt_base {

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

} /* namespace owt_base */

#endif /* FrameConverter_h */

