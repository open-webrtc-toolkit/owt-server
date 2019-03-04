// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef I420BufferManager_h
#define I420BufferManager_h

#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <webrtc/common_video/include/i420_buffer_pool.h>
#include <webrtc/api/video/video_frame.h>

#include "logger.h"

namespace woogeen_base {

class I420BufferManager {
    DECLARE_LOGGER();

public:
    I420BufferManager(uint32_t maxFrames);
    ~I420BufferManager();

    rtc::scoped_refptr<webrtc::I420Buffer> getFreeBuffer(uint32_t width, uint32_t height);
private:
    boost::scoped_ptr<webrtc::I420BufferPool> m_bufferPool;
};

}
#endif /* I420BufferManager_h */
