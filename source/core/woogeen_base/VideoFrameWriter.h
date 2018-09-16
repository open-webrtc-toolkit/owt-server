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

#ifndef VideoFrameWriter_h
#define VideoFrameWriter_h

#include <string>

#include <logger.h>

#include <webrtc/api/video/video_frame.h>
#include <webrtc/api/video/video_frame_buffer.h>

#include "MediaFramePipeline.h"

#ifdef ENABLE_MSDK
#include "I420BufferManager.h"
#endif

namespace woogeen_base {

class VideoFrameWriter {
    DECLARE_LOGGER();

public:
    VideoFrameWriter(const std::string& name);
    ~VideoFrameWriter();

    void write(const Frame& frame);

protected:
    FILE *getVideoFp(webrtc::VideoFrameBuffer *videoFrameBuffer);
    void write(webrtc::VideoFrameBuffer *videoFrameBuffer);

private:
    std::string m_name;

    FILE *m_fp;
    uint32_t m_index;

    int32_t m_width;
    int32_t m_height;

#ifdef ENABLE_MSDK
    boost::scoped_ptr<I420BufferManager> m_bufferManager;
#endif
};

}

#endif // VideoFrameWriter_h
