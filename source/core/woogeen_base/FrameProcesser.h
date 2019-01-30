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

#ifndef FrameProcesser_h
#define FrameProcesser_h

#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>

#include <webrtc/api/video/video_frame.h>
#include <webrtc/system_wrappers/include/clock.h>

#include <JobTimer.h>
#include "MediaFramePipeline.h"

#ifdef ENABLE_MSDK
#include "MsdkFrame.h"
#include "MsdkBase.h"
#endif

#include "I420BufferManager.h"

#include "FrameConverter.h"
#include "FFmpegDrawText.h"

namespace woogeen_base {

class FrameProcesser : public VideoFrameProcesser, public JobTimerListener {
    DECLARE_LOGGER();

    const uint32_t kMsToRtpTimestamp = 90;

public:
    FrameProcesser();
    ~FrameProcesser();

    void onFrame(const Frame&);
    bool init(FrameFormat format, const uint32_t width, const uint32_t height, const uint32_t frameRate);

    void onTimeout();

    void drawText(const std::string& textSpec);
    void clearText();

protected:
    bool filterFrame(const Frame& frame);
#ifdef ENABLE_MSDK
    boost::shared_ptr<woogeen_base::MsdkFrame> getMsdkFrame(const uint32_t width, const uint32_t height);

    void SendFrame(boost::shared_ptr<woogeen_base::MsdkFrame> msdkFrame, uint32_t timeStamp);
#endif
    void SendFrame(rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer, uint32_t timeStamp);

private:
    uint32_t m_lastWidth;
    uint32_t m_lastHeight;

    FrameFormat m_format;
    uint32_t m_outWidth;
    uint32_t m_outHeight;
    uint32_t m_outFrameRate;

#ifdef ENABLE_MSDK
    boost::shared_ptr<mfxFrameAllocator> m_allocator;
    std::vector<boost::shared_ptr<woogeen_base::MsdkFrame>> m_framePool;
    boost::shared_ptr<woogeen_base::MsdkFrame> m_activeMsdkFrame;
#endif

    boost::scoped_ptr<I420BufferManager> m_bufferManager;
    rtc::scoped_refptr<webrtc::I420Buffer> m_activeI420Buffer;

    boost::shared_mutex m_mutex;

    const webrtc::Clock *m_clock;
    boost::scoped_ptr<FrameConverter> m_converter;
    boost::scoped_ptr<JobTimer> m_jobTimer;

    boost::shared_ptr<woogeen_base::FFmpegDrawText> m_textDrawer;
};

} /* namespace woogeen_base */

#endif /* FrameProcesser_h */

