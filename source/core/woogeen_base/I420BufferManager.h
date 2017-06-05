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

    void putBusyFrame(boost::shared_ptr<webrtc::VideoFrame> frame);
    boost::shared_ptr<webrtc::VideoFrame> getBusyFrame();

private:
    boost::scoped_ptr<webrtc::I420BufferPool> m_bufferPool;

    boost::shared_ptr<webrtc::VideoFrame> m_busyFrame;
    boost::mutex m_mutex;
};

}
#endif /* I420BufferManager_h */
